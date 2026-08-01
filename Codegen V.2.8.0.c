/////////////////////////////////////////////////////////////
//   FILE TO IMAGE

/************************** include *****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <direct.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include "cdgn_global.h"

/***************  Preprocessor   **************/

#define ATARI800    1		// Set to 0 only if you are assembling code for a non-ATARI system.
#define DISASSEMBLE 1		// Probably don't need to change this.
#define SHOW_POSTFIX   0	// Shows arithmetic evaluluation for each line: infix  postfix  result
#define DELETE_TEMP_ATR    1// Delete temp.atr before starting.
#define IMBED_SRC_FILENAME 1// Embed SRC file path and name into this program. See cdgn_global.h for examples.
#define SORT_EQU_EPZ 1		//Place EQU constants before EPZ addresses in symtab. !!! Set to 0 for SOURCGEN 3.2 !!!
#define COMMENTS 1			//Set to 1 to include comments in generated code.
#define COMMENTS2 1			//Set to 1 to include comments after EQU and EPZ.


#define ROUND_TRIP_VERIFICATION 1



/**************************  define  *****************/
#define MAX_BIN_SIZ 0x1ffff

#define MAXMACSYMS 200
#define MAXLOCLABS 50
#define MXLBS 10				//maximum # of labels in present macro
#define MAXMACLEV 10			// maximum # of nesting levels in macros
#define NOTFOUND 0xeffff
#define MAX_TXT_SIZ 0xfffff

#define MAX_LINE_SIZE 255
#define PRN_BUF_SIZE 255
#define MAX_LABEL_SIZE 80		// (not symbol) see MAX_SYMBOL_SIZE in cdgn_global.h
#define MAXDUMS 20
#define LSTBYTES 4

enum { UNUSED, SYMBOL, MACRO, MACRO_UNUSED, MACRO_LAB };


/**************************  prototypes  ***************/


uint32_t expresstoi(void);
uint32_t symtoi1(void);
uint32_t expresstoi_p2(void);
void error(char* errstr);
int chkopc(const unsigned char opcstring[]);
void toxlbuf(uint8_t);
void find_operand(void);
static void operand_to_filename(char filename[]);
void list_label(int i);
void list_label_xasm(int i);
void list_label_f(int i);
void list_label_f2(int i);		//label zu LIS file T
//uint32_t symtoi2(char* strbeg);
uint32_t symtoi3(char* strbeg);
void sectyp(char line_type);
char fptsmb(int i);
int prec(char symbol);
uint16_t parseNumber(char* txtptr);
int isSpecialCharacter(char ch);
static void print_visible(const char* s);
/*************************  global variables  ************/

const unsigned char opcdat[] = "BITSTYLDYCPYCPXORAANDEORADCSTALDACMPSBCASLROLLSRRORSTXLDXDECINC";

const unsigned char data[21][15] = {

	/* BIT */{ 0,0,0x24,0x2C,0,0,0,0,0,0xD6,0x67,0x46,0x48,0x4A},
	/* STY */{ 0,0,0x84,0x8C,0x94,0,0,0,0,0,0xD6,0x67,0x46,0,0},
	/* LDY */{ 0,0xA0,0xA4,0xAC,0xB4,0xBC,0,0,0,0,0xD6,0x67,0x46,0xD0,0x4A},
	/* CPY */{ 0,0xC0,0xC4,0xCC,0,0,0,0,0,0,0xD6,0x67,0x46,0,0},
	/* CPX */{ 0,0xE0,0xE4,0xEC,0,0,0,0,0,0,0xD6,0x67,0x46,0,0},
	/* ORA */{ 0,9,5,0xD,0x15,0x1D,0,0x19,1,0x11,0xD6,0x67,0x46,0,0},
	/* AND */{ 0,0x29,0x25,0x2D,0x35,0x3D,0,0x39,0x21,0x31,0xD6,0x67,0x46,0,0},
	/* EOR */{ 0,0x49,0x45,0x4D,0x55,0x5D,0,0x59,0x41,0x51,0xD6,0x67,0x46,0,0},
	/* ADC */{ 0,0x69,0x65,0x6D,0x75,0x7D,0,0x79,0x61,0x71,0xD6,0x67,0x46,0xBF,0x4A},
	/* STA */{ 0,0,0x85,0x8D,0x95,0x9D,0,0x99,0x81,0x91,0xD6,0x67,0x46,0,0},
	/* LDA */{ 0,0xA9,0xA5,0xAD,0xB5,0xBD,0,0xB9,0xA1,0xB1,0xD6,0x67,0x46,0,0},
	/* CMP */{ 0,0xC9,0xC5,0xCD,0xD5,0xDD,0,0xD9,0xC1,0xD1,0xD6,0x67,0x46,0,0},
	/* SBC */{ 0,0xE9,0xE5,0xED,0xF5,0xFD,0,0xF9,0xE1,0xF1,0xD6,0x67,0x46,0x9E,0x49}, // first 10 bytes are possible codes
	/* ASL */{ 0xA,0,6,0xE,0x16,0x1E,0,0,0,0,0xD6,0x67,0x46,0,0},
	/* ROL */{ 0x2A,0,0x26,0x2E,0x36,0x3E,0,0,0,0,0xD6,0x67,0x46,0x37,0x4A},
	/* LSR */{ 0x4A,0,0x46,0x4E,0x56,0x5E,0,0,0,0,0xD6,0x67,0x46,0xF3,0x49},
	/* ROR */{ 0x6A,0,0x66,0x6E,0x76,0x7E,0,0,0,0,0xD6,0x67,0x46,0x8D,0x49},
	/* STX */{ 0,0,0x86,0x8E,0,0,0x96,0,0,0,0xD6,0x67,0x46,0x15,0x4A},//In ATMAS Should be $96,0, not 0,$96,  -- STX aa,Y not STX aaaa,Y
	/* LDX */{ 0,0xA2,0xA6,0xAE,0,0,0xB6,0xBE,0,0,0xD6,0x67,0x46,0,0},
	/* DEC */{ 0,0,0xC6,0xCE,0xD6,0xDE,0,0,0,0,0xD6,0x67,0x46,0,0},
	/* INC */{ 0,0,0xE6,0xEE,0xF6,0xFE,0,0,0,0,0,0,0,0,0}
};


#if ATARI800
unsigned int sgndtb[100] = { 0 }, * sgndtbptr1, * sgndtbptr2, pcsv;
#endif

char ch = 0;
char section_type = 0;
char* txtptr, * txtptr_sav;	//pointer for all addresses in this buffer
char* xlbfptr;
char* linbegptr;	// begin of present line

size_t txtsiz = 0, include_txtsiz = 0;

uint16_t symnum = 0, symval = 0;
int linnum_sav2 = 0, linnum = 0, linnum_sav = 0, mismatch_linnum = 0;
uint16_t pc = 0, pc_sav = 0, mismatch_pc = 0;
int presymnum = 0;
int secsiz = 0;
short linlen = 0;	// length of present line in text buffer
int cnt = 0;
short pass = 0;
short labflg = 0;
short orgflg = 0;
short cpfflg = 0;
short lstflg = 0;
short include_flg = 0;
short mismatch_flg = 0;
short data_flg = 0;
short lensv = 0;
int bin = 0;
short find_symad_flg = 0;
short maclevel = 0;
short symfg = 0;
int donum = 0;
int loopnum = 0;
int minusnum = 0;
int plusnum = 0;
int skipnum = 0;
int contnum = 0;
int endifnum = 0;
short bytcnt = 0;
short prnlnum = 0;
uint8_t syms_in_expr = 0;

char prnlnbufbeg[PRN_BUF_SIZE] = { 0 };
short mismatchln_flg[10] = { 0 };
char src_filename[MAX_FILENAME_LEN + 1] = { 0 };
char opcode[MAX_LABEL_SIZE + 1] = { 0 }, operand[MAX_FILENAME_LEN + 1] = { 0 };
char outflgs[MAX_LABEL_SIZE + 1] = { 0 };
char prnlnbuf2[10][PRN_BUF_SIZE] = { {0} };
char cmpflbytln[9][MAX_LABEL_SIZE + 1] = { {0} };
char linbuf[MAXMACLEV][MAX_LINE_SIZE + 1] = { {0} };

ssstype	sss[MAXSYMS] = { 0 };

int ssscmpadr(const ssstype* dummy1, const ssstype* dummy2);

FILE* stream;
FILE* stream2;
FILE* stream3;
FILE* stream4;
fpos_t pos2;

#if DISASSEMBLE

void disassemble(const char* filename);

uint8_t* xlram = NULL; // Declare xlram as a global variable



#endif

static const unsigned char	opcdat1[] = "BRKPHPASLCLCPLPROLSECRTIPHALSRCLIRTSPLARORSEI"
"DEYTXATYATXSTAYTAXCLVTSXINYDEXCLDINXNOPSED";
static const unsigned char dat1[] = { 0,8,10,24,40,42,56,	64,72,74,88,96,104,106,120,
					136,138,152,154,168,170,184,186,200,202,216,232,234,248 };

static const unsigned char opcds[] = " BIT STY LDY CPY CPX ORA AND EOR ADC STA LDA CMP SBC ASL ROL "
"LSR ROR STX LDX DEC INC "
"BRK PHP ASL CLC PLP ROL SEC RTI PHA LSR CLI RTS PLA ROR SEI "
"DEY TXA TYA TXS TAY TAX CLV TSX INY DEX CLD INX NOP SED "
"BPL BMI BVC BVS BCC BCS BNE BEQ JMP JSR ";// for checking labels

static const unsigned char assdirecs[] = " EQU EPZ ORG ASC DFB DFW DSKIMAGE XFDIMAGE ATRIMAGE RUNEMUL CPFL ORG APPEND RMB INITRMB "
"ADDTOIMG SAVEAS BSAVE FILLMEM _INCLUDE LINKFILE LINK SMTFILE "
#if !ATMAS_II_COMPATIBLE
".ascii .text .byte .word .org ";
#endif
static const unsigned char assdirecssolo[] = " OUT NOHEADER OBJFILE SMTTOIMG VERIFY ";

static const unsigned char brastrdat[] = "BPLBMIBVCBVSBCCBCSBNEBEQ";
static unsigned short bradat[] = { 0x10,0x30,0x50,0x70,0x90,0xb0,0xd0,0xf0 };
static char macdumtab[MAXDUMS][10];		// moved to static
static char dumbuf[80];						// moved to static
static char macsymbol[MAXMACSYMS][10] = { {0} };
static char* macptrtab[MAXMACSYMS] = { 0 };
static char loclabtb[MAXLOCLABS][5] = { {0} };
static int labexttab[MAXLOCLABS] = { 0 };			// 0001 to 9999
static int lclb[MAXMACLEV][MXLBS] = { {0} };		// table of label nos. in present macro
static unsigned short lclbnum[MAXMACLEV] = { 0 };
static char include_filename[MAX_FILENAME_LEN + 1] = { 0 };
static char loafilename[MAX_FILENAME_LEN + 1] = { 0 };
static char smbfilename[MAX_FILENAME_LEN + 1] = { 0 };
static char lstfilename[MAX_FILENAME_LEN + 1] = { 0 };
static char addtoimg_filename[64][20] = { {0} };			//
static char append_filename[36][20] = { {0} };				//
static char link_filename[36][20] = { {0} };				//
static char linkfile_filename[MAX_FILENAME_LEN + 1] = { 0 };
static char emul_filename[MAX_FILENAME_LEN + 1] = { 0 };
static char pathstr[MAX_FILENAME_LEN + 1] = { 0 };
static char dsk_filename[MAX_FILENAME_LEN + 1] = { 0 };
static char cmp_file[MAX_FILENAME_LEN + 1] = { 0 };
char current_global[MAX_LABEL_SIZE] = "";


/****************************    main()     ****************/


