#include "parser.h"

static Token currentToken;

// nombre del identificador parseado (almacenado por dirdcl)
static char parsed_name[256];

static void nextToken(void) {
    currentToken = getNextToken();
}

static void dcl(char *out, size_t maxLen, int *error);
static void dirdcl(char *out, size_t maxLen, int *error);
static ResultadoParseo parseDeclaracion(char *outDescription, size_t maxLen);

static int esInicioDeDeclaracion(void)
{
    return (currentToken.tipo == TOKEN_KEYWORD) || (currentToken.tipo == TOKEN_IDENT);
}

void parseUT(void)
{
    char linea[4096];
    char descripcion[2048];

    while (fgets(linea, sizeof(linea), stdin))
    {
        if (linea[0] == '\n') break;

        linea[strcspn(linea, "\r\n")] = 0; 
        iniciarScannerDesdeCadena(linea);

        nextToken();

        while (currentToken.tipo != TOKEN_END)
        {
            if (!esInicioDeDeclaracion())
            {
                fprintf(stderr, "\033[31mError: se esperaba tipo al inicio de declaración\033[0m\n");
                while (currentToken.tipo != TOKEN_SEMICOLON && currentToken.tipo != TOKEN_END)
                    nextToken();
                if (currentToken.tipo == TOKEN_SEMICOLON) nextToken();
                continue;
            }

            ResultadoParseo r = parseDeclaracion(descripcion, sizeof(descripcion));

            if (r == OK)
                fprintf(stderr, "\033[32m%s\033[0m\n", descripcion);
            else if (r == ERROR_SINTACTICO)
            {
                fprintf(stderr, "\033[31mError sintáctico cerca de: %s\033[0m\n",
                        currentToken.lexema ? currentToken.lexema : "(fin)");
                while (currentToken.tipo != TOKEN_SEMICOLON && currentToken.tipo != TOKEN_END)
                    nextToken();
                if (currentToken.tipo == TOKEN_SEMICOLON) nextToken();
            }
            else if (r == FALTA_MEMORIA)
            {
                fprintf(stderr, "\033[31mError: falta memoria\033[0m\n");
                return;
            }
        }
    }
}

ResultadoParseo parseDeclaracion(char *outDescription, size_t maxLen)
{
    if (!outDescription || maxLen == 0) return FALTA_MEMORIA;

    parsed_name[0] = '\0';
    outDescription[0] = '\0';

    char datatype[256] = {0};
    if (currentToken.tipo == TOKEN_KEYWORD || currentToken.tipo == TOKEN_IDENT)
    {
        strncpy(datatype, currentToken.lexema ? currentToken.lexema : "", sizeof(datatype)-1);
    }
    else
        return ERROR_SINTACTICO;

    nextToken();  

    int err = 0;
    dcl(outDescription, maxLen, &err);
    if (err) return ERROR_SINTACTICO;

    
    if (currentToken.tipo != TOKEN_SEMICOLON)
        return ERROR_SINTACTICO;

    nextToken(); 

    if (parsed_name[0] == '\0')
        return ERROR_SINTACTICO;

    const char *p = outDescription;
    while (*p == ' ') p++;

    char tmp[2048];
    snprintf(tmp, sizeof(tmp), "%s: %s%s%s",
             parsed_name,
             p[0] ? p : "",
             p[0] ? " " : "",
             datatype);

    strncpy(outDescription, tmp, maxLen-1);
    outDescription[maxLen-1] = '\0';

    return OK;
}

// dcl: cuenta '*' luego llama a dirdcl y añade "apuntador a" por cada '*'
static void dcl(char *out, size_t maxLen, int *error) {
    int ns = 0;
    while (currentToken.tipo == TOKEN_ASTERISK) {
        ns++;
        nextToken();
    }

    dirdcl(out, maxLen, error);
    if (*error) return;

    while (ns-- > 0) {
        if ((size_t)(strlen(out) + strlen(" apuntador a") + 1) < maxLen) {
            strncat(out, " apuntador a", maxLen - strlen(out) - 1);
        } else {
            *error = 1;
            return;
        }
    }
}

