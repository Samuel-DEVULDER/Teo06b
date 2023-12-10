/*
 * amiga/gui.c - Gestion MENU simpliste (ASCII).
 * 
 * Créé par Samuel Devulder, 08/98.
 */

#include "flags.h"
#include "main.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <exec/types.h>
#include <exec/execbase.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>

#include "main.h"
#include "mapper.h"
#include "sound.h"
#include "regs.h"
#include "graph.h"
#include "diskio.h"
#include "keyb.h" /* for numlock */

#include "zfile.h"

/***************************************************************************/

#define SIZE 256 /* taille des chaines */

/***************************************************************************/

/* gestion des K7 virtuelles */
FILE *K7in=NULL;

extern int exact;
extern int do_gamma;
extern int do_fastgfx;
extern int sound_ok;
extern int use_os;
extern int MODE80_ID;
extern int MODE40_ID;

char disk0name[SIZE];
char disk1name[SIZE];
char memo7name[SIZE];
char cass7name[SIZE];

static FILE *WIN;

extern void ShowScreen(int on);
extern void GetModeIDs(void);

/***************************************************************************/
#define bl(x) ((x)==' ' || (x)=='\t')
#define be(x) (bl((x)) || (x)=='=')
#define PREFFILE    "PROGDIR:teo.prefs"
#define EXACT "exact"
#define GAMMA "gamma"
#define SOUND "sound"
#define OSFRD "osfrd"
#define DISK0 "disk0"
#define DISK1 "disk1"
#define K7NAM "k7nam"
#define M7NAM "m7nam"
#define NUMLK "numlk"
#define M80ID "m80id"
#define M40ID "m40id"
#define FSGFX "fsgfx"
void LoadPrefs(void)
{
    FILE *f = fopen(PREFFILE, "r");

    if(!f) return;
    while(!feof(f)) {
        char buf[256];
	if(fgets(buf, sizeof(buf), f)) {
	   char *a1, *a2, *s = buf;
	   for(; bl(*s); ++s); a1 = s;
	   for(; *s && !be(*s); ++s); if(!*s) continue; *s++ = 0;
	   for(; be(*s); ++s); a2 = s;
	   for(; *s && *s != '\n'; ++s); *s = 0;
           /* process args */
           if(!strcmp(a1, EXACT)) exact      = atoi(a2);
           if(!strcmp(a1, GAMMA)) do_gamma   = atoi(a2);
           if(!strcmp(a1, SOUND)) sound_ok   = atoi(a2);
           if(!strcmp(a1, OSFRD)) use_os     = atoi(a2);
           if(!strcmp(a1, M80ID)) MODE80_ID  = atoi(a2);
           if(!strcmp(a1, M40ID)) MODE40_ID  = atoi(a2);
           if(!strcmp(a1, FSGFX)) do_fastgfx = atoi(a2);
#ifdef AMIGA
           if(!strcmp(a1, NUMLK)) {
                kb_state=atoi(a2)?KB_NUMLOCK_FLAG:0;
                set_leds(kb_state); /* on met a jour la led NUMLOCK */
                ResetPortManette0();
	   }
#endif
           if(!strcmp(a1, DISK0)) strcpy(disk0name, a2);
           if(!strcmp(a1, DISK1)) strcpy(disk1name, a2);
           if(!strcmp(a1, K7NAM)) strcpy(cass7name, a2);
           if(!strcmp(a1, M7NAM)) strcpy(memo7name, a2);
        } else break;
    }
    fclose(f);
}

void SavePrefs(void)
{
    FILE *f = fopen(PREFFILE, "w");
    if(!f) return;
#define OUT_INT(f, T, v) fprintf(f, "%s=%d\n", T, v);
#define OUT_STR(f, T, v) fprintf(f, "%s=%s\n", T, v);
    OUT_INT(f, EXACT, exact);
    OUT_INT(f, GAMMA, do_gamma);
    OUT_INT(f, SOUND, sound_ok);
    OUT_INT(f, OSFRD, use_os);
    OUT_INT(f, NUMLK, (kb_state&KB_NUMLOCK_FLAG)?1:0);
    OUT_INT(f, M80ID, MODE80_ID);
    OUT_INT(f, M40ID, MODE40_ID);
    OUT_INT(f, FSGFX, do_fastgfx);
    OUT_STR(f, DISK0, disk0name);
    OUT_STR(f, DISK1, disk1name);
    OUT_STR(f, K7NAM, cass7name);
    OUT_STR(f, M7NAM, memo7name);
    fclose(f);
}

