/*

	VERSION     : 2.0
	MODULE      : keyb.h
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

*/

#ifndef KEYB_H
#define KEYB_H

extern volatile int TO_keypressed;
extern volatile int kb_state;

extern volatile int command;

#define ACTIVATE_MENU  1
#define OPTION_MENU    2
#define DEBUGGER       3
#define GFX_MENU       4

extern void ResetPortManette0(void);

extern void InitKeyboard(void);

extern void InstallKeyboard(void);

extern void ShutDownKeyboard(void);

#ifdef AMIGA
#include <stdio.h>
#define KB_CAPSLOCK_FLAG 	1
#define KB_NUMLOCK_FLAG 	2
#define KB_SHIFT_FLAG		4
#define KB_ALT_FLAG		8
#define set_leds(x) do { printf(((x)&KB_NUMLOCK_FLAG)?\
	"\r\t\t\tMode pavé numérique    \r":\
	"\r\t\t\tMode émulation joystick\r");\
	fflush(stdout);} while(0)
extern void NotifyKey(int key, int release);
#endif

#endif
