/*

	TO8

	Par Gilles FETIS

	mail : FETIS_Gilles@compuserve.com
	page : http://ourworld.compuserve.com/homepages/FETIS_Gilles


*/


/*

	VERSION     : 0.4
	MODULE      : mapper.c
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	Module d'émulation du hardware & de la carte mémoire

*/

#include "flags.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef DJGPP
#include <dpmi.h>
#endif
#ifdef USE_ALLEGRO
#include <allegro.h>
#endif
#include "keyb.h"
#include "sound.h"
#include "mapper.h"
#include "graph.h"
#include "regs.h"


/* tableau des 64ko de memoire adresses par le MC6809 */
#ifndef REG_MAPPER
BYTE8 *MAPPER[MAPPER_SIZE];
#else
static int LMEM[256+MAPPER_SIZE];
#define _MAPPER ((BYTE8**)&LMEM[256])
//static BYTE8 *_MAPPER[16];
#endif

/* flag - cette page de RAM est elle de la VRAM */
/*int*/
BYTE8 IS_VRAM[16];

/* 512ko de RAM */
BYTE8 *RAM[32];

/* une banque non accessible en écriture pour la compatibilité */
BYTE8 * EXT;

/* drapeau d'accés en ecriture à la fenetre RAM2 */
int BankW=-1;

/* 64ko de ROM */
BYTE8 * ROM[4];


/* acces hard */
BYTE8 * HARD;

/* 2x4ko de ROM moniteur */
BYTE8 * MON[2];
BYTE8 *MON0_7E7, *MON1_7E7;


/* drapeau d'attente sur interrupt */
int wait=0;

/* variable de comptage de cycles processeur */
#ifndef REG_cl
unsigned int cl;
#endif

int baROM=0;
int baMON=0;
int baRAM=0;


BYTE port0;

/* Registres du PIA manettes */
BYTE ORA2,ORB2,DDRA2,DDRB2,CRA2,CRB2;


/* registres du 6846 */
BYTE CRC,PRC;

/* Registres du 6821 */
BYTE ORA,ORB,DDRA,DDRB,CRA,CRB;

/* Le Gate Array Mémoire */
BYTE GE7E5,GE7E6,GE7E7;

/* Le Gate Array Video */
BYTE GE7DA,GE7DB,GE7DC,GE7DD;

static int palette[32];

void REGPARM map_none(unsigned int ADR, BYTE OP);
void REGPARM map_ram(unsigned int ADR, BYTE OP);
void REGPARM map_rom(unsigned int ADR, BYTE OP);
void REGPARM hardware(unsigned int ADR,BYTE OP);
int  REGPARM ld_ram(unsigned int ADR);
int  REGPARM ld_hwd(unsigned int ADR);

#define mr(X) void REGPARM map_ram##X(unsigned int ADR, BYTE OP)  {GETMAP2(X,ADR)=OP;}
mr(6); mr(7); mr(8); mr(9); 
mr(10); mr(11); mr(12); mr(13);

store_funcs store8tab[16] = 
{
#if 0 /* sam: orig code 10/01/2000 */
map_none, map_none, map_none, map_none,
fdraw2, fdraw2,                           
map_ram, map_ram, map_ram, map_ram, 
map_ram, map_ram, map_ram, map_ram, 
hardware, map_none
#else
map_none, map_none, map_none, map_none,
fdraw2, fdraw2,                           
map_ram6, map_ram7, map_ram8, map_ram9, 
map_ram10, map_ram11, map_ram12, map_ram13, 
hardware, map_none
#endif
};
/*
store_funcs *store8tab=_store8tab;
*/

load_funcs load8tab[16] = 
{
ld_ram, ld_ram, ld_ram, ld_ram,
ld_ram, ld_ram,
ld_ram, ld_ram, ld_ram, ld_ram,
ld_ram, ld_ram, ld_ram, ld_ram,
ld_hwd, ld_ram
};
/*
load_funcs *load8tab=_load8tab;
*/

