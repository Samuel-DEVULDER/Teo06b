/*

	MARCEL O CINQ
	*************

	l'émulateur de thomson MO5

	Par Gilles FETIS

	mail : FETIS_Gilles@compuserve.com
	page : http://ourworld.compuserve.com/homepages/FETIS_Gilles


*/


/*

	VERSION     : 2.0
	MODULE      : MC6809_2.C
	Créé par    : Gilles FETIS
	Modifié par :

	Module émulation du 6809 (Part II)
	Version avec décompte des cycles d'horloge

*/

#include "flags.h" /* sam: ou sinon pb avec REGPARM dans fdraw() */

#include <stdlib.h>
#include <stdio.h>
#include "mapper.h"
#include "index.h"
#include "regs.h"
#include "mc6809.h"
#include "graph.h"

/* Mise à jour de D */
#define CALCD D=(A<<8)|B
#define CALCAB B=D&0xFF;A=D>>8

/* Lecture de l'instruction/operandes */
#define FETCH m=LOAD8(PC);PC++
#define FETCHW m=LOAD8(PC);m<<=8;++PC;m|=LOAD8(PC);++PC

/* divers modes d'adressage retourne adr dans M */
#define IMMED8 M=PC;PC++
#define IMMED16 M=PC;PC+=2

#define SETABF(val) /*D=val;*/B=val&0xFF;val>>=8;ovfl=sign=val;A=val&0xFF;res=val|B

#ifdef REG_M
#ifndef __GNUC__
#define LD_16(M,m) M=LOAD8(m);M<<=8;++m;M|=LOAD8(m)
#define DIREC M=DP;M<<=8;M|=LOAD8(PC);PC++;
#define ETEND M=LOAD8(PC);M<<=8;++PC;M|=LOAD8(PC);++PC
#else
#ifdef NO_PROTECT
#define LD_16(M,m) M=*(unsigned short*)&LOAD8(m);++m;__asm__ __volatile__("moveb %1,%0" : "=&d" (M) : "dmi" (LOAD8(m)), "0" (M))
#else
#define LD_16(M,m) M=LOAD8(m);M<<=8;++m;M|=LOAD8(m)
#endif
#define DIREC __asm__ __volatile__("movew _DP+3,"REG_M"; moveb %0,"REG_M : : "dmi" (LOAD8(PC)));++PC;
#define ETEND LD_16(M,PC);++PC
#undef FETCHW
#define FETCHW LD_16(m,PC);++PC
#undef SETABF
#define SETABF(val) __asm__ __volatile__("moveb %0,_B+3; lsrl #8,%0; moveb %0,_A+3; moveb %0,_ovfl+3; moveb %0,_sign+3; orb _B+3,%0; movew %0,_res+2" : : "d" (val))
#endif
#else
#define LD_16(M,m) M=(LOAD8(m)<<8)|LOAD8(m+1);++m;
#define DIREC M=(DP<<8)|LOAD8(PC);PC++;
#define ETEND M=(LOAD8(PC)<<8)|LOAD8(PC+1);PC+=2
#endif

#define INDEXE {int m=LOAD8(PC);PC++;(*(*INDEX[m]))();}

/* les adresses LOAD8 et STORE8 sont protégées contre >0xFFFF */
/* seul STORE8 est protégé contre le >255 */
#define LOAD16 ((LOAD8(M)<<8)|LOAD8(M+1))
#define STORE16(Z) STORE8(M,(Z)>>8);STORE8(M+1,(Z))

#define mem16(A) ((LOAD8(A)<<8)|LOAD8(A+1))

#define RES(x) res=(res&~0xFF)|((x)&0xFF)
#ifdef AMIGA
#define NUL(X) ((char*)&res)[3]=(X)?1:0
#else
#define NUL(X) RES(X|(X>>8))
#endif

extern void perif(void); /* il n'y a pas de main.h */