int main(int argc, char* argv[]) {
	/*xlram_codetpy = malloc (0xffff); /* each address could be assigned a flag used in symtyp
				Addresses in normal code which do not coincide with an opcode are assigned zero */

	xlram = (char*)malloc(0xffff);
	if (xlram == NULL) {
		printf("Can't allocate memory for text buffer.\a\n");
		hit_Enter();
		exit(1);
	}

	char* txtbuf = (char*)malloc(MAX_TXT_SIZ);
	if (txtbuf == NULL) {
		printf("Can't allocate memory for text buffer.\a\n");
		hit_Enter();
		exit(1);
	}

	char* xlbuf = (char*)malloc(MAX_BIN_SIZ);
	if (xlbuf == NULL) {
		printf("Can't allocate memory for text buffer.\a\n");
		hit_Enter();
		exit(1);
	}

	/******************    INITIALIZATION    ************/

	char* includebuf;
	char* txtendptr;	// pointer to end of text Buffer (=10)
	char* linptr;
	char* ptr;			// temporary pointer
	char* ptr2;			// temporary pointer
	char* tpsav;
	char* operptr;
	char* lnbfptr;
	char* labptr;

	short verif_flg = 0; // 0 = no verification, 1 = verification, 2 = round trip verification

#if ROUND_TRIP_VERIFICATION

	for (;;) {		// beginning of long loop for verif_flg == 2
		if (verif_flg == 2) {
			puts("\n\n*******************************************************\n");
			puts("Starting round trip verification:\n");
		}


#endif

		//int braoffset = 0;
		int pos = 0;
		char ch = 0;
		int offset = 0;
		unsigned int operval = 0;
		int operno = 0;
		int i = 0, intval = 0, intval_sav = 0;
		char label[MAX_LABEL_SIZE] = { 0 };
		char labelstr[MAX_LABEL_SIZE] = { 0 };
		char opcstr[MAX_LABEL_SIZE] = { 0 };

		//int temp = 0;
		int intab[] = { 0x40,0,0x20,0x60,0xA0,0xC0,0x80,0xE0 };



		int dumnum = 0, macroflg = 0;


		short macnum = 0;

		char* linbegsav[MAXMACLEV] = { 0 };
		short linlensav[MAXMACLEV] = { 0 };
		char* macargsptr[MAXMACLEV] = { 0 };

		short loclabnum = 0;						// # of labels in loclabtb


		int loclabtbpos;
		char lab[5] = { 0 };


		char prnlnbuf[120] = { 0 };
		short twiceflg = 0;


		short link_cnt = 0;
		short lo_fg = 0;
		short objfile_flag = 0;
		short addtoimg_cnt = 0;
		short append_cnt = 0;
		short rmbflg = 0;
		int rmbadr = 0;
		short smttoimg_flg = 0;
		char tmptyp;
		fpos_t pos1;
		bool star_flg = 0;
		bool first_NNsymbol = 1;
		bool label_only_flg = 0;
		

		if (verif_flg == 0) {


			/******************** File ******************************/
			puts("***********************************************************************");
			puts("*                                                                     *");
			puts("*                           CODEGEN V.2.8.0                           *");
			puts("*                                                                     *");
			puts("* An ATMAS-II compatible 6502-Macro-Cross-Assembler for 8-bit Ataris  *");
			puts("*                                                                     *");
			puts("*               Copyright 2004-2024 by Richard Jackman                *");
			puts("*               http://www.richard-jackman.de/Atari_XL/               *");
			puts("*                                                                     *");
			puts("* - assembles a source-file (.SRC)                                    *");
			puts("* - creates a boot image (temp.atr) containing DOS25 and AUTORUN.SYS  *");
			puts("*                                                                     *");
			puts("* See read_me.txt and SGEN42.SRC for further options and instructions.*");
			puts("*                                                                     *");
			puts("***********************************************************************\n");


			/********************* Source File *********************/

			// 1. Command-line argument takes highest priority
			if (argc > 1) strcpy_s(src_filename, sizeof(src_filename), argv[1]);
			else {

#if IMBED_SRC_FILENAME
				// 2. Embedded filename if compiled that way
				strcpy_s(src_filename, sizeof(src_filename), SRC_FILENAME);
#else
				// 3. Ask user interactively
				printf("Please enter name of file to be assembled>");

				if (fgets(src_filename, sizeof(src_filename), stdin) != NULL) {
					// Remove the newline character if present
					size_t length = strlen(src_filename);
					if (length > 0 && src_filename[length - 1] == '\n') {
						src_filename[length - 1] = '\0';
					}

					// Use src_filename here
					printf("Source filename is: %s\n", src_filename);
				}
				else {
					printf("Error reading input.\n");
					return 1;
				}
#endif
			}

			// Add .src extension if none is present
			if (strchr(src_filename, '.') == 0) strcat_s(src_filename, sizeof(src_filename), ".src");

			printf("\n%s\n", src_filename);

			size_t len = strlen(src_filename);

			if (len < 4 || _stricmp(src_filename + len - 4, ".SRC") != 0) {
				printf("File must end with '.SRC'\n");
				hit_Enter();
				exit(1);
			}

			if (strchr(src_filename, '\\')) //If dir path designated
			{
				//Change directory path to SG dir
				strcpy_s(pathstr, sizeof(pathstr), src_filename);
				ptr = strrchr(pathstr, '\\');
				*++ptr = '\0';
				printf("\nDirectory path: %s\n", pathstr);
				if (_chdir(pathstr)) error("chdir failed");

				//remove path from src_filename
				ptr = src_filename;
				ptr2 = strrchr(src_filename, '\\') + 1;
				do
					*ptr++ = *ptr2++;
				while (*ptr2);
				*ptr = '\0';
			}
		}//end of if (verif_flg == 0)


		printf("Source file: %s -- ", src_filename);

		if (fopen_s(&stream, src_filename, "r") != 0) {
			printf("Can't open %s\n", src_filename);
			perror("Error opening file");
			hit_Enter();
			exit(1);
		}


		txtsiz = fread(txtbuf, 1, MAX_TXT_SIZ, stream);// Read SRC file into buffer
		fclose(stream);

		if (txtsiz == MAX_TXT_SIZ) {
			puts("File is too long");
			hit_Enter();
			exit(1);
		}
		printf("%zi bytes\n", txtsiz);


		/********************* Output Files *********************/

		if (verif_flg == 0) {

			strcpy_s(lstfilename, sizeof(lstfilename), src_filename);

			char* dot_position = strchr(lstfilename, '.');
			strcpy_s(dot_position, strlen(".LIS") + 1, ".LIS");

			strcpy_s(loafilename, sizeof(loafilename), "AUTORUN.SYS");
			strcpy_s(dsk_filename, sizeof(dsk_filename), "temp.atr");

#if DELETE_TEMP_ATR

			if (remove(dsk_filename) == 0) {
				printf("%s deleted successfully.\n", dsk_filename);
			}
			else {
				printf("%s not found.\n", dsk_filename);
			}
#endif

		}//end of if (verif_flg == 0)

		/********************** Now work on buffer *********************/

		txtendptr = txtbuf + txtsiz;

		symnum = 0;
		*outflgs = 0;

		mismatch_flg = 0;
		find_symad_flg = 1;
		maclevel = 0;
		//*emul_filename = 0;
		//*dsk_filename = 0;
		addtoimg_filename[0][0] = 0;
		append_filename[0][0] = 0;
		linkfile_filename[0] = 0;
		smbfilename[0] = 0;

		if (verif_flg == 2) {
			//cpfflg = 1;
			if (fopen_s(&stream, cmp_file, "rb") != 0)
				file_err("Can't open CPFL file", cmp_file);
		}

		for (i = 0; i < MAXMACLEV; i++)
			lclbnum[i] = 0;


		for (ptr = txtbuf; ptr < txtendptr; ptr++)
			if (*ptr == '\r') {
				if (*(ptr + 1) == '\n')
					*ptr = ' ';
				else
					*ptr = '\n';
			}
		*txtendptr++ = '\n';
		*txtendptr++ = '\n';
		*txtendptr++ = '\0';



		/********************** Main loop *******************/


		for (pass = 1; pass <= 2; pass++) {
#if ATARI800
			sgndtbptr1 = sgndtbptr2 = sgndtb;
#endif
			printf("\n  pass %d\n", pass);
			linptr = txtptr = txtbuf;
			xlbfptr = xlbuf;
			orgflg = 0;
			loclabnum = 0;
			macroflg = 0;
			donum = 0;
			loopnum = 0;
			minusnum = 0;
			plusnum = 1;
			skipnum = 1;
			contnum = 1;
			endifnum = 1;

			symnumext = symnum;
			include_flg = 0;

			

			for (linnum = 0; !((*txtptr == 0) && (include_flg == 0)); txtptr = linbegptr + linlen) {

				if ((*txtptr == 0) && (include_flg)) {
					linnum = linnum_sav;
					txtptr = txtptr_sav;
					include_flg = 0;
				}

				if (strpbrk(outflgs, "Ll")) {
					if (*prnlnbufbeg || *prnlnbuf) {		// if there's something to print

						/* Pad short lines to full width (unless in do-not-expand-macro mode) */
						if (bytcnt < LSTBYTES) {
							if (!strpbrk(outflgs, "Mm")) {

								for (i = LSTBYTES - bytcnt; i > 0; i--)
									strcat_s(prnlnbufbeg, sizeof(prnlnbufbeg), "   ");
							}
						}
						strcat_s(prnlnbufbeg, sizeof(prnlnbufbeg), "\t");

						/* Append the source line */
						strcat_s(prnlnbufbeg, sizeof(prnlnbufbeg), prnlnbuf);

						fprintf(stream2, "%s", prnlnbufbeg);	//print line to LIS file
						
						if (cpfflg)
							if (mismatchln_flg[0])
								fprintf(stream2, "%s\n", cmpflbytln[0]);

						if (bytcnt > LSTBYTES) {

							for (i = 1; i <= prnlnum; i++) {
								fprintf(stream2, "%s\n", prnlnbuf2[i - 1]);
								if (cpfflg)
									if (mismatchln_flg[i])
										fprintf(stream2, "%s\n", cmpflbytln[i]);
							}
						}
						//fprintf (stream2,"%s\n",linbuf[maclevel]); //
						*prnlnbuf = '\0';
						*prnlnbufbeg = '\0';
						*prnlnbuf2[0] = '\0';
						bytcnt = 0;
						prnlnum = 0;
						data_flg = 0;

						for (i = 0; i < 10; i++)
							mismatchln_flg[i] = 0;
					}
				}

				if (!maclevel) linnum++;				//no change of line number if macro
				
				if (label_only_flg == 0){
					*label = '\0';
					labflg = 0;
				}
				*operand = '\0';
				linbegptr = linptr = txtptr;			//set line begin to txtptr
				lnbfptr = linbuf[maclevel];
				linlen = (short)(strchr(linbegptr, '\n') + 1L - linbegptr);	//find linlen

				if (!maclevel && pass == 1)	// Test line for illegal characters.
				{
					for (cnt = 1; cnt <= linlen; cnt++) {
						if (*linptr & 128) //character is inverted
						{
							linbegptr[linlen] = 0;
							printf("\a\nIllegal character at line %i:\n", linnum);
							puts(linbegptr);
							hit_Enter();
							exit(1);
						}
						linptr++;
					}
					linptr = linbegptr;
				}

				while (*linptr != '\n') {
					/* Determine and place local label in macro */

					*lnbfptr = 0;
					if ((*linptr == '@') && maclevel) {
						ptr = linptr - 1;
						cnt = 0;
						while (isalnum(*ptr))		// Find beginning of local label.
						{
							ptr--;
							cnt++;
						}
						if (strchr(", \n+-", *ptr))	// Characters that can precede a local label!!!!!!!
						{
							if (cnt > 4)
								error("Local labels cannot be longer than 5 characters");
							ptr++;
							labptr = lab;
							while (cnt) {
								cnt--;
								*labptr++ = *ptr++;
							}
							*labptr = 0;

							for (loclabtbpos = 0; loclabtbpos < loclabnum; loclabtbpos++)
								if (strcmp(loclabtb[loclabtbpos], lab) == 0) break; // local label found
							if (loclabtbpos == loclabnum) // label into loclabtb
							{
								strcpy_s(loclabtb[loclabnum], sizeof(loclabtb[loclabnum]), lab);
								labexttab[loclabnum++] = 0;
							}
							cnt = 0;
							while (cnt < lclbnum[maclevel]) {
								if (lclb[maclevel][cnt] == loclabtbpos)
									break;
								cnt++;
							}
							if (cnt == lclbnum[maclevel]) {
								lclb[maclevel][cnt] = loclabtbpos;
								labexttab[loclabtbpos]++;
								lclbnum[maclevel]++;
							}
							// Add four digits to label
							sprintf_s(lnbfptr, MAX_LINE_SIZE, "%04d", labexttab[loclabtbpos]);
							lnbfptr += 4;
							linptr++;
						}
					}

					/********** Dummy in macro ***********/

					if ((*linptr & 128) && !macroflg)   // inverted characters
					{
						ptr = macargsptr[maclevel];
						cnt = *linptr & 127;
						while (cnt) {
							while (*ptr != ',')
								ptr++;
							ptr++;
							cnt--;
						}
						if (strncmp(lnbfptr - 4, "ASC ", 4) == 0) {
							ch = *ptr;
							while (*ptr != '\n') {
								*lnbfptr++ = *ptr++;
								if (*ptr == ch) {
									*lnbfptr++ = *ptr++;
									break;
								}
							}
						}
						else
							while (strchr(",\t\n ", *ptr) == 0)
								*lnbfptr++ = *ptr++;
						while (*linptr & 128)
							linptr++;
					}
					else	// character not inverted
						if (*linptr != '\n') *lnbfptr++ = *linptr++;
				}

				if ((!macroflg) && maclevel)
					txtptr = linbuf[maclevel];

				*lnbfptr++ = '\n';
				*lnbfptr = '\0';


				/**********  Test first character of line ***********/

				switch (*txtptr) {
				case '?':
				case '\n':					// empty line
				case ';':	continue;
				case '*':	star_flg = 1;	// comment
					continue;
				case ' ':					// white space
				case '\t':	break;
				case '#':
					// Skip block from this '#' to the next line that begins with '#',
					// or to the end of the text if no second '#' exists.
					do {
						txtptr = strchr(txtptr, '\n');
						linnum++;

						if (txtptr == NULL) {
							// Reached end of string/file without finding another '#'
							txtptr = "";          // or set to the end of your buffer
							break;
						}

						txtptr++;                 // move past the '\n'
					} while (*txtptr != '#');
					linlen = 1;

					if (*(txtptr + 1) == '#') {	// ## indicates section omitted from assembly
						txtptr++;
						linlen = 2;
					}// else # indicates section omitted from assembly
					linbegptr = txtptr;

					continue;

				case  '-':
					strcpy_s(label, sizeof(label), "minus");
					txtptr++;
					break;
				case  '+':
					strcpy_s(label, sizeof(label), "plus");
					txtptr++;
					break;
				default:				// label
					pos = 0;
#if ATMAS_II_COMPATIBLE_ONLY
					if (!isalpha(*txtptr)) {
						printf("%c is not valid", *txtptr);
						error("Label must begin with an alphabetic character.");
					}
					while (isalnum(*txtptr)) {
						label[pos++] = (char)(*txtptr++);
					}
#else  // !ATMAS_II_COMPATIBLE_ONLY
					if (!isalpha(*txtptr) && strchr("._", *txtptr) == NULL) {
						printf("%c is not valid", *txtptr);
						error("Label must begin with an alphabetic character or '.' or '_'.");
					}
					while (isalnum(*txtptr) || strchr("._", *txtptr)) {
						label[pos++] = (char)(*txtptr++);
					}
#endif
					if (pos > MAX_SYMBOL_SIZE)
						error("Label is too long");
					if (*txtptr == '@')
						label[pos++] = *txtptr++;
					if (!isspace(*txtptr))
						error("Illegal character at end of label");

					label[pos] = 0;

					sprintf_s(labelstr, MAX_LABEL_SIZE, " %s ", label);

					if (strlen(label) == 3) {
						unsigned char lab[4];
						lab[0] = (unsigned char)toupper(label[0]);
						lab[1] = (unsigned char)toupper(label[1]);
						lab[2] = (unsigned char)toupper(label[2]);
						lab[3] = '\0';
						if (strstr((const char*)opcds, (const char*)lab) != NULL)
							error("You cannot have an opcode at the beginning of a line");
					}
					if (strstr(assdirecs, labelstr) || strstr(assdirecssolo, labelstr))
						error("This is an assembler directive. It can't be at the beginning of a line");
					break;
				}

				label_only_flg = 0;

				/*	********* find opcode ****************************/

				while ((*txtptr == ' ') || (*txtptr == '\t'))
					txtptr++;
#if !ATMAS_II_COMPATIBLE_ONLY
				if (*label && ((*txtptr == '\n') || (*txtptr == ';'))) {// label on empty line
					
					label_only_flg = 1;
					continue;
				}

#endif
				tpsav = txtptr;
				if (*txtptr == '\n') continue;

#if !ATMAS_II_COMPATIBLE_ONLY
				if (*txtptr == ';') continue;	// comment (ATMAS-II incompatibility)
#endif

				ptr = opcode;
#if ATMAS_II_COMPATIBLE_ONLY
				if (isalpha(*txtptr) == 0)
					error("Alphabetic character expected");

				while (isalnum(*txtptr))
					*ptr++ = (char)(*txtptr++);	//opcode is upper case

#else  // !ATMAS_II_COMPATIBLE_ONLY
				if (isalpha(*txtptr) == 0 && strchr("._", *txtptr) == NULL)
					error("Alphabetic character or '.' or '_' expected");

				while (isalnum(*txtptr) || strchr("._", *txtptr))
					*ptr++ = (char)(*txtptr++);
#endif
				if (!isspace(*txtptr)) {
					printf("Illegal character '%c' in symbol", *txtptr);
					error("Space expected");
				}

				*ptr = 0; //opcode finalised

				/*  Check operand for directives   */

				if (!(*txtptr == ' ') || *(txtptr + 1) == ' ' || *(txtptr + 1) == '\n' || *(txtptr + 1) == '\t') {
					sprintf_s(opcstr, MAX_LABEL_SIZE, " %s ", opcode);
					if (strstr(assdirecs, opcstr))
						error("Directive must be followed by a single space (not tab) and an operand");
				}
				*prnlnbuf = '\0';
				*prnlnbufbeg = '\0';
				if ((pass == 2) && strpbrk(outflgs, "Ll")) {
					if (strcmp(opcode, "MACRO") && strcmp(opcode, "MEND") && !macroflg) /* Not in macro */
					{
						if ((maclevel == 0) || !strpbrk(outflgs, "Mm")) // || macro expanded
						{

							if (   (!strstr("EQU EPZ ORG", opcode) && (strlen(opcode) == 3))  ) {//regular opcode
								if (pc != pc_sav) {

									sprintf_s(prnlnbufbeg, sizeof(prnlnbufbeg), "%4X:", pc);	// PC ADDRESS
								}
								else
									sprintf_s(prnlnbufbeg, sizeof(prnlnbufbeg), "    :");
								pc_sav = pc;
							}
							else sprintf_s(prnlnbufbeg, sizeof(prnlnbufbeg), "     ");

							size_t len = strlen(prnlnbuf);
							size_t rem = sizeof(prnlnbuf) - len;

							if (*label) {
								if (strcmp(label, "minus") == 0) {
									len += snprintf(prnlnbuf + len, rem, "-");
								}
								else if (strcmp(label, "plus") == 0) {
									len += snprintf(prnlnbuf + len, rem, "+");
								}
								else {
									len += snprintf(prnlnbuf + len, rem, "%s", label);
								}
								rem = sizeof(prnlnbuf) - len;
							}

							// always add the tab after the label or whitespace
							len += snprintf(prnlnbuf + len, rem, "\t");
							rem = sizeof(prnlnbuf) - len;

							// indentation
							for (int i = 0; i < maclevel; i++) {
								len += snprintf(prnlnbuf + len, rem, ": ");
								rem = sizeof(prnlnbuf) - len;
							}

							// skip leading whitespace / label and append the rest of the line
							char* p = strpbrk(linbuf[maclevel], " \t");
							if (p) {
								p += strspn(p, " \t");          // skip the whitespace itself
								if (*p)
									snprintf(prnlnbuf + len, rem, "%s", p);
							}
							// if there is no whitespace at all, we simply add nothing more
						}
					}
				}

				/* LABEL AT LINE BEGIN  LABEL AT LINE BEGIN LABEL AT LINE BEGIN LABEL AT LINE BEGIN*/

				if (*label) {
					
					if (pass == 1) {
						if (symnum >= MAXSYMS - 1)
							error("Too many labels");
						if (macnum >= MAXMACSYMS - 1)
							error("Too many labels");
					}

					/* EPZ EQU  LABEL AT LINE BEGIN*/

					if ((strcmp(opcode, "EQU") == 0) || (strcmp(opcode, "EPZ") == 0)) {
						txtptr++;
						i = expresstoi();
						if (pass == 1) {
							if (i != NOTFOUND) {
								strcpy_s(sss[symnum].symbol, sizeof(sss[symnum].symbol), label);
								sss[symnum].symadr = i;
#if SORT_EQU_EPZ
								if ((strcmp(opcode, "EQU") == 0) && (i < 256))
									sss[symnum].symtyp = ']';
								else //if (strcmp(opcode, "EPZ") == 0)
#endif
									sss[symnum].symtyp = '#';


#if COMMENTS2
								i = 0;

								ptr = txtptr;

								while (*ptr && strchr(" \t*;", *ptr)) {
									ptr++;
								}
								while (*ptr != '\n' && i < COMMENT_LEN - 1) {
									sss[symnum].comment[i++] = *ptr++;
								}
								sss[symnum].comment[i] = 0;
#endif
								symnum++;
							}
						}
						continue;
					}


					/* RMB  LABEL AT LINE BEGIN*/			//(Init ving memory bytes)

					if (strcmp(opcode, "RMB") == 0) {
						if (!rmbflg)error("INITRMB value must precede this line");

						if (pass == 1)

						{
							strcpy_s(sss[symnum].symbol, sizeof(sss[symnum].symbol), label);
							sss[symnum++].symadr = rmbadr;
							sss[symnum - 1].symtyp = '#';
						}
						txtptr++;
						rmbadr += expresstoi_p2();

						continue;
					}

					/* MACRO  LABEL AT LINE BEGIN*/

					if (strcmp(opcode, "MACRO") == 0)		// MovAl   MACRO A,N
					{
						if (macroflg) error("MEND missing");
						macroflg++;
						if (pass == 1) {
							find_operand();					// A,N

							operptr = operand;
							dumnum = 0;
							do {
								pos = 0;
								while ((*operptr != ',') && (*operptr != 0))	// macdumtab[0]="A"
									macdumtab[dumnum][pos++] = *operptr++;		// macdumtab[1]="B"
								macdumtab[dumnum][pos] = 0;
								dumnum++;
							} while (*operptr++);

							strcpy_s(macsymbol[macnum], sizeof(macsymbol[macnum]), label);		// macsymbol[macnum]    MovAl
							if (*(txtptr - 1) == '\n') txtptr--;
							macptrtab[macnum++] = strchr(txtptr, '\n') + 1;		// macptrtab[macnum++]     pointer
						}
						continue;
					}

					/*  LABEL AT LINE BEGIN IN CODE */

					else {
						labflg = 1;
						if (strcmp(label, "do") == 0) sprintf_s(label, sizeof(label), "do%u", ++donum);
						if (strcmp(label, "loop") == 0) sprintf_s(label, sizeof(label), "loop%u", ++loopnum);
						if (strcmp(label, "minus") == 0) sprintf_s(label, sizeof(label), "minu%u", ++minusnum);
						if (strcmp(label, "plus") == 0) sprintf_s(label, sizeof(label), "plus%u", plusnum++);
						if (strcmp(label, "skip") == 0) sprintf_s(label, sizeof(label), "skip%u", skipnum++);
						if (strcmp(label, "cont") == 0) sprintf_s(label, sizeof(label), "cont%u", contnum++);
						if (strcmp(label, "endif") == 0) sprintf_s(label, sizeof(label), "endi%u", endifnum++);
#if !ATMAS_II_COMPATIBLE_ONLY
						if (*label == '.') { // local label, must be appended to previous global label
							char temp[MAX_LABEL_SIZE];

							strcpy_s(temp, sizeof(temp), label);

							if (*current_global == '\0')
								error("Local label without preceding global label");

							strcpy_s(label, MAX_LABEL_SIZE, current_global);
							strcat_s(label, MAX_LABEL_SIZE, temp);
						}
						else {
							strcpy_s(current_global, sizeof(current_global), label);
						}
#endif

					}
					if ((pass == 1) && !(macroflg)) {
						strcpy_s(sss[symnum].symbol, sizeof(sss[symnum].symbol), label);

#if COMMENTS
						i = 0;
						if (star_flg) {
							ptr = linbegptr - 2;
							while (*ptr != '\n')
								ptr--;
							ptr++;
							if (*ptr++ == '*') { // comment at line begin
								while (strchr(" \t*", *ptr))
									ptr++;
								while (*ptr != '\n' && *ptr != '*' && i < COMMENT_LEN - 1) {
									sss[symnum].comment[i++] = *ptr++;
								}
								sss[symnum].comment[i] = 0;
								//if (sss[symnum].comment[0]) {
									//printf("%s\t%s\n", sss[symnum].symbol, sss[symnum].comment);
									//getchar();
								//}
							}
						}

#endif
						sss[symnum++].symadr = pc;
					}
				} /*END LABEL AT LINE BEGIN LABEL AT LINE BEGIN LABEL AT LINE BEGIN LABEL AT LINE BEGIN*/

	/* MEND */

				if (strcmp(opcode, "MEND") == 0) {
					if (macroflg)
						macroflg--;
					else {
						if (!maclevel) error("MEND without MACRO");
						lclbnum[maclevel] = 0;
						linbegptr = linbegsav[--maclevel];
						linlen = linlensav[maclevel];
					}
					continue;
				}

				/* MACROFLG */

				if (macroflg) {
					if (pass == 1) {
						find_operand();
						operptr = operand;
						if (*operand) {
							operptr--; txtptr--;
							do {
								operptr++; txtptr++;
								if (*operand == '(') operptr++;

								pos = 0;
								while (isalnum(*operptr)) {
									dumbuf[pos++] = *operptr++;				// dubuf == operand
									txtptr++;
								}
								dumbuf[pos] = 0;
								for (cnt = 0; cnt < dumnum; cnt++) {
									if (strcmp(macdumtab[cnt], dumbuf) == 0) {
										while (pos)
											*(txtptr - pos--) = (char)cnt | 128;
										break;/* out of for loop */
									}
								}
							} while (*operptr);
						}
					}
					continue;
				}

				/* LINKFILE */

				if (strcmp(opcode, "LINKFILE") == 0) {
					if (pass == 2)
						operand_to_filename(linkfile_filename);
					continue;
				}

				/* LINK */

				if (strcmp(opcode, "LINK") == 0) {
					if (pass == 2) {
						if (!linkfile_filename[0])
							error("LINKFILE must be designated before this line");
						operand_to_filename(link_filename[link_cnt++]);
					}
					continue;
				}

				/* APPEND */

				if (strcmp(opcode, "APPEND") == 0) {
					if (pass == 2)
						operand_to_filename(append_filename[append_cnt++]);
					continue;
				}
				/* INCLUDE */

				if (strcmp(opcode, "_INCLUDE") == 0) {
					if (include_flg) error("Can't have 'INCLUDE' in an included file.");
					include_flg = 1;
					operand_to_filename(include_filename);

					printf("\nIncluding %s", include_filename);


					if (fopen_s(&stream3, include_filename, "r") != 0) {
						printf("\nCan't open INCLUDE file %s\n", include_filename);
						perror("");
						hit_Enter();
						exit(1);
					}

					includebuf = txtendptr;
					include_txtsiz = fread(includebuf, 1, MAX_TXT_SIZ - txtsiz, stream3);
					fclose(stream3);

					txtsiz += include_txtsiz;
					if (txtsiz > MAX_TXT_SIZ - txtsiz) error("INCLUDE File is too long");

					printf(": %zi bytes\n", include_txtsiz);

					txtendptr = includebuf + include_txtsiz;

					*txtendptr++ = '\n';
					*txtendptr++ = '\0';
					txtptr_sav = linbegptr + linlen;
					linnum_sav = linnum;

					linbegptr = includebuf;
					linlen = 0;
					linnum = 0;

					continue;
				}

				/* ADDTOIMG  SMTFILE  RUN_EMUL  DSKIMAGE */

				if (pass == 2) {
					if (strcmp(opcode, "ADDTOIMG") == 0) {
						operand_to_filename(addtoimg_filename[addtoimg_cnt++]); continue;
					}
					if (strcmp(opcode, "SMTFILE") == 0) {
						operand_to_filename(smbfilename); continue;
					}
					if (strcmp(opcode, "RUNEMUL") == 0) {
						operand_to_filename(emul_filename); continue;
					}
					if ((strcmp(opcode, "DSKIMAGE") == 0) || (strcmp(opcode, "XFDIMAGE") == 0) || (strcmp(opcode, "ATRIMAGE") == 0)) {
						operand_to_filename(dsk_filename); continue;
					}
				}

				/* SAVEAS */

				if ((strcmp(opcode, "SAVEAS") == 0) || (strcmp(opcode, "BSAVE") == 0)) {
					if (pass == 1)
						operand_to_filename(loafilename);
					continue;
				}


				/* NOHEADER OBJFILE */

				if ((strcmp(opcode, "NOHEADER") == 0) || (strcmp(opcode, "OBJFILE") == 0)) {
					objfile_flag = 1;
					continue;
				}

				/* SMTTOIMG */

				if (strcmp(opcode, "SMTTOIMG") == 0) {
					smttoimg_flg = 1;
					continue;
				}
#if ROUND_TRIP_VERIFICATION
				/* VERIFY */

				if (strcmp(opcode, "VERIFY") == 0) {
					verif_flg = 1;
					continue;
				}
#endif
				/* CPFL */

				if (strcmp(opcode, "CPFL") == 0) {
					if (pass == 2) {
						operand_to_filename(cmp_file);

						if (fopen_s(&stream, cmp_file, "rb") != 0) 
							file_err("Can't open CPFL file", cmp_file);
						else printf("Opening CPFL file %s\n", cmp_file);
						cpfflg = 1;
					}
					continue;
				}



				/* OUT */
				if ((strcmp(opcode, "OUT") == 0)) {
					if (pass == 2) {
						find_operand();
						strcpy_s(outflgs, sizeof(outflgs), operand);

						if (*outflgs && !lstflg) {
							lstflg = 1;
							if (fopen_s(&stream2, lstfilename, "w") != 0) file_err("Can't create LST file", lstfilename);
						}
					}
					continue;
				}

				/* INITRMB */			//(Init reserving memory bytes)

				if (strcmp(opcode, "INITRMB") == 0) {
					txtptr++;
					rmbadr = expresstoi_p2();
					rmbflg = 1;
					continue;
				}

				/* ORG */

				if ((strcmp(opcode, "ORG") == 0)
#if !ATMAS_II_COMPATIBLE
					|| (strcmp(opcode, ".org")) == 0
#endif
					) {
					txtptr++;
					{
						if (!orgflg)											/* first ORG */
						{
							if (*txtptr == '*')
								error("ORG value is undefined");
							orgflg = 1;
						}
						else {
							printf("to %4X\n", pc - 1);		/*write NDAD */
#if ATARI800
							* sgndtbptr1++ = pc - 1;
#endif
						}
						//if (*txtptr == '*')
						sectyp('*');
						pc = expresstoi();

#if ATARI800
						if (!objfile_flag)	// If not an oject file,
						{					// we need the load-file header specific to ATARI.
							pcsv = pc;
							toxlbuf(255);
							toxlbuf(255);
							toxlbuf((char)pcsv);
							toxlbuf((char)(pcsv / 256));
							toxlbuf((char)*sgndtbptr2);
							toxlbuf((char)((*sgndtbptr2++) / 256));
							pc = pcsv;
						}
#endif
						printf("Segment %4X ", pc);
					

					}
					continue;
				}



				/* DFB */

				if (strcmp(opcode, "DFB") == 0
#if !ATMAS_II_COMPATIBLE					
					|| (strcmp(opcode, ".byte") == 0)
#endif						
					) {
					data_flg = 1;
					txtptr++;
					if (*txtptr == '%')
						sectyp('%');
					else if (isalpha(*txtptr)) {
						char* txtptr_save = txtptr;
						char byte = (char)symtoi1();
						if (memchr(esc_ch_tab, byte, 16))
							sectyp(',');		//'?' changed to ','
						else
							sectyp('#');
						txtptr = txtptr_save;
					}
					else if (*txtptr == '\'')
						sectyp(',');		//'?' changed to ','
					else sectyp('#');
					txtptr--;
					do {
						txtptr++;
						toxlbuf((char)expresstoi_p2());
					} while (*txtptr == ',');
					continue;
				}

				/* DFW */ //?????????????????????????????????????

				if (strcmp(opcode, "DFW") == 0
#if !ATMAS_II_COMPATIBLE					
					|| (strcmp(opcode, ".word") == 0)
#endif						
					) {
					char* tmp_ptr;
					data_flg = 1;

					do {
						txtptr++;

						tmp_ptr = txtptr;
						while (isalnum(*tmp_ptr)) tmp_ptr++;
						if (*tmp_ptr == '-' && *(tmp_ptr + 1) == '1' && !isdigit(*(tmp_ptr + 2)))
							sectyp('-');
						else
							sectyp('=');

						symval = expresstoi_p2();
						toxlbuf((char)symval);
						toxlbuf((char)(symval >> 8));
					} while (*txtptr == ',');


					if ((*(txtptr - 2) == '-') && (*(txtptr - 1) == '1'))
						sectyp('-');

					continue;
				}

				/* FILLMEM */

				if (strcmp(opcode, "FILLMEM") == 0) {
					data_flg = 1;
					if (*(txtptr + 1) == '\'') sectyp('"'); else sectyp('#');
					txtptr++;
					cnt = expresstoi_p2();
					if (*txtptr != ',') error("Comma followed by byte value expected");
					txtptr++;
					ch = (char)expresstoi_p2();
					for (i = 0; i < cnt; i++)
						toxlbuf(ch);
					continue;
				}

				/* ASC */

				if ((strcmp(opcode, "ASC") == 0)
#if !ATMAS_II_COMPATIBLE					
					|| (strcmp(opcode, ".ascii") == 0) || (strcmp(opcode, ".text") == 0)
#endif					
					) {// calculated using txtptr, not operand, since operand is not yet set

					data_flg = 1;
					do {
						txtptr++;
						switch (*txtptr++) {
						case '%':								/* screen code  */
							sectyp('$');
							while ((*txtptr != '%') && (*txtptr != '\n')) {
								offset = *txtptr >> 5;
								ch = *txtptr++ & 0x1f;
								ch |= *(intab + offset);
								toxlbuf(ch);
							}
							break;

						case '$':								/* screen code, end inverse  */
							sectyp('$');
							while ((*txtptr != '$') && (*txtptr != '\n')) {
								offset = *txtptr >> 5;
								ch = *txtptr++ & 0x1f;
								ch |= *(intab + offset);
								toxlbuf((char)(ch | 128));
							}
							break;
						case '/':								/* inverse ASC */
							sectyp('"');
							while ((*txtptr != '/') && (*txtptr != '\n'))
								toxlbuf((char)(128 | *txtptr++));
							break;
						case '\\':								/* ASC, end inverse  */
							sectyp('\\');
							for (; ; ) {
								ch = *txtptr++;
								if ((*txtptr == '\\') || (*txtptr == '\n')) break;
								toxlbuf(ch);
							}
							toxlbuf((char)(128 | ch));
							break;
						default:								/* normal ASC: kann use any other non alnum character */
							sectyp('"');
							if (!isalnum(*(txtptr - 1))) {
								ch = *(txtptr - 1);
								while ((*txtptr != ch) && (*txtptr != '\n'))
									toxlbuf(*txtptr++);
							}
							else
								error("invalid string marking");
						}
					} while (*(++txtptr) == ',');
					continue;
				}

				/* Find operand   Find operand  Find operand  Find operand  Find operand  Find operand  Find operand */


				find_operand();


				/* OPCODE IS A MACRO */

				for (i = 0; i < macnum; i++)
					if (strcmp(macsymbol[i], opcode) == 0) break;
				if (i < macnum) {
					linbegsav[maclevel] = linbegptr;
					linlensav[maclevel] = linlen;
					maclevel++;
					macargsptr[maclevel] = txtptr;
					linbegptr = macptrtab[i];
					linlen = 0;
					continue;
				}

				/* OPC without operand */

				cnt = 0;
				if (!(*operand)) {
					if ((cnt = chkopc(opcdat1)) >= 0) /* rts, sec, plp, ...... */
					{
						sectyp(';');
						toxlbuf((char)*(dat1 + cnt));
						continue;
					}
					else {
						if (strlen(opcode) == 3){
							unsigned char op[4];
							op[0] = (unsigned char)toupper(opcode[0]);
							op[1] = (unsigned char)toupper(opcode[1]);
							op[2] = (unsigned char)toupper(opcode[2]);
							op[3] = '\0';
							if (strstr((const char*)opcds, (const char*)op) != NULL) 
								error("Opcode must be followed by a single space (not a tab) and an operand");
						}
						strcpy_s(operand, sizeof(operand), opcode);
						strcpy_s(opcode, sizeof(opcode), "JSR");
						txtptr = tpsav;
					}
				}


				/* JMP */

				if (strcmp(opcode, "JMP") == 0) {
					sectyp(';');
					if (*operand == '(') {
						if (operand[strlen(operand) - 1] != ')') error("Illegal syntax or addressing");
						toxlbuf(108);
					}
					else
						toxlbuf(76);

					operval = expresstoi_p2();


					toxlbuf((char)operval);
					toxlbuf((char)(operval >> 8));
					continue;
				}

				/* Relative Branches (BRA, BCC, BCS, BEQ, BNE, BPL, BMI, BVC, BVS, etc. */

				if ((cnt = chkopc(brastrdat)) >= 0) {
					sectyp(';');
					toxlbuf((char)bradat[cnt]);				// output the correct opcode

					uint32_t target = expresstoi_p2();
					int braoffset = (int)target - (int)(pc + 1);	// signed offset

					if (pass == 2) {
						if (braoffset < -128 || braoffset >127) {
							printf("\nAttempted to branch a distance of %d bytes "
								"(target=0x%X, pc=0x%X). Range is -128 to 127.\n",
								braoffset, target, pc);
							error("Branch out of range");
						}
					}
					toxlbuf((int8_t)braoffset);
					continue;
				}

				/* Subroutine */

				if (strcmp(opcode, "JSR") != 0) { // not a JSR
					if ((cnt = chkopc(opcdat)) < 0)		/* opcode not found, must be subroutine */
					{
						if ((strcmp(opcode, "MACRO") == 0) ||
							(strcmp(opcode, "EPZ") == 0) ||
							(strcmp(opcode, "EQU") == 0))
							error("Label missing at beginning of line"); // since otherwise these would have been taken care of

						if ((pass == 1) && (strstr(assdirecs, opcode)))
							continue;
						printf("\n\nopcode: %s", opcode);
						//error("Undefined macro or opcode, or subroutine can't have an operand");
					}

				}


				/* JSR */

				if (!strcmp(opcode, "JSR")) { // JSR opcode found
					if (strchr(";'!(", section_type) == 0) sectyp(';');
					symval = expresstoi_p2();
					toxlbuf(0x20);
					toxlbuf((char)symval);
					toxlbuf((char)(symval >> 8));
					continue;
				}

				/* opc 'c */

				if (operand[0] == '\'' ) {
					
					size_t len = strlen(operand);

					if (!((len == 2) ||
						(len == 3 && operand[2] == '\''))){ 
						//printf("\n\noperand: %s\noperand length: %zu", operand, len);

						for (size_t i = 0; i <= strlen(operand); i++) {
							printf("operand[%zu] = 0x%02X '%c'\n",
								i,
								(unsigned char)operand[i],
								operand[i] ? operand[i] : '.');
						}

						error("invalid character constant");
					}
					sectyp('\'');
					if (data[cnt][NN] == 0)
						error("direct addressing (') cannot be used with this opcode");
					txtptr++;
					
					ch = *txtptr++;
					if (*txtptr == '\'')
						txtptr++;
					toxlbuf((char)data[cnt][NN]);
					toxlbuf(ch);
					continue;

				}

				/* opc #nn */


				else if (operand[0] == '#') {
					syms_in_expr = 0;
					if (data[cnt][NN] == 0) error("direct addressing (#) cannot be used with this opcode");

					txtptr++;
#if !ATMAS_II_COMPATIBLE_ONLY
					if (operand[1] == '>' || operand[1] == '<')
						txtptr++;
#endif
					intval = expresstoi_p2();
					if (pc == 0x3165) //0x4946
						i = 99;

					if ((operand[1] == '\'') && strlen(operand) == 4)
						sectyp('\'');


					if (operand[1] == '%')							// e.g. LDA #%11000000
						sectyp('!');

					else // all other possibilities

					{

						if (pass == 2) {
							intval_sav = intval;
							lo_fg = 1;

							if (*txtptr == ':')							// e.g. LDA #OKTXT:H
							{											// where OKTXT is any address
								txtptr++;
								if (toupper(*txtptr) == 'H') {
									intval >>= 8;
									lo_fg = 0;
								}
							}
#if !ATMAS_II_COMPATIBLE_ONLY
							else if (operand[1] == '>') {
								intval >>= 8;
								lo_fg = 0;
							}
#endif
							if (lo_fg) {
								if (intval > 255) {	// e.g. LDA #OKTXT:L
									if (first_NNsymbol) {
										sectyp('&');
										first_NNsymbol = false;
									}
									else
										first_NNsymbol = true;
								}

								else if (symfg && syms_in_expr == 1) {			// zero page
									for (i = 0; i < symnum; i++) {
										if (sss[i].symadr == intval && sss[i].symtyp == ']') {
											sectyp('[');
											break;
										}
									}
									if (sss[i].symtyp != ']') {
										sectyp('(');	// taken care of by distinguishing between EQU and EPZ ?
									}
								}
							}
							else //lo_fg == 0
								if (intval_sav > 255) {	// e.g. LDA #OKTXT:L
									if (first_NNsymbol) {
										sectyp('@');
										first_NNsymbol = false;
									}
									else
										first_NNsymbol = true;
								}
						}
					}

					if (strchr("!&@([", section_type) == 0) //if not one of these
						sectyp(';');//The above affact only single lines
					toxlbuf((char)data[cnt][NN]);
					toxlbuf((char)intval);
					continue;
				}

				/***********  Commands in data block  ***************/

				operno = ERROR;

				sectyp(';');

				symval = expresstoi();

				if (symval < 256)// txtptr -> first byte after expression
				{

					if ((*(txtptr - 1) == ')') && (*operand == '(')) txtptr--;	//needed for expresstoi(new)

					switch (*txtptr) {
					case ' ':
					case '\t':
					case '\n':   operno = AA; break;     //OPC without operand
					case ',':
						switch (*(++txtptr)) {
						case 'Y':					//   ,Y
						case 'y':
							if (*(opcode + 2) == 'Y')
								error("Can't have ,Y with this opcode");
							if (*(opcode + 2) == 'X')	//  OPX aa,Y
								operno = AAY;
							else
								operno = AAAAY;		//  OPA $0059,Y
							break;
						case 'X':					//   ,X
						case 'x':
							if (*(opcode + 2) == 'X')
								error("Can't have X after comma with this opcode");
							switch (*(++txtptr)) {
							case ')':			//  OPC (aa,X)
								if (*operand != '(') error("Forward bracket missing");
								operno = BAAXB; break;
							default:
								operno = AAX; break; // OPC aa,X
							}
							break;
						default: error("Must have X or Y after comma -- "); break;
						}
						break;
					case ')':
						if (*operand != '(') error("Forward bracket missing");
						switch (*(++txtptr)) {
						case ',':
							switch (*(++txtptr)) {
							case 'Y':
							case 'y':
								if ((*(opcode + 2) == 'Y') || (*(opcode + 2) == 'X'))
									error("Can't use indirect addressing with this opcode");

								operno = BAABY; break; // OPA (AA),Y
							default:error("Must have Y after comma for indirect addressing"); break;
							}
							break;
						default: error("Comma missing after bracket"); break;
						}
						break;
					default: printf("Unexpected character: '%c' -- ", *txtptr); error("Symbol contains illegal character"); break;
					}

				}


				else /* symval >= 256 */
				{
					if (*operand == '(')
						error("Zero page address expected -- Operand not defined -- Label may be unknown");


					switch (*txtptr) {
					case ' ':
					case '\t':
					case '\n':
					case 13: operno = AAAA; break;
					case ',':
						switch (*(++txtptr)) {
						case 'Y':
						case 'y':
							if (*(opcode + 2) == 'Y')
								error("Can't have ,Y with this opcode");
							operno = AAAAY; break;
						case 'X':
						case 'x':
							if (*(opcode + 2) == 'X')
								error("Can't have ,X with this opcode");
							operno = AAAAX; break;
						default: error("Must have X or Y after ','"); break;
						}
						break;
					default: error("Illegal operand"); break;
					}
				}



				if (operno == ERROR)
					error("Illegal addressing");
				if (pass == 2)
					if (*xlbfptr != (char)data[cnt][operno]) {
						printf("\n\nOpcode mismatch %X <> %X", 0xff & (*xlbfptr), 0xff & data[cnt][operno]);
						error("This zero-page label must be defined before present line.");
					}
				if (cnt < 0 || operno < 0 || operno >= 13) {
					error("Index out of bounds in data array");
				}
				else if (data[cnt][operno] == 0) error("This addressing is not possible with this opcode");
				toxlbuf((char)data[cnt][operno]);		// opcode command
				toxlbuf((char)symval);					// address low byte

				switch (operno)								// now address high byte (if applicable)
				{
				case AAAA:
				case AAAAX:
				case AAAAY:
					toxlbuf((char)(symval >> 8));
					break;
				}



			}// end toxlbuf  end toxlbuf end toxlbuf end toxlbuf end toxlbuf end toxlbuf end toxlbuf end toxlbuf
			printf("to %4X\n", pc - 1);

			if (pass == 1) {
				puts("Checking symbols ...");

				for (i = 0; i < macnum; i++) {
					if (symnum >= MAXSYMS - 1) err("Too many labels");
					if (macptrtab[i])
						sss[symnum].macf = MACRO;
					else
						sss[symnum].macf = MACRO_UNUSED;
					strcpy_s(sss[symnum++].symbol, sizeof(sss[symnum].symbol), macsymbol[i]);

				}

				bool d40f_found = 0;

				for (i = 0; i < symnum; i++) {
					if (sss[i].symadr == 0xd40f)
						d40f_found = 1;
				}
				if (d40f_found == 0) {
					sss[symnum].symtyp = '#';
					strcpy_s(sss[symnum].symbol, sizeof(sss[symnum].symbol), "NMIST");
					sss[symnum++].symadr = 0xd40f;
				}


				qsort(sss, symnum, sizeof(ssstype), strcmp);


				for (i = symnum - 1; i > 1; i--) {
					if (strcmp(sss[i].symbol, sss[i - 1].symbol) == 0) {
						printf("\nSame label twice -\n");
						list_label(i - 1);
						list_label(i);
						twiceflg = 1;
					}
				}
			}

			if (twiceflg) {
				hit_Enter();
				exit(1);
			}
#if ATARI800
			* sgndtbptr1 = pc - 1;

#endif



		} // END PASSES END PASSES END PASSES END PASSES END PASSES END PASSES END PASSES END PASSES


		if (verif_flg == 0 || verif_flg == 1) {

			qsort(sss, symnumext, sizeof(ssstype), ssscmpadr);


#if SORT_EQU_EPZ

			int scan;
			int insert = 0;
			int lastzp = 0;
			ssstype ssstemp;

			while (sss[lastzp].symadr <= 0xFF)
				lastzp++;

			if (lastzp > 0)
				lastzp--;

			for (scan = 0; scan <= lastzp; scan++) {

				if (sss[scan].symtyp == ']') {

					ssstemp = sss[scan];

					for (i = scan; i > insert; i--)
						sss[i] = sss[i - 1];

					sss[insert++] = ssstemp;
				}
			}

#endif // 

			/* Remove unneeded symbols */

			puts("");
			symnum = 0;
			for (i = 0; i < symnumext; i++) {
				if (sss[i].macf == MACRO) continue;
				if (symnum > 0) tmptyp = sss[symnum - 1].symtyp;
				if (((sss[i].macf == UNUSED) || (sss[i].macf == MACRO_UNUSED)) && sss[i].symbol[0])
					printf("Unused symbol: %4X %s\n", sss[i].symadr, sss[i].symbol);

				if (symnum > 0) {
					if (sss[i].symadr == sss[symnum - 1].symadr)	/* double entry */
					{
						if (sss[symnum - 1].symbol[0] && sss[i].symbol[0]) /* 2 labels */
						{
							if ((sss[symnum - 1].macf == MACRO_LAB) || (sss[i].macf == MACRO_LAB)) {
								if (sss[symnum - 1].macf == MACRO_LAB)

									sss[symnum - 1] = sss[i];
								continue;
							}

							else
								printf("%s and %s both = $%X\n", sss[i].symbol, sss[symnum - 1].symbol, sss[i].symadr);
						}
						if (!(sss[symnum - 1].symbol[0])) // no symbol
							sss[symnum - 1] = sss[i];

						if (sss[i].symtyp == 0)
							sss[symnum - 1].symtyp = tmptyp; //restore type
						continue;
					}
				}
				sss[symnum++] = sss[i];
			}








			/* Symbol table */
			if (strpbrk(outflgs, "Nn")) {
				fprintf(stream2, "\n\n**** Symbol table *****\n\n");
				for (i = 0; i < symnumext; i++) {
					if (sss[i].symbol)
						list_label_f(i);
				}
			}

			/* xasm Symbol table. Call using "yr xasm.lab" in debugger console */
			if (strpbrk(outflgs, "Xx")) {
				if (fopen_s(&stream4, "xasm.lab", "w") != 0) file_err("Can't create LST file", "xasm.lab");
				
				for (i = 0; i < symnumext; i++) {
					if ((sss[i].symbol[0]) && (sss[i].symtyp != ']')) {
						
						fprintf(stream4, "%04X  %s\n", sss[i].symadr, sss[i].symbol);
						
					}
				}
				fclose(stream4);
			}

			/* SOURCGEN symbol table */
			if (strpbrk(outflgs, "Tt")) {
				if (fprintf(stream2, "\n\n**** SOURCGEN symbol table *****\n\n") < 0)
					perror("Error writing to file");
				for (i = 0; i < symnumext; i++) {
					if (sss[i].symtyp)
						fprintf(stream2, "%c ", sss[i].symtyp);
					else fprintf(stream2, "  ");
					fprintf(stream2, "%d ", sss[i].ofst);
					//if (sss[i].comment[0])
						//fprintf(stream2, "*** %s\n", sss[i].comment);
					list_label_f2(i);

				}
			}
			if (lstflg) fclose(stream2);


			fclose(stream);
			/* APPEND  APPEND APPEND APPEND APPEND APPEND APPEND APPEND APPEND APPEND APPEND APPEND*/
			if (*append_filename) {
				txtsiz = xlbfptr - xlbuf;

				for (i = 0; i < append_cnt; i++) {
					printf("Appending %s -- ", append_filename[i]);
					if (fopen_s(&stream, append_filename[i], "rb") != 0) file_err("Can't find", append_filename[i]);
					if ((txtsiz = fread(xlbfptr, 1, MAX_BIN_SIZ - (xlbfptr - xlbuf), stream)) == 0) {
						printf("file size: %zi\n", txtsiz);
						file_err("Can't read", append_filename[i]);
					}
					printf("file size: %zi\n", txtsiz);
					xlbfptr += txtsiz;
					fclose(stream);
				}
			}


			/* LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA */
			if (fopen_s(&stream, loafilename, "wb") != 0) {
				puts("Can't open LOA file.");
				perror("");
				hit_Enter();
				exit(1);
			}

			printf("\nSaving binary file %s\n", loafilename);
			txtsiz = fwrite(xlbuf, 1, xlbfptr - xlbuf, stream);
			if (txtsiz != xlbfptr - xlbuf)
				file_err("Can't save", loafilename);
			printf("%zi bytes\n", txtsiz);
			fclose(stream);

			/* Link Link Link Link Link Link Link Link Link Link Link Link Link Link Link Link */
			if (*linkfile_filename) {
				xlbfptr = xlbuf;

				for (i = 0; i < link_cnt; i++) {
					printf("Linking %s -- ", link_filename[i]);
					if (fopen_s(&stream, link_filename[i], "rb") != 0) file_err("Can't find", link_filename[i]);
					txtsiz = fread(xlbfptr, 1, MAX_BIN_SIZ - (xlbfptr - xlbuf), stream);
					if (txtsiz == 0)
						file_err("Can't read", link_filename[i]);

					printf("file size: %zi\n", txtsiz);
					xlbfptr += txtsiz;
					fclose(stream);
				}
				if (fopen_s(&stream, linkfile_filename, "wb") != 0) file_err("Can't open", linkfile_filename); /* write to LINKFILE */

				printf("\nSaving binary file %s\n", linkfile_filename);
				txtsiz = fwrite(xlbuf, 1, xlbfptr - xlbuf, stream);
				if (txtsiz != xlbfptr - xlbuf)
					puts("Write error");
				printf("%zd bytes\n", txtsiz);
				fclose(stream);
			}


			/* SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT SMT */


			if (*smbfilename == 0) {
				strcpy_s(smbfilename, sizeof(smbfilename), loafilename);

				char* dot_position = strchr(smbfilename, '.');
				if (dot_position != NULL) {
					strcpy_s(dot_position, strlen(".SMT") + 1, ".SMT");
				}
				else
					strcat_s(smbfilename, sizeof(smbfilename), ".SMT");
			}


			if (fopen_s(&stream, smbfilename, "wb") != 0) {
				perror("Error opening file");
				hit_Enter();
				exit(1);
			}

			printf("Saving symbol-file %s\n", smbfilename);

			fputc(0xfb, stream);
			fputc(0x00, stream);
			fputc(0x00, stream);
			fputc(0xff, stream);
			fputc(0xff, stream);
			fgetpos(stream, &pos1);
			fputc(0, stream);
			fputc(0, stream);
			secsiz = 0;
			fgetpos(stream, &pos2);
			for (i = 0; i < symnumext; i++) {
				lensv = fptsmb(i);
				secsiz += lensv;
			}
			fsetpos(stream, &pos1);
			fputc((char)secsiz, stream);
			fputc((char)(secsiz >> 8), stream);

			fclose(stream);

			//end Save SMT



			/******************* Save to image ************************************/

			if (*dsk_filename) {
				//printf("\nXFDIMAGE: %s\n\n",dsk_filename);

				if (!linkfile_filename[0])
					file_to_image(loafilename, dsk_filename);
				else
					file_to_image(linkfile_filename, dsk_filename);

				if (smttoimg_flg)
					file_to_image(smbfilename, dsk_filename);

				if (*addtoimg_filename)								/* ADDTOIMG */
				{
					for (i = 0; i < addtoimg_cnt; i++) {
						file_to_image(addtoimg_filename[i], dsk_filename);
					}
				}
			}
			// end Save to image

			/****************** Result of comparison with LOA file made in pass2 ********************/
		}
		//else {//verif_flg == 1;
			if (cpfflg || verif_flg == 2) {
				if (verif_flg == 2)
					printf("\nThe reassembled code was compared with %s\n", cmp_file);
				else
					printf("\nThe generated LOA file was compared with %s\n", cmp_file);
				if (mismatch_flg) {
					puts("");

					puts("  ##          ##");
					puts("    ##      ##");
					puts("      ##  ##");
					printf("\a        ##         MISMATCH at address $%X, line %i\n", mismatch_pc - 1, linnum);
					puts("      ##  ##           See '******' in LIS file");
					puts("    ##      ##");
					puts("  ##          ##");
					hit_Enter();
					if (cpfflg) exit(1);
				}
				else {
					cpfflg = 0;
					puts("");

					puts("      o          k       k");
					puts("    o   o        k     k");
					puts("  o       o      k   k");
					puts(" o         o     k k                They are the same.");
					puts("  o       o      k   k");
					puts("    o   o        k     k");
					puts("      o          k       k");

				}

			}
			// end Compare with LOA file

//			hit_Enter();


//			return 0;
		//}//end verif_flg == 2
#if DISASSEMBLE
		if (verif_flg == 0 || verif_flg == 1)
			disassemble(loafilename);
#endif

		if (verif_flg == 0 || verif_flg == 2) {
			//	free(txtbuf);
			//	free(xlbuf);
			//	free(xlram);

					/*************Run Atari800 Emulator **************************************************/

			if (*emul_filename) {
				strcat_s(emul_filename, sizeof(emul_filename), " ");
				strcat_s(emul_filename, sizeof(emul_filename), dsk_filename);
				printf("\nStarting emulator--\n%s\n\n", emul_filename);
				printf("Please close the emulator before re-running Codegen.\n");

				if (system(emul_filename)) puts("Emulator not found");
			}
			//hit_Enter();


			return 0;
		}// end Run Atari800 Emulator

			/***************************************************************/

#if ROUND_TRIP_VERIFICATION
		if (verif_flg == 1) {
			verif_flg = 2;
			strcpy_s(src_filename, sizeof(src_filename), "AUTORUN.SRC");
			strcpy_s(cmp_file, sizeof(cmp_file), loafilename);
		}
	}//end of long loop for verif_flg == 2
#endif	

	hit_Enter();


	return 0;
}

