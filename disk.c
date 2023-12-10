/*	  

	VERSION     : 2.0
        MODULE      : disk.c
	Créé par    : Gilles FETIS
        Modifié par : Alexandre PUKALL Mai 1998

        Gestion du format SAP 1.0 lecture et ecriture disquette

        Il ne reste plus qu'a autoriser l'ecriture dans le fichier
        SAP sur le disque dur et non plus en memoire.
        Il suffit d'ouvrir le fichier SAP a chaque ecriture de secteur,
        de faire un fseek pour le positionner, d'ecrire les donnees et
        de refermer le fichier ( pour eviter des problemes en cas d'arret
        brutal )

*/

#include "flags.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef USE_ALLEGRO
#include <allegro.h>
#endif
#include "regs.h"
#include "mc6809.h"
#include "mapper.h"
#include "graph.h" /* sam: ou sinon pb car fdraw() non declare (cf REGPARM) */

#include "diskio.h"

#include "zfile.h"

#define NBSIDE 2
#define NBTRACK 80
#define NBSECT 16
#define SECTSIZE 256

void code(void); /* sam: ou sinon usage avant declaration dans assemble() */

/*

Attention!!! : il faut supprimer ces variables globales
seuls certains objets precis peuvent le rester a condition
d'etre declarés comme des structures

*/
unsigned short ax,bx,cx,dx,si,tmp,x1a2,x1a0[5],res1,i2,inter,cfc,cfd,compte,fois;
unsigned char cle[11],entree;
unsigned char bufferc[256];
int DRV,TRK,SEC,BUF;

int SAPOK=0;
char SAPfname[2][1024];
int erreur=0;
int ret=0;

/*

*/

typedef struct status_t
{
    unsigned char format;
    unsigned char protection;
    unsigned char piste;
    unsigned char secteur;
    unsigned char crc1sect;
    unsigned char crc2sect;
} status_t;

/* sam: disk[][][] pese ~800Kb, ca genera pour les petites machines
   avec moins de 2Mo ou si la ram est beaucoup fragmentee.. 
   (dans le fond, je m'en fiche: j'ai 8Mo et VMM fait un tres bon
   defragmenteur :-) */
typedef unsigned char s_disk[NBSIDE][NBTRACK][NBSECT][SECTSIZE];
typedef status_t    s_status[NBSIDE][NBTRACK][NBSECT];

#ifdef SMALL_MODEL
static s_disk   *disk_;
static s_status *status_;
#define disk   (*disk_)
#define status (*status_)
static void init_disk_status(void) {
    if(!disk_)   disk_   = calloc(sizeof(*disk_),1);
    if(!status_) status_ = calloc(sizeof(*status_),1);
    if(!disk || !status) {
	fprintf(stderr, "Can't alloc disk memory.\n");
	crash();
    }
}
#else
#ifndef __SASC
#define __far
#endif
static __far s_disk disk;
static __far s_status status;
static void init_disk_status(void) {
}
#endif

unsigned short crcpuk_temp;

/*
  appeler 'table' une table n'est pas une bonne idee il faut preciser
  à quoi sert cette table
  */

static const unsigned short table[]={
    
    0x0000, 0x1081, 0x2102, 0x3183,
    0x4204, 0x5285, 0x6306, 0x7387,
    0x8408, 0x9489, 0xa50a, 0xb58b,
    0xc60c, 0xd68d, 0xe70e, 0xf78f
	
    };

int decode_pc1(void);

void crc_pukall(short c)
{
    register short index;
    
    index = (crcpuk_temp ^ c) & 0xf;
    crcpuk_temp = ((crcpuk_temp>>4) & 0xfff) ^ table[index];
    
    c >>= 4;
    
    index = (crcpuk_temp ^ c) & 0xf;
    crcpuk_temp = ((crcpuk_temp>>4) & 0xfff) ^ table[index];
}

