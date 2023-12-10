/*

	VERSION     : 2.0
	MODULE      : gui.c
	Créé par    : Gilles FETIS
               : Jéremie GUILLAUME - alias "JnO"

   Gestion MENU

*/

#include <stdio.h>
#include <stdlib.h>
#ifdef DJGPP
#include <dir.h>
#endif
#include <sys/stat.h>
#include <allegro.h>
#include "mapper.h"
#include "sound.h"
#include "regs.h"
#include "graph.h"

#include "persfsel.h"

/* gestion des K7 virtuelles */
FILE *K7in=NULL;

extern int exact;

/***************************************************************************
 ***  Dialog boxes code using Allegro's GUI                              ***
 ***************************************************************************/

void menu2(void);


DIALOG k7dial[] =
{
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,    19,   20,   0,    D_DISABLED,0,  0,    NULL },
{ d_ctext_proc,      160,  30,    0,    0,    19,   20,   0,    0,       0,    0,    "Gestion des cassettes" },
{ d_button_proc,     30,   50,  260,   16,    19,   20,   'l',    D_EXIT,  0,    0,    "Mettre une K7 en &lecture" },
{ d_button_proc,     30,   74,  260,   16,    19,   20,   'r',    D_EXIT,  0,    0,    "&Rembobiner la K7 en lecture" },
{ d_button_proc,     30,   116,  260,   16,    19,   20,   'e',    D_EXIT,  0,    0,    "Mettre une K7 en &enregistrement" },
{ d_button_proc,    164,  170,   126,   16,   19,   20,    0,    D_EXIT,  0,    0,    "RETOUR" },
{ NULL,               0,    0,    0,    0,     0,    0,   0,    0,       0,    0,    NULL }
};

void menuk7(void)
{
    static int first=1;
    int ret;

    /* nom complet du fichier */  
    static char K7fname[1024];
  
    /* la premiere fois on tente d'ouvrir le repertoir par defaut */
    if (first)
    {
	struct stat s;
	/* verifie la presence du repertoire K7 */
    	stat("./k7", &s);
    	if (S_ISDIR(s.st_mode))
	{
	   strcpy(K7fname,"./k7/");	
	}      	
	else
	{
	   strcpy(K7fname,".");	
	}

	first=0;
    }


    clear_keybuf();
    ret=popup_dialog(k7dial,-1);

    if (ret==2)
    {
        int ret2;
        ret2=file_select("Choisissez votre K7",K7fname,"K7");
        if (ret2)
        {
            fclose(K7in);
            K7in=fopen(K7fname,"rb");
        }
    }
    else
    if ((ret==3)&&(K7in!=NULL))
    {
        fseek(K7in,0,SEEK_SET);
    }
    else
    if (ret==4)
    {
    }
    /* fileselect a un petit bug ! */
    screen_refresh=TRUE;
}

DIALOG diskdial[] =
{
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,    19,   20,   0,    D_DISABLED,0,  0,    NULL },
{ d_ctext_proc,      160,  30,    0,    0,    19,   20,   0,    0,       0,    0,    "Gestion des disquettes" },
{ d_button_proc,     30,   50,  260,   16,    19,   20,   '0',    D_EXIT,  0,    0,    "Disk &0 (interne face inf)" },
{ d_button_proc,     30,   74,  260,   16,    19,   20,   '1',    D_EXIT,  0,    0,    "Disk &1 (interne face sup)" },
{ d_button_proc,     30,   98,  260,   16,    19,   20,   '2',    D_DISABLED,  0,    0,    "Disk &2 (externe face inf)" },
{ d_button_proc,     30,  122,  260,   16,    19,   20,   '3',    D_DISABLED,  0,    0,    "Disk &3 (externe face sup)" },
{ d_button_proc,     30,  146,  260,   16,    19,   20,   '3',    D_DISABLED,  0,    0,    "conversion TDS -> SAP" },
{ d_button_proc,    164,  170,   126,   16,   19,   20,    0,    D_EXIT,  0,    0,    "RETOUR" },
{ NULL,               0,    0,    0,    0,     0,    0,   0,    0,       0,    0,    NULL }
};

void menudisk(void)
{
    static int first=1;
    int ret;

    /* nom complet du fichier */  
    static char DISKfname[1024];
  
    /* la premiere fois on tente d'ouvrir le repertoir par defaut */
    if (first)
    {
	struct stat s;
	/* verifie la presence du repertoire K7 */
    	stat("./disks", &s);
    	if (S_ISDIR(s.st_mode))
	{
	   strcpy(DISKfname,"./disks/");
	}      	
	else
	{
	   strcpy(DISKfname,".");	
	}

	first=0;
    }



    clear_keybuf();
    ret=popup_dialog(diskdial,-1);

    if (ret==2)
    {
        int ret2;
        ret2=file_select("Choisissez votre DISK 0",DISKfname,"SAP");
        if (ret2)
        {
            LoadSAP(DISKfname,0);
        }
    }
    else
    if (ret==3)
    {
        int ret2;
        ret2=file_select("Choisissez votre DISK 1",DISKfname,"SAP");
        if (ret2)
        {
            LoadSAP(DISKfname,1);
        }
    }
    /* fileselect a un petit bug ! */
    screen_refresh=TRUE;

}