/****************************************+*********************/

/******************* F U N C T I O N S ***********************/

/*************************************************************/




uint32_t expresstoi() {
	char infix[50] = { 0 }, postfix[50] = { 0 };
	char stack[30] = { 0 };
	int intstack[30] = { 0 };
	int symval_list[30] = { 0 };

	char notfound = 0;
	//char temp = 0;
	char symval_cnt = '\0';
	unsigned int i = 0, j = 0, k = 0;
	int tos = 0;
	int aa, bb;
	int result = 0;

	size_t length;

	char* tptrsv = txtptr;
	char* ptr = infix;

	// tokenise ************

	if (*operand == '(')      //because txtptr++ if beginning bracket (multiple beginning brackets?!!!!!!)
		*ptr++ = '(';

	if ((*txtptr == '-') || (*txtptr == '+'))	// "- " is DO, "+ " is CONT

		if (isspace(*(txtptr + 1)))
			return (symtoi1());
	do {
		switch (*txtptr) {
		case '+':
		case '-':
			if ((tptrsv == txtptr) || (*(txtptr - 1) == '(')) {
				symval_list[symval_cnt] = 0;
				*ptr++ = symval_cnt++ + 'A';
			}
		case '/':
		case '*':
		case ')':
		case '(':
			*ptr++ = *txtptr++;
			break;
		case '\'':           /* Character constant */

			txtptr++;        // skip opening quote

			if (*txtptr == '\0')
				error("Missing character after opening quote");

			symval_list[symval_cnt] = (unsigned char)*txtptr++;

			if (*txtptr != '\'')
				error("Missing closing quote");

			txtptr++;        // skip closing quote

			*ptr++ = symval_cnt++ + 'A';
			break;

		default:
			if ((symval_list[symval_cnt] = symtoi1()) == NOTFOUND) notfound = 1;
			*ptr++ = symval_cnt++ + 'A';
		}
	} while (!(isspace(*txtptr) || (*txtptr == ',') || (*txtptr == ':')));

	if (*txtptr == ',')					//if ",X)"
		if (toupper(*(txtptr + 1)) == 'X')
			if (*(txtptr + 2) == ')')
				if (isspace(*(txtptr + 3)))
					*ptr++ = ')';			// add ')'

	*ptr = '\0';


	// interpret '*'  *******

	ptr = strchr(infix, '*');
	while (ptr) {
		if (!isalpha(*(ptr + 1)) && (*(ptr + 1) != '(')) {
			symval_list[symval_cnt] = pc;
			*ptr = symval_cnt++ + 'A';
		}

		ptr = strchr(++ptr, '*');
	}

	//convert to postfix  *********

	i = j = k = 0;
	length = 0;

	length = strlen(infix);
	for (i = 0; i < length; i++) {
		if (!strchr("+-*/()", infix[i]))		//Numbers are added to the postfix string
			postfix[j++] = infix[i];

		else//If an operator or a bracket is encountered...
		{
			if (tos == 0) //If the stack is empty, the operator is added to it

				stack[tos++] = infix[i];

			else //stack not empty
			{       //Operators are pushed or popped based on the order of precedence
				if (infix[i] != ')' && infix[i] != '(') {
					if (prec(infix[i]) <= prec(stack[tos - 1]))
						postfix[j++] = stack[--tos];
					stack[tos++] = infix[i];
				}
				else {
					if (infix[i] == '(')
						stack[tos++] = infix[i];

					if (infix[i] == ')') {
						char temp = stack[--tos];
						while (temp != '(') {
							postfix[j++] = temp;
							if (tos == 0) error("Missing opening parenthisis");
							temp = stack[--tos];
						}
					}
				}

			}

		}
		postfix[j] = '\0';

	}

	while (tos != 0) {
		postfix[j++] = stack[--tos];
	}
	postfix[j] = '\0';


	// convert postfix to int ***************

	tos = 0;
	ptr = postfix;
	while (*ptr) {
		if (isalpha(*ptr)) {
			if ((*ptr - 'A') < 0) {
				printf("Program error at codegen line %d -- invalid negative argument--", __LINE__ + 4);
				error("invalid argument");
			}
			intstack[tos++] = symval_list[*ptr - 'A'];
		}
		else {
			if (tos <= 0) {
				printf("Program error at codegen line %d -- Buffer overrun due to negative argument", __LINE__ + 4);
				error("invalid argument");
			}
			bb = intstack[--tos];
			aa = intstack[--tos];

			switch (*ptr) {
			case '+': intstack[tos++] = aa + bb; break;
			case '-': intstack[tos++] = aa - bb; break;
			case '*': intstack[tos++] = aa * bb; break;
			case '/': intstack[tos++] = aa / bb; break;
			default:
				if (*ptr == '(') error("Missing closing parenthisis");
				printf("infix: %s   -Character '%c' in %s  operand is %s", infix, *ptr, postfix, operand);
				error("Illegal character in postfix expression");
				break;
			}
		}
		ptr++;
	}

	if (strstr(opcode, "),Y")) txtptr--;

	if (tos <= 0) {
		printf("Program error at line %d -- Buffer overrun due to negative argument", __LINE__ + 4);
		error("invalid argument");
	}
	result = intstack[--tos];

	//		if (tos) error ("Illegal syntax in expression");
	if (tos) {

		printf("\n\noperand: %s   infix: %s   postfix: %s", operand, infix, postfix);
		error("Illegal syntax. Can't resolve expression");
	}

	if (notfound) result = NOTFOUND;

#if SHOW_POSTFIX
	if (pass == 2) {
		if (linnum_sav2 != linnum)
			printf("line %4d  ", linnum);
		else
			printf("           ");
		printf("%s  %s  %d  $%x\n", infix, postfix, result, result & 0xFFFF);

		linnum_sav2 = linnum;
	}
#endif

	return (result);
}


