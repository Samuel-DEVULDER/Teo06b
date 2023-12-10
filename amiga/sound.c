/*
 * amiga/sound.c 
 *
 * Gestion du son pour l'emul Teo de Gilles Fétis
 *
 * Créé par Samuel Devulder, 08/98.
 */

#include "flags.h"

/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <devices/audio.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>

#ifdef __GNUC__
#include <proto/alib.h>
#endif
#include <proto/exec.h>

/***************************************************************************/

#include "mapper.h"
#include "sound.h"

/*
note:  Il y a deux modes de sortie audio, chacun occupant une IORequest.

Le 1er mode est le mode 'exact speed', ou un sample est calibre pour
faire 1/20 secondes et dans lequel l'emul ecrit en fonction du nombre
de cycles d'horloge emule. Ce buffer permet de plus d'attendre la 
synchro sur la fin de sample, et donc limite l'emul a 1Mhz au plus.

Dans le 2nd mode, les acces aux registres audio TO8 par l'emul sont
directement transmises aux registres audio amiga (au travers d'un buffer
rikiki qui boucle en permanence). Dans ce mode, il n'y a pas de
synchro et l'emul va a sa vitesse Maxi.

Pour les machines lentes, le 2eme mode est preferable (moins
gourmant en resources).
*/

/***************************************************************************/

extern int exact;

int thomsound;
int sound_ok;

#define AUDIOSIZE ((FREQ/50)&~1)

int soundbufsize=AUDIOSIZE;
static UBYTE samp_buf[AUDIOSIZE];

static UWORD *direct_buffer;
static UBYTE *sample_buffer;

static UBYTE whichchannel[]={1,2,4,8};
static struct IOAudio *IOdirect; /* direct audio */
static struct IOAudio *IOsample; /* sample */
static struct MsgPort *AudioMP;
static UBYTE  devopen, direct_ok, sample_ok;

/***************************************************************************/
/* 
 * note: il n'y a pas a dire.. C'est vraiment tres complique l'audio
 * respectant le systeme sur amiga :-/ 
 */

/*
 * Ici on alloue un canal (sans le locker)
 */
static int AllocChannel(struct IOAudio *IO);
/* deplace a la fin pour eviter un pb avec les spilled registers */

/*
 * Calcule la bonne base d'horloge suivant le type de machine
 */
static int clockval(void)
{
   struct GfxBase *GB;
   int val;
   
   GB = (void*)OpenLibrary("graphics.library",0L);
   if (!GB) return 0;
   if (GB->DisplayFlags & PAL)
       val = 3546895;        /* PAL clock */
   else
       val = 3579545;        /* NTSC clock */
   CloseLibrary((void*)GB);
   return val;
}

/*
 * On fait le menage avant de sortir
 */
static void exit_thom_sound(void)
{
    if(devopen)  {CloseDevice((void*)IOdirect);devopen = 0;}
    /* en principe le close device, nous a aussi annule IOsample car
       cette requette IO a la meme clef que IOdirect */
    if(IOsample) {DeleteIORequest((void*)IOsample);IOsample = NULL;}
    if(IOdirect) {DeleteIORequest((void*)IOdirect);IOdirect = NULL;}
    if(AudioMP)  {DeleteMsgPort((void*)AudioMP);AudioMP  = NULL;}
    if(direct_buffer) {FreeMem((APTR)direct_buffer,2);direct_buffer = NULL;}
    if(sample_buffer) {FreeMem((APTR)sample_buffer,soundbufsize);sample_buffer = NULL;}
}

/*
 * On initialise le son (c'est pas simple).
 */
