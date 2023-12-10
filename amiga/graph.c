/*
 *      amiga/graph.c
 *      
 *      Module graphique pour l'emulateur thomson.
 *
 *      Samuel Devulder, 08/98
 */

#include "flags.h"

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <devices/audio.h>
#include <intuition/intuitionbase.h>

#ifdef __GNUC__
#include <proto/alib.h>
#endif
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/keymap.h>
#include <proto/asl.h>

#include <hardware/custom.h>
#include <hardware/cia.h>

#include "mapper.h"
#include "graph.h"
#include "main.h"

#include "keyb.h"
#include "keyb_int.h"

/*
note: Le code ici n'est pas genial car il poke directement dans la
memoire video. Cela ne marchera donc pas pour les amigas equipes d'une
carte graphique. Dans ce cas, il faudrait pour les modes non chunky
(40 cols to7, 80cols) s'allouer un bitmap 8x1, poker dedans, puis 
demander l'affichage avec BltBitmapXXX() et pour les modes chunky
(bitmap 4, bitmap 16), utiliser un WritePixelArray() a la place.
Cependant, je crains que cela ralentisse bcp l'affichage. 

Bref: a voir pour plus tard (si on m'offre un 060 + carte gfx :))).
*/

/***************************************************************************/

/* 
 * Parametres de compilation propre a graph.c
 */

/* 
  Definir ceci pour eviter un test dans fdraw, mais on a un peu de 'garbage' 
  en bas de l'ecran
*/
#define NO5F40

/*
  Definir ceci pour emuler le tour, mais le code sera un peu plus lent
*/
#define EMUL_TOUR

/***************************************************************************/

static void Open_Scr_Win(int width, int height, int depth, ULONG modeid);
static struct BitMap *myAllocBitMap(ULONG sizex, ULONG sizey, ULONG depth, 
				    ULONG flags, struct BitMap *friend_bitmap);
static void myFreeBitMap(struct BitMap *bm);

extern void LoadPrefs(void);
extern void SavePrefs(void);

void RefreshScreen(void);
void UpdateScreen(void);
void ShowScreen(int);
void UpdatePal(void);

/***************************************************************************/

#define Scr_width(x)  (x)
#ifdef NO5F40
#define Scr_height(x) ((x)+8)
#else 
#define Scr_height(x) (x)
#endif
#ifdef EMUL_TOUR
#define Scr_depth(x) ((x)+1)
#else
#define Scr_depth(x) (x)
#endif

/***************************************************************************/

#define CHIPSET ((struct Custom*) 0xDFF000)
#define CIAA    ((struct CIA *)   0xBFE001)

#ifdef __SASC
extern struct ExecBase *SysBase;
#endif
struct IntuitionBase *IntuitionBase;
struct GfxBase       *GfxBase;
struct Library       *AslBase;

struct Screen *Scr;
struct Window *Win;

/* bitmap gfx */
int use_os = 1;
int do_fastgfx = 1;
UBYTE *video_ram;
static struct BitMap *bitmap;
static UBYTE *sbitmap;
static int sdelta1, sdelta2, sdelta3, sdelta4;
static int os39, bitmap_alloc;

/* tables de decodages pour accelerer l'affichage */
static UBYTE dec16_1[256];      /* mode bitmap 16 */
static UBYTE dec16_2[256];

static UBYTE dec04_1[256];      /* mode bitmap 4 non documente */
static UBYTE dec04_2[256];

static UBYTE dec7co[256][4];    /* mode 40 col to7 */
static UBYTE dec7op[256][4];

static void init_decoders(void);
static void init_modes(void);

/* conversion gamma */
static UBYTE gamma[16]={0,5,7,8,9,10,10,11,12,12,13,14,14,14,15,15};
static UWORD gamma_rvb[4096];
static UWORD gaoff_rvb[4096];
int do_gamma = 1;

/* palette to8 */
static UWORD pal[16];
static UWORD cmap[32];
static UWORD cmap_changed;
static int   cmap_len;
static int last_tour_idx;

/* flag mode */
static UBYTE mode80;
ULONG MODE40_ID = -1;
ULONG MODE80_ID = -1;

