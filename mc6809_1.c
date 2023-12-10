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
	MODULE      : MC6809_1.C
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	Module émulation 6809 (Part I)
	version avec décompte des cycles d'horloge
	
	V2.0 : émulation CC rapide (cf funzy TO7)

*/

#include "flags.h" /* sam: ou sinon pb avec REGPARM dans fdraw() */

#include <stdlib.h>
#include <stdio.h>
#include "mapper.h"
#include "graph.h"
#include "index.h"
#include "regs.h"


/* Mise à jour de D */
#define CALCD D=(A<<8)|B
#define CALCAB	B=D&0xFF;A=D>>8

/* Lecture de l'instruction/operandes */
#define FETCH m=LOAD8(PC);++PC;

/* divers modes d'adressage retourne adr dans M */
#define IMMED8	M=PC;PC++
#define IMMED16 M=PC;PC+=2

#ifdef REG_M
# ifndef __GNUC__
#  define LD_16(M,m) M=LOAD8(m);M<<=8;++m;M|=LOAD8(m)
#  define DIREC M=DP;M<<=8;M|=LOAD8(PC);PC++;
#  define ETEND M=LOAD8(PC);M<<=8;++PC;M|=LOAD8(PC);++PC
# else
#  ifdef NO_PROTECT
#   define LD_16(M,m) M=*(unsigned short*)&LOAD8(m);++m;__asm__ __volatile__("moveb %1,%0" : "=&d" (M) : "dmi" (LOAD8(m)), "0" (M))
#  else
#   define LD_16(M,m) M=LOAD8(m);M<<=8;++m;M|=LOAD8(m)
#  endif
#  define DIREC __asm__ __volatile__("movew _DP+3,"REG_M"; moveb %0,"REG_M : : "dmi" (LOAD8(PC)));++PC;
#  define ETEND LD_16(M,PC);++PC
#  undef CALCAB
#  define CALCAB ((char*)&B)[3]=D; ((char*)&A)[3]=((char*)&D)[2];
# endif
#else
# define LD_16(M,m) M=(LOAD8(m)<<8)|LOAD8(m+1);++m;
# define DIREC M=(DP<<8)|LOAD8(PC);PC++;
# define ETEND M=(LOAD8(PC)<<8)|LOAD8(PC+1);PC+=2
#endif

#define INDEXE	{int m=LOAD8(PC);PC++;(*(*INDEX[m]))();}

/* les adresses LOAD8 et STORE8 sont protégées contre >0xFFFF */
/* seul STORE8 est protégé contre le >255 */
#define LOAD16	((LOAD8(M)<<8)|LOAD8(M+1))
#define STORE16(Z) STORE8(M,(Z)>>8);STORE8(M+1,(Z))

#define VZERO1 m1=ovfl
#define VZERO2 VZERO1
/*
#define VZERO2 m1=~m2
*/

#define RES(x) res=(res&~0xFF)|((x)&0xFF)
#ifdef AMIGA
#define NUL(X) ((char*)&res)[3]=(X)?1:0
#else
#define NUL(X) RES(X|(X>>8))
#endif

/* m registre résultat 8 bits */
/* M registre résutlat 16 bits */
unsigned int A,B,DP,CC;
unsigned int Y,U,S,D;

#ifndef REG_m
unsigned int m;
#endif
#ifndef REG_M
unsigned int M;
#endif
#ifndef REG_PC
unsigned int PC;
#endif
#ifndef REG_X
unsigned int X;
#endif

/* emulation CC rapide */

int res,m1,m2,sign,ovfl,h1,h2,ccrest;
/*
int ccrest;
unsigned short res;
unsigned char m1,m2,sign,ovfl,h1,h2;
*/
void (* PFONCT[256])(void); /* sam: (void) manquait */
void (* PFONCTS[256])(void);
void (* PFONCTSS[256])(void);

int *exreg[16];

