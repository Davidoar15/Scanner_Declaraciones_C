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

void parseUT(){
    char linea[4096];
    char descripcion[2048];

    while (1)
    {
        if (fgets(linea, sizeof(linea), stdin) == NULL)
            break;
        
        if (linea[0] == '\n')
            break;

        iniciarScannerDesdeCadena(linea);

        ResultadoParseo result = parseDeclaracion(descripcion, sizeof(descripcion));

        switch (result)
        {
            case OK:
                fprintf(stderr, "\033[32m%s\033[0m\n", descripcion);
                break;

            case ERROR_LINEA_VACIA:
                break;

            case ERROR_SINTACTICO:
                linea[strcspn(linea, "\r\n")] = 0; // Se limpia el \n para el print
                fprintf(stderr, "\033[31mError sintactico en: %s\033[0m\n", linea);
                break;

            case FALTA_MEMORIA:
                fprintf(stderr, "\033[31mError: memoria insuficiente parseando.\033[0m\n");
                break;

            default:
                fprintf(stderr, "\033[31mError desconocido parseando.\033[0m\n");
                break;
        }
    }
}

ResultadoParseo parseDeclaracion(char *outDescription, size_t maxLen)
{
    if (!outDescription || maxLen == 0)
        return FALTA_MEMORIA;

    parsed_name[0] = '\0';
    outDescription[0] = '\0';

    // leer primer token
    nextToken();

    if (currentToken.tipo == TOKEN_END)
        return ERROR_LINEA_VACIA;

    char datatype[256] = {0};
    if (currentToken.tipo == TOKEN_KEYWORD || currentToken.tipo == TOKEN_IDENT)
    {
        strncpy(datatype, currentToken.lexema ? currentToken.lexema : "", sizeof(datatype)-1);
        datatype[sizeof(datatype)-1] = '\0';
    }
    else
        return ERROR_SINTACTICO;
    

    nextToken();

    int err = 0;
    dcl(outDescription, maxLen, &err);

    if (err)
        return ERROR_SINTACTICO;

    // esperamos terminar la declaración con ';'
    if (currentToken.tipo == TOKEN_SEMICOLON)
        nextToken();
    
    else
        return ERROR_SINTACTICO;
    

    if (parsed_name[0] == '\0')
        return ERROR_SINTACTICO;

    char tmp[maxLen];
    tmp[0] = '\0';

    // quitar espacios redundantes al inicio de outDescription 
    const char *pdesc = outDescription;
    while (*pdesc == ' ')
        pdesc++;

    if (snprintf(tmp, sizeof(tmp), "%s: %s %s", parsed_name, pdesc, datatype) >= (int)sizeof(tmp))
        return FALTA_MEMORIA;
    

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
            {
                strncat(out, " funcion que retorna", maxLen - strlen(out) - 1);
            }
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
            {
                strncat(out, " funcion que retorna", maxLen - strlen(out) - 1);
            }
            else
            {
                *error = 1;
                return;
            }
            nextToken(); // consumir '('
            // Consumir lista de parametros (sin analizarlos)
            while (currentToken.tipo != TOKEN_RPAREN && currentToken.tipo != TOKEN_END)
            {
                nextToken();
            }
            if (currentToken.tipo != TOKEN_RPAREN)
            {
                *error = 1;
                return;
            }
            nextToken(); // consumir ')'
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
                {
                    strncat(out, " array de", maxLen - strlen(out) - 1);
                }
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
            {
                snprintf(tmp, sizeof(tmp), " array[%s] de", inside);
            }
            else
            {
                snprintf(tmp, sizeof(tmp), " array de");
            }
            
            if ((size_t)(strlen(out) + strlen(tmp) + 1) < maxLen)
            {
                strncat(out, tmp, maxLen - strlen(out) - 1);
            }
            else
            {
                *error = 1;
                return;
            }
        }
    }
}
