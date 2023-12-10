/*
 * mem_mng.c: memory manager du TO8
 *             Ce module est independant du reste de l'emulateur et vient se
 *             greffer sur le debugger. Il permet de naviguer dans la memoire
 *             du TO8 sans influer sur son fonctionnement.
 *            directive de non-compilation: NO_MEMORY_MANAGER
 */

#ifdef DJGPP
#ifndef NO_DEBUGGER
#ifndef NO_MEMORY_MANAGER

#include <stdio.h>
#include <conio.h>
#include "debug.h"
#include "mapper.h"
#include "regs.h"

/* parametres de disposition des modules a l'ecran */
#define MENU_NLINE       5
#define MENU_POS_X       4
#define MENU_POS_Y      11
#define DISASS_POS_X    42
#define DISASS_POS_Y    15
#define   DUMP_POS_X    42
#define   DUMP_POS_Y    10
#define   DUMP_NBYTES    8
#define DIALOG_POS_X     4
#define DIALOG_POS_Y    18

static char map_line[9][81]={
"                               TO8 Memory Manager                               ",
"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄ¿",
"³0                3³4       5³6                 9³A                 D³E       F³",
"³0     BASICs     F³0 Video F³0     System      F³0      User       F³0  I/O  F³",
"³0      ROM       F³0  RAM  F³0      RAM        F³0      RAM        F³0  ROM  F³",
"³0                F³0       F³0                 F³0                 F³0       F³",
"ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄ´",
"³     bank   /3    ³bank   /1³                   ³     bank   /31    ³bank   /1³",
"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÙ                   ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÙ"
};

static char menu_line[MENU_NLINE][24]={ "l : load memory portion",
                                        "s : save memory portion",
                                        "j : jump to <address>  ",
                                        "q : quit memory manager",
                                        "TAB,+,- : navigate     "};

static int hot_point_loc[5][2]={{12,8},{26,8},{61,8},{76,8},{39,25}};
static int    current_bank[4];
static int        max_bank[4]={  3,    1,   31,  1};
static int        bank_loc[4]={  0,    4,  0xA,0xE};
static int       bank_size[4]={  4,    2,    4,  2};
static BYTE *VRAM[2];
static BYTE **mapper_table[4]={ROM, VRAM, RAM,MON};


/*
 * SetHotPoint: deplace le curseur de selection a l'emplacement voulu
 */

static void SetHotPoint(int hot_point)
{
    static int prev_hot_point;

  /* on eteint d'abord l'emplacement precedent du curseur */
    gotoxy(hot_point_loc[prev_hot_point][0],hot_point_loc[prev_hot_point][1]);
    if (prev_hot_point == 4)
    {
        textbackground(BLACK);
        textcolor(WHITE);
        cprintf("->");
    }
    else
    {
        textbackground(BLACK);
        textcolor(LIGHTGREEN);
        cprintf("%2d",current_bank[prev_hot_point]);
    }

  /* on allume le nouveau */
    gotoxy(hot_point_loc[hot_point][0],hot_point_loc[hot_point][1]);
    if (hot_point == 4)
    {
        textbackground(WHITE);
        textcolor(BLACK);
        cprintf("->");
    }
    else
    {
        textbackground(LIGHTGREEN);
        textcolor(BLACK);
        cprintf("%2d",current_bank[hot_point]);
    }
    prev_hot_point=hot_point;
}


/*
 * DeleteBox: efface une portion rectangulaire de l'ecran
 */

static void DeleteBox(int left, int top, int right, int bottom)
{
    register int i,j;

    for (i=top; i<=bottom; i++)
    {
        gotoxy(left,i);
        for (j=left; j<=right; j++)
            putch(32);
    }
}


/*
 * UpdateDisassAndDump: desassemble l'instruction suivante et met a jour si
 *                      necessaire le dump memoire
 */

