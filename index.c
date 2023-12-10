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
	MODULE      : index.c
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	Emulation 6809

	Fétis G. 02/1996

	Module émulation rapide du mode indexé

	Ce module utilise une table de fonctions indexée
	par le postoctet.

	tous les modes non documentés retournent 0

	10/97
	Cette version est adaptée aux compilateurs 32 bits

*/

#include "flags.h"

#include <stdio.h>
#include "mapper.h"
#include "index.h"
#include "regs.h"

#define mem16(ADR) (LOAD8(ADR)<<8)|LOAD8((ADR+1)&0xFFFF)

void (* INDEX[256])(void); /* sam: (void) manquait */

/*
  format i_c1_c2_c2p_c3
  c1 = (d)irect / (i)ndirect
  c2 = (0)bits (5)bits (8)bits (16)bits A B D PC8 PC16 P1 P2 M1 M2 e16
  c2p = si c2==5, deplacement 0,1 ... positif M0,M1 ... négatif (M0=-1!!)
  c3 = registre X Y U S (sauf PC8 PC16 et e16)
*/

/* 
note de sam: je me demande si on ne pourrait pas virer les variables 
globales M et m au profit de l'usage de variables automatiques (ie. registres) 
allouees dans les fonctions emulants les opcodes. Cela donnerait quelque chose 
comme:

unsigned int (*INDEX[256])(void);
void i_d_X(void) {return X;} 
... etc...

mc6809_1.c:
#define INDEXE m=LOAD8(PC);PC++;M=(*(*INDEX[m]))()
#define LD8(R,adr,c)  adr;sign=R=LOAD8(M);m1=ovfl;res=(res&0x100)|sign;cl+=c
...
void LDAx(void) {int m,M; LD8(A,INDEXE,4);}

Le compilo va ainsi eviter plusieurs acces en mémoire.
*/

#if 0
#define LSW(X,V) X = ((X)&~0xFFFF)|(unsigned short)(V)
#else
#define LSW(X,V) X = (unsigned short)(V)
#endif

