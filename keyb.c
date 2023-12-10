

/*

	MARCEL O CINQ
	*************

	l'émulateur de thomson MO5

	Par Gilles FETIS

	mail : FETIS_Gilles@compuserve.com
	page : http://ourworld.compuserve.com/homepages/FETIS_Gilles


*/


/*

	VERSION     : 2.0 Allegro
	MODULE      : keyb.c
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	
*/

#include "flags.h"

#ifdef USE_ALLEGRO
#include <allegro.h>
#endif

#include "mapper.h"
#include "keyb_int.h"
#include "keyb.h"

volatile int TO_keypressed=0;
volatile int command=0;  /* commande a passer a la boucle principale */
volatile int kb_state=0; /* contient l'etat des touches et leds du clavier PC:
                           - SHIFT (gauche et droit confondus)
                           - ALTGr
                           - NUMLOCK
                           - CAPSLOCK                                  */
static volatile int j0_dir[2]; /* 2 dernieres directions de la manette */

static unsigned char key_code[256]={
  0,255, 42, 34, 26, 18, 10,  2, 50, 58, 66, 74, 77, 13,151, 49,
 43, 35, 27, 19, 11,  3, 51, 59, 67, 75, 78, 61, 71,255, 44, 36,
 28, 20, 12,  4, 52, 60, 68, 76, 70,  0,255,189, 48, 40, 32, 24,
 16,  8, 56, 64, 72, 69,255,  0, 21, 53,255, 33,  1,  9, 17, 25,
  0,  0,  0,  0,  0,255,  0,254,254,254,  0,254,254,254,  0,254,
254,254,254,254,  0,  0,208,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 55,255,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  7,  5,  0,  0, 14,  0,  6,  0,  0,
 62,  0, 15, 23};
 /* key_code[] convertit le scancode d'une touche PC (voir keyb_int.h pour
    la liste des scancodes) en la valeur que produit le clavier TO8 pour la
    meme touche; il y a trois valeurs particulieres :
     - 0 designe une touche non mappee sur le TO8,
     - 254 designe une touche du clavier numerique du PC
     - 255 designe une touche speciale du PC (shift, altgr, ...) mais n'a pas
       d'utilisation dans le traitement */

static unsigned char key_altgr_code[128]={0,0,0,0,41,173,45,0,0,197,0,169,63,191};
 /* key_altgr_code[] convertit le scancode d'une touche PC pressee en meme
    temps que Altgr en sa valeur clavier TO8; necessaire car certains signes
    du clavier TO8 sont en Altgr+touche sur le PC (#, @, ...) */

static unsigned char key_pad_code[2][13]={ {29,37,54,0,30,46,47,0,22,38,79,31,39},
                                           {0xa,0xe,0x6,0,0xb,0,0x7,0,0x9,0xd,0x5,0,0} };
 /* key_pad_code[] convertit le scancode d'une touche du clavier numerique PC
    en sa valeur clavier TO8 ou en sa valeur port manette TO8 */


/*
 * ResetPortManette0: remet au repos le port physique manette 0
 */

void ResetPortManette0(void)
{
    ORA2=((ORA2|0xF)&(DDRA2^0xFF))|(ORA2&DDRA2);

    if (CRA2&4)
        MON[0][0x7CC]=MON[1][0x7CC]=ORA2;

    ORB2=((ORB2|0x40)&(DDRB2^0xFF))|(ORB2&DDRB2);

    if (CRB2&4)
        MON[0][0x7CD]=MON[1][0x7CD]=ORB2;

    CRA2|=0x80;
    MON[0][0x7CE]=MON[1][0x7CE]=CRA2;

    j0_dir[0]=j0_dir[1]=0xF;
}

#ifdef USE_ALLEGRO
static END_OF_FUNCTION(ResetPortManette0)
 /* cette ligne sert simplement a reperer la fin de la fonction et est
    utilisee lors du verrouillage en memoire physique; elle n'a pas de valeur
    fonctionnelle */