/***************************************************************************/

/* commandes ANSI de base */
static void GOTOXY(int x, int y)
{
    fprintf(WIN,"\033[%d;%dH",y,x);
}

static void FOND(int col)
{
    fprintf(WIN,"\033[4%dm",col);
}

static void FORME(int col)
{
    fprintf(WIN,"\033[3%dm",col);
}

static void SCREEN(int forme, int fond)
{
    fprintf(WIN,"\033[3%d;4%d;>%dm",forme,fond,fond);
}

static void CLS(void)
{
    fprintf(WIN,"\f");
}

static void BEEP(void)
{
    fprintf(WIN,"\a");
}

static void WRAP(int on)
{ 
    fprintf(WIN,on?"\033[?7h":"\033[?7l");
}

static void BOLD(int on)
{
    fprintf(WIN,on?"\033[1m":"\033[22m");
}

static void CURSOR(int on)
{
    fprintf(WIN,on?"\033[ p":"\033[0 p");
}

static void CLEOD(void)
{
    fprintf(WIN,"\033[J");
}

/***************************************************************************/
/*
 * Elements de base de l'intergace gfx
 */

#define GRIS  0
#define NOIR  1
#define BLANC 2
#define BLEU  3
#define JAUNE 6

static void HLINE(void)
{
    FORME(BLEU);FOND(GRIS);
    fprintf(WIN,
"_____________________________________________________________________________\n");
}

static void HLINE2(void)
{
    FORME(BLEU);FOND(GRIS);
    fprintf(WIN,
"_____________________________________________________________________________\n"
"   ______                                                        ______      \n");
}

static void SLINE(void)
{
    fprintf(WIN,
"                                                                             \n");
}

static void ZONE(int on)
{
    FORME(BLEU);FOND(GRIS);fprintf(WIN,"|");
    FOND(on?JAUNE:NOIR);fprintf(WIN,"______");
    FOND(GRIS); fprintf(WIN,"|");
}

static void TEOTitle(char *menutitle)
{
    int i;
    GOTOXY(0,0);
    FORME(BLANC);FOND(BLEU);
    fprintf(WIN,"   ____________    __________     _________                                  \n");
    fprintf(WIN,"  |____    ____|  |   _______|   /   ___   \\                                 \n");
    fprintf(WIN,"       |  |       |  |____      |   /   \\   |                                \n");
    fprintf(WIN,"       |  |       |   ____|     |  {     }  | ");  FORME(NOIR); fprintf(WIN, "        __     __              \n"); FORME(BLANC);
    fprintf(WIN,"       |  |       |  |_______   |   \\___/   | "); FORME(NOIR); fprintf(WIN," \\  /  |  |   |__              \n"); FORME(BLANC);
    fprintf(WIN,"       |__|       |__________|   \\_________/  "); FORME(NOIR); fprintf(WIN,"  \\/   |__| o |__| (%c)         \n",VERNUM); FORME(BLANC);
    fprintf(WIN,"                                                                             \n");
    FOND(GRIS);FORME(NOIR);
    SLINE();
    i = 75-strlen(menutitle);
    while(i>0) {--i; fputc(' ',WIN);}
    fprintf(WIN,"%s  \n",menutitle);
    FORME(BLEU);
    SLINE();
    {char *l = "COMPILED ON: "__DATE__", "__TIME__;
    fprintf(WIN,l);
    i = 78-strlen(l); while(i>0) {--i; fputc(' ',WIN);}
    fputc('\n',WIN);
    }
/*    SLINE(); */
}

static void TEOLine(char num, char car, char *item)
{
    int i;
    fprintf(WIN,"  "); ZONE(1);FORME(NOIR);
    fprintf(WIN," %c %s", num, item);
    i = 51 - strlen(item);
    while(i>0) {--i; fputc(' ',WIN);}
    if(car) {
	ZONE(1); FORME(NOIR);
	fprintf(WIN,car==27?" ESC ":" %c   ",car);
    }
    fprintf(WIN,"\n");
}

/***************************************************************************/

#define WINTITLE VERSION", Amiga port by Samuel Devulder"

static int open_cnt;

#if defined(REG_MAPPER)&&defined(FindTask)
#undef FindTask
#endif

