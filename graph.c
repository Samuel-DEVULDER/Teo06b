/*

	MARCEL O CINQ
	*************

	l'émulateur de thomson MO5

	Par Gilles FETIS

	mail : FETIS_Gilles@compuserve.com
	page : http://ourworld.compuserve.com/homepages/FETIS_Gilles


*/


/*

	VERSION     : 2.0
	MODULE      : GRAPH.C
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	Gestion de l'écran du MO5
*/

/****************** start2 ***/
#ifdef DJGPP
#include <pc.h>
#include <go32.h>
#include <sys/farptr.h>
#endif
#include <allegro.h>
#include "mapper.h"
#include "graph.h"
#include "main.h"


/* pour reduire la taille de l'executable */
DECLARE_COLOR_DEPTH_LIST(
    COLOR_DEPTH_8
)

int graphic_mode;
int mode80_vbl;
/****************** end2 ***/

int screen_refresh;

static int gamma[16]={0,25,32,37,41,44,46,49,51,53,55,57,59,61,62,63};

static PALETTE palette={ { 0, 0, 0}, /* bordure de l'ecran */
                         { 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},
                         { 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},
                         { 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},
                         { 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},{ 0, 0, 0},
                         { 0, 0, 0},{63,63,63},   /* pointeur du c.o. */
                         { 0, 0,63},{ 0,63,63},   /* GUI */
                         { 0, 0,63},{51,62,57} }; /* page modif. palette */

/************************ start ***/
static int border_color;
static BITMAP *buffer;                  /*                          */
static int pixel_height[5]={2,3,2,3,2}; /* pour le mode 80 colonnes */
static int pixel_pos[5]={0,2,5,7,10};   /*                          */


/*
 * SetBorderColor: fixe la couleur du pourtour de l'ecran en copiant le tri-
 *                 plet RGB choisi sur la couleur 0.
 *                 Meme remarque qu'a la fonction precedente pour le test.
 */

void SetBorderColor(int index)
{
    RGB new_rgb,old_rgb;

    get_color(index+1,&new_rgb);
    get_color(0,&old_rgb);

    if ((new_rgb.r != old_rgb.r) || (new_rgb.g != old_rgb.g) || (new_rgb.b != old_rgb.b))
        set_color(0,&new_rgb);

    border_color=index;
}


/*
 * SetColor: convertit la couleur du format TO8 au format RGB et la depose
 *           dans la palette; on est oblige de tout decaler de 1 car, en mode
 *           VGA 13h, 0 est la couleur du bord de l'ecran.
 *           Le test de difference est utile car la palette est rafraichie en
 *           permanence dans l'ecran de modification des couleurs.
 */

void SetColor(int index,int v)
{
    RGB new_rgb,old_rgb;

    new_rgb.r=gamma[v&0xF];
    new_rgb.g=gamma[(v&0xF0)>>4];
    new_rgb.b=gamma[(v&0xF00)>>8];

    get_color(index+1,&old_rgb);

    if ((new_rgb.r != old_rgb.r) || (new_rgb.g != old_rgb.g) || (new_rgb.b != old_rgb.b))
        set_color(index+1,&new_rgb);

    if (index == border_color)
        SetBorderColor(index);  /* on ajuste le pourtour de l'ecran */
}


/*
 * fdraw: accélérateur graphique (update d'un octet de VRAM)
 */