int getcc(void)
{
#if 0

	unsigned char cc;
	cc = (res>>8)&1;
	cc |= ((((int)(unsigned char)res)-1)>>8) & 4;
	cc |= (((char)(~((char)m1^(char)m2)) & (char)((char)m1^(char)ovfl)) & 0x80)>>6;
	cc |= (sign&128)>>4;
#ifndef NO_BCD
 	cc |= ((char)(((char)h1&15) + ((char)h2&15)) & 16)*2;
#endif
	return CC = ccrest | cc;
#else

#if 0
       CC=
	  0 /*((((h1&15)+(h2&15))&16)<<1)*/
	  |((sign&0x80)>>4)
	  |/*(((((int)(unsigned char)res)-1)>>8) & 4)*/((((res&0xff)==0)&1)<<2)
	  |((	((~(m1^m2))&(m1^ovfl)) &0x80)>>6)
	  |((res&0x100)>>8)
	  |ccrest;
#endif

#if 0
       CC=
#ifdef NO_BCD
	  0
#else
	  ((((h1&15)+(h2&15))&16)<<1)
#endif
	  |/*((sign>>4)&0x08)*/((sign&0x80)>>4)
	  |/*(((((int)(unsigned char)res)-1)>>8) & 4)*/ ((((res&0xff)==0)&1)<<2)
	  |/*((((~(m1^m2))&(m1^ovfl))>>6)&2)*/((	((~(m1^m2))&(m1^ovfl)) &0x80)>>6)
	  |((res>>8)&1)/*((res&0x100)>>8)*/
	  |ccrest;
        return CC;
#else
       CC=
#ifdef NO_BCD
	  0
#else
	  ((((h1&15)+(h2&15))&16)<<1)
#endif
	  |((sign>>4)&0x08)/*((sign&0x80)>>4)*/
	  |/*(((((int)(unsigned char)res)-1)>>8) & 4)*/ ((((res&0xff)==0)&1)<<2)
	  |((((~(m1^m2))&(m1^ovfl))>>6)&(char)2)
/*((	((~(m1^m2))&(m1^ovfl)) &0x80)>>6)*/
	  |((res>>8)&1)/*((res&0x100)>>8)*/
	  |ccrest;
        return CC;
#endif

#endif
}

void REGPARM setcc(int i)
{
  m1=m2=0;
  res=((i&1)<<8)|(4^(i&4));
  ovfl=/*(i&(char)2)<<6*/i<<6;
  sign=/*(i&(char)8)<<4*/i<<4;
#ifndef NO_BCD
  h1=h2=(i&32)>>2;
#endif
  ccrest=i&0xd0;
}

/* LOAD */

#if !defined(__GNUC__) /* sam: 1 31/01 */
#define LD8(R,adr,c)  VZERO1;adr;sign=R=LOAD8(M);cl+=c;RES(sign);
#define LD16(R,adr,c) VZERO1;adr;cl+=c;{int val=(sign=LOAD8(M))<<8;++M;R=val|LOAD8(M);}NUL(R)
#else
#define LD8(R,adr,c)  VZERO1;adr;sign=R=LOAD8(M);cl+=c;RES(sign)
#define LD16(R,adr,c) VZERO1;adr;LD_16(m,M);__asm__ __volatile__("movew %0,_"#R"+2; sne _res+3; slt _sign+3" : : "dmi" (m));cl+=c
#endif

