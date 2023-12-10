/*
 *
 *    Flags de compilation globaux
 *
 */

/* sam: hum, ne serait-il pas plus judicieux d'appeller ce 
fichier config.h vu qu'il sert a definir la configuration
de compilation ? */

/*
 *
 */

#ifndef FLAGS_H
#define FLAGS_H

/*
sam: Définir ceci si on veut une estimation de vitesse
*/
#define EVAL_SPEED

/*
sam: Définir ceci si on ne veut pas du debugger
*/
#ifdef FAST
#define NO_DEBUGGER
#endif

/*
Définir ceci si on ne veut pas du gestionnaire de memoire 
dans le debugger (marche pas sous amiga)
*/
#define NO_MEMORY_MANAGER

/*
sam: Définir ceci si on veut utiliser les codes ansi "ESC["
pour positionner le curseur dans debug.c (amiga, unix/xterm, ...)
*/
#define ANSI_ESC

/*
sam: Définir ceci si on ne veut pas de la decompression
automatique des fichiers en lecture
#define NO_ZFILE
*/

/*
Définir ceci si on utilise allegro
#define USE_ALLEGRO
*/

/*
sam: Définir ceci si on ne veut pas traiter l'attente d'IRQ (CWAIT).
Ca donne un code plus rapide (3% sur amiga apparemment)
*/
#define NO_WAIT

/*
sam: Définir cela pour retirer l'emul du code de protection
(plus rapide sans)
*/
#ifdef FAST
#define NO_PROTECT
#endif

/*
sam: Définir ceci pour eviter l'emul des instructions BCD (servent a
rien et ralentissent l'evaluation des flags)
*/
#ifdef FAST
#define NO_BCD
#endif

/*
sam: Définir ceci pour utiliser un code de lecture souris plus
efficace (lecture dans la structure Screen directement).
Pas franchement interressant pour les amiga rapides
*/
#ifdef FAST
#define FAST_MOUSE
#endif

/*
sam: Définir ceci pour eviter les shift dans l'acces a la mémoire
     (interressant pour 68040+)
#define BIG_MAPPER
*/

/* 
sam: Définir REGPARM pour utiliser le passage de parametre
par des registres. (depend du compilo)
*/
#if defined(AMIGA) && defined(__GNUC__) && !defined(__PROFILE__)
#define REGPARM __attribute__((regparm(2)))
#endif

/* sam: Utiliser ceci pour assigner un registre fixe
pour les viables globales suivantes. 

Ceci pourrait etre utile pour d'autres CPUs avec pleins de
registres (PPC, ..)
#define REG_M  "d7"
#define REG_m  "d6"
#define REG_PC "a4"
*/
#define REG_M  "d7"
#define REG_m  "d6"
#define REG_PC "d5"
#define REG_cl "a3"
#ifndef SMALL_MODEL
#define REG_MAPPER "a4"
#endif

//#define REG_PC "d5"
//#define REG_MAPPER "a4"

/***************************************************************
sam: Les variables globales assignees a des registres doivent 
etre definies avant les fonctions, donc dans flags.h
***************************************************************/
#if defined(__GNUC__)
#define GLOB_REG(T,x) \
  register T x asm(REG_##x); \
  asm("reg_"#x"_"REG_##x":");
#ifdef REG_m
GLOB_REG(unsigned int, m)
#endif
#ifdef REG_M
GLOB_REG(unsigned int, M)
#endif
#ifdef REG_PC
GLOB_REG(unsigned int, PC)
#endif
#ifdef REG_X
GLOB_REG(unsigned int, X)
#endif
#ifdef REG_cl
GLOB_REG(unsigned int, cl)
#endif
#ifdef REG_MAPPER
GLOB_REG(unsigned char **,MAPPER)
#endif
#else
#undef REG_m
#undef REG_M
#undef REG_PC
#undef REG_X
#undef REG_cl
#undef REG_MAPPER
#endif

/* sam: pour garder la coherence */
#ifndef REGPARM
#define REGPARM
#endif

/* bon ya eu un probleme: sortie imprevue */
extern void crash(void);
/* sortie normale */
extern void normal_end(void);

#endif /* FLAGS_H */
