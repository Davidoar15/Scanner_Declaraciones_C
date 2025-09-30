#include "Scanner.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Estado: modo cadena (inputStr != NULL) o stdin
static const char *inputStr = NULL;
static size_t inputPos = 0;

// pushback interno para stdin
static int hasPushedChar = 0;
static int pushedChar = 0;

// KEYWORDS 
static const char *keywords[] = {
    "void","char","short","int","long","float","double","signed","unsigned",
    "struct","union","enum","typedef","_Bool","_Complex","_Atomic","_Noreturn",
    "const","volatile","static","extern","register","inline","restrict",
    "for","while","do","if","else","return", NULL
};

static int isKeyword(const char *s) {
    for (int i = 0; keywords[i]; ++i) {
        if (strcmp(s, keywords[i]) == 0) return 1;
    }
    return 0;
}

// Inicialización 
void scannerInitFromString(const char *s) {
    if (s) {
        inputStr = s;
        inputPos = 0;
    } else {
        inputStr = NULL;
        inputPos = 0;
    }
    hasPushedChar = 0;
    pushedChar = 0;
}

// Lectura: peek/get
static inline char peekChar(void) {
    if (inputStr) {
        return (inputStr[inputPos]) ? inputStr[inputPos] : '\0';
    }
    if (hasPushedChar) return (char)pushedChar;
    int c = getchar();
    if (c == EOF) return '\0';
    hasPushedChar = 1;
    pushedChar = c;
    return (char)c;
}

static inline char getChar(void) {
    if (inputStr) {
        char c = inputStr[inputPos];
        if (c != '\0') inputPos++;
        return c ? c : '\0';
    }
    if (hasPushedChar) {
        hasPushedChar = 0;
        return (char)pushedChar;
    }
    int c = getchar();
    return (c == EOF) ? '\0' : (char)c;
}

static char *xStrdup(const char *s) {
    const char *src = s ? s : "";
    size_t n = strlen(src) + 1;
    char *dup = (char *)malloc(n);
    if (!dup) {
        fprintf(stderr, "Error: memoria insuficiente en xStrdup()\n");
        exit(EXIT_FAILURE);
    }
    memcpy(dup, src, n);
    return dup;
}

// makeToken 
static Token makeToken(TipoToken tipo, const char *lexema) {
    Token token;
    token.tipo = tipo;
    token.lexema = xStrdup(lexema ? lexema : "");
    token.valor.int_value = 0;
    return token;
}

void tokenFree(Token *token) {
    if (!token) return;
    if (token->lexema) {
        free(token->lexema);
        token->lexema = NULL;
    }
}

void printToken(const Token *token) {
    if (!token) return;
    const char *name = "UNKNOWN";
    switch (token->tipo) {
        case TOKEN_ERROR: name = "ERROR"; break;
        case TOKEN_END: name = "END"; break;
        case TOKEN_IDENT: name = "IDENT"; break;
        case TOKEN_KEYWORD: name = "KEYWORD"; break;
        case TOKEN_INT_DEC: name = "INT_DEC"; break;
        case TOKEN_INT_HEX: name = "INT_HEX"; break;
        case TOKEN_INT_OCTAL: name = "INT_OCT"; break;
        case TOKEN_FLOAT: name = "FLOAT"; break;
        case TOKEN_CHAR_LITERAL: name = "CHAR"; break;
        case TOKEN_STRING_LITERAL: name = "STRING"; break;
        case TOKEN_SEMICOLON: name = "SEMICOLON"; break;
        case TOKEN_COMMA: name = "COMMA"; break;
        case TOKEN_LPAREN: name = "LPAREN"; break;
        case TOKEN_RPAREN: name = "RPAREN"; break;
        case TOKEN_LBRACKET: name = "LBRACKET"; break;
        case TOKEN_RBRACKET: name = "RBRACKET"; break;
        case TOKEN_ASTERISK: name = "ASTERISK"; break;
        case TOKEN_ASSIGN: name = "ASSIGN"; break;
        case TOKEN_OTHER: name = "OTHER"; break;
    }
    if (token->tipo == TOKEN_INT_DEC || token->tipo == TOKEN_INT_HEX || token->tipo == TOKEN_INT_OCTAL)
        printf("%s \"%s\" value=%ld\n", name, token->lexema, token->valor.int_value);
    else if (token->tipo == TOKEN_FLOAT)
        printf("%s \"%s\" value=%g\n", name, token->lexema, token->valor.float_value);
    else
        printf("%s \"%s\"\n", name, token->lexema);
}