void LDAi(void) {LD8(A,IMMED8,2);}
void LDAd(void) {LD8(A,DIREC,4);}
void LDAe(void) {LD8(A,ETEND,5);}
void LDAx(void) {LD8(A,INDEXE,4);}
void LDBi(void) {LD8(B,IMMED8,2);}
void LDBd(void) {LD8(B,DIREC,4);}
void LDBe(void) {LD8(B,ETEND,5);}
void LDBx(void) {LD8(B,INDEXE,4);}
void LDDi(void) {LD16(D,IMMED16,3);CALCAB;}
void LDDd(void) {LD16(D,DIREC,5);CALCAB;}
void LDDe(void) {LD16(D,ETEND,6);CALCAB;}
void LDDx(void) {LD16(D,INDEXE,6);CALCAB;}
void LDUi(void) {LD16(U,IMMED16,3);}
void LDUd(void) {LD16(U,DIREC,5);}
void LDUe(void) {LD16(U,ETEND,6);}
void LDUx(void) {LD16(U,INDEXE,5);}
void LDSi(void) {LD16(S,IMMED16,3);}
void LDSd(void) {LD16(S,DIREC,5);}
void LDSe(void) {LD16(S,ETEND,6);}
void LDSx(void) {LD16(S,INDEXE,5);}
void LDXi(void) {LD16(X,IMMED16,3);}
void LDXd(void) {LD16(X,DIREC,5);}
void LDXe(void) {LD16(X,ETEND,6);}
void LDXx(void) {LD16(X,INDEXE,5);}
void LDYi(void) {LD16(Y,IMMED16,3);}
void LDYd(void) {LD16(Y,DIREC,5);}
void LDYe(void) {LD16(Y,ETEND,6);}
void LDYx(void) {LD16(Y,INDEXE,5);}


/* STORE */

/*
#define ST8(R,adr,c)  adr;STORE8(M,R);VZERO1;sign=R;RES(sign);cl+=c
#define ST16(R,adr,c) adr;STORE16(R);VZERO1;sign=R>>8;RES(sign|R);cl+=c
*/
#define ST8(R,adr,c)  VZERO1;sign=R;RES(sign);cl+=c;adr;STORE8(M,R)
#define ST16(R,adr,c) VZERO1;cl+=c;adr;NUL(R);STORE8(M,(sign=R>>8));++M;STORE8(M,R)
#define STAB(adr,c) VZERO1;cl+=c;adr;NUL((m=B)|A);STORE8(M,sign=A);++M;STORE8(M,m)
void STAd(void) {ST8(A,DIREC,4);}
void STAe(void) {ST8(A,ETEND,5);}
void STAx(void) {ST8(A,INDEXE,4);}
void STBd(void) {ST8(B,DIREC,4);}
void STBe(void) {ST8(B,ETEND,5);}
void STBx(void) {ST8(B,INDEXE,4);}
#if 1
void STDd(void) {CALCD;ST16(D,DIREC,5);}
void STDe(void) {CALCD;ST16(D,ETEND,6);}
void STDx(void) {CALCD;ST16(D,INDEXE,6);}
#else
void STDd(void) {STAB(DIREC,6);}
void STDe(void) {STAB(ETEND,6);}
void STDx(void) {STAB(INDEXE,6);}
#endif
void STUd(void) {ST16(U,DIREC,5);}
void STUe(void) {ST16(U,ETEND,6);}
void STUx(void) {ST16(U,INDEXE,5);}
void STSd(void) {ST16(S,DIREC,5);}
void STSe(void) {ST16(S,ETEND,6);}
void STSx(void) {ST16(S,INDEXE,5);}
void STXd(void) {ST16(X,DIREC,5);}
void STXe(void) {ST16(X,ETEND,6);}
void STXx(void) {ST16(X,INDEXE,5);}
void STYd(void) {ST16(Y,DIREC,5);}
void STYe(void) {ST16(Y,ETEND,6);}
void STYx(void) {ST16(Y,INDEXE,5);}

/* LOAD effective address */
/*
#define LEA(R)	INDEXE;R=M;RES(R|(R>>8));cl+=4
*/
#define LEA(R)	INDEXE;R=M;NUL(R);cl+=4
void LEASx(void) {INDEXE;S=M;cl+=4;}
void LEAUx(void) {INDEXE;U=M;cl+=4;}
void LEAXx(void) {LEA(X);}
void LEAYx(void) {LEA(Y);}

