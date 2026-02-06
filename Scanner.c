#include "Scanner.h"

#define MAX_LEXEMA 256
#define POOL_SLOTS 128

static char pool_lexemas[POOL_SLOTS][MAX_LEXEMA];
static char *pool_ptrs[POOL_SLOTS];
static int pool_siguiente = 0;

// Copia lexema al pool y devuelve indice usado 
static int poolGuardarLexema(const char *string) {
    int index = pool_siguiente;
    pool_siguiente = (pool_siguiente + 1) % POOL_SLOTS;

    // Truncamiento seguro
    strncpy(pool_lexemas[index], string ? string : "", MAX_LEXEMA - 1);
    pool_lexemas[index][MAX_LEXEMA - 1] = '\0';
    pool_ptrs[index] = pool_lexemas[index];

    return index;
}

// Entrada. Si token 'lexema' se guardo en index, se devuelve puntero y char**
static const char *poolLexemaPuntero(int index) {
    return pool_ptrs[index];
}
static char **poolLexemaPunteroDoble(int index) {
    return &pool_ptrs[index];
}

// Entrada. modo cadena/string o stdin
static const char *cadenaEntrada = NULL;
static size_t posEntrada = 0;

// Keywords
static const char *keywords[] = {
    "void", "char", "short", "int", "long", "float", "double",
    "signed", "unsigned", "struct", "union", "enum", "typedef",
    "_Bool", "_Complex", "_Atomic", "_Noreturn", "const", 
    "volatile", "static", "extern", "register", "inline",
    "restrict", "for", "while", "do", "if", "else", "return", NULL
};

static int esKeyword(const char *string) {
    for (int i = 0; keywords[i]; ++i) {
        if (strcmp(string, keywords[i]) == 0) return 1;
    }
    return 0;
}

// Inicializacion 
void iniciarScannerDesdeCadena(const char *string) {
    if (string) {
        cadenaEntrada = string;
        posEntrada = 0;
    } else {
        cadenaEntrada = NULL;
        posEntrada = 0;
    }
}

// Lectura: peek / get 
static inline char verProximoCaracter(void) {
    if (cadenaEntrada) {
        return (cadenaEntrada[posEntrada]) ? cadenaEntrada[posEntrada] : '\0';
    }

    int c = getchar();
    if (c == EOF) return '\0';
    ungetc(c, stdin);
    return c;
}

static inline char obtenerCaracter(void) {
    if (cadenaEntrada) {
        char c = cadenaEntrada[posEntrada];
        if (c != '\0') posEntrada++;
        return c ? c : '\0';
    }

    int c = getchar();
    return (c == EOF) ? '\0' : c;
}

// Manejo de escapes para literales
static int agregarEscapeAlLexema(char *lex, size_t *len, size_t cap) {
    char c = obtenerCaracter();

    if (c == '\0') return 0;
    if (*len + 2 >= cap) return 0;

    lex[(*len)++] = '\\';
    lex[(*len)++] = c;
    lex[*len] = '\0';

    if (c == 'x' || c == 'X') {
        int cnt = 0;
        while (cnt < 2 && isxdigit((unsigned char)verProximoCaracter())) {
            if (*len + 1 >= cap) return 0;

            lex[(*len)++] = obtenerCaracter();
            lex[*len] = '\0';
            cnt++;
        }

        if (cnt == 0) return 0;
    }

    return 1;
}

// Crear token. Llena 'lexema' en pool y prepara el token.valor.ident como char** apuntando al puntero del pool
static Token crearTokenConLexema(TipoToken tipo, const char *lexema) {
    Token token;
    token.tipo = tipo;
    int index = poolGuardarLexema(lexema);
    token.lexema = poolLexemaPuntero(index);
    token.valor.ident = poolLexemaPunteroDoble(index);
    token.valor.int_value = 0;
    token.valor.float_value = 0.0;

    return token;
}

Token scanIdentOKeyword(char c){
    char lex[MAX_LEXEMA];
    size_t len = 0;
    lex[len++] = c;

    while (isalnum((unsigned char)verProximoCaracter()) || verProximoCaracter() == '_') {
        if (len < sizeof(lex) - 1)
            lex[len++] = obtenerCaracter();
        else
            obtenerCaracter();
    }
    lex[len] = '\0';

    if (esKeyword(lex))
        return crearTokenConLexema(TOKEN_KEYWORD, lex);
    return crearTokenConLexema(TOKEN_IDENT, lex);
}