int prec(char symbol) {
	if (symbol == '(')
		return 0;
	if (symbol == ')')
		return 0;
	if (symbol == '+' || symbol == '-')
		return 1;
	if (symbol == '*' || symbol == '/')
		return 2;
	return 0;
}
// end expresstoi 

/********************* expresstoi_p2 ****************************/

uint32_t expresstoi_p2() {
	unsigned int val;

	find_symad_flg = 0;
	val = expresstoi();
	find_symad_flg++;
	return(val);
}
// end expresstoi_p2

/*********** symtoi1   converts symbol or number to integer (return value begins at txtptr.
					  When done, txtptr is the address directly after the location of symbol.
					  If evaluation is not possible, the value returned is NOTFOUND
					  */
uint32_t symtoi1() {
	char strbeg[MAX_LABEL_SIZE] = { 0 };
	char* ptr;
	char ch = 0;
	unsigned int i = 0, chrcnt = 0;
	ssstype* sssptr;

	ptr = strbeg;
	ch = *txtptr;
	symfg = 0;



	if (ch == '*')					/* *:program counter) */
	{
		txtptr++;
		return(pc);
	}
	if (isdigit(ch))				/*  :Decimal */
	{
		while (isdigit(*txtptr))
			*ptr++ = *txtptr++;
		*ptr = '\0';
		return(atoi(strbeg));
	}
	if (ch == '%')					/*  %:Binary */
	{
		txtptr++;
		if (!(*txtptr == '0' || *txtptr == '1')) error("Binary digit 0 or 1 expected.");
		ch = 0;
		chrcnt = 1;
		while (*txtptr == '0' || *txtptr == '1') {
			if (chrcnt++ > 8) error("Too many binary digits. Maximum number of digits is 8. e.g. %10010011");
			ch <<= 1;
			ch += *txtptr++ - '0';
		}
		return(ch);
	}
	if (ch == '$')					/*  $:Hex */
	{
		txtptr++;
		if (isxdigit(*txtptr) == 0) error("Hexidecimal digit expected");
		chrcnt = 1;
		while (isxdigit(*txtptr)) {
			if (chrcnt++ > 4) error("Too many hexdecimal digits. Maximum number of digits is 4 eg. $1234.");
			*ptr++ = *txtptr++;
		}
		*ptr = '\0';
		if (sscanf_s(strbeg, "%x", &i) < 0) err("scanning string.");
		return(i);
	}
	if (*txtptr == '\'')			/*  ':ASCII character */
	{
		txtptr++;
		ch = *txtptr++;
		if (*txtptr == '\'')
			txtptr++;
		return(ch);
	}

	switch (ch) {
	case '+': strcpy_s(strbeg, sizeof(strbeg), "plus"); break;
	case '-': strcpy_s(strbeg, sizeof(strbeg), "minus"); break;
	default: //label
		if (isspace(*txtptr)) error("Ilegal space, tab, or end of line");
#if ATMAS_II_COMPATIBLE_ONLY
		if (!(isalpha(*txtptr))) 
			error("Label must begin with a letter");
		while (isalnum(*txtptr))
			*ptr++ = (char)(*txtptr++);
		*ptr = '\0';//label
#else
		if (!(isalpha(*txtptr) || *txtptr == '.')) 
			error("Label must begin with a letter or period");
		while (isalnum(*txtptr) || *txtptr == '.' || *txtptr == '_')
			*ptr++ = *txtptr++;

		*ptr = '\0';//label
#endif
	}

	//printf ("opcode: %s\tstrbeg: %s\n",operand,strbeg);

	if (pass == 1) {
		if (find_symad_flg)
			return(symtoi3(strbeg));
	}
	else /* pass == 2 */
	{
		if (strcmp(strbeg, "do") == 0) sprintf_s(strbeg, sizeof(strbeg), "do%u", donum);
		if (strcmp(strbeg, "loop") == 0) sprintf_s(strbeg, sizeof(strbeg), "loop%u", loopnum);
		if (strcmp(strbeg, "minus") == 0) sprintf_s(strbeg, sizeof(strbeg), "minu%u", minusnum);
		if (strcmp(strbeg, "plus") == 0) sprintf_s(strbeg, sizeof(strbeg), "plus%u", plusnum);
		if (strcmp(strbeg, "skip") == 0) sprintf_s(strbeg, sizeof(strbeg), "skip%u", skipnum);
		if (strcmp(strbeg, "cont") == 0) sprintf_s(strbeg, sizeof(strbeg), "cont%u", contnum);
		if (strcmp(strbeg, "endif") == 0) sprintf_s(strbeg, sizeof(strbeg), "endi%u", endifnum);

#if !ATMAS_II_COMPATIBLE_ONLY
		if (*strbeg == '.') { // local label, must be appended to previous global label
			char temp[MAX_LABEL_SIZE];

			strcpy_s(temp, sizeof(temp), strbeg);

			if (*current_global == '\0')
				error("Local label without preceding global label");

			strcpy_s(strbeg, MAX_LABEL_SIZE, current_global);
			strcat_s(strbeg, MAX_LABEL_SIZE, temp);
		}
#endif

		if ((sssptr = bsearch(strbeg, sss, symnum, sizeof(ssstype), strcmp)) != 0) {
			if (maclevel)
				sssptr->macf = MACRO_LAB;
			else
				sssptr->macf = SYMBOL;

			char* txtptr_tmp = txtptr;
			if (*txtptr == '+' && (isdigit(*(txtptr + 1)) || *(txtptr + 1) == '$'))	//e.g. LDA SYMBOL+2
			{
				uint8_t ofst_candidate = expresstoi(txtptr);
				if (ofst_candidate > sssptr->ofst && ofst_candidate < 128)		// if offset > than what is already set
					//sssptr->ofst = *(txtptr + 1) - '0';		// Set offset
					sssptr->ofst = ofst_candidate;				// Set offset
			}
			else if ((*txtptr == '-') && (!isalpha(*(txtptr + 1))))			//e.g. LDA SYMBOL-3
				if ((signed char)(256 - (*(txtptr + 1) - '0')) < sssptr->ofst)		// if offset < than what is already set
					sssptr->ofst = (signed char)(256 - (*(txtptr + 1) - '0')); // Set offset

			txtptr = txtptr_tmp;
			symfg = 1;
			if (pass == 2)
				sssptr->frequency++;
			syms_in_expr++;
			return(sssptr->symadr);
		}
		else {
			error("label undefined");
		}
	}
	return(NOTFOUND);
}
// end symtoi1

