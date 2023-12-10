/*

	VERSION     : 2.0
	MODULE      : sound.h
	Cree par    : Gilles FETIS
	Modifie par :

*/

#define FREQ 10000

extern int thomsound;

extern int * BSTORE;

extern int soundbufsize;

#ifdef AMIGA
void REGPARM output_sound(int value);
#endif

extern void init_thom_sound(void);

extern void SWAP_SOUND(void);
extern void CALC_SOUND(void);
extern void CLEAR_SOUND(void);