void REGPARM hardware(unsigned int ADR,BYTE OP)
{
    OP &= 255; /* ca evite des catastrophes */
    if((short)ADR==(short)0xe7c3) goto do_e7c3;
    switch(ADR)
    {
        case 0xE7C1:  /* gestion du CRC 6846 systeme */
	CRC=OP;
	if (!(CRC&2) && !(PRC&0x20)) /* request clavier */
	{
	    if (!(PRC&8) == !(kb_state&KB_CAPSLOCK_FLAG))
	    {
		kb_state^=KB_CAPSLOCK_FLAG;
		set_leds(kb_state);
	    }
	    port0|=2;            /* output clavier a 1 */
	    MON[0][0x7C0]=MON[1][0x7C0]=port0;
	}
	
	if (thomsound && ((MON[0][0x7C1] & 0x08)!=(CRC & 0x08)))
#ifdef AMIGA
	    output_sound((CRC&0x08)?0x7F:0x00);
#else
	    BSTORE[((cl*(FREQ/50))/20000) % soundbufsize]=((CRC&0x08)<<5) | 0x8000;
#endif
	
	MON[0][0x7C1]=MON[1][0x7C1]=CRC;
	break;
	
        case 0xE7C3:  /* gestion du mapper Forme/Fond */
do_e7c3:
	if ((OP&0x01)==0x00)
	{
	    SETMAP(0x4000, &RAM[0][0x0000]);
	    SETMAP(0x5000, &RAM[0][0x1000]);
	}
	else
	{
	    SETMAP(0x4000, &RAM[0][0x2000]);
	    SETMAP(0x5000, &RAM[0][0x3000]);
	}
	if(!(((char)OP^(char)MON[0][0x7C3])&(char)~1)) goto end_e7c3; /* sam: */
	
	if (OP&0x10)  /* swap rom moniteur */
	{
	    SETMAP(0xE000, &MON[1][0x0000]);
	    SETMAP(0xF000, &MON[1][0x1000]);
	    baMON=1;
	}
	else
	{
	    SETMAP(0xE000, &MON[0][0x0000]);
	    SETMAP(0xF000, &MON[0][0x1000]);
	    baMON=0;
	}
	
	if (OP&4)  /* selection cartouche / rom interne */
	{
	    int MEMO_PRC;
	    MEMO_PRC=PRC;
	    PRC&=0x04;
	    /* reset ROM mapping values */
	    hardware(0xE7E6,GE7E6);
	    PRC=MEMO_PRC;
	}
	else
	{
	    SETMAP(0x0000, &EXT[0x0000]);
	    SETMAP(0x1000, &EXT[0x1000]);
	    SETMAP(0x2000, &EXT[0x2000]);
	    SETMAP(0x3000, &EXT[0x3000]);
	}
	
	if (OP&0x20)  /* bit 5 ack. clavier a 1 */
	{
	    OP&=0xF7;  /* bit 3 LED clavier non modifiable */
	    if (!(kb_state&KB_CAPSLOCK_FLAG))
		OP|=8;
	    
	    if (!(PRC&0x20))
	    {
		port0&=0xFD;  /* la remontee a 1 force le bit output
				 clavier a 0 */
		MON[0][0x7C0]=MON[1][0x7C0]=port0;
	    }
	}   
end_e7c3:	
	PRC=(PRC&0x82) | (OP&0x7D);
	MON[0][0x7C3]=MON[1][0x7C3]=PRC;
	break;
	
        /* PIA systeme */
        case 0xE7C8:  /* acces ORA ou DDRA */
	if (CRA&4)
	{
	    ORA=(ORA&(DDRA^0xFF))|(OP&DDRA);
	    MON[0][0x7C8]=MON[1][0x7C8]=ORA;
	}
	else
	{
	    DDRA=OP;
	    MON[0][0x7C8]=MON[1][0x7C8]=DDRA;
	}
	break;
	
        case 0xE7C9:  /* acces ORB ou DDRB */
	if (CRB&4)
	{
	    ORB=(ORB&(DDRB^0xFF))|(OP&DDRB);
	    MON[0][0x7C9]=MON[1][0x7C9]=ORB;
	}
	else
	{
	    DDRB=OP;
	    MON[0][0x7C9]=MON[1][0x7C9]=DDRB;
	    
	    /* commute RAM mode PIA */
	    if (!(GE7E7&0x10))
	    {
		int tmp=DDRB&0xF8;
		
		baRAM=2;
		if (tmp==0x08) baRAM=2;
		if (tmp==0x10) baRAM=3;
		if (tmp==0xE0) baRAM=4;
		if (tmp==0xA0) baRAM=5;
		if (tmp==0x60) baRAM=6;
		if (tmp==0x20) baRAM=7;
		
 	        SETMAP(0xA000, &RAM[baRAM][0x0000]);
 	        SETMAP(0xB000, &RAM[baRAM][0x1000]);
 	        SETMAP(0xC000, &RAM[baRAM][0x2000]);
 	        SETMAP(0xD000, &RAM[baRAM][0x3000]);
		BankW=-1;
	    }
	}
	break;
	
        case 0xE7CA:  /* accŠs … CRA */
	CRA=(CRA&0xC0)|(OP&0x3F);
	MON[0][0x7CA]=MON[1][0x7CA]=CRA;
	break;
	
        case 0xE7CB:  /* accŠs … CRB */
	CRB=(CRB&0xC0)|(OP&0x3F);
	MON[0][0x7CB]=MON[1][0x7CB]=CRB;
	break;

        /* PIA manettes */
        case 0xE7CC:  /* acces ORA2 ou DDRA2 */
	if (CRA2&4)
	{
	    ORA2=(ORA2&(DDRA2^0xFF))+(OP&DDRA2);
	    MON[0][0x7CC]=MON[1][0x7CC]=ORA2;
	}
	else
	{
	    DDRA2=OP;
	    MON[0][0x7CC]=MON[1][0x7CC]=DDRA2;
	}
	break;
	
        case 0xE7CD:  /* acces ORB2 ou DDRB2 */
	if (CRB2&4)
	{
	    ORB2=(ORB2&(DDRB2^0xFF))+(OP&DDRB2);
	    if (thomsound)
#ifdef AMIGA
		output_sound((ORB2&0x3F)*2);
#else
	        BSTORE[((cl*(FREQ/50))/20000) % soundbufsize]=(ORB2&0x3F) | 0x80;
#endif
	    MON[0][0x7CD]=MON[1][0x7CD]=ORB2;
	}
	else
	{
	    DDRB2=OP;
	    MON[0][0x7CD]=MON[1][0x7CD]=DDRB2;
	}
	break;
	
        case 0xE7CE:  /* acces a CRA2 */
	CRA2=(CRA2&0xC0)+(OP&0x3F);
	MON[0][0x7CE]=MON[1][0x7CE]=CRA2;
	MON[0][0x7CC]=MON[1][0x7CC]=(CRA2&4 ? ORA2 : DDRA2);
	break;

        case 0xE7CF:  /* acces a CRB2 */
	CRB2=(CRB2&0xC0)+(OP&0x3F);
	MON[0][0x7CF]=MON[1][0x7CF]=CRB2;
	MON[0][0x7CD]=MON[1][0x7CD]=(CRB2&4 ? ORB2 : DDRB2);
	break;

        case 0xE7DA:  /* enter color value */
	if (GE7DB&1) {
	    palette[GE7DB]=(OP&0xF)+0xE0;
	    SetColor((GE7DB-1)>>1, palette[GE7DB-1]+(palette[GE7DB]<<8));
	} else palette[GE7DB]=OP;
	GE7DB=(GE7DB+1)&0x1F;
	MON[0][0x7DB]=MON[1][0x7DB]=GE7DB;
	MON[0][0x7DA]=MON[1][0x7DA]=palette[GE7DB];
	break;

        case 0xE7DB:  /* set index of next color to change */
	GE7DB=OP&0x1f;
	MON[0][0x7DB]=MON[1][0x7DB]=GE7DB;
	MON[0][0x7DA]=MON[1][0x7DA]=palette[GE7DB];
	break;

        case 0xE7DC:  /* select graphic mode */
	SetGraphicMode(OP);
	break;

        case 0xE7DD:  /* select border color */
	if ((GE7DD&0x0F)!=(OP&0x0F)) SetBorderColor(OP&0x0F);
	/* si modif des bits 6 OU 7 => la VRAM change de page*/
	if ((OP&0xC0)!=(GE7DD&0xC0))
	{
	    GE7DD=OP;
	    /* force le nouvel etat */
	    hardware(0xE7E6,GE7E6);
	    hardware(0xE7E5,GE7E5);
	    screen_refresh=1; /* sam: TRUE n'est pas defini apparemment */
#ifdef AMIGA
	    video_ram = RAM[GE7DD>>6];
#endif	
	}
	MON[0][0x7DD]=MON[1][0x7DD]=GE7DD=OP;
	break;
	
        case 0xE7E5:  /* commutation de RAM en mode Gate Array */
	baRAM=OP&0x1F;
	
	if (baRAM<32) /* la condition est tjs vraie mais conservee pour un eventuel mode 256ko */
	{
            SETMAP(0xA000, &RAM[baRAM][0x0000]);
            SETMAP(0xB000, &RAM[baRAM][0x1000]);
            SETMAP(0xC000, &RAM[baRAM][0x2000]);
            SETMAP(0xD000, &RAM[baRAM][0x3000]);
	    GE7E5=OP;
	    MON[0][0x7E5]=GE7E5;
	    MON[1][0x7E5]=GE7E5;
	    BankW=-1;
	    if (baRAM==(GE7DD>>6)) 
	    {
		store8tab[0xA]=store8tab[0xB]=store8tab[0xC]=store8tab[0xD]=fdraw2;
	    }
	    else
	    {
		store8tab[0xA]=store8tab[0xB]=store8tab[0xC]=store8tab[0xD]=map_ram;
	    }
	}
	else
	{
            SETMAP(0xA000, &EXT[0x0000]);
            SETMAP(0xB000, &EXT[0x1000]);
            SETMAP(0xC000, &EXT[0x2000]);
            SETMAP(0xD000, &EXT[0x3000]);
	    GE7E5=OP;
	    MON[0][0x7E5]=GE7E5;
	    MON[1][0x7E5]=GE7E5;
	    BankW=0;
	}
	    
	break;
	    
        case 0xE7E6:  /* commutation de l'espace ROM */
	/* commutation de l'espace ROM */		
	if ((OP&0x20)==0x20)
	{
	    int bank;
	    bank=OP&0x1F;
	    SETMAP(0x0000, &RAM[bank][0x2000]);
	    SETMAP(0x1000, &RAM[bank][0x3000]);
	    SETMAP(0x2000, &RAM[bank][0x0000]);
	    SETMAP(0x3000, &RAM[bank][0x1000]);
	    if(bank == (GE7DD>>6)) 
	    {
		store8tab[0] = store8tab[1] = store8tab[2] = store8tab[3] = fdraw2;
	    } else 
	    {
		store8tab[0] = store8tab[1] = store8tab[2] = store8tab[3] = map_rom;
	    }
	}
	else 
	{
	    if ((PRC&0x04)==0x04)
	    {
		SETMAP(0x0000, &ROM[baROM][0x0000]);
		SETMAP(0x1000, &ROM[baROM][0x1000]);
		SETMAP(0x2000, &ROM[baROM][0x2000]);
		SETMAP(0x3000, &ROM[baROM][0x3000]);
		store8tab[0] = store8tab[1] = store8tab[2] = store8tab[3] = map_rom;
	    }
	    else
	    {
		SETMAP(0x0000, &EXT[0x0000]);
		SETMAP(0x1000, &EXT[0x1000]);
		SETMAP(0x2000, &EXT[0x2000]);
		SETMAP(0x3000, &EXT[0x3000]);
		store8tab[0] = store8tab[1] = store8tab[2] = store8tab[3] = map_rom;
	    }
	}
	GE7E6=OP;
	MON[0][0x7E6]=GE7E6;
	MON[1][0x7E6]=GE7E6;
	break;

        case 0xE7E7:
	GE7E7=(OP&0x5F)+(GE7E7&0xA0);
	MON[0][0x7E7] = MON[1][0x7E7] = GE7E7;
	break;

        default:
	if ((ADR>=0xE7C2) && (ADR<0xE800))
	    MON[0][ADR&0xFFF]=MON[1][ADR&0xFFF]=OP;
    } /* end of switch */
}


