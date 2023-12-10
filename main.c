/*
 * Configuration
 */
#include "flags.h"

/*
 * Includes portables
 */
#include <stdio.h>
#include <stdlib.h>
#ifdef EVAL_SPEED
#include <time.h>
#endif
#include <string.h>

/*
 * Includes specifiques a la plateforme
 */
#ifdef DJGPP
#include <dir.h>
#include <pc.h>
#endif

#ifdef USE_ALLEGRO
#include <allegro.h>
#endif /* ALLEGRO */

/*
 * Includes propres a l'emulateur
 */
#include "regs.h"
#include "mouse.h"	/* sam: je l'ai deplace ici pour eviter un pb avec une redef de 
				BYTE dans les os-includes amiga definissante mouse_x,y */
#include "mapper.h"
#include "debug.h"
#include "mc6809.h"
#include "setup.h"
#include "setup2.h"
#include "index.h"
#include "graph.h"
#include "keyb.h"
#include "keyb_int.h"
#include "sound.h"
#include "diskio.h"
#include "main.h"

#include "zfile.h"

/* Lecture de l'instruction/operandes */
#define FETCH m=LOAD8(PC);PC++

/* compteur de frame video */
#ifdef VBL_INT
volatile 
#endif
int vbl;

/* drapeau de mode "exact speed" */
int exact;

/* gestion des K7 virtuelles */
extern FILE *K7in;

/* sam: il manque un gui.h */
extern void menu(void);
extern void menu2(void);
#ifdef AMIGA
extern void menu3(void);
#endif

/*
 *  benh, la ca s'est mal passe ...
 */
void crash(void)
{
    fprintf(stderr, "Abnormal termination.");
    exit(EXIT_FAILURE);
}

/*
 * Sortie normale
 */
void normal_end(void) 
{
    exit(EXIT_SUCCESS);
}

/*
 *  point d'acces aux I/O patchees
 */

void perif(void)
{
    switch (PC)
    {
        case 0x315A:  /* routine de selection souris/crayon optique */
            InstallPointer(GETMAP(0x6074)&0x80 ? MOUSE : LIGHTPEN);
            LDXi();
            break;

        case 0x337E:  /* routine GETL crayon optique   */
        case 0x3F97:  /* routine GETL crayon optique 2 */
            switch(GE7DC)
            {
                case 0x2A: /* mode 80 colonnes */
                    X=mouse_x;
                    Y=mouse_y/2;
                    break;

                case 0x3F: /* mode superposition 4 pages */
                case 0x7B: /* mode bitmap 16 couleurs */
                    X=mouse_x/2;
                    Y=mouse_y;
                    break;

                default:
                    X=mouse_x;
                    Y=mouse_y;
                    break;
            }
            res&=0xff;  /* la lecture est toujours bonne */
            break;

        case 0x3686:  /* page de modification palette: debut */
            GE7DC=SPECIAL_MODE;
            SetGraphicMode(INIT);
            LDAi();
            break;

        case 0x357A:  /* page de modification palette: fin */
            if(GE7DC == SPECIAL_MODE) {
	        GE7DC=0;
        	SetGraphicMode(INIT); 
            }
            TSTe();
            break;

        case 0xF42A:  /* routine SCAN souris */
            switch (GE7DC)
            {
                case 0x2A: /* mode 80 colonnes */
                    GETMAP(0x60D6)=(mouse_y/2)/256;
                    GETMAP(0x60D7)=(mouse_y/2)%256;
                    GETMAP(0x60D8)=(mouse_x/2)/256;
                    GETMAP(0x60D9)=(mouse_x/2)%256;
                    break;

                case 0x3F: /* mode superposition 4 pages */
                case 0x7B: /* mode bitmap 16 couleurs */
                    GETMAP(0x60D6)=(mouse_y)/256;
                    GETMAP(0x60D7)=(mouse_y)%256;
                    GETMAP(0x60D8)=(mouse_x/2)/256;
                    GETMAP(0x60D9)=(mouse_x/2)%256;
                    break;

                default:
                    GETMAP(0x60D6)=(mouse_y)/256;
                    GETMAP(0x60D7)=(mouse_y)%256;
                    GETMAP(0x60D8)=(mouse_x)/256;
                    GETMAP(0x60D9)=(mouse_x)%256;
                    break;
            }

            port0|=0x80;
            MON[0][0x7C0]=MON[1][0x7C0]=port0;

            break;

        case 0xFA5A:  /* cette partie provient de funzyTO770 */
            switch (GETMAP(0x6029))
            {
                case 1:
                    GETMAP(0x602A) = 1;
                    break;

                case 2:
		    if (K7in != NULL)
                    	B=fgetc(K7in)&0xFF;
                    break;

                case 4:
                case 8:
                default:
                    GETMAP(0x602A) = 16;
            }
            res&=0xff;
            break;

        case 0xFCF5:  /* routine de modification de la palette */
            ANDBd();
            map8(0xE7DA,MON[0][0x7DA]);  /* on avance le curseur palette */
            break;

 	/* IO Diskette */
        case 0xE0FF:
             InitDiskCtrl();
             break;

        case 0xE3A8:
#ifdef AMIGA
	    diskled_timeout = 50; /* on allume la pseudo led du disk */
				  /* pendant 50/50=1 secondes */
#endif
            ReadSector();
            break;

        case 0xE178:
#ifdef AMIGA
	     diskled_timeout = 50;
#endif
             WriteSector();
             break;

    } /* end of switch */
}