#ifdef DJGPP
void fdraw(unsigned int adr,BYTE val)
{
    register int i;
    unsigned int dos_adr,col,pt,c1,c2;
             int y,VramPage=GE7DD>>6;

    if (val != REFRESH)  /* pour pouvoir rafraichir l'ecran du TO8 */
        GETMAP(adr) = val;

    adr=adr&0x1FFF;
    if (adr>=0x1F40)
        return;
  
    if (graphic_mode != GFX_TEXT)
    {
        pt=RAM[VramPage][0x2000+adr];
        col=RAM[VramPage][adr];

        _farsetsel(_dos_ds);      /* on fixe l'adresse dos de base */
        dos_adr=0xa0000+(adr<<3); /* on calcule l'adresse dos effective */

        switch (GE7DC)
        {
            case 0x21: /* mode bitmap 4 couleurs */
                pt<<=1;

                for (i=0; i<8; i++)
                    _farnspokeb(dos_adr+i, ((pt>>(7-i))&2)+((col>>(7-i))&1)+1);
                break;

            case 0x24: /* mode commutation page 1 */
                for (i=0; i<8; i++)
                    _farnspokeb(dos_adr+i, (0x80>>i)&pt ? 2 : 1);
                break;

            case 0x25: /* mode commutation page 2 */
                for (i=0; i<8; i++)
                    _farnspokeb(dos_adr+i, (0x80>>i)&col ? 3 : 1);
                break;

            case 0x26: /* mode superposition 2 pages */
                for (i=0; i<8; i++)
                    _farnspokeb(dos_adr+i, (0x80>>i)&pt ? 2 : ((0x80>>i)&col ? 3 : 1) );
                break;

            case 0x2A: /* mode 80 colonnes */
                if (graphic_mode == GFX_VESA1)  /* mode exact */
                {
                    for (i=0; i<8; i++)
                    {
                        buffer->line[0][i]=(0x80>>i)&pt ? 2 : 1;
                        buffer->line[0][i+8]=(0x80>>i)&col ? 2 : 1;
                    }

                    y=adr/40;
                    stretch_blit(buffer,screen,0,0,16,1,(adr%40)*16,(y/5)*12+pixel_pos[y%5],16,pixel_height[y%5]);
                }
                else  /* pseudo-mode */
                {
                    for (i=0;i<4;i++)
                    {
                        _farnspokeb(dos_adr+i  ,( ( pt>>(6-(i<<1)) )&1)+1);
                        _farnspokeb(dos_adr+i+4,( (col>>(6-(i<<1)) )&1)+1);
                    }
                }
                break;

            case 0x3F: /* mode superposition 4 pages */
                for (i=0; i<4; i++)
                    /* on modifie les pixels deux par deux */
                    _farnspokew(dos_adr+(i<<1), (0x80>>i)&pt  ? 0x202 :
                                               ((0x08>>i)&pt  ? 0x303 :
                                               ((0x80>>i)&col ? 0x404 :
                                               ((0x08>>i)&col ? 0x505 : 0x101))));
                break;

            case 0x41: /* mode bitmap 4 non documenté */
                for (i=0;i<4;i++)
                {
                     _farnspokeb(dos_adr+i  ,((  pt>>(6-(i<<1)) )&3) +1 );
                     _farnspokeb(dos_adr+i+4,(( col>>(6-(i<<1)) )&3) +1 );
                }
                break;

            case 0x7B: /* mode bitmap 16 couleurs */
                /* on modifie les pixels deux par deux */
                _farnspokew(dos_adr  ,((( pt&0xF0)>>4)+1)*0x101);
                _farnspokew(dos_adr+2, (( pt&0xF)     +1)*0x101);
                _farnspokew(dos_adr+4,(((col&0xF0)>>4)+1)*0x101);
                _farnspokew(dos_adr+6, ((col&0xF)     +1)*0x101);
                break;

            case SPECIAL_MODE: /* ecran de modif. palette: 18 couleurs */
                c1=((col>>3)&7)+(((~col)&0x40)>>3)+1;
                c2=(col&7)+(((~col)&0x80)>>4)+1;

                if (adr<0x1A18)
                    if (c1==15)
                    {
                        c1=21;
                        c2=22;
                    }
                    else
                    {
                        c1=22;
                        c2=21;
                    }

                for (i=0; i<8; i++)
                    _farnspokeb(dos_adr+i, (0x80>>i)&pt ? c1 : c2);
                break;

            case 0:    /* mode 40 colonnes 16 couleurs */
            default:
                c1=((col>>3)&7)+(((~col)&0x40)>>3)+1;
                c2=(col&7)+(((~col)&0x80)>>4)+1;

                for (i=0; i<8; i++)
                    _farnspokeb(dos_adr+i, (0x80>>i)&pt ? c1 : c2);
                break;
        } /* end of switch */
    }

}

static END_OF_FUNCTION(fdraw)

