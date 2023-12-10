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
	MODULE      : setup2.c
	Créé par    : Gilles FETIS
	Modifié par :

	Module d'initialisation de la table du désassembleur intégré

*/

#include "flags.h"

#ifndef NO_DEBUGGER

#include <string.h>
#include "debug.h"

/*Les 4 premieres lettres sont le mnémonique
suivi de:
i opérande immédiat 8 bits
I immédiat 16 bits
- pas d'opérande
d mode direct
e mode étendu
x indirect
o offset sur 1 octet
O offset sur 2 octets
r 2 registres (échanges)
R liste de registres (Pile)
*/

void setup_desass(void)
{
  int l;

  for (l=0;l<256;l++)
  {
   strcpy(MNEMO[l],"ILL ");
   strcpy(MNEMO[l],"ILL ");
   strcpy(MNEMO11[l],"ILL ");
  }

  /* LDA opcode */
  strcpy(MNEMO[0x86],"LDA i");
  strcpy(MNEMO[0x96],"LDA d");
  strcpy(MNEMO[0xB6],"LDA e");
  strcpy(MNEMO[0xA6],"LDA x");

  /* LDB opcode */
  strcpy(MNEMO[0xC6],"LDB i");
  strcpy(MNEMO[0xD6],"LDB d");
  strcpy(MNEMO[0xF6],"LDB e");
  strcpy(MNEMO[0xE6],"LDB x");

  /* LDD opcode */
  strcpy(MNEMO[0xCC],"LDD I");
  strcpy(MNEMO[0xDC],"LDD d");
  strcpy(MNEMO[0xFC],"LDD e");
  strcpy(MNEMO[0xEC],"LDD x");

  /* LDU opcode */
  strcpy(MNEMO[0xCE],"LDU I");
  strcpy(MNEMO[0xDE],"LDU d");
  strcpy(MNEMO[0xFE],"LDU e");
  strcpy(MNEMO[0xEE],"LDU x");

  /* LDX opcode */
  strcpy(MNEMO[0x8E],"LDX I");
  strcpy(MNEMO[0x9E],"LDX d");
  strcpy(MNEMO[0xBE],"LDX e");
  strcpy(MNEMO[0xAE],"LDX x");

  /* LDS opcode */
  strcpy(MNEMO10[0xCE],"LDS I");
  strcpy(MNEMO10[0xDE],"LDS d");
  strcpy(MNEMO10[0xFE],"LDS e");
  strcpy(MNEMO10[0xEE],"LDS x");

  /* LDY opcode */
  strcpy(MNEMO10[0x8E],"LDY I");
  strcpy(MNEMO10[0x9E],"LDY d");
  strcpy(MNEMO10[0xBE],"LDY e");
  strcpy(MNEMO10[0xAE],"LDY x");

  /* STA opcode */
  strcpy(MNEMO[0x97],"STA d");
  strcpy(MNEMO[0xB7],"STA e");
  strcpy(MNEMO[0xA7],"STA x");

  /* STB opcode */
  strcpy(MNEMO[0xD7],"STB d");
  strcpy(MNEMO[0xF7],"STB e");
  strcpy(MNEMO[0xE7],"STB x");

  /* STD opcode */
  strcpy(MNEMO[0xDD],"STD d");
  strcpy(MNEMO[0xFD],"STD e");
  strcpy(MNEMO[0xED],"STD x");

  /* STS opcode */
  strcpy(MNEMO10[0xDF],"STS d");
  strcpy(MNEMO10[0xFF],"STS e");
  strcpy(MNEMO10[0xEF],"STS x");

  /* STU opcode */
  strcpy(MNEMO[0xDF],"STU d");
  strcpy(MNEMO[0xFF],"STU e");
  strcpy(MNEMO[0xEF],"STU x");

  /* STX opcode */
  strcpy(MNEMO[0x9F],"STX d");
  strcpy(MNEMO[0xBF],"STX e");
  strcpy(MNEMO[0xAF],"STX x");

  /* STY opcode */
  strcpy(MNEMO10[0x9F],"STY d");
  strcpy(MNEMO10[0xBF],"STY e");
  strcpy(MNEMO10[0xAF],"STY x");

  /* LEA opcode */
  strcpy(MNEMO[0x32],"LEASx");
  strcpy(MNEMO[0x33],"LEAUx");
  strcpy(MNEMO[0x30],"LEAXx");
  strcpy(MNEMO[0x31],"LEAYx");

  /* CLR opcode */
  strcpy(MNEMO[0x0F],"CLR d");
  strcpy(MNEMO[0x7F],"CLR e");
  strcpy(MNEMO[0x6F],"CLR x");
  strcpy(MNEMO[0x4F],"CLRA-");
  strcpy(MNEMO[0x5F],"CLRB-");

  /* EXG */
  strcpy(MNEMO[0x1E],"EXG r");

  /* TFR */
  strcpy(MNEMO[0x1F],"TFR r");

  /* PSH */
  strcpy(MNEMO[0x34],"PSHSR");
  strcpy(MNEMO[0x36],"PSHUR");

  /* PUL */
  strcpy(MNEMO[0x35],"PULSR");
  strcpy(MNEMO[0x37],"PULUR");

  /* INC */
  strcpy(MNEMO[0x4C],"INCA-");
  strcpy(MNEMO[0x5C],"INCB-");
  strcpy(MNEMO[0x7C],"INC e");
  strcpy(MNEMO[0x0C],"INC d");
  strcpy(MNEMO[0x6C],"INC x");

  /* DEC */
  strcpy(MNEMO[0x4A],"DECA-");
  strcpy(MNEMO[0x5A],"DECB-");
  strcpy(MNEMO[0x7A],"DEC e");
  strcpy(MNEMO[0x0A],"DEC d");
  strcpy(MNEMO[0x6A],"DEC x");

  /* BIT */
  strcpy(MNEMO[0x85],"BITAi");
  strcpy(MNEMO[0x95],"BITAd");
  strcpy(MNEMO[0xB5],"BITAe");
  strcpy(MNEMO[0xA5],"BITAx");
  strcpy(MNEMO[0xC5],"BITBi");
  strcpy(MNEMO[0xD5],"BITBd");
  strcpy(MNEMO[0xF5],"BITBe");
  strcpy(MNEMO[0xE5],"BITBx");

  /* CMP */
  strcpy(MNEMO[0x81],"CMPAi");
  strcpy(MNEMO[0x91],"CMPAd");
  strcpy(MNEMO[0xB1],"CMPAe");
  strcpy(MNEMO[0xA1],"CMPAx");
  strcpy(MNEMO[0xC1],"CMPBi");
  strcpy(MNEMO[0xD1],"CMPBd");
  strcpy(MNEMO[0xF1],"CMPBe");
  strcpy(MNEMO[0xE1],"CMPBx");
  strcpy(MNEMO10[0x83],"CMPDI");
  strcpy(MNEMO10[0x93],"CMPDd");
  strcpy(MNEMO10[0xB3],"CMPDe");
  strcpy(MNEMO10[0xA3],"CMPDx");
  strcpy(MNEMO11[0x8C],"CMPSI");
  strcpy(MNEMO11[0x9C],"CMPSd");
  strcpy(MNEMO11[0xBC],"CMPSe");
  strcpy(MNEMO11[0xAC],"CMPSx");
  strcpy(MNEMO11[0x83],"CMPUI");
  strcpy(MNEMO11[0x93],"CMPUd");
  strcpy(MNEMO11[0xB3],"CMPUe");
  strcpy(MNEMO11[0xA3],"CMPUx");
  strcpy(MNEMO[0x8C],"CMPXI");
  strcpy(MNEMO[0x9C],"CMPXd");
  strcpy(MNEMO[0xBC],"CMPXe");
  strcpy(MNEMO[0xAC],"CMPXx");
  strcpy(MNEMO10[0x8C],"CMPYI");
  strcpy(MNEMO10[0x9C],"CMPYd");
  strcpy(MNEMO10[0xBC],"CMPYe");
  strcpy(MNEMO10[0xAC],"CMPYx");

  /* TST */
  strcpy(MNEMO[0x4D],"TSTA-");
  strcpy(MNEMO[0x5D],"TSTB-");
  strcpy(MNEMO[0x0D],"TST d");
  strcpy(MNEMO[0x7D],"TST e");
  strcpy(MNEMO[0x6D],"TST x");

  /* AND */
  strcpy(MNEMO[0x84],"ANDAi");
  strcpy(MNEMO[0x94],"ANDAd");
  strcpy(MNEMO[0xB4],"ANDAe");
  strcpy(MNEMO[0xA4],"ANDAx");
  strcpy(MNEMO[0xC4],"ANDBi");
  strcpy(MNEMO[0xD4],"ANDBd");
  strcpy(MNEMO[0xF4],"ANDBe");
  strcpy(MNEMO[0xE4],"ANDBx");
  strcpy(MNEMO[0x1C],"ANDCCi");

  /* OR */
  strcpy(MNEMO[0x8A],"ORA i");
  strcpy(MNEMO[0x9A],"ORA d");
  strcpy(MNEMO[0xBA],"ORA e");
  strcpy(MNEMO[0xAA],"ORA x");
  strcpy(MNEMO[0xCA],"ORB i");
  strcpy(MNEMO[0xDA],"ORB d");
  strcpy(MNEMO[0xFA],"ORB e");
  strcpy(MNEMO[0xEA],"ORB x");
  strcpy(MNEMO[0x1A],"ORCCi");

  /* EOR */
  strcpy(MNEMO[0x88],"EORAi");
  strcpy(MNEMO[0x98],"EORAd");
  strcpy(MNEMO[0xB8],"EORAe");
  strcpy(MNEMO[0xA8],"EORAx");
  strcpy(MNEMO[0xC8],"EORBi");
  strcpy(MNEMO[0xD8],"EORBd");
  strcpy(MNEMO[0xF8],"EORBe");
  strcpy(MNEMO[0xE8],"EORBx");

  /* COM */
  strcpy(MNEMO[0x03],"COM d");
  strcpy(MNEMO[0x73],"COM e");
  strcpy(MNEMO[0x63],"COM x");
  strcpy(MNEMO[0x43],"COMA-");
  strcpy(MNEMO[0x53],"COMB-");

  /* NEG */
  strcpy(MNEMO[0x00],"NEG d");
  strcpy(MNEMO[0x70],"NEG e");
  strcpy(MNEMO[0x60],"NEG x");
  strcpy(MNEMO[0x40],"NEGA-");
  strcpy(MNEMO[0x50],"NEGB-");

  /* ABX */
  strcpy(MNEMO[0x3A],"ABX -");

  /* ADC */
  strcpy(MNEMO[0x89],"ADCAi");
  strcpy(MNEMO[0x99],"ADCAd");
  strcpy(MNEMO[0xB9],"ADCAe");
  strcpy(MNEMO[0xA9],"ADCAx");
  strcpy(MNEMO[0xC9],"ADCBi");
  strcpy(MNEMO[0xD9],"ADCBd");
  strcpy(MNEMO[0xF9],"ADCBe");
  strcpy(MNEMO[0xE9],"ADCBx");

  /* ADD */
  strcpy(MNEMO[0x8B],"ADDAi");
  strcpy(MNEMO[0x9B],"ADDAd");
  strcpy(MNEMO[0xBB],"ADDAe");
  strcpy(MNEMO[0xAB],"ADDAx");
  strcpy(MNEMO[0xCB],"ADDBi");
  strcpy(MNEMO[0xDB],"ADDBd");
  strcpy(MNEMO[0xFB],"ADDBe");
  strcpy(MNEMO[0xEB],"ADDBx");
  strcpy(MNEMO[0xC3],"ADDDI");
  strcpy(MNEMO[0xD3],"ADDDd");
  strcpy(MNEMO[0xF3],"ADDDe");
  strcpy(MNEMO[0xE3],"ADDDx");

  /* MUL */
  strcpy(MNEMO[0x3D],"MUL -");


  /* SBC */
  strcpy(MNEMO[0x82],"SBCAi");
  strcpy(MNEMO[0x92],"SBCAd");
  strcpy(MNEMO[0xB2],"SBCAe");
  strcpy(MNEMO[0xA2],"SBCAx");
  strcpy(MNEMO[0xC2],"SBCBi");
  strcpy(MNEMO[0xD2],"SBCBd");
  strcpy(MNEMO[0xF2],"SBCBe");
  strcpy(MNEMO[0xE2],"SBCBx");

  /* SUB */
  strcpy(MNEMO[0x80],"SUBAi");
  strcpy(MNEMO[0x90],"SUBAd");
  strcpy(MNEMO[0xB0],"SUBAe");
  strcpy(MNEMO[0xA0],"SUBAx");
  strcpy(MNEMO[0xC0],"SUBBi");
  strcpy(MNEMO[0xD0],"SUBBd");
  strcpy(MNEMO[0xF0],"SUBBe");
  strcpy(MNEMO[0xE0],"SUBBx");
  strcpy(MNEMO[0x83],"SUBDI");
  strcpy(MNEMO[0x93],"SUBDd");
  strcpy(MNEMO[0xB3],"SUBDe");
  strcpy(MNEMO[0xA3],"SUBDx");

  /* SEX */
  strcpy(MNEMO[0x1D],"SEX -");

  /* ASL */
  strcpy(MNEMO[0x08],"ASL d");
  strcpy(MNEMO[0x78],"ASL e");
  strcpy(MNEMO[0x68],"ASL x");
  strcpy(MNEMO[0x48],"ASLA-");
  strcpy(MNEMO[0x58],"ASLB-");

  /* ASR */
  strcpy(MNEMO[0x07],"ASR d");
  strcpy(MNEMO[0x77],"ASR e");
  strcpy(MNEMO[0x67],"ASR x");
  strcpy(MNEMO[0x47],"ASRA-");
  strcpy(MNEMO[0x57],"ASRB-");

  /* LSR */
  strcpy(MNEMO[0x04],"LSR d");
  strcpy(MNEMO[0x74],"LSR e");
  strcpy(MNEMO[0x64],"LSR x");
  strcpy(MNEMO[0x44],"LSRA-");
  strcpy(MNEMO[0x54],"LSRB-");

  /* ROL */
  strcpy(MNEMO[0x09],"ROL d");
  strcpy(MNEMO[0x79],"ROL e");
  strcpy(MNEMO[0x69],"ROL x");
  strcpy(MNEMO[0x49],"ROLA-");
  strcpy(MNEMO[0x59],"ROLB-");

  /* ROR */
  strcpy(MNEMO[0x06],"ROR d");
  strcpy(MNEMO[0x76],"ROR e");
  strcpy(MNEMO[0x66],"ROR x");
  strcpy(MNEMO[0x46],"RORA-");
  strcpy(MNEMO[0x56],"RORB-");

  /* BRA */
  strcpy(MNEMO[0x20],"BRA o");
  strcpy(MNEMO[0x16],"LBRAO");

  /* JMP */
  strcpy(MNEMO[0x0E],"JMP d");
  strcpy(MNEMO[0x7E],"JMP e");
  strcpy(MNEMO[0x6E],"JMP x");

  /* BSR */
  strcpy(MNEMO[0x8D],"BSR o");
  strcpy(MNEMO[0x17],"LBSRO");

  /* JSR */
  strcpy(MNEMO[0x9D],"JSR d");
  strcpy(MNEMO[0xBD],"JSR e");
  strcpy(MNEMO[0xAD],"JSR x");

  /* BRN */
  strcpy(MNEMO[0x21],"BRN o");
  strcpy(MNEMO10[0x21],"LBRNO");

  /* NOP */
  strcpy(MNEMO[0x12],"NOP -");

  /* RTS */
  strcpy(MNEMO[0x39],"RTS -");

  /* BCC */
  strcpy(MNEMO[0x24],"BCC o");
  strcpy(MNEMO10[0x24],"LBCCO");

  /* BCS */
  strcpy(MNEMO[0x25],"BCS o");
  strcpy(MNEMO10[0x25],"LBCSO");

  /* BEQ */
  strcpy(MNEMO[0x27],"BEQ o");
  strcpy(MNEMO10[0x27],"LBEQO");

  /* BNE */
  strcpy(MNEMO[0x26],"BNE o");
  strcpy(MNEMO10[0x26],"LBNEO");

  /* BGE */
  strcpy(MNEMO[0x2C],"BGE o");
  strcpy(MNEMO10[0x2C],"LBGEO");

  /* BLE */
  strcpy(MNEMO[0x2F],"BLE o");
  strcpy(MNEMO10[0x2F],"LBLEO");

  /* BLS */
  strcpy(MNEMO[0x23],"BLS o");
  strcpy(MNEMO10[0x23],"LBLSO");

  /* BGT */
  strcpy(MNEMO[0x2E],"BGT o");
  strcpy(MNEMO10[0x2E],"LBGTO");

  /* BLT */
  strcpy(MNEMO[0x2D],"BLT o");
  strcpy(MNEMO10[0x2D],"LBLTO");

  /* BHI */
  strcpy(MNEMO[0x22],"BHI o");
  strcpy(MNEMO10[0x22],"LBHIO");

  /* BMI */
  strcpy(MNEMO[0x2B],"BMI o");
  strcpy(MNEMO10[0x2B],"LBMIO");

  /* BPL */
  strcpy(MNEMO[0x2A],"BPL o");
  strcpy(MNEMO10[0x2A],"LBPLO");

  /* BVC */
  strcpy(MNEMO[0x28],"BVC o");
  strcpy(MNEMO10[0x28],"LBVCO");

  /* BVS */
  strcpy(MNEMO[0x29],"BVS o");
  strcpy(MNEMO10[0x29],"LBVSO");

  /* SWI1&3 */
  strcpy(MNEMO[0x3F],"SWI i");
  strcpy(MNEMO11[0x3F],"SWI3-");

  /* RTI */
  strcpy(MNEMO[0x3B],"RTI -");
}

#endif