/*
 *	lecture de la ROM (+cartouche pour TO)
 */

static void LoadRom(void)
{
  register int i,j;
  FILE *file;
  char  rom_name[6][13]={ "b512_b0.rom","b512_b1.rom","basic1.rom",
                          "fichier.rom","to8mon1.rom","to8mon2.rom" };
  int rom_length[6]={ 0x4000,0x4000,0x4000,0x4000,0x2000,0x2000 };
#ifndef __SASC
  BYTE8 *rom_loc[6]={ ROM[0],ROM[1],ROM[2],ROM[3],MON[0],MON[1] };
#else /* sam: SAS/C n'aime pas les initialisations non constantes */
  BYTE8 *rom_loc[6];
  rom_loc[0] = ROM[0]; rom_loc[1] = ROM[1]; rom_loc[2] = ROM[2]; 
  rom_loc[3] = ROM[3]; rom_loc[4] = MON[0]; rom_loc[5] = MON[1];
#endif
  for (j=0; j<6; j++)
  {
	file=fopen(rom_name[j],"rb");
#ifdef AMIGA
	if(!file) {
	  	char buf[80];
	  	strcpy(buf, "PROGDIR:");
	  	strcpy(buf+8, rom_name[j]);
	  	file=fopen(buf, "r");
	}
#endif
	if (file == NULL)
	{
	  /* pour eviter de perdre un temps precieux ... */
	  fprintf(stderr,"Manque le fichier :%s",rom_name[j]);
	  crash();
	}

      for (i=0;i<rom_length[j];i++)
          rom_loc[j][i]=fgetc(file)&0xFF;
      fclose(file);
  }
}

/*
 * ColdReset: simule un de/rebranchement du TO8
 */