Token scanHexNumber(char *lex, size_t len) {
    lex[len++] = obtenerCaracter();
    int any = 0;

    while (isxdigit((unsigned char)verProximoCaracter())){
        if(len < sizeof(lex) - 1)
            lex[len++] = obtenerCaracter();
        else
            obtenerCaracter();
        any = 1;
    }
    
    lex[len] = '\0';

    if (!any)
        return crearTokenConLexema(TOKEN_ERROR, "Hexadecimal mal formado");
    
    Token token = crearTokenConLexema(TOKEN_INT_HEX, lex);
    token.valor.int_value = strtol(token.lexema, NULL, 0);
    return token;
}

Token scanOctalNumber(char *lex, size_t len){
    while (verProximoCaracter() >= '0' && verProximoCaracter() <= '7'){
        if (len < sizeof(lex) - 1)
            lex[len++] = obtenerCaracter();
        else
            obtenerCaracter();
    }
    //Se omite toda la parte del chequeo por si no es ./e/E ya que se hace en scanNumber
    lex[len] = '\0';
    Token token = crearTokenConLexema(TOKEN_INT_OCTAL, lex);
    token.valor.int_value = strtol(token.lexema, NULL, 8);
    return token;

}

Token scanFloatODecimal(char *lex, size_t len)
{
    int esFloat = 0;

    if (lex[0] == '.') 
        esFloat = 1;

    while (isdigit((unsigned char)verProximoCaracter()) && len < sizeof(lex) - 1)
        lex[len++] = obtenerCaracter();

    if (verProximoCaracter() == '.')
    {
        esFloat = 1;
        lex[len++] = obtenerCaracter();
        while (isdigit((unsigned char)verProximoCaracter()) && len < sizeof(lex) - 1)
            lex[len++] = obtenerCaracter();
    }

    if (verProximoCaracter() == 'e' || verProximoCaracter() == 'E')
    {
        esFloat = 1;
        lex[len++] = obtenerCaracter();
        if (verProximoCaracter() == '+' || verProximoCaracter() == '-')
            lex[len++] = obtenerCaracter();

        if (!isdigit((unsigned char)verProximoCaracter()))
        {
            lex[len] = '\0'; // Asegurar terminación para el mensaje de error
            return crearTokenConLexema(TOKEN_ERROR, "Exponente flotante mal formado");
        }
        
        // Si hay un dígito, consumimos todos los que sigan
        while (isdigit((unsigned char)verProximoCaracter()) && len < sizeof(lex) - 1)
            lex[len++] = obtenerCaracter();
    }

    lex[len] = '\0';

    if (esFloat) {
        Token token = crearTokenConLexema(TOKEN_FLOAT, lex);
        token.valor.float_value = strtod(token.lexema, NULL);
        return token;
    }

    Token token = crearTokenConLexema(TOKEN_INT_DEC, lex);
    token.valor.int_value = strtol(token.lexema, NULL, 10);
    return token;
}

Token scanStringLiteral()
{
    char lex[MAX_LEXEMA];
    size_t len = 0;

    if(len < sizeof(lex) - 1)
        lex[len++] = '"';

    while(1)
    {
        char d = obtenerCaracter();
        if (d == '\0')
            return crearTokenConLexema(TOKEN_ERROR, "Literal de string sin terminacion");

        if(d == '"')
        {
            if (len < sizeof(lex) - 1)
                    lex[len++] = '"';

            lex[len] = '\0';
            return crearTokenConLexema(TOKEN_STRING_LITERAL, lex);
        }

        if (d == '\\')
        {
            if (!agregarEscapeAlLexema(lex, &len, sizeof(lex)))
            {
                // Si 'len' está al límite, no podemos agregar el escape.
                if (len + 2 >= sizeof(lex)) // +2 por \ y el char
                    return crearTokenConLexema(TOKEN_ERROR, "String literal demasiado largo");
                
                return crearTokenConLexema(TOKEN_ERROR, "Escape no valido");
            }
        }
        else
        {
            if (len < sizeof(lex) - 1)
                lex[len++] = d;
            else
                return crearTokenConLexema(TOKEN_ERROR, "String literal demasiado largo");
        }
    }
}

Token scanCharLiteral(){
    char lex[MAX_LEXEMA];
    size_t len = 0;

    if (len < sizeof(lex) - 1)
        lex[len++] = '\'';
    
    char d = obtenerCaracter();
    if (d == '\0')
        return crearTokenConLexema(TOKEN_ERROR, "Literal de char no terminado");
        
    if (d == '\\')
    {
        if (!agregarEscapeAlLexema(lex, &len, sizeof(lex)))
            return crearTokenConLexema(TOKEN_ERROR, "Escape no valido en char");

        char closing = obtenerCaracter();
        if (closing != '\'')
            return crearTokenConLexema(TOKEN_ERROR, "Literal de char incompleto");

        if (len < sizeof(lex) - 1)
                lex[len++] = '\'';

        lex[len] = '\0';

        return crearTokenConLexema(TOKEN_CHAR_LITERAL, lex);
    }
    else
    {
        char closing = obtenerCaracter();
        if (closing != '\'')
            return crearTokenConLexema(TOKEN_ERROR, "Literal de char incompleto");
        if (len < sizeof(lex) - 1)
            lex[len++] = d;
        if (len < sizeof(lex) - 1)
            lex[len++] = '\'';

        lex[len] = '\0';

        return crearTokenConLexema(TOKEN_CHAR_LITERAL, lex);
    }
}

