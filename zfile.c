 /*
  * zfile.c
  *
  * Routines pour manipuler les fichiers compressés de facon transparente
  * en lecture. (code utilisé aussi dans UAE, l'emulateur amiga de Bernd
  * Schmidt)
  *
  * Créé par Samuel Devulder.
  */

#include "flags.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ZFILE_C /* pour eviter de masquer fopen/fclose */
#include "zfile.h"

#ifndef NO_ZFILE

struct zfile
{
    struct zfile *next;
    FILE *f;
    char name[L_tmpnam];
};
 
static struct zfile *zlist = 0;
static char atexit_called = 0;

/*
 * gzip decompression
 */
static int gunzip (const char *decompress, const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst) 
	return 1;
    sprintf (cmd, "%s -c -d %s >%s", decompress, src, dst);
    return !system (cmd);
}

/*
 * lha decompression
 */
static int lha (const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst)
	return 1;
#if defined(AMIGA)
    sprintf (cmd, "lha -q -N p %s >%s", src, dst);
#else
    sprintf (cmd, "lha pq %s >%s", src, dst);
#endif
    return !system (cmd);
}

/*
 * (pk)unzip decompression
 */
static int unzip (const char *src, const char *dst)
{
    char cmd[1024];
    if (!dst)
	return 1;
#if defined AMIGA || defined __unix
    sprintf (cmd, "unzip -p %s >%s", src, dst);
#elif defined MSDOS /* sam: il faudrait verifier les arguments en lignes de commande pour pkunzip */
    sprintf (cmd, "pkunzip -p %s >%s", src, dst);
#else
    return 0;
#endif
    return !system (cmd);
}

#ifdef AMIGA
/*
 * Check for direct reading
 */
static int chk_dfx(const char *src, const char *dst)
{
    char cmd[1024];
    int unit, side;

    if(sscanf(src,"df%d:%d", &unit, &side) != 2) 
    if(sscanf(src,"/df%d/%d",&unit, &side) != 2) { 
       side = 0;
       if(sscanf(src,"df%d:", &unit) != 1) 
       if(sscanf(src,"/df%d", &unit) != 1) return 0;
    }
    if (!dst) return 1;
    sprintf(cmd, "todisk DSK u%ds%d TOSAP %s", unit, side, dst);
    if(system (cmd)) return 0;
    sprintf(cmd, "%s_f%d.sap", dst, side);
    rename(cmd, dst);
    return 1;
}
#endif

/*
 * decompresses the file (or just check if dest is null)
 */
static int uncompress (const char *name, char *dest)
{
    char *ext = strrchr (name, '.');
    char nam[1024];

    /* si le fichier a deja une extension: */
    if (ext != NULL && access (name, 0) >= 0) {
	ext++;
	if (stricmp (ext, "z") == 0
	    || stricmp (ext, "gz") == 0)
	    return gunzip ("gzip", name, dest);
	if (stricmp (ext, "bz") == 0)
	    return gunzip ("bzip", name, dest);
	if (stricmp (ext, "bz2") == 0)
	    return gunzip ("bzip2", name, dest);
	if (stricmp (ext, "lha") == 0
	    || stricmp (ext, "lzh") == 0)
	    return lha (name, dest);
	if (stricmp (ext, "zip") == 0)
	     return unzip (name, dest);
    }
#ifdef AMIGA
    if(chk_dfx(name, dest)) return 1;
#endif    

    /* sinon: on essaye en ajoutant une extension */
    if (access (strcat (strcpy (nam, name), ".z"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".Z"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".gz"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".GZ"), 0) >= 0)
	return gunzip ("gzip", nam, dest);

    if (access (strcat (strcpy (nam, name), ".bz"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".BZ"), 0) >= 0)
	return gunzip ("bzip", nam, dest);

    if (access (strcat (strcpy (nam, name), ".bz2"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".BZ2"), 0) >= 0)
	return gunzip ("bzip2", nam, dest);

    if (access (strcat (strcpy (nam, name), ".lha"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".LHA"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".lzh"), 0) >= 0
	|| access (strcat (strcpy (nam, name), ".LZH"), 0) >= 0)
	return lha (nam, dest);

    if (access (strcat (strcpy (nam, name),".zip"),0) >= 0
	|| access (strcat (strcpy (nam, name),".ZIP"),0) >= 0)
       return unzip (nam, dest);

    return 0;
}

/*
 * called on exit() to cleanup temporary files
 */
static void zfile_exit (void)
{
    struct zfile *l;
    
    while ((l = zlist)) {
	zlist = l->next;
	fclose (l->f);
	unlink (l->name); /* sam: in case unlink() after fopen() fails */
	free (l);
    }
}

/*
 * verifie si c'est un mode en ecriture
 */
static int check_write(const char *mode)
{
  while(*mode) if(*mode=='w' || *mode=='+' || *mode=='a')  return 1; 
	       else ++mode;
  return 0;
}

/*
 * fopen() for a compressed file
 */
FILE *zfile_open (const char *name, const char *mode)
{
    struct zfile *l;

    /* si le fichier n'est pas compresse ou si on y accede en ecriture,
       on utilise le fopen stdio normal */
    if (! uncompress (name, NULL)) return fopen (name, mode);
    if (check_write(mode)) {
	fprintf(stderr,"Tentative d'ecriture sur le fichier compresse: \"%s\"\n",name);
	return NULL;
    }

    l = malloc(sizeof(*l)); if (!l) return NULL;
    
    if(!atexit_called) {atexit(zfile_exit);atexit_called = 1;}
 
    /* creation du fichier temporaire qui va recevoir le fichier decompresse */
    tmpnam (l->name);

    printf("Uncompressing \"%s\"...",name);fflush(stdout);
    /* decompression */
    if (! uncompress (name, l->name)) goto erreur;
    printf("\r                                                             \r");
    fflush(stdout);

    /* ouverture du fichier decompresse */
    if( ! (l->f = fopen (l->name, mode))) goto erreur;

    /* ajout a la liste */
    l->next = zlist;
    zlist = l;

    return l->f;
erreur:
    unlink (l->name);
    free (l);
    return NULL;
}

/*
 * fclose() but for a compressed file
 */
int zfile_close (FILE *f)
{
    struct zfile *pl = NULL;
    struct zfile *l  = zlist;
    int ret;

    while (l && l->f!=f) {
	pl = l;
	l = l->next;
    }
    if (l == 0) return fclose (f);

    ret = fclose (l->f);
    unlink (l->name);

    if(!pl)
	zlist = l->next;
    else
	pl->next = l->next;
    free (l);

    return ret;
}
#endif