void ColdReset(void)
{
  /* cette ligne entraine la mise a zero par le TO8 de tous les registres du
     moniteur (plage 6000-6FFF) */
    GETMAP(0x60FF) = 0;

    port0=0x80;
    MON[0][0x7C0]=MON[1][0x7C0]=port0;

    PRC=0xFD;   /*  bit 1: enfoncement du bouton du crayon optique */
    MON[0][0x7C3]=MON[1][0x7C3]=PRC;

    DDRA2=0;
    ORA2=0xFF;
    MON[0][0x7CC]=MON[1][0x7CC]=ORA2;

    DDRB2=0;
    ORB2=0xFF;
    MON[0][0x7CD]=MON[1][0x7CD]=ORB2;

    CRA2=0xC4;
    MON[0][0x7CE]=MON[1][0x7CE]=CRA2;

    CRB2=0xC4;
    MON[0][0x7CF]=MON[1][0x7CF]=CRB2;

    InstallPointer(MOUSE); /* la souris est le perif. de pointage par
                               defaut au demarrage */

    GE7DC=0;
    MON[0][0x7DC]=MON[1][0x7DC]=GE7DC;
    SetGraphicMode(INIT);
    mode80_vbl=0;

    PC=(LOAD8(0xFFFE)<<8)|LOAD8(0xFFFF);
    DP=0;
    S=0x8000;
    CC=0;
    setcc(CC);
}

/*
 * Met a jour le registe E7E7
 */
static inline void uE7E7(int V)
{
  GE7E7     = V;
  *MON0_7E7 = V;
  *MON1_7E7 = V;
}

#ifdef NO_WAIT
#define wait 0
#endif

/*
 * Fetch and exec
 */
static inline void fetch_n_exec(void)
{
   void (*pf)(); 
#ifdef REG_M
   M=0;
#endif
#if defined(REG_PC) && defined(REG_m) && defined(REG_MAPPER)
   __asm__ __volatile__(
       "bfextu "REG_PC"{#16:#4},"REG_m"
        movel "REG_MAPPER"@("REG_m":l:4),%0 
        moveb %0@("REG_PC":l),"REG_m"
        addqw #1,"REG_PC"
        movel %1@("REG_m":l:4),%0" : "=a" (pf) : "a" (PFONCT)
   ); 
   (*pf)();
#else
   m = LOAD8(PC); ++PC; 
   (*PFONCT[m])();
#endif
}

/*
 * interrupt timer
 */
static char precise_timer = 0;
static int  timer_val = 12499*8;
static inline int timerout(unsigned int nb_cycles)
{
#ifndef REG_m
   int
#endif
   m = 0;
#ifndef REG_M
   int
#endif	 
   M  = timer_val;
   M -= nb_cycles;
   if((int)M < 0) {
      M += (MON[0][0x7C6]*256 + MON[0][0x7C7])*8;
      if((int)M < 0) M = 0;
      precise_timer = ((int)M < 1000000/50);
      m = 1;
   }
   timer_val = M;
   return m;
}

static unsigned int IRQ_cl, IRQ_inst;
static void ChkIRQ(void)
{
   unsigned int nb_cycles = cl - IRQ_cl;
   IRQ_cl = cl;
   if(timerout(nb_cycles)) {
      MON[0][0x7C0] = MON[1][0x7C0] = (port0 |= 1);
      IRQ(); IRQ_inst=10; do { fetch_n_exec(); --IRQ_inst;} while(IRQ_inst);
      MON[0][0x7C0] = MON[1][0x7C0] = (port0 &=~1);
   } else if (TO_keypressed) {TO_keypressed=0; IRQ();}  
}

/*
 * Run6809AndSyncOnVideo: fait tourner le 6809 en le synchronisant sur le
 *                        faisceau video ligne par ligne
 */
static unsigned int Run6809AndSyncOnVideo(int number_of_lines, unsigned int exact_cl)
{
    /* on ne verifie pas le clavier pour toutes les lignes */
    while(number_of_lines--) {
      /* bordure gauche de la ligne non mappee sur la VRAM */
      exact_cl += (CYCLES_PER_LINE-BYTES_PER_LINE)/2;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();

      /* partie centrale de la ligne mappee sur la VRAM (40 octets) */
      uE7E7(GE7E7 | 0x20);
      exact_cl += BYTES_PER_LINE;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();

      /* bordure droite de la ligne non mappee sur la VRAM */
      uE7E7(GE7E7 & 0xDF);
      exact_cl += (CYCLES_PER_LINE-BYTES_PER_LINE)/2;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();
   }
   return exact_cl;
}

