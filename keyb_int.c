/*
 * keyb_int.c: module de gestion de l'interruption materielle clavier INT 9
 *             et de commutation entre les differents handlers.
 *   Il installe la fonction MyKeyInt() comme handler d'interruption en mode
 *  protege, en utilisant le module de gestion des irq d'Allegro.
 *   Le handler realise un traitement primaire de l'evenement clavier et
 *  transmet le resultat a la fonction qui a ete passee en argument
 *  de DOS_InitKeyboar(), sous la forme d'un scancode 8 bits (le bit 7
 *  signalant une touche etendue) et d'un flag de frappe/relachement.
 *
 *  plateforme: MS-DOS
 *  code base sur le module \djgpp\keyboard.c de Allegro 3.0
 */

#include <stdlib.h>
#ifdef DJGPP
#include <pc.h>
#include <sys/movedata.h>
#endif
#include <allegro.h>
#include "keyb_int.h"

#ifdef DJGPP

#define KEYBOARD_INT 9


/* cette fonction du module irq.c n'est pas declaree dans allegro.h */
extern int  _install_irq(int, int(* )(void));

static void (*KeyboardCallBack)(int, int)=NULL;
static __dpmi_paddr  kb_vector[3];
 /* structures de sauvegarde du vecteur d'interruption du clavier: une pour
    le BIOS, une pour le handler d'Allegro, une pour le handler TO8 */


/*
 * MyKeyInt: handler pour l'interruption materielle clavier INT 9
 *           Il est appele des qu'une touche est frappee ou relachee.
 */

static int MyKeyInt(void)
{
   static   int key_pause_loop;
   static   int key_extended;
            int key,release;

   key=inportb(0x60);         /* lit le scancode materiel de la touche */

   if (key_pause_loop)
       key_pause_loop--;

   else if (key == 0xE1)
       key_pause_loop=5;      /* on saute les 5 codes suivants */

   else if (key == 0xE0)      /* touche etendue, on lit le code suivant */
       key_extended=1;

   else
   {
       release = (key&0x80)>>7; /* bit 7 allume: la touche est relachee */
       key &= 0x7f;             /* bits 0-6: le scancode proprement dit */

       if (key_extended)
       {
           key|=0x80;    /* on allume le bit 7 pour les touches etendues */
           key_extended=0;
       }

       (*KeyboardCallBack)(key, release);
        /* appel de la routine de gestion du clavier */
   }

   outportb(0x20,0x20);       /* on remercie l'interruption */
   return 0;                  /* on arrete ici la chaine de l'interruption */
}

static END_OF_FUNCTION(MyKeyInt)


/*
 * DOS_InstallKeyboard: installe le handler d'interruption clavier passe
 *                      en argument
 */

void DOS_InstallKeyboard(int mode)
{
    __dpmi_set_protected_mode_interrupt_vector(KEYBOARD_INT,&kb_vector[mode]);
}


/*
 * DOS_InitKeyboard: initialise les trois handlers clavier et sauvegarde
 *                   leur vecteur d'interruption
 *  func(int, int): fonction de callback clavier acceptant deux arguments, le
 *    premier est le scancode et le second le flag frappe/relachement.
 */

void DOS_InitKeyboard(void (*func)(int, int))
{
    LOCK_FUNCTION(MyKeyInt);
    KeyboardCallBack=func;

  /* on recupere la valeur du vecteur d'interruption de l'interruption
     clavier INT 9 qui pointe pour le moment sur le handler du BIOS et on le
     stocke dans la stucture appropriee bios_kb_vector */
    __dpmi_get_protected_mode_interrupt_vector(KEYBOARD_INT,&kb_vector[0]);

  /* on recupere la valeur du vecteur d'interruption de l'interruption
     clavier INT 9 qui pointe sur le handler d'Allegro et on le stocke
     dans la stucture appropriee allegro_kb_vector */
    install_keyboard();  /* installe le handler clavier d'Allegro */
    __dpmi_get_protected_mode_interrupt_vector(KEYBOARD_INT,&kb_vector[1]);


  /* il n'est pas correct de passer la fonction MyKeyInt() directement comme
     handler a l'interruption clavier a l'aide des fonctions ad hoc de la
     librairie C standard; en effet tout ce qui est touche pendant
     l'interruption doit etre verrouille en memoire physique, y compris le
     bout de code (wrapper) qui fait la liaison entre l'interruption et la
     fonction C, et ceci n'est pas assure par libc (oubli ou volonte de faire
     simple ? voir la djgpp faq paragraphe 18.9). Il faut donc utiliser le
     module irq.c d'Allegro, qui lui implemente correctement les
     interruptions (reentrantes) en mode protege */
    _install_irq(KEYBOARD_INT, MyKeyInt);

  /* on recupere la valeur du vecteur d'interruption de l'interruption
     clavier INT 9 qui pointe sur le handler d'Allegro et on le stocke dans
     la stucture appropriee to8_kb_vector */
    __dpmi_get_protected_mode_interrupt_vector(KEYBOARD_INT,&kb_vector[2]);


  /* on redonne le controle au BIOS */
    __dpmi_set_protected_mode_interrupt_vector(KEYBOARD_INT,&kb_vector[0]);
}

#endif

#ifdef Xwin_ALLEGRO

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

extern volatile int _Xwin_interrupts_disabled;

#define ENABLE() ( _Xwin_interrupts_disabled = FALSE )
#define DISABLE() ( _Xwin_interrupts_disabled = TRUE )

static void (*KeyboardCallBack)(int, int)=NULL;

static void keyboardint2 (int temp);
static void (*_Xwin_keyboardint) (int temp) = keyboardint2;

void
_Xwin_keyboard_update2 (int temp)
{
  (_Xwin_keyboardint) (temp);
}

/* keyboardint:
 *  Hardware level keyboard interrupt (int 9) handler.
 */
void
keyboardint2 (int temp)
{
   static   int key_pause_loop=0;
   static   int key_extended=0;
   int release;
   
   if (key_pause_loop)
       key_pause_loop--;

   else if (temp == 0xE1)
       key_pause_loop=5;      /* on saute les 5 codes suivants */

   else if (temp == 0xE0)      /* touche etendue, on lit le code suivant */
       key_extended=1;

   else
   {
      release = (temp & 0x80);      /* bit 7 means key was released */
      temp &= 0x7F;                 /* bits 0-6 is the scan code */
      
      if (key_extended)
      {
           temp|=0x80;    /* on allume le bit 7 pour les touches etendues */
           key_extended=0;
       }
	
      (*KeyboardCallBack)(temp,release);	
   }
}

void install_XwinAllegKEY(void (*func)(int, int))
{
   KeyboardCallBack=func;

   DISABLE();
   _Xwin_keyboardint = keyboardint2;
   ENABLE();
}

#endif