static void UpdateDisassAndDump(void)
{
    unsigned int q=P_PC/DUMP_NBYTES;
    register int i;
             int c;

    textbackground(BLACK);
    textcolor(WHITE);

    movetext(DISASS_POS_X,DISASS_POS_Y+2,80,25,DISASS_POS_X,DISASS_POS_Y+1);
     /* decale l'ecran de desassemblage d'une ligne vers le haut */

    DeleteBox(DISASS_POS_X,25,79,25);

    gotoxy(DISASS_POS_X,25);
    desass();

    if ((P_PC/DUMP_NBYTES) != q)
    {
        movetext(DUMP_POS_X,DUMP_POS_Y+2,80,DISASS_POS_Y-1,DUMP_POS_X,DUMP_POS_Y+1);
         /* decale l'ecran de dump d'une ligne vers le haut */

        DeleteBox(DUMP_POS_X,DISASS_POS_Y-1,80,DISASS_POS_Y-1);

        gotoxy(DUMP_POS_X,DISASS_POS_Y-1);
        cprintf("%04X ",q*DUMP_NBYTES);   /* affiche l'adresse */

        for (i=0; i<DUMP_NBYTES; i++)
            cprintf("%02X ",LOAD8(q*DUMP_NBYTES+i));

        cprintf(" ");
        for (i=0; i<DUMP_NBYTES; i++)
        {
            c=LOAD8(q*DUMP_NBYTES+i);
            if ((c>=32) && (c<=125))
                cprintf("%c",c);
            else
                cprintf(".");
        }
    }
}


static void ReadFileName(int pos_x, int pos_y, const char *mesg, char *file_name)
{
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(pos_x,pos_y);
    cputs(mesg);
    _setcursortype(_NORMALCURSOR);
    cscanf("%s",file_name);
    _setcursortype(_NOCURSOR);
}


static void ReadAddress(int pos_x, int pos_y, const char *mesg, int *addr)
{
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(pos_x,pos_y);
    cputs(mesg);
    _setcursortype(_NORMALCURSOR);
    cscanf("%4x",addr);
    _setcursortype(_NOCURSOR);
}


/*
 * MemoryManager: point d'entree du module; affiche l'ecran du memory manager
 *                et lance la boucle principale
 */

