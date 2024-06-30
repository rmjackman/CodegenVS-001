
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

#define STAND_ALONE 0

#define SAVE_DISK_IMAGE		1	// updates the XFD or ATR image file.
#define SHOW_DIR			1	// Show directory. ( Subroutine )
#define REBUILD_VTOC		1	// rebuilds VTOC and compares with old VTOC. Corrects file length in directory.  ( Subroutine )
#define CREATE_EMPTY_ED_IMAGE 0 // Creates an empty enhanced density disk image
#define CREATE_ATR_FROM_XFD 0	// Creates an ATR disk image from an XFD disk image

/* Diagnostics: */
#define SHOW_FILE_SECS	0	// Show file-number mismatches and sectors used by each file. ( multiple blocks of code )
#define SHOW_DIR64		0	// Show all 64 entry slots in the directory. (  Subroutine  )
#define SHOW_ATR_HEADER 0	// Show the header of an ATR disk image. ( single block of code )
#define SHOW_VTOC_MAP	0	// Show the VTOC map. ( Subroutine )
#define SHOW_FINDFREE	0	// Show each free sector found in VTOC ( several inserts )
#define CMP_DISK		0	// ( inserts and print_mismatches() )
#define FIRST_SECS_VTOC 0	// Show bytes in first four sectors of disk image, and in the VTOC sectors ( print4secs() )

/* Macros */
#define DPEEK(arr,i, j) (unsigned char)arr[i][j] + (unsigned char)arr[i][j + 1] * 256
#define FILE_NEXTSEC ((unsigned char)disk[sec][sectlen - 2]) + ((unsigned char)(disk[sec][sectlen - 3] & 0b00000011)) * 256
#define FLAG disk[dirsec][entrybeg]

void showdir(uint8_t columns);
void showdir64();
void showvtoc_map();
void rebuild_vtoc();
void cmp_vtoc_maps();
#if CMP_DISK
void print_mismatches();
#endif
char printbinary(unsigned char);
void print4secs();
uint16_t findfree(uint16_t sec);
void show_file_secs();
void replace_unprintable_characters(char* str);

#define SD_SEC_LEN 128
#define DD_SEC_LEN 256
#define TOTAL_SD_SECS 720
#define TOTAL_ED_SECS 1040
#define MAX_LEN 720 * 256	
#define VTOC_SEC360 360
#define MAPBYT_360_10 10
#define VTOC_SEC1024 1024
#define MAPBYT_1024_84 84
#define	DIR_BEG_SEC 361
#define DIR_END_SEC 368
#define DIR_ENTRY_SIZE 16   // Size of each directory entry
#define DIR_SEC_LEN 128
#define DIR_FLNM_LEN 11


bool sec_in_map1024_flg;
uint16_t total_secs = TOTAL_SD_SECS;		//total sectors
uint16_t sec;		//present sector
uint16_t sectlen;	//length of sector SD, DD: 128 ; ED: 256
uint8_t disk[TOTAL_ED_SECS][DD_SEC_LEN] = { {0} };	//complete disk image
#if CMP_DISK
uint8_t ds_sav[TOTAL_ED_SECS][DD_SEC_LEN] = { {0} };
#endif
uint8_t vtoc_sav[2][128] = { {0} };
uint8_t sector_filnr[1024] = { 0xff };
char axlflnm[DIR_FLNM_LEN + 1] = { 0 };	//atari filename FILENAMEEXT\0 (12 bytes)
uint8_t enhanced_density = 1;	//disk format--	0: single density and double density
							//				1: enhanced density


//////// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void file_to_image(char file_filename[500], char diskimage_name[500]) {
	uint16_t dirsec; 
	uint8_t entrybeg;
	uint32_t i, j, k;
	uint16_t cnt = 0;
	uint16_t cnt1024 = 0;
	char* asptr, * fsptr;
	bool entryfound, endfound;
	


	uint8_t header_len;	// length of header-- XFD: 0, ATR: 16
	uint32_t lenf;					//length of file
	uint32_t filsiz;		//length of Atari file
	char fs[500] = { 0 };
	uint8_t header[16] = { 0 };

	uint16_t filno_in_sec;
	uint16_t secold;
	uint8_t filpos_in_dir;		//present file in directory
	uint16_t frsectors_total, frsectors_360, frsectors_1024;
	uint16_t required_secs, seccount_360, seccount_1024, seccount_total;
	uint32_t fillen;
	bool mismatch_fg = 0;
	char* ext;
	char* ptr;
	FILE* filestream;
	

#if CREATE_EMPTY_ED_IMAGE


	if (fopen_s(&filestream, "empty_ed.xfd", "wb") != 0) {
		perror("Can't create empty_ed.xfd");
		exit(1);
	}
	for (i = 0; i < TOTAL_ED_SECS * SD_SEC_LEN; i++)
		fputc('\0', filestream);
	fclose(filestream);
	printf("\n\nempty_ed.xfd has been created.\n\n");
	hit_Enter();
	exit(1);

#endif

#if CREATE_ATR_FROM_XFD
	char atr_filename[500] = { 0 };
	FILE* filestream3;