/* CLEAR */
void CLRA(void) {VZERO1;sign=res=A=0;cl+=2;}
void CLRB(void) {VZERO1;sign=res=B=0;cl+=2;}
/*
#define CLR(adr,c) adr;STORE8(M,0);VZERO2;sign=res=0;cl+=c
*/
#define CLR(adr,c) VZERO2;sign=res=0;cl+=c;adr;STORE8(M,0)
void CLRd(void) {CLR(DIREC,6);}
void CLRe(void) {CLR(ETEND,7);}
void CLRx(void) {CLR(INDEXE,6);}

/* Exchange/transfer */
#if defined(REG_PC) || defined(REG_X) /* || autres REG_??? */
void EXG(void)
{
  register int k __asm("d0") ,l __asm("d1");
  FETCH;
  switch((0xF0 & m) >> 4) {
    case 0x0: CALCD;k=D; break;  case 0x1: k=X; break;
    case 0x2: k=Y; break;        case 0x3: k=U; break;
    case 0x4: k=S; break;        case 0x5: k=PC; break;
    case 0x8: k=A; break;        case 0x9: k=B; break;
    case 0xA: k=getcc(); break;  case 0xB: k=DP; break;
    default: k=0; break;
  }
  switch(0x0F & m) {
    case 0x0: CALCD;l=D; break;  case 0x1: l=X; break;
    case 0x2: l=Y; break;        case 0x3: l=U; break;
    case 0x4: l=S; break;        case 0x5: l=PC; break;
    case 0x8: l=A; break;        case 0x9: l=B; break;
    case 0xA: l=getcc(); break;  case 0xB: l=DP; break;
    default: l=0; break;
  }
  switch((0xF0 & m)>>4) {
    case 0x0: D=l;CALCAB;     break; case 0x1: X=l; break;
    case 0x2: Y=l; break;            case 0x3: U=l; break;
    case 0x4: S=l; break;            case 0x5: PC=l; break;
    case 0x8: A=l; break;            case 0x9: B=l; break;
    case 0xA: setcc(l); break;       case 0xB: DP=l; break;
  }
  switch(0x0F & m) {
    case 0x0: D=k;CALCAB;     break; case 0x1: X=k; break;
    case 0x2: Y=k; break;            case 0x3: U=k; break;
    case 0x4: S=k; break;            case 0x5: PC=k; break;
    case 0x8: A=k; break;            case 0x9: B=k; break;
    case 0xA: setcc(k); break;       case 0xB: DP=k; break;
  }
  cl += 8;
}
#else
void EXG(void)
{
  int	 k,l;
  int		 o1,o2;
  int	 *p,*q;
  FETCH;
  o1=(m&0xF0)>>4;
  o2=(m)&0x0F;

  if ((p=exreg[o1])) k=*p;

  else	 if (o1)	k=getcc();
  else k=(A<<8)+B;

  if ((q=exreg[o2]))
  {
	 l=*q;
	 *q=k;
  }
  else
  if (o2)
  {
	 l=getcc();
	 setcc(k);
  }
  else
  {
	 l=(A<<8)+B;
	 A=(k>>8)&255;
	 B=k&255;
  }
  if (p)	*p=l;
  else if (o1)	setcc(l);
  else
  {
	 A=(l>>8)&255;
	 B=l&255;
  }
  cl+=8;
}
#endif