static void OpenWIN(void)
{
    ULONG win_ptr;
    struct Process *pr;
    
    ++open_cnt; if(WIN) return;
    
    /* on evite les requesters */
    pr = (void*)FindTask(NULL);
    win_ptr = (ULONG)pr->pr_WindowPtr;
    pr->pr_WindowPtr = (APTR)-1;
    
    /* on essaye KingKon en 1er car on peut specifier la fonte */
    WIN = fopen("KRAW:0/0/640/256/"WINTITLE"/FONT topaz.8/PLAIN","r+");
    if(!WIN) 
        WIN = fopen("KRAW:0/0/640/200/"WINTITLE"/FONT topaz.8/PLAIN","r+");
#ifdef AMIGA
    {static int warn; if(!WIN && !warn) fprintf(stderr,
    "TEO suggests you to mount KRAW: to improve the ascii GUI\n"), warn=1;}
#endif
    if(!WIN) 
        WIN = fopen("RAW:0/0/640/256/"WINTITLE,"r+");
    if(!WIN) 
        WIN = fopen("RAW:0/0/640/200/"WINTITLE,"r+");
    CURSOR(0); WRAP(0);

    if(WIN) ShowScreen(0); else perror("Can't open window");
    
    pr->pr_WindowPtr = (APTR)win_ptr;
}

static void CloseWIN(void)
{
    if(--open_cnt) return;
    ShowScreen(1);
    fclose(WIN);
    WIN = NULL;
    ShowScreen(1);
}

/***************************************************************************/

static int local_getch(void)
{
    int c;
    do c = fgetc(WIN); while(c=='\r');
    return c;
}

/***************************************************************************/

static int match_name(char *motif, char *txt)
{
    while(*motif && tolower(*motif)==tolower(*txt)) {
        ++motif;
        ++txt;
    }
    return (0==*motif);
}

/***************************************************************************/

static void pad_name(char *name, char dir, int len)
{
    fprintf(WIN,"%s%c ",name,dir);
    len -= strlen(name)+2;  
    if(len>4) fprintf(WIN,"\033[%dC",len);
    else while(len>0) {fputc(' ',WIN);--len;}
}

/***************************************************************************/

static int is_dir(const char *dir, const char *nam)
{
    char filename[MAXNAMLEN];
    struct stat st_buf;
    
    strncpy(filename, dir, MAXNAMLEN);
    strncat(filename, nam, MAXNAMLEN);
    stat(filename, &st_buf);        
    return (st_buf.st_mode&S_IFMT)==S_IFDIR;
}

/***************************************************************************/

/* routine de file-completion */
static int complete(char *name, int len)
{
    DIR *dir;
    struct dirent *de;
    static  char dirname[MAXNAMLEN]; /* static pour economiser la pile */
    static  char bestmatch[MAXNAMLEN];
    int nb_match;
    int name_idx = 0;
    int col_size = 6;
    int col_num  = 0;
    
    len--; /* pour tenir compte de '\0' */
    {   
        int i;
        strncpy(dirname, name, len);
        for(i = 0; (i<len) && name[i]; ++i) {
           if(name[i]==':' || name[i]=='/') {
              name_idx = i+1;
           }
        }
        dirname[name_idx] = 0;
    }
    
#ifdef AMIGA
    dir = opendir(dirname); 
#else /* unix */
    dir = opendir(*dirname?dirname:".");
#endif
    if(!dir) {BEEP(); return 0;}
    
    /* 1ere lecture => compte des fichiers matchants,
       calcul de taille colonne,
       calcul du motif le plus grand */
    bestmatch[0] = '\0';
    nb_match = 0;
    while((de = readdir(dir))) if(match_name(name+name_idx,de->d_name)) {
        int s = strlen(de->d_name);
        if(s > col_size) col_size = s;
        ++nb_match;
        if(nb_match == 1) { /* 1er match */
            for(s=0;(s < MAXNAMLEN-1) &&
                (bestmatch[s] = de->d_name[s]); ++s);
            if((s<MAXNAMLEN-1) && is_dir(dirname, de->d_name))
                bestmatch[s++] = '/';
            bestmatch[s] = '\0';
        } else {
            for(s=0; (s<MAXNAMLEN-1) && bestmatch[s] && 
                de->d_name[s] && 
                (tolower(bestmatch[s]) == tolower(de->d_name[s]));
                ++s);
            bestmatch[s] = '\0';
        }
    }
    col_size += 2;
    col_num = 77/col_size;
    
    rewinddir(dir);
    if(nb_match==0) { /* pas de matchs => beep */
        BEEP();
    } else { /* plusieurs matchs, on affiche les choix */
        int i;
        
        for(i = 0; (name_idx + i<len) && 
            (name[name_idx + i] = bestmatch[i]); ++i);
        name[name_idx + i] = '\0';
        if(nb_match>1) {
            fprintf(WIN,"\n");
            i = 0;
            while((de = readdir(dir))) 
                if(match_name(name+name_idx,de->d_name)) {
                    pad_name(de->d_name, 
                             is_dir(dirname, de->d_name)?
                             '/':' ',
                             col_size);
                    if(++i == col_num) fprintf(WIN,"\n"),i=0;
                }
            if(i) fprintf(WIN,"\n");
        }
    }
    closedir(dir);
    return nb_match;
}