/********* chkopc    returns the position (0, 1, 2,.. ) of opc in string
						returns -1 if not found
						*/
int chkopc(const unsigned char* opcstring) {
	int cnt = 0;
	const unsigned char* ptr = opcstring;
	unsigned char op[3];

	/* Validate that the (global) opcode is a proper 3-byte null-terminated string */
	if (!opcode || opcode[0] == 0 || opcode[1] == 0 ||
		opcode[2] == 0 || opcode[3] != 0)
		return -1;

	/* Convert the three characters to upper case once */
	op[0] = (unsigned char)toupper(opcode[0]);
	op[1] = (unsigned char)toupper(opcode[1]);
	op[2] = (unsigned char)toupper(opcode[2]);

	while (*ptr) {
		if (op[0] == ptr[0] && op[1] == ptr[1] && op[2] == ptr[2])
			return cnt;
		ptr += 3;
		cnt++;
	}
	return -1;
}
// end chkopc

/********* error **********/

void error(char* errstr) {
	size_t len;
	char partstr[100];

	printf("\n\nERROR --\n%s\nat ", errstr);
	if (pc) printf("address $%X, ", pc);
	printf("line %d\n\n\a", linnum);
	print_visible(linbuf[0]);

	if (maclevel == 0) {
		len = txtptr - linbegptr;
		strcpy_s(partstr, sizeof(partstr), linbuf[0]);
		partstr[len] = '\0';
		printf("%s", partstr);
		//printf(" txtptr = %x linbegptr = %x len = %i",txtptr,linbegptr,len);
		//printf("\n%s",linbegptr);
	}
	else {
		printf("Error in macro:\n%s", linbuf[maclevel]);
	}
	hit_Enter();
	exit(linnum);
}
// end error