extern void crash(void); /* il n'y a pas de main.h */

void ABXi(void) {m=X;m+=B;X=X&~0xFFFF;X|=m&0xFFFF;cl+=3;} /* sam: ajoute masquage debut */
/*
#define ADD8(R,adr,c) int val;adr;val=LOAD8(M);m1=h1=R;m2=h2=val;R+=val;ovfl=sign=res=R;R&=0xFF;cl+=c
*/
#ifndef NO_BCD
#define ADD8(R,adr,c) int val;adr;val=LOAD8(M);m1=h1=R;m2=h2=val;val+=R;ovfl=sign=res=val;R=val&0xFF;cl+=c
#else
#define ADD8(R,adr,c) int val;adr;m2=val=LOAD8(M);val+=(m1=R);ovfl=sign=res=val;R=val&0xFF;cl+=c
#endif

void ADDAi(void) {ADD8(A,IMMED8,2);}
void ADDAd(void) {ADD8(A,DIREC,4);}
void ADDAe(void) {ADD8(A,ETEND,5);}
void ADDAx(void) {ADD8(A,INDEXE,4);}
void ADDBi(void) {ADD8(B,IMMED8,2);}
void ADDBd(void) {ADD8(B,DIREC,4);}
void ADDBe(void) {ADD8(B,ETEND,5);}
void ADDBx(void) {ADD8(B,INDEXE,4);}

/*
#define ADDD(adr,c) int val;adr;val=LOAD16;m2=val>>8;D=((m1=A)<<8)+B+val;A=D>>8;B=0;B|=D&0xFF;ovfl=sign=res=A;res|=B;A&=0xFF;cl+=c
*/
//#define ADDD(adr,c) int val;adr;m2=val=LOAD8(M);++M;val<<=8;val|=LOAD8(M);D=((m1=A)<<8)+B+val;A=(ovfl=sign=(D>>8)&255);B=D&0xFF;res=/*A|B*/(D>>8)|(D&255);cl+=c
#define ADDD(adr,c) int val;adr;val=(m1=A)+(m2=LOAD8(M));++M;val<<=8;val+=LOAD8(M)+B;SETABF(val);cl+=c
void ADDDi(void) {ADDD(IMMED16,4);}
void ADDDd(void) {ADDD(DIREC,6);}
void ADDDe(void) {ADDD(ETEND,7);}
void ADDDx(void) {ADDD(INDEXE,6);}

#ifndef NO_BCD
#define ADC8(R,adr,c) int val;adr;val=LOAD8(M);m2=val;h2=val+((res&0x100)>>8);val=(m1=h1=R)+h2;ovfl=sign=res=val;R=val&0xFF;cl+=c
#else
#define ADC8(R,adr,c) int val;adr;val=LOAD8(M);m2=val;val+=(m1=R)+((res&0x100)>>8);ovfl=sign=res=val;R=val&0xFF;cl+=c
#endif
void ADCAi(void) {ADC8(A,IMMED8,2);}
void ADCAd(void) {ADC8(A,DIREC,4);}
void ADCAe(void) {ADC8(A,ETEND,5);}
void ADCAx(void) {ADC8(A,INDEXE,4);}
void ADCBi(void) {ADC8(B,IMMED8,2);}
void ADCBd(void) {ADC8(B,DIREC,4);}
void ADCBe(void) {ADC8(B,ETEND,5);}
void ADCBx(void) {ADC8(B,INDEXE,4);}

void MUL(void)
{
  int k;
  k=(unsigned short)A*(unsigned short)B;
  if(!k) {res=A=B=0; return;}
/*  A=0;A|=(k>>8)&0xFF;*/
  B=k&0xFF;res=(k<<1)|1;A=k>>8;
  cl+=11;
}