int init_pc1(unsigned char drive1,unsigned char track1,unsigned char sect1)
{
    unsigned int charge;
    
    init_disk_status();
    
    cle[0]=0x42 ^ (SEC-1);
    cle[1]=0x2A ^ TRK;
    cle[2]=0x94 ^ status[drive1][track1][sect1].crc1sect;
    cle[3]=0xC4 ^ status[drive1][track1][sect1].crc2sect;
    cle[4]=0x09;
    cle[5]=0x29;
    cle[6]=0x13;
    cle[7]=0x15;
    cle[8]=0xA9;
    cle[9]=0x22;
    
    decode_pc1();
    
    if ( LOAD8(0x604A)<=0x3F )
    {
	entree=crcpuk_temp>>8;
	if ( entree != status[drive1][track1][sect1].crc1sect )
	{
	    return(1);
	}
	
	entree=crcpuk_temp&255;
	if ( entree != status[drive1][track1][sect1].crc2sect )
	{
	    return(1);
	}
	return(0);
    }
    
    for (fois=0;fois<256;fois++)
    {
	bufferc[fois]=disk[DRV][TRK][SEC-1][fois];
    }
    
    charge   = LOAD8(0x604A);
    charge <<= 8;
    
    cle[0]=LOAD8(charge) ^ (SEC-1);
    cle[1]=LOAD8(charge+1) ^ TRK;
    cle[2]=LOAD8(charge+2) ^ status[drive1][track1][sect1].crc1sect;
    cle[3]=LOAD8(charge+3) ^ status[drive1][track1][sect1].crc2sect;
    cle[4]=LOAD8(charge+4);
    cle[5]=LOAD8(charge+5);
    cle[6]=LOAD8(charge+6);
    cle[7]=LOAD8(charge+7);
    cle[8]=LOAD8(charge+8);
    cle[9]=LOAD8(charge+9);
    
    decode_pc1();
    
    entree=crcpuk_temp>>8;
    if ( entree != status[drive1][track1][sect1].crc1sect )
    {
	return(1);
    }
    
    entree=crcpuk_temp&255;
    if ( entree != status[drive1][track1][sect1].crc2sect )
    {
	return(1);
    }
    return(0);
    
}

int verify_sap_ecri(unsigned char drive,unsigned char track,unsigned char sect)
{
    init_disk_status();
    
    if ( (status[drive][track][sect].protection==1) || (status[drive][track][sect].protection==3) )
    {
	erreur=1;
	return(1);
    }
    
    if (status[drive][track][sect].format==5)
    {
	erreur=4;
	return(1);
    }
    
    crcpuk_temp = 0xffff;
    
    crc_pukall(status[drive][track][sect].format);
    crc_pukall(status[drive][track][sect].protection);
    crc_pukall(status[drive][track][sect].piste);
    crc_pukall(status[drive][track][sect].secteur);
    
    for (fois=0;fois<=255;fois++)
    {
	crc_pukall( LOAD8((BUF+fois)%0xFFFF) );
	//       crc_pukall( LOAD8((BUF+fois)&0xFFFF) );
    }
    
    entree=crcpuk_temp>>8;
    status[drive][track][sect].crc1sect=entree;
    
    entree=crcpuk_temp&255;
    status[drive][track][sect].crc2sect=entree;
    
    erreur=0;
    return(0);
    
}

int verify_sap_lect(unsigned char drive,unsigned char track,unsigned char sect)
{
    init_disk_status();
    
    if ( status[drive][track][sect].format==5)
    {
	for (fois=0;fois<256;fois++)
        {
	    bufferc[fois]=disk[drive][track][sect][fois];
        }
	erreur=init_pc1(drive,track,sect);
	if (erreur==0)
        {
	    for (fois=0;fois<256;fois++)
	    {
		STORE8(((BUF+fois)%0xFFFF),bufferc[fois]);
	    }
	    
	    erreur=0;
	    return(0);
        }
	else
        {
	    erreur=4;
	    return(1);
        }
    }
    
    erreur=0;
    return(0);
    
}


