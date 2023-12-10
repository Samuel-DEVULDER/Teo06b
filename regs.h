/*

	VERSION     : 2.0
	MODULE      : regs.h
	Cree par    : Gilles FETIS
	Modifie par :

*/


extern unsigned int A,B,DP,CC;
extern unsigned int Y,U,S,D;

#ifndef REG_m
extern unsigned int m;
#endif
#ifndef REG_M
extern unsigned int M;
#endif
#ifndef REG_PC
extern unsigned int PC;
#endif
#ifndef REG_X
extern unsigned int X;
#endif

/* emulation CC rapide */
/*
extern int ccrest;
extern unsigned short res;
extern unsigned char m1,m2,sign,ovfl,h1,h2;
*/
extern  int     res,m1,m2,sign,ovfl,h1,h2,ccrest;
extern  int     *(exreg[16]);