#else
/*
 * non VGA specific version
 * the screen is supposed to be 320x200x256
 * used colors are 1 to 16 (due to a PC limitation for border color)
 */
void fdraw(unsigned int adr,BYTE val)
{
    register int i;
    unsigned int dos_adr,col,pt,c1,c2;
             int y,VramPage=GE7DD>>6;
	     int whx,why;

    if (val != REFRESH)  /* pour pouvoir rafraichir l'ecran du TO8 */
        GETMAP(adr) = val;

    adr=adr&0x1FFF;
    if (adr>=0x1F40)
        return;
  
    if (graphic_mode != GFX_TEXT)
    {
        pt=RAM[VramPage][0x2000+adr];
        col=RAM[VramPage][adr];

     	whx=(adr % 40)<<3;  /* calculates base coordinates */
	why=adr / 40;


        switch (GE7DC)
        {
            case 0x21: /* mode bitmap 4 couleurs */
                pt<<=1;
                for (i=0; i<8; i++)
		    putpixel(screen,whx+i,why,((pt>>(7-i))&2)+((col>>(7-i))&1)+1);
                break;

            case 0x24: /* mode commutation page 1 */
                for (i=0; i<8; i++)
                    putpixel(screen,whx+i,why,(0x80>>i)&pt ? 2 : 1);
                break;

            case 0x25: /* mode commutation page 2 */
                for (i=0; i<8; i++)
                    putpixel(screen,whx+i,why, (0x80>>i)&col ? 3 : 1);
                break;

            case 0x26: /* mode superposition 2 pages */
                for (i=0; i<8; i++)
                    putpixel(screen,whx+i,why, (0x80>>i)&pt ? 2 : ((0x80>>i)&col ? 3 : 1) );
                break;

            case 0x2A: /* mode pseudo 80 colonnes pour l'instant */
   
                {
                    for (i=0;i<4;i++)
                    {
                        putpixel(screen,whx+i,why,( ( pt>>(6-(i<<1)) )&1)+1);
                        putpixel(screen,whx+i+4,why,( (col>>(6-(i<<1)) )&1)+1);
                    }
                }
                break;

            case 0x3F: /* mode superposition 4 pages */
                for (i=0; i<4; i++)
		{
                    /* on modifie les pixels deux par deux */
                    putpixel(screen,whx+i<<1,why, (0x80>>i)&pt  ? 0x2 :
                                           ((0x08>>i)&pt  ? 0x3 :
                                           ((0x80>>i)&col ? 0x4 :
                                           ((0x08>>i)&col ? 0x5 : 0x1))));
                    putpixel(screen,whx+i<<1,why, (0x80>>i)&pt  ? 0x2 :
                                           ((0x08>>i)&pt  ? 0x3 :
                                           ((0x80>>i)&col ? 0x4 :
                                           ((0x08>>i)&col ? 0x5 : 0x1))));
		}
                break;

            case 0x41: /* mode bitmap 4 non documenté */
                for (i=0;i<4;i++)
                {
                     putpixel(screen,whx+i,why,((  pt>>(6-(i<<1)) )&3) +1 );
                     putpixel(screen,whx+i+4,why,(( col>>(6-(i<<1)) )&3) +1 );
                }
                break;

            case 0x7B: /* mode bitmap 16 couleurs */
                /* on modifie les pixels deux par deux */
                putpixel(screen,whx,why,((( pt&0xF0)>>4)+1));
                putpixel(screen,whx+1,why,((( pt&0xF0)>>4)+1));	

                putpixel(screen,whx+2,why,(pt&0xF)+1);
                putpixel(screen,whx+3,why,(pt&0xF)+1);

                putpixel(screen,whx+4,why,((( col&0xF0)>>4)+1));
                putpixel(screen,whx+5,why,((( col&0xF0)>>4)+1));	

                putpixel(screen,whx+6,why,(col&0xF)+1);
                putpixel(screen,whx+7,why,(col&0xF)+1);

                break;

            case SPECIAL_MODE: /* ecran de modif. palette: 18 couleurs */
                c1=((col>>3)&7)+(((~col)&0x40)>>3)+1;
                c2=(col&7)+(((~col)&0x80)>>4)+1;

                if (adr<0x1A18)
                    if (c1==15)
                    {
                        c1=21;
                        c2=22;
                    }
                    else
                    {
                        c1=22;
                        c2=21;
                    }

                for (i=0; i<8; i++)
                    putpixel(screen,whx+i,why, (0x80>>i)&pt ? c1 : c2);
                break;

            case 0:    /* mode 40 colonnes 16 couleurs */
            default:
                c1=((col>>3)&7)+(((~col)&0x40)>>3)+1;
                c2=(col&7)+(((~col)&0x80)>>4)+1;

                for (i=0; i<8; i++)
                    putpixel(screen,whx+i,why, (0x80>>i)&pt ? c1 : c2);
                break;
        } /* end of switch */
    }

}
#endif /* LINUX */