#define SBC8(R,adr,c) int val;adr;val=-LOAD8(M);m2=val;val+=(m1=R)-((res&0x100)>>8);ovfl=sign=res=val;R=val&0xFF;cl+=c
void SBCAi(void) {SBC8(A,IMMED8,2);}
void SBCAd(void) {SBC8(A,DIREC,4);}
void SBCAe(void) {SBC8(A,ETEND,5);}
void SBCAx(void) {SBC8(A,INDEXE,4);}
void SBCBi(void) {SBC8(B,IMMED8,2);}
void SBCBd(void) {SBC8(B,DIREC,4);}
void SBCBe(void) {SBC8(B,ETEND,5);}
void SBCBx(void) {SBC8(B,INDEXE,4);}

/*
#define SUB8(R,adr,c) int val;adr;val=LOAD8(M);m1=R;m2=-val;R-=val;ovfl=sign=res=R;R&=0xFF;cl+=c
*/
#define SUB8(R,adr,c) int val;adr;m2=val=-LOAD8(M);val+=(m1=R);ovfl=sign=res=val;R=val&0xFF;cl+=c
void SUBAi(void) {SUB8(A,IMMED8,2);}
void SUBAd(void) {SUB8(A,DIREC,4);}
void SUBAe(void) {SUB8(A,ETEND,5);}
void SUBAx(void) {SUB8(A,INDEXE,4);}
void SUBBi(void) {SUB8(B,IMMED8,2);}
void SUBBd(void) {SUB8(B,DIREC,4);}
void SUBBe(void) {SUB8(B,ETEND,5);}
void SUBBx(void) {SUB8(B,INDEXE,4);}

/*
#define SUBD(adr,c) int val;adr;val=LOAD16;m1=A;m2=(-val)>>8;D=(A<<8)+B-val;A=D>>8;B=D&0xFF;ovfl=sign=res=A;res|=B;A&=0xFF;cl+=c
*/
#if 1
//#define SUBD(adr,c) int val;adr;val=LOAD8(M);val<<=8;++M;val|=LOAD8(M);val=-val;m2=val>>8;val+=((m1=A)<<8)+B;D=val;B=val&0xFF;A=(ovfl=sign=res=val>>8)&0xFF;res|=B;cl+=c
//#define SUBD(adr,c) int val;adr;val=LOAD8(M);val<<=8;++M;val|=LOAD8(M);val=-val;m2=val>>8;val+=((m1=A)<<8)+B;SETABF(val);cl+=c
//broken: #define SUBD(adr,c) adr;m=(m2=-LOAD8(M))+(m1=A);m<<=8;++M;m+=B-LOAD8(M);SETABF(m);cl+=c
#define SUBD(adr,c) adr;LD_16(m,M);m=-m;m2=m>>8;m+=(m1=A)<<8;m+=B;SETABF(m);cl+=c
#else
#define SUBD(adr,c) int val=0;adr;LD_16(val,M);val=-val;m2=val>>8;val+=((m1=A)<<8)+B;D=val;B=val&0xFF;A=(ovfl=sign=val>>8)&0xFF;res=val|(val>>8);cl+=c
#endif
void SUBDi(void) {SUBD(IMMED16,2);}
void SUBDd(void) {SUBD(DIREC,4);}
void SUBDe(void) {SUBD(ETEND,5);}
void SUBDx(void) {SUBD(INDEXE,4);}

void SEX(void)
{
  int t;
  cl+=2;
  RES(B);
  if (B&0x80) t=255; else t=0;
  sign=A=t;
}

void ASLAi(void) {m1=m2=A;{int m=(ovfl=sign=res=A<<1);((char*)&A)[3]=m;}cl+=2;}
void ASLBi(void) {m1=m2=B;{int m=(ovfl=sign=res=B<<1);((char*)&B)[3]=m;}cl+=2;}

