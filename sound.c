/*

	VERSION     : 2.0
	MODULE      : sound.c
	Cree par    : Gilles FETIS
	Modifie par :

*/

#include <allegro.h>
#include "sound.h"

int thomsound;
int * BSTORE;

int soundbufsize=FREQ/50+50;
static int B1[FREQ/50+50];
static SAMPLE spl;
static char spdata[((FREQ/50+50)*2)*4];
static int voice;
static int buf=0;


void init_thom_sound(void)
{
  int i;

  for (i=0;i<(FREQ/50)+50;i++) B1[i]=0x01;
  for (i=0;i<((FREQ/50+50)*2)*4;i++) spdata[i]=127;
  BSTORE=&B1[0];

  if (install_sound(DIGI_AUTODETECT,MIDI_NONE,NULL)==0)
  {
   spl.bits=8;
   spl.data=spdata;
   spl.freq=FREQ;
   spl.priority=255;
   spl.len=((FREQ/50)*2)*4;
   spl.loop_start=0;
   spl.loop_end=spl.len;
   spl.param=0;
   lock_sample(&spl);

   voice=allocate_voice(&spl);
   voice_set_frequency(voice, FREQ);
   voice_set_playmode(voice, PLAYMODE_LOOP);
   voice_start(voice);
   thomsound=-1;
  }
}


void CALC_SOUND(void)
{
  int i,j;
  static int S1B[8];
  static int W1B=0,V1B=0;
  static int last1B=0;
  static int S6B[4];
  static int W6B=0,V6B=0;
  static int last6B=0;

  int v;


  if (thomsound)
  {

    buf=(buf+1)%8;
    /* calcul du son avec filtrage different pour chaque generateur */
    /* filtrage passe bas faible pour le gene 6bits                 */
    /* filtrage passe haut fort pour le gene 1bit                   */
    /* +desactvation de chaque gene au bout de 4 sec d'inactivite   */
    for (i=0;i<(FREQ/50);i++)
    {
      W1B=(W1B+1) & 0x7;
      W6B=(W6B+1) & 0x1;
	   if (BSTORE[i] == 0x8100) {V1B=31;last1B=0;}
	   else
	   if (BSTORE[i] == 0x8000) {V1B=0;last1B=0;}
	   else
      if (BSTORE[i]!=0xFFFF) {V6B=((BSTORE[i]) &0x3f);last6B=0;}

      S1B[W1B]=V1B;S6B[W6B]=V6B;


      v=0;
	
      if (last1B<50*4) for (j=0;j<8;j++) v=v+S1B[j];
      else v+=15*8;

      if (last6B<50*4) for (j=0;j<2;j++) v=v+S6B[j];
      else v+=31*2;

 	spdata[i+buf*(FREQ/50)]=v>>2;
	if (last1B<50*4) last1B++;
	if (last6B<50*4) last6B++;
      BSTORE[i]=0xFFFF;
    }
  }
  else
  {
    buf=(buf+1)%8;
    for (i=0;i<(FREQ/50);i++)
    {
	     spdata[i+buf*(FREQ/50)]=0;
        BSTORE[i]=0xFFFF;
    }
  }
}

void SWAP_SOUND(void)
{
  if (thomsound)
  {
    if (buf == 0)
        voice_set_position(voice,4*(FREQ/50));
    if (buf == 4)
       voice_set_position(voice,0*(FREQ/50));
  }
}

void CLEAR_SOUND(void)
{
   int i;
   buf=(buf+1)%8;
   for (i=0;i<((FREQ/50+50)*2)*4;i++) spdata[i]=127;

   for (i=0;i<FREQ/50+50;i++) BSTORE[i]=0xFFFF;

   /* attendre qu'allegro genere une interrupt de son */
   rest(300);
}