#if defined(REG_PC) || defined(REG_X) /* || autres REG_??? */
void TFR(void)
{
  int k;
  FETCH;
  switch((0xF0 & m) >> 4) {
    case 0x0: CALCD;k=D; break;  case 0x1: k=X; break;
    case 0x2: k=Y; break;        case 0x3: k=U; break;
    case 0x4: k=S; break;        case 0x5: k=PC; break;
    case 0x8: k=A; break;        case 0x9: k=B; break;
    case 0xA: k=getcc(); break;  case 0xB: k=DP; break;
    default: k=0; break;
  }
  switch(0x0F & m) {
    case 0x0: D=k;CALCAB; break;     case 0x1: X=k; break;
    case 0x2: Y=k; break;            case 0x3: U=k; break;
    case 0x4: S=k; break;            case 0x5: PC=k; break;
    case 0x8: A=k; break;            case 0x9: B=k; break;
    case 0xA: setcc(k); break;       case 0xB: DP=k; break;
  }
 cl+=6;
}
#else
void TFR(void)
{
 int		k;
 int		o1,o2;
 int		*p,*q;
 FETCH;
 o1=((m)&0xF0)>>4;
 o2=(m)&0x0F;

 if ((p=exreg[o1]))
     k=*p;
 else if (o1)
     k=getcc();
      else k=(A<<8)+B;

 if ((q=exreg[o2]))
     *q=k;
 else if (o2)
          setcc(k);
      else
      {
          A=(k>>8)&255;
          B=k&255;
      }
 cl+=6;
}
#endif

/* Stack operations */
#define push16(Z) do {--M;BURST_W(M,Z);--M;BURST_W(M,Z>>8);cl+=2;} while(0)
//#define push16(Z) do {--M;BURST_W(M,Z);--M;BURST_W(M,((unsigned char*)&Z)[2]);cl+=2;} while(0)
#define push8(Z)  do {--M;BURST_W(M,Z);++cl;} while(0)
void PSHS(void)
{
#if 1
  PREP_BURST_W;
  FETCH;
  M = S;
  if (m & 0x80) push16(PC);
  if (m & 0x40) push16(U);
  if (m & 0x20) push16(Y);
  if (m & 0x10) push16(X);
  if (m & 0x08) push8(DP);
  if (m & 0x04) push8(B);
  if (m & 0x02) push8(A);
  if (m & 0x01) {getcc();push8(CC);}
  S = M;
  cl+=5;
#else
  static unsigned char tmp[4*2+4];
  unsigned char *s = tmp;
  short int i;
  FETCH;
  if (m & 0x01) {getcc();*s++ = CC;}
  if (m & 0x02) *s++ = A;
  if (m & 0x04) *s++ = B;
  if (m & 0x08) *s++ = DP;
  if (m & 0x10) {*(unsigned short*)s = X;  s += 2;}
  if (m & 0x20) {*(unsigned short*)s = Y;  s += 2;}
  if (m & 0x40) {*(unsigned short*)s = U;  s += 2;}
  if (m & 0x80) {*(unsigned short*)s = PC; s += 2;}
  i = s - tmp;
  cl += 5 + i;
  M=S;
  while(i--) {--M;STORE8(M, *--s);}
  S=M;
#endif
}
void PSHU(void)
{
  PREP_BURST_W;
  FETCH;
  M = U;
  if (m & 0x80) push16(PC);
  if (m & 0x40) push16(S);
  if (m & 0x20) push16(Y);
  if (m & 0x10) push16(X);
  if (m & 0x08) push8(DP);
  if (m & 0x04) push8(B);
  if (m & 0x02) push8(A);
  if (m & 0x01) {getcc();push8(CC);}
  U = M;
  cl+=5;
}
#define pull16(Z) do {/*int z=LOAD8(M);z<<=8;++M;Z=z|LOAD8(M);*/int z; LD_16(z,M);Z=z;++M;cl+=2;} while(0)
#define pull16_(Z) do {Z=LOAD8(M)<<8;++M;Z|=LOAD8(M);++M;cl+=2;} while(0)
#define pull8(Z)  do {Z=LOAD8(M);++cl;++M;} while(0)
void PULS(void)
{
  FETCH;
  M = S;
  if (m & 0x01) {pull8(CC);setcc(CC);}
  if (m & 0x02) pull8(A);
  if (m & 0x04) pull8(B);
  if (m & 0x08) pull8(DP);
  if (m & 0x10) pull16(X);
  if (m & 0x20) pull16(Y);
  if (m & 0x40) pull16(U);
  if (m & 0x80) pull16(PC);
  S = M;
  cl+=5;
}
void PULU(void)
{
  FETCH;
  M = U;
  if (m & 0x01) {pull8(CC);setcc(CC);}
  if (m & 0x02) pull8(A);
  if (m & 0x04) pull8(B);
  if (m & 0x08) pull8(DP);
  if (m & 0x10) pull16(X);
  if (m & 0x20) pull16(Y);
  if (m & 0x40) pull16(S);
  if (m & 0x80) pull16(PC);
  U = M;
  cl+=5;
}

