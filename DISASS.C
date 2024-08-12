/************************** include *****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#include "cdgn_global.h"
int getch(void);

#define SYMBOL_OFFSET 9 // SYMBOL+1, SYMBOL+2 ...
#define ASC_STR_LEN 23
#define DFB_BYTES_PER_LINE 8
#define MAX_SEGMENTS 256
#define LINE_BUFFER_SIZE 1000
char line_buffer[LINE_BUFFER_SIZE] = ""; // Global buffer

#define SHOW_SECTYP_AND_ADDRESS 0
#define USE_SYMTAB_OFFSET 1
#define HIDE_JSR 1
#define SKA_AS_COMMENT 1
#define PRINT_DISS_LINNUM 0
#define PRINT_PC_ADDRESS 1
#define SHOW_B_W_FREQUENCY 1
uint16_t disassln(uint16_t pc);
void phb(uint8_t* ptr);
uint16_t adnmb(uint8_t* ptr);
uint16_t adnmw(uint8_t* ptr);
void adnmw2(uint16_t ska);
char intern_to_atascii(char ch);
void pts(const char* str);
void pts_p(const char* format, void* arg);
void ptc(char c);
char ptb(unsigned char num);
uint8_t read_loadfile(const char filename[FILENAME_MAX]);
void pt_ska_comment(uint16_t adr);
void print_line();

uint16_t sno;
char sctp;
uint16_t ska_sav, lo_hi_operand; 
char lo_hi_task;
uint8_t segment;
uint8_t number_of_segments;
uint16_t start_address[MAX_SEGMENTS];
uint16_t end_address[MAX_SEGMENTS];
uint16_t pc, pcsav,adr;
char comment_string[10] = "";
int symtab_epz = 0;
int symtab_init = 0;
bool epz_flg = 0;
uint16_t isav = 0;
int diss_linnum = 0;
int line_skip = 0;
char output_filename[FILENAME_MAX];
FILE* outputfile;

void disassemble(const char* filename) {
	char ch;
	int i = 0;
	char* extension_pos = strrchr(filename, '.'); // Find the last occurrence of '.'





	if (extension_pos == NULL) {
		fprintf(stderr, "Error: File has no extension.\n");
		return;
	}

	size_t base_filename_len = extension_pos - filename; // Calculate the length of the base filename
	snprintf(output_filename, FILENAME_MAX, "%.*s.SRC", (int)base_filename_len, filename); // Create new filename

	
	if (fopen_s(&outputfile, output_filename, "w") != 0) {
		perror("Error opening output file");
		return;
	}
	printf("\n\n\nDIS Filename %s\n\n\n", output_filename);

	number_of_segments = read_loadfile(filename); //loafilename

/********      Init symtab_epz  *******/

	symtab_epz = 0;
	while (sss[symtab_epz].symtyp=='[')
		symtab_epz++;
	printf("symtab_epz = %d\n", symtab_epz);