/* souris */
extern int pointer_visible;
static UWORD *Sprite;

/***************************************************************************/

int screen_refresh; /* flag de ??? */
int mode80_vbl;

/***************************************************************************/
/*
 * Update d'un octet en ram video
 */
#ifndef __GNUC__
#define __asm(x)
#endif

typedef void REGPARM (*draw_func)(ULONG adr);
static draw_func drawtab[256];

static void REGPARM commut_page_1(ULONG adr)
{
    UBYTE *p  = &video_ram[adr];
    UBYTE *a  = sbitmap + adr;
    *(UBYTE*)a = p[0x2000]; a += sdelta1;
    *(UBYTE*)a = 0;
    a += sdelta2; *a = 0;
    a += sdelta3; *a = 0;
}

static void REGPARM commut_page_2(ULONG adr)
{
    UBYTE *p = &video_ram[adr];
    UBYTE *a = sbitmap + adr;
    *a = 0; a += sdelta1;
    *a = p[0x0000];
    a += sdelta2; *a = 0;
    a += sdelta3; *a = 0;
}

static void REGPARM supp_2_pages(ULONG adr)
{
    UBYTE *p = &video_ram[adr];
    UBYTE *a = sbitmap + adr;
    *a = p[0x2000]; a += sdelta1;
    *a = p[0x0000] & ~p[0x2000];
    a += sdelta2; *a = 0;
    a += sdelta3; *a = 0;
}

static void REGPARM bitmap4(ULONG adr)
{
    UBYTE *p = &video_ram[adr];
    UBYTE *a = sbitmap + adr;
    *a = p[0x0000]; a += sdelta1;
    *a = p[0x2000]; 
    a += sdelta2; *a = 0;
    a += sdelta3; *a = 0;
}

static void REGPARM mode80simu(ULONG adr)
{
    UBYTE *a __asm("a0") = &video_ram[adr];
    UBYTE p = a[0x0000], q = a[0x2000];
    p  = dec04_2[p];
    p |= dec04_1[q];
    a = sbitmap + adr;
    *a = p; a += sdelta1; 
    *a = 0; a += sdelta2; 
    *a = 0; a += sdelta3; 
    *a = 0;
}

static void REGPARM mode80cols(ULONG adr)
{
    UBYTE *v __asm("a1") = &video_ram[adr];
    UBYTE *a __asm("a0") = sbitmap;
    a+=adr; a+=adr;
    *a++ = v[0x2000];
    *a   = v[0x0000];
}
 
static void REGPARM bitmap4nd(ULONG adr)
{
#if 0
    UBYTE col = video_ram[adr];
    UBYTE pt  = video_ram[0x2000+adr];
    adr += (ULONG) sbitmap;
    *(UBYTE*)adr = dec04_1[pt & 0x55]|dec04_2[col & 0x55]; adr += sdelta1;
    *(UBYTE*)adr = dec04_1[pt & 0xAA]|dec04_2[col & 0xAA]; 
#else
    UBYTE pt,col;
    col = video_ram[adr];
    pt  = video_ram[adr+0x2000];
{
    register UBYTE *dec1 __asm("a1")=dec04_1, *dec2 __asm("a6")=dec04_2;
    register UBYTE *a __asm("a0") = (void*)(adr + (ULONG) sbitmap);
    *a = dec1[pt & 0x55]|dec2[col & 0x55]; a += sdelta1;
    *a = dec1[pt & 0xAA]|dec2[col & 0xAA]; a += sdelta2;
    *a = 0; a += sdelta3;
    *a = 0;
}
#endif
} 

static void REGPARM specmode(ULONG adr)
{
    int col = (adr<0x1A18)?0xE6:video_ram[adr];
    int pt  = video_ram[0x2000+adr];
    UBYTE *de = dec7co[col];
    UBYTE *op = dec7op[pt];

    adr += (ULONG) sbitmap;
    *(UBYTE*)adr = op[*de++]; adr += sdelta1;
    *(UBYTE*)adr = op[*de++]; adr += sdelta2;
    *(UBYTE*)adr = op[*de++]; adr += sdelta3;
    *(UBYTE*)adr = op[*de];
}

