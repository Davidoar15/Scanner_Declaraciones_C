#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "Scanner.h"

typedef enum {
    OK,
    ERROR_SINTACTICO,
    ERROR_LINEA_VACIA,
    FALTA_MEMORIA
} ResultadoParseo;

void parseUT(void);

#endif /* PARSER_H */