// Manejo de escapes en lexema
static int appendEscapeToLexeme(char *lex, int *len, size_t cap) {
    char c = getChar();
    if (c == '\0') return 0;
    if (*len + 2 >= (int)cap) return 0;
    lex[(*len)++] = '\\';
    lex[(*len)++] = c;
    lex[*len] = '\0';
    if (c == 'x' || c == 'X') {
        int cnt = 0;
        while (cnt < 2 && isxdigit((unsigned char)peekChar())) {
            if (*len + 1 >= (int)cap) return 0;
            lex[(*len)++] = getChar();
            lex[*len] = '\0';
            cnt++;
        }
        if (cnt == 0) return 0;
    }
    return 1;
}

// getNextToken: funcion scanner
Token getNextToken(void) {
    char lex[1024];
    int llen = 0;
    char c;

    // saltar espacios 
    while (1) {
        c = getChar();
        if (c == '\0') return makeToken(TOKEN_END, "<EOF>");
        if (!isspace((unsigned char)c)) break;
    }

    // IDENT / KEYWORD 
    if (isalpha((unsigned char)c) || c == '_') {
        llen = 0;
        lex[llen++] = c;
        while (isalnum((unsigned char)peekChar()) || peekChar() == '_') {
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
            else getChar();
        }
        lex[llen] = '\0';
        if (isKeyword(lex)) return makeToken(TOKEN_KEYWORD, lex);
        else return makeToken(TOKEN_IDENT, lex);
    }

    // Numeros (dec/hex/oct) y float 
    if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)peekChar()))) {
        llen = 0;
        int isFloat = 0;
        if (c == '0') {
            lex[llen++] = c;
            if (peekChar() == 'x' || peekChar() == 'X') {
                lex[llen++] = getChar();
                int any = 0;
                while (isxdigit((unsigned char)peekChar())) {
                    if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                    else getChar();
                    any = 1;
                }
                lex[llen] = '\0';
                if (!any) return makeToken(TOKEN_ERROR, "Malformed hex");
                Token token = makeToken(TOKEN_INT_HEX, lex);
                token.valor.int_value = strtol(token.lexema, NULL, 0);
                return token;
            } else if (peekChar() >= '0' && peekChar() <= '7') {
                while (peekChar() >= '0' && peekChar() <= '7') {
                    if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                    else getChar();
                }
                lex[llen] = '\0';
                Token token = makeToken(TOKEN_INT_OCTAL, lex);
                token.valor.int_value = strtol(token.lexema, NULL, 8);
                return token;
            } else {
                lex[llen] = '\0';
                Token token = makeToken(TOKEN_INT_DEC, lex);
                token.valor.int_value = 0;
                return token;
            }
        }

        if (c == '.') {
            isFloat = 1;
            lex[llen++] = c;
            while (isdigit((unsigned char)peekChar())) {
                if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                else getChar();
            }
            if (peekChar() == 'e' || peekChar() == 'E') {
                lex[llen++] = getChar();
                if (peekChar() == '+' || peekChar() == '-') lex[llen++] = getChar();
                if (!isdigit((unsigned char)peekChar())) return makeToken(TOKEN_ERROR, "Malformed float exponent");
                while (isdigit((unsigned char)peekChar())) {
                    if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                    else getChar();
                }
            }
            lex[llen] = '\0';
            Token token = makeToken(TOKEN_FLOAT, lex);
            token.valor.float_value = strtod(token.lexema, NULL);
            return token;
        }

        lex[llen++] = c;
        while (isdigit((unsigned char)peekChar())) {
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
            else getChar();
        }
        if (peekChar() == '.') {
            isFloat = 1;
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
            else getChar();
            while (isdigit((unsigned char)peekChar())) {
                if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                else getChar();
            }
        }
        if (peekChar() == 'e' || peekChar() == 'E') {
            isFloat = 1;
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
            else getChar();
            if (peekChar() == '+' || peekChar() == '-') {
                if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                else getChar();
            }
            if (!isdigit((unsigned char)peekChar())) return makeToken(TOKEN_ERROR, "Malformed exponent");
            while (isdigit((unsigned char)peekChar())) {
                if (llen < (int)sizeof(lex) - 1) lex[llen++] = getChar();
                else getChar();
            }
        }
        lex[llen] = '\0';
        if (isFloat) {
            Token token = makeToken(TOKEN_FLOAT, lex);
            token.valor.float_value = strtod(token.lexema, NULL);
            return token;
        } else {
            Token token = makeToken(TOKEN_INT_DEC, lex);
            token.valor.int_value = strtol(token.lexema, NULL, 10);
            return token;
        }
    }

    // String literal 
    if (c == '"') {
        llen = 0;
        if (llen < (int)sizeof(lex) - 1) lex[llen++] = '"';
        while (1) {
            char d = getChar();
            if (d == '\0') return makeToken(TOKEN_ERROR, "Unterminated string literal");
            if (d == '"') {
                if (llen < (int)sizeof(lex) - 1) lex[llen++] = '"';
                lex[llen] = '\0';
                return makeToken(TOKEN_STRING_LITERAL, lex);
            }
            if (d == '\\') {
                if (!appendEscapeToLexeme(lex, &llen, sizeof(lex))) return makeToken(TOKEN_ERROR, "Invalid escape");
            } else {
                if (llen < (int)sizeof(lex) - 1) lex[llen++] = d;
            }
        }
    }

    // Char literal 
    if (c == '\'') {
        llen = 0;
        if (llen < (int)sizeof(lex) - 1) lex[llen++] = '\'';
        char d = getChar();
        if (d == '\0') return makeToken(TOKEN_ERROR, "Unterminated char literal");
        if (d == '\\') {
            if (!appendEscapeToLexeme(lex, &llen, sizeof(lex))) return makeToken(TOKEN_ERROR, "Invalid escape in char");
            char closing = getChar();
            if (closing != '\'') return makeToken(TOKEN_ERROR, "Unterminated char literal");
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = '\'';
            lex[llen] = '\0';
            return makeToken(TOKEN_CHAR_LITERAL, lex);
        } else {
            char closing = getChar();
            if (closing != '\'') return makeToken(TOKEN_ERROR, "Unterminated char literal");
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = d;
            if (llen < (int)sizeof(lex) - 1) lex[llen++] = '\'';
            lex[llen] = '\0';
            return makeToken(TOKEN_CHAR_LITERAL, lex);
        }
    }

    // símbolos simples 
    switch (c) {
        case ';': return makeToken(TOKEN_SEMICOLON, ";");
        case ',': return makeToken(TOKEN_COMMA, ",");
        case '(': return makeToken(TOKEN_LPAREN, "(");
        case ')': return makeToken(TOKEN_RPAREN, ")");
        case '[': return makeToken(TOKEN_LBRACKET, "[");
        case ']': return makeToken(TOKEN_RBRACKET, "]");
        case '*': return makeToken(TOKEN_ASTERISK, "*");
        case '=': return makeToken(TOKEN_ASSIGN, "=");
        default: {
            char s[4] = {0};
            s[0] = c;
            s[1] = '\0';
            return makeToken(TOKEN_OTHER, s);
        }
    }
}