/* pareil qu'au dessus mais on verifie les IRQ sur chaque ligne */
static unsigned int Run6809AndChkIRQ(unsigned int exact_cl)
{
    short int number_of_lines = LINES_IN_SCREEN_WINDOW;
    /* on ne verifie pas le clavier pour toutes les lignes */
    while(number_of_lines--) {
      ChkIRQ();
      /* bordure gauche de la ligne non mappee sur la VRAM */
      exact_cl += (CYCLES_PER_LINE-BYTES_PER_LINE)/2;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();

      /* partie centrale de la ligne mappee sur la VRAM (40 octets) */
      uE7E7(GE7E7 | 0x20);
      exact_cl += BYTES_PER_LINE;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();

      /* bordure droite de la ligne non mappee sur la VRAM */
      uE7E7(GE7E7 & 0xDF);
      exact_cl += (CYCLES_PER_LINE-BYTES_PER_LINE)/2;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();
   }
   return exact_cl;
}

/*
 * Run6809AndRefreshScreen: fait tourner le 6809 en rafraichissant l'ecran
 *                          (fonction inline car appelee en un seul point)
 */
static unsigned int Run6809AndRefreshScreen(unsigned int exact_cl)
{
    int gpl_no = 0;
    register short int i,j,k;

    for (i=LINES_IN_SCREEN_WINDOW; i--;) {
      ChkIRQ();
      /* bordure gauche de la ligne non mappee sur la VRAM */
      exact_cl += (CYCLES_PER_LINE-BYTES_PER_LINE)/2;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();

      /* partie centrale de la ligne mappee sur la VRAM */
      uE7E7(GE7E7 | 0x20);
      for (j=BYTES_PER_LINE/LINE_GRANULARITY; j--;) {
         /* on decoupe la ligne en petits groupes d'octets dont la
         longueur est LINE_GRANULARITY */
         for (k=LINE_GRANULARITY; k--;) refresh_gpl(gpl_no++);
         exact_cl += LINE_GRANULARITY;
         while ((!wait) && (cl<exact_cl)) fetch_n_exec();
      }

      /* bordure droite de la ligne non mappee sur la VRAM */
      uE7E7(GE7E7 & 0xDF);
      exact_cl += (CYCLES_PER_LINE-BYTES_PER_LINE)/2;
      while ((!wait) && (cl<exact_cl)) fetch_n_exec();
   }
   return exact_cl;
}

/*
 * emule le cpu le temps d'un ballayage vertical
 */
static inline void do_vbl(void)
{
   unsigned int exact_cl = 0; /* debut de la frame video */
   
   m=M=0; /* pour nettoyer les debordements accidentels */

#ifndef VBL_INT
   ++vbl;
#endif

   /* bordure haute de l'écran */
   exact_cl = Run6809AndSyncOnVideo(LINES_IN_TOP_SCREEN_BORDER, exact_cl);

   /* fenetre centrale de l'ecran */
   uE7E7(GE7E7 | 0x80); 
   if (screen_refresh) { /* rafraichissement de l'ecran ? */
        screen_refresh = FALSE;
        exact_cl = Run6809AndRefreshScreen(exact_cl);
   } else if(precise_timer) {
   	exact_cl = Run6809AndChkIRQ(exact_cl);
   } else {
	exact_cl = Run6809AndSyncOnVideo(LINES_IN_SCREEN_WINDOW, exact_cl);
   }

   /* bordure du bas de l'écran */
   uE7E7(GE7E7 & 0x7F);
   exact_cl = Run6809AndSyncOnVideo(LINES_IN_BOTTOM_SCREEN_BORDER, exact_cl);

   /* passage eventuel en mode 80 colonnes statique */
   if (vbl == mode80_vbl) SetGraphicMode(MODE80);

   /* fin de la frame video et remontee du faisceau */
   ChkIRQ();
   while (cl<CYCLES_PER_VIDEO_FRAME) fetch_n_exec();
       cl -= CYCLES_PER_VIDEO_FRAME;
   IRQ_cl -= CYCLES_PER_VIDEO_FRAME; 
}