static void REGPARM bitmap16(ULONG adr)
{
    UBYTE pt,col;
    col = video_ram[adr];
    pt  = video_ram[adr+0x2000];
{
    register UBYTE *dec1 __asm("a1")=dec16_1, *dec2 __asm("a6")=dec16_2;
    register UBYTE *a __asm("a0") = (void*)(adr + (ULONG) sbitmap);
    *a = dec1[pt & 0x11]|dec2[col & 0x11]; a += sdelta1;
    *a = dec1[pt & 0x22]|dec2[col & 0x22]; a += sdelta2;
    *a = dec1[pt & 0x44]|dec2[col & 0x44]; a += sdelta3;
    *a = dec1[pt & 0x88]|dec2[col & 0x88];
}
}

static void REGPARM to770mode(ULONG adr)
{
   register    UBYTE *op __asm("a1"), *de __asm("a6");
    de = video_ram;de+=adr;
    op = dec7op[de[0x2000]];
    de = dec7co[de[0x0000]];
{
    register UBYTE *a __asm("a0") = (void*)(adr+(ULONG)sbitmap);
    *a = op[*de++]; a += sdelta1;
    *a = op[*de++]; a += sdelta2;
    *a = op[*de++]; a += sdelta3;
    *a = op[*de];
}
}

static void REGPARM supp_4_pages(ULONG adr)
{
    UBYTE col = video_ram[adr];
    UBYTE pt  = video_ram[0x2000+adr];
    UBYTE pt2, cl2;

    pt2  = (pt&0x80)?1*16:(pt&0x08)?2*16:(col&0x80)?3*16:(col&0x08)?4*16:0;
    pt2 |= (pt&0x40)?1:(pt&0x04)?2:(col&0x40)?3:(col&0x04)?4:0;
    cl2  = (pt&0x20)?1*16:(pt&0x02)?2*16:(col&0x20)?3*16:(col&0x02)?4*16:0;
    cl2 |= (pt&0x10)?1:(pt&0x01)?2:(col&0x10)?3:(col&0x01)?4:0;
    pt = pt2; col = cl2;

    adr += (ULONG) sbitmap;
    *(UBYTE*)adr = dec16_1[pt & 0x11]|dec16_2[col & 0x11]; adr += sdelta1;
    *(UBYTE*)adr = dec16_1[pt & 0x22]|dec16_2[col & 0x22]; adr += sdelta2;
    *(UBYTE*)adr = dec16_1[pt & 0x44]|dec16_2[col & 0x44]; adr += sdelta3;
    *(UBYTE*)adr = dec16_1[pt & 0x88]|dec16_2[col & 0x88];
}

static draw_func drawfunc = to770mode;

void REGPARM refresh_gpl(unsigned int no)
{
#ifndef NO5F40
    if (no>=0x1F40) return;
#endif
    (*drawfunc)(no);
}

void __inline REGPARM fdraw2(unsigned int adr, BYTE val)
{
    GETMAP(adr) = val;
    adr &= 0x1FFF;
    
#ifndef NO5F40
    if (adr>=0x1F40) return;
#endif

    (*drawfunc)(adr);
}

void __inline REGPARM fdraw(unsigned int adr, BYTE val)
{
    if (val != REFRESH)  /* pour pouvoir rafraichir l'ecran du TO8 */
	    GETMAP(adr) = val;
    
    adr &= 0x1FFF;
    
#ifndef NO5F40
    if (adr>=0x1F40) return;
#endif
    (*drawfunc)(adr);
}

/***************************************************************************/
/*
 * Cloture de la fenetre et de l'ecran
 */
static void Close_Win_Scr(void)
{
    Disable(); /* comme ca handle_events videra effectivement le buffer */
    if(Win)    {handle_events();ResetPortManette0();/*MON[0][0x7CC]=MON[1][0x7CC]=0;*/
		CloseWindow((void*)Win); Win = NULL;}
    if(Scr)    {CloseScreen((void*)Scr); Scr = NULL;}
    Enable();
    if(bitmap_alloc)  {WaitBlit();myFreeBitMap(bitmap);bitmap_alloc=0;}
}