void menumemo7(void)
{
    static int first=1;
    int ret;
    static char MEMO7fname[1024];
    FILE * fin;
    unsigned int i;
    char c;


    /* la premiere fois on tente d'ouvrir le repertoir par defaut */
    if (first)
    {
	struct stat s;
	/* verifie la presence du repertoire memo7 */
    	stat("./memo7", &s);
    	if (S_ISDIR(s.st_mode))
	{
	   strcpy(MEMO7fname,"./memo7/");
	}      	
	else
	{
	   strcpy(MEMO7fname,".");	
	}

	first=0;
    }

    ret=file_select("Choisissez votre MEMO7",MEMO7fname,"M7");

    if (ret)
    {
    	fin=fopen(MEMO7fname,"rb");
    	for (i=0x0;i<0x2000;i++)
    	{
           c=fgetc(fin);
           EXT[i]=c&0xFF;
    	}
    	fclose(fin);
    }
    
    /* fileselect a un petit bug ! */
    screen_refresh=TRUE;

}

DIALOG menudial[] =
{
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,  280,  180,    19,   20,   0,    D_DISABLED,0,  0,    NULL },
{ d_ctext_proc,      160,  20,    0,    0,    19,   20,   0,    0,       0,    0,    "TEO ALPHA 0.6" },
{ d_ctext_proc,      160,  30,    0,    0,    19,   20,   0,    0,       0,    0,    " Gilles Fétis - Eric Botcazou " },
{ d_ctext_proc,      160,  40,    0,    0,    19,   20,   0,    0,       0,    0,    " Alex Pukall - Jérémie Guillaume " },
{ d_button_proc,     30,   60,  260,   16,    19,   20,   'd',    D_EXIT,  0,    0,    "&Disquettes..." },
{ d_button_proc,     30,   84,  260,   16,    19,   20,   'k',    D_EXIT,  0,    0,    "&K7..." },
{ d_button_proc,     30,   108,  260,   16,    19,   20,   'm',    D_EXIT,  0,    0,    "&Mémo 7..." },
{ d_button_proc,     30,  132,  260,   16,    19,   20,   'o',  D_EXIT,  0,    0,    "&Options && RESET..." },
{ d_button_proc,     30,  170,   126,   16,   19,   20,   'q',    D_EXIT,  0,    0,    "&QUITTER" },
{ d_button_proc,    164,  170,   126,   16,   19,   20,    0,    D_EXIT,  0,    0,    "RETOUR" },
{ NULL,               0,    0,    0,    0,     0,    0,   0,    0,       0,    0,    NULL }
};

void menu(void)
{
   int ret;

   gui_fg_color=19;gui_bg_color=20;

   /* we never read the key buffer, we have to clear it to avoid problems */
   clear_keybuf();
   ret=popup_dialog(menudial,-1);

   if (ret==5)
   {
       menuk7();
   }
   else
   if (ret==4)
   {
       menudisk();
   }
   else
   if (ret==6)
   {
       menumemo7();
   }
   else
   if (ret==7)
   {
      menu2();
   }
   else
   if (ret==8)
   {
      fclose(K7in);
      normal_end();
   }
    /* fileselect a un petit bug ! */
    screen_refresh=TRUE;

}

DIALOG menudial2[] =
{
/* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp) */
{ d_shadow_box_proc,  20,  10,   280,  180,   19,   20,      0,   D_DISABLED,0,  0,    NULL },
{ d_ctext_proc,      160,  20,     0,    0,   19,   20,      0,   0,       0,    0,    "TEO alpha 0.6" },
{ d_ctext_proc,      160,  30,     0,    0,   19,   20,      0,   0,       0,    0,    "Menu des comm/options avancées" },
{ d_button_proc,      30,  40,   260,   16,   19,   20,    'r',   D_EXIT,  0,    0,    "&Réinitialiser le TO8" },
{ d_button_proc,      30,  64,   260,   16,   19,   20,    'h',   D_EXIT,  0,    0,    "RESET &hard" },
{ d_radio_proc,       30,  88,   126,   16,   19,   20,    'e',   0,       1,    0,    "Vitesse &exacte" },
{ d_radio_proc,      166,  88,   126,   16,   19,   20,    'd',   0,       1,    0,    "a &donf..." },
{ d_radio_proc,       30, 112,   126,   16,   19,   20,    's',   0,       2,    0,    "&son" },
{ d_radio_proc,      166, 112,   126,   16,   19,   20,    'p',   0,       2,    0,    "&pas de son" },
{ d_button_proc,    164,  170,   126,   16,   19,   20,      0,    D_EXIT,  0,    0,    "RETOUR" },
{ NULL,                0,   0,     0,    0,    0,    0,      0,   0,       0,    0,    NULL }
};

void menu2(void)
{
   int ret;

   gui_fg_color=19;gui_bg_color=20;

   /* we never read the key buffer, we have to clear it to avoid problems */
   clear_keybuf();
   if (exact) {menudial2[5].flags=D_SELECTED;menudial2[6].flags=0;}
   else       {menudial2[5].flags=0;         menudial2[6].flags=D_SELECTED;}

   if (thomsound) {menudial2[7].flags=D_SELECTED;menudial2[8].flags=0;}
   else           {menudial2[7].flags=0;         menudial2[8].flags=D_SELECTED;} 

   ret=popup_dialog(menudial2,-1);

   if ((menudial2[5].flags & D_SELECTED)!=0) exact=-1;
   else exact=0;

   if ((menudial2[7].flags & D_SELECTED)!=0) thomsound=-1;
   else thomsound=0;
   
   if (ret==3)
   {
      PC=(LOAD8(0xFFFE)<<8)|LOAD8(0xFFFF);
   }

   else if (ret==4)
       ColdReset();

    /* fileselect a un petit bug ! */
    screen_refresh=TRUE;

}



/***************************************************************************
 *** End of dialog boxes code                                            ***
 ***************************************************************************/