/********      Write header                  ********/
	
	printf ("Writing header\n");
	bool found = 0;
	
	//printf("symnum = %d\n\n", symnum);
	for (i = 0; i < symnum; i++) {
		found = 0;
		if (sss[i].symbol) {
			//printf("i = %d",i);
			for (segment = 1; segment <= number_of_segments; segment++) {
				if ((sss[i].symadr >= start_address[segment]
					&& sss[i].symadr <= end_address[segment]) // Check if symbol is in segment
					|| sss[i].symtyp == '*') {  // Check if ORG
					found = 1;
					break;
				}
				
			}
			//printf("    Segment = %d\n", segment-1);
		}
		if (found == 0) {
			
			if (sss[i].symadr <= 0xFF && !(sss[i].symtyp == '[')) {
				if (epz_flg == 0) {
					print_line();//empty line
					epz_flg = 1;
				}
				pts_p("%s\tEPZ ", sss[i].symbol);
	
			}
			else {
				epz_flg = 0;
				pts_p("%s\tEQU ", sss[i].symbol);
			}
			pts_p("$%X", sss[i].symadr);

#if SHOW_B_W_FREQUENCY

			if (sss[i].ofst) {
				pts("  w");
			}
			else {
				pts("  b");
			}
			pts_p("%i", sss[i].frequency); 
#endif // 
			
			if (sss[i].comment[0])
				pts_p("\t%s", sss[i].comment);
			
			print_line();
		}
	}

	
	for (segment = 1; segment <= number_of_segments; segment++) {

		printf("Writing segment %d\n", segment);
		pc = start_address[segment];
		
		fprintf(outputfile, "\n\t\tORG $%x\n\n", pc); // Write to the file
		diss_linnum +=3;
		
		sno = 0;
		while (sss[sno].symadr < pc) {
			sno++;
			if (sno == symnum) {
				sno--;
				break;				//symbol list ended. Go back one
			}
		}
		if (sss[sno].symadr > pc) sno--;//symbol not found. Go back one
										//If address in symbol table, stay there
#define PRINT_COMMENTS 0
#if PRINT_COMMENTS

		//for (i = 0; i < symnumext; i++) {

			//if (sss[i].comment[0])
				//printf("*** %s ***\n", sss[i].comment);

		//}
#endif

		ch = 0;
		while (1) {
			pcsav = pc;
			pc = disassln(pc);

			if (!((lo_hi_task == 1) || (lo_hi_task == 2))) {
				
				print_line(); //printline

			
			}
			if (pc == end_address[segment] + 1)
				break;
			if (pc > end_address[segment] + 1) {
				printf("\aEnd of segment not recognised\n");
				break;
			}
		}
	}
	fclose(outputfile); // Close the file
	puts("Done.\n");
	return;
}
/****************** disassln  ********************/