/***************************************************************************/
/*
 * Effectue le menage avant de sortir
 */
static void gfxleave(void)
{
    CIAA->ciapra &= ~2;
    Close_Win_Scr();
    if(Sprite) {FreeMem((void*)Sprite,(4+2*3)*sizeof(*Sprite)); Sprite = NULL;}
    if(IntuitionBase) {CloseLibrary((void*)IntuitionBase); IntuitionBase = NULL;}
    if(GfxBase) {CloseLibrary((void*)GfxBase); GfxBase = NULL;}
    if(AslBase) {CloseLibrary((void*)AslBase); AslBase = NULL;}
    SavePrefs();
    printf("                                                            \n");
}

/***************************************************************************/
/* 
 * Calcule le bitmap representant le sprite
 */
static void sprite_bitmap(UWORD *sprite, char *desc)
{
    int bit = 0x8000;
    sprite += 2;
    sprite[0] = sprite[1] = 0;
    while(*desc) {
	if(*desc=='\n') {
	    sprite+=2;
	    sprite[0] = sprite[1] = 0; 
	    bit = 0x8000;
	} else {
	    int col = *desc-'0';
	    if(col & 1) sprite[0] |= bit;
	    if(col & 2) sprite[1] |= bit;
	    bit /=2 ;
	}
	++desc;
    }
}

/***************************************************************************/
/*
 * activation sprite visible/invisible
 */
void setup_sprite(int visible)
{
    if(!Sprite) return;
    
    if(visible) {
	sprite_bitmap(Sprite,"323\n"
                             "212\n"
 		             "323");
    } else {
	sprite_bitmap(Sprite,"000\n"
 		             "000\n"
 		             "000");
    }
}

/***************************************************************************/
/* recupere un mode graphique
 */
static ULONG SelectModeID(char *title, int width, int height, int depth)
{
    struct ScreenModeRequester *ScreenRequest;
    ULONG ID = 0;

    if(!AslBase) return 0;

    ScreenRequest = AllocAslRequest(ASL_ScreenModeRequest, NULL);
    if(!ScreenRequest) {
        fprintf(stderr, "Unable to allocate screen mode requester.\n");
        return 0;
    }
    
    if(AslRequestTags(ScreenRequest,
                      ASLSM_TitleText,     (ULONG)title,
                      ASLSM_MinWidth,             width,
                      ASLSM_MinHeight,            height,
                      ASLSM_MinDepth,             depth,
                      ASLSM_InitialDisplayWidth,  width,
                      ASLSM_InitialDisplayHeight, height,
                      ASLSM_InitialDisplayDepth,  depth,
                      ASLSM_InitialDisplayID,     (width>=640)?HIRES_KEY:LORES_KEY,
                      TAG_DONE)) { ID = ScreenRequest->sm_DisplayID; }

    FreeAslRequest(ScreenRequest);

    return ID;
}

/***************************************************************************/
/* choix des modes graphiques
 */
void GetModeIDs(void)
{
    if(MODE80_ID == -1) 
    MODE80_ID = SelectModeID("TEO: Select gfxmode for 80-columns:",
    		             Scr_width(640), Scr_height(200), Scr_depth(1));
    if(MODE80_ID == 0) MODE80_ID = DEFAULT_MONITOR_ID | HIRES_KEY;

    if(MODE40_ID == -1) 
    MODE40_ID = SelectModeID("TEO: Select gfxmode for 40-columns:",
    		             Scr_width(320), Scr_height(200), Scr_depth(4));
    if(MODE40_ID == 0) MODE40_ID = DEFAULT_MONITOR_ID | LORES_KEY;
}

/***************************************************************************/
/*
 * Passe en mode 80 colonnes, 2 couls
 */