#endif




	///////////////////////////////////////////////////////////////////////////////////////////
	

	// test validity of extension in disk image file name

	ext = diskimage_name + strlen(diskimage_name) - 4;
	ptr = ext;
	for (i = 0; i <= 3; i++) *ptr++ = tolower(*ptr);
	if (strcmp(ext, ".xfd") == 0)
		header_len = 0;
	else
		if (strcmp(ext, ".atr") == 0)
			header_len = 16;
		else {
			printf("%s Only ATR and XFD disk images are supported.", ext);
			hit_Enter();
			exit(1);
		}
	//



	if (fopen_s(&filestream, "savexfd.cfg", "wb") != 0) {
		puts("Can't open savexfd.cfg");
		perror("");
		exit(1);
	}
	else {
		fputs(diskimage_name, filestream);
		fclose(filestream);
	}




	// open disk image file


	if (fopen_s(&filestream, diskimage_name, "rb") != 0)	// disk image not found
	{
#if STAND_ALONE
		printf("Can't open %s", diskimage_name);
		perror("");
		exit(1);

#else	// Create new image containing DOS 2.5	

		if (errno == ENOENT) {
			printf("'%s' does not exist. Creating new image from DOS25 data.\n", diskimage_name);
		}
		else if (errno == EACCES) {
			printf("\n\aERROR: The file '%s' is currently in use.\nPlease close the Atari emulator.", diskimage_name);
			hit_Enter();
			exit(1);
		}
		
		
		if (fopen_s(&filestream, diskimage_name, "wb") != 0) {
			printf("Can't create '%s'", diskimage_name);
			perror("");
			exit(1);
		}

		if (fwrite(dos25_block, header_len, 1, filestream) != 1) { 
			perror("Error writing dos25_block header"); exit(1); 
		}
		else
			printf("dos25_block header written\n");

		if (fwrite(dos25_block + 16, DOS_BLOCK_SIZE - 16, 1, filestream) != 1) 		{ 
			perror("Error writing dos25_block"); exit(1); 
		}
		else
			printf("dos25_block written\n");

		for (i = DOS_BLOCK_SIZE - 16 + 1; i <= (360 - 1) * 128; i++) {
			if (fputc('\0', filestream) == EOF) { 
				perror("Error writing null character"); exit(1); }
		}

		if (fwrite(dos25_votc_dir, DOS25_VOTC_DIR_SIZE, 1, filestream) != 1) { 
			perror("Error writing dos25_votc_dir"); exit(1); 
		}

		for (i = (720 - 360 - 1) * 128 + DOS25_VOTC_DIR_SIZE; i < 720 * 128; i++) { 
			if (fputc('\0', filestream) == EOF) { perror("Error writing null character"); exit(1); } }

		if (filestream != NULL) {
			if (fclose(filestream) == 0) {
				printf("File closed successfully.\n");
			}
			else {
				perror("Error closing file");
			}
			filestream = NULL; // Set the file stream pointer to NULL
		}
		else {
			printf("File stream is already NULL.\n");
		}

		if (fopen_s(&filestream, diskimage_name, "rb") != 0) {	//Open diskimage file for reading.
			printf("Can't open %s", diskimage_name);
			perror("");
			exit(1);
		}
#endif	
	}
	printf("\nDisk image: %s\n", diskimage_name);

	fseek(filestream, 0, SEEK_END);
	lenf = ftell(filestream);

	if (lenf == -1L) {
		fclose(filestream);
		err("Error getting file length\n");
	}

	if (lenf > (uint32_t)(MAX_LEN + header_len)) file_err("File is too long:", diskimage_name);

	printf("Image has %li bytes\n", lenf);



	switch (lenf - header_len) {
	case TOTAL_SD_SECS * SD_SEC_LEN:
		printf("Single density:");
		enhanced_density = 0;
		total_secs = TOTAL_SD_SECS;
		sectlen = SD_SEC_LEN;
		break;
	
	case TOTAL_ED_SECS * SD_SEC_LEN:
		printf("Enhanced density:");
		enhanced_density = 1;
		total_secs = TOTAL_ED_SECS;
		sectlen = SD_SEC_LEN;
		break;
	
	case TOTAL_SD_SECS* DD_SEC_LEN:
		printf("Double density:");
		enhanced_density = 0;
		total_secs = TOTAL_SD_SECS;
		sectlen = DD_SEC_LEN;
		break;
	default:
		err("Size of disk image does not match SD, ED or DD.");
	}

	printf(" %i sectors of %i bytes\n", total_secs, sectlen);


	/***************************************************************/

	if (header_len == 16) {
		fseek(filestream, 0, SEEK_SET);			// Reset file ptr to beginning of ATR-file
		fread(header, 1, header_len, filestream);
	}



	/***** Show ATR header **************************************/

#if SHOW_ATR_HEADER
	if (header_len == 16) {
		printf("\nThe size of the ATR disk is $%X without header\n", lenf - header_len);
		fseek(filestream, 0, SEEK_SET);			// Reset file ptr to beginning of ATR-file
		puts("The standard ATR file format is: (first byte listed asss 1, not 0)");

		printf("%2X", fgetc(filestream)); puts(" * 01: $0296 (sum of NICKATARI) ");
		printf("%2X", fgetc(filestream)); puts(" * 02: ");
		printf("%2X", fgetc(filestream)); puts(" * 03: size of this disk image, in paragraphs (size/$10) , low");
		printf("%2X", fgetc(filestream)); puts(" * 04: size of this disk image, in paragraphs (size/$10) , high");
		printf("%2X", fgetc(filestream)); puts(" * 05: sector size. ($80 or $100) bytes/sector, low");
		printf("%2X", fgetc(filestream)); puts(" * 06: sector size. ($80 or $100) bytes/sector, high");
		printf("%2X", fgetc(filestream)); puts(" * 07: high part of size, in paragraphs (added by REV 3.00) ");
		printf("%2X", fgetc(filestream)); puts(" * 08..16: Unused");
		printf("%2X", fgetc(filestream)); puts(" * Remainder of image contains Atari data. ");
	}
	//
#endif


#if CREATE_ATR_FROM_XFD
	if (header_len == 0) {
		strcpy_s(atr_filename, sizeof(atr_filename), diskimage_name);
		ptr = strrchr(atr_filename, '.');
		*ptr = '\0';
		strcat_s(atr_filename, sizeof(".ATR") + 1, ".ATR");

		if (fopen_s(&filestream3, atr_filename, "wb") != 0) {
			printf("Can't open %s\n", atr_filename);
			perror("");
			exit(1);
		}
		printf("\nDisk image: ");
		puts(atr_filename);

		printf("The size of the ATR disk is $%X without header\n", lenf - header_len);
		//		fseek (filestream3, 0 , SEEK_SET);			// Reset file ptr to begenning of ATR-file

		fputc(0x96, filestream3);           // * 01: $0296 (sum of NICKATARI) ");
		fputc(0x02, filestream3);
		fputc((lenf / 0x10) % 256, filestream3);// * 03: size of this disk image, in paragraphs (size/$10) , low");
		fputc((lenf / 0x10) / 256, filestream3);// * 04: size of this disk image, in paragraphs (size/$10) , high");
		fputc(sectlen % 256, filestream3);        // * 05: sector size. ($80 or $100) bytes/sector, low");
		fputc(sectlen / 256, filestream3);        // * 06: sector size. ($80 or $100) bytes/sector, high");
		fputc((lenf / 0x10) / 256, filestream3);// * 07: high part of size, in paragraphs (added by REV 3.00) ");
		for (cnt = 8; cnt <= 16; cnt++)// * 08..16: Unused");
			fputc(0, filestream3);
	}

#endif


	/************** Get whole disk image *******************************/
	printf("Get disk image .......\n");

	if (fseek(filestream, header_len, SEEK_SET))			// Reset file ptr
		err("\nfailed to reset file pointer");

	for (sec = 1; sec < total_secs; sec++) {
		if (fread(&disk[sec][0], 1, sectlen, filestream) == 0)
			err("\nEnd of file reached before all sectors read");
	}
	fclose(filestream);

#if FIRST_SECS_VTOC
	print4secs();
#endif

	// (secs 1 to 3 are always 128 bytes)
#if CMP_DISK
	// write the whole of disk to ds_sav
	for (i = 0; i < TOTAL_ED_SECS; i++) {
		for (j = 0; j < DD_SEC_LEN; j++) {
			ds_sav[i][j] = disk[i][j];
		}
	}