/***************************************************************************/

static int GetFile(const char *prompt, char *nam, int len)
{
    int pos, i, slen;
    char *name;
    
    name = malloc(len+1);
    if(!name) return 0; /* pb */
    strcpy(name,nam);

    FOND(GRIS);FORME(NOIR);CURSOR(1);WRAP(1);
    
    fprintf(WIN,"\n%s: %s",prompt,name); fflush(WIN);
    
    pos = slen = strlen(name);
    
    while(1) {
        int c = fgetc(WIN);
        switch(c) {
          case 12: /* ctrl-l => refresh */
            fprintf(WIN,"\f%s: %s",prompt,name);
            i = slen-pos;
            if(i) fprintf(WIN,"\033[%dD",i);
            break;
            
          case 27: /* ESC => return sans rien */
            free(name);
	    CURSOR(0);WRAP(0);
            return 0;
            
          case '\r': case '\n':  /* return */
            fprintf(WIN,"\n");
            fflush(WIN);
            strcpy(nam, name);
            free(name);
	    CURSOR(0);WRAP(0);
            return 1;
            
          case '\b': /* backspace */
            if(pos>0) {
                fprintf(WIN,"\b%s ",name+pos);
                --pos;
                for(i=0; (name[pos+i] = name[pos+i+1]); ++i);
                fprintf(WIN,"\033[%dD",i+1);
                --slen;
            } else  BEEP();
            break;
            
          case 11: /* ctrl-k : coupage fin-ligne */
            for(i=0; name[pos+i]; ++i) fputc(' ',WIN);
            if(i) fprintf(WIN,"\033[%dD",i);
            name[pos] = '\0';
            slen = pos;
            break;
            
          case 127: /* delete */
            if(name[pos]) {
                fprintf(WIN,"%s ",name+pos+1);
                for(i = 0; (name[pos+i] = name[pos+i+1]); ++i);
                fprintf(WIN,"\033[%dD",i+1);
                --slen;
            } else  BEEP();
            break;
            
          case 0x9b: /* deplacement curseur */
            switch(fgetc(WIN)) {
              case 'C': /* -> */
                if(pos<slen) fprintf(WIN,"\033[C"), ++pos;
                break;
                
              case 'D': /* <- */
                if(pos>0) fprintf(WIN,"\033[D"), --pos;
                break;
                
              case 'Z': /* shift + tab */
                if(pos<=0) break;
                fprintf(WIN,"\033[%dD",pos);
                do --pos; 
                while(pos>0 && 
                      name[pos-1]!='/' && name[pos-1]!=':');
                if(pos) fprintf(WIN,"\033[%dC",pos);
                break;
                
              case ' ': /* shift + ... */
                switch(fgetc(WIN)) {
                  case 'A': /* shift + <- */ 
                    if(pos) fprintf(WIN,"\033[%dD",pos);
                    pos = 0;
                    break;
                    
                  case '@': /* shift + -> */
                    fprintf(WIN,"%s",name+pos);
                    pos = slen;
                    break;
                }
                break;
                
            }
            break;
            
          case '\t': /* tab = completion */
            for(i=0; name[pos+i]; ++i) fputc(' ',WIN);
            if(i) fprintf(WIN,"\033[%dD",i);
            name[pos] = '\0';
            i = complete(name, len);
            if(i == 1) {
                if(pos) fprintf(WIN,"\033[%dD",pos);
                fprintf(WIN,"%s",name);
            } else if(i>1) {
                fprintf(WIN,"\n%s: %s",prompt,name);
            }
            pos = slen = strlen(name);                      
            break;
            
          default: /* ajout lettre */
            if((c>=' ') && (slen < len-1)) {
                for(i = pos; name[i]; ++i);
                while(i>=pos) {name[i+1] = name[i]; --i;}
                name[pos] = c;
                fprintf(WIN,"%s",name+pos);
                ++slen; ++pos;
                i = slen - pos;
                if(i) fprintf(WIN,"\033[%dD",i);
            } else  BEEP();
            break;
        }
        fflush(WIN);
    }
}