/* Increment/Decrement */
#if defined(AMIGA)&&defined(__GNUC__)
#define ASMOPT(x,op,t) __asm__ __volatile__(op",%0": "=&d" (t) : "0" (t))
#else
#define ASMOPT(x,op,t) x
#endif
void INCA(void) {int t; m2=0;m1=t=A;ASMOPT(t=(t&~255)|(t+1)&255,"addqb #1",t);ovfl=sign=A=t;RES(sign);cl+=2;}
void INCB(void) {int t; m2=0;m1=t=B;ASMOPT(t=(t&~255)|(t+1)&255,"addqb #1",t);ovfl=sign=B=t;RES(sign);cl+=2;}

/*
#define INC(adr,c) int val;adr;val=LOAD8(M);m1=val;m2=0;val++;STORE8(M,val);ovfl=sign=val&0xFF;res=(res&0x100)|sign;cl+=c
*/
#define INC(adr,c) int val;adr;m2=0;m1=val=LOAD8(M);ASMOPT(val=(val+1)&255,"addqb #1",val);ovfl=sign=val;RES(sign);cl+=c;STORE8(M,val)
void INCe(void) {INC(ETEND,7);}
void INCd(void) {INC(DIREC,6);}
void INCx(void) {INC(INDEXE,6);}

void DECA(void) {int t; ((short*)&m2)[1]=0x80;m1=t=A;ASMOPT(t=(t-1)&255,"subqb #1",t);cl+=2;ovfl=sign=A=t;RES(sign);}
void DECB(void) {int t; ((short*)&m2)[1]=0x80;m1=t=B;ASMOPT(t=(t-1)&255,"subqb #1",t);cl+=2;ovfl=sign=B=t;RES(sign);}
/*
#define DEC(adr,c) int val;adr;val=LOAD8(M);m1=val;m2=0x80;val--;STORE8(M,val);ovfl=sign=val&0xFF;res=(res&0x100)|sign;cl+=c
*/
#define DEC(adr,c) int val;adr;((short*)&m2)[1]=0x80;m1=val=LOAD8(M);ASMOPT(val=(val-1)&255,"subqb #1",val);ovfl=sign=val;RES(val);cl+=c;STORE8(M,val)
void DECe(void) {DEC(ETEND,7);}
void DECd(void) {DEC(DIREC,6);}
void DECx(void) {DEC(INDEXE,6);}

/* Test et comparaisons */

#define BIT(R,adr,c) adr;VZERO1;sign=R&LOAD8(M);RES(sign);cl+=c
void BITAi(void) {BIT(A,IMMED8,2);}
void BITAd(void) {BIT(A,DIREC,4);}
void BITAe(void) {BIT(A,ETEND,5);}
void BITAx(void) {BIT(A,INDEXE,4);}
void BITBi(void) {BIT(B,IMMED8,2);}
void BITBd(void) {BIT(B,DIREC,4);}
void BITBe(void) {BIT(B,ETEND,5);}
void BITBx(void) {BIT(B,INDEXE,4);}

