/*

	MARCEL O CINQ
	*************

	l'émulateur de thomson MO5

	Par Gilles FETIS

	mail : FETIS_Gilles@compuserve.com
	page : http://ourworld.compuserve.com/homepages/FETIS_Gilles


*/


/*

	VERSION     : 2.0
	MODULE      : DEBUG.C
	Créé par    : Gilles FETIS
	Modifié par : Eric Botcazou

	Débuggeur & désassembleur intégré
        directive de non compilation: NO_DEBUGGER
*/

#include "flags.h"

#ifndef NO_DEBUGGER

#include <stdio.h>
#include <stdlib.h>

#ifdef DJGPP
#include <conio.h>
#endif
#ifdef USE_ALLEGRO
#include <allegro.h>
#endif

#include "mapper.h"
#include "regs.h"
#include "mc6809.h"
#include "graph.h"
#include "mem_mng.h"

/* Lecture de l'instruction/operandes */
#define FETCH m=LOAD8(PC);PC++

char MNEMO[256][6];
char MNEMO10[256][6];
char MNEMO11[256][6];

unsigned int P_PC;

static char reg[12][3]={"D","X","Y","U","S","PC","","","A","B","CC","DP"};
static int stack_reg[2][8]={ {5,3,2,1,11,9,8,10},
                             {5,4,2,1,11,9,8,10} };

#ifndef DJGPP
#ifdef ANSI_ESC
#define cprintf printf
#define gotoxy(a,b) printf("\033[%d;%dH",b-1,a-1)
#define BLACK 1
#define LIGHTCYAN 2
#define YELLOW 3
#define WHITE 4
#define putch(A) 	  putc(A,stdout)
#define textcolor(x)      printf("\033[3%dm",x)
#define textbackground(x) printf("\033[4%d;>%dm",x,x)
#define clrscr()	  printf("\f"),fflush(stdout)
#define cscanf scanf
#define CURSOR_ON	  printf("\033[ p")
#define CURSOR_OFF	  printf("\033[0 p")
#else /* !ANSI_ESC */
#define cprintf printf
#define gotoxy(a,b) printf("\n");
#define BLACK 1
#define LIGHTCYAN 2
#define YELLOW 3
#define WHITE 4
#define putch(A) putc(A,stdout)
#define textbackground(A)
#define textcolor(A)
#define clrscr()
#define cscanf scanf
#define CURSOR_ON
#define CURSOR_OFF
#endif /* ANSI_ESC */
#endif /* DJGPP */

/*
 * aff_r: affichage des registres du 6809
 */


static void aff_r(void)
{
	gotoxy(1,1);
	cprintf("PC : %04x DP : %02x",PC,DP);
	gotoxy(1,2);
	cprintf("X  : %04x Y  : %04x",X,Y);
	gotoxy(1,3);
	cprintf("U  : %04x S  : %04x",U,S);
	gotoxy(1,4);
	cprintf("A  : %02x   B  : %02x",A,B);
	gotoxy(1,5);
	cprintf("CC : %c%c%c%c%c%c%c%c"
	,((CC&0x80)==0x80)?'E':'.'
	,((CC&0x40)==0x40)?'F':'.'
	,((CC&0x20)==0x20)?'H':'.'
	,((CC&0x10)==0x10)?'I':'.'
	,((CC&0x08)==0x08)?'N':'.'
	,((CC&0x04)==0x04)?'Z':'.'
	,((CC&0x02)==0x02)?'V':'.'
	,((CC&0x01)==0x01)?'C':'.'
	);

	gotoxy(1,6);cprintf("[S  ] [S+1] [S+2] [S+3] [S+4]\n");
	gotoxy(1,7);cprintf(" %2X    %2X    %2X    %2X    %2X\n",LOAD8(S),LOAD8(S+1),LOAD8(S+2),LOAD8(S+3),LOAD8(S+4));
}