/***************************************************************************/

int menuk7(void)
{
    FILE *f;
    
    do {
	TEOTitle("MENU K7");
	HLINE2();
	TEOLine('0',  27, "RETOUR");
	HLINE2();
	TEOLine('1', 'L', "CHOIX K7 LECTURE");
	TEOLine('2', 'R', "REMBOBINER K7");
	HLINE();
	CLEOD();
        fflush(WIN);
        switch(local_getch()) {
	case '0':
	case EOF: case 0:
	case 27: 
	    return 0; 
	    break;
	case '1':
	case 'L': case 'l': 
            if(GetFile("Entrez le nom d'une K7 en lecture", cass7name, SIZE)) {
                if((f=fopen(cass7name,"r"))) {
                    if(K7in) fclose(K7in);
                    K7in = f;
                    return 1;
                } else perror(cass7name);
            }
            break;            
	case '2':
	case 'R': case 'r':
            if(K7in) {fseek(K7in,0,SEEK_SET); return 1;}
            break;
	default: BEEP();fflush(WIN); break;
        }
    } while(1);
}

/***************************************************************************/

int menudisk(void)
{
    do {
	TEOTitle("MENU D7");
	HLINE2();
	TEOLine('0',27, "RETOUR");
	HLINE2();
	TEOLine('1','A', "DISK 0 (INTERNE FACE INF)");
	TEOLine('2','B', "DISK 1 (INTERNE FACE SUP)");
	HLINE();
	CLEOD();
        fflush(WIN);
        switch(local_getch()) {
	case '1': case 'A': case 'a':
            if(GetFile("Entrez le nom du DISK 0", disk0name, SIZE)) {
		LoadSAP(disk0name, 0); 
		return 1;
	    }
            break;
        case '2': case 'B': case 'b':
            if(GetFile("Entrez le nom du DISK 1", disk1name, SIZE)) {
                LoadSAP(disk1name, 1); 
		return 1; 
	    }
            break;
	case '0': case EOF: 
	case 27: 
	    return 0; 
	default: BEEP();fflush(WIN); break;
        }
    } while(1);
}

/***************************************************************************/

int menumemo7(void)
{
    TEOTitle("MENU MEMO7");
    HLINE();
    CLEOD();
    if(GetFile("Entrez le nom d'une cartouche MEMO7", memo7name, SIZE)) {
        FILE *f;
        if((f=fopen(memo7name,"r"))) {
            int i;
            for (i=0x0;i<0x2000;i++) {
                EXT[i]=fgetc(f)&0xFF;
            }
            fclose(f);
            return 1;
        } else  perror(memo7name);
    }
    return 0;
}

/***************************************************************************/

static int menu_gfx(void)
{
    do {
    	char buf[80];
	TEOTitle("MENU OPTIONS GRAPHIQUES");
	HLINE2();
	TEOLine('0', 27,  "RETOUR");
	HLINE2();
	TEOLine('1', 'G', do_gamma?"CORRECTION GAMMA: ON":"CORRECTION GAMMA: OFF");
	TEOLine('2', 'U', use_os?"UTILISATION OS: ON":"UTILISATION OS: OFF");
	TEOLine('3', 'F', do_fastgfx?"GRAPHISME: FAST":"GRAPHISME: SLOW");
	sprintf(buf, "HIRES MODE: $%08x", MODE80_ID);
	TEOLine('4', 'H', buf);
	sprintf(buf, "LORES MODE: $%08x", MODE40_ID);
	TEOLine('5', 'L', buf);
	HLINE();
	CLEOD();
        fflush(WIN);
        switch(local_getch()) {
	case '0': case 27:
	    return 0;
	    break;
	case '1': case 'G': case 'g': 
            do_gamma = do_gamma?0:1; 
            SetGraphicMode(INIT); 
	    ShowScreen(0);
            break;
	case '2': case 'U': case 'u': 
            use_os = use_os?0:1; 
            SetGraphicMode(INIT);
	    ShowScreen(0);
            break;
        case '3': case 'F': case 'f':
            do_fastgfx = do_fastgfx?0:1;
            SetGraphicMode(INIT);
	    ShowScreen(0);
            break;
        case '4': case 'H': case 'h':
            MODE80_ID = -1; GetModeIDs();
            SetGraphicMode(INIT);
	    ShowScreen(0);
            break;
        case '5': case 'L': case 'l':
            MODE40_ID = -1; GetModeIDs();
            SetGraphicMode(INIT);
	    ShowScreen(0);
            break;
	default: BEEP(); break;
        }
    } while(1);
}