void REGPARM map8(unsigned int ADR,BYTE OP)
{
#if 0 /* sam: 10/01/2000 orig code */
  int page;
  ADR &= 0xFFFF;
  page=ADR>>12;
  if (page==0xE) hardware(ADR,OP);
  else if ((page>=0x4) && (page<0xE)) GETMAP(ADR) = OP;
  else if ((ADR<0x8) && ((GE7E6&0x20)==0))
  {
     baROM=ADR&0x03;
    /* commutation des banques ROM */
    if ((PRC&0x04)!=0)
    {
      SETMAP(0x0000, &ROM[baROM][0x0000]);
      SETMAP(0x1000, &ROM[baROM][0x1000]);
      SETMAP(0x2000, &ROM[baROM][0x2000]);
      SETMAP(0x3000, &ROM[baROM][0x3000]);
    }
  }
  /* ecriture en RAM mappée en espace ROM */
  else if ((page<0x4) && ((GE7E6&0x60)==0x60))
  {
     GETMAP(ADR) = OP;
  }
#else
  switch((ADR >> 12) & 15) {
     case 0xF: return;
     case 0xE: hardware(ADR, OP); return;
     case 0x4: case 0x5: case 0x6: case 0x7: case 0x8: case 0x9:
     case 0xA: case 0xB: case 0xC: case 0xD: GETMAP(ADR) = OP; return;
     default: /* 0-3 */
     /* RAM mappée en ROM */
     if ((GE7E6 & 0x60) == 0x60) {GETMAP(ADR) = OP; return;}
     /* commutation banques ROM */
     if ((GE7E6 & 0x20) == 0 /* && ADR < 0x8 */) {
       baROM = ADR & 0x03;
       if ((PRC&0x04)!=0) {
          SETMAP(0x0000, &ROM[baROM][0x0000]);
          SETMAP(0x1000, &ROM[baROM][0x1000]);
          SETMAP(0x2000, &ROM[baROM][0x2000]);
          SETMAP(0x3000, &ROM[baROM][0x3000]);
       }
     }
     return;
  }
#endif
}

