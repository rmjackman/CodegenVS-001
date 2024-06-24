/************************** include *****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
int getch(void);

typedef struct
	{
	char symbol[9];
	unsigned int symadr;
	char macf;
	char symtyp;
	}	ssstype;

#define MAXSYMS 2000

extern	ssstype sss[MAXSYMS];
extern	unsigned int symnum;
extern	char *xlram;

#define pts(s) printf(s)

void disassemble(unsigned int pc); 
unsigned int disassln(unsigned int pc);
void phb(char *ptr);
void adnmb(char *ptr);
void adnmw(char *ptr);
void adnmw2(unsigned int ska);

int sno;



void disassemble(unsigned int pc) 
	{
	char ch;
	sno = 0;
	
	while (sss[sno].symadr < pc) 
		{
		sno++;
		if (sno==symnum) 
			{
			sno--;
			break;
			}
		}
	if (sss[sno].symadr > pc) sno--;
		
	ch = 0;
	while(toupper(ch) != 'X')
		{
		pc = disassln(pc);
		ch = getch();
		}
	}

unsigned int disassln(unsigned int ska)
	{
	char offset;
	char *ptr;
	char *codeptr;
	char sctp;
	char cnt;
	unsigned int temp;
	char linbuf[] = "         ";
	char *lnbfptr;
	
	printf("\n$%X ",ska); 

	if ((sss[sno+1].symadr==ska)&&(sno<symnum-1)) 
		{
		sno++;
		cnt = 0;
		lnbfptr = linbuf;
		if (sss[sno].symbol) 
			while (sss[sno].symbol[cnt])
				*lnbfptr++ = sss[sno].symbol[cnt++];
		}
	printf("%s",linbuf);
	sctp = sss[sno].symtyp;

	codeptr= xlram+ska;
	
	switch(sctp)
		{
		case '#':
			cnt = 6;
			printf("DFB ");
			while (1)
				{
				printf("$%X",*codeptr++);
				ska++;
				if (sss[sno+1].symadr == ska) break;
				if (cnt-- == 0) break;
				putchar(',');
				}
			return(ska);
		case '=':
			printf("DFW ");
			adnmw(codeptr);
			return (ska+2);
		case '\0':
		case ';' :	break;
		case '\\':
			printf ("ASC ");
			putchar('\\');
			while (!((*codeptr)&128))
				{
				putchar(*codeptr++);
				ska++;
				}
			putchar((*codeptr)&127);
			putchar('\\');
			return(++ska);
		
		default:
			cnt = 32;
			printf ("ASC ");
			putchar('"');
			while (1)
				{
				putchar(*codeptr++);
				ska++;
				if (sss[sno+1].symadr == ska) break;
				if (cnt-- == 0) break;
				}
			putchar('"');
			return(ska);
		}
	offset = abofst[*codeptr];
	ptr = abtab + offset;
	putchar(*ptr++);
	putchar(*ptr++);
	putchar(*ptr);	
	
	switch (opertytb[*codeptr++])
		{
		case OPC	:		return(ska + 1);
		
		case NN	:
			pts(" #");
			phb(codeptr);
			return(ska+2);
		case AA	:
			putchar(' ');	
			adnmb(codeptr);
			return(ska+2);
		case AAX	:
			putchar(' ');	
			adnmb(codeptr);
			pts(",X");
			return(ska+2);
		case AAY	:
			putchar(' ');	
			adnmb(codeptr);
			pts(",Y");			
			return(ska+2);
		case BAABY	:
			pts(" (");		
			adnmb(codeptr);
			pts("),Y");
			return(ska+2);
		case BAAXB	:
			pts(" (");		
			adnmb(codeptr);
			pts( ",X)" );
			return(ska+2);
		case AAAA	:
			putchar(' ');	
			adnmw(codeptr);
			return(ska+3);
		case AAAAX	:
			putchar(' ');	
			adnmw(codeptr);
			pts( ",X" );			
			return(ska+3);
		case AAAAY	:
			putchar(' ');	
			adnmw(codeptr);
			pts(",Y");
			return(ska+3);
		case BAAAAB	:
			pts(" (");	
			adnmw(codeptr);
			putchar( ')' );
			return(ska+3);
		case BRA	:
			temp = ska + 2;
			ska += 2+ (signed char) *codeptr;
			putchar(' ');
			adnmw2(ska);
			return(temp);  

		default	:
			pts("  Disassembly not possible");
			putchar ('?');
			return(ska+1);
		}
	}
	
void adnmw(char *ptr)
	{
	unsigned int ska,temp;
	
	ska = *ptr++;
	temp = *ptr;
	temp <<= 8;
	ska += temp;
	adnmw2(ska);
	return;
	}
	
void adnmw2(unsigned int ska)
	{
	int	i;
	unsigned int temp;
	
	for (i=0; i<symnum, sss[i].symadr <= ska; i++)
		{
		if (sss[i].macf) continue;
		if (sss[i].symadr == ska)
			{
			pts(sss[i].symbol);
			return;
			}
		}
	temp = ska - sss[i-1].symadr;
	printf ("%s+",sss[i-1].symbol);
	if (temp>9) putchar('$');
	printf ("%X",temp);

	return;
	}
	
	
void adnmb(char *ptr)
	{
	unsigned int ska;
	
	ska = *ptr++;
	adnmw2(ska);
	return;
	}
	
void phb(char *ptr)
	{
	if (*ptr>9) putchar('$');
	printf ("%X", *ptr);
	return;
	}
	