#endif


	/******************** Check Integrity of Disk *************************************/
	printf("\nChecking integrity of file structure\n\n");

	char filpos_in_dir_sav = 255;
	endfound = 0;
	filpos_in_dir = 0;
	mismatch_fg = 0;
	//memset(sector_filnr, 0xff, 1024);			// Yes, that code is correct. (ChatGTP)
	for (dirsec = DIR_BEG_SEC; dirsec < 369; dirsec++)				// For each Directory sector
	{
		for (entrybeg = 0; entrybeg <= 112; entrybeg += 16)			// For each directory entry in sector
		{
			if (FLAG == 0)				// If byte 0 of entry == 0, end of file list
			{
				endfound++;
				break;
			}
			if (!(FLAG & 128))				// If byte 0 of entry not inverted, valid file present
			{
				fillen = DPEEK(disk, dirsec, entrybeg + 1);				//bytes 1,2: File length (not crucial)
				sec = DPEEK(disk, dirsec, entrybeg + 3);					//bytes 3,4: first data sector of file
				if(sec >= total_secs){
					showdir64();
					err("\nInvalid sector number");
				}


				memcpy(axlflnm, &disk[dirsec][entrybeg + 5], DIR_FLNM_LEN);		//bytes 5 to 15: File name
				axlflnm[DIR_FLNM_LEN] = '\0';
#if STAND_ALONE 
				if (argcount != 3) printf("%2i> %s %3i\n", filpos_in_dir, axlflnm, fillen);
#endif

				do {
					filno_in_sec = (unsigned char)disk[sec][sectlen - 3] >> 2;	//filenumber contained in this file sector
					
					if (filno_in_sec != filpos_in_dir) {

						if (filpos_in_dir_sav != filpos_in_dir) {
							printf("\a\nWARNING- A sector in '%s' points to sector-%i, which is used by file-%i", axlflnm, sec, filno_in_sec);
							filpos_in_dir_sav = filpos_in_dir;
							mismatch_fg = 1;
							//hit_Enter();
						}
					}
					sector_filnr[sec] = (char)filpos_in_dir;
					//printf(" %-3i",sector_filnr[sec]);
					sec = FILE_NEXTSEC;		//Last two bytes: next sector
				} while (sec); // zero indicates EOF
			}
			filpos_in_dir++;
		}
		if (endfound)
			break;
	}
	if (mismatch_fg == 1) {
		printf("\nThe structure of these files is corrupted!!!\n");
		printf("Continue on to file-sector map...\n");
		hit_Enter();

		show_file_secs();
		hit_Enter();
	}
	else {
		
#if SHOW_FILE_SECS
		printf("Showing file-sector map...\n");
		show_file_secs();
#endif
	}

	for (i = 0; i <= 3; i++)		// First four sectors are off limits
		sector_filnr[i] = 128;

	for (i = VTOC_SEC360; i <= DIR_END_SEC; i++)	// VTOC and Directory
		sector_filnr[i] = 128;


#if SHOW_DIR	
	showdir(3);
#endif

#if SHOW_DIR64	
	showdir64();
#endif

#if SHOW_VTOC_MAP
	showvtoc_map();
#endif

#if REBUILD_VTOC
	rebuild_vtoc();
	cmp_vtoc_maps();
#endif
#if FIRST_SECS_VTOC
	print4secs();
#endif

#if SHOW_VTOC_MAP
	showvtoc_map();
#endif



	/*******************************************************/
#if STAND_ALONE == 0
	if (sectlen == DD_SEC_LEN)//TURBODOS -- this is not a complete test for TURBODOS
	{

		strcpy_s(fs, sizeof(fs), file_filename);
		for (i = 0; i < strlen(fs); i++)
			fs[i] = toupper(fs[i]);
		if (strcmp(fs, "AUTORUN.SYS") == 0) {
			puts("Can't use AUTORUN.SYS with TURBODOS");
			strcpy_s(file_filename, sizeof(file_filename), src_filename);
			//strcpy(strchr(file_filename, '.'), ".LOA");
			strcpy_s(strchr(file_filename, '.'), strlen(".LOA") + 1, ".LOA");

			remove(file_filename);
			if (rename("AUTORUN.SYS", file_filename) == 0)
				printf("AUTORUN.SYS has been renamed to %s\n", file_filename);
			else
				err("AUTORUN.SYS could not be renamed");
		}
		if (strcmp(fs, "AUTORUN.SMT") == 0) {
			puts("Can't use AUTORUN.SMT with TURBODOS");
			strcpy_s(file_filename, sizeof(file_filename), src_filename);
			strcpy_s(strchr(file_filename, '.'), strlen(".SMT") + 1, ".SMT");
			remove(file_filename);
			if (rename("AUTORUN.SMT", file_filename) == 0)
				printf("AUTORUN.SMT has been renamed to %s\n", file_filename);
			else
				err("AUTORUN.SMT could not be renamed");
		}
	}
#endif
	/*******************************************************/


	strcpy_s(fs, sizeof(fs), file_filename);
	while (strchr(fs, '?'))
		*strchr(fs, '?') = ' ';



	/**************** Format file name for directory *******************/

	if ((fsptr = strrchr(fs, '\\')) != 0)
		fsptr++;
	else
		fsptr = fs;

	// Check for correct filename format while moving filename to axlflnm.



	strcpy_s(axlflnm, sizeof(axlflnm), "           ");//8+3 blanks
	asptr = axlflnm;


	if (!isalpha(*fsptr)) //check that first character of filename is a letter
		err("Filename must begin with a letter.");


	cnt = 0;
	while (isalnum(*fsptr)) {
		if (++cnt == 9)
			err("Filename is too long.");
		*asptr++ = toupper(*fsptr++);
	}

	if (*fsptr)			// non alphanum character present
	{
		if (*fsptr++ == '.')	// file extention present
		{
			asptr = axlflnm + 8;
			cnt = 0;
			while (*fsptr) {
				if (++cnt == 4)
					err("Filename extension is too long.");

				if (!isalnum(*fsptr))
					err("Only alpha-numerical characters are allowed in extension.");

				*asptr++ = toupper(*fsptr++);
			}
		}
		else
			err("Only alpha-numerical characters are allowed in filename.");

	}

	//	printf ("Directory entry in xfd disk: '%s'\n",axlflnm);
	//	hit_Enter();
	//	exit(1);

		/**************** Open ATARI file for reading ***************************/


	if (fopen_s(&filestream, fs, "rb") != 0) {
		printf("Can't open %s. ", fs);
		perror("");
		exit(1);
	}




	fseek(filestream, 0, SEEK_END);			// find size of file (filsiz)
	filsiz = ftell(filestream);

	if (filsiz == 0) file_err("File is empty:", file_filename);

	required_secs = filsiz / (sectlen - 3); // full sectors
	if (filsiz % (sectlen - 3))				// partial sectors
		required_secs++;

	fseek(filestream, 0, SEEK_SET);			// Reset file ptr

	frsectors_total = frsectors_360 = DPEEK(disk, VTOC_SEC360, 3);//VOTC





	if (enhanced_density)
	{
		frsectors_1024 = DPEEK(disk, VTOC_SEC1024, 122);	//free sectors in enhanced region 1024 
		frsectors_total = frsectors_360 + frsectors_1024;
	}
	//S.155, Atari Profibuch)




	/********************    Search for file in directory   ********************/

	printf("\nSearch for '%s' in directory..............", axlflnm);

	filpos_in_dir = 0;
	sec = 0;
	entryfound = endfound = sec_in_map1024_flg = 0;

	for (dirsec = DIR_BEG_SEC; dirsec < 369; dirsec++)				// For each Directory sector
	{
		for (entrybeg = 0; entrybeg <= 112; entrybeg += 16)			// For each directory entry in sector
		{
			if (FLAG == 0)				// If byte 0 (flag) of entry == 0, end of file list
			{
				endfound++;
				break;
			}
			if (!(FLAG & 0b10000000))							// If byte 0 of entry not inverted, valid file present
			{
				if (memcmp(&(disk[dirsec][entrybeg + 5]), axlflnm, DIR_FLNM_LEN) == 0)	// filename entry found
				{ //Writing 8 + 3 causes message: A sub-expression may overfow before being assigned to a larger type
					entryfound++;
					break;
				}
			}
			filpos_in_dir++;
		}
		if (entryfound || endfound)
			break;
	}

	if (entryfound) {
		printf("\nThe entry '%s' has been found at position %i\n", axlflnm, filpos_in_dir);
		frsectors_total += DPEEK(disk, dirsec, entrybeg + 1);// add size of file already on disk
	}
	else
		printf("\nThe entry '%s' was not found\n\n", axlflnm);

	printf("%i sectors available for %s\n", frsectors_total, fs);

	if (required_secs > frsectors_total)
		err("The file is too large to fit on this disk");



	/* Directory entry (fillen is needed only for DIR listing) */