/*
#define CMP8(R,adr,c) int val;adr;val=LOAD8(M);m1=R;m2=-val;ovfl=sign=res=R-val;cl+=c
#define CMP16(R,adr,c) {int val;adr;val=LOAD16;m1=R>>8;m2=(-val)>>8;ovfl=sign=res=(R-val)>>8;res|=(R-val)&0xFF;cl+=c;}
*/
#define CMP8(R,adr,c) int val;adr;val=-(LOAD8(M));m2=val;val+=(m1=R);ovfl=sign=res=val;cl+=c
/* sam: ici ya ptet un bug que j'ai introduit */
/*
#define CMP16(R,adr,c) {int val;adr;m1=R>>8;val=-(get16(M));m2=val>>8;val+=R;res=(ovfl=sign=val>>8)|val;cl+=c;}
*/

/*
#define CMP16(R,adr,c) {int val;m1=R>>8;adr;LD_16(val,M);val=-val;m2=val>>8;val+=R;res=(ovfl=sign=val>>8)|(val&255);cl+=c;}
*/
#define CMP16(R,adr,c) ((unsigned char*)&m1)[3]=((unsigned char*)&R)[2];adr;LD_16(m,M);m=-m;m2=m>>8;m+=R;res=(ovfl=sign=m>>8)|(m&255);cl+=c;
void CMPAi(void) {CMP8(A,IMMED8,2);}
void CMPAd(void) {CMP8(A,DIREC,4);}
void CMPAe(void) {CMP8(A,ETEND,5);}
void CMPAx(void) {CMP8(A,INDEXE,4);}
void CMPBi(void) {CMP8(B,IMMED8,2);}
void CMPBd(void) {CMP8(B,DIREC,4);}
void CMPBe(void) {CMP8(B,ETEND,5);}
void CMPBx(void) {CMP8(B,INDEXE,4);}
void CMPDi(void) {CALCD;CMP16(D,IMMED16,5)}
void CMPDd(void) {CALCD;CMP16(D,DIREC,7)}
void CMPDe(void) {CALCD;CMP16(D,ETEND,8)}
void CMPDx(void) {CALCD;CMP16(D,INDEXE,7)}
void CMPSi(void) {CMP16(S,IMMED16,5);}
void CMPSd(void) {CMP16(S,DIREC,7);}
void CMPSe(void) {CMP16(S,ETEND,8);}
void CMPSx(void) {CMP16(S,INDEXE,7);}
void CMPUi(void) {CMP16(U,IMMED16,5);}
void CMPUd(void) {CMP16(U,DIREC,7);}
void CMPUe(void) {CMP16(U,ETEND,8);}
void CMPUx(void) {CMP16(U,INDEXE,7);}
void CMPXi(void) {CMP16(X,IMMED16,5);}
void CMPXd(void) {CMP16(X,DIREC,7);}
void CMPXe(void) {CMP16(X,ETEND,8);}
void CMPXx(void) {CMP16(X,INDEXE,7);}
void CMPYi(void) {CMP16(Y,IMMED16,5);}
void CMPYd(void) {CMP16(Y,DIREC,7);}
void CMPYe(void) {CMP16(Y,ETEND,8);}
void CMPYx(void) {CMP16(Y,INDEXE,7);}

void TSTAi(void) {VZERO1;sign=A;RES(sign);cl+=2;}
void TSTBi(void) {VZERO1;sign=B;RES(sign);cl+=2;}

#define TST(adr,c) VZERO2;adr;sign=LOAD8(M);RES(sign);cl+=c
void TSTd(void) {TST(DIREC,6);}
void TSTe(void) {TST(ETEND,7);}
void TSTx(void) {TST(INDEXE,6);}

