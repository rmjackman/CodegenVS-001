#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Assume these functions and variables are defined elsewhere
int symtoi1();   // Converts a symbol to its integer value
void error(const char* msg);  // Error handling function
char* txtptr;    // Pointer to the current position in the input string
int NOTFOUND;    // Special value indicating a symbol was not found
int pc;          // Some global variable, probably representing a program counter or similar
char* operand;   // Current operand
char* opcode;    // Current opcode

// Function to return precedence of operators
int prec(char op) {
    switch (op) {
    case '+':
    case '-':
        return 1;
    case '*':
    case '/':
        return 2;
    default:
        return 0;
    }
}

unsigned int expresstoi() {
    int i, j, k, tos, a, b, result = 0;
    char notfound = 0;
    char* tptrsv, * ptr, * ptr_next;
    char infix[50], postfix[50];
    int symval_list[30];
    char symval_cnt = 0; // Changed from '\0' to 0 for clarity
    size_t length;
    char temp;
    char stack[30];
    int intstack[30];

    tptrsv = txtptr;
    ptr = infix;

    // Tokenize the input expression
    if (*operand == '(') *ptr++ = '(';

    if (isspace(*(txtptr + 1))) {
        if ((*txtptr == '-') || (*txtptr == '+')) {
            return symtoi1();  // "- " is DO, "+ " is CONT
        }
    }

    do {
        switch (*txtptr) {
        case '+':
        case '-':
            if ((tptrsv == txtptr) || (*(txtptr - 1) == '(')) {
                symval_list[symval_cnt] = 0;
                *ptr++ = symval_cnt++ + 'A';
            }
            // Fall through to default behavior
        case '/':
        case '*':
        case ')':
        case '(':
            *ptr++ = *txtptr++;
            break;
        default:
            if ((symval_list[symval_cnt] = symtoi1()) == NOTFOUND) notfound = 1;
            *ptr++ = symval_cnt++ + 'A';
        }
    } while (!(isspace(*txtptr) || (*txtptr == ',') || (*txtptr == ':')));

    if (*txtptr == ',') {
        if ((*(txtptr + 1) == 'X' || *(txtptr + 1) == 'x') && *(txtptr + 2) == ')' && isspace(*(txtptr + 3))) {
            *ptr++ = ')';
        }
    }

    *ptr = '\0';

    // Interpret '*' in the infix expression
    ptr = strchr(infix, '*');
    while (ptr) {
        ptr_next = ptr + 1;
        if (!isalpha(*ptr_next) && (*ptr_next != '(')) {
            symval_list[symval_cnt] = pc;
            *ptr = symval_cnt++ + 'A';
        }
        ptr = strchr(ptr_next, '*');
    }

    // Convert infix to postfix
    tos = j = 0;
    length = strlen(infix);

    for (i = 0; i < length; i++) {
        if (!strchr("+-*/()", infix[i])) {
            postfix[j++] = infix[i];  // Numbers are added to the postfix string
        }
        else {
            if (tos == 0) {
                stack[tos++] = infix[i];
            }
            else {
                if (infix[i] != ')' && infix[i] != '(') {
                    while (tos > 0 && prec(infix[i]) <= prec(stack[tos - 1])) {
                        postfix[j++] = stack[--tos];
                    }
                    stack[tos++] = infix[i];
                }
                else if (infix[i] == '(') {
                    stack[tos++] = infix[i];
                }
                else if (infix[i] == ')') {
                    while (tos > 0 && stack[tos - 1] != '(') {
                        postfix[j++] = stack[--tos];
                    }
                    if (tos == 0) error("Missing opening parenthesis");
                    tos--; // Pop the '(' from stack
                }
            }
        }
    }

    while (tos > 0) {
        if (stack[tos - 1] == '(') error("Missing closing parenthesis");
        postfix[j++] = stack[--tos];
    }
    postfix[j] = '\0';

    // Convert postfix to integer result
    tos = 0;
    ptr = postfix;

    while (*ptr) {
        if (isalpha(*ptr)) {
            intstack[tos++] = symval_list[*ptr - 'A'];
        }
        else {
            b = intstack[--tos];
            a = intstack[--tos];
            switch (*ptr) {
            case '+': intstack[tos++] = a + b; break;
            case '-': intstack[tos++] = a - b; break;
            case '*': intstack[tos++] = a * b; break;
            case '/': intstack[tos++] = a / b; break;
            default:
                error("Illegal character in postfix expression");
                break;
            }
        }
        ptr++;
    }

    result = intstack[--tos];
    if (tos) {
        printf("\n\noperand: %s   infix: %s   postfix: %s", operand, infix, postfix);
        error("Illegal syntax. Can't resolve expression");
    }

    if (notfound) result = NOTFOUND;
    return result;
}