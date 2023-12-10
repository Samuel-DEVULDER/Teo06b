/*
 * mouse.c: gestion de la souris et du crayon optique du TO8
 */

#include "flags.h" /* sam: ou sinon: pbs avec REGPARM */

#include <allegro.h>
#include "mapper.h"
#include "graph.h"
#include "keyb.h"
#include "mouse.h"

static int perif_installed;
static BITMAP *pen_pointer;
static char pen_pointer_data[64]={ 0,0,1,1,1,0,0,0,
                                   0,0,1,2,1,0,0,0,
                                   1,1,1,2,1,1,1,0,
                                   1,2,2,2,2,2,1,0,
                                   1,1,1,2,1,1,1,0,
                                   0,0,1,2,1,0,0,0,
                                   0,0,1,1,1,0,0,0,
                                   0,0,0,0,0,0,0,0 };

/*
 * MouseCallBack: routine de gestion de la souris du TO8 appelee par
 *                l'interruption materielle souris INT 0x33 via Allegro
 */

static void MouseCallback(int flags)
{
    if (flags&(MOUSE_FLAG_LEFT_UP|MOUSE_FLAG_LEFT_DOWN|MOUSE_FLAG_RIGHT_UP
                                                     |MOUSE_FLAG_RIGHT_DOWN))
    {
        ORA2=(((ORA2&0xFC)|(~mouse_b&3))&(DDRB2^0xFF))|(ORA2&DDRB2);

        if (CRA2&4)
            MON[0][0x7CC]=MON[1][0x7CC]=ORA2;
    }

    if (flags&MOUSE_FLAG_MOVE)
    {
        port0&=0x7F;  /* signal de mouvement souris */
        MON[0][0x7C0]=MON[1][0x7C0]=port0;
    }

    TO_keypressed=1;
}

END_OF_FUNCTION(MouseCallback)


/*
 * LightpenCallback: routine de gestion du crayon optique du TO8 appelee par
 *                   l'interruption materielle souris INT 0x33 via Allegro
 */

static void LightpenCallback(int flags)
{
    register int i;
    static   int crayx,crayy,pointer_on;
             int cx,cy;

    if (flags&(MOUSE_FLAG_LEFT_UP|MOUSE_FLAG_LEFT_DOWN))
    {
        PRC=(PRC&0xFD)|((mouse_b&1)<<1);
        MON[0][0x7C3]=MON[1][0x7C3]=PRC;
    }

    if (flags&(MOUSE_FLAG_MOVE|MOUSE_FLAG_RIGHT_DOWN))
    {
        if (graphic_mode == GFX_VESA1)  /* mode 640*480 */
        {
            cx=MIN(crayx+4,SCREEN_W-1)>>4;
            crayx=MAX(crayx-3,0)>>4;
            cy=((MIN(crayy+4,SCREEN_H-1)*5)/12)*40+0x4000;
            crayy=((MAX(crayy-3,0)*5)/12)*40+0x4000;

            while (crayy<=cy)
            {
                for (i=crayx; i<=cx; i++)
                    fdraw(crayy+i,REFRESH);
                crayy+=40;
            }  /* on restaure la portion d'ecran sous le pointeur */
        }
        else  /* mode 320*200 */
        {
            cx=MIN(crayx+4,SCREEN_W-1)>>3;
            crayx=MAX(crayx-3,0)>>3;
            cy=MIN(crayy+4,SCREEN_H-1)*40+0x4000;
            crayy=MAX(crayy-3,0)*40+0x4000;

            while (crayy<=cy)
            {
                for (i=crayx; i<=cx; i++)
                    fdraw(crayy+i,REFRESH);
                crayy+=40;
            } /* on restaure la portion d'ecran sous le pointeur */
        }

        crayx=mouse_x;
        crayy=mouse_y;

        if (flags&MOUSE_FLAG_RIGHT_DOWN)
            pointer_on^=1;

        if (pointer_on)
            draw_sprite(screen, pen_pointer, crayx-3, crayy-3);
    }
}

static END_OF_FUNCTION(LightpenCallback)


/*
 * InitPointer: initialise le module souris/crayon optique
 */

void InitPointer(void)
{
    register int i;

  /* creation du pointeur crayon optique */
    pen_pointer=create_bitmap(8,8);
    for (i=0; i<64; i++)
        switch (pen_pointer_data[i])
        {
            case 1:
                pen_pointer->line[i/8][i&7] = 17;
                break;
            case 2:
                pen_pointer->line[i/8][i&7] = 18;
                break;
            default:
                pen_pointer->line[i/8][i&7] = 0;
        }

  /* on force l'autodetection de la souris au demarrage */
    ROM[3][0x2E6C]=0x21; /* BRanch Never */

    lock_bitmap(pen_pointer);
    LOCK_FUNCTION(MouseCallback);
    LOCK_FUNCTION(LightpenCallback);
    install_mouse();
}


/*
 * InstallPointer: installe le peripherique de pointage choisi
 */

void InstallPointer(int perif)
{
    PRC&=0xFD;  /* bouton crayon optique relache */
    MON[0][0x7C3]=MON[1][0x7C3]=PRC;

    port0|=0x80;  /* souris au repos */
    MON[0][0x7C0]=MON[1][0x7C0]=port0;

    ResetPortManette0();

    switch (perif)
    {
        case MOUSE:
            kb_state|=KB_NUMLOCK_FLAG;
            set_leds(kb_state);

            mouse_callback=MouseCallback;
            perif_installed=MOUSE;
            break;

        case LIGHTPEN:
            mouse_callback=LightpenCallback;
            perif_installed=LIGHTPEN;
            break;

        case PREVIOUS:
            mouse_callback=(perif_installed == MOUSE ? MouseCallback
                                                     : LightpenCallback);
            break;
    }
}


/*
 * ShutDownPointer: desactive le peripherique de pointage du TO8
 */

void ShutDownPointer(void)
{
    mouse_callback=NULL;
}


