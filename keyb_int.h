/*
 * keyb_int.h: header fonctionnel du module d'interruption clavier
 */

#ifndef KEYB_INT_H
#define KEYB_INT_H

#define BIOS_KB    0
#define ALLEGRO_KB 1
#define TO8_KB     2

extern void DOS_InitKeyboard(void (* )(int, int));

extern void DOS_InstallKeyboard(int);

#ifdef Xwin_ALLEGRO
void install_XwinAllegKEY();
#endif

/* sam: je ne sais pas trop ou declarer cela, mais etant donne
que eval_events() simule entre autre les interruptions clavier, 
ici cela ne me semble pas trop mal */
#ifdef AMIGA
extern void handle_events(void);
extern int  diskled_timeout;
#endif

/* scancodes des touches passes par le handler d'interruption */
#define KEY__ESC                1
#define KEY__1                  2
#define KEY__2                  3
#define KEY__3                  4
#define KEY__4                  5
#define KEY__5                  6
#define KEY__6                  7
#define KEY__7                  8
#define KEY__8                  9
#define KEY__9                 10
#define KEY__0                 11
#define KEY__CLOSEBRACE        12
#define KEY__EQUALS            13
#define KEY__BACKSPACE         14
#define KEY__TAB               15 
#define KEY__A                 16
#define KEY__Z                 17
#define KEY__E                 18
#define KEY__R                 19
#define KEY__T                 20
#define KEY__Y                 21
#define KEY__U                 22
#define KEY__I                 23
#define KEY__O                 24
#define KEY__P                 25
#define KEY__HAT               26
#define KEY__DOLLAR            27
#define KEY__ENTER             28
#define KEY__LCONTROL          29
#define KEY__Q                 30
#define KEY__S                 31
#define KEY__D                 32
#define KEY__F                 33
#define KEY__G                 34
#define KEY__H                 35
#define KEY__J                 36
#define KEY__K                 37
#define KEY__L                 38
#define KEY__M		       39
#define KEY__PERCENT	       40
#define KEY__SQUARE	       41
#define KEY__LSHIFT            42
#define KEY__ASTERISK          43
#define KEY__W                 44
#define KEY__X                 45
#define KEY__C                 46
#define KEY__V                 47
#define KEY__B                 48
#define KEY__N                 49
#define KEY__COMMA             50
#define KEY__SEMICOLON         51
#define KEY__COLON             52
#define KEY__EXCLAM            53
#define KEY__RSHIFT            54
#define KEY__ASTERISK_PAD      55	
#define KEY__ALT               56
#define KEY__SPACE             57
#define KEY__CAPSLOCK          58
#define KEY__F1                59
#define KEY__F2                60
#define KEY__F3                61
#define KEY__F4                62
#define KEY__F5                63
#define KEY__F6                64
#define KEY__F7                65
#define KEY__F8                66
#define KEY__F9                67
#define KEY__F10               68
#define KEY__NUMLOCK           69
#define KEY__SCRLOCK           70
#define KEY__7_PAD             71
#define KEY__8_PAD             72
#define KEY__9_PAD             73
#define KEY__MINUS_PAD         74
#define KEY__4_PAD             75
#define KEY__5_PAD             76
#define KEY__6_PAD             77
#define KEY__PLUS_PAD          78
#define KEY__1_PAD             79
#define KEY__2_PAD             80
#define KEY__3_PAD             81
#define KEY__0_PAD             82
#define KEY__POINT_PAD         83
#define KEY__PRTSCR   	       84 /* non transmise sous Windows */

#define KEY__LOWER	       86
#define KEY__F11               87
#define KEY__F12               88

#define KEY__ENTER_PAD	      156
#define KEY__RCONTROL         157

#define KEY__SLASH_PAD        181

#define KEY__ALTGR            184

#define KEY__HOME             199
#define KEY__UP               200
#define KEY__PGUP             201

#define KEY__LEFT             203

#define KEY__RIGHT            205

#define KEY__END              207
#define KEY__DOWN             208
#define KEY__PGDN             209
#define KEY__INSERT           210
#define KEY__DEL              211

#define KEY__PAUSE            /* non transmise sous Windows */


#endif

