/*
 * zfile.h
 * 
 * Protoypes pour zfile.c - acces aux fichiers compresses. 
 * desactivation en definissant NO_ZFILE
 *
 * Codé par Samuel Devulder
 */

#ifndef ZFILE_H
#define ZFILE_H
#ifndef NO_ZFILE

int zfile_close(FILE *);
FILE *zfile_open(const char *name, const char *mode);

#ifndef ZFILE_C /* == kernel */

/* ici on enrobe les routines de stdio pour utiliser 
   zfile a la place */
#define fopen  zfile_open
#define fclose zfile_close

#endif /* ZFILE_C */
#endif /* NO_ZFILE */
#endif /* ZFILE_H */