int verify_sap(unsigned char drive,unsigned char track,unsigned char sect)
{
    init_disk_status();
    
    crcpuk_temp = 0xffff;
    
    crc_pukall(status[drive][track][sect].format);
    crc_pukall(status[drive][track][sect].protection);
    crc_pukall(status[drive][track][sect].piste);
    crc_pukall(status[drive][track][sect].secteur);
    
    if ( (track != status[drive][track][sect].piste) || ((sect+1) != status[drive][track][sect].secteur) )
    {
	erreur=4;
	return(1);
    }
    
    for (fois=0;fois<=255;fois++)
    {
	crc_pukall(disk[drive][track][sect][fois]);
    }
    
    entree=crcpuk_temp>>8;
    if ( entree != status[drive][track][sect].crc1sect )
    {
	erreur=8;
	return(1);
    }
    
    entree=crcpuk_temp&255;
    if ( entree != status[drive][track][sect].crc2sect )
    {
	erreur=8;
	return(1);
    }
    
    if ( status[drive][track][sect].format==4)
    {
	erreur=4;
	return(1);
    }
    
    erreur=0;
    return(0);
}



void LoadTDS(char * name,int side)
{
    FILE * f;
    char descript[0xA0];
    
    int i,j;
    
    init_disk_status();
    
    /* lecture du descripteur TDS */
    f=fopen(name,"rb");
    if (f==NULL) return;
    
    fread(descript,sizeof(char),(size_t)0xA0,f);
    
    for (i=0;i<0xA0;i++)
	for (j=0;j<8;j++)
	{
	    if (   ( descript[i]&(1<<j) )!=0   )
	    {
		int SEC,TRK;
		SEC=(i%2)*8+j;
		TRK=i/2;
		
		fread(disk[side][TRK][SEC],sizeof(char),(size_t)SECTSIZE,f);
	    }
	}
    
    fclose(f);
    
    /* les extensions SAP sont invalidées pour ce disk */
    SAPOK=0;
}

/*
  ecriture au format TDS
  */
void SaveTDS(char * name,int side)
{
    FILE * f;
    char descript[0xA0];
    
    int i,j;
    
    init_disk_status();
    
    /* lecture du descripteur TDS */
    f=fopen(name,"wb");
    if (f==NULL) return;
    
    /* construction du header */
    for(i=0;i<0xA0;i++)
    {
	descript[i]=0;
	for(j=0;j<8;j++)
	{
	    if (disk[side][20][1+(i*8)/SECTSIZE][(i*8)%SECTSIZE]!=0xFF)
		descript[i]|=(1<<j);
	}
    }
    fwrite(descript,sizeof(char),(size_t)0xA0,f);
    
    
    for (i=0;i<0xA0;i++)
	for (j=0;j<8;j++)
	{
	    if (   ( descript[i]&(1<<j) )!=0   )
	    {
		int SEC,TRK;
		SEC=(i%2)*8+j;
		TRK=i/2;
		
		fwrite(disk[side][TRK][SEC],sizeof(char),(size_t)SECTSIZE,f);
	    }
	}
    
    fclose(f);
}

#define SAP_HEADER_SIZE 66
#define SAP_SECT_SIZE 262