uint16_t disassln(uint16_t ska) {
	uint8_t offset;
	char* ptr;
	uint8_t* codeptr;

	char cnt;
	unsigned int temp;
	char linbuf[] = "         ";
	char* lnbfptr = linbuf;
	//char* lnbfptrsv;
	char ch;

/*	if (sss[sno].comment[0]) {
		pts_p("*** %s ***\n", sss[sno].comment);
		printf("*** %s ***\n", sss[sno].comment);
	}*/

	line_buffer[0] = '\0';

	if ((sss[sno ].symadr == ska) && (sno < symnum - 1)) {// needed for & and first address in segment
		cnt = 0;
		lnbfptr = linbuf;
		if (sss[sno].symbol)
			while (sss[sno].symbol[cnt])
				*lnbfptr++ = sss[sno].symbol[cnt++];
	}

	if ((sss[sno + 1].symadr == ska) && (sno < symnum - 1)) {// needed for everything else
		sno++;
		cnt = 0;
		lnbfptr = linbuf;
		if (sss[sno].symbol)
			while (sss[sno].symbol[cnt])
				*lnbfptr++ = sss[sno].symbol[cnt++];
	}
	
	sctp = sss[sno].symtyp;

#if SHOW_SECTYP_AND_ADDRESS

	if (sss[sno].symadr == ska) {
		if (sctp)
			printf("%c ", sctp);
		else
			printf("  ");
	}
	else
		printf("  ");

	
	printf("$%X ", ska);

#endif
	if (sss[sno].comment[0] && sss[sno].symadr == pc) {
		pts_p("*** %s ***", sss[sno].comment);
		print_line();
	}


	pts_p("%s", linbuf); // Label at line beginning


	symtab_init = symtab_epz;

	codeptr = xlram + ska;
	//printf("%c ", sctp);
	switch (sctp) {
	case '\0':
	case ' ':
	case ';':
	case '\'':
	case '!':
	case '&':
	case '@':
	case '(': 
	case '<':	break; //normal opcodes

	case '#':
	case '['://(EQU CONSTANT <= $FF)
		cnt = DFB_BYTES_PER_LINE - 1;
		pts("DFB ");
		while (1) {
			pts_p("$%X", *codeptr++);
			ska++;
			if (sss[sno + 1].symadr == ska) break; //symbol found: New line
			if (cnt-- == 0) break;
			ptc(',');
		}
		return(ska);
	case '%':
		pts("DFB ");
		ptc('%');
		ptb(*codeptr);
		return(++ska);
	case '=':
	case '|':
       		pts("DFW ");
			if (pc == 0x492c) // 0x492c (4742)
				adr = adr;
			adr = adnmw(codeptr);
			pt_ska_comment(adr);
		return (ska + 2);
	case '-':
	case '_':
		pts("DFW ");
		adr = adnmw(codeptr);
		pts("-1");
		pt_ska_comment(adr);
		return (ska + 2);
	case '\\':
		pts("ASC ");
		ptc('\\');
		while (!((*codeptr) & 128)) {
			ptc(*codeptr++);
			ska++;
		}
		ptc((*codeptr) & 127);
		ptc('\\');
		return(++ska);

	case '"':
		cnt = ASC_STR_LEN;
		pts("ASC ");
		if (*codeptr & 128) {
			ptc('/');
			while (*codeptr & 128) {
				ptc(127 & *codeptr++);
				ska++;
				if (sss[sno + 1].symadr == ska) break;
				if (cnt-- == 0) break;
			}
			ptc('/');
			return(ska);
		}
		else {
			ptc('"');
			while (!(*codeptr & 128)) {
				ptc(*codeptr++);
				ska++;
				if (sss[sno + 1].symadr == ska) break;
				if (cnt-- == 0) break;
			}
			ptc('"');
			return(ska);
		}
	case '$':
		cnt = 23;
		
		pts("ASC ");
		if (*codeptr & 128) {
			ptc('$');
			while (*codeptr & 128) {
				ch = intern_to_atascii(*codeptr);
				ptc(127 & ch);
				codeptr++;
				ska++;
				if (sss[sno + 1].symadr == ska) break;
				if (cnt-- == 0) break;
			}
			ptc('$');
			return(ska);
		}
		else {
			ptc('%');
			while (!(*codeptr & 128)) {
				ch = intern_to_atascii(*codeptr);
				ptc(ch);
				codeptr++;
				ska++;
				if (sss[sno + 1].symadr == ska) break;
				if (cnt-- == 0) break;
			}
			ptc('%');
			return(ska);
		}
		case '*':
			
    		ska = (sss[sno + 1].symadr);//next symbol-table entry
			pts_p("\n\t\tORG $%X\n", ska);
			return(ska);

		
	default:
		pts_p("The section type '%c' is not yet implemented\n", sctp);

	}
	offset = abofst[*codeptr];
	ptr = abtab + offset;


#if HIDE_JSR
	if (offset == 0x54) { //JSR
		size_t current_length_sv = strlen(line_buffer);
		
		adr = adnmw(++codeptr);
		pt_ska_comment(adr);

		if (isalpha(line_buffer[current_length_sv]))
			return(ska + 3);
		else{ 
			line_buffer[current_length_sv] = 0; 
			codeptr--;
		}
	}
#endif
	
		ptc(*ptr++);// Print the opcode except JSR
		ptc(*ptr++);
		ptc(*ptr);
	
		if (offset == 0x7e) { //RTS

			line_skip++;
		}

	switch (opertytb[*codeptr++]) {
	case OPC:		
		return(ska + 1);

	case NN:
		symtab_init = 0;
		if (sctp == '\'') {
			if (isprint(*codeptr)) {
				pts_p(" '%c'", *codeptr);
				pt_ska_comment((uint16_t)* codeptr);
			}
			else
			pts_p(" #$%X", *codeptr);

			return(ska + 2);
		}
		else {

			pts(" #");

			if (sctp == '!') {
				ptc('%');
				ptb(*codeptr);
				pt_ska_comment((uint16_t)*codeptr);
				return(ska + 2);
			}
			else if (sss[sno].symtyp == '(') {
				if (ska == 0x8b)
					ska = ska;
				adr = adnmb(codeptr);
				pt_ska_comment(adr);
				return(ska + 2);
			}
			else if (sctp == '&') {
				if (lo_hi_task == 0) {				// :L
					if ((ska == sss[sno].symadr) && (sss[sno].symtyp == '&')) {
						lo_hi_task = 1;
						ska_sav = ska;
						return(ska + 2);
					}
					else{//treat as sctp == ';'
						phb(codeptr);
						return(ska + 2);
					}
				}
				else if (lo_hi_task == 1) {				// :H
					lo_hi_operand = *codeptr << 8;
					lo_hi_task = 2;
					return(ska_sav);
				}
				else if (lo_hi_task == 2) {				// :L
					lo_hi_operand += *codeptr;
					adnmw2(lo_hi_operand);
					pts(":L");
					pt_ska_comment(lo_hi_operand);
					lo_hi_task = 3;
					return(ska + 2);
				}
				else if (lo_hi_task == 3) {					//:L
					lo_hi_task = 0;
					adnmw2(lo_hi_operand);
					pts(":H");
					pt_ska_comment(lo_hi_operand);
					lo_hi_operand = *codeptr << 8;
					
					return(ska + 2);					// :H
				}
			}
			else {
			//	if (*codeptr < 0x80) {//not user zero page variables
			//		adnmb(codeptr);
			//		pt_ska_comment((uint16_t)*codeptr);
			//	}
			//	else//avoid user zero page variables
					phb(codeptr);
				return(ska + 2);
			}
		}
		
	case AA:
		ptc(' ');
		adr = adnmb(codeptr);
		pt_ska_comment(adr);
		return(ska + 2);
	case AAX:
		ptc(' ');
		adr = adnmb(codeptr);
		pts(",X");
		pt_ska_comment(adr);
		return(ska + 2);
	case AAY:
		ptc(' ');
		adr = adnmb(codeptr);
		pts(",Y");
		pt_ska_comment(adr);
		return(ska + 2);
	case BAABY:
		pts(" (");
		adr = adnmb(codeptr);
		pts("),Y");
		pt_ska_comment(adr);
		return(ska + 2);
	case BAAXB:
		pts(" (");
		adr = adnmb(codeptr);
		pts(",X)");
		pt_ska_comment(adr);
		return(ska + 2);
	case AAAA:
		ptc(' ');
		adr = adnmw(codeptr);
		pt_ska_comment(adr);
		if (offset == 0x51) { //JMP

			line_skip++;
		}
		return(ska + 3);
	case AAAAX:
		ptc(' ');
		adr = adnmw(codeptr);
		pts(",X");
		pt_ska_comment(adr);
		return(ska + 3);
	case AAAAY:
		ptc(' ');
		adr = adnmw(codeptr);
		pts(",Y");
		pt_ska_comment(adr);
		return(ska + 3);
	case BAAAAB:
		pts(" (");
		adr = adnmw(codeptr);
		ptc(')');
		pt_ska_comment(adr);
		return(ska + 3);
	case BRA:
		temp = ska + 2;
		ska += 2 + (signed char)*codeptr;
		ptc(' ');
		adnmw2(ska);
		pt_ska_comment(ska);

			line_skip++;
		return(temp);

	default:
		pts("  Disassembly not possible");
		ptc('?');
		return(ska + 1);
	}
}