/*   0     |     1      |    2       |      3          |      4        |   5-12      | 13-15   */
/*   flag  |  fillen:L  |  fillen:H  |  startsector:L  | startsector:H |  Filename   |  ext    */

/* Flagbits:   7.filedeleted   6.sec<720 only   5.filelocked   1. DOS2   0.sec>=720 used   */

/* File structure
			  |         sctlen-3            | sctlen-2    |    sctlen-1                   */
			  /* 0-7C(FC)   | 7D(FD)   7 - 6     1,0      |  7E(FE)     |   7F(FF)                      */
			  /*  DATA      |         fileno   nxtsec:H   |  nxtsec:L   |   # bytes (usually sctlen-3)  */

/* VTOC_SEC360 */
/*   0      |      1      |     2       |         3           |        4         | 5 - 9    */
/*  2 (DOS) | 707 or 1010 total sectors |     free sectors (0, when over 707)    | reserved */


/******************** If entry found: Delete file in directory and VOTC *******************************/

	if (entryfound) {
		printf("Entry found.....deleting %s in directory\n\n", fs);

		FLAG |= 0b10000000;// delete entry in directory

		sec = DPEEK(disk, dirsec, entrybeg + 3);//Start sector
		cnt = 0;
		cnt1024 = 0;

		do// won't work if length of file is 0
		{
			sector_filnr[sec] = 0xff;

			if (sec < TOTAL_SD_SECS) {
				disk[VTOC_SEC360][MAPBYT_360_10 + sec / 8] |= (unsigned char)(0b10000000 >> (sec % 8));//set bit to 1 in VOTC
				cnt++;
			}
			else {
				disk[VTOC_SEC1024][MAPBYT_1024_84 + (sec - TOTAL_SD_SECS) / 8] |= (unsigned char)(0b10000000 >> ((sec - TOTAL_SD_SECS) % 8));//set bit to 1 in VOTC -- ED
				cnt1024++;
			}

			sec = FILE_NEXTSEC;

		} while (sec);

		frsectors_360 += cnt;
		disk[VTOC_SEC360][3] = frsectors_360 & 0xFF; // Write lower byte of frsectors to VTOC
		disk[VTOC_SEC360][4] = frsectors_360 >> 8;   // Write higher byte of frsectors to VTOC

		frsectors_total = frsectors_360;

		if (enhanced_density) {
			frsectors_1024 += cnt1024;
			disk[VTOC_SEC1024][122] = frsectors_1024 & 0xFF; // Write lower byte of frsectors_1024 to VTOC
			disk[VTOC_SEC1024][123] = frsectors_1024 >> 8;   // Write higher byte of frsectors_1024 to VTOC

			frsectors_total += frsectors_1024;
		}
	}

	// end Delete file in directory and VOTC

		/********************    Find free directory position (filpos_in_dir)   ********************/

#if REBUILD_VTOC
	rebuild_vtoc();
	cmp_vtoc_maps();
#endif

#if SHOW_VTOC_MAP
	showvtoc_map();
#endif



	printf("Find free directory position (filpos_in_dir)\n\n");

	filpos_in_dir = 0;
	sec = 0; 
	entryfound = 0;

	for (dirsec = DIR_BEG_SEC; dirsec <= DIR_END_SEC; dirsec++)						// For each Directory sector
	{
		for (entrybeg = 0; entrybeg <= 112; entrybeg += 16)					// For each directory entry in sector
		{
			if ((FLAG & 128) || (FLAG == 0))	//erased or unused
			{
				entryfound++;
				break;
			}
			filpos_in_dir++;
			//printf ("filpos_in_dir = %i, first byte = %i\n",filpos_in_dir,FLAG);
		}
		if (entryfound)
			break;
	}

	if (filpos_in_dir == 64)
		err("Directory is full with 64 files.");

	/**************** Write directory entry *************************************/

	printf("Position %d in directory is free.\n", filpos_in_dir);
	printf("Writing %s to disk image\n", fs);

	sec = findfree(0);	// determine sec, and change bit to 0 in VTOC when found

	short isv = dirsec;			//Will be needed to set flag if some sectors > 0
	short jsv = entrybeg;



	disk[dirsec][entrybeg + 1] = (char)(filsiz / (sectlen - 3) + 1);			//File length in secs (needed only for DIR listing)
	disk[dirsec][entrybeg + 2] = (char)((filsiz / (sectlen - 3) + 1) / 256);

	disk[dirsec][entrybeg + 3] = (char)sec;					//Start sector number
	disk[dirsec][entrybeg + 4] = (char)(sec / 256);

	memcpy(&disk[dirsec][entrybeg + 5], axlflnm, DIR_FLNM_LEN);
	
	//printf ("Start sector low: %X    hi: %X\n\n",(unsigned char)disk[dirsec][entrybeg+3],(unsigned char)disk[dirsec][entrybeg+4]);
	//printf ("Start sector: %i\n",DPEEK(disk, dirsec, entrybeg + 3);



	/**************** Get full sectors **************************************/
	printf("Geting full sectors from disk image\n\n");

	k = 0;
	seccount_360 = 0;
	seccount_1024 = 0;			//ED



	if (filsiz >= (uint32_t)(sectlen - 3))//at least one full sector
	{
		for (i = 1; i <= filsiz - (sectlen - 3); i += (sectlen - 3)) {
			fread(&disk[sec][0], (size_t)(sectlen - 3), 1, filestream);	//read sectlen-3 bytes into disk
			secold = sec;
			sec < TOTAL_SD_SECS ? seccount_360++ : seccount_1024++;																			//ED
			sec = findfree(sec);								//determine sec, and change bit to 0 in VTOC when found

			//file position in directory / next sec H:
			disk[secold][sectlen - 3] = (uint8_t)((4 * filpos_in_dir) + (sec / 256));
			disk[secold][sectlen - 2] = (uint8_t)(sec & 0xFF);					    //next sec L:
			disk[secold][sectlen - 1] = (uint8_t)(sectlen - 3);					//number of data bytes
		}
	}


	/**************** Get final (partial) sector *****************************/

	printf("Geting final (partial) sector\n\n");

	i = filsiz % (sectlen - 3); // No partial sector if i == 0
	if (i) {
		fread(&disk[sec][0], i, 1, filestream);
		disk[sec][sectlen - 3] = (uint8_t)(filpos_in_dir * 4);	//file position (high byte of following sector: 0)
		disk[sec][sectlen - 2] = (uint8_t)0;					//low byte des folgenden Sectors: 0
		disk[sec][sectlen - 1] = (uint8_t)i;					//number of data bytes
		sec < TOTAL_SD_SECS ? seccount_360++ : seccount_1024++;											//ED
	}
	fclose(filestream);

	seccount_total = seccount_360 + seccount_1024;
	if (seccount_total - required_secs) //this is just a check
	{
		printf("\aRequire %i sectors, but %i were written", required_secs, seccount_total);
		hit_Enter();
	}



	/****************	Write # 'free sectors' to VTOC (sec 360 ) ********************/

	printf("Write # free sectors to VTOC (sec 360 )\n\n");

	frsectors_360 = DPEEK(disk, VTOC_SEC360, 3);												//ED
	frsectors_360 -= seccount_360;																//ED
	disk[VTOC_SEC360][3] = (unsigned char)(frsectors_360 & 0xFF);  //LO
	disk[VTOC_SEC360][4] = (unsigned char)(frsectors_360 >> 8);   //HI    (same result as below)
	printf("%d free sectors < 720:\n", frsectors_360);											//ED
	frsectors_total = frsectors_360;															//ED

	if (enhanced_density)//VTOC (sector 1024)															//ED
	{
		frsectors_1024 = DPEEK(disk, VTOC_SEC1024, 122);										//ED
		frsectors_1024 -= seccount_1024;														//ED
		disk[VTOC_SEC1024][122] = (unsigned char)(frsectors_1024 % 0xFF);						//ED
		disk[VTOC_SEC1024][123] = (unsigned char)(frsectors_1024 / 0xFF);						//ED
		printf("%d free enhanced sectors > 720:\n", frsectors_1024);							//ED
		frsectors_total += frsectors_1024;														//ED

		// enhanced density -- mirror bytes 16 to 99 of sector 360 in sector 1024
		// bytes 0 to 83 are probably not necessary
		for (j = 16; j <= 99; j++)																//ED
			disk[VTOC_SEC1024][j - 16] = disk[VTOC_SEC360][j]; 									//ED
	}

	//Flag- only sectors <720: bit-6 = 1  ; some sectors are >720: bit-0 = 1 
	(seccount_1024 == 0) ? (disk[isv][jsv] = 0b01000010) : (disk[isv][jsv] = 0b00000011);		//ED

	printf("%d total free sectors:\n", frsectors_total);										//ED



	/******************      Save disk image          ***********************************/