void LoadSAP(char * name,int side)
{
    FILE * f;
    
    static unsigned char buff[4096];
    int i,j,k;
    
    init_disk_status();
    
    f=fopen(name,"rb");
    if (f==NULL) return;
    
    /* mémorise le nom de l'archive pour uilisation ultérieure */
    strcpy(SAPfname[side],name);
    
    fread(buff,sizeof(char),(size_t)SAP_HEADER_SIZE,f);
    
    for (i=0;i<80;i++)
	for (j=0;j<16;j++)
	{
	    int SEC,TRK;
	    SEC=j;
	    TRK=i;
	    fread(buff,sizeof(char),(size_t)SAP_SECT_SIZE,f);
	    status[side][TRK][SEC].format=buff[0];
	    status[side][TRK][SEC].protection=buff[1];
	    status[side][TRK][SEC].piste=buff[2];
	    status[side][TRK][SEC].secteur=buff[3];
	    
	    for (k=0;k<SECTSIZE;k++)
		disk[side][TRK][SEC][k]=(buff[4+k]^0xB3);
	    status[side][TRK][SEC].crc1sect=buff[4+k];
	    status[side][TRK][SEC].crc2sect=buff[4+k+1];
	}
    fclose (f);
    
    /* les extensions SAP sont activées sur ce disk */
    SAPOK=1;
}

void InitDiskCtrl(void)
{
    getcc();
    CC&=0xFE;
    setcc(CC);
    STORE8(0x604E,'D');
    return;
}

void ReadSector(void)
{
    int i;
    
    init_disk_status();
    
    DRV=LOAD8(0x6049);
    TRK=(LOAD8(0x604A)<<8)|LOAD8(0x604B);
    SEC=LOAD8(0x604C);
    
    BUF=(LOAD8(0x604F)<<8)|LOAD8(0x6050);
    
    if ((DRV>1) || (TRK>=NBTRACK) || (SEC>NBSECT))
    {
	getcc();
	CC|=0x01;
	setcc(CC);
	STORE8(0x604E,0x10);
	return;
    }
    
    if (SAPOK==0)
	for (i=0;i<256;i++) 
	    STORE8(((BUF+i) & 0xFFFF),disk[DRV][TRK][SEC-1][i]);
    else
    {
	ret=verify_sap(DRV,TRK,SEC-1);
	if (ret==0)
	{
	    ret=verify_sap_lect(DRV,TRK,SEC-1);
	    if (ret==0)
		for (i=0;i<256;i++) 
		    STORE8(((BUF+i) & 0xFFFF),disk[DRV][TRK][SEC-1][i]);
	}
    }
    
    if (ret==0)
    {
	STORE8(0x604E,0x00);
	getcc();
	CC&=0xFE;
	setcc(CC);
    }
    else
    {
	STORE8(0x604E,erreur);
	getcc();
	CC=CC|0x1;
	setcc(CC);
    }
}