void init_thom_sound(void)
{
    exact = sound_ok = thomsound = 0;
    
    atexit(exit_thom_sound);
    
    /* 1er: on alloue un port de reponse */
    AudioMP  = (void*)CreateMsgPort(); 
    if(!AudioMP) goto err;

    /* 2eme: on alloue des IO Requests */
    IOdirect = (void *)CreateIORequest(AudioMP,sizeof(struct IOAudio)); 
    IOsample = (void *)CreateIORequest(AudioMP,sizeof(struct IOAudio)); 
    if(!IOsample || !IOdirect) goto err;
    
    /* 3eme: Ouverture du device sans allocation de canaux */
    devopen = 0;
    IOdirect->ioa_Length   = 0; /* pas de generation de clef */
    if(OpenDevice(AUDIONAME, 0, (void*)IOdirect, 0)) goto err;
    devopen = 1;

    /* 4eme: Allocation canal direct-audio */
    IOdirect->ioa_AllocKey = 0; /* on force la generation d'une clef */
    if(!AllocChannel(IOdirect)) goto err;

    /* 5eme: Allocation canal sample */
    *IOsample = *IOdirect; /* on recopie la clef et autre trucs (msgport) */
    if(!AllocChannel(IOsample)) goto err;

    /* 6eme: Mise en place du mode direct-audio, alloc buffer chipram */
    direct_buffer = (void *)AllocMem(2,MEMF_CHIP|MEMF_CLEAR);
    if(!direct_buffer) goto err;

    /* 7eme: Lancement de la requete IO avec le canal et le buffer chipram */
    IOdirect->ioa_Request.io_Command = CMD_WRITE;
    IOdirect->ioa_Request.io_Flags   = ADIOF_PERVOL|IOF_QUICK;
    IOdirect->ioa_Data               = (void*)direct_buffer;
    IOdirect->ioa_Length             = 2;
    IOdirect->ioa_Period             = 500;
    IOdirect->ioa_Volume             = 64;
    IOdirect->ioa_Cycles             = 0;
    BeginIO((void*)IOdirect);
    direct_ok = 1;
    
    /* 8eme: Mise en place du mode sample, alloc memoire chip */
    soundbufsize &= ~1; /* doit etre pair */
    sample_buffer = (void *)AllocMem(soundbufsize,MEMF_CHIP|MEMF_CLEAR);
    if(!sample_buffer) goto err;

    /* 9eme: Initialisation de la 2nd requete audio (sample) */
    IOsample->ioa_Request.io_Command = CMD_WRITE;
    IOsample->ioa_Request.io_Flags   = ADIOF_PERVOL|IOF_QUICK;
    IOsample->ioa_Data               = (void*)sample_buffer;
    IOsample->ioa_Length             = soundbufsize;
    IOsample->ioa_Period             = clockval()/FREQ;
    IOsample->ioa_Volume             = 64;
    IOsample->ioa_Cycles             = 0;
    if(!IOsample->ioa_Period) goto err;
    BeginIO((void*)IOsample);
    sample_ok = 1;
    
    /* 10eme: Hehe il aura bien fallu 10 etapes et chacune ayant la 
       possibilite d'echouer */
    sound_ok  = 1;
    thomsound = 1;
    return;
err:
    printf("Désolé, sortie son indisponible!\n");
}

/*
 * Routine appelle dans mapper.c:hardware() pour sortir un son
 */
void REGPARM output_sound(int value)
{
    if(!sound_ok) return;
    
    /* mode direct: on envoit le sample aux registres amiga */
    if(!exact) {*direct_buffer = value|(value<<8); return;}
    
    /* mode sample: on synchronise sur le cycle cpu emule */
    /* le msb est positionne signifiant a CALC_SOUND() que cette */
    /* valeur a ete positionnee dans l'emul */
    samp_buf[((cl*(FREQ/50))/20000) % AUDIOSIZE] = value|0x80;
             /*   ^^^^^^^^^^^^^^^^ */
             /* normallement la formule synchronise un cpu avec */
             /* des cycles a 1Mhz decoupes en paquets de 1/50   */
             /* secondes, sur une sortie audio a FREQ Hz        */
}

/* 
 * Comble les trous entre les echantillons (un peu a la facon du
 * HoldAndModify).
 */
void CALC_SOUND(void) 
{
    static UBYTE lastval;
    int i;
    
    if(!sound_ok || !exact || !thomsound) return;
    
    if(samp_buf[0] & 0x80) samp_buf[0] &= 0x7F;
    else samp_buf[0] = lastval;
    for(i=1;i<AUDIOSIZE;++i) {
	if(samp_buf[i]&0x80) samp_buf[i] &= 0x7F; /* modify */
	else  samp_buf[i] = samp_buf[i-1];        /* hold */
    }
    lastval = samp_buf[i-1];
}

/*
 * Attend la fin du cycle courant, puis recopie le sample
 */
void SWAP_SOUND(void) 
{
    short i;

    if(!sound_ok || !exact) return;
    
    /* Attente de fin du cycle */
    IOsample->ioa_Request.io_Command = ADCMD_WAITCYCLE;
    IOsample->ioa_Request.io_Flags   = 0;
    DoIO((void*)IOsample);
    
    /* recopie a toute vitesse */
    for(i=AUDIOSIZE;i--;) sample_buffer[i] = samp_buf[i];
}

/*
 * Mise a zero de la sortie audio-sample
 */
void CLEAR_SOUND(void) 
{
    short i;
    if(!sound_ok) return;
    *direct_buffer = 0;
    for(i=AUDIOSIZE;i--;) sample_buffer[i] = 0;
}

/*
 * Ici on alloue un canal (sans le locker)
 */
#if (defined(REG_MAPPER)||defined(SMALL_MODEL))&&defined(DoIO)
#undef DoIO
#endif
static int AllocChannel(struct IOAudio *IO)
{
    IO->ioa_Request.io_Command = ADCMD_ALLOCATE;
    IO->ioa_Request.io_Flags   = 0;
    IO->ioa_Request.io_Message.mn_Node.ln_Pri = 127;
    IO->ioa_Data               = whichchannel;
    IO->ioa_Length             = sizeof(whichchannel);
    DoIO((void*)IO);
    if(IO->ioa_Request.io_Error) return 0;

    return 1;
}