#if SAVE_DISK_IMAGE
	if ((strcmp(ext, ".XFD") == 0) || (strcmp(ext, ".xfd") == 0))
		header_len = 0;
	else
		header_len = 16;

	if (fopen_s(&filestream, diskimage_name, "wb") != 0) {
		printf("Can't create %s", diskimage_name);
		perror("");
		exit(1);
	}

#if SHOW_VTOC_MAP
	showvtoc_map();
#endif

#if REBUILD_VTOC
	rebuild_vtoc();
	cmp_vtoc_maps();
#endif
#if CMP_DISK
	print_mismatches();
#endif

	printf("\nSaving %s", diskimage_name);
	fwrite(header, 1, header_len, filestream);
	
	// SD XFD disk size: 128 * 720 = 92,160 bytes 
	// ED XFD disk size: 128 * TOTAL_ED_SECS = 133,120 bytes
	total_secs = (enhanced_density) ? TOTAL_ED_SECS : TOTAL_SD_SECS;

	for (sec = 1; sec <= total_secs; sec++)
		if (fwrite(&disk[sec][0], sectlen, 1, filestream) != 1) puts("disk write error");
	fclose(filestream);

	printf("...... done.\n\n");

#else
	printf("\n\aWARNING- disk image not saved!\n");
	printf("To save disk image, set 'SAVE_DISK_IMAGE 1' in file_to_image.c");
	hit_Enter();

#endif //Save disk image


/****************     Create ATR image from XFD image  ******************************************/


#if CREATE_ATR_FROM_XFD
	if (header_len == 0) {
		printf("Saving %s\n\n", atr_filename);

		total_secs = (enhanced_density) ? TOTAL_ED_SECS : TOTAL_SD_SECS;

		for (sec = 1; sec <= total_secs; sec++)
			if (fwrite(&disk[sec][0], sectlen, 1, filestream3) != 1) {
				puts("disk write error");
				perror("");
				exit(1);
			}
		fclose(filestream3);
	}

#endif

	/********************************/


	printf("\n\n*************************************************************************\n\n");

} //End Main




/*************************************************************************************/
/******************* S U B R O U T I N E S *******************************************/
/*************************************************************************************/





/****************     Subroutine  findfree  ******************************************/



uint16_t findfree(uint16_t sc) {
	uint16_t  pos = 0, entrybeg, sec;
	uint8_t byt, bitcor;
	bool found = 0;
	uint16_t vtoc_sec = VTOC_SEC360;
	uint16_t vtoc_byt = MAPBYT_360_10;
	

	if (sc < TOTAL_SD_SECS) {
		for (pos = MAPBYT_360_10 + sc / 8; pos < MAPBYT_360_10 + 90; pos++)		//look for free byte (90 * 8 = 720)
		{
			if (disk[VTOC_SEC360][pos])						//at least one bit is set -- free sector
			{
				found = 1;
#if SHOW_FINDFREE
				printf("A bit, 1, found in byte %i of VTOC sector 360\n", pos);
#endif
				break;
			}
		}
#if SHOW_FINDFREE
		printf("disk[VTOC_SEC360][%d] ", pos); printbinary(disk[VTOC_SEC360][pos]); printf("\n");
#endif
	}
	if (found == 0)	//either enhanced density or no free sector
	{
		if (total_secs == TOTAL_ED_SECS)       //****************** enhanced density **********************************
		{
			if (sc < TOTAL_SD_SECS)
				sc = TOTAL_SD_SECS;
			vtoc_sec = VTOC_SEC1024;
			vtoc_byt = MAPBYT_1024_84;

			for (pos = MAPBYT_1024_84 + (sc - TOTAL_SD_SECS) / 8; pos < MAPBYT_1024_84 + 38; pos++)		//look for free byte in sec 1024
			{
				if (disk[VTOC_SEC1024][pos])						//at least one bit is set -- free sector
				{
					found = 1;
#if SHOW_FINDFREE
					printf("A bit, 1, found in byte %i of VTOC sector 1024\n", pos);
#endif
					sec_in_map1024_flg = 1;
					break;
				}
			}

		}
		else //SD or DD
			err("Free sector not found -- disk full");
	}
	if (found == 0)
		err("Free sector not found -- disk full");

	byt = disk[vtoc_sec][pos];
	bitcor = 0b01111111;

	for (entrybeg = 0; entrybeg <= 7; entrybeg++)		//look for free bit
	{
		if (byt & 0b10000000)
			break;
		byt <<= 1;
		bitcor >>= 1;
		bitcor |= 0b10000000;
	}

	sec = (pos - vtoc_byt) * 8 + entrybeg;
	if (sec_in_map1024_flg)
		sec += TOTAL_SD_SECS;

#if SHOW_FINDFREE
	printf("Sector%4i, slot %i\n\n", sec, entrybeg + 1);
#endif

	if (((sec >= VTOC_SEC360) && (sec <= DIR_END_SEC)) || (sec >= VTOC_SEC1024)) {
		err("\aTried to write file to invalid sector (VTOC or directory)");
	}
	disk[vtoc_sec][pos] &= bitcor;	//set the bit in vtoc_sec

	//printf ("sector: %d\t pos= %d\tvalue = %d\n",sec,pos,disk[vtoc_sec][pos]);

	return(sec);
}





