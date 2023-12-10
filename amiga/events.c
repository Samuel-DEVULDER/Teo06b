/*
 *      amiga/events.c
 *      
 *      Module simulant les call-backs et autres interrupt vbl du code PC.
 *      (La souris et le clavier en particulier).
 *
 *      Créé par: Samuel Devulder, 08/98
 */

#include "flags.h"
#include "diskio.h"

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <devices/audio.h>

#ifdef __GNUC__
#include <proto/alib.h>
#endif
#include <proto/dos.h>
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

#include "mouse.h"
#include "keyb.h"
#include "keyb_int.h"

/***************************************************************************/

#define CHIPSET ((struct Custom*) 0xDFF000)
#define CIAA    ((struct CIA *)   0xBFE001)

extern struct Library *AslBase;

extern struct Screen *Scr;
extern struct Window *Win;
extern int screen_refresh;
extern void ShowScreen(int on);
extern void setup_sprite(int visible);  /* voir amiga/graph.c */
extern void UpdateScreen(void);         /* idem */
extern void RefreshScreen(void);        /* idem */
extern void UpdatePal(void);            /* ditto */

/* acces disk */
int diskled_timeout;
extern char disk0name[];
extern char disk1name[];
extern int MODE80_ID, MODE40_ID, do_fastgfx;
static void disk_req(char *diskname, int diskno);

/* souris */
#ifndef FAST_MOUSE
int mouse_x, mouse_y;
#endif
static int perif_mouse, mouse_buttons, sprite_visible;

/***************************************************************************/
/* Conversion scancode Amiga => PC */
static UBYTE key_amiga2key_pc[128] = {
    KEY__HOME /* `~ */, 
    
    KEY__1, KEY__2, KEY__3, KEY__4, KEY__5, KEY__6, KEY__7, KEY__8, 
    KEY__9, KEY__0, KEY__CLOSEBRACE, /*KEY__EQUALS*//*KEY__EXCLAM*/KEY__6, KEY__EXCLAM /* \ */,  
    
    0, KEY__0_PAD, 
 
    KEY__A, KEY__Z, KEY__E, KEY__R, KEY__T, KEY__Y, KEY__U, KEY__I, 
    KEY__O, KEY__P, KEY__HAT, KEY__DOLLAR, 
    
    0, KEY__1_PAD, KEY__2_PAD, KEY__3_PAD, 
    
    KEY__Q, KEY__S, KEY__D, KEY__F, KEY__G, KEY__H, KEY__J, KEY__K, 
    KEY__L, KEY__M, KEY__PERCENT, KEY__ASTERISK,
    
    0, KEY__4_PAD, KEY__5_PAD, KEY__6_PAD, 
    
    0 /* <> */, KEY__W, KEY__X, KEY__C, KEY__V, KEY__B, KEY__N, 
    KEY__COMMA, KEY__SEMICOLON, KEY__COLON, /*KEY__EXCLAM*/KEY__EQUALS, 
    
    0, KEY__POINT_PAD, KEY__7_PAD, KEY__8_PAD, KEY__9_PAD,
    
    KEY__SPACE, KEY__BACKSPACE, KEY__TAB, KEY__ENTER_PAD, KEY__ENTER, 
    KEY__ESC, KEY__DEL /* DEL */, 0, 0, 0, KEY__MINUS_PAD, 
    
    0, KEY__UP, KEY__DOWN, KEY__RIGHT, KEY__LEFT, 
    
    KEY__F1, KEY__F2, KEY__F3, KEY__F4, KEY__F5, KEY__F6, KEY__F7, 
    KEY__F8, KEY__F9, KEY__F10,
    
    KEY__NUMLOCK /* [ */, KEY__SCRLOCK /* ] */,  KEY__SLASH_PAD, 
    KEY__ASTERISK_PAD, KEY__PLUS_PAD,
    
    KEY__INSERT /* HELP */, 
    
    KEY__LSHIFT, KEY__RSHIFT, KEY__CAPSLOCK, KEY__LCONTROL, KEY__ALT, 
    KEY__ALTGR, KEY__ALTGR /* LAmiga */, KEY__RCONTROL /* RAmiga */
    };