Token scanSimbolo(char c){
    switch (c)
    {
        case ';':
            return crearTokenConLexema(TOKEN_SEMICOLON, ";");
        case ',':
            return crearTokenConLexema(TOKEN_COMMA, ",");
        case '(':
            return crearTokenConLexema(TOKEN_LPAREN, "(");
        case ')':
            return crearTokenConLexema(TOKEN_RPAREN, ")");
        case '[':
            return crearTokenConLexema(TOKEN_LBRACKET, "[");
        case ']':
            return crearTokenConLexema(TOKEN_RBRACKET, "]");
        case '*':
            return crearTokenConLexema(TOKEN_ASTERISK, "*");
        case '=':
            return crearTokenConLexema(TOKEN_ASSIGN, "=");
        default:
        {
            char s[4] = {0};
            s[0] = c;
            s[1] = '\0';

            return crearTokenConLexema(TOKEN_OTHER, s);
        }
    }
}

Token scanNumber(char c)
{
    char lex[MAX_LEXEMA];
    size_t len = 0;
    lex[len++] = c;

    if (c == '0')
    {
        if (verProximoCaracter() == 'x' || verProximoCaracter() == 'X')
            return scanHexNumber(lex, len);

        if (verProximoCaracter() >= '0' && verProximoCaracter() <= '7')
            return scanOctalNumber(lex, len);
    }

    return scanFloatODecimal(lex, len);
}

// getNextToken. Funcion principal del Scanner
Token getNextToken(void)
{
    char c;

    // Saltar espacios
    do
    {
        c = obtenerCaracter();
        if (c == '\0')
            return crearTokenConLexema(TOKEN_END, "<EOF>");
    } while(isspace((unsigned char)c));

    // Ident / Keyword
    if (isalpha((unsigned char)c) || c == '_')
    {
        return scanIdentOKeyword(c);
    }

    // Numeros hex/oct y float/dec
    if(isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)verProximoCaracter())))
    {
        return scanNumber(c);
    }

    //String literals
    if(c == '"')
    {
        return scanStringLiteral();    
    }

    //Char literals
    if(c == '\'')
    {
        return scanCharLiteral();
    }

    // Simbolos simples
    return scanSimbolo(c);    
}

// printToken. Info del token 
void printToken(const Token *token) {
    if (!token) return;
    const char *nombre = "UNKNOWN";
    
    switch (token->tipo) {
        case TOKEN_ERROR: nombre = "ERROR"; break;
        case TOKEN_END: nombre = "END"; break;
        case TOKEN_IDENT: nombre = "IDENT"; break;
        case TOKEN_KEYWORD: nombre = "KEYWORD"; break;
        case TOKEN_INT_DEC: nombre = "INT_DEC"; break;
        case TOKEN_INT_HEX: nombre = "INT_HEX"; break;
        case TOKEN_INT_OCTAL: nombre = "INT_OCT"; break;
        case TOKEN_FLOAT: nombre = "FLOAT"; break;
        case TOKEN_CHAR_LITERAL: nombre = "CHAR"; break;
        case TOKEN_STRING_LITERAL: nombre = "STRING"; break;
        case TOKEN_SEMICOLON: nombre = "SEMICOLON"; break;
        case TOKEN_COMMA: nombre = "COMMA"; break;
        case TOKEN_LPAREN: nombre = "LPAREN"; break;
        case TOKEN_RPAREN: nombre = "RPAREN"; break;
        case TOKEN_LBRACKET: nombre = "LBRACKET"; break;
        case TOKEN_RBRACKET: nombre = "RBRACKET"; break;
        case TOKEN_ASTERISK: nombre = "ASTERISK"; break;
        case TOKEN_ASSIGN: nombre = "ASSIGN"; break;
        case TOKEN_OTHER: nombre = "OTHER"; break;
    }
    if (token->tipo == TOKEN_INT_DEC || token->tipo == TOKEN_INT_HEX || token->tipo == TOKEN_INT_OCTAL) {
        printf("%s \"%s\" value=%ld\n", nombre, token->lexema, token->valor.int_value);
    } else if (token->tipo == TOKEN_FLOAT) {
        printf("%s \"%s\" value=%g\n", nombre, token->lexema, token->valor.float_value);
    } else {
        printf("%s \"%s\"\n", nombre, token->lexema);
    }
}