/*
 * Affiche la vitesse de l'emul
 */
static void eval_speed(void)
{
#ifdef EVAL_SPEED
#       define     SEC   2 /* affichage toutes les <SEC> sec */
	static int cpt = 50*SEC;
	if(!--cpt) {
		static clock_t last_t;
		clock_t t = clock();
		cpt = 50*SEC;
		if(t-last_t) {
		      fprintf(stderr,"Speed=%ld%%            \r",
		             (100*CLOCKS_PER_SEC*SEC)/(t-last_t));
		      fflush(stderr);
		}
		last_t = t;		
	}
#endif
}

/*
 * mode command
 */ 
static void do_command(void)
{
     /* et si on vidait le buffer sonore hein? */
     CLEAR_SOUND();

     ShutDownKeyboard(); /* mise au repos du clavier TO8 */
     ShutDownPointer();  /* mise au repos de la souris TO8 */

     switch (command) {
     case ACTIVATE_MENU: 
        menu();  
        break;
     case OPTION_MENU:   
        menu2(); break;
#ifdef AMIGA
     case GFX_MENU:
        menu3(); break;
#endif
#ifndef NO_DEBUGGER
     case DEBUGGER:
#ifdef DJGPP
        DOS_InstallKeyboard(BIOS_KB);
#endif /* DJGPP */
        ReturnText();
        debugger();
        SetGraphicMode(INIT);
        break;
#endif /* NO_DEBUGGER */
     default:
        menu();
     }
     InstallPointer(PREVIOUS);
     InstallKeyboard();
}

/*
 *  boucle principale de l'emulateur
 */
static void full_speed(void)
{
#ifdef VBL_INT
   int lastvbl=0;
#ifdef DJGPP
   LOCK_VARIABLE(vbl);
   LOCK_FUNCTION(vblupdate);
#endif
#ifdef USE_ALLEGRO
   install_timer();
   install_int_ex(vblupdate, BPS_TO_TIMER(VIDEO_FRAME_FREQ));
#endif /* USE_ALLEGRO */
#endif /* VBL_INT */

   cl=0; vbl=0; exact=-1;

   InstallKeyboard();  /* installation du clavier TO8 */
   init_thom_sound();  /* installation du son         */

//   SetGraphicMode(INIT);

   ColdReset();
   command=0;

   while (TRUE) {  /* boucle principale de l'emulateur */
#ifdef AMIGA
	handle_events();
#endif
        if (command) do_command();
        
        do_vbl();

        /* gestion du son et synchronisation sur frequence reelle */
        CALC_SOUND();

#ifdef VBL_INT
	/* sync with frame rate */
        if (exact) while (vbl==lastvbl) {}
        lastvbl=vbl;
#endif

        SWAP_SOUND();
#ifndef NO_WAIT
	/* version + mieux bien du Wait ... */
        if (wait) {cl = 0; wait = 0;}
#endif
   	eval_speed();
    }  /* fin de la boucle principale de l'emulateur */
}


