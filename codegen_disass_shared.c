
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

    printf(BYELLOW "\n\nRecorded warnings: %d\n\n", warning_count);

    for (int i = 0; i < warning_count; i++) {
        printf("%4d. %s:%d: %s\n",
            i + 1,
            warnings[i].filename,
            warnings[i].line,
            warnings[i].message);
    }

    number = 1;

    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;

    printf("Enter warning number to open in Notepad++ "
        "(0 or Q to finish) [%ld]: ", number);
    fflush(stdout);

    for (;;) {
        /* Save the prompt's starting position. */
        GetConsoleScreenBufferInfo(hOutput, &csbi);
        COORD prompt_pos = csbi.dwCursorPosition;

        char input[32] = "";
        int pos = 0;
        int ch;

        while ((ch = _getch()) != '\r' && ch != '\n') {
            if (ch == 'q' || ch == 'Q') {
                goto finished;
            }

            if (ch == '\b') {
                if (pos > 0) {
                    pos--;
                    printf("\b \b");
                    fflush(stdout);
                }
            }
            else if (ch >= '0' && ch <= '9' &&
                pos < (int)sizeof(input) - 1) {
                input[pos++] = (char)ch;
                putchar(ch);
                fflush(stdout);
            }
        }

        input[pos] = '\0';

        if (pos > 0) {
            char* endptr;
            long value = strtol(input, &endptr, 10);

            if (*endptr != '\0') {
                continue;
            }

            number = value;
        }

        if (number == 0) {
            break;
        }

        if (number < 1 || number > warning_count) {
            printf("\nEnter a number between 1 and %d, or 0 to finish.\n",
                warning_count);
            number = 1;
        }
        else {
            warning_entry* warning = &warnings[number - 1];
            open_notepadpp_line(warning->filename, warning->line);

            number++;
            if (number > warning_count) {
                number = 0;
            }
        }

        /* Clear the previous prompt line using the Windows API. */
        GetConsoleScreenBufferInfo(hOutput, &csbi);

        COORD clear_pos = { 0, csbi.dwCursorPosition.Y };
        DWORD line_length = csbi.dwSize.X;

        FillConsoleOutputCharacterA(
            hOutput, ' ', line_length, clear_pos, &written);

        FillConsoleOutputAttribute(
            hOutput, csbi.wAttributes, line_length, clear_pos, &written);

        SetConsoleCursorPosition(hOutput, clear_pos);

        printf("Enter warning number to open in Notepad++ "
            "(0 or Q to finish) [%ld]: ", number);
        fflush(stdout);
    }

finished:
    printf(RESET); ;
}


int enable_ansi_colors(void) {
    HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;

    if (hOutput == NULL || hOutput == INVALID_HANDLE_VALUE)
        return 0;

    if (!GetConsoleMode(hOutput, &mode))
        return 0;

    if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        return 1;

    return SetConsoleMode(
        hOutput, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}