static void des_indexe(int m)
{
	int mmm;

	if (!(m&0x80)) /* 5-Bit Offset */
	{
            if (m&0x10)
                cprintf("-%x,%s",(-m)&0xF,reg[((m&0x60)>>5)+1]);
            else
                cprintf("%x,%s",m&0xF,reg[((m&0x60)>>5)+1]);
            return;
        }

	switch (m&0x1F)
	{
	case 0x04 :	cprintf(",%s",reg[((m&0x60)>>5)+1]);
			break;
	case 0x14 :	cprintf("[,%s]",reg[((m&0x60)>>5)+1]);
			break;
	case 0x08 :   	mmm=LOAD8(P_PC);P_PC++;
			cprintf("%x,%s",mmm,reg[((m&0x60)>>5)+1]);
			break;
	case 0x18 :   	mmm=LOAD8(P_PC);P_PC++;
			cprintf("[%x,%s]",mmm,reg[((m&0x60)>>5)+1]);
			break;
	case 0x09 :   	mmm=LOAD8(P_PC)<<8;P_PC++;
			mmm|=LOAD8(P_PC);P_PC++;
			cprintf("%x,%s",mmm,reg[((m&0x60)>>5)+1]);
			break;
	case 0x19 :   	mmm=LOAD8(P_PC)<<8;P_PC++;
			mmm|=LOAD8(P_PC);P_PC++;
			cprintf("[%x,%s]",mmm,reg[((m&0x60)>>5)+1]);
			break;
	case 0x06 :   	cprintf("A,%s",reg[((m&0x60)>>5)+1]);
			break;
	case 0x16 :   	cprintf("[A,%s]",reg[((m&0x60)>>5)+1]);
			break;
	case 0x05 :   	cprintf("B,%s",reg[((m&0x60)>>5)+1]);
			break;
	case 0x15 :   	cprintf("[B,%s]",reg[((m&0x60)>>5)+1]);
			break;
	case 0x0B :   	cprintf("D,%s",reg[((m&0x60)>>5)+1]);
			break;
	case 0x1B :   	cprintf("[D,%s]",reg[((m&0x60)>>5)+1]);
			break;
	case 0x00 :   	cprintf(",%s+",reg[((m&0x60)>>5)+1]);
			break;
	case 0x01 :   	cprintf(",%s++",reg[((m&0x60)>>5)+1]);
			break;
	case 0x11 :   	cprintf("[,%s++]",reg[((m&0x60)>>5)+1]);
			break;
	case 0x02 :   	cprintf(",-%s",reg[((m&0x60)>>5)+1]);
			break;
	case 0x03 :   	cprintf(",--%s",reg[((m&0x60)>>5)+1]);
			break;
	case 0x13 :   	cprintf("[,--%s]",reg[((m&0x60)>>5)+1]);
			break;
	case 0x0C :   	mmm=LOAD8(P_PC);P_PC++;
			cprintf("%x,PC",mmm);
			break;
	case 0x1C :   	mmm=LOAD8(P_PC);P_PC++;
			cprintf("[%x,PC]",mmm);
			break;
	case 0x0D :   	mmm=LOAD8(P_PC)<<8;P_PC++;
			mmm|=LOAD8(P_PC);P_PC++;
			cprintf("%x,PC",mmm);
			break;
	case 0x1D :   	mmm=LOAD8(P_PC)<<8;P_PC++;
			mmm|=LOAD8(P_PC);P_PC++;
			cprintf("[%x,PC]",mmm);
			break;
	case 0x1F :   	mmm=LOAD8(P_PC)<<8;P_PC++;
			mmm|=LOAD8(P_PC);P_PC++;
			cprintf("[%x]",mmm);
			break;
	default   :	cprintf("Illegal !");
	}
}


static void r_pile(char w, int stack, int m)
{
    register int i;
             int first=1;

    if (w == 'H')  /* PUSH */
    {
        for (i=0; i<8; i++)
            if ((0x80>>i)&m)
            {
                if (first)
                    first=0;
                else
                    putch(',');

                cprintf("%s",reg[stack_reg[stack][i]]);
            }
    }
    else  /* PULL */
        for (i=7; i>=0; i--)
            if ((0x80>>i)&m)
            {
                if (first)
                    first=0;
                else
                    putch(',');

                cprintf("%s",reg[stack_reg[stack][i]]);
            }
}


void desass(void)
{
  char *mnemo;
  int mm;

  cprintf("%04X ",P_PC);
  mm=LOAD8(P_PC);P_PC++;

  if (mm==0x10)
  {
    mm=LOAD8(P_PC);P_PC++;
    mnemo=MNEMO10[mm];
    cprintf("10 %02X %c%c%c%c ",mm,mnemo[0],mnemo[1],mnemo[2],mnemo[3]);
  }
  else if (mm==0x11)
  {
    mm=LOAD8(P_PC);P_PC++;
    mnemo=MNEMO11[mm];
    cprintf("11 %02X %c%c%c%c ",mm,mnemo[0],mnemo[1],mnemo[2],mnemo[3]);
  }
  else
  {
    mnemo=MNEMO[mm];
    cprintf("%02X    %c%c%c%c ",mm,mnemo[0],mnemo[1],mnemo[2],mnemo[3]);
  }

  switch (mnemo[4])
  {
  case 'I' :    mm=LOAD8(P_PC);P_PC++;
		cprintf("#%02X",mm);
		mm=LOAD8(P_PC);P_PC++;
		cprintf("%02X",mm);
		break;
  case 'i' :    mm=LOAD8(P_PC);P_PC++;
		cprintf("#%02X",mm);
		break;
  case 'e' :    mm=LOAD8(P_PC);P_PC++;
		cprintf(">%02X",mm);
		mm=LOAD8(P_PC);P_PC++;
		cprintf("%02X",mm);
		break;
  case 'd' :    mm=LOAD8(P_PC);P_PC++;
		cprintf(".%02X",mm);
		break;
  case 'o' :    mm=LOAD8(P_PC);P_PC++;
		cprintf(">%04X",P_PC+(signed char)mm);
		break;
  case 'O' :    mm=LOAD8(P_PC)<<8;P_PC++;
		mm|=LOAD8(P_PC);P_PC++;
		cprintf(">%04X",P_PC+(signed short)mm);
		break;
  case 'x' :	mm=LOAD8(P_PC);P_PC++;
		des_indexe(mm);
		break;
  case 'r' :	mm=LOAD8(P_PC);P_PC++;
		cprintf("%s,%s",reg[mm>>4],reg[mm&0xF]);
		break;
  case 'R' :	mm=LOAD8(P_PC);P_PC++;
		r_pile(mnemo[2],mnemo[3]=='S' ? 0 : 1,mm);
		break;
  }
}


