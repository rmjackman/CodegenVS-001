#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

// Function to remove leading numbers from a line
void remove_line_numbers(char *line) {
    char *ptr = line;
    while (isdigit((unsigned char)*ptr)) {
        ptr++;
    }
    while (isspace((unsigned char)*ptr)) {
        ptr++;
    }
    if (ptr != line) {
        memmove(line, ptr, strlen(ptr) + 1);
    }
}

void convert_mac65_to_atmasii(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    int in_macro = 0;

    while (fgets(line, sizeof(line), input) != NULL) {
        // Strip leading/trailing whitespace
        char *line_start = line;
        while (*line_start == ' ' || *line_start == '\t') line_start++;

        char *line_end = line_start + strlen(line_start) - 1;
        while (line_end > line_start && (*line_end == ' ' || *line_end == '\t' || *line_end == '\n' || *line_end == '\r')) {
            *line_end = '\0';
            line_end--;
        }

        // Remove line numbers
        remove_line_numbers(line_start);

        // Convert comments from ';' to '*'
        char *comment = strchr(line_start, ';');
        if (comment != NULL) {
            *comment = '*';
        }

        // Handle specific MAC65 directives and convert them to Atmas II equivalents
        if (strncmp(line_start, ".BYTE", 5) == 0) {
            memmove(line_start + 3, line_start + 5, strlen(line_start) - 4);
            strncpy(line_start, "DFB", 3);

        } else if (strncmp(line_start, ".WORD", 5) == 0) {
            memmove(line_start + 3, line_start + 5, strlen(line_start) - 4);
            strncpy(line_start, "DFW", 3);

        } else if (strncmp(line_start, ".ASC", 4) == 0) {
            // Assuming .ASC to ASC conversion is direct
            memmove(line_start + 3, line_start + 4, strlen(line_start) - 3);
            strncpy(line_start, "ASC", 3);

        } else if (strncmp(line_start, ".EQU", 4) == 0) {
            memmove(line_start + 3, line_start + 4, strlen(line_start) - 3);
            strncpy(line_start, "EQU", 3);

        } else if (strncmp(line_start, ".ORG", 4) == 0) {
            memmove(line_start + 3, line_start + 4, strlen(line_start) - 3);
            strncpy(line_start, "ORG", 3);

        } else if (strncmp(line_start, ".MACRO", 6) == 0) {
            in_macro = 1;
            memmove(line_start + 5, line_start + 6, strlen(line_start) - 5);
            strncpy(line_start, "MACRO", 5);

        } else if (strncmp(line_start, ".MEND", 5) == 0) {
            in_macro = 0;
            memmove(line_start + 4, line_start + 5, strlen(line_start) - 4);
            strncpy(line_start, "MEND", 4);

        } else if (in_macro && strchr(line_start, ':')) {
            // Append '@' to labels within macros
            strcat(line_start, "@");
        }

        // Write the converted line to the output file
        fprintf(output, "%s\n", line_start);
    }
}

int main() {
    FILE *input = fopen("source_mac65.asm", "r");
    if (input == NULL) {
        perror("Error opening input file");
        return EXIT_FAILURE;
    }

    FILE *output = fopen("source_atmasii.src", "w");
    if (output == NULL) {
        perror("Error opening output file");
        fclose(input);
        return EXIT_FAILURE;
    }

    convert_mac65_to_atmasii(input, output);

    fclose(input);
    fclose(output);

    printf("Conversion complete! Check 'source_atmasii.asm' for the result.\n");

    return EXIT_SUCCESS;
}