int main(int argc, char** argv)
{
#ifdef USE_ALLEGRO /* sam: */
        allegro_init();
#endif
	printf("TEO - Version alpha 0.6 Aout 1998\n");
	printf("Emulateur                 : Gilles Fetis 1997/1998\n");
	printf("Clavier/Souris/GFX/BugFix : Eric Botcazou     1998\n");
	printf("Gestion protection disk   : Alex Pukall       1998\n");
        printf("Design Menus              : Jeremie Guillaume 1998\n");
#ifdef AMIGA
        printf("Adaptation Amiga          : Samuel Devulder   1998-2002\n");
#endif
        printf("\n");
	printf("\n\nMail : FETIS_Gilles@compuserve.com");
	printf("\n       Eric.Botcazou@ens.fr");
#ifdef AMIGA
	printf("\n       Samuel.Devulder@info.unicaen.fr");
#endif
	printf("\n\nAide rapide: [ESC] Appel du menu / Activate menu");
#ifdef AMIGA
	printf("\n             [F6]  Choix disque 0 / Choose disk 0");
	printf("\n             [F7]  Choix disque 1 / Choose disk 1");
	printf("\n             [F8]  Choix mode gfx / Choose gfx mode");
#endif
	printf("\n             [F9]  Menu des Options / Option Menu");
#ifndef NO_DEBUGGER
	printf("\n             [F10] Debogueur / Debugger");
#endif
#ifndef AMIGA
	printf("\n\nAppuyez sur une touche / Hit any key!\n");

        while (!kbhit())
            ;
#else
	printf("\n\n");
#endif

	/* setup de tous les modules */
	setup_mem();
	setup_6809();
	setup_index();

#ifndef NO_DEBUGGER
        setup_desass();
#endif

        LoadRom();
        InitKeyboard(); /* avant setupgraph ou sinon les prefs numlock sont pas prises en compte */
        SetupGraph(); /* apres LoadRom() */
        InitPointer();

/*
	PATCH une partie des fonctions du moniteur

	la sequence 02 39 correspond a
	Illegal (instruction)
	RTS
	le TRAP active la gestion des
	peripheriques, la valeur du
	PC a ce moment permet de determiner
	la fonction a effectuer

*/

/* Init du ctrleur disk */
    MON[0][0x00FE]=0x02;
    MON[0][0x00FF]=0x39;

/* lecture d'un secteur disk */
    MON[0][0x03A7]=0x02;
    MON[0][0x03A8]=0x39;

/* écriture d'un secteur disk */
    MON[0][0x0177]=0x02;
    MON[0][0x0178]=0x39;

/* routine de gestion K7 */
    MON[0][0x1A59]=0x02;
    MON[0][0x1A5A]=0x39;

/* routine de modification de la palette */
    MON[0][0x1CF4]=0x02;

/* routine SCAN souris */
    MON[1][0x1429]=0x02;
    MON[1][0x142A]=0x39;

/* routine de selection souris/crayon optique */
    ROM[3][0x3159]=0x02;

/* routine GETL crayon optique */
    ROM[3][0x337D]=0x02;
    ROM[3][0x337E]=0x39;

/* routine GETL crayon optique 2 */
    ROM[3][0x3F96]=0x02;
    ROM[3][0x3F97]=0x39;

/* page de modification palette: debut */
    ROM[3][0x3685]=0x02;

/* page de modification palette: fin */
    ROM[3][0x3579]=0x02;

/* Ligne de commande */
    do {
      int i;
      for(i=1;i<argc;) {
         char *curr = argv[i++];
         char *next = i<argc?argv[i]:"";
         if(!strcmp(curr, "-0")) {LoadSAP(next, 0); ++i;} else
         if(!strcmp(curr, "-1")) {LoadSAP(next, 1); ++i;} else
         if(!strcmp(curr, "-7")) {K7in = fopen(next, "r"); ++i;} else
         if(!strcmp(curr, "-m")) {
            FILE *f = fopen(next, "r"); ++i;
            if(f) {int i;
               for (i=0x0;i<0x2000;i++) EXT[i]=fgetc(f)&0xFF;
               fclose(f);
            }
         } else {
            printf("Usage: %s [-0 file] [-1 file] [-7 file] [-m file]\n", 
                    argv[0]);
            normal_end();
         }
      } 
    } while(0);

    full_speed();  /* lance l'emulation */
    normal_end();
}