void menu3(void)
{
    OpenWIN();
    menu_gfx();
    CloseWIN();
}

/***************************************************************************/

static int menu_ext(void)
{
    do {
	TEOTitle("MENU OPTIONS AVANCEES");
	HLINE2();
	TEOLine('0', 27,  "RETOUR");
	HLINE2();
	TEOLine('1', 'R', "REINITIALISER LE TO8");
	TEOLine('2', 'H', "RESET HARD");
	HLINE2();
	TEOLine('3', 'V', exact?"VITESSE: EXACTE":"VITESSE: A DONF");
	if(sound_ok);
	TEOLine('4', 'S', thomsound?"SON: ON":"SON: OFF");
	TEOLine('5', 'G', "MENU GRAPHIQUE");
	HLINE();
	CLEOD();
        fflush(WIN);
        switch(local_getch()) {
	case '0': case 27:
	    return 0;
	    break;
	case '1': case 'R': case 'r': 
            PC=(LOAD8(0xFFFE)<<8)|LOAD8(0xFFFF); 
            return 1; 
	case '2': case 'H': case 'h': 
            ColdReset(); 
            return 1;
	case '3': case 'E': case 'e': case 'V': case 'v': 
            exact = exact?0:-1; 
            break;
	case '4': case 'S': case 's': 
            if(sound_ok) thomsound = thomsound?0:-1;
            if(!thomsound) CLEAR_SOUND();
	    break;
	case '5': case 'G': case 'g': 
	    return menu_gfx();
	    break;
	case 'P': case 'p':
            CLEAR_SOUND();
            thomsound = 0;
            break;
	default: BEEP(); break;
        }
    } while(1);
}

void menu2(void)
{
    OpenWIN();
    menu_ext();
    CloseWIN();
}

/***************************************************************************/

#if 0 /*def __GNUC__*/
#define MINSTACK 32000
int __stack = MINSTACK;        /* ixemul */
int __stk_safezone = MINSTACK; /* libnix */
__stackext
#endif
void menu(void)
{
    int ok = 0;
    
    OpenWIN();
    
    do {
	TEOTitle("MENU PRINCIPAL");
	HLINE2();
	TEOLine('0', 27,  "RETOUR A TEO");
	TEOLine('1', 'Q', "QUITTER TEO");
	HLINE2();
	TEOLine('2', 'D', "DISQUETTES");
	TEOLine('3', 'K', "CASSETTE");
	TEOLine('4', 'M', "MEMO7");
	TEOLine('5', 'O', "OPTIONS & RESET");
	TEOLine('6', 'G', "OPTIONS GRAPHIQUES");
	HLINE();
	CLEOD();
        fflush(WIN);
        switch(local_getch()) {
	case '0': case 27:  
	    ok = 1; 
	    break;
	case EOF: case 0:
	case '1': case 'Q': case 'q': 
	    fclose(K7in);
	    CloseWIN();
	    normal_end();
	    break;
	case '2': case 'D': case 'd': 
	    menudisk(); 
	    break;
	case '3': case 'K': case 'k': 
	    menuk7(); 
	    break;
	case '4': case 'M': case 'm': 
	    menumemo7(); 
	    break;
	case '5': case 'O': case 'o': 
	    ok = menu_ext(); 
	    break;
	case '6': case 'G': case 'g':
	    ok = menu_gfx();
	    break;
	default: BEEP(); break;
        }
    } while(!ok);
    
    CloseWIN();
}