void toxlbuf(uint8_t ch) {

	if (!orgflg)
		error("ORG value must precede this line");

	*xlbfptr = ch;
#if DISASSEMBLE
	* (xlram + pc) = ch;
#endif
	xlbfptr++;
	pc++;

	if (pass == 2) {
		bytcnt++;

		/* Comparison / verification mode */
		if (cpfflg) {
			bin = fgetc(stream);
			if (bin != (ch & 255)) {
				if (mismatch_flg == 0) {
					mismatch_flg = 1;
					mismatch_linnum = linnum;
					mismatch_pc = pc;
				}
				if (mismatch_flg == 1) {
					if ((bytcnt == 1) && (data_flg == 0)) {
						mismatch_flg = 2;
						mismatch_linnum = linnum;
						mismatch_pc = pc;
					}
				}
			}

		}

		/* Hex listing */
		if (strpbrk(outflgs, "Ll") && !strpbrk(outflgs, "Mm")) {

			char tempstr[16];

			sprintf_s(tempstr, sizeof(tempstr), " %02X", ch & 0xFF);
			
			if (bytcnt <= LSTBYTES)
				strcat_s(prnlnbufbeg, sizeof(prnlnbufbeg), tempstr);
			else {

				if (bytcnt % LSTBYTES == 1) {
					prnlnum++;
					if (strcmp (opcode, "ORG")) { // if not ORG
						sprintf_s(prnlnbuf2[prnlnum - 1], sizeof(prnlnbuf2[prnlnum - 1]), 
							"%4X:", pc - 1);
					}
					else { // if ORG
						sprintf_s(prnlnbuf2[prnlnum - 1], sizeof(prnlnbuf2[prnlnum - 1]),
							"     ");
					}
				}
				strcat_s(prnlnbuf2[prnlnum - 1], sizeof(prnlnbuf2[prnlnum - 1]), tempstr);
			}

			/* Comparison column */
			if (cpfflg) {
				if (bytcnt % LSTBYTES == 1) {
					if (bin != (ch & 255))
						sprintf_s(cmpflbytln[prnlnum], sizeof(cmpflbytln[prnlnum]), "******");
					else
						sprintf_s(cmpflbytln[prnlnum], sizeof(cmpflbytln[prnlnum]), "<><><>");
				}
				if (bin != (ch & 255)) {
					mismatchln_flg[prnlnum] = 1;
					sprintf_s(tempstr, sizeof(tempstr), "%02X)", bin);
				}
				else
					sprintf_s(tempstr, sizeof(tempstr), " %02X", bin); // space is for alignment

				strcat_s(cmpflbytln[prnlnum], sizeof(cmpflbytln[prnlnum]), tempstr);
			}
		}
	}
}
// end toxlbuf