/******************** error *********************************************/

void err(char* errstr)

{
	printf("\n\n\aERROR: %s\n", errstr);
	hit_Enter();
	exit(1);
}

/******************** error *********************************************/

void file_err(char* errstr, char filename[]) {

	printf("\n\n\aERROR: %s %s\n", errstr, filename);
	hit_Enter();
	exit(1);
}
/***********************************************************************************/





// *****************Rebuild VTOC*********************

void rebuild_vtoc() {
	unsigned int i, j;
	uint16_t dirsec, sec, freesecs, sec_count;
	uint8_t entrybeg, filpos_in_dir;
	uint32_t fillen;
	bool endfound = 0;
	uint16_t cnt = 0;
	bool enh_flg;
	char flag;

	for (j = 0; j < 128; j++)	  // save the VTOC before rebuilding
	{								
		vtoc_sav[0][j] = disk[VTOC_SEC360][j];
		vtoc_sav[1][j] = disk[VTOC_SEC1024][j];
	}

	printf("\nRebuilding VTOC ....... (REBUILD_VTOC is set to 1 in source code.)\n\n");

	filpos_in_dir = 0;
	memset(sector_filnr, 0xff, 1024);
	for (dirsec = DIR_BEG_SEC; dirsec < 369; dirsec++)					// For each Directory sector
	{
		for (entrybeg = 0; entrybeg <= 112; entrybeg += 16)			// For each directory entry in sector
		{
			flag = FLAG;
			if (flag == 0)					// If byte 0 of entry == 0, end of file list
			{
				endfound = 1;
				break;
			}
			if (!(flag & 128))				// If byte 0 of entry not inverted, non-deleted file present
			{
				sec_count = 0;
				enh_flg = 0;
				sec = DPEEK(disk, dirsec, entrybeg + 3);							//bytes 3,4: first data sector of file
															 

				do {
					sector_filnr[sec] = (char)filpos_in_dir;
					//printf(" %-3i",sector_filnr[sec]);
					sec_count++;
					if (sec > TOTAL_SD_SECS) enh_flg = 1;
					sec = FILE_NEXTSEC;//Last two bytes: next sector

				} while (sec); // zero indicates EOF

				fillen = DPEEK(disk, dirsec, entrybeg + 1);
				if (fillen != sec_count) {
					printf("Correcting directory....\n");
					disk[dirsec][entrybeg + 1] = sec_count & 0xFF; // Write lower byte of file length to directory
					disk[dirsec][entrybeg + 2] = sec_count >> 8; // Write higher byte of file length to directory
					printf("The displayed size in sectors of file-%i has been changed from %i to %i\n", filpos_in_dir, fillen, sec_count);
				}
				if (enh_flg) {
					FLAG |= 0b00000001;
					FLAG &= 0b10111111;
				}
				else {
					FLAG |= 0b01000000;
					FLAG &= 0b11111110;
				}
				if (FLAG != flag) {
					printf("Flag of file-%i corrected: ", filpos_in_dir);
					printf("was "); printbinary(flag); printf(", is now "); printbinary(FLAG); printf("\n");
				}
			}
			filpos_in_dir++;
		}
		if (endfound)
			break;
	}

	// count sectors used by files

	freesecs = 707;
	for (cnt = 0; cnt < TOTAL_SD_SECS; cnt++) {
		if (sector_filnr[cnt] != 0xff)
			freesecs--;
	}
	disk[VTOC_SEC360][3] = freesecs & 0xFF; // Write lower byte of freesectors to VTOC
	disk[VTOC_SEC360][4] = freesecs >> 8; // Write higher byte of freesectors to VTOC

	freesecs = 1010 - 707;
	for (cnt = TOTAL_SD_SECS; cnt < 1024; cnt++) {
		if (sector_filnr[cnt] != 0xff)
			freesecs--;
	}
	disk[VTOC_SEC1024][122] = freesecs & 0xFF; // Write lower byte of freesecs to VTOC
	disk[VTOC_SEC1024][123] = freesecs >> 8; // Write higher byte of freesecs to VTOC


	// Complete filling sector_filnr[]
	// an entry 0 to 64 indicates sector in use. (64 indicates directory or VTOC)

	for (i = VTOC_SEC360; i <= DIR_END_SEC; i++)
		sector_filnr[i] = 128;					// 128 is cannot be a file number


	for (int i = MAPBYT_360_10; i <= 99; i++)
		disk[VTOC_SEC360][i] = 0b11111111;		// set all bits in map to 1 (unused)
	for (int i = MAPBYT_1024_84; i <= 121; i++)
		disk[VTOC_SEC1024][i] = 0b11111111;		// for ED too.

	for (i = 0; i <= 3; i++)		// First four sectors are off limits
		sector_filnr[i] = 128;		// 128 is non zero and not a file number

	sector_filnr[720] = 128;		// 1st sector in enhanced is unused

	i = 0;
	do {
		unsigned char bytepos = (i / 8) + MAPBYT_360_10; // Calculate byte position in disk[VTOC_SEC360], offset 10
		unsigned char bitpos = i % 8; // Calculate bit position within the byte

		if (sector_filnr[i] != 0xff)  // if sector is in use,
		{
			// clear the corresponding bit in disk[VTOC_SEC360] to 0.  
													// (128 >> bitpos) generates a mask with a single bit set at the position determined by bitpos.
			disk[VTOC_SEC360][bytepos] &= ~(128 >> bitpos); //~(128 >> bitpos) complements this mask, making all bits 1 except the one at the position determined by bitpos.
			cnt++;									//disk[VTOC_SEC360][bytepos] &= ~(128 >> bitpos) performs a bitwise AND operation 
			//to clear the bit at the specified position within disk[VTOC_SEC360][bytepos].
		}

		i++; // Move to the next element in sector_filnr

	} while (i < TOTAL_SD_SECS); // Continue until all elements in sector_filnr are processed



	if (total_secs == TOTAL_ED_SECS)       //enhanced density
	{
		i = 0; //
		j = 6 * 8;
		do {
			unsigned char bytepos = (i / 8); // Calculate byte position in disk[VTOC_SEC1024]
			unsigned char bitpos = i % 8; // Calculate bit position within the byte

			if (sector_filnr[j] != 0xff) {
				// Clear the corresponding bit in disk[VTOC_SEC1024] to 0
				disk[VTOC_SEC1024][bytepos] &= ~(128 >> bitpos);
			}

			i++; // Move to the next element in sector_filnr
			j++;

		} while (j < 1024); // Continue until all elements in sector_filnr are processed
	}

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////

/************** Show complete Directory - all 64 entries ****************/

/* Directory entry (fillen is needed only for DIR listing) */
/*   0     |     1      |    2       |      3          |      4        |   5-12      | 13-15   */
/*   flag  |  fillen:L  |  fillen:H  |  startsector:L  | startsector:H |  Filename   |  ext    */

/* Flagbits:   7.filedeleted   6. sec<720 only   5.filelocked  4.  3.  2.  1. DOS2   0.  sec>=720 in use */

void showdir(uint8_t columns) {
	uint16_t dirsec, sec_count = 0;
	unsigned int cnt = 1;
	uint8_t flag, entrybeg;
	uint8_t file_num = 0;
	char axlflnm[DIR_FLNM_LEN+1] = { 0 };

	for (dirsec = DIR_BEG_SEC; dirsec <= DIR_END_SEC; dirsec++) // directory is in sectors 361 to 368
	{
		for (entrybeg = 0; entrybeg <= 112; entrybeg += DIR_ENTRY_SIZE) // maximum of 64 entries of length 16, 7 entries per sector
		{
			flag = disk[dirsec][entrybeg];
			if (!(flag & 0b10000000) && flag) // File not deleted
			{
				// Copy XL File name
				memcpy(axlflnm, &disk[dirsec][entrybeg + 5], DIR_FLNM_LEN);
				
				replace_unprintable_characters(axlflnm);
				// Display file information
				printf("%c%2i) %11s%c %5i",
					(flag & 0b00000001) ? '<' : ' ',
					file_num++,
					axlflnm,
					(flag & 0b00000001) ? '>' : ' ',
					DPEEK(disk, dirsec, entrybeg + 1)
				);

				sec_count += DPEEK(disk, dirsec, entrybeg + 1);

				printf((cnt++ % columns) ? " | " : "\n"); // three columns
			}
		}
	}
	printf("\n%i sectors in use according to directory\n", sec_count);
	printf("%i free sectors\n", DPEEK(disk, VTOC_SEC360, 1) - sec_count); // (707 or 1010) - sec_count
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
void showdir64() {
	unsigned int dirsec;
	uint8_t flag, entrybeg;
	uint8_t filpos_in_dir = 0;
	
	char axlflnm[DIR_FLNM_LEN + 1 ] = { 0 };

	printf("\nDirectory listing:\n\n");
	printf("filpos   Filename    sectors  dirsec  entrybeg    flag\n\n");
	for (dirsec = DIR_BEG_SEC; dirsec <= DIR_END_SEC; dirsec++) { //directory is in sectors 361 to 368

		for (entrybeg = 0; entrybeg <= 112; entrybeg += DIR_ENTRY_SIZE) { //maximum of 64 entiries of length 16, 7 entries pro sector

			flag = FLAG;
			memcpy(axlflnm, &disk[dirsec][entrybeg + 5], DIR_FLNM_LEN);		//XL File name
			replace_unprintable_characters(axlflnm);

			printf("%2i.  ", filpos_in_dir);
			if (flag & 0b10000000)
				printf("d ");
			else if (flag & 0b00000001)
				printf("< ");
			else
				printf("  ");

			printf("%11s", axlflnm);

			if (flag & 0b10000000)
				printf(" d");
			else if (flag & 0b00000001)
				printf(" >");
			else
				printf("  ");

			printf("  %5i   ", DPEEK(disk, dirsec, entrybeg + 1));

			if (entrybeg == 0)
				printf("%4i  ", dirsec);
			else
				printf("      ");

			printf("   %4i     ", entrybeg);

			printbinary(flag);

			if (flag & 0b10000000) printf("  deleted");

			printf("\n");
			filpos_in_dir++;

		}
	}
	printf("filpos   Filename    sectors  dirsec  entrybeg    flag");
	//hit_Enter();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void showvtoc_map() {
	unsigned int cnt;
	uint16_t mapfrsecs = 0;

	for (cnt = 0; cnt < sectlen; cnt++) {
		printf("Byte%3i, ", cnt);

		switch (cnt) {
		case 0:
			printf("%02x    DOS %i     ", disk[VTOC_SEC360][0], disk[VTOC_SEC360][0]);
			break;
		case 1:
			printf("%02x    total sectors: %i", disk[VTOC_SEC360][1], DPEEK(disk, VTOC_SEC360, 1));
			break;
		case 2:
			printf("%02x", disk[VTOC_SEC360][2]);
			break;
		case 3:
			printf("%02x    free sectors < 720: %i", disk[VTOC_SEC360][3], DPEEK(disk, VTOC_SEC360, 3));
			break;
		case 4:
			printf("%02x", disk[VTOC_SEC360][4]);
			break;
		case 9:
			printf("             VTOC Map");
			break;
		default:
			if (cnt >= MAPBYT_360_10 && cnt <= 99)
				printf("Sector%4i>  ", (cnt - MAPBYT_360_10) * 8);
			else if (cnt >= 5 && cnt <= 99)
				printf("          ");
			else if (cnt >= 100)
				printf("         ");
			break;
		}

		if (cnt >= MAPBYT_360_10 && cnt <= 99)
			mapfrsecs += printbinary(disk[VTOC_SEC360][cnt]);
		//printf("  ");
		else
			printf("            ");

		if (enhanced_density) // Enhanced format
		{
			printf("   ");
			if (cnt == 82)
				printf("  ENHANCED DENSITY SECTORS");
			else if (cnt == 83)
				printf("VTOC Map");
			else if (cnt >= MAPBYT_1024_84 && cnt < 122) {
				mapfrsecs += printbinary(disk[VTOC_SEC1024][cnt]);
				printf(" Sector%4i, Byte %i", ((cnt + 6) * 8), cnt);
			}
			else if (cnt == 122)
				printf("%02x  Free sectors enhanced: %i", disk[VTOC_SEC1024][cnt], DPEEK(disk, VTOC_SEC1024, 122));
			else if (cnt == 123)
				printf("%02x    ", disk[VTOC_SEC1024][cnt]);
			else if (cnt == 124)
				printf("    Total free sectors: %i", DPEEK(disk, VTOC_SEC360, 3) + DPEEK(disk, VTOC_SEC1024, 122));
		}
		if (cnt == 126)
			printf("    Free bits in map: %i", mapfrsecs);

		printf("\n");
	}
	//hit_Enter();
}


void cmp_vtoc_maps() {
	unsigned int cnt;
	uint16_t mapfrsecs = 0;
	bool votc_same_fg = 1;

	printf("\nComparing old VTOC with re-build\n");
	printf("\n                     Old VTOC                   VTOC re-build");
	for (cnt = 0; cnt < 5; cnt++) {
		printf("\nByte% 3i", cnt);
		if (cnt == 0) {
			printf("               DOS %i", vtoc_sav[0][0]);
			printf("                         DOS %i", disk[VTOC_SEC360][0]);
		}
		else if (cnt == 2) {
			printf("          total sectors %4i", DPEEK(vtoc_sav, 0, 1));
			printf("           total sectors %4i", DPEEK(disk, VTOC_SEC360, 1));
		}
		else if (cnt == 4) {
			printf("       free sectors (<720)%4i", DPEEK(vtoc_sav, 0, 3));
			printf("      free sectors (<720)%4i", DPEEK(disk, VTOC_SEC360, 3));
		}
	}
	printf("\n");

	for (cnt = MAPBYT_360_10; cnt < 100; cnt++) {
		if (vtoc_sav[0][cnt] != disk[VTOC_SEC360][cnt]) {
			votc_same_fg = 0;
			printf("Byte#% 3i ", cnt);				//VTOC bytes in Sector 360
			printf("Sec% 4i>  ", (cnt - MAPBYT_360_10) * 8);
			printbinary(vtoc_sav[0][cnt]);
			printf(" ");
			printbinary(disk[VTOC_SEC360][cnt]);
			printf("\n");
		}
	}
	printf("\n");

	if (enhanced_density) //enhanced format
	{
		for (cnt = MAPBYT_1024_84; cnt < 122; cnt++) {
			if (vtoc_sav[1][cnt] != disk[VTOC_SEC1024][cnt]) {
				votc_same_fg = 0;
				printf("Byte#% 3i ", cnt);				//VTOC bytes in Sector 360
				printf("Sec% 4i>  ", (cnt - MAPBYT_360_10) * 8);
				printbinary(vtoc_sav[1][cnt]);
				printf(" ");
				printbinary(disk[VTOC_SEC1024][cnt]);
				printf(">\n");
			}
		}
		printf("\n Free enhanced sectors: %4i\n", DPEEK(disk, VTOC_SEC1024, 122));
		printf(" Total free sectors:%4i\n", DPEEK(disk, VTOC_SEC360, 3) + DPEEK(disk, VTOC_SEC1024, 122));
		printf(" Free bits in map:%4i", mapfrsecs);
		printf("\n");
	}
	if (votc_same_fg)
		printf("The old VTOC and the re-build are identical.\n\n");
	else {
		printf("The old VTOC and the re-build are NOT identical.\n");
		hit_Enter();
	}

}


char printbinary(unsigned char num) {
	// Determine the number of bits needed to represent the integer
	uint8_t numBits = 8;
	uint8_t bit_val;
	uint8_t nr_of_1s = 0;
	// Print binary string with leading zeros
	for (int i = numBits - 1; i >= 0; i--) {
		bit_val = (num >> i) & 1;
		nr_of_1s += bit_val;
		printf("%d", bit_val);
	}
	return (nr_of_1s);

}

#if CMP_DISK

void print_mismatches() {
	int sector, byte;
	int mismatch_count = 0;

	printf("\nMismatched sectors, bytes, and values:\n");

	for (sector = 0; sector < TOTAL_ED_SECS; sector++) {
		for (byte = 0; byte < DD_SEC_LEN; byte++) {
			if (disk[sector][byte] != ds_sav[sector][byte]) {
				printf("Sector: %4d, Byte: %3d, Value in disk: %2x, Value in ds_sav: %2x\n",
					sector, byte, disk[sector][byte], ds_sav[sector][byte]);
				mismatch_count++;
			}
		}
	}
	if (mismatch_count == 0) {
		printf("No mismatches found.\n");
	}
}
#endif


#if FIRST_SECS_VTOC

#define SECTOR_COUNT 4
#define BYTES_128_SECTOR 128
#define BYTES_PER_LINE 16  // Number of bytes to print per line

void print_sector(int sector) {
	printf("\nSector %d\n", sector);
	char char_buffer[BYTES_PER_LINE + 1]; // Buffer to store the character representation
	char_buffer[BYTES_PER_LINE] = '\0';   // Null-terminate the buffer

	for (int byte = 0; byte < BYTES_128_SECTOR; byte++) {
		// Print each byte as two-digit hexadecimal
		printf("%02x ", disk[sector][byte]);

		// Store the character or replace non-displayable characters with a space
		char_buffer[byte % BYTES_PER_LINE] = isprint(disk[sector][byte]) ? disk[sector][byte] : ' ';

		// After every BYTES_PER_LINE bytes, print the character representation
		if ((byte + 1) % BYTES_PER_LINE == 0) {
			printf("  %s\n", char_buffer); // Print the character representation after the hex values
		}
	}

	// Print the remaining characters for the last line if it's not complete
	int remaining_bytes = BYTES_128_SECTOR % BYTES_PER_LINE;
	if (remaining_bytes > 0) {
		// Add padding spaces if the last line is not full
		for (int i = remaining_bytes; i < BYTES_PER_LINE; i++) {
			printf("   "); // Print three spaces to align with the hex format
		}
		printf("  %s\n", char_buffer); // Print the last character representation
	}
}


void print4secs() {
	printf("\nThe first %d sectors are:\n", SECTOR_COUNT);
	for (int sector = 0; sector < SECTOR_COUNT; sector++) {
		print_sector(sector, 1);
	}
	print_sector(VTOC_SEC360);
	print_sector(VTOC_SEC1024);
	hit_Enter();
}
#endif

void show_file_secs() {
	uint8_t entrybeg;
	uint8_t filpos_in_dir = 0;
	bool endfound = 0;
	uint16_t sec, dirsec, fillen;
	char axlflnm[DIR_FLNM_LEN+1] = { 0 }; // +1 for null terminator
	int cnt = 0;
	uint8_t filno_in_sec = 0;

	for (dirsec = DIR_BEG_SEC; dirsec <= DIR_END_SEC; dirsec++)				// For each Directory sector
	{
		for (entrybeg = 0; entrybeg <= 112; entrybeg += DIR_ENTRY_SIZE)			// For each directory entry in sector
		{
			if (FLAG == 0)				// If byte 0 of entry == 0, end of file list
			{
				endfound++;
				break;
			}
			if (!(FLAG & 128))				// If byte 0 of entry not inverted, valid file present
			{
				fillen = DPEEK(disk, dirsec, entrybeg + 1);				//bytes 1,2: File length (not crucial)
				sec = DPEEK(disk, dirsec, entrybeg + 3);					//bytes 3,4: first data sector of file
				memcpy(axlflnm, &disk[dirsec][entrybeg + 5], DIR_FLNM_LEN);		//bytes 5 to 15: File name
				

				printf("%i. '%s'\n", filpos_in_dir, axlflnm);
				
				cnt = 0;
				do {
					printf(" %i", sec);
					if (disk[sec][sectlen - 1] != sectlen - 3) 
						printf(" (%i bytes)", disk[sec][sectlen - 1]); //partial sector

					filno_in_sec = disk[sec][sectlen - 3] >> 2;	//filenumber contained in this file sector

					if (filno_in_sec != filpos_in_dir)
						printf("(%i)", filno_in_sec);

					if (++cnt % 10 == 0)
						printf("\n");

					sector_filnr[sec] = (char)filpos_in_dir;
					
					sec = FILE_NEXTSEC;		//Last two bytes: next sector
				} while (sec); // zero indicates EOF

				printf("\n%i sectors, %i sectors indicated in directory \n\n", cnt, fillen);
			}
			filpos_in_dir++;
		}
		if (endfound)
			break;
	}
}

void replace_unprintable_characters(char *str) {
	while (*str) {
		if (!isprint((unsigned char)*str)) {
			*str = ' ';
		}
		str++;
	}
}