static void SetMode80(void)
{
    register int i;

    set_color(0,&palette[17]);
     /* la bordure de l'ecran n'est plus geree en mode VESA; la couleur 0
         sert lors de l'initialisation du mode d'ou le choix du noir */

    set_gfx_mode(GFX_AUTODETECT,640,480,0,0);
    set_palette_range(palette,1,22,FALSE);
    graphic_mode=GFX_VESA1;

    for (i=0x4000; i<0x5F40; i++)  /* on redessine l'ecran */
        fdraw(i,REFRESH);
}



static void SetMode40(void)
{
    register int i;

    set_gfx_mode(GFX_VGA,320,200,0,0);
    set_palette_range(palette,0,22,FALSE);
    graphic_mode=GFX_VGA;

    for (i=0x4000; i<0x5F40; i++)  /* on redessine l'ecran */
        fdraw(i,REFRESH);
}


/*
 * SetGraphicMode: selectionne le mode graphique de l'emulateur en fonction de
 *                 celui du TO8: VESA 1.2 640*480 pour le mode 80 colonnes,
 *                 VGA 13h 320*200 pour tous les autres modes.
 */

void SetGraphicMode(unsigned int mode)
{
    switch (mode)
    {
        case MODE80:
            get_palette_range(palette,0,16);  /* on sauve la palette */
            SetMode80();
            mode80_vbl=0;
            break;

        case INIT:
            if (GE7DC == 0x2A)
                SetMode80();
            else
                SetMode40();
            break;

        default:
            if (mode == GE7DC)
                break;

            GE7DC=mode;
            MON[0][0x7DC]=MON[1][0x7DC]=GE7DC;

            if (GE7DC == 0x2A)  /* mode 80 colonnes */
                mode80_vbl=vbl+MODE80_DELAY*VIDEO_FRAME_FREQ;
            else
            {
                mode80_vbl=0;

                if (graphic_mode == GFX_VESA1)
                {
                    get_palette_range(palette,1,16); /* on sauve la palette */
                    SetMode40();
                    break;
                }
            }

            screen_refresh=TRUE;
    } /* end of switch */
}


void ReturnText(void)
{
    get_palette_range(palette,0,16); /* on sauvegarde la palette */
    set_gfx_mode(GFX_TEXT,0,0,0,0);
    graphic_mode=GFX_TEXT;
}



void SetupGraph(void)
{

    buffer=create_bitmap(16,1);

#ifdef DJGPP
    LOCK_FUNCTION(fdraw);
#endif

/* L'ecran de selection des couleurs realise le tour de force d'afficher
   simultanement 18 couleurs differentes, alors que ce nombre est limite a 16
   par la quantite de memoire video (16k) du TO8. L'astuce consiste a changer
   la palette de couleurs lorsque le faisceau est au milieu de l'ecran; cela
   necessite evidemment un timing parfait qu'il n'est pas envisageable de
   reproduire par emulation.
   On supprime donc le flipping des palettes */

    ROM[3][0x38EB]=0x7E;
    ROM[3][0x38EC]=0x39;
    ROM[3][0x38ED]=0x10;
    ROM[3][0x395F]=0x12;
    ROM[3][0x3960]=0x12;
    ROM[3][0x396F]=0x12;
    ROM[3][0x3970]=0x12;
}