int REGPARM ld_ram(unsigned int ADR)
{
    return GETMAP(ADR);
}
int REGPARM ld_hwd(unsigned int ADR)
{
    static int TESTPROT=0;
    if((ADR&0xFFFF) == 0xE7D0) 
    {
	if (MON[0][0x7D0]==0x1B) {
	    TESTPROT=1;
	    MON[0][0x7D0]=MON[1][0x7D0]=2;
	    return(MON[0][0x7D0]);
	} else if (TESTPROT==1) {
	    TESTPROT=2;
	    MON[0][0x7D3]=MON[1][0x7D3]=0xFB;
	    MON[0][0x7D0]=MON[1][0x7D0]=128;
	    return(MON[0][0x7D0]);
	} else if (TESTPROT==2) {
	    MON[0][0x7D3]=MON[1][0x7D3]=0xF7;
	    MON[0][0x7D0]=MON[1][0x7D0]=128;
	    return(MON[0][0x7D0]);
	}
    }
    return GETMAP(ADR);
}

int REGPARM load8(unsigned int ADR)
{
    static int TESTPROT=0;
    
    if ( (MON[0][0x7D0]==0x1B) && ((ADR&0xFFFF)==0xE7D0) )
    {
	TESTPROT=1;
	MON[0][0x7D0]=MON[1][0x7D0]=2;
	return(MON[0][0x7D0]);
    }
    else
    if  ( (TESTPROT==1) && ((ADR&0xFFFF)==0xE7D0) )
    {
	TESTPROT=2;
	MON[0][0x7D3]=MON[1][0x7D3]=0xFB;
	MON[0][0x7D0]=MON[1][0x7D0]=128;
	return(MON[0][0x7D0]);
    }
    else
    if ( (TESTPROT==2) && ((ADR&0xFFFF)==0xE7D0) )
    {
	MON[0][0x7D3]=MON[1][0x7D3]=0xF7;
	MON[0][0x7D0]=MON[1][0x7D0]=128;
	return(MON[0][0x7D0]);
    }
    return GETMAP(ADR);
}

