#include "cdgn_global.h"


/*char abtab[] = " ? ANDASLBCCBCSBEQBITBMI"
					"BNEBPLBRKBVCBVSCLCCLDCLI"
					"CLVCMPCPXCPYDECDEXDEYEOR"
					"INCINXINYJMPJSRLDALDXLDY"
					"LSRNOPORAPHAPHPPLAPLPROL"
					"RORRTIRTSSBCSECSEDSEISTA"
					"STXSTYTAXTAYTSXTXATXSTYA"
					"ADC";							*/


char abtab[] =
" ? "                                              /* 000 */
"AND" "ASL" "BCC" "BCS" "BEQ" "BIT" "BMI" "BNE"   /* 003–024 */
"BPL" "BRK" "BVC" "BVS" "CLC" "CLD" "CLI" "CLV"   /* 027–048 */
"CMP" "CPX" "CPY" "DEC" "DEX" "DEY" "EOR" "INC"   /* 051–072 */
"INX" "INY" "JMP" "JSR" "LDA" "LDX" "LDY" "LSR"   /* 075–096 */
"NOP" "ORA" "PHA" "PHP" "PLA" "PLP" "ROL" "ROR"   /* 099–120 */
"RTI" "RTS" "SBC" "SEC" "SED" "SEI" "STA" "STX"   /* 123–144 */
"STY" "TAX" "TAY" "TSX" "TXA" "TXS" "TYA" "ADC";  /* 147–168 */

char abofst[] =   { 30,102,0,0,0,102,6,0    , 108,102,6,0,0,102,6,0
         , 27,102,0,0,0,102,6,0      , 39,102,0,0,0,102,6,0
         
		, 84,3,0,0,18,3,117,0      , 114,3,117,0,18,3,117,0
         , 21,3,0,0,0,3,117,0      , 132,3,0,0,0,3,117,0

         , 123,69,0,0,0,69,96,0    , 105,69,96,0,81,69,96,0
         , 33,69,0,0,0,69,96,0     , 45,69,0,0,0,69,96,0

         , 126,168,0,0,0,168,120,0  , 111,168,120,0,81,168,120,0
         , 36,168,0,0,0,168,120,0   , 138,168,0,0,0,168,120,0



         , 0,141,0,0,147,141,144,0   , 66,0,159,0,147,141,144,0
         , 9,141,0,0,147,141,144,0   , 165,141,162,0,0,141,0,0

         , 93,87,90,0,93,87,90,0    , 153,87,150,0,93,87,90,0
         , 12,87,0,0,93,87,90,0     , 48,87,156,0,93,87,90,0

         , 57,51,0,0,57,51,60,0     , 78,51,63,0,57,51,60,0
         , 24,51,0,0,0,51,60,0      , 42,51,0,0,0,51,60,0

         , 54,129,0,0,54,129,72,0   , 75,129,99,0,54,129,72,0
         , 15,129,0,0,0,129,72,0    , 135,129,0,0,0,129,72,0	};

char opertytb[] = {OPC,BAAXB,Q,Q,Q,AA,AA,Q,				//256 bytes corespond to opcodes
						OPC,NN,OPC,Q,Q,AAAA,AAAA,Q,
						BRA,BAABY,Q,Q,Q,AAX,AAX,Q,		//$10
						OPC,AAAAY,Q,Q,Q,AAAAX,AAAAX,Q,
				
				AAAA,BAAXB,Q,Q,AA,AA,AA,Q,				//$20
				OPC,NN,OPC,Q,AAAA,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,Q,AAX,AAX,Q,				//$30
				OPC,AAAAY,Q,Q,Q,AAAAX,AAAAX,Q,
				
				OPC,BAAXB,Q,Q,Q,AA,AA,Q,				//$40
				OPC,NN,OPC,Q,AAAA,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,Q,AAX,AAX,Q,				//$50
				OPC,AAAAY,Q,Q,Q,AAAAX,AAAAX,Q,
				
				OPC,BAAXB,Q,Q,Q,AA,AA,Q,				//$60
				OPC,NN,OPC,Q,BAAAAB,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,Q,AAX,AAX,Q,				//$70
				OPC,AAAAY,Q,Q,Q,AAAAX,AAAAX,Q,
				
				
				
				Q,BAAXB,Q,Q,AA,AA,AA,Q,					//$80
				OPC,Q,OPC,Q,AAAA,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,AAX,AAX,AAY,Q,			//$90
				OPC,AAAAY,OPC,Q,Q,AAAAX,Q,Q,
				
				NN,BAAXB,NN,Q,AA,AA,AA,Q,				//$A0
				OPC,NN,OPC,Q,AAAA,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,AAX,AAX,AAY,Q,			//$B0
				OPC,AAAAY,OPC,Q,AAAAX,AAAAX,AAAAY,Q,
				
				NN,BAAXB,Q,Q,AA,AA,AA,Q,				//$C0
				OPC,NN,OPC,Q,AAAA,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,Q,AAX,AAX,Q,				//$D0
				OPC,AAAAY,Q,Q,Q,AAAAX,AAAAX,Q,
				
				NN,BAAXB,Q,Q,AA,AA,AA,Q,				//$E0
				OPC,NN,OPC,Q,AAAA,AAAA,AAAA,Q,
				BRA,BAABY,Q,Q,Q,AAX,AAX,Q,				//$F0
				OPC,AAAAY,Q,Q,Q,AAAAX,AAAAX,Q };


unsigned char oplen[256] = {
	/* 00 */ 1,2,0,0,0,2,2,0, 1,2,1,0,0,3,3,0,
	/* 10 */ 2,2,0,0,0,2,2,0, 1,3,0,0,0,3,3,0,
	/* 20 */ 3,2,0,0,2,2,2,0, 1,2,1,0,3,3,3,0,
	/* 30 */ 2,2,0,0,0,2,2,0, 1,3,0,0,0,3,3,0,

	/* 40 */ 1,2,0,0,0,2,2,0, 1,2,1,0,3,3,3,0,
	/* 50 */ 2,2,0,0,0,2,2,0, 1,3,0,0,0,3,3,0,
	/* 60 */ 1,2,0,0,0,2,2,0, 1,2,1,0,3,3,3,0,
	/* 70 */ 2,2,0,0,0,2,2,0, 1,3,0,0,0,3,3,0,

	/* 80 */ 0,2,0,0,2,2,2,0, 1,0,1,0,3,3,3,0,
	/* 90 */ 2,2,0,0,2,2,2,0, 1,3,1,0,0,3,3,0,
	/* A0 */ 2,2,2,0,2,2,2,0, 1,2,1,0,3,3,3,0,
	/* B0 */ 2,2,0,0,2,2,2,0, 1,3,1,0,3,3,3,0,

	/* C0 */ 2,2,0,0,2,2,2,0, 1,2,1,0,3,3,3,0,
	/* D0 */ 2,2,0,0,0,2,2,0, 1,3,0,0,0,3,3,0,
	/* E0 */ 2,2,0,0,2,2,2,0, 1,2,1,0,3,3,3,0,
	/* F0 */ 2,2,0,0,0,2,2,0, 1,3,0,0,0,3,3,0
};


char esc_ch_tab[16] = {27,28,29,30,31,125,126,127,155,156,157,158,159,253,254,255 };