/*
#define LSW(X,V) X = ((X)&~0xFFFF)|((V)&0xFFFF)
#define LSW(X,V) X = (V)&0xFFFF
*/
#define W0(X,V) X = (V)&0xFFFF
/*
#define LSW(X,V) X = (V)&0xFFFF
*/
#if defined(REG_M)&&defined(__OPTIMIZE__)
#define SETM(BASE,OFF1,OFF2) \
	if(OFF1<0)\
		if(OFF2) __asm__ __volatile__("movew _"#BASE"+2,"REG_M"\n\tsubqw #-"#OFF1","REG_M"\n\tsubqw #-"#OFF2","REG_M);\
		else __asm__ __volatile__("movew _"#BASE"+2,"REG_M"\n\tsubqw #-"#OFF1","REG_M);\
	else {\
		if(OFF2) __asm__ __volatile__("movew _"#BASE"+2,"REG_M"\n\taddqw #"#OFF1","REG_M"\n\taddqw #"#OFF2","REG_M);\
		else __asm__ __volatile__("movew _"#BASE"+2,"REG_M"\n\taddqw #"#OFF1","REG_M);\
	}
#define ADDS(BASE,OFF) \
	if(OFF<0) __asm__ __volatile__("subqw #-"#OFF",_"#BASE"+2");\
	else      __asm__ __volatile__("addqw #"#OFF",_"#BASE"+2")
#define ADD_BYTE_REG(M,A,X) __asm__ __volatile__("moveb _"#A"+3,%0; extw %0; addw _"#X"+2,%0" : "=&d" (M) : "0" (M))
#define ADD_BYTE(M,X) __asm__ __volatile__("extw %0; addw _"#X"+2,%0" : "=&d" (M) : "0" (M))
#define ADD_WORD(M,X) __asm__ __volatile__("addw _"#X"+2,%0" : "=&d" (M) : "0" (M))
/* pb d'acces non aligne pour les 2 suivants */
#ifdef NO_PROTECT
#define LD_16(M,m) M=*(unsigned short*)&LOAD8(m);++m;__asm__ __volatile__("moveb %1,%0" : "=&d" (M) : "dmi" (LOAD8(m)), "0" (M))
#else
#define LD_16(M,m) M=LOAD8(m);M<<=8;++m;M|=LOAD8(m)
#endif
#define CALC_D(m) __asm__ __volatile__("movew _A+3,%0; moveb _B+3,%0" : "=&d" (m) : "0" (m))
#else
#define SETM(BASE,OFF1,OFF2) LSW(M,BASE+OFF1+OFF2)
#define ADDS(BASE,OFF) LSW(BASE,BASE+OFF)
#define ADD_BYTE_REG(M,A,X) LSW(M,X+(char)A)
#define ADD_BYTE(M,X) LSW(M,X+(char)M)
#define ADD_WORD(M,X) LSW(M,X+M)
#define LD_16(M,m) M=LOAD8(m);M<<=8;++m;M|=LOAD8(m)
#define CALC_D(m) m=A;m<<=8;m|=B
#endif

/* sans deplacement */
void i_d_X(void)
{
  M=X;
}
void i_d_Y(void)
{
  M=Y;
}
void i_d_U(void)
{
  M=U;
}
void i_d_S(void)
{
  M=S;
}

/* 5bits X */

void i_d_5_0_X(void)
{
  M=X;cl++;
}
void i_d_5_1_X(void)
{
  SETM(X,1,0);
  ++cl;
}
void i_d_5_2_X(void)
{
  SETM(X,2,0);cl++;
}
void i_d_5_3_X(void)
{
  SETM(X,3,0);cl++;
}
void i_d_5_4_X(void)
{
  SETM(X,4,0);cl++;
}
void i_d_5_5_X(void)
{
  SETM(X,5,0);cl++;
}
void i_d_5_6_X(void)
{
  SETM(X,6,0);cl++;
}
void i_d_5_7_X(void)
{
  SETM(X,7,0);cl++;
}
void i_d_5_8_X(void)
{
  SETM(X,8,0);cl++;
}
void i_d_5_9_X(void)
{
  SETM(X,8,1);cl++;
}
void i_d_5_A_X(void)
{
  SETM(X,8,2);cl++;
}
void i_d_5_B_X(void)
{
  SETM(X,8,3);cl++;
}
void i_d_5_C_X(void)
{
  SETM(X,8,4);cl++;
}
void i_d_5_D_X(void)
{
  SETM(X,8,5);cl++;
}
void i_d_5_E_X(void)
{
  SETM(X,8,6);cl++;
}
void i_d_5_F_X(void)
{
  SETM(X,8,7);cl++;
}

void i_d_5_M0_X(void)
{
  SETM(X,-8,-8);cl++;
}
void i_d_5_M1_X(void)
{
  SETM(X,-8,-7);cl++;
}
void i_d_5_M2_X(void)
{
  SETM(X,-8,-6);cl++;
}
void i_d_5_M3_X(void)
{
  SETM(X,-8,-5);cl++;
}
void i_d_5_M4_X(void)
{
  SETM(X,-8,-4);cl++;
}
void i_d_5_M5_X(void)
{
  SETM(X,-8,-3);cl++;
}
void i_d_5_M6_X(void)
{
  SETM(X,-8,-2);cl++;
}
void i_d_5_M7_X(void)
{
  SETM(X,-8,-1);cl++;
}
void i_d_5_M8_X(void)
{
  SETM(X,-8,0);cl++;
}
void i_d_5_M9_X(void)
{
  SETM(X,-7,0);cl++;
}
void i_d_5_MA_X(void)
{
  SETM(X,-6,0);cl++;
}
void i_d_5_MB_X(void)
{
  SETM(X,-5,0);cl++;
}
void i_d_5_MC_X(void)
{
  SETM(X,-4,0);cl++;
}
void i_d_5_MD_X(void)
{
  SETM(X,-3,0);cl++;
}
void i_d_5_ME_X(void)
{
  SETM(X,-2,0);cl++;
}
void i_d_5_MF_X(void)
{
  SETM(X,-1,0);cl++;
}

/* 5bits Y */

void i_d_5_0_Y(void)
{
  M=Y;cl++;
}
void i_d_5_1_Y(void)
{
  SETM(Y,1,0);cl++;
}
void i_d_5_2_Y(void)
{
  SETM(Y,2,0);cl++;
}
void i_d_5_3_Y(void)
{
  SETM(Y,3,0);cl++;
}
void i_d_5_4_Y(void)
{
  SETM(Y,4,0);cl++;
}
void i_d_5_5_Y(void)
{
  SETM(Y,5,0);cl++;
}
void i_d_5_6_Y(void)
{
  SETM(Y,6,0);cl++;
}
void i_d_5_7_Y(void)
{
  SETM(Y,7,0);cl++;
}
void i_d_5_8_Y(void)
{
  SETM(Y,8,0);cl++;
}
void i_d_5_9_Y(void)
{
  SETM(Y,8,1);cl++;
}
void i_d_5_A_Y(void)
{
  SETM(Y,8,2);cl++;
}
void i_d_5_B_Y(void)
{
  SETM(Y,8,3);cl++;
}
void i_d_5_C_Y(void)
{
  SETM(Y,8,4);cl++;
}
void i_d_5_D_Y(void)
{
  SETM(Y,8,5);cl++;
}
void i_d_5_E_Y(void)
{
  SETM(Y,8,6);cl++;
}
void i_d_5_F_Y(void)
{
  SETM(Y,8,7);cl++;
}

void i_d_5_M0_Y(void)
{
  SETM(Y,-8,-8);cl++;
}
void i_d_5_M1_Y(void)
{
  SETM(Y,-8,-7);cl++;
}
void i_d_5_M2_Y(void)
{
  SETM(Y,-8,-6);cl++;
}
void i_d_5_M3_Y(void)
{
  SETM(Y,-8,-5);cl++;
}
void i_d_5_M4_Y(void)
{
  SETM(Y,-8,-4);cl++;
}
void i_d_5_M5_Y(void)
{
  SETM(Y,-8,-3);cl++;
}
void i_d_5_M6_Y(void)
{
  SETM(Y,-8,-2);cl++;
}
void i_d_5_M7_Y(void)
{
  SETM(Y,-8,-1);cl++;
}
void i_d_5_M8_Y(void)
{
  SETM(Y,-8,0);cl++;
}
void i_d_5_M9_Y(void)
{
  SETM(Y,-7,0);cl++;
}
void i_d_5_MA_Y(void)
{
  SETM(Y,-6,0);cl++;
}
void i_d_5_MB_Y(void)
{
  SETM(Y,-5,0);cl++;
}
void i_d_5_MC_Y(void)
{
  SETM(Y,-4,0);cl++;
}
void i_d_5_MD_Y(void)
{
  SETM(Y,-3,0);cl++;
}
void i_d_5_ME_Y(void)
{
  SETM(Y,-2,0);cl++;
}
void i_d_5_MF_Y(void)
{
  SETM(Y,-1,0);cl++;
}
/* 5 bits U */
void i_d_5_0_U(void)
{
  M=U;cl++;
}
void i_d_5_1_U(void)
{
  SETM(U,1,0);cl++;
}
void i_d_5_2_U(void)
{
  SETM(U,2,0);cl++;
}
void i_d_5_3_U(void)
{
  SETM(U,3,0);cl++;
}
void i_d_5_4_U(void)
{
  SETM(U,4,0);cl++;
}
void i_d_5_5_U(void)
{
  SETM(U,5,0);cl++;
}
void i_d_5_6_U(void)
{
  SETM(U,6,0);cl++;
}
void i_d_5_7_U(void)
{
  SETM(U,7,0);cl++;
}
void i_d_5_8_U(void)
{
  SETM(U,8,0);cl++;
}
void i_d_5_9_U(void)
{
  SETM(U,8,1);cl++;
}
void i_d_5_A_U(void)
{
  SETM(U,8,2);cl++;
}
void i_d_5_B_U(void)
{
  SETM(U,8,3);cl++;
}
void i_d_5_C_U(void)
{
  SETM(U,8,4);cl++;
}
void i_d_5_D_U(void)
{
  SETM(U,8,5);cl++;
}
void i_d_5_E_U(void)
{
  SETM(U,8,6);cl++;
}
void i_d_5_F_U(void)
{
  SETM(U,8,7);cl++;
}

void i_d_5_M0_U(void)
{
  SETM(U,-8,-8);cl++;
}
void i_d_5_M1_U(void)
{
  SETM(U,-8,-7);cl++;
}
void i_d_5_M2_U(void)
{
  SETM(U,-8,-6);cl++;
}
void i_d_5_M3_U(void)
{
  SETM(U,-8,-5);cl++;
}
void i_d_5_M4_U(void)
{
  SETM(U,-8,-4);cl++;
}
void i_d_5_M5_U(void)
{
  SETM(U,-8,-3);cl++;
}
void i_d_5_M6_U(void)
{
  SETM(U,-8,-2);cl++;
}
void i_d_5_M7_U(void)
{
  SETM(U,-8,-1);cl++;
}
void i_d_5_M8_U(void)
{
  SETM(U,-8,0);cl++;
}
void i_d_5_M9_U(void)
{
  SETM(U,-7,0);cl++;
}
void i_d_5_MA_U(void)
{
  SETM(U,-6,0);cl++;
}
void i_d_5_MB_U(void)
{
  SETM(U,-5,0);cl++;
}
void i_d_5_MC_U(void)
{
  SETM(U,-4,0);cl++;
}
void i_d_5_MD_U(void)
{
  SETM(U,-3,0);cl++;
}
void i_d_5_ME_U(void)
{
  SETM(U,-2,0);cl++;
}
void i_d_5_MF_U(void)
{
  SETM(U,-1,0);cl++;
}

/* 5 bits S */
void i_d_5_0_S(void)
{
  M=S;cl++;
}
void i_d_5_1_S(void)
{
  SETM(S,1,0);cl++;
}
void i_d_5_2_S(void)
{
  SETM(S,2,0);cl++;
}
void i_d_5_3_S(void)
{
  SETM(S,3,0);cl++;
}
void i_d_5_4_S(void)
{
  SETM(S,4,0);cl++;
}
void i_d_5_5_S(void)
{
  SETM(S,5,0);cl++;
}
void i_d_5_6_S(void)
{
  SETM(S,6,0);cl++;
}
void i_d_5_7_S(void)
{
  SETM(S,7,0);cl++;
}
void i_d_5_8_S(void)
{
  SETM(S,8,0);cl++;
}
void i_d_5_9_S(void)
{
  SETM(S,8,1);cl++;
}
void i_d_5_A_S(void)
{
  SETM(S,8,2);cl++;
}
void i_d_5_B_S(void)
{
  SETM(S,8,3);cl++;
}
void i_d_5_C_S(void)
{
  SETM(S,8,4);cl++;
}
void i_d_5_D_S(void)
{
  SETM(S,8,5);cl++;
}
void i_d_5_E_S(void)
{
  SETM(S,8,6);cl++;
}
void i_d_5_F_S(void)
{
  SETM(S,8,7);cl++;
}

void i_d_5_M0_S(void)
{
  SETM(S,-8,-8);cl++;
}
void i_d_5_M1_S(void)
{
  SETM(S,-8,-7);cl++;
}
void i_d_5_M2_S(void)
{
  SETM(S,-8,-6);cl++;
}
void i_d_5_M3_S(void)
{
  SETM(S,-8,-5);cl++;
}
void i_d_5_M4_S(void)
{
  SETM(S,-8,-4);cl++;
}
void i_d_5_M5_S(void)
{
  SETM(S,-8,-3);cl++;
}
void i_d_5_M6_S(void)
{
  SETM(S,-8,-2);cl++;
}
void i_d_5_M7_S(void)
{
  SETM(S,-8,-1);cl++;
}
void i_d_5_M8_S(void)
{
  SETM(S,-8,0);cl++;
}
void i_d_5_M9_S(void)
{
  SETM(S,-7,0);cl++;
}
void i_d_5_MA_S(void)
{
  SETM(S,-6,0);cl++;
}
void i_d_5_MB_S(void)
{
  SETM(S,-5,0);cl++;
}
void i_d_5_MC_S(void)
{
  SETM(S,-4,0);cl++;
}
void i_d_5_MD_S(void)
{
  SETM(S,-3,0);cl++;
}
void i_d_5_ME_S(void)
{
  SETM(S,-2,0);cl++;
}
void i_d_5_MF_S(void)
{
  SETM(S,-1,0);cl++;
}

/* PC relatif direct */
void i_d_PC8(void)
{
  M=LOAD8(PC);++PC;
#if defined(REG_M) && defined(REG_PC)
  __asm__ __volatile__("extw "REG_M"; addw "REG_PC","REG_M);
#else
  LSW(M,PC+(char)M);
#endif
  cl++;
}

void i_d_PC16(void)
{
  LD_16(M,PC);++PC;
#if defined(REG_M) && defined(REG_PC)
  __asm__ __volatile__("addw "REG_PC","REG_M);
#else
  LSW(M,PC+M);
#endif
  cl+=5;
}

/* PC relatif indirect */
void i_i_PC8(void)
{
  m=(char)LOAD8(PC);++PC;m+=PC;
  LD_16(M,m);cl+=5;
}
void i_i_PC16(void)
{
  /*int*/
  LD_16(m,PC);++PC; m+=PC;
  LD_16(M,m);
  cl+=8;
}
/* etendu indirect */
void i_i_e16(void)
{
  LD_16(m,PC);++PC;
  LD_16(M,m);
  cl+=5;
}


/* auto-increment X */
void i_d_P1_X(void)
{
  M=X; ADDS(X,1);
  cl+=2;
}
void i_d_P2_X(void)
{
  M=X; ADDS(X,2);
  cl+=3;
}
void i_d_M1_X(void)
{
  ADDS(X,-1); M=X;
  cl+=2;
}
void i_d_M2_X(void)
{
  ADDS(X,-2); M=X;
  cl+=3;
}
void i_d_A_X(void)
{
  ADD_BYTE_REG(M,A,X);
  cl+=1;
}
void i_d_B_X(void)
{
  ADD_BYTE_REG(M,B,X);
  cl+=1;
}
void i_d_D_X(void)
{
  CALC_D(M);
  ADD_WORD(M,X);
  cl+=4;
}

void i_d_8_X(void)
{
  M=LOAD8(PC);++PC;
  ADD_BYTE(M,X);
  cl+=1;
}
void i_d_16_X(void)
{
  LD_16(M,PC);++PC;
  ADD_WORD(M,X); cl+=4;
}

/* Indirect */
/* auto-increment X */
void i_i_P2_X(void)
{ /* sam: */
  m=X;LD_16(M,m);++m;X=m;
  cl+=6;
}
void i_i_M2_X(void)
{
  ADDS(X,-2);m=X;
  LD_16(M,m);
  cl+=6;
}

void i_i_0_X(void)
{
  m=X;LD_16(M,m);
  cl+=3;
}
void i_i_A_X(void)
{
  ADD_BYTE_REG(m,A,X);
  LD_16(M,m);
  cl+=4;
}
void i_i_B_X(void)
{
  ADD_BYTE_REG(m,B,X);
  LD_16(M,m);
  cl+=4;
}
void i_i_D_X(void)
{
  /*int m*/
  CALC_D(m); ADD_WORD(m,X);
  LD_16(M,m);
  cl+=7;
}

void i_i_8_X(void)
{
  m=(char)LOAD8(PC); ++PC;
  ADD_WORD(m,X);
  LD_16(M,m);
  cl+=4;
}
void i_i_16_X(void)
{
  LD_16(m,PC); ADD_WORD(m,X);
  LD_16(M,m);
  cl+=7;
}

/* auto-increment Y */
void i_d_P1_Y(void)
{
  M=Y; ADDS(Y,1);
  cl+=2;
}
void i_d_P2_Y(void)
{
  M=Y; ADDS(Y,2);
  cl+=3;
}
void i_d_M1_Y(void)
{
  ADDS(Y,-1); M=Y;cl+=2;
}
void i_d_M2_Y(void)
{
  ADDS(Y,-2); M=Y;
  cl+=3;
}

void i_d_A_Y(void)
{
  ADD_BYTE_REG(M,A,Y);
  cl++;
}
void i_d_B_Y(void)
{
  ADD_BYTE_REG(M,B,Y);
  cl++;
}
void i_d_D_Y(void)
{
  CALC_D(M);
  ADD_WORD(M,Y);
  cl+=2;
}

void i_d_8_Y(void)
{
  M=LOAD8(PC);++PC;
  ADD_BYTE(M,Y);cl++;
}
void i_d_16_Y(void)
{
  LD_16(M,PC);++PC;
  ADD_WORD(M,Y); cl+=4;
}

/* Indirect */
/* auto-increment Y */
void i_i_P2_Y(void)
{
  m=Y;LD_16(M,m);
  ADDS(Y,2);cl+=6;
}
void i_i_M2_Y(void)
{
  ADDS(Y,-2);m=Y;
  LD_16(M,m);
  cl+=6;
}

void i_i_0_Y(void)
{
  m=Y;LD_16(M,m);
  cl+=3;
}
void i_i_A_Y(void)
{
  ADD_BYTE_REG(m,A,Y);
  LD_16(M,m);
  cl+=4;
}
void i_i_B_Y(void)
{
  ADD_BYTE_REG(m,B,Y);
  LD_16(M,m);
  cl+=4;
}
void i_i_D_Y(void)
{
  
  CALC_D(m);
  LD_16(M,m);
  cl+=7;
}

void i_i_8_Y(void)
{
  m=(char)LOAD8(PC);++PC;
  ADD_WORD(m,Y);
  LD_16(M,m);
  cl+=4;
}
void i_i_16_Y(void)
{
  LD_16(m,PC);++PC;
  ADD_WORD(m,Y);
  LD_16(M,m);
  cl+=7;
}

/* auto-increment U */
void i_d_P1_U(void)
{
  M=U; ADDS(U,1);
  cl+=2;
}
void i_d_P2_U(void)
{
  M=U; ADDS(U,2);
  cl+=3;
}
void i_d_M1_U(void)
{
  ADDS(U,-1); M=U;
}
void i_d_M2_U(void)
{
  ADDS(U,-2); M=U;
  cl+=3;
}

void i_d_A_U(void)
{
  ADD_BYTE_REG(M,A,U); cl++;
}
void i_d_B_U(void)
{
  ADD_BYTE_REG(M,B,U);cl++;
}
void i_d_D_U(void)
{
  CALC_D(M);
  ADD_WORD(M,U); cl+=4;
}

void i_d_8_U(void)
{
  M=LOAD8(PC); ++PC;
  ADD_BYTE(M,U); cl++;
}
void i_d_16_U(void)
{
  LD_16(M,PC);++PC;
  ADD_WORD(M,U);cl+=4;
}

/* Indirect */
/* auto-increment U */
void i_i_P2_U(void)
{
  m=U; LD_16(M,m);++m;U=m;
  cl+=6;
}
void i_i_M2_U(void)
{
  ADDS(U,-2);
  m=U;LD_16(M,m);
  cl+=6;
}

void i_i_0_U(void)
{
  m=U;LD_16(M,m);
  cl+=3;
}
void i_i_A_U(void)
{
  ADD_BYTE_REG(m,A,U);
  LD_16(M,m);
  cl+=4;
}
void i_i_B_U(void)
{
  ADD_BYTE_REG(m,B,U);
  LD_16(M,m);
  cl+=4;
}
void i_i_D_U(void)
{
  /*int*/
  CALC_D(m); ADD_WORD(m,U);
  LD_16(M,m);
  cl+=7;
}

void i_i_8_U(void)
{
  m=(char)LOAD8(PC);ADD_WORD(m,U);PC += 1;
  LD_16(M,m);
  cl+=4;
}
void i_i_16_U(void)
{
  LD_16(m,PC);++PC;
  ADD_WORD(m,U);
  LD_16(M,m);
  cl+=7;
}
/* auto-increment S */
void i_d_P1_S(void)
{
  M=S; ADDS(S,1);
  cl+=2;
}
void i_d_P2_S(void)
{
  M=S; ADDS(S,2);
  cl+=3;
}
void i_d_M1_S(void)
{
  ADDS(S,-1); M=S;
  cl+=2;
}
void i_d_M2_S(void)
{
  ADDS(S,-2); M=S;
  cl+=3;
}

void i_d_A_S(void)
{
  ADD_BYTE_REG(M,A,S); cl++;
}
void i_d_B_S(void)
{
  ADD_BYTE_REG(M,B,S); cl++;
}
void i_d_D_S(void)
{
  CALC_D(M);
  ADD_WORD(M,S); cl+=4;
}

void i_d_8_S(void)
{
  M=LOAD8(PC);++PC;
  ADD_BYTE(M,S); cl++;
}
void i_d_16_S(void)
{
  LD_16(M,PC);++PC;
  ADD_WORD(M,S); cl+=4;
}

/* Indirect */
/* auto-increment S */
void i_i_P2_S(void)
{
  m=S;LD_16(M,m);++m;S=m;
  cl+=6;
}
void i_i_M2_S(void)
{
  ADDS(S,-2);
  m=S;LD_16(M,m);
  cl+=6;
}

void i_i_0_S(void)
{
  m=S;LD_16(M,m);
  cl+=3;
}
void i_i_A_S(void)
{
  ADD_BYTE_REG(m,A,S);
  LD_16(M,m);
  cl+=4;
}
void i_i_B_S(void)
{
  ADD_BYTE_REG(m,B,S);
  LD_16(M,m);
  cl+=4;
}
void i_i_D_S(void)
{
  CALC_D(m);ADD_WORD(m,S);
  LD_16(M,m);
  cl+=7;
}

void i_i_8_S(void)
{
  m=(char)LOAD8(PC);ADD_WORD(m,S);++PC;
  LD_16(M,m);
  cl+=4;
}
void i_i_16_S(void)
{
  LD_16(m,PC);++PC;
  ADD_WORD(m,S);
  LD_16(M,m);
  cl+=7;
}

void i_undoc(void)
{
  /*
  printf("Illegal indexed mode");
  crash();
  */
  M=0;
}

void setup_index(void)
{
  INDEX[0x00]=i_d_5_0_X;
  INDEX[0x01]=i_d_5_1_X;
  INDEX[0x02]=i_d_5_2_X;
  INDEX[0x03]=i_d_5_3_X;
  INDEX[0x04]=i_d_5_4_X;
  INDEX[0x05]=i_d_5_5_X;
  INDEX[0x06]=i_d_5_6_X;
  INDEX[0x07]=i_d_5_7_X;

  INDEX[0x08]=i_d_5_8_X;
  INDEX[0x09]=i_d_5_9_X;
  INDEX[0x0A]=i_d_5_A_X;
  INDEX[0x0B]=i_d_5_B_X;
  INDEX[0x0C]=i_d_5_C_X;
  INDEX[0x0D]=i_d_5_D_X;
  INDEX[0x0E]=i_d_5_E_X;
  INDEX[0x0F]=i_d_5_F_X;

  INDEX[0x10]=i_d_5_M0_X;
  INDEX[0x11]=i_d_5_M1_X;
  INDEX[0x12]=i_d_5_M2_X;
  INDEX[0x13]=i_d_5_M3_X;
  INDEX[0x14]=i_d_5_M4_X;
  INDEX[0x15]=i_d_5_M5_X;
  INDEX[0x16]=i_d_5_M6_X;
  INDEX[0x17]=i_d_5_M7_X;

  INDEX[0x18]=i_d_5_M8_X;
  INDEX[0x19]=i_d_5_M9_X;
  INDEX[0x1A]=i_d_5_MA_X;
  INDEX[0x1B]=i_d_5_MB_X;
  INDEX[0x1C]=i_d_5_MC_X;
  INDEX[0x1D]=i_d_5_MD_X;
  INDEX[0x1E]=i_d_5_ME_X;
  INDEX[0x1F]=i_d_5_MF_X;

  INDEX[0x20]=i_d_5_0_Y;
  INDEX[0x21]=i_d_5_1_Y;
  INDEX[0x22]=i_d_5_2_Y;
  INDEX[0x23]=i_d_5_3_Y;
  INDEX[0x24]=i_d_5_4_Y;
  INDEX[0x25]=i_d_5_5_Y;
  INDEX[0x26]=i_d_5_6_Y;
  INDEX[0x27]=i_d_5_7_Y;

  INDEX[0x28]=i_d_5_8_Y;
  INDEX[0x29]=i_d_5_9_Y;
  INDEX[0x2A]=i_d_5_A_Y;
  INDEX[0x2B]=i_d_5_B_Y;
  INDEX[0x2C]=i_d_5_C_Y;
  INDEX[0x2D]=i_d_5_D_Y;
  INDEX[0x2E]=i_d_5_E_Y;
  INDEX[0x2F]=i_d_5_F_Y;

  INDEX[0x30]=i_d_5_M0_Y;
  INDEX[0x31]=i_d_5_M1_Y;
  INDEX[0x32]=i_d_5_M2_Y;
  INDEX[0x33]=i_d_5_M3_Y;
  INDEX[0x34]=i_d_5_M4_Y;
  INDEX[0x35]=i_d_5_M5_Y;
  INDEX[0x36]=i_d_5_M6_Y;
  INDEX[0x37]=i_d_5_M7_Y;

  INDEX[0x38]=i_d_5_M8_Y;
  INDEX[0x39]=i_d_5_M9_Y;
  INDEX[0x3A]=i_d_5_MA_Y;
  INDEX[0x3B]=i_d_5_MB_Y;
  INDEX[0x3C]=i_d_5_MC_Y;
  INDEX[0x3D]=i_d_5_MD_Y;
  INDEX[0x3E]=i_d_5_ME_Y;
  INDEX[0x3F]=i_d_5_MF_Y;

  INDEX[0x40]=i_d_5_0_U;
  INDEX[0x41]=i_d_5_1_U;
  INDEX[0x42]=i_d_5_2_U;
  INDEX[0x43]=i_d_5_3_U;
  INDEX[0x44]=i_d_5_4_U;
  INDEX[0x45]=i_d_5_5_U;
  INDEX[0x46]=i_d_5_6_U;
  INDEX[0x47]=i_d_5_7_U;

  INDEX[0x48]=i_d_5_8_U;
  INDEX[0x49]=i_d_5_9_U;
  INDEX[0x4A]=i_d_5_A_U;
  INDEX[0x4B]=i_d_5_B_U;
  INDEX[0x4C]=i_d_5_C_U;
  INDEX[0x4D]=i_d_5_D_U;
  INDEX[0x4E]=i_d_5_E_U;
  INDEX[0x4F]=i_d_5_F_U;

  INDEX[0x50]=i_d_5_M0_U;
  INDEX[0x51]=i_d_5_M1_U;
  INDEX[0x52]=i_d_5_M2_U;
  INDEX[0x53]=i_d_5_M3_U;
  INDEX[0x54]=i_d_5_M4_U;
  INDEX[0x55]=i_d_5_M5_U;
  INDEX[0x56]=i_d_5_M6_U;
  INDEX[0x57]=i_d_5_M7_U;

  INDEX[0x58]=i_d_5_M8_U;
  INDEX[0x59]=i_d_5_M9_U;
  INDEX[0x5A]=i_d_5_MA_U;
  INDEX[0x5B]=i_d_5_MB_U;
  INDEX[0x5C]=i_d_5_MC_U;
  INDEX[0x5D]=i_d_5_MD_U;
  INDEX[0x5E]=i_d_5_ME_U;
  INDEX[0x5F]=i_d_5_MF_U;

  INDEX[0x60]=i_d_5_0_S;
  INDEX[0x61]=i_d_5_1_S;
  INDEX[0x62]=i_d_5_2_S;
  INDEX[0x63]=i_d_5_3_S;
  INDEX[0x64]=i_d_5_4_S;
  INDEX[0x65]=i_d_5_5_S;
  INDEX[0x66]=i_d_5_6_S;
  INDEX[0x67]=i_d_5_7_S;

  INDEX[0x68]=i_d_5_8_S;
  INDEX[0x69]=i_d_5_9_S;
  INDEX[0x6A]=i_d_5_A_S;
  INDEX[0x6B]=i_d_5_B_S;
  INDEX[0x6C]=i_d_5_C_S;
  INDEX[0x6D]=i_d_5_D_S;
  INDEX[0x6E]=i_d_5_E_S;
  INDEX[0x6F]=i_d_5_F_S;

  INDEX[0x70]=i_d_5_M0_S;
  INDEX[0x71]=i_d_5_M1_S;
  INDEX[0x72]=i_d_5_M2_S;
  INDEX[0x73]=i_d_5_M3_S;
  INDEX[0x74]=i_d_5_M4_S;
  INDEX[0x75]=i_d_5_M5_S;
  INDEX[0x76]=i_d_5_M6_S;
  INDEX[0x77]=i_d_5_M7_S;

  INDEX[0x78]=i_d_5_M8_S;
  INDEX[0x79]=i_d_5_M9_S;
  INDEX[0x7A]=i_d_5_MA_S;
  INDEX[0x7B]=i_d_5_MB_S;
  INDEX[0x7C]=i_d_5_MC_S;
  INDEX[0x7D]=i_d_5_MD_S;
  INDEX[0x7E]=i_d_5_ME_S;
  INDEX[0x7F]=i_d_5_MF_S;

  INDEX[0x80]=i_d_P1_X;
  INDEX[0x81]=i_d_P2_X;
  INDEX[0x82]=i_d_M1_X;
  INDEX[0x83]=i_d_M2_X;
  INDEX[0x84]=i_d_X;
  INDEX[0x85]=i_d_B_X;
  INDEX[0x86]=i_d_A_X;
  INDEX[0x87]=i_undoc;	/* empty */
  INDEX[0x88]=i_d_8_X;
  INDEX[0x89]=i_d_16_X;
  INDEX[0x8A]=i_undoc;	/* empty */
  INDEX[0x8B]=i_d_D_X;
  INDEX[0x8C]=i_d_PC8;
  INDEX[0x8D]=i_d_PC16;
  INDEX[0x8E]=i_undoc;	/* empty */
  INDEX[0x8F]=i_undoc;	/* empty */
  INDEX[0x90]=i_undoc;	/* empty */
  INDEX[0x91]=i_i_P2_X;
  INDEX[0x92]=i_undoc;	/* empty */
  INDEX[0x93]=i_i_M2_X;
  INDEX[0x94]=i_i_0_X;
  INDEX[0x95]=i_i_B_X;
  INDEX[0x96]=i_i_A_X;
  INDEX[0x97]=i_undoc;	/* empty */
  INDEX[0x98]=i_i_8_X;
  INDEX[0x99]=i_i_16_X;
  INDEX[0x9A]=i_undoc;	/* empty */
  INDEX[0x9B]=i_i_D_X;
  INDEX[0x9C]=i_i_PC8;
  INDEX[0x9D]=i_i_PC16;
  INDEX[0x9E]=i_undoc;	/* empty */
  INDEX[0x9F]=i_i_e16;

  INDEX[0xA0]=i_d_P1_Y;
  INDEX[0xA1]=i_d_P2_Y;
  INDEX[0xA2]=i_d_M1_Y;
  INDEX[0xA3]=i_d_M2_Y;
  INDEX[0xA4]=i_d_Y;
  INDEX[0xA5]=i_d_B_Y;
  INDEX[0xA6]=i_d_A_Y;
  INDEX[0xA7]=i_undoc;	/* empty */
  INDEX[0xA8]=i_d_8_Y;
  INDEX[0xA9]=i_d_16_Y;
  INDEX[0xAA]=i_undoc;	/* empty */
  INDEX[0xAB]=i_d_D_Y;
  INDEX[0xAC]=i_d_PC8;
  INDEX[0xAD]=i_d_PC16;
  INDEX[0xAE]=i_undoc;	/* empty */
  INDEX[0xAF]=i_undoc;	/* empty */
  INDEX[0xB0]=i_undoc;	/* empty */
  INDEX[0xB1]=i_i_P2_Y;
  INDEX[0xB2]=i_undoc;	/* empty */
  INDEX[0xB3]=i_i_M2_Y;
  INDEX[0xB4]=i_i_0_Y;
  INDEX[0xB5]=i_i_B_Y;
  INDEX[0xB6]=i_i_A_Y;
  INDEX[0xB7]=i_undoc;	/* empty */
  INDEX[0xB8]=i_i_8_Y;
  INDEX[0xB9]=i_i_16_Y;
  INDEX[0xBA]=i_undoc;	/* empty */
  INDEX[0xBB]=i_i_D_Y;
  INDEX[0xBC]=i_i_PC8;
  INDEX[0xBD]=i_i_PC16;
  INDEX[0xBE]=i_undoc;	/* empty */
  INDEX[0xBF]=i_i_e16;

  INDEX[0xC0]=i_d_P1_U;
  INDEX[0xC1]=i_d_P2_U;
  INDEX[0xC2]=i_d_M1_U;
  INDEX[0xC3]=i_d_M2_U;
  INDEX[0xC4]=i_d_U;
  INDEX[0xC5]=i_d_B_U;
  INDEX[0xC6]=i_d_A_U;
  INDEX[0xC7]=i_undoc;	/* empty */
  INDEX[0xC8]=i_d_8_U;
  INDEX[0xC9]=i_d_16_U;
  INDEX[0xCA]=i_undoc;	/* empty */
  INDEX[0xCB]=i_d_D_U;
  INDEX[0xCC]=i_d_PC8;
  INDEX[0xCD]=i_d_PC16;
  INDEX[0xCE]=i_undoc;	/* empty */
  INDEX[0xCF]=i_undoc;	/* empty */
  INDEX[0xD0]=i_undoc;	/* empty */
  INDEX[0xD1]=i_i_P2_U;
  INDEX[0xD2]=i_undoc;	/* empty */
  INDEX[0xD3]=i_i_M2_U;
  INDEX[0xD4]=i_i_0_U;
  INDEX[0xD5]=i_i_B_U;
  INDEX[0xD6]=i_i_A_U;
  INDEX[0xD7]=i_undoc;	/* empty */
  INDEX[0xD8]=i_i_8_U;
  INDEX[0xD9]=i_i_16_U;
  INDEX[0xDA]=i_undoc;	/* empty */
  INDEX[0xDB]=i_i_D_U;
  INDEX[0xDC]=i_i_PC8;
  INDEX[0xDD]=i_i_PC16;
  INDEX[0xDE]=i_undoc;	/* empty */
  INDEX[0xDF]=i_i_e16;

  INDEX[0xE0]=i_d_P1_S;
  INDEX[0xE1]=i_d_P2_S;
  INDEX[0xE2]=i_d_M1_S;
  INDEX[0xE3]=i_d_M2_S;
  INDEX[0xE4]=i_d_S;
  INDEX[0xE5]=i_d_B_S;
  INDEX[0xE6]=i_d_A_S;
  INDEX[0xE7]=i_undoc;	/* empty */
  INDEX[0xE8]=i_d_8_S;
  INDEX[0xE9]=i_d_16_S;
  INDEX[0xEA]=i_undoc;	/* empty */
  INDEX[0xEB]=i_d_D_S;
  INDEX[0xEC]=i_d_PC8;
  INDEX[0xED]=i_d_PC16;
  INDEX[0xEE]=i_undoc;	/* empty */
  INDEX[0xEF]=i_undoc;	/* empty */
  INDEX[0xF0]=i_undoc;	/* empty */
  INDEX[0xF1]=i_i_P2_S;
  INDEX[0xF2]=i_undoc;	/* empty */
  INDEX[0xF3]=i_i_M2_S;
  INDEX[0xF4]=i_i_0_S;
  INDEX[0xF5]=i_i_B_S;
  INDEX[0xF6]=i_i_A_S;
  INDEX[0xF7]=i_undoc;	/* empty */
  INDEX[0xF8]=i_i_8_S;
  INDEX[0xF9]=i_i_16_S;
  INDEX[0xFA]=i_undoc;	/* empty */
  INDEX[0xFB]=i_i_D_S;
  INDEX[0xFC]=i_i_PC8;
  INDEX[0xFD]=i_i_PC16;
  INDEX[0xFE]=i_undoc;	/* empty */
  INDEX[0xFF]=i_i_e16;

}
