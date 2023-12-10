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
	MODULE      : setup.c
	Créé par    : Gilles FETIS
	Modifié par :

	Création de la table de fonction des instructions du 6809

*/

#include "flags.h"

#include <stdio.h>
#include <string.h>

#include "mapper.h"
#include "regs.h"
#include "mc6809.h"
#include "index.h"

void setup_6809(void)
{
  int l,i;
  cl = 0;
  res=m1=m2=sign=ovfl=ccrest=0;
  for(i=0;i<16;i++) exreg[i]=NULL;
#ifndef REG_X
  exreg[1]=&X;
#endif
  exreg[2]=&Y;
  exreg[3]=&U;
  exreg[4]=&S;
#ifndef REG_PC
  exreg[5]=&PC;
#endif
  exreg[8]=&A;
  exreg[9]=&B;
  exreg[11]=&DP;


  for (l=0;l<256;l++)
  {
   PFONCT[l]=unknown;
   PFONCTS[l]=unknown;
   PFONCTSS[l]=unknown;
  }
  /* Code 10xx */
  PFONCT[0x10]=CODE10xx;

  /* Code 11xx */
  PFONCT[0x11]=CODE11xx;

  /* LDA opcode */
  PFONCT[0x86]=LDAi;
  PFONCT[0x96]=LDAd;
  PFONCT[0xB6]=LDAe;
  PFONCT[0xA6]=LDAx;

  /* LDB opcode */
  PFONCT[0xC6]=LDBi;
  PFONCT[0xD6]=LDBd;
  PFONCT[0xF6]=LDBe;
  PFONCT[0xE6]=LDBx;

  /* LDD opcode */
  PFONCT[0xCC]=LDDi;
  PFONCT[0xDC]=LDDd;
  PFONCT[0xFC]=LDDe;
  PFONCT[0xEC]=LDDx;

  /* LDU opcode */
  PFONCT[0xCE]=LDUi;
  PFONCT[0xDE]=LDUd;
  PFONCT[0xFE]=LDUe;
  PFONCT[0xEE]=LDUx;

  /* LDX opcode */
  PFONCT[0x8E]=LDXi;
  PFONCT[0x9E]=LDXd;
  PFONCT[0xBE]=LDXe;
  PFONCT[0xAE]=LDXx;

  /* LDS opcode */
  PFONCTS[0xCE]=LDSi;
  PFONCTS[0xDE]=LDSd;
  PFONCTS[0xFE]=LDSe;
  PFONCTS[0xEE]=LDSx;

  /* LDY opcode */
  PFONCTS[0x8E]=LDYi;
  PFONCTS[0x9E]=LDYd;
  PFONCTS[0xBE]=LDYe;
  PFONCTS[0xAE]=LDYx;

  /* STA opcode */
  PFONCT[0x97]=STAd;
  PFONCT[0xB7]=STAe;
  PFONCT[0xA7]=STAx;

  /* STB opcode */
  PFONCT[0xD7]=STBd;
  PFONCT[0xF7]=STBe;
  PFONCT[0xE7]=STBx;

  /* STD opcode */
  PFONCT[0xDD]=STDd;
  PFONCT[0xFD]=STDe;
  PFONCT[0xED]=STDx;

  /* STS opcode */
  PFONCTS[0xDF]=STSd;
  PFONCTS[0xFF]=STSe;
  PFONCTS[0xEF]=STSx;

  /* STU opcode */
  PFONCT[0xDF]=STUd;
  PFONCT[0xFF]=STUe;
  PFONCT[0xEF]=STUx;

  /* STX opcode */
  PFONCT[0x9F]=STXd;
  PFONCT[0xBF]=STXe;
  PFONCT[0xAF]=STXx;

  /* STY opcode */
  PFONCTS[0x9F]=STYd;
  PFONCTS[0xBF]=STYe;
  PFONCTS[0xAF]=STYx;

  /* LEA opcode */
  PFONCT[0x32]=LEASx;
  PFONCT[0x33]=LEAUx;
  PFONCT[0x30]=LEAXx;
  PFONCT[0x31]=LEAYx;

  /* CLR opcode */
  PFONCT[0x0F]=CLRd;
  PFONCT[0x7F]=CLRe;
  PFONCT[0x6F]=CLRx;
  PFONCT[0x4F]=CLRA;
  PFONCT[0x5F]=CLRB;

  /* EXG */
  PFONCT[0x1E]=EXG;

  /* TFR */
  PFONCT[0x1F]=TFR;

  /* PSH */
  PFONCT[0x34]=PSHS;
  PFONCT[0x36]=PSHU;

  /* PUL */
  PFONCT[0x35]=PULS;
  PFONCT[0x37]=PULU;

  /* INC */
  PFONCT[0x4C]=INCA;
  PFONCT[0x5C]=INCB;
  PFONCT[0x7C]=INCe;
  PFONCT[0x0C]=INCd;
  PFONCT[0x6C]=INCx;

  /* DEC */
  PFONCT[0x4A]=DECA;
  PFONCT[0x5A]=DECB;
  PFONCT[0x7A]=DECe;
  PFONCT[0x0A]=DECd;
  PFONCT[0x6A]=DECx;

  /* BIT */
  PFONCT[0x85]=BITAi;
  PFONCT[0x95]=BITAd;
  PFONCT[0xB5]=BITAe;
  PFONCT[0xA5]=BITAx;
  PFONCT[0xC5]=BITBi;
  PFONCT[0xD5]=BITBd;
  PFONCT[0xF5]=BITBe;
  PFONCT[0xE5]=BITBx;

  /* CMP */
  PFONCT[0x81]=CMPAi;
  PFONCT[0x91]=CMPAd;
  PFONCT[0xB1]=CMPAe;
  PFONCT[0xA1]=CMPAx;
  PFONCT[0xC1]=CMPBi;
  PFONCT[0xD1]=CMPBd;
  PFONCT[0xF1]=CMPBe;
  PFONCT[0xE1]=CMPBx;
  PFONCTS[0x83]=CMPDi;
  PFONCTS[0x93]=CMPDd;
  PFONCTS[0xB3]=CMPDe;
  PFONCTS[0xA3]=CMPDx;
  PFONCTSS[0x8C]=CMPSi;
  PFONCTSS[0x9C]=CMPSd;
  PFONCTSS[0xBC]=CMPSe;
  PFONCTSS[0xAC]=CMPSx;
  PFONCTSS[0x83]=CMPUi;
  PFONCTSS[0x93]=CMPUd;
  PFONCTSS[0xB3]=CMPUe;
  PFONCTSS[0xA3]=CMPUx;
  PFONCT[0x8C]=CMPXi;
  PFONCT[0x9C]=CMPXd;
  PFONCT[0xBC]=CMPXe;
  PFONCT[0xAC]=CMPXx;
  PFONCTS[0x8C]=CMPYi;
  PFONCTS[0x9C]=CMPYd;
  PFONCTS[0xBC]=CMPYe;
  PFONCTS[0xAC]=CMPYx;

  /* TST */
  PFONCT[0x4D]=TSTAi;
  PFONCT[0x5D]=TSTBi;
  PFONCT[0x0D]=TSTd;
  PFONCT[0x7D]=TSTe;
  PFONCT[0x6D]=TSTx;

  /* AND */
  PFONCT[0x84]=ANDAi;
  PFONCT[0x94]=ANDAd;
  PFONCT[0xB4]=ANDAe;
  PFONCT[0xA4]=ANDAx;
  PFONCT[0xC4]=ANDBi;
  PFONCT[0xD4]=ANDBd;
  PFONCT[0xF4]=ANDBe;
  PFONCT[0xE4]=ANDBx;
  PFONCT[0x1C]=ANDCCi;

  /* OR */
  PFONCT[0x8A]=ORAi;
  PFONCT[0x9A]=ORAd;
  PFONCT[0xBA]=ORAe;
  PFONCT[0xAA]=ORAx;
  PFONCT[0xCA]=ORBi;
  PFONCT[0xDA]=ORBd;
  PFONCT[0xFA]=ORBe;
  PFONCT[0xEA]=ORBx;
  PFONCT[0x1A]=ORCCi;

  /* EOR */
  PFONCT[0x88]=EORAi;
  PFONCT[0x98]=EORAd;
  PFONCT[0xB8]=EORAe;
  PFONCT[0xA8]=EORAx;
  PFONCT[0xC8]=EORBi;
  PFONCT[0xD8]=EORBd;
  PFONCT[0xF8]=EORBe;
  PFONCT[0xE8]=EORBx;

  /* COM */
  PFONCT[0x03]=COMd;
  PFONCT[0x73]=COMe;
  PFONCT[0x63]=COMx;
  PFONCT[0x43]=COMAi;
  PFONCT[0x53]=COMBi;

  /* NEG */
  PFONCT[0x00]=NEGd;
  PFONCT[0x70]=NEGe;
  PFONCT[0x60]=NEGx;
  PFONCT[0x40]=NEGAi;
  PFONCT[0x50]=NEGBi;

  /* ABX */
  PFONCT[0x3A]=ABXi;

  /* ADC */
  PFONCT[0x89]=ADCAi;
  PFONCT[0x99]=ADCAd;
  PFONCT[0xB9]=ADCAe;
  PFONCT[0xA9]=ADCAx;
  PFONCT[0xC9]=ADCBi;
  PFONCT[0xD9]=ADCBd;
  PFONCT[0xF9]=ADCBe;
  PFONCT[0xE9]=ADCBx;

  /* ADD */
  PFONCT[0x8B]=ADDAi;
  PFONCT[0x9B]=ADDAd;
  PFONCT[0xBB]=ADDAe;
  PFONCT[0xAB]=ADDAx;
  PFONCT[0xCB]=ADDBi;
  PFONCT[0xDB]=ADDBd;
  PFONCT[0xFB]=ADDBe;
  PFONCT[0xEB]=ADDBx;
  PFONCT[0xC3]=ADDDi;
  PFONCT[0xD3]=ADDDd;
  PFONCT[0xF3]=ADDDe;
  PFONCT[0xE3]=ADDDx;

  /* MUL */
  PFONCT[0x3D]=MUL;


  /* SBC */
  PFONCT[0x82]=SBCAi;
  PFONCT[0x92]=SBCAd;
  PFONCT[0xB2]=SBCAe;
  PFONCT[0xA2]=SBCAx;
  PFONCT[0xC2]=SBCBi;
  PFONCT[0xD2]=SBCBd;
  PFONCT[0xF2]=SBCBe;
  PFONCT[0xE2]=SBCBx;

  /* SUB */
  PFONCT[0x80]=SUBAi;
  PFONCT[0x90]=SUBAd;
  PFONCT[0xB0]=SUBAe;
  PFONCT[0xA0]=SUBAx;
  PFONCT[0xC0]=SUBBi;
  PFONCT[0xD0]=SUBBd;
  PFONCT[0xF0]=SUBBe;
  PFONCT[0xE0]=SUBBx;
  PFONCT[0x83]=SUBDi;
  PFONCT[0x93]=SUBDd;
  PFONCT[0xB3]=SUBDe;
  PFONCT[0xA3]=SUBDx;

  /* SEX */
  PFONCT[0x1D]=SEX;

  /* ASL */
  PFONCT[0x08]=ASLd;
  PFONCT[0x78]=ASLe;
  PFONCT[0x68]=ASLx;
  PFONCT[0x48]=ASLAi;
  PFONCT[0x58]=ASLBi;

  /* ASR */
  PFONCT[0x07]=ASRd;
  PFONCT[0x77]=ASRe;
  PFONCT[0x67]=ASRx;
  PFONCT[0x47]=ASRAi;
  PFONCT[0x57]=ASRBi;

  /* LSR */
  PFONCT[0x04]=LSRd;
  PFONCT[0x74]=LSRe;
  PFONCT[0x64]=LSRx;
  PFONCT[0x44]=LSRAi;
  PFONCT[0x54]=LSRBi;

  /* ROL */
  PFONCT[0x09]=ROLd;
  PFONCT[0x79]=ROLe;
  PFONCT[0x69]=ROLx;
  PFONCT[0x49]=ROLAi;
  PFONCT[0x59]=ROLBi;

  /* ROR */
  PFONCT[0x06]=RORd;
  PFONCT[0x76]=RORe;
  PFONCT[0x66]=RORx;
  PFONCT[0x46]=RORAi;
  PFONCT[0x56]=RORBi;

  /* BRA */
  PFONCT[0x20]=BRA;
  PFONCT[0x16]=LBRA;

  /* JMP */
  PFONCT[0x0E]=JMPd;
  PFONCT[0x7E]=JMPe;
  PFONCT[0x6E]=JMPx;

  /* BSR */
  PFONCT[0x8D]=BSR;
  PFONCT[0x17]=LBSR;

  /* JSR */
  PFONCT[0x9D]=JSRd;
  PFONCT[0xBD]=JSRe;
  PFONCT[0xAD]=JSRx;

  /* BRN */
  PFONCT[0x21]=BRN;
  PFONCTS[0x21]=LBRN;

  /* NOP */
  PFONCT[0x12]=NOP;

  /* RTS */
  PFONCT[0x39]=RTS;

  /* BCC */
  PFONCT[0x24]=BCC;
  PFONCTS[0x24]=LBCC;

  /* BCS */
  PFONCT[0x25]=BCS;
  PFONCTS[0x25]=LBCS;

  /* BEQ */
  PFONCT[0x27]=BEQ;
  PFONCTS[0x27]=LBEQ;

  /* BNE */
  PFONCT[0x26]=BNE;
  PFONCTS[0x26]=LBNE;

  /* BGE */
  PFONCT[0x2C]=BGE;
  PFONCTS[0x2C]=LBGE;

  /* BLE */
  PFONCT[0x2F]=BLE;
  PFONCTS[0x2F]=LBLE;

  /* BLS */
  PFONCT[0x23]=BLS;
  PFONCTS[0x23]=LBLS;

  /* BGT */
  PFONCT[0x2E]=BGT;
  PFONCTS[0x2E]=LBGT;

  /* BLT */
  PFONCT[0x2D]=BLT;
  PFONCTS[0x2D]=LBLT;

  /* BHI */
  PFONCT[0x22]=BHI;
  PFONCTS[0x22]=LBHI;

  /* BMI */
  PFONCT[0x2B]=BMI;
  PFONCTS[0x2B]=LBMI;

  /* BPL */
  PFONCT[0x2A]=BPL;
  PFONCTS[0x2A]=LBPL;

  /* BVC */
  PFONCT[0x28]=BVC;
  PFONCTS[0x28]=LBVC;

  /* BVS */
  PFONCT[0x29]=BVS;
  PFONCTS[0x29]=LBVS;

  /* SWI1&3 */
  PFONCT[0x3F]=SWI;
  PFONCT[0x02]=TRAP;

  /* RTI */
  PFONCT[0x3B]=RTI;

  /* opcode 01 */
  PFONCT[0x01]=OP01;

  /* DAA */
  PFONCT[0x19]=DAA;

  /* CWAI */
  PFONCT[0x3C]=CWAI;
}