static void SetMode80(void)
{
    Close_Win_Scr(); if(MODE80_ID==-1) GetModeIDs(); mode80 = 1;
    Open_Scr_Win(Scr_width(640), Scr_height(200), Scr_depth(1), MODE80_ID);
    
    sbitmap = bitmap->Planes[0];
    sdelta1 = 0; sdelta2 = 0; sdelta3 = 0; sdelta4 = 0;

    RefreshScreen();
    UpdateScreen();
}

/***************************************************************************/
/*
 * Passe en mode 40 col, 16 coul
 */
static void SetMode40(void)
{
    Close_Win_Scr(); if(MODE40_ID==-1) GetModeIDs(); mode80 = 0;
    Open_Scr_Win(Scr_width(320), Scr_height(200), Scr_depth(4), MODE40_ID);

    sbitmap = bitmap->Planes[0];
    sdelta1 = bitmap->Planes[1] - bitmap->Planes[0];
    sdelta2 = bitmap->Planes[2] - bitmap->Planes[1];
    sdelta3 = bitmap->Planes[3] - bitmap->Planes[2];
    sdelta4 = bitmap->Planes[0] - bitmap->Planes[3];

    RefreshScreen();
    UpdateScreen();
}

/***************************************************************************/

static void Open_Scr_Win(int width, int height, int depth, ULONG modeid)
{
    Scr = OpenScreenTags(NULL,
    			   SA_Left,0,       SA_Top,0,
                           SA_Width,        width,
                           SA_Height,       height,
                           SA_Depth,        depth,
    			   SA_Title,        (ULONG)"TEO",
                           SA_DisplayID,    modeid,
                           SA_ShowTitle,    FALSE,
                           SA_Quiet,        TRUE,
                           SA_Behind,	    TRUE,
                           SA_Type,         CUSTOMSCREEN,
                           SA_AutoScroll,   TRUE,
                           /* v39 stuff here: */
                           (os39?TAG_IGNORE:TAG_DONE), 0,
                           SA_Interleaved,  TRUE,
                           SA_Exclusive,    TRUE,
                           TAG_DONE);
    if(!Scr) {fprintf(stderr,"Can't open %dx%d screen!\n",width,height); crash();}
    Win = OpenWindowTags(NULL,
                         WA_Width,        Scr->Width,
                         WA_Height,       Scr->Height,
                         WA_CustomScreen, (ULONG)Scr,
                         WA_Backdrop,     TRUE,
                         WA_Borderless,   TRUE,
                         WA_RMBTrap,      TRUE,
                         WA_ReportMouse,  TRUE,
                         WA_IDCMP,        IDCMP_MOUSEBUTTONS|
                                          IDCMP_RAWKEY|
                                          IDCMP_MOUSEMOVE,
                         TAG_DONE);
    if(!Win) {fprintf(stderr,"Can't open %dx%d window!\n",width,height); crash();}

    if(Sprite) SetPointer(Win, Sprite, 3, 3, -2, -1);

    if(use_os) {
       bitmap = myAllocBitMap(width, height, mode80?1:4,BMF_CLEAR, Scr->RastPort.BitMap);
       if(!bitmap)  {fprintf(stderr,"Can't alloc bitmap!\n"); crash();}
       bitmap_alloc = 1;
    } else {
       bitmap = Scr->RastPort.BitMap;
       bitmap_alloc = 0;
    }
    cmap_len = 1<<depth;
#ifdef EMUL_TOUR
    SetAPen(Win->RPort,mode80?2:16);
    RectFill(Win->RPort,0,0,mode80?639:319,199);
    SetBorderColor(last_tour_idx);
#endif
    {int i;for(i=mode80?2:16;i--;) SetColor(i,pal[i]);}
    ShowScreen(1);
}

/***************************************************************************/
/*
 * Mise a jour de l'ecran
 */
void UpdateScreen(void)
{
#ifdef BltBitMap
#undef BltBitMap
#endif
    if(use_os) {
	BltBitMap(bitmap, 0, 0,
		  Win->RPort->BitMap, 0, 0, mode80?640:320, 200,
		  0xC0, 0xFF, NULL);
    }
    UpdatePal();
}

/***************************************************************************/
/*
 * Reaffiche completement l'ecran
 */
