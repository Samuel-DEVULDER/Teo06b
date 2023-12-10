/*

	VERSION     : 2.0 (TO8)
	MODULE      : mapper.h
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	Definition des macros pour l'utilisation de la
	memoire paginee de la machine virtuelle

*/

#define BYTE unsigned int

#ifdef AMIGA /* amiga ou autre vieux cpu ou l'acces 32bit n'est pas le plus rapide */
#define BYTE8  unsigned char
#define BYTE16 unsigned short
#else
#define BYTE8  BYTE
#define BYTE16 BYTE
#endif

extern int page;

#ifdef BIG_MAPPER
# define MAPPER_SIZE    (0x10000/4)
# define MAPPTR(ADR)    (*(BYTE8**)(((BYTE8*)MAPPER) + ((ADR)&0xF000)))
#else
# define MAPPER_SIZE    0x10
# define MAPPTR(ADR)    MAPPER[((ADR)&0xFFFF)>>12]
#endif

#ifndef REG_MAPPER
extern BYTE8 * MAPPER[];
#else
extern BYTE8 * _MAPPER[];
#endif

/*
pour eviter de faire (ADR & 0xFFF): plus rapide.
*/
#define MAP_OFFSET

#ifdef MAP_OFFSET
# define GETMAP(ADR)        MAPPTR(ADR)[(ADR)]
# define SETMAP(ADR, VAL)   MAPPTR(ADR) = ((VAL)-((ADR)&0xF000))
# define GETMAP2(A1, A2)    MAPPTR(A1<<12)[(A2)]
#else
# define GETMAP(ADR)        MAPPTR(ADR)[(ADR)&0xFFF]
# define SETMAP(ADR, VAL)   MAPPTR(ADR) = (VAL) 
# define GETMAP2(A1, A2)    MAPPTR(A1<<12)[(A2)&0xFFF]
#endif

/* drapeau d'actualisation de l'écran */
extern BYTE8 IS_VRAM[16];

/* 512ko de RAM */
extern BYTE8 * RAM[32];

#ifdef AMIGA
extern BYTE8 *video_ram;
#endif

/* espace MEMO7 */
extern BYTE8 * EXT;

/* 16ko de ROM */
extern BYTE8 * ROM[4];

extern BYTE8 * MON[2], *MON0_7E7, *MON1_7E7;

typedef void (*store_funcs)(unsigned int, BYTE) REGPARM;
typedef int  (*load_funcs)(unsigned int) REGPARM;

extern store_funcs store8tab[16];
extern load_funcs  load8tab[16];

#if !defined(__GNUC__)||!defined(AMIGA)
#define STORE8(ADR,OP) (*store8tab[((ADR)&0xFFFF)>>12])(ADR,(OP))
#define PREP_BURST_W
#define BURST_W STORE8
#else
#define PREP_BURST_W
#define BURST_W STORE8

//#define PREP_BURST_W    store_funcs *sto=store8tab
//#define BURST_W(ADR,OP) (*sto[((ADR)&0xFFFF)>>12])((ADR),(OP))

//#define STORE8(ADR,OP)  (*store8tab[((ADR)&0xFFFF)>>12])(ADR,(OP))
#define STORE8(ADR,OP) do {register store_funcs *sto __asm("a0")=store8tab; (*sto[((ADR)&0xFFFF)>>12])((ADR),(OP));} while(0)
#endif

#ifdef NO_PROTECT
#define LOAD8(ADR) (GETMAP(ADR))
#else
#define LOAD8(ADR) ((*load8tab[((ADR)&0xFFFF)>>12])(ADR))
#endif

/* drapeau d'attente sur interrupt */
extern int wait;

/* clock : comptage des cycles */
#ifndef REG_cl
extern unsigned int cl;
#endif

extern int baROM;
extern int baMON;
extern int baRAM;

extern BYTE port0;

/* Registres du PIA manettes */
extern BYTE ORA2,ORB2,DDRA2,DDRB2,CRA2,CRB2;

/* registres du 6846 */
extern BYTE CRC,PRC;

/* Registres du 6821 */
extern BYTE ORA,ORB,DDRA,DDRB,CRA,CRB;

/* regs du Gate Array */
extern BYTE GE7E5,GE7E6,GE7E7;

/* Le Gate Array Video */
extern BYTE GE7DA,GE7DB,GE7DC,GE7DD;

extern void setup_mem(void);
extern void REGPARM hardware(unsigned int ADR, BYTE OP);
extern void REGPARM map8(unsigned int ADR, BYTE OP);
extern int  REGPARM load8(unsigned int ADR);
