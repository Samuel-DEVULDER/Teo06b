/*

	VERSION     : 2.0
	MODULE      : index.h
	Cree par    : Gilles FETIS
	Modifie par : Eric Botcazou

	Fetis G. 02/1996


*/

extern void (*INDEX[256])(void); /* sam: (void) manquait */

extern void setup_index(void);