extern void NotifyKey(int key, int release);

/***************************************************************************/
/*
 * code de gestion de la souris..
 */
void InitPointer(void)
{
    /* on force l'autodetection de la souris au demarrage */
    ROM[3][0x2E6C]=0x21; /* BRanch Never */
}

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
            setup_sprite(0);
            perif_mouse = 1;
            break;
            
          case LIGHTPEN:
            setup_sprite(1);
            sprite_visible = 1;
            perif_mouse = 0;
            break;
            
          case PREVIOUS:
            break;
        }
}

void ShutDownPointer(void)
{
}

/***************************************************************************/
/*
 * Gestion du joystick
 */
static void handle_joystick(void)
{
    static int oldfire=0x80, oldjoy=0x3030;
    int joy, fire;
    
    /* fire */
    fire = ( CIAA->ciapra & 0x0080 ); /* !!! fire == fire_release */
    if(fire != oldfire) {
        oldfire = fire;
        ORB2=((fire ? ORB2|0x40 : ORB2&0xBF)&(DDRB2^0xFF))|(ORB2&DDRB2);
        if (CRB2&4) MON[0][0x7CD]=MON[1][0x7CD]=ORB2;
        CRA2=(fire ? CRA2|0x80 : CRA2&0x7F);
        MON[0][0x7CE]=MON[1][0x7CE]=CRA2;
    }
    
    /* direction */
    joy  = CHIPSET->joy1dat & 0x0303;
    if(joy != oldjoy) {
        int dir = 0;
        oldjoy = joy;
        joy ^= joy>>1;
        if(!(joy & 0x0002)) dir |= 8; /* right */
        if(!(joy & 0x0200)) dir |= 4; /* left */
        if(!(joy & 0x0001)) dir |= 2; /* bot */
        if(!(joy & 0x0100)) dir |= 1; /* top */
        ORA2=(((ORA2&0xF0)|dir)&(DDRA2^0xFF))|(ORA2&DDRA2);
        if (CRA2&4) MON[0][0x7CC]=MON[1][0x7CC]=ORA2;
    }
}

/***************************************************************************/
/*
 * active la led tout le temps que diskled_timeout > 0;
 */
static void handle_disk_led(void)
{
    if(diskled_timeout) {
        if(--diskled_timeout) CIAA->ciapra &=~2;
        else                  CIAA->ciapra |= 2;
    }
}

/***************************************************************************/
#ifdef REG_MAPPER
#undef SetMode
#undef Input
#endif
int getch(void)
{
	int c;
	fflush(stdout);
	SetMode(Input(),1);
	c = getc(stdin);
	SetMode(Input(),0);
	return c;
}

/***************************************************************************/
/*
 * la routine suivante est appellee a chaque VBL thomson. Elle simule les
 * callback et interrupt du code source PC.
 */
