#ifndef SCANNER_H
#define SCANNER_H

#include <stdio.h>

// Tipos de token (sin línea/columna)
typedef enum {
    TOKEN_ERROR,
    TOKEN_END,
    TOKEN_IDENT,
    TOKEN_KEYWORD,
    TOKEN_INT_DEC,
    TOKEN_INT_HEX,
    TOKEN_INT_OCTAL,
    TOKEN_FLOAT,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_ASTERISK,
    TOKEN_ASSIGN,
    TOKEN_OTHER
} TipoToken;

typedef union {
    long int_value;
    double float_value;
} ValorToken;

typedef struct {
    char *lexema;      
    TipoToken tipo;
    ValorToken valor; 
} Token;

void scannerInitFromString(const char *s);
// devuelve el siguiente token; llamar TokenFree(&t) después de usarlo
Token getNextToken(void);
void tokenFree(Token *t);

// imprime token en stdout 
void printToken(const Token *t);

#endif /* SCANNER_H */