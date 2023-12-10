/*
 * main.h: header fonctionnel et parametres de l'emulation
 */

#ifndef MAIN_H
#define MAIN_H

#define VERSION "Teo v0.6 alpha Aout 1998"
#define VERNUM  'f'

#define VIDEO_FRAME_FREQ 50 /* Hz: frequence de synchronisation video */

#define CYCLES_PER_VIDEO_FRAME 19968
 /* duree exacte en nb de cycles CPU d'une frame video */

#define LINES_IN_TOP_SCREEN_BORDER 50

#define LINES_IN_SCREEN_WINDOW 200

#define LINES_IN_BOTTOM_SCREEN_BORDER 50

#define CYCLES_PER_LINE 64
 /* duree exacte en nb de cycles CPU du tracage d'une ligne */

#define BYTES_PER_LINE 40
 /* nombre d'octets de vram par ligne mais aussi nombre de cycles necessaires
    a leur tracage car la vitesse est de 1 octet/cycle */

#define LINE_GRANULARITY 8
 /* nombre d'octets affiches lors d'un rafraichissement elementaire */

#define CYCLES_PER_VIDEO_IRQ 80
 /* On fait durer 80 cycles d'horloge la requete d'interruption: en effet, en
    mode editeur BASIC, le TO8 passe son temps a appeler la fonction GETCS
    (lecture du clavier) qui se trouve en EFC1 bank 1 du moniteur; or la ban-
    que active est la bank 0 donc le TO8 commute en permanence les banques.
    La fonction de commutation est en FFA0 bank 0 et force par deux fois le
    bit I de la CC a 1; lorsque l'IRQ tombe au milieu, elle est rejetee par
    le MC6809 et on perd soit une lecture clavier soit un clignotement.
    30 cycles sont suffisamment longs pour attendre la redescente du bit I
    et en meme temps suffisamment courts pour ne pas declencher deux IRQs */

#define CURSOR_FREQ_RATIO  5 /* par rapport a la frequence video */

#define MODE80_DELAY  3 /* sec; delai de transition 80 colonnes */

/* sam: apparemment TRUE n'est pas toujours defini */
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifdef USE_ALLEGRO
#define VBL_INT /* sam: la vbl est generee par interrupt */
#endif

extern
#ifdef VBL_INT
volatile
#endif
int vbl;

extern void crash(void);

extern void perif(void);

extern void ColdReset(void); /* sam: pour amiga/gui.c */

#endif /* MAIN_H */