/************** adnmw returns address to be printed *****************/
uint16_t adnmw(uint8_t* ptr) {
	uint16_t ska;

	uint8_t low_bite = *ptr++;
	uint8_t high_bite = *ptr;
	ska = (uint16_t)low_bite + 256 * ((uint16_t)high_bite);
	if ((sctp == '-') || (sctp == '_'))
		ska++;
	adnmw2(ska);
	return(ska);
}

void adnmw2(uint16_t ska) {

	uint16_t i=0;
	uint16_t offset=0;

	isav = i = symtab_init;

	while (sss[i].symadr <= ska) {
		if (sss[i].symbol[0]) {         // symbol exists at this i
			isav = i;
			if (sss[i].symadr == ska) {     // symbol found at this address?
				pts(sss[i].symbol);         // symbol to string
				
				return;
			}
		}
		i++;
	}

	if (i == 0) {							// no symbols at all??????????????????
		if (ska > 9) ptc('$'); // only if $ is needed
		pts_p("%X", ska);
		return;
	}

	while (sss[i].symbol[0] == 0) i++;	// find first symbol after ska
	


#if USE_SYMTAB_OFFSET

	if (ska == 0x549B)
		ska = ska;

	if (sss[isav].ofst > 0) {								// positive offset has priority
		if (ska - sss[isav].symadr <= sss[isav].ofst) {     // eg. STA SYMBOL+1
			pts_p("%s", sss[isav].symbol);
			pts_p("+%d", ska - sss[isav].symadr);
		}
		else {
			if (ska > 9) ptc('$'); // only if $ is needed
			pts_p("%X", ska);
		}
	}
	
	else if (sss[i].ofst & 128) { // is negative
		if (sss[i].symadr - ska <= (uint8_t)sss[i].ofst & 127) {    // eg. STA SYMBOL-1

			pts_p("%s", sss[i].symbol);
			pts_p("-%d", sss[i].symadr - ska);
		}
		else {
			if (ska > 9) ptc('$'); // only if $ is needed
			pts_p("%X", ska);
		}
	}


#else

	offset = ska - sss[isav].symadr;
	
	if (offset <= SYMBOL_OFFSET) {     // eg. STA SYMBOL+1
		pts_p("%s", sss[isav].symbol);
		pts_p("+%d", offset);
	}
	else if ( (sss[i].symadr - ska == 1) && (sss[i].symbol[0])) {   // eg. STA SYMBOL-1

		pts_p("%s", sss[i].symbol);
		pts("-1");
	
	}
	


#endif
	else {
		if (ska > 9) ptc('$'); // only if $ is needed
		pts_p("%X", ska);
	}
	



#define TEST 0
#if TEST
		if (pc >= 0x600) {
			printf("\npc = $%x\n", pc);
			printf("i = %d\n", i);
			printf("ska = $%x\n", ska);
			printf("isav = %d\n", isav);
			printf("sss[i].symadr = $%x\n", sss[i].symadr);
			printf("sss[isav].symadr = $%x\n", sss[isav].symadr);
			printf("sss[i].ofst = $%x\n", sss[i].ofst);
			printf("sss[isav].ofst = $%x\n", sss[isav].ofst);
			printf("sss[i].symbol = %s\n", sss[i].symbol);
			printf("sss[isav].symbol = %s\n", sss[isav].symbol);
			printf("%s\n", line_buffer);
}
#endif // TEST

	return;
}


