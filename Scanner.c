#include "Scanner.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ---------- Estado (cadena) ---------- 
static const char *scannerInput = NULL;
static size_t scannerPosition = 0;

// Lista de keywords
static const char *keywords[] = {
    "void","char","short","int","long","float","double","signed","unsigned",
    "struct","union","enum","typedef","_Bool","_Complex","_Atomic","_Noreturn",
    "const","volatile","static","extern","register","inline","restrict",
    "for","while","do","if","else","return", NULL
};
static int isKeyword(const char *s) {
    for (int i = 0; keywords[i]; ++i)
        if (strcmp(s, keywords[i]) == 0) return 1;
    return 0;
}

// Inicializar scanner desde una cadena
void ScannerInitFromString(const char *s) {
    scannerInput = s ? s : "";
    scannerPosition = 0;
}

// para la lectura
static char scannerPeek(void) {
    if (!scannerInput) return '\0';
    return scannerInput[scannerPosition];
}
static char scannerGet(void) {
    char c = scannerPeek();
    if (c != '\0') scannerPosition++;
    return c;
}
static void scannerUnget(void) {
    if (scannerPosition == 0) return;
    scannerPosition--;
}

/* strdup seguro */
static char *strdupSafe(const char *s) {
    if (!s) return NULL;
    char *r = malloc(strlen(s) + 1);
    if (r) strcpy(r, s);
    return r;
}

// Crear token
static Token makeToken(TipoToken tipo, const char *lexema) {
    Token token;
    token.tipo = tipo;
    token.lexema = strdupSafe(lexema ? lexema : "");
    token.valor.int_value = 0;
    return token;
}

void TokenFree(Token *token) {
    if (!token) return;
    if (token->lexema) { 
        free(token->lexema); 
        token->lexema = NULL; 
    }
}