/*
#define ASL(adr,c) int val;adr;val=LOAD8(M);m1=m2=val;val<<=1;STORE8(M,val);ovfl=sign=res=val;cl+=c
*/
#define ASL(adr,c) int val;adr;m1=m2=val=LOAD8(M);val<<=1;ovfl=sign=res=val;cl+=c;STORE8(M,val)
void ASLd(void) {ASL(DIREC,6);}
void ASLe(void) {ASL(ETEND,7);}
void ASLx(void) {ASL(INDEXE,6);}

#if defined(AMIGA)&&defined(__GNUC__)
#define ASMOPT(x,op,t) __asm__ __volatile__(op",%0": "=&d" (t) : "0" (t))
#else
#define ASMOPT(x,op,t) x
#endif

void ASRAi(void) {int oa,t;t=oa=A;ASMOPT(t=(t>>1)|(t&0x80),"asrb #1",t);sign=A=t;res=sign|(oa<<8);cl+=2;}
void ASRBi(void) {int ob,t;t=ob=B;ASMOPT(t=(t>>1)|(t&0x80),"asrb #1",t);sign=B=t;res=sign|(ob<<8);cl+=2;}

/*
#define ASR(adr,c) int val;adr;val=LOAD8(M);res=(val&1)<<8;val=(val>>1)|(val&0x80);STORE8(M,val);sign=val;res|=sign;cl+=c
*/
#define ASR(adr,c) int val;adr;val=LOAD8(M);m=val<<8;ASMOPT(val=(val>>1)|(val&0x80),"asrb #1",val);sign=val;m|=sign;res=m;cl+=c;STORE8(M,val)
void ASRd(void) {ASR(DIREC,6);}
void ASRe(void) {ASR(ETEND,7);}
void ASRx(void) {ASR(INDEXE,6);}

void LSRAi(void) {int r=A;A=(A>>1);sign=0;res=A|(r<<8);cl+=2;}
void LSRBi(void) {int r=B;B=(B>>1);sign=0;res=B|(r<<8);cl+=2;}
/*
#define LSR(adr,c) int val;adr;val=LOAD8(M);res=(val&1)<<8;val=(val>>1);STORE8(M,val);sign=0;res|=val;cl+=c
*/
#define LSR(adr,c) int val;adr;val=LOAD8(M);m=val<<8;val=(val>>1);sign=0;m|=val;res=m;cl+=c;STORE8(M,val)
void LSRd(void) {LSR(DIREC,6);}
void LSRe(void) {LSR(ETEND,7);}
void LSRx(void) {LSR(INDEXE,6);}

void ROLAi(void) {m1=m2=A;{int m=(ovfl=sign=res=(A<<1)|((res&0x100)>>8));((char*)&A)[3]=m;}cl+=2;}
void ROLBi(void) {m1=m2=B;{int m=(ovfl=sign=res=(B<<1)|((res&0x100)>>8));((char*)&B)[3]=m;}cl+=2;}

/*
#define ROL(adr,c) int val;adr;val=LOAD8(M);m1=m2=val;val=(val<<1)|((res&0x100)>>8);STORE8(M,val);ovfl=sign=res=val;cl+=c
*/
#define ROL(adr,c) int val;adr;val=LOAD8(M);m1=m2=val;val=(val<<1)|((res&0x100)>>8);ovfl=sign=res=val;cl+=c;STORE8(M,val)
void ROLd(void) {ROL(DIREC,6);}
void ROLe(void) {ROL(ETEND,7);}
void ROLx(void) {ROL(INDEXE,6);}

void RORAi(void) {int i;i=A;A=(((i&255)|(res&~0xFF))>>1)&255;sign=A;res=(i<<8)|sign;cl+=2;}
void RORBi(void) {int i;i=B;B=(((i&255)|(res&~0xFF))>>1)&255;sign=B;res=(i<<8)|sign;cl+=2;}
/*
#define ROR(adr,c) int i,val;adr;i=val=LOAD8(M);val=(val|(res&0x100))>>1;STORE8(M,val);sign=val;res=((i&1)<<8)|sign;cl+=c
*/
#define ROR(adr,c) int i,val;adr;i=val=LOAD8(M);val=((val|(res&~0xFF))>>1);sign=val&=255;;res=(i<<8)|sign;cl+=c;STORE8(M,val)
void RORd(void) {ROR(DIREC,6);}
void RORe(void) {ROR(ETEND,7);}
void RORx(void) {ROR(INDEXE,6);}