void RefreshScreen(void)
{
    draw_func df = drawfunc;
#ifndef REG_M
    int M;
#endif

    video_ram = RAM[GE7DD>>6];
    for (M=0x1F40; M--;) (*df)(M);
    M = screen_refresh = 0;
}

/***************************************************************************/
/* 
 * passage de l'ecran au 1er/2nd plan
 */
void ShowScreen(int on)
{
    if(!Scr) return;
    if(on) {cmap_changed=1;UpdatePal();ScreenToFront(Scr);ActivateWindow(Win);}
    else    ScreenToBack(Scr);
}

/***************************************************************************/
/*
 * Choix du mode graphique.
 */
void SetGraphicMode(unsigned int mode)
{
    video_ram = RAM[GE7DD>>6];
    switch (mode) {
    case MODE80: /* 80cols static ! */
	drawfunc = mode80cols;
	SetMode80();
	mode80_vbl=0;
    break;
	    
    case INIT: /* forcage de mode video */
	if (GE7DC == 0x2A) {
	    drawfunc = mode80cols;
	    SetMode80();
	} else {
	    drawfunc = (GE7DC==SPECIAL_MODE)?specmode:drawtab[GE7DC&255];
	    SetMode40();
	}
	RefreshScreen();
	UpdateScreen();
    break;
	    
    default: /* mode video dynamique */
	if (mode == GE7DC) break; /* no op */
	    
	MON[0][0x7DC] = MON[1][0x7DC] = GE7DC = mode;

	if (GE7DC == 0x2A) { /* passage en mode approx */
	    if(!mode80) {
		mode80_vbl = vbl + MODE80_DELAY * VIDEO_FRAME_FREQ;
		drawfunc = mode80simu;
	    } else {
		mode80_vbl = 0;
		drawfunc = mode80cols;
	    }
	} else { /* passage en 40cols */
	    drawfunc = (mode==SPECIAL_MODE)?specmode:drawtab[mode&255];
	    mode80_vbl = 0;
	    if(mode80) SetMode40();
	}
	if(!do_fastgfx) screen_refresh = TRUE;
    } /* end of switch */
}

void ReturnText(void)
{
	if(Scr) ScreenToBack(Scr);
}