static unsigned int readword(void)
{
	unsigned int v;
	gotoxy(1,20);cprintf("Enter HEX Value :");
	cscanf("%x",&v);
	gotoxy(1,21);cprintf("Val is : %4X < OK >",v);
	getch();
	return v;
}


static void DisplayHardRegisters()
{
  clrscr();
  textcolor(LIGHTCYAN);

  gotoxy(1,1);
  cprintf("GATE MODE PAGE");
  gotoxy(1,2);
  cprintf("E7E5=%02X | E7E6=%02X | E7E7=%02X",GE7E5,GE7E6,GE7E7);

  gotoxy(1,4);
  cprintf("GATE VIDEO");
  gotoxy(1,5);
  cprintf("E7DA=%02X | E7DB=%02X | E7DC=%02X | E7DD=%02X",GE7DA,GE7DB,GE7DC,GE7DD);

  gotoxy(1,7);
  cprintf("6846");
  gotoxy(1,8);
  cprintf("E7C0 %02X E7C1 %02X E7C2 %02X E7C3 %02X",
	  port0,CRC,MON[0][0x7C3],PRC);
  gotoxy(1,9);
  cprintf("E7C4 %02X E7C5 %02X E7C6 %02X E7C7 %02X",
	  MON[0][0x7C4],MON[0][0x7C5],MON[0][0x7C6],MON[0][0x7C7]);
}


/*
 * debugger: fonction principale du debugger
 */

void debugger(void)
{
  register int i;
           int ex=0,next;
  char c;

#ifdef Xwin_ALLEGRO
  /*
   * le clavier ne fonctionne pas sous XwinAllegro
   * on se contente d'afficher 2 ou 3 trucs
   */
  
  getcc();
  aff_r();
  DisplayHardRegisters();
 
  return;
#endif

#ifdef DJGPP
  _setcursortype(_NOCURSOR);
#else
  CURSOR_OFF;
#endif /* DJGPP */
  do
  {
    textbackground(BLACK);
    textcolor(LIGHTCYAN);
    clrscr();
    gotoxy(1,12);cprintf("s : step");
    gotoxy(1,13);cprintf("u : go until PC > actual PC");
    gotoxy(1,14);cprintf("b : set breakpoint");
    gotoxy(1,15);cprintf("d : display screen");
    gotoxy(1,16);cprintf("r : display registers");
    gotoxy(1,17);cprintf("q : quit debugger");

#ifndef NO_MEMORY_MANAGER
    gotoxy(1,18);cprintf("m : memory manager");
#endif

    textcolor(YELLOW);
    getcc();
    aff_r();

    textcolor(WHITE);
    P_PC=PC;
    for (i=0;i<16;i++)
    {
        gotoxy(40,i+1);
        desass();
    }

    c=getch();
    if ((c=='s') || (c=='S'))
    {
      next=FETCH;
      (*(*PFONCT[next]))();
    }

    if ((c=='d') || (c=='D'))
    {
        SetGraphicMode(INIT);
        getch();
        ReturnText();
#ifdef DJGPP
        _setcursortype(_NOCURSOR);
#endif
    }

    if ((c=='r') || (c=='R'))
    {
      DisplayHardRegisters();
      getch();
    }
    if ((c=='u') || (c=='U'))
    {
      unsigned int mem_PC;

      mem_PC=PC;
      do
      {
	next=FETCH;
	(*(*PFONCT[next]))();
      } while ((PC<=mem_PC)||(PC>mem_PC+8));
    }
    if ((c=='b') || (c=='B'))
    {
      unsigned int BPOINT;

      BPOINT=readword();

      do
      {
	next=FETCH;
	(*(*PFONCT[next]))();
      } while ((PC<=BPOINT)||(PC>BPOINT+8));

    }

#ifndef NO_MEMORY_MANAGER
    if ((c=='m') || (c=='M'))
        MemoryManager();
#endif

    if ((c=='q') || (c=='Q')) ex=1;

  } while (ex==0);
#ifndef DJGPP
  CURSOR_ON;
#endif
}

#else /* NO_DEBUGGER */
char MNEMO[256][6];
char MNEMO10[256][6];
char MNEMO11[256][6];

void debugger(void)
{
}
#endif