/* Branchements et sauts */

void BRA(void)
{
int m;
  FETCH;
/*  PC+=(signed char)m;*/
  m = (char)m; PC+=m;
  cl+=3;
}

#if defined(REG_PC)&&defined(REG_m)&&defined(AMIGA)
#define add_pc_m __asm__ __volatile__("addw "REG_PC","REG_m"; movel "REG_m","REG_PC)
#else
#define add_pc_m PC=(PC+m)&0xFFFF
#endif

void LBRA(void)
{
  FETCHW;
  add_pc_m;
/*  PC=(PC+m)&0xFFFF;*/
  cl += 5;
}

void JMPd(void)
{
#if 1
  m=DP;m<<=8;m|=LOAD8(PC);
  PC=m;
#else
  PC=(DP<<8)|LOAD8(PC);
#endif
/*
  FETCH;
  PC=(DP<<8)|m;*/
  cl+=3;
}

void JMPe(void)
{
#if 0|1 /* ici */
  M=LOAD8(PC)<<8;PC++;M|=LOAD8(PC);
#else
  LD_16(M,PC);
#endif
  PC=M;
  cl+=4;
}

void JMPx(void)
{
  INDEXE;
  PC=M;
  cl+=3;
}

void BSR(void)
{
//int m;
  FETCH;
  M = S; --M; STORE8(M,PC); --M; STORE8(M,PC>>8); S = M;
  m=(signed char)m; PC+=m;
  cl+=7;
}

void LBSR(void)
{
  FETCHW;/*  M = PC; m=LOAD8(M)<<8;++M; m|=LOAD8(M); ++M; PC=M; */
  M = S; --M; STORE8(M,PC); --M; STORE8(M,PC>>8); S = M;
  add_pc_m; /*PC=(PC+m)&0xFFFF;*/
  cl += 9;
}

void JSRd(void)
{
  FETCH;
  M = S; --M; STORE8(M,PC); --M; STORE8(M,PC>>8); S = M;
  PC=(DP<<8)|m;
  cl+=7;
}

void JSRe(void)
{
  ETEND;
  m = S; --m; STORE8(m,PC); --m; STORE8(m,PC>>8); S = m;
  PC=M;
  cl+=8;
}

void JSRx(void)
{
  INDEXE;
  m = S; --m; STORE8(m,PC); --m; STORE8(m,PC>>8); S = m;
  PC=M;
  cl+=7;
}

void BRN(void)
{
  FETCH;
  cl+=3;
}

void LBRN(void)
{
  FETCHW;
  cl+=5;
}

void NOP(void)
{
  cl+=2;
}

void RTS(void)
{
  M=S;LD_16(PC,M);S=++M;
  cl+=5;
}

/* Branchements conditionnels */