uint16_t adnmb(uint8_t* ptr) {
	uint8_t byte;

	byte = *ptr++;
	adnmw2((uint16_t)byte);
	return(byte);
}

void phb(uint8_t* ptr) {
	if (*ptr > 9) 
		ptc('$');
	pts_p("%X", *ptr);
	return;
}
//32,32,-64,0,32,32,-64,0
//1, 1,-2, 0, 1, 1, -2 ,0

char intern_to_atascii(char ch) {
	char chmasked = ch & 0b01111111;
	if (chmasked < 64)
		ch += 32;
	else if (chmasked < 96)
		ch -= 64;
	return(ch);
}

void pts(const char* str) {
	size_t current_length = strlen(line_buffer);
	snprintf(line_buffer + current_length, LINE_BUFFER_SIZE - current_length, "%s", str);
}

void pts_p(const char* format, void* arg) {
	size_t current_length = strlen(line_buffer);
	char temp_buffer[LINE_BUFFER_SIZE];
	snprintf(temp_buffer, sizeof(temp_buffer), format, arg);
	strncat_s(line_buffer, LINE_BUFFER_SIZE, temp_buffer, LINE_BUFFER_SIZE - current_length - 1);
}

void ptc(char c) {
	size_t current_length = strlen(line_buffer);
	if (current_length < LINE_BUFFER_SIZE - 1) { // Ensure there's space for the character and null terminator
		line_buffer[current_length] = c;
		line_buffer[current_length + 1] = '\0';
	}
}