void SetupGraph(void)
{
    atexit(gfxleave);
    
    LoadPrefs();
    
    CIAA->ciapra |= 2;
    
    os39 = (((struct ExecBase *)SysBase)->LibNode.lib_Version>=39);
    
    IntuitionBase = (void*)OpenLibrary("intuition.library",0);
    if(!IntuitionBase) {
	fprintf(stderr,"No intuition ?\n");
	crash();
    }
    
    GfxBase = (void*)OpenLibrary("graphics.library",0);
    if(!GfxBase) {
	fprintf(stderr,"No gfx ?\n");
	crash();
    }

    AslBase = (void*)OpenLibrary("asl.library",36);
    
    GetModeIDs();

    Sprite = AllocMem((4+2*3)*sizeof(*Sprite), MEMF_CHIP|MEMF_CLEAR);
    if(!Sprite) {
	fprintf(stderr, "Warning: Can't alloc sprite buffer !\n");
	return;
    }
    
    init_decoders();
    init_modes();

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

/***************************************************************************/
/*
 * mise en place des tables de precalcul pour les dec7cours
 */
static void init_decoders(void)
{
    int i,j,k;

    /* gamma correction */
    for(i=0; i<16;++i) for(j=0; j<16; ++j) for(k=0; k<16;++k) {
	gamma_rvb[(i<<8)|(j<<4)|k] = (gamma[k]<<8) | (gamma[j]<<4) | gamma[i];
	gaoff_rvb[(i<<8)|(j<<4)|k] = (k<<8)|(j<<4)|i;
    }
    
    /* bitmap 16 */
    for(i=0; i<16; ++i) 
	for(j=0; j<16; ++j)  dec16_2[(i<<4)|j]=((i?12:0)|(j?3:0));
    for(i=0; i<256; ++i) dec16_1[i] = dec16_2[i]<<4;
    
    /* bitmap 4 non doc */
    for(i=0; i<256; ++i) dec04_2[i] = ((i&0xC0)?8:0)|((i&0x30)?4:0)|((i&0x0C)?2:0)|((i&0x03)?1:0);
    for(i=0; i<256; ++i) dec04_1[i] = dec04_2[i]<<4;
    
    /* mode 40 col to770 */
    for(j=0;j<16;++j) for(i=0;i<16;++i) for(k=0;k<4;++k) {
	dec7co[(j*8+(i&7)+(i&8)*16)^0xC0][k] =
	    (((j>>k)&1)<<1) | ((i>>k)&1);
    }
    for(i=0;i<256;++i) {
	dec7op[i][0] = 0x00;
	dec7op[i][1] = ~i;
	dec7op[i][2] = i;
	dec7op[i][3] = 0xFF;
    }
}

static void init_modes(void)
{
    int i;
    for(i=0;i<256;++i) drawtab[i] = to770mode;
    drawtab[0x21] = bitmap4;
    drawtab[0x24] = commut_page_1;
    drawtab[0x25] = commut_page_2;
    drawtab[0x26] = supp_2_pages;
    drawtab[0x2A] = mode80simu;
    drawtab[0x3F] = supp_4_pages;
    drawtab[0x41] = bitmap4nd;
    drawtab[0x7B] = bitmap16;
    video_ram = RAM[GE7DD>>6];
}

/***************************************************************************/
void UpdatePal(void)
{
   if(cmap_changed) {
      cmap_changed = 0;
      LoadRGB4(&Scr->ViewPort, cmap, cmap_len);
   }
}

static void SetPal(int i, int rvb)
{
    if(do_gamma) rvb = gamma_rvb[rvb];
    else         rvb = gaoff_rvb[rvb];
    if(cmap[i]!=rvb) {
        cmap[i] = rvb;
        cmap_changed = 1;
    }
    if(!use_os && !do_fastgfx && IntuitionBase->FirstScreen == Scr) CHIPSET->color[i] = rvb;
}

/*
 * Mise en place de la palette.
 */
void SetColor(int index, int v)
{
    v &= 4095;
    pal[index] = v;
#ifdef EMUL_TOUR
    if(index == last_tour_idx) SetBorderColor(index);
    index += mode80?2:16;
#endif
    SetPal(index, v);
}

/*
 * SetBorderColor: fixe la couleur du pourtour de l'ecran
 */
void SetBorderColor(int index)
{
#ifdef EMUL_TOUR
    int v = pal[index];
    int j;
    last_tour_idx = index;
    for(j=mode80?2:16;j--;) SetPal(j, v);
#endif
}

/***************************************************************************/

static struct BitMap *myAllocBitMap(ULONG sizex, ULONG sizey, ULONG depth, 
				    ULONG flags, struct BitMap *friend_bitmap)
{
    struct BitMap *bm;
    unsigned long extra;
    
    if(os39) return AllocBitMap(sizex, sizey, depth, flags, friend_bitmap);
    
    extra = (depth > 8) ? depth - 8 : 0;
    bm = AllocVec( sizeof *bm + (extra * 4), MEMF_CLEAR );
    
    if( bm )
      {
	  ULONG i;
	  InitBitMap(bm, depth, sizex, sizey);
	  for( i=0; i<depth; i++ )
	    {
		if( !(bm->Planes[i] = AllocRaster(sizex, sizey)) )
		  {
		      while(i--) FreeRaster(bm->Planes[i], sizex, sizey);
		      FreeVec(bm);
		      bm = 0;
		      break;
		  }
	    }
      }
    return bm;
}

/****************************************************************************/

static void myFreeBitMap(struct BitMap *bm)
{
    if(os39) {FreeBitMap(bm);return;}
    while(bm->Depth--)
      FreeRaster(bm->Planes[bm->Depth], bm->BytesPerRow*8, bm->Rows);
    FreeVec(bm);
}

/***************************************************************************/

/* afin que SAS/C appelle bien atexit() en cas de ^C */
#ifdef __SASC /* damn compiler! */
void __regargs _CXBRK(void) {crash();}
#endif
