/*
 * mouse.h: header fonctionnel de la gestion souris
 */

#ifndef MOUSE_H
#define MOUSE_H

#define MOUSE    1
#define LIGHTPEN 2
#define PREVIOUS 3

extern void InitPointer(void);

extern void InstallPointer(int);

extern void ShutDownPointer(void);

#ifdef AMIGA
#ifdef FAST_MOUSE
#include <intuition/screens.h>
extern struct Screen *Scr;
#define mouse_x (Scr->MouseX*(((GE7DC==0x2A) && (Scr->Width<640))?2:1))
#define mouse_y (Scr->MouseY*(((GE7DC==0x2A) && (Scr->Height<480))?2:1))
#else
extern int mouse_x; /* from event.c */
extern int mouse_y;
#endif
#endif

#endif

