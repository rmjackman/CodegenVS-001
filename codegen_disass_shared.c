
/*
 * codegen_disass_shared.c
 *
 * Shared warning management for Codegen.c and disass.c.
 */

#include "cdgn_global.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


 /* ------------------------------------------------------------
    Warning storage
    ------------------------------------------------------------ */

static warning_entry warnings[MAX_WARNINGS];
static int warning_count = 0;


/* ------------------------------------------------------------
   Initialise warning storage
   Call once at the beginning of the complete CODEGEN operation.
   ------------------------------------------------------------ */

void warnings_init(void) {
    warning_count = 0;
    memset(warnings, 0, sizeof(warnings));
}


/* ------------------------------------------------------------
   Record a warning
   ------------------------------------------------------------ */

void record_warning(const char* filename, int line, const char* message) {
    if (warning_count >= MAX_WARNINGS) {
        fprintf(stderr,
            "Warning list full: maximum of %d warnings reached.\n",
            MAX_WARNINGS);
        return;
    }

    warning_entry* warning = &warnings[warning_count];

    strcpy_s(warning->filename, sizeof(warning->filename),
        filename ? filename : "");

    warning->line = line;

    strcpy_s(warning->message, sizeof(warning->message),
        message ? message : "");

    warning_count++;
}


/* ------------------------------------------------------------
   Open a source file at a specified line in Notepad++
   ------------------------------------------------------------ */

void open_notepadpp_line(const char* filename, int line) {
    char notepad_path[MAX_PATH];
    char command_line[MAX_PATH * 2];
    STARTUPINFOA startup_info = { 0 };
    PROCESS_INFORMATION process_info = { 0 };

    startup_info.cb = sizeof(startup_info);

    /* Try the standard 64-bit Notepad++ installation first. */
    strcpy_s(notepad_path, sizeof(notepad_path),
        "C:\\Program Files\\Notepad++\\notepad++.exe");

    if (GetFileAttributesA(notepad_path) == INVALID_FILE_ATTRIBUTES) {
        /* Try the standard 32-bit installation location. */
        strcpy_s(notepad_path, sizeof(notepad_path),
            "C:\\Program Files (x86)\\Notepad++\\notepad++.exe");
    }

    if (GetFileAttributesA(notepad_path) == INVALID_FILE_ATTRIBUTES) {
        printf("Cannot find Notepad++.\n");
        return;
    }

    if (sprintf_s(command_line, sizeof(command_line),
        "\"%s\" -n%d \"%s\"",
        notepad_path, line, filename) < 0) {
        printf("Cannot construct the Notepad++ command line.\n");
        return;
    }

    if (!CreateProcessA(
        notepad_path,
        command_line,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &startup_info,
        &process_info)) {

        printf("Unable to start Notepad++ (Windows error %lu).\n",
            GetLastError());
        return;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
}


/* ------------------------------------------------------------
   Display all recorded warnings and allow the user to open one
   in Notepad++ by entering its warning number.
   Enter 0 or Q to finish.
   ------------------------------------------------------------ */

void review_warnings(void) {
    char input[64];
    char* endptr;
    long number;

    if (warning_count == 0) {
        return;
    }

    printf("\n\nRecorded warnings: %d\n\n", warning_count);

    for (int i = 0; i < warning_count; i++) {
        printf("%4d. %s:%d: %s\n",
            i + 1,
            warnings[i].filename,
            warnings[i].line,
            warnings[i].message);
    }

    for (;;) {
        printf("\nEnter warning number to open in Notepad++ "
            "(0 or Q to finish): ");

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        /* Allow the user to quit with Q. */
        if (tolower((unsigned char)input[0]) == 'q') {
            break;
        }

        number = strtol(input, &endptr, 10);

        if (endptr == input) {
            printf("Please enter a warning number, 0, or Q.\n");
            continue;
        }

        /* Reject extra non-whitespace characters. */
        while (isspace((unsigned char)*endptr)) {
            endptr++;
        }

        if (*endptr != '\0') {
            printf("Invalid input.\n");
            continue;
        }

        if (number == 0) {
            break;
        }

        if (number < 1 || number > warning_count) {
            printf("Enter a number between 1 and %d, or 0 to finish.\n",
                warning_count);
            continue;
        }

        warning_entry* warning = &warnings[number - 1];

        open_notepadpp_line(warning->filename, warning->line);
    }
}