/*		***********Find Operand *****************     */

void find_operand(void) {

	int cnt = 0;
	char* linptr;

	switch (*txtptr++) {
	case '\n':
	case '\t':	break;
	case ' ':
		linptr = txtptr;
		if (!strchr(" \t\n", *txtptr))	// there is an operand
		{
			if (*txtptr == '(') txtptr++;

			while (!(strchr(" \t\n", *linptr)))									// create operand[]
			{

				if ((*linptr == '\'') && (*(linptr + 1) == ' ') && (*(linptr + 2) == '\'')) {	// test for ' '
					operand[cnt++] = *linptr++;
					operand[cnt++] = *linptr++;
				}

				operand[cnt++] = *linptr++;
			}
		}
	}
	operand[cnt] = '\0';
}
// end Find Operand

/*		*************** src_filename Operand *****************     *////////////////

static void operand_to_filename(char filename[MAX_FILENAME_LEN + 1]) {

	int cnt = 0;
	char* linptr;

	switch (*txtptr++) {
	case '\n':
	case '\t': error("Filename missing");	break;
	case ' ':
		linptr = txtptr;
		if (!strchr(" \t\n", *txtptr))	// there is an operand
		{
			if (*linptr == '"') {
				for (; ; )									// create operand[]
				{
					operand[cnt++] = *linptr++;
					if (strchr("\"\t\n", *linptr)) {
						operand[cnt++] = *linptr++;
						break;
					}

				}
			}
			else {
				while (!(strchr(" \t\n", *linptr)))							// create operand[]
				{
					operand[cnt++] = *linptr++;
				}
			}
		}
		else
			error("Filename missing");	break;
	}
	operand[cnt] = 0;

	strcpy_s(filename, MAX_FILENAME_LEN, operand);
}
// end operand_to_filename