/* Booleans op */
/*
#define AND(R,adr,c)	int val;adr;val=LOAD8(M);VZERO1;R&=val;sign=R;res=(res&0x100)|sign;cl+=c
*/
#define AND(R,adr,c)	adr;VZERO1;sign=(R&=LOAD8(M));RES(sign);cl+=c
void ANDAi(void) {AND(A,IMMED8,2);}
void ANDAd(void) {AND(A,DIREC,4);}
void ANDAe(void) {AND(A,ETEND,5);}
void ANDAx(void) {AND(A,INDEXE,4);}
void ANDBi(void) {AND(B,IMMED8,2);}
void ANDBd(void) {AND(B,DIREC,4);}
void ANDBe(void) {AND(B,ETEND,5);}
void ANDBx(void) {AND(B,INDEXE,4);}

void ANDCCi(void)
{
  FETCH;
  getcc();CC&=m;setcc(CC);
  cl+=3;
}

/*
#define OR(R,adr,c) int	val;adr;val=LOAD8(M);VZERO1;R|=val;sign=R;res=(res&0x100)|sign;cl+=c
*/
#define OR(R,adr,c) adr;VZERO1;R|=LOAD8(M);sign=R;RES(sign);cl+=c
void ORAi(void) {OR(A,IMMED8,2);}
void ORAd(void) {OR(A,DIREC,4);}
void ORAe(void) {OR(A,ETEND,5);}
void ORAx(void) {OR(A,INDEXE,4);}
void ORBi(void) {OR(B,IMMED8,2);}
void ORBd(void) {OR(B,DIREC,4);}
void ORBe(void) {OR(B,ETEND,5);}
void ORBx(void) {OR(B,INDEXE,4);}

void ORCCi(void)
{
  FETCH;
  getcc();CC|=m;setcc(CC);
  cl+=3;
}

/*
#define EOR(R,adr,c)	int val;adr;val=LOAD8(M);VZERO1;R^=val;sign=R;res=(res&0x100)|sign;cl+=c
*/
#define EOR(R,adr,c)	VZERO1;adr;R^=LOAD8(M);sign=R;RES(sign);cl+=c
void EORAi(void) {EOR(A,IMMED8,2);}
void EORAd(void) {EOR(A,DIREC,4);}
void EORAe(void) {EOR(A,ETEND,5);}
void EORAx(void) {EOR(A,INDEXE,4);}
void EORBi(void) {EOR(B,IMMED8,2);}
void EORBd(void) {EOR(B,DIREC,4);}
void EORBe(void) {EOR(B,ETEND,5);}
void EORBx(void) {EOR(B,INDEXE,4);}

void COMAi(void) {VZERO1;A^=0xFF;sign=A;res=sign|0x100;cl+=2;}
void COMBi(void) {VZERO1;B^=0xFF;sign=B;res=sign|0x100;cl+=2;}

/*
#define COM(adr,c) int val;adr;val=LOAD8(M);VZERO2;val=(~val)&0xFF;STORE8(M,val);sign=val;res=sign|0x100;cl+=c
*/
#define COM(adr,c) adr;sign=LOAD8(M)^0xFF;res=sign|0x100;cl+=c;STORE8(M,sign)
void COMd(void) {COM(DIREC,6);}
void COMe(void) {COM(ETEND,7);}
void COMx(void) {COM(INDEXE,6);}

void NEGAi(void) {m1=A;m2=-A;A=m2&255;cl+=2;ovfl=sign=res=m2;}
void NEGBi(void) {m1=B;m2=-B;B=m2&255;cl+=2;ovfl=sign=res=m2;}

/*
#define NEG(adr,c) int val;adr;val=LOAD8(M);m1=val;m2=-val;val=-val;STORE8(M,val);ovfl=sign=res=val;cl+=c
*/
#define NEG(adr,c) int val;adr;m1=val=LOAD8(M);m2=-val;ovfl=sign=res=m2;cl+=c;STORE8(M,m2)
void NEGd(void) {NEG(DIREC,6);}
void NEGe(void) {NEG(ETEND,7);}
void NEGx(void) {NEG(INDEXE,6);}
