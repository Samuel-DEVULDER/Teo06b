/*

	VERSION     : 1.0
	MODULE      : GRAPH.H
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

*/

#ifndef GRAPH_H
#define GRAPH_H

#define REFRESH      0x10000
#define SPECIAL_MODE 0x10000
#define INIT         0x10000
#define MODE80       0x20000


extern int graphic_mode;

extern int mode80_vbl;

extern int screen_refresh;

extern 
#ifdef VBL_INT
volatile 
#endif
int vbl; /* il n'y a pas de main.h */


extern void SetColor(int, int);

extern void SetBorderColor(int);

extern void REGPARM fdraw(unsigned int, BYTE);

#ifdef AMIGA
extern void REGPARM fdraw2(unsigned int adr, BYTE val); /* un draw sans test sur REFRESH */
extern void REGPARM refresh_gpl(unsigned int no);
#else
#define fdraw2 		fdraw
#define refresh_gpl(no) fdraw(0x4000+(no), REFRESH)
#endif

extern void SetGraphicMode(unsigned int);

extern void ReturnText(void);

extern void SetupGraph(void);

#endif