#endif

/*
 * KeyboardHandler: est appele par l'interruption materielle clavier et gere
 *                  le clavier et les manettes du TO8.
 *                   key: scancode de la touche frappee/relachee
 *                        (voir keyb_int.h pour la liste des scancodes)
 *                   release: flag de relachement de la touche
 */

static void KeyboardHandler(int key, int release)
{
    unsigned char code;

    switch (key)
    {
        case KEY__ESC:
            if (!release)
                command=ACTIVATE_MENU;
            break;

        case KEY__F9:
            if (!release)
                command=(kb_state&KB_SHIFT_FLAG)?GFX_MENU:OPTION_MENU;
            kb_state &= ~KB_SHIFT_FLAG;
            break;

#ifndef NO_DEBUGGER
        case KEY__F10:
            if (!release)
                command=DEBUGGER;
            break;
#endif

        case KEY__LSHIFT:
        case KEY__RSHIFT:
            kb_state=(release ? kb_state&~KB_SHIFT_FLAG : kb_state|KB_SHIFT_FLAG);
            break;

        case KEY__LCONTROL:  /* CNT */
            MON[1][0x1125]=(release ? 0 : 1);
            break;

        case KEY__RCONTROL:  /* le control droit emule le bouton joystick 0
                                en mode manette (NUMLOCK eteint) */
            if (!(kb_state&KB_NUMLOCK_FLAG))
            {
                ORB2=((release ? ORB2|0x40 : ORB2&0xBF)&(DDRB2^0xFF))|(ORB2&DDRB2);

                if (CRB2&4)
                    MON[0][0x7CD]=MON[1][0x7CD]=ORB2;

                CRA2=(release ? CRA2|0x80 : CRA2&0x7F);
                MON[0][0x7CE]=MON[1][0x7CE]=CRA2;
            }
            break;

        case KEY__ALTGR:
            kb_state=(release ? kb_state&~KB_ALT_FLAG : kb_state|KB_ALT_FLAG);
            break;

        case KEY__CAPSLOCK:
            if (!release)
            {
                kb_state^=KB_CAPSLOCK_FLAG;
                set_leds(kb_state); /* on met a jour la led CAPSLOCK */

                PRC=(kb_state&KB_CAPSLOCK_FLAG ? PRC&0xF7 : PRC|0x08);
                MON[0][0x7C3]=MON[1][0x7C3]=PRC;
            }
            break;

        case KEY__NUMLOCK:
            if (!release)
            {
                kb_state^=KB_NUMLOCK_FLAG;
                set_leds(kb_state); /* on met a jour la led NUMLOCK */
                ResetPortManette0();
            }
            break;

        default:
            if ((code=key_code[key])==254)  /* touche du pave numerique ? */
            {
                if (kb_state&KB_NUMLOCK_FLAG)  /* mode pave numerique ? */
                    code=key_pad_code[0][key-KEY__7_PAD];

                else  /* mode manette */
                {
                    if ((code=key_pad_code[1][key-KEY__7_PAD]))
                    {
                        if (release)
                        {
                            if (code == j0_dir[0])
                            {
                                j0_dir[0]=j0_dir[1];
                                j0_dir[1]=0xF;
                            }
                            else if (code == j0_dir[1])
                                j0_dir[1]=0xF;
                        }
                        else
                        {
                            if (code != j0_dir[0])
                            {
                                j0_dir[1]=j0_dir[0];
                                j0_dir[0]=code;
                            }
                        }

                        ORA2=(((ORA2&0xF0)|j0_dir[0])&(DDRA2^0xFF))|(ORA2&DDRA2);
                        if (CRA2&4)
                            MON[0][0x7CC]=MON[1][0x7CC]=ORA2;
                    }
                    break; /* fin du traitement pour le pave numerique en
                              mode manette */
                }
            }
            else if ((kb_state&KB_ALT_FLAG) && key_altgr_code[key])
                     code=key_altgr_code[key];
             /* on remplace le code donne par key_code[] par celui donne par
                key_altgr_code[] si ce dernier est non nul et si Altgr est
                pressee */

	    TO_keypressed=1;

            if (code--)  /* touche mappee ? */
            {
                if (kb_state&KB_SHIFT_FLAG)  /* shift presse ? */
                    code^=0x80;

                if (release)  /* touche relachee ? */
                {
                    if (code == MON[1][0x10F8])
                    {
                        ORA=((ORA&0xFE)&(DDRA^0xFF))|(ORA&DDRA); /* keytest a 0 */

                        if (CRA&4)
                            MON[0][0x7C8]=MON[1][0x7C8]=ORA;
                    }
                }
                else  /* touche appuyee */
                {
                    MON[1][0x10F8]=code;

                    ORA=((ORA|1)&(DDRA^0xFF))|(ORA&DDRA); /* keytest a 1 */

                    if (CRA&4)
                        MON[0][0x7C8]=MON[1][0x7C8]=ORA;

                    port0|=2;            /* output clavier a 1 */
                    MON[0][0x7C0]=MON[1][0x7C0]=port0;
                }
            }
    } /* end of switch */
}