void WriteSector(void)
{
    int i;
    
    init_disk_status();
    
    DRV=LOAD8(0x6049);
    TRK=(LOAD8(0x604A)<<8)|LOAD8(0x604B);
    SEC=LOAD8(0x604C);
    
    BUF=(LOAD8(0x604F)<<8)|LOAD8(0x6050);
    
    if ((DRV>1) || (TRK>=NBTRACK) || (SEC>NBSECT))
    {
	getcc();
	CC|=0x01;
	setcc(CC);
	STORE8(0x604E,0x10);
	return;
    }
    
    if (SAPOK==0)
	for (i=0;i<256;i++) disk[DRV][TRK][SEC-1][i]=LOAD8((BUF+i)%0xFFFF);
    else
    {
	ret=verify_sap(DRV,TRK,SEC-1);
	if (ret==0)
	{
	    ret=verify_sap_ecri(DRV,TRK,SEC-1);
	    if (ret==0)
	    {
#if 0 /* sam: heu pourquoi open() ici plutot que fopen() ? */
		int fSAP;
		for (i=0;i<256;i++) disk[DRV][TRK][SEC-1][i]=LOAD8((BUF+i)%0xFFFF);
		fSAP=open(SAPfname[DRV],O_RDWR | O_BINARY);
		if (fSAP>0)
		{
		    int i;
		    unsigned char temp[SECTSIZE+2];
		    size_t posInSAP;
		    posInSAP= SAP_HEADER_SIZE + (TRK*NBSECT+(SEC-1))*SAP_SECT_SIZE + 4;
		    lseek(fSAP,posInSAP,SEEK_SET);
		    for (i=0;i<SECTSIZE;i++) temp[i]=disk[DRV][TRK][SEC-1][i]^0xB3;
		    temp[i]=status[DRV][TRK][SEC-1].crc1sect;
		    temp[i+1]=status[DRV][TRK][SEC-1].crc2sect;
		    write(fSAP,temp,SECTSIZE+2);
		    close(fSAP);
		}
#else /* sam:ici au moins avec fopen() ca marche en conjonction de zfile.h */
		FILE *fSAP;
		for (i=0;i<256;i++) disk[DRV][TRK][SEC-1][i]=LOAD8((BUF+i)%0xFFFF);
		fSAP=fopen(SAPfname[DRV],"rb+");
		if (fSAP)
		{
		    int i;
		    unsigned char temp[SECTSIZE+2];
		    size_t posInSAP;
		    posInSAP= SAP_HEADER_SIZE + (TRK*NBSECT+(SEC-1))*SAP_SECT_SIZE + 4;
		    fseek(fSAP,posInSAP,SEEK_SET);
		    for (i=0;i<SECTSIZE;i++) temp[i]=disk[DRV][TRK][SEC-1][i]^0xB3;
		    temp[i]=status[DRV][TRK][SEC-1].crc1sect;
		    temp[i+1]=status[DRV][TRK][SEC-1].crc2sect;
		    fwrite(temp,SECTSIZE+2,1,fSAP);
		    fclose(fSAP);
		}
#endif
	    }
	}
    }
    
    if (ret==0)
    {
	STORE8(0x604E,0x00);
	getcc();CC&=0xFE;setcc(CC);
    }
    else
    {
	STORE8(0x604E,erreur);
	getcc();CC|=0x01;setcc(CC);
    }
}


void assemble(void)
{
    
    x1a0[0]= ( cle[0]*256 )+ cle[1];
    code();
    inter=res1;
    
    x1a0[1]= x1a0[0] ^ ( (cle[2]*256) + cle[3] );
    code();
    inter=inter^res1;
    
    x1a0[2]= x1a0[1] ^ ( (cle[4]*256) + cle[5] );
    code();
    inter=inter^res1;
    
    x1a0[3]= x1a0[2] ^ ( (cle[6]*256) + cle[7] );
    code();
    inter=inter^res1;
    
    x1a0[4]= x1a0[3] ^ ( (cle[8]*256) + cle[9] );
    code();
    inter=inter^res1;
    i2=0;
}

void code(void)
{
    dx=x1a2+i2;
    ax=x1a0[i2];
    cx=0x015a;
    bx=0x4e35;
    
    tmp=ax;
    ax=si;
    si=tmp;
    
    tmp=ax;
    ax=dx;
    dx=tmp;
    
    if (ax!=0)
    {
	ax=ax*bx;
    }
    
    tmp=ax;
    ax=cx;
    cx=tmp;
    
    if (ax!=0)
    {
	ax=ax*si;
	cx=ax+cx;
    }
    
    tmp=ax;
    ax=si;
    si=tmp;
    ax=ax*bx;
    dx=cx+dx;
    
    ax=ax+1;
    
    x1a2=dx;
    x1a0[i2]=ax;
    
    res1=ax^dx;
    i2=i2+1;
}


int decode_pc1(void)
{
    short c1;
    si=0;
    x1a2=0;
    i2=0;
    
    crcpuk_temp = 0xffff;
    
    for (fois=0;fois<=255;fois++)
    {
	c1=bufferc[fois];
	assemble(); 
	cfc=inter>>8;
	cfd=inter&255;  
	
	c1 = c1 ^ (cfc^cfd);
	
	for (compte=0;compte<=9;compte++)
	{
	    cle[compte]=cle[compte]^c1;
	}
	bufferc[fois]=c1;
	crc_pukall(bufferc[fois]);
    }
    return 0; /* sam: fonction declaree retournant int  */
}