int ssscmpadr(const ssstype* a1, const ssstype* a2) {
	return (a1->symadr - a2->symadr);
}

// end ssscmpadr



void list_label(int i) {
	printf("%s\t", sss[i].symbol);
	if (sss[i].macf == MACRO)
		printf("MACRO\n");
	else
		printf(" = %X\n", sss[i].symadr);
}


void list_label_f(int i) {
	fprintf(stream2, "%s\t", sss[i].symbol);
	if (sss[i].macf == MACRO)
		fprintf(stream2, "MACRO\n");
	else
		fprintf(stream2, " = %X\n", sss[i].symadr);
}

void list_label_f2(int i) {

	if (sss[i].macf == MACRO)
		fprintf(stream2, "MACRO\n");
	else
		fprintf(stream2, "%X\t", sss[i].symadr);
	fprintf(stream2, "%s\n", sss[i].symbol);
}

void list_label_xasm(int i) {

	if (sss[i].macf == MACRO)
		fprintf(stream4, "MACRO\n");
	else
		fprintf(stream4, "     %04X  ", sss[i].symadr);
	fprintf(stream4, "%s\n", sss[i].symbol);
}


uint32_t symtoi3(char* strbeg) {
	int i, j;
	if (symnum == 0) return(NOTFOUND);
	i = 0;
	while (i < symnum) {
		j = 0;
		while (sss[i].symbol[j] == strbeg[j]) {
			if ((!strbeg[j] && !sss[i].symbol[j])) return(sss[i].symadr);
			j++;
		}
		i++;
	}
	return(NOTFOUND);
}



void sectyp(char line_type) {
	//if(pc == 0x4729)
		//pc=pc;
	if (pass == 2 && strchr("&@([", line_type)) {
		for (int i = 0; i < symnum; i++) {
			if (sss[i].symadr == pc) {
				sss[i].symtyp = line_type;
				return;
			}
		}

		if (symnumext >= MAXSYMS)
			error("Too many symbols");
		else {
			sss[symnumext].symtyp = line_type;
			sss[symnumext++].symadr = pc;
		}
		return;
	}

	if (!labflg && (line_type == ';') && strchr("!&@'([", section_type))
		line_type = section_type;


	if ((line_type != section_type) || labflg) // if new sec type or a label
	{
		section_type = line_type;
		if (labflg)								//if there is a label....
		{
			if (pass == 1) /**/
			{
				if (line_type == ';')			//with normal code (;)..
					sss[symnum - 1].symtyp = 0;	// label is enough, so no entry
				else sss[symnum - 1].symtyp = line_type; // else make entry
			}
			if ((pass == 2) && (line_type == '&')) /**/
			{
				sss[symnum - 1].symtyp = '&';
			}
			if ((pass == 2) && (line_type == '@')) /**/
			{
				sss[symnum - 1].symtyp = '@';
			}

		}
		else /* no label */
			if ((pass == 2) && (pc > 0xff)) {
				if (symnumext >= MAXSYMS)
					error("Too many symbols");
				else {
					sss[symnumext].symtyp = line_type;
					sss[symnumext++].symadr = pc;
				}
			}
	}
}
/***** symbol to be ported to SOURCGEN **/
char fptsmb(int i) {
	char* lnbfptr;
	int cnt, comment_bytes_cnt;
	char len;
	char temp_symtype;

	if ((sss[i].symadr == sss[i - 1].symadr) && (i > 0))		/* double entry */
	{
		temp_symtype = sss[i].symtyp;
		if (sss[i - 1].symbol[0])
			sss[i] = sss[i - 1];
		if (sss[i - 1].symtyp == 0)
			sss[i].symtyp = temp_symtype;// restore symbol type

		secsiz -= lensv;
		fsetpos(stream, &pos2);
	}
	lnbfptr = linbuf[0];
	cnt = 0;
	if (sss[i].macf == MACRO)
		return(0);
	while (sss[i].symbol[cnt])
		*lnbfptr++ = sss[i].symbol[cnt++];

	if (sss[i].comment[0]) {
		*lnbfptr++ = ';';
		comment_bytes_cnt = 0;
		cnt++;
		while (sss[i].comment[comment_bytes_cnt]) {
			if (sss[i].comment[comment_bytes_cnt] == 9)	//Replace tab with ATARI TAB
				sss[i].comment[comment_bytes_cnt] = 127;
			*lnbfptr++ = sss[i].comment[comment_bytes_cnt++];
			cnt++;
		}
		if (sss[i].symtyp == 0) //comment must end with valid tytpe char
			if (isSpecialCharacter(sss[i].comment[--comment_bytes_cnt]))
				sss[i].symtyp = ';';
	}


	if (sss[i].symtyp) {
		*lnbfptr = sss[i].symtyp;
		cnt++;
	}
	len = cnt + 4;
	fgetpos(stream, &pos2);
	fputc((char)sss[i].symadr, stream);
	fputc((char)((sss[i].symadr) >> 8), stream);
	fputc(sss[i].ofst, stream);
	fputc(len, stream);
	lnbfptr = linbuf[0];
	while (cnt) {
		fputc(*lnbfptr++, stream);
		cnt--;
	}
	return(len);
}

void hit_Enter(void) {
	printf("\nHit ENTER to continue--");
	if (getchar() == EOF) error("ABORT");
}

uint16_t parseNumber(char* txtptr) {
	char* endptr;
	long int parsedValue = 0;

	txtptr++;
	while (*txtptr != '\0') {
		long int number = 0;

		if (*txtptr == '$') {
			// Handle hexadecimal number
			number = strtol(txtptr + 1, &endptr, 16);
		}
		else if (isdigit(*txtptr)) {
			// Handle decimal number
			number = strtol(txtptr, &endptr, 10);
		}
		else {
			// Skip non-numeric characters
			txtptr++;
			continue;
		}

		parsedValue += number;

		// Move to the next potential '+' sign
		txtptr = endptr;

		if (*txtptr == '+') {
			txtptr++; // Move past the '+'
		}
	}

	// Check for conversion errors
	if (parsedValue >= 0 && parsedValue <= UINT16_MAX) {
		return (uint16_t)parsedValue;
	}
	else {
		// Error in conversion
		return 0; // You can handle this error case accordingly
	}
}

int isSpecialCharacter(char ch) {
	if (!((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || isdigit(ch) || isspace(ch))) {
		return 1; // Character is a special character
	}
	return 0; // Character is not a special character
}

static void print_visible(const char* s) {
	while (*s) {
		unsigned char c = (unsigned char)*s;

		if (c == '\t') {
			putchar('\t');              // preserve layout
		}
		else if (c == 0x0A) {
			putchar('\n');              // preserve layout
		}
		else if (c < 32) {
			printf("<%02X>", c);        // show hidden control chars
		}
		else {
			putchar(c);
		}

		s++;
	}
}