#if defined(REG_MAPPER)&&defined(GetMsg)
#undef GetMsg
#undef ReplyMsg
#endif
void handle_events(void)
{
    struct IntuiMessage *msg;
    int mode_change = 0;
    
/*    if(screen_refresh) RefreshScreen();*/
    if(0==(vbl&2)) UpdateScreen();
    UpdatePal();

    handle_joystick();
    handle_disk_led();
    
    while((msg=(void*)GetMsg(Win->UserPort))) {
        int class,code,qual;
        class   = msg->Class;
        code    = msg->Code;
        qual    = msg->Qualifier;
#ifndef FAST_MOUSE
        mouse_x = msg->MouseX;
        mouse_y = msg->MouseY;
        if(GE7DC==0x2A && (Scr->Width < 640)) mouse_x *= 2;
	if(GE7DC==0x2A && (Scr->Height< 480)) mouse_y *= 2;
#endif
        ReplyMsg((struct Message*)msg);
        
        switch(class) {
          case IDCMP_MOUSEBUTTONS:
            if(perif_mouse) {
                if(code==SELECTDOWN) mouse_buttons |= 1; else
                if(code==SELECTUP)   mouse_buttons &=~1; else
                if(code==MENUDOWN)   mouse_buttons |= 2; else
                if(code==MENUUP)     mouse_buttons &=~2;
                ORA2=(((ORA2&0xFC)|(~mouse_buttons))&(DDRB2^0xFF))|(ORA2&DDRB2);
                if (CRA2&4) MON[0][0x7CC]=MON[1][0x7CC]=ORA2;
		TO_keypressed=1;
            } else {
                if(code==SELECTDOWN) PRC |= 2; else
                if(code==SELECTUP)   PRC &=~2; else
                if(code==MENUUP) setup_sprite(sprite_visible ^= 1);
                MON[0][0x7C3]=MON[1][0x7C3]=PRC;
            }
            break;
            
          case IDCMP_MOUSEMOVE:
            if(perif_mouse) {
                port0&=0x7F;  /* signal de mouvement souris */
                MON[0][0x7C0]=MON[1][0x7C0]=port0;
		TO_keypressed=1;
            }
            break;
            
          case IDCMP_RAWKEY: 
            if((code&127)==98) { 
                /* le casplock amiga envoit un seul signal a chaque pression */
                /* donc pour emuler celui du PC on appelle 2 fois la routine de */
                /* callback */
                NotifyKey(KEY__CAPSLOCK,0);
                NotifyKey(KEY__CAPSLOCK,1);
	    } else if((code&127)==48) { 
                /* les touches <> sont mappées différement sur PC */
                NotifyKey(KEY__ALTGR,code&128);
		NotifyKey(KEY__DEL,  code&128);
	    } else if(code==(86+127)) { /* F6 */
		disk_req(disk0name, 0);
	    } else if(code==(87+127)) { /* F7 */
		disk_req(disk1name, 1);
	    } else if(code==(88+127)) { /* F8 */
	        if(kb_state & KB_SHIFT_FLAG) {	            
		    do_fastgfx = do_fastgfx?0:1;
                    RefreshScreen();UpdateScreen();
		} else mode_change = 1;
            } else {
                NotifyKey(key_amiga2key_pc[code&127],code&128);
            }
            break;
            
          default: printf("Unknown class: %08x\n",class);
        }
    }
    if(mode_change) {
	if(GE7DC==0x2A) MODE80_ID = -1;
	else            MODE40_ID = -1;
	SetGraphicMode(INIT);
    }
}

/***************************************************************************/
static void disk_req(char *diskname, int diskno)
{
    struct FileRequester *FileRequest;
    char title[80], path[256], name[256], *s, *t;

    /* requester title */
    sprintf(title, "TEO: Choix du disque %d", diskno);

    /* split dir part */
    for(s=path, t=diskname; (*s=*t); ++s, ++t); 
    for(;s>path; --s) if(*s==':' || *s=='/') {++s;break;}
    strcpy(name, s); *s=0;

    if(!AslBase) return;
    
    FileRequest = AllocAslRequest(ASL_FileRequest, NULL);
    if(!FileRequest) {
        fprintf(stderr,"Unable to allocate file requester.\n");
        return;
    }

    ShowScreen(0);
    if(AslRequestTags(FileRequest,
                      ASLFR_TitleText,      (ULONG)title,
                      ASLFR_InitialDrawer,  (ULONG)path,
                      ASLFR_InitialFile,    (ULONG)name,
                      ASLFR_InitialPattern, (ULONG)"#?.(sap|zip|lha|z|gz|k7)",
                      ASLFR_DoPatterns,     TRUE,
                      ASLFR_RejectIcons,    TRUE,
                      TAG_DONE)) {
	    char *s = diskname;
	    char *t = FileRequest->fr_Drawer;
	    while(*t) *s++ = *t++;
	    if(*diskname && s[-1]!=':' && s[-1]!='/') *s++ = '/';
	    t = FileRequest->fr_File;
	    while(*t) *s++ = *t++;
	    *s = '\0';
	    if(*diskname) LoadSAP(diskname, diskno);
        }
    ShowScreen(1);
    FreeAslRequest(FileRequest);
}
