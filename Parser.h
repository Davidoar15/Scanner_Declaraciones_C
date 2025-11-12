#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "Scanner.h"

typedef enum {
    OK,
    ERROR_SINTACTICO,
    FALTA_MEMORIA
} ResultadoParseo;

ResultadoParseo parseDcl(char *outDescription, size_t maxLen);

int esLineaVacia(const char* linea);

#endif /* PARSER_H */