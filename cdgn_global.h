//#define SRC_FILENAME "C:\\Users\\Acustica\\My Drive (richard.miles.jackman@gmail.com)\\CODEGEN-2\\SGEN418.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\AUTORUN.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\AUTORUN_1.SRC"
//#define SRC_FILENAME "h:\\My Drive\\CODEGEN-2\\FINDBUGS.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\SGEN33.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\SGEN420.SRC"
//#define SRC_FILENAME "h:\\My Drive\\CODEGEN-2\\SecType.SRC" 
//#define SRC_FILENAME "h:\\My Drive\\CODEGEN-2\\OPCTEST.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\SGEN32_no_macros.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\TEMP.SRC"
//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\SGEN4xx.SRC"
//#define SRC_FILENAME "C:\\Users\\Rick\\source\\repos\\rmjackman\\CodegenVS-001\\6502 Source\\SGEN4xx.SRC"
//#define SRC_FILENAME "C:\\Users\\Acustica\\source\\repos\\rmjackman\\CodegenVS-001\\6502 Source\\SGEN4xx.SRC"
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\T31.SRC"

#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\T31.SRC"

#include <stdint.h>


#ifndef GDGN_GLOBAL_H

#define GDGN_GLOBAL_H

#define CREATE_DBG 1


#define DOS_BLOCK_SIZE (333-4)*32-16
#define DOS25_VOTC_DIR_SIZE 128+32
#define MAX_FILENAME_LEN 256

#define MAXSYMS 2500
#define COMMENT_LEN 60

// Change color from green to violet
#define uint8_t uint8_t
#define uint16_t uint16_t
#define uint32_t uint32_t

typedef struct {
	char symbol[9];
	char comment[COMMENT_LEN];
	uint16_t symadr;
	int8_t ofst;
	uint8_t macf;
	char symtyp;
	char word_or_byte;// 0 = byte, 1 = word
	uint16_t frequency;
}	ssstype;

ssstype sss[MAXSYMS];
uint16_t symnum, symnumext;
uint8_t* xlram;

void hit_Enter(void);
char src_filename[MAX_FILENAME_LEN];
char loafilename[MAX_FILENAME_LEN];

const unsigned char dos25_block[DOS_BLOCK_SIZE];
const unsigned char dos25_votc_dir[DOS25_VOTC_DIR_SIZE];

uint16_t findfree(uint16_t);
void err(char* errstr);
void file_err(char* errstr, char filename[]);
void check_sector(uint16_t sec);
void file_to_image(char file_filename[], char diskimage_name[]);

void disassemble(const char* filename);
char abtab[];
char abofst[];
char opertytb[];
char esc_ch_tab[];

enum { OPC, NN, AA, AAAA, AAX, AAAAX, AAY, AAAAY, BAAXB, BAABY, BRA, BAAAAB, ERROR };
#define Q OPC

unsigned char oplen[256];

#endif // GDGN_GLOBAL_H



#ifndef DBG_H
#define DBG_H

#include <stdio.h>

typedef struct DbgContext {
    FILE* fp;

    unsigned next_file_id;
    unsigned next_seg_id;
    unsigned next_span_id;
    unsigned next_line_id;
    unsigned next_sym_id;

} DbgContext;

/* --- function prototypes --- */

void dbg_file(DbgContext* ctx, unsigned id, const char* name);

void dbg_seg(DbgContext* ctx, unsigned id, unsigned start, const char* name);

void dbg_span(DbgContext* ctx,
    unsigned id,
    unsigned seg,
    unsigned start,
    unsigned size);

void dbg_line(DbgContext* ctx,
    unsigned id,
    unsigned file,
    unsigned line,
    unsigned span);

void dbg_sym(DbgContext* ctx,
    unsigned id,
    const char* name,
    unsigned value,
    unsigned size);

void dbg_close(DbgContext* ctx);

#endif /* DBG_H */