// Impresión simple
void PrintToken(const Token *token) {
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

/* Añade escape textual al buffer (dado que guardamos el lexema textual).
   Devuelve 1 si OK, 0 en error. */
static int appendEscapeToBuf(char *buf, int *bi, size_t buf_sz) {
    char c = scannerGet();
    if (c == '\0') return 0;
    if (*bi + 2 >= (int)buf_sz) return 0;
    buf[(*bi)++] = '\\';
    buf[(*bi)++] = c;
    buf[*bi] = '\0';
    if (c == 'x' || c == 'X') {
        int cnt = 0;
        while (cnt < 2 && isxdigit((unsigned char)scannerPeek())) {
            if (*bi + 1 >= (int)buf_sz) return 0;
            buf[(*bi)++] = scannerGet();
            buf[*bi] = '\0';
            cnt++;
        }
        if (cnt == 0) return 0;
    }
    return 1;
}

// GetNextToken
Token GetNextToken(void) {
    char buf[1024];
    int bi = 0;
    char c;

    // Saltar espacios 
    while (1) {
        c = scannerGet();
        if (c == '\0') return makeToken(TOKEN_END, "<EOF>");
        if (!isspace((unsigned char)c)) break;
    }

    // IDENT / KEYWORD 
    if (isalpha((unsigned char)c) || c == '_') {
        bi = 0;
        buf[bi++] = c;
        while (isalnum((unsigned char)scannerPeek()) || scannerPeek() == '_') {
            if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
            else scannerGet();
        }
        buf[bi] = '\0';
        if (isKeyword(buf)) return makeToken(TOKEN_KEYWORD, buf);
        else return makeToken(TOKEN_IDENT, buf);
    }

    // Números (dec/hex/oct) y floats
    if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)scannerPeek()))) {
        bi = 0;
        int is_float = 0;
        if (c == '0') {
            buf[bi++] = c;
            if (scannerPeek() == 'x' || scannerPeek() == 'X') {
                buf[bi++] = scannerGet();
                int any = 0;
                while (isxdigit((unsigned char)scannerPeek())) {
                    if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                    else scannerGet();
                    any = 1;
                }
                buf[bi] = '\0';
                if (!any) return makeToken(TOKEN_ERROR, "Malformed hex");
                Token t = makeToken(TOKEN_INT_HEX, buf);
                t.valor.int_value = strtol(t.lexema, NULL, 0);
                return t;
            } else if (scannerPeek() >= '0' && scannerPeek() <= '7') {
                while (scannerPeek() >= '0' && scannerPeek() <= '7') {
                    if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                    else scannerGet();
                }
                buf[bi] = '\0';
                Token t = makeToken(TOKEN_INT_OCTAL, buf);
                t.valor.int_value = strtol(t.lexema, NULL, 8);
                return t;
            } else {
                buf[bi] = '\0';
                Token t = makeToken(TOKEN_INT_DEC, buf);
                t.valor.int_value = 0;
                return t;
            }
        }

        if (c == '.') {
            is_float = 1;
            buf[bi++] = c;
            while (isdigit((unsigned char)scannerPeek())) {
                if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                else scannerGet();
            }
            if (scannerPeek() == 'e' || scannerPeek() == 'E') {
                buf[bi++] = scannerGet();
                if (scannerPeek() == '+' || scannerPeek() == '-') buf[bi++] = scannerGet();
                if (!isdigit((unsigned char)scannerPeek())) return makeToken(TOKEN_ERROR, "Malformed float exponent");
                while (isdigit((unsigned char)scannerPeek())) {
                    if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                    else scannerGet();
                }
            }
            buf[bi] = '\0';
            Token t = makeToken(TOKEN_FLOAT, buf);
            t.valor.float_value = strtod(t.lexema, NULL);
            return t;
        }

        // empieza con 1..9 
        buf[bi++] = c;
        while (isdigit((unsigned char)scannerPeek())) {
            if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
            else scannerGet();
        }
        if (scannerPeek() == '.') {
            is_float = 1;
            if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet(); else scannerGet();
            while (isdigit((unsigned char)scannerPeek())) {
                if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                else scannerGet();
            }
        }
        if (scannerPeek() == 'e' || scannerPeek() == 'E') {
            is_float = 1;
            if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet(); else scannerGet();
            if (scannerPeek() == '+' || scannerPeek() == '-') {
                if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                else scannerGet();
            }
            if (!isdigit((unsigned char)scannerPeek())) return makeToken(TOKEN_ERROR, "Malformed exponent");
            while (isdigit((unsigned char)scannerPeek())) {
                if (bi < (int)sizeof(buf)-1) buf[bi++] = scannerGet();
                else scannerGet();
            }
        }
        buf[bi] = '\0';
        if (is_float) {
            Token t = makeToken(TOKEN_FLOAT, buf);
            t.valor.float_value = strtod(t.lexema, NULL);
            return t;
        } else {
            Token t = makeToken(TOKEN_INT_DEC, buf);
            t.valor.int_value = strtol(t.lexema, NULL, 10);
            return t;
        }
    }

    // STRING 
    if (c == '"') {
        bi = 0;
        if (bi < (int)sizeof(buf)-1) buf[bi++] = '"';
        while (1) {
            char d = scannerGet();
            if (d == '\0') return makeToken(TOKEN_ERROR, "Unterminated string literal");
            if (d == '"') {
                if (bi < (int)sizeof(buf)-1) buf[bi++] = '"';
                buf[bi] = '\0';
                return makeToken(TOKEN_STRING_LITERAL, buf);
            }
            if (d == '\\') {
                if (!appendEscapeToBuf(buf, &bi, sizeof(buf))) return makeToken(TOKEN_ERROR, "Invalid escape");
            } else {
                if (bi < (int)sizeof(buf)-1) buf[bi++] = d;
            }
        }
    }

    // CHAR 
    if (c == '\'') {
        bi = 0;
        if (bi < (int)sizeof(buf)-1) buf[bi++] = '\'';
        char d = scannerGet();
        if (d == '\0') return makeToken(TOKEN_ERROR, "Unterminated char literal");
        if (d == '\\') {
            if (!appendEscapeToBuf(buf, &bi, sizeof(buf))) return makeToken(TOKEN_ERROR, "Invalid escape in char");
            char closing = scannerGet();
            if (closing != '\'') return makeToken(TOKEN_ERROR, "Unterminated char literal");
            if (bi < (int)sizeof(buf)-1) buf[bi++] = '\'';
            buf[bi] = '\0';
            return makeToken(TOKEN_CHAR_LITERAL, buf);
        } else {
            char closing = scannerGet();
            if (closing != '\'') return makeToken(TOKEN_ERROR, "Unterminated char literal");
            if (bi < (int)sizeof(buf)-1) buf[bi++] = d;
            if (bi < (int)sizeof(buf)-1) buf[bi++] = '\'';
            buf[bi] = '\0';
            return makeToken(TOKEN_CHAR_LITERAL, buf);
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