#ifdef USE_ALLEGRO
static END_OF_FUNCTION(KeyboardHandler)
#endif

/*
 * InitKeyboard: effectue quelques modifications mineures de la routine de
 *               lecture clavier TO8 (adresse: 0xF08E bank 1).
 *       MS-DOS: verrouillage de la memoire touchee par l'interruption
 */

void InitKeyboard(void)
{
    int adr[10]={0x0A5,0x0A6,0x0A7,0x0F7,0x124,0x277,0x2DA,0x24F,0x292,0x287};
    unsigned char val[10]={0x7E,0xF0,0xE5,0x86,0x86,0x26,0x2A,0x2D,0x21,0x5F};
    register int i;

    for (i=0; i<10; i++)  /* on modifie la routine clavier TO8 */
        MON[1][0x1000+adr[i]]=val[i];

    kb_state=KB_NUMLOCK_FLAG;

#ifdef DJGPP
    LOCK_VARIABLE(command);
    LOCK_VARIABLE(kb_state);
    LOCK_VARIABLE(key_code);
    LOCK_VARIABLE(key_altgr_code);
    LOCK_VARIABLE(key_pad_code);
    LOCK_FUNCTION(ResetPortManette0);
    LOCK_FUNCTION(KeyboardHandler);
     /* on verrouille en memoire physique tout ce qui est touche pendant
        l'interruption clavier */

    DOS_InitKeyboard(KeyboardHandler);
#endif
}


/*
 * InstallKeyboard: installe le clavier et le port manette du TO8
 */

void InstallKeyboard(void)
{
    port0&=0xFD;                             /* output clavier a 0 */
    MON[0][0x7C0]=MON[1][0x7C0]=port0;

    ORA=((ORA&0xFE)&(DDRA^0xFF))|(ORA&DDRA); /* keytest a 0 */

    if (CRA&4)
        MON[0][0x7C8]=MON[1][0x7C8]=ORA;

    MON[1][0x1125]=0;          /* CNT relachee */

    kb_state&=~KB_SHIFT_FLAG;  /* shift relache */
    set_leds(kb_state);

    ResetPortManette0();       /* touches manette 0 relachees */

    command=0;

#ifdef DJGPP
    DOS_InstallKeyboard(TO8_KB);
#endif
#ifdef Xwin_ALLEGRO
   remove_keyboard();
   install_XwinAllegKEY(KeyboardHandler);
#endif
}


void ShutDownKeyboard(void)
{
#ifdef DJGPP
    DOS_InstallKeyboard(ALLEGRO_KB);
#endif
#ifdef Xwin_ALLEGRO
   install_keyboard();
#endif
}

/*
sam: ceci est utilise dans amiga/events.c
*/
#ifdef AMIGA
void NotifyKey(int key, int release)
{
    KeyboardHandler(key, release);
}
#endif