void BCC(void)
{
int m;
  FETCH;
  if (!(res&0x100)) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBCC(void)
{
  FETCHW;
  if (!(res&0x100)) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BCS(void)
{
  int m;
  FETCH;
  if (res&0x100) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBCS(void)
{
  FETCHW;
  if (res&0x100) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BEQ(void)
{
int m;
  FETCH;
  if (!(res&0xff)) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBEQ(void)
{
  FETCHW;
  if (!(res&0xff)) {add_pc_m;cl+=6;}
  else cl+=5; /* sam: was 6 */
}

void BNE(void)
{
  int m;
  FETCH;
  if (res&0xff) {m=(char)m;PC+=m;}
/*  PC += m&((-(res&0xff))>>8);*/
  cl+=3;
}

void LBNE(void)
{
  FETCHW;
  if (res&0xff) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BGE(void)
{
int m;
  FETCH;
  if (!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBGE(void)
{
  FETCHW;
  if (!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BLE(void)
{
int m;
  FETCH;
  if ( (!(res&0xff))
        ||((sign^((~(m1^m2))&(m1^ovfl)))&0x80) ) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBLE(void)
{
  FETCHW;
  if ( (!(res&0xff))
        ||((sign^((~(m1^m2))&(m1^ovfl)))&0x80) ) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BLS(void)
{
int m;
  FETCH;
//  if ((res&0x100)||(!(res&0xff))) 
  if((signed short)((res & 0x1ff)<<7) <= 0)
  {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBLS(void)
{
  FETCHW;
//  if ((res&0x100)||(!(res&0xff))) 
  if((signed short)((res & 0x1ff)<<7) <= 0)
  {add_pc_m;cl+=6;}
  else cl+=5;
}

void BGT(void)
{
int m;
  FETCH;
  if ( (res&0xff)
        &&(!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) ) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBGT(void)
{
  FETCHW;
  if ( (res&0xff)
        &&(!((sign^((~(m1^m2))&(m1^ovfl)))&0x80)) ) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BLT(void)
{
int m;
  FETCH;
  if ((sign^((~(m1^m2))&(m1^ovfl)))&0x80) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBLT(void)
{
  FETCHW;
  if ((sign^((~(m1^m2))&(m1^ovfl)))&0x80)  {add_pc_m;cl+=6;}
  else cl+=5;
}

void BHI(void)
{
int m;
  FETCH;
//  if ((!(res&0x100))&&(res&0xff)) 
  if((signed short)((res & 0x1ff)<<7) > 0)
  {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBHI(void)
{
  FETCHW;
//  if ((!(res&0x100))&&(res&0xff)) 
  if((signed short)((res & 0x1ff)<<7) > 0)
  {add_pc_m;cl+=6;}
  cl+=5;
}

void BMI(void)
{
int m;
  FETCH;
  if (sign&0x80) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBMI(void)
{
  FETCHW;
  if (sign&0x80) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BPL(void)
{
int m;
  FETCH;
  if (!(sign&0x80)) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBPL(void)
{
  FETCHW;
  if (!(sign&0x80)) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BVS(void)
{
int m;
  FETCH;
  if ( (!((m1^m2)&0x80))&&((m1^ovfl)&0x80) ) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBVS(void)
{
  FETCHW;
  if ( (!((m1^m2)&0x80))&&((m1^ovfl)&0x80) ) {add_pc_m;cl+=6;}
  else cl+=5;
}

void BVC(void)
{
int m;
  FETCH;
  if ( ((m1^m2)&0x80)||(!((m1^ovfl)&0x80)) ) {m=(signed char)m;PC+=m;}
  cl+=3;
}

void LBVC(void)
{
  FETCHW;
  if ( ((m1^m2)&0x80)||(!((m1^ovfl)&0x80)) ) {add_pc_m;cl+=6;}
  else cl+=5;
}

void CODE10xx(void)
{
  int m;
  FETCH;
  (*(*PFONCTS[m]))();
}

void CODE11xx(void)
{
  int m;
  FETCH;
  (*(*PFONCTSS[m]))();
}

void unknown(void)
{
  /*
  return_text();
  printf("unknown code at PC=%4x mem=%2x [%2X] %2x %2x\n",PC-1,LOAD8(PC-2),LOAD8(PC-1),LOAD8(PC),LOAD8(PC+1));
  printf("X=%4x Y=%4x U=%4x S=%4x \n",X,Y,U,S);
  printf("A=%2x B=%2x CC=%2x DP=%2x \n",A,B,CC,DP);
  crash();
  */
}

void TRAP(void)
{
  perif();
}

void SWI(void)
{
  PREP_BURST_W;
  getcc();
  CC|=0x80; /* bit E à 1 */
  /* setcc(CC); */
  M = S;
  --M; BURST_W(M,PC); --M; BURST_W(M,PC>>8);
  --M; BURST_W(M,U); --M; BURST_W(M,U>>8);
  --M; BURST_W(M,Y); --M; BURST_W(M,Y>>8);
  --M; BURST_W(M,X); --M; BURST_W(M,X>>8);
  --M; BURST_W(M,DP);
  --M; BURST_W(M,B);
  --M; BURST_W(M,A);
  --M; BURST_W(M,CC);
  S = M;

/*** start2 ***/
  CC|=0x50;    /* valable pour SWI1 */
  setcc(CC);
/*** end2 ***/

//  PC=(LOAD8(0xFFFA)<<8)|LOAD8(0xFFFB);
  M=0xFFFA;LD_16(PC,M);
  cl+=19;
}

void RTI(void)
{
  M = S;
  CC=LOAD8(M); ++M;
  if ((CC&0x80))
  {
    A=LOAD8(M);++M;
    B=LOAD8(M);++M;
    DP=LOAD8(M);++M;
#if 0|1
    {int m=LOAD8(M)<<8;++M;X=m|LOAD8(M);++M;}
    {int m=LOAD8(M)<<8;++M;Y=m|LOAD8(M);++M;}
    {int m=LOAD8(M)<<8;++M;U=m|LOAD8(M);++M;}
/*
    X=LOAD8(M)<<8;++M;X|=LOAD8(M);++M;
    Y=LOAD8(M)<<8;++M;Y|=LOAD8(M);++M;
    U=LOAD8(M)<<8;++M;U|=LOAD8(M);++M;
*/
#else
    LD_16(m,M);X=m;++M;
    LD_16(m,M);Y=m;++M;
    LD_16(m,M);U=m;++M;
#endif
    cl+=15;
  }
  else cl+=6;
#if 0|1
  PC=LOAD8(M);PC<<=8;++M;PC|=LOAD8(M);++M;
#else
  LD_16(PC,M);++M;
#endif
  S = M;
  setcc(CC);
}

void OP01(void)
{
  PC++;
  cl+=2;/* ??? */
}

void DAA(void)
{
#ifndef NO_BCD
   int     i=A+(res&0x100);
   if (((A&15)>9)||((h1&15)+(h2&15)>15)) i+=6;
   if (i>0x99) i+=0x60;
   sign=res=i;
   A=i&255;
   cl+=2;
#else
   printf("DAA\n");
#endif
}

void CWAI(void)
{
  CC&=LOAD8(PC);
  setcc(CC);
  PC++;
  cl+=20;
  wait=-1;
}

void IRQ(void)
{
	if(ccrest & 0x10) return;
	PREP_BURST_W;
        M = S;
        --M; BURST_W(M,PC); --M; BURST_W(M,PC>>8);
        --M; BURST_W(M,U);  --M; BURST_W(M,U>>8);
        --M; BURST_W(M,Y);  --M; BURST_W(M,Y>>8);
        --M; BURST_W(M,X);  --M; BURST_W(M,X>>8);
        --M; BURST_W(M,DP);
        --M; BURST_W(M,B);
        --M; BURST_W(M,A);
	/* mise à 1 du bit E sur le CC */
        getcc(); CC |= 0x80;
        --M; BURST_W(M,CC);
        S = M;
#if 0|1
//	PC=(LOAD8(0xFFF8)<<8)|LOAD8(0xFFF9);
	PC=LOAD8(0xFFF8);PC<<=8;PC|=LOAD8(0xFFF9);
#else
	M=0xFFF8;LD_16(PC,M);
#endif
#if 0
	CC|=0x10;
	/* setcc(CC); */
        ccrest = CC&0xD0;
#endif
	ccrest |= 0x10;
	cl+=19;
}