void MemoryManager(void)
{
    register int i;
             int c,hot_point=4;
    BYTE *old_mapper[16];

  /* on sauvegarde d'abord l'etat du mapper */
    for (i=0; i<16; i++)
        old_mapper[i]=MAPPER[i];

  /* on lit ensuite l'etat des banques memoire et du PC */
    VRAM[0]=RAM[0];
    VRAM[1]=RAM[0]+0x2000;
    current_bank[0]=baROM;
    current_bank[1]=PRC&1;
    current_bank[2]=baRAM;
    current_bank[3]=baMON;
    P_PC=PC;

  /* on affiche l'ecran du memory manager */
    clrscr();

    /* les trois barres de titres */
    textbackground(BLUE);
    textcolor(LIGHTGRAY);
    cputs(map_line[0]);
    gotoxy(DUMP_POS_X-3,DUMP_POS_Y);
    cprintf("               Memory Dump                ");
    gotoxy(DISASS_POS_X-3,DISASS_POS_Y);
    cprintf("            6809 Disassembler             ");

    /* la carte de la memoire */
    textbackground(BLACK);
    textcolor(WHITE);
    gotoxy(1,2);
    for (i=1; i<7; i++)
        cputs(map_line[i]);

    /* le desassembleur */
    for (i=0; i<25-DISASS_POS_Y; i++)
        UpdateDisassAndDump();

    /* les volets des banques */
    textcolor(LIGHTGREEN);
    gotoxy(1,8);
    cputs(map_line[7]);
    cputs(map_line[8]);
    for (i=0; i<4; i++)
    {
        gotoxy(hot_point_loc[i][0],hot_point_loc[i][1]);
        cprintf("%2d",current_bank[i]);
    }

    /* le menu */
    textcolor(LIGHTCYAN);
    for (i=0; i<MENU_NLINE; i++)
    {
        gotoxy(MENU_POS_X,MENU_POS_Y+i);
        cputs(menu_line[i]);
    }

    SetHotPoint(hot_point);

    do  /* boucle principale */
    {
        switch (c=getch())
        {
            case 9: /* TAB: circulation du curseur */
                if (++hot_point == 5)
                    hot_point=0;
                SetHotPoint(hot_point);
                break;

            case '+': /* banque suivante ou instruction suivante */
                if (hot_point == 4)
                    UpdateDisassAndDump();
                else
                {
                    if (current_bank[hot_point] < max_bank[hot_point])
                    {
                        current_bank[hot_point]++;
                        for (i=0; i<bank_size[hot_point]; i++)
                            MAPPER[bank_loc[hot_point]+i]=
                             mapper_table[hot_point][current_bank[hot_point]]+i*0x1000;
                        SetHotPoint(hot_point);
                    }
                }
                break;

            case '-': /* banque precedente */
                if (hot_point != 4)
                {
                    if (current_bank[hot_point] > 0)
                    {
                        current_bank[hot_point]--;
                        for (i=0; i<bank_size[hot_point]; i++)
                            MAPPER[bank_loc[hot_point]+i]=
                             mapper_table[hot_point][current_bank[hot_point]]+i*0x1000;
                        SetHotPoint(hot_point);
                    }
                }
                break;

            case 'j': /* saut a l'adresse voulue */
            case 'J':
                ReadAddress(DIALOG_POS_X,DIALOG_POS_Y,"Enter new address: ",&P_PC);
                DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y);
                hot_point=4;
                SetHotPoint(hot_point);
                UpdateDisassAndDump();
                break;

            case 'l': /* chargement d'une portion memoire */
            case 'L':
                {
                    char file_name[16];
                    int  addr,d;
                    FILE *file;

                    ReadFileName(DIALOG_POS_X,DIALOG_POS_Y,"Enter file name: ",file_name);
                    ReadAddress(DIALOG_POS_X,DIALOG_POS_Y+1,"Enter address (from): ",&addr);

	            if ((file=fopen(file_name,"rb")) == NULL)
	            {
                        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+2);
			cprintf("Error !!!");
                    }
                    else
                    {
                        while ((addr<0x10000)&&((d=fgetc(file))!=EOF))
                        {
                            MAPPTR(addr)[addr&0xFFF]=d&0xFF;
                            addr++;
                        }
                        fclose(file);
                        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+2);
                        cprintf("Completed");
                    }
                    gotoxy(DIALOG_POS_X,DIALOG_POS_Y+3);
                    cprintf("<space> to resume");
                    while (getch() != 32)
                        ;
                    DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y+3);
                }
                break;

            case 's': /* sauvegarde d'une portion memoire */
            case 'S':
                {
                    char file_name[16];
                    int  addr1,addr2;
                    FILE *file;

                    ReadFileName(DIALOG_POS_X,DIALOG_POS_Y,"Enter file name: ",file_name);
                    ReadAddress(DIALOG_POS_X,DIALOG_POS_Y+1,"Enter address (from): ",&addr1);
                    ReadAddress(DIALOG_POS_X,DIALOG_POS_Y+2,"Enter address (to): ",&addr2);

	            if ((file=fopen(file_name,"wb")) == NULL)
	            {
                        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+3);
			cprintf("Error !!!");
                    }
                    else
                    {
                        for (i=addr1; i<=addr2; i++)
                            fputc(MAPPTR(i)[i&0xFFF]&0xFF, file);
                        fclose(file);
                        gotoxy(DIALOG_POS_X,DIALOG_POS_Y+3);
                        cprintf("Completed");
                    }
                    gotoxy(DIALOG_POS_X,DIALOG_POS_Y+4);
                    cprintf("<space> to resume");
                    while (getch() != 32)
                        ;
                    DeleteBox(DIALOG_POS_X,DIALOG_POS_Y,40,DIALOG_POS_Y+4);
                }
                break;
        } /* end of switch */
    }
    while ((c != 'q') && (c != 'Q'));

  /* on restaure l'ancien mapper */
    for (i=0; i<16; i++)
        MAPPER[i]=old_mapper[i];
}

#endif /* NO_MEMORY_MANAGER */
#endif /* NO_DEBUGGER */

#endif