// dirdcl: maneja ( dcl ) o name, luego [] y () sucesivos 
static void dirdcl(char *out, size_t maxLen, int *error)
{
    if (currentToken.tipo == TOKEN_LPAREN)
    {
        nextToken();
        dcl(out, maxLen, error);

        if (*error)
            return;

        if (currentToken.tipo != TOKEN_RPAREN)
        {
            *error = 1;
            return;
        }
        nextToken();
    }
    else if (currentToken.tipo == TOKEN_IDENT)
    {
        strncpy(parsed_name, currentToken.lexema ? currentToken.lexema : "", sizeof(parsed_name) - 1);
        parsed_name[sizeof(parsed_name) - 1] = '\0';
        nextToken();
    }
    else
    {
        *error = 1;
        return;
    }

    // Manejo de [] y () en cadena 
    // (FIX) Añadido TOKEN_LPAREN para manejar "funcion(con_parametros)"
    while (currentToken.tipo == TOKEN_INVOKE || currentToken.tipo == TOKEN_LBRACKET || currentToken.tipo == TOKEN_LPAREN)
    {
        if (currentToken.tipo == TOKEN_INVOKE) // int x()
        {
            if ((size_t)(strlen(out) + strlen(" funcion que retorna") + 1) < maxLen)
                strncat(out, " funcion que retorna", maxLen - strlen(out) - 1);
            else
            {
                *error = 1;
                return;
            }
            nextToken();
        }
        else if (currentToken.tipo == TOKEN_LPAREN) // int x(int i)
        {
            if ((size_t)(strlen(out) + strlen(" funcion que retorna") + 1) < maxLen)
                strncat(out, " funcion que retorna", maxLen - strlen(out) - 1);
            else
            {
                *error = 1;
                return;
            }
            nextToken(); // consumir '('

            int nesting = 1;
            while (nesting > 0 && currentToken.tipo != TOKEN_END)
            {
                if (currentToken.tipo == TOKEN_LPAREN)
                    nesting++;
                else if (currentToken.tipo == TOKEN_RPAREN)
                    nesting--;

                if (nesting > 0)
                    nextToken();
            }
            
            if (nesting > 0)
            {
                *error = 1;
                return;
            }
            nextToken(); // consumir el ultimo ')'
        }
        else if (currentToken.tipo == TOKEN_LBRACKET)
        {
            // consumir '[' 
            nextToken();
            // construir contenido entre [ ... ] 
            char inside[128] = {0};
            size_t ipos = 0;

            // si es inmediato ']' -> array de 
            if (currentToken.tipo == TOKEN_RBRACKET)
            {
                if ((size_t)(strlen(out) + strlen(" array de") + 1) < maxLen)
                    strncat(out, " array de", maxLen - strlen(out) - 1);
                else
                {
                    *error = 1;
                    return;
                }
                // consumir ']' 
                nextToken();
                continue;
            }

            // sino, acumular tokens hasta ']'
            while (currentToken.tipo != TOKEN_RBRACKET && currentToken.tipo != TOKEN_END)
            {
                const char *lex = currentToken.lexema ? currentToken.lexema : "";
                size_t l = strlen(lex);
                if (ipos + l + 2 < sizeof(inside))
                {
                    if (ipos > 0)
                        inside[ipos++] = ' ';
                    memcpy(inside + ipos, lex, l);
                    ipos += l;
                    inside[ipos] = '\0';
                }
                else
                {
                    *error = 1;
                    return;
                }
                nextToken();
            }

            if (currentToken.tipo != TOKEN_RBRACKET)
            {
                *error = 1;
                return;
            }
            // ahora currentToken es ']'
            nextToken();

            char tmp[256];
            if (inside[0] != '\0')
                snprintf(tmp, sizeof(tmp), " array[%s] de", inside);
            else
                snprintf(tmp, sizeof(tmp), " array de");
            
            if ((size_t)(strlen(out) + strlen(tmp) + 1) < maxLen)
                strncat(out, tmp, maxLen - strlen(out) - 1);
            else
            {
                *error = 1;
                return;
            }
        }
    }
}