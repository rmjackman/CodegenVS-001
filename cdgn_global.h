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
//#define SRC_FILENAME "g:\\My Drive\\CODEGEN-2\\T31.SRC"
 
//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\SGEN420.SRC"
//#define SRC_FILENAME "C:\\Users\\Acustica\\source\\repos\\rmjackman\\CodegenVS-001\\6502 Source\\SGEN420.SRC"
//#define SRC_FILENAME "C:\\Users\\Rick\\source\\repos\\rmjackman\\CodegenVS-001\\6502 Source\\SGEN420.SRC"

//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\T31.SRC"
//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\T31.SRC"
//#define SRC_FILENAME "C:\\Users\\Acustica\\source\\repos\\rmjackman\\CodegenVS-001\\6502 Source\\T31.SRC"
//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\Test_DA.SRC"  

//#define SRC_FILENAME "C:\\Users\\Lloyd\\source\\repos\\CodegenVS-001\\6502 Source\\Test_DA.SRC"
//#define SRC_FILENAME "C:\\Users\\Lloyd\\source\\repos\\CodegenVS-001\\6502 Source\\T31.SRC"
//#define SRC_FILENAME "C:\\Users\\Lloyd\\source\\repos\\CodegenVS-001\\6502 Source\\SGEN420.SRC"
//#define SRC_FILENAME "C:\\Users\\Lloyd\\source\\repos\\CodegenVS-001\\6502 Source\\TURBO_IN.src"

//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\Test_DA.SRC"
#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\T31.SRC"
//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\SGEN420.SRC"
//#define SRC_FILENAME "C:\\Users\\DAW\\source\\repos\\CodegenVS-001\\6502 Source\\TURBO_IN.src"

#define SHOW_SETTING(name) \
    printf("%-28s %s\n", #name, (name) ? "ON" : "OFF")

#define EQU_IN_SECTION 1
#define SYMBOL_OFFSET 5 // SYMBOL+1, SYMBOL+2 ...

#include <stdint.h>


#ifndef GDGN_GLOBAL_H

#define GDGN_GLOBAL_H

#define ATMAS_II_COMPATIBLE 0

#define CREATE_DBG 0


#define DOS_BLOCK_SIZE (333-4)*32-16
#define DOS25_VOTC_DIR_SIZE 128+32
#define MAX_FILENAME_LEN 256
#define MAX_SYMBOL_SIZE 16           //Set to 8 for ATMAS II compatibility
#define MAXSYMS 2500
#define COMMENT_LEN 60

#define E_5A_5B 1
// Change color from green to violet
#define uint8_t uint8_t
#define uint16_t uint16_t
#define uint32_t uint32_t

typedef struct {
	char symbol[MAX_SYMBOL_SIZE + 1];
	char comment[COMMENT_LEN + 1];
    uint16_t symadr;
	int8_t ofst;
	uint8_t macf;
	char symtyp;
	char word_or_byte;// 0 = byte, 1 = word
	uint16_t frequency;
    int8_t frequ_constant; // 0 = not used in any immediate addressing or DFB, 1 = used
	uint16_t oper_symval; // The value of the symbol in the operand, if any
}	ssstype;
ssstype sss[MAXSYMS];
uint16_t symnum, symnumext;

uint8_t* xlram;

void hit_Enter(void);
char src_filename[MAX_FILENAME_LEN + 1];
static char loafilename[MAX_FILENAME_LEN + 1];

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
unsigned char esc_ch_tab[];

enum { OPC, NN, AA, AAAA, AAX, AAAAX, AAY, AAAAY, BAAXB, BAABY, BRA, BAAAAB, ERROR };
#define Q OPC

unsigned char oprndlen[256];

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

void dbg_seg(DbgContext* ctx, unsigned id, unsigned start);

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
    unsigned value);

void dbg_close(DbgContext* ctx);

#endif /* DBG_H */

#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"

#define BRED    "\x1b[91m"
#define BGREEN  "\x1b[92m"
#define BYELLOW "\x1b[93m"
#define BBLUE   "\x1b[94m"