/* sam: tentative avec un tableau */
void REGPARM map_none(unsigned int ADR, BYTE OP) 
{
}
void REGPARM map_ram(unsigned int ADR, BYTE OP) 
{
    GETMAP(ADR) = OP;
/*     
    MAPPER[(ADR&0xFFFF)>>12][ADR&0xFFF]=OP;
*/
}
void REGPARM map_rom(unsigned int ADR, BYTE OP) 
{
    if (((GE7E6&0x20)==0) /* sam: 10/01/2000 && ((short)(ADR)<0x8)*/)
    {
	baROM=ADR&3;
	/* commutation des banques ROM */
	if ((PRC&0x04)!=0)
	{
	    SETMAP(0x0000, &ROM[baROM][0x0000]);
	    SETMAP(0x1000, &ROM[baROM][0x1000]);
	    SETMAP(0x2000, &ROM[baROM][0x2000]);
	    SETMAP(0x3000, &ROM[baROM][0x3000]);
	}
    }
    /* ecriture en RAM mappée en espace ROM */
    else if ((GE7E6&0x60)==0x60)
    {
	GETMAP(ADR) = OP;
    }
}

void setup_mem(void)
{
  int i;

#ifdef REG_MAPPER
  MAPPER = _MAPPER;
#endif

#define malloc(x) calloc(x,1)

  for (i=0;i<4;i++)
  {
    ROM[i]=malloc(0x4000*sizeof(ROM[0]));if (ROM[i]!=NULL) printf("Allocated 16ko ROM Bank[%d]\r",i); else {printf("\nFailed allocated 16ko ROM\n");crash();}
								       /* sam:              ^ */
                    /* sam: j'ai modifie l'affichage ici, car celui ci encombre moins l'ecran */
								
  }

  for (i=0;i<32;i++)
  {
    RAM[i]=malloc(0x4000*sizeof(RAM[0]));if (RAM[i]!=NULL) printf("Allocated 16ko RAM Bank[%d]\r",i); else {printf("\nFailed allocated 16ko RAM2\n");crash();}
  }

  EXT=malloc(0x4000*sizeof(EXT[0]));if (EXT!=NULL) printf("Allocated 16ko Non accessible RAM \r"); else {printf("\nFailed allocated 16ko EXT\n");crash();}


  /* fills with 255 */
  for (i=0;i<0x4000;i++) EXT[i]=0xFF;

  for (i=0;i<2;i++)
  {
    MON[i]=malloc(0x2000*sizeof(MON[0]));

    if (MON[i]!=NULL)
        printf("Allocated 8ko MONITOR Bank[%d]       \r",i);

    else
    {
        printf("\nFailed allocated 8ko MON\n");
        crash();
    }
  }

  MON0_7E7 = &MON[0][0x7E7];
  MON1_7E7 = &MON[1][0x7E7];

  printf("                                          \r");

  SETMAP(0x0000, &ROM[0][0x0000]);
  SETMAP(0x1000, &ROM[0][0x1000]);
  SETMAP(0x2000, &ROM[0][0x2000]);
  SETMAP(0x3000, &ROM[0][0x3000]);

  SETMAP(0x4000, &RAM[0][0x0000]);
  SETMAP(0x5000, &RAM[0][0x1000]);

  SETMAP(0x6000, &RAM[1][0x0000]);
  SETMAP(0x7000, &RAM[1][0x1000]);
  SETMAP(0x8000, &RAM[1][0x2000]);
  SETMAP(0x9000, &RAM[1][0x3000]);

  SETMAP(0xA000, &RAM[2][0x0000]);
  SETMAP(0xB000, &RAM[2][0x1000]);
  SETMAP(0xC000, &RAM[2][0x2000]);
  SETMAP(0xD000, &RAM[2][0x3000]);

  SETMAP(0xE000, &MON[0][0x0000]);
  SETMAP(0xF000, &MON[0][0x1000]);

/* init du mapper de VRAM */
  for (i=0;i<16;i++) IS_VRAM[i]=0;

/* l'adresse normale de la RAM video pour le redraw */
  IS_VRAM[4]=-1;
  IS_VRAM[5]=-1;

#ifdef DJGPP
  /* on locke en memoire physique toutes les variables manipulees dans le
     cadre d'une interruption materielle */
  _go32_dpmi_lock_data(MON[0],0x2000*sizeof(BYTE));
  _go32_dpmi_lock_data(MON[1],0x2000*sizeof(BYTE));
  /* cette partie de la RAM peut etre de la VRAM */
  for (i=0;i<4;i++)
      _go32_dpmi_lock_data(RAM[i],0x4000*sizeof(BYTE));
  LOCK_VARIABLE(port0);
  LOCK_VARIABLE(PRC);
  LOCK_VARIABLE(DDRA);
  LOCK_VARIABLE(ORA);
  LOCK_VARIABLE(DDRA2);
  LOCK_VARIABLE(ORA2);
  LOCK_VARIABLE(DDRB2);
  LOCK_VARIABLE(ORB2);
  LOCK_VARIABLE(CRA2);

  /* les variables utiles a fdraw sont lockees aussi */
  LOCK_VARIABLE(GE7DC);
  LOCK_VARIABLE(GE7DD);
  _go32_dpmi_lock_data(MAPPER,sizeof(MAPPER));
#endif /* DJGPP */
}