char ptb(unsigned char num) {
	// Determine the number of bits needed to represent the integer
	uint8_t numBits = 8;
	uint8_t bit_val;
	uint8_t nr_of_1s = 0;
	// Print binary string with leading zeros
	for (int i = numBits - 1; i >= 0; i--) {
		bit_val = (num >> i) & 1;
		nr_of_1s += bit_val;
		pts_p("%d", bit_val);
	}
	return (nr_of_1s);

}



uint8_t read_loadfile(const char filename[FILENAME_MAX]) {
	FILE* file;
	uint8_t segment = 1;

	// Open binary file for reading
	printf("\nReading from %s\n\n", filename	);
	if (fopen_s(&file, filename, "rb") != 0) {
		perror("Error opening file");
		return(1);
	}

	printf("Segment\tfrom\t to\n");

	while (!feof(file)) {

		uint8_t start_high, start_low, end_high, end_low;

		fread(&start_low, sizeof(uint8_t), 1, file); // Read start address LOW

		if (feof(file)) {
			fprintf(stderr, "End of file reached as expected.\n");
			break; // Exit the loop
		}
		fread(&start_high, sizeof(uint8_t), 1, file);	// Read start address HIGH

		if (segment == 1) {
			if (!(start_low == 0xFF && start_high == 0xFF)) // Check for mandatory 0xFF bytes on first iteration
				fprintf(stderr, "Error: This is not a load-file.\n");
		}
		if (start_low == 0xFF && start_high == 0xFF) { // Ignore 0xFF bytes
			fread(&start_low, sizeof(uint8_t), 1, file); // Read start address LOW again.
			fread(&start_high, sizeof(uint8_t), 1, file);// Read start address HIGH again.
		}
		start_address[segment] = (start_high << 8) | start_low;

		fread(&end_low, sizeof(uint8_t), 1, file);
		fread(&end_high, sizeof(uint8_t), 1, file);

		end_address[segment] = (end_high << 8) | end_low;

		uint16_t segment_length = end_address[segment] - start_address[segment] + 1;
		uint16_t xlram_start = start_address[segment];

		fread(&xlram[xlram_start], sizeof(uint8_t), segment_length, file);

		if (feof(file)) {
			fprintf(stderr, "Error: End of file reached prematurely.\n");
			break;
		}
		// Print the table for the current segment
		printf("   %d\t%X\t%X\n", segment, start_address[segment], end_address[segment]);

		segment++;
	}
	return(segment-1);
}

void pt_ska_comment(uint16_t ska) {

#if SKA_AS_COMMENT

	if (strlen(line_buffer) < 18)
		ptc('\t');
	pts_p("\t*%X", ska);
	ptc('\t');                  // tab
	if(sss[isav].symadr == ska)
		pts(sss[isav].comment);
#endif


}

void print_line() {
	
	//if (diss_linnum == 5709)
	//	getch();
	diss_linnum++;

#if	PRINT_DISS_LINNUM
	fprintf(outputfile, "%i  ", diss_linnum);
#endif

#if PRINT_PC_ADDRESS
	if (pcsav != 0)
		fprintf(outputfile, "$%04X", pcsav);
	fprintf(outputfile, "\t");
#endif



	fprintf(outputfile, "%s\n", line_buffer);

	if (line_skip) {
		if (lo_hi_task != 1 && lo_hi_task != 2) {
			diss_linnum++;

#if PRINT_DISS_LINNUM
			fprintf(outputfile, "%i\n", diss_linnum);
#endif
		}
		line_skip = 0;
	}
	*line_buffer = '\0'; // Reset line_buffer
}