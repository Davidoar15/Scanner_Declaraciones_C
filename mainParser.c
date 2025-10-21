#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Parser.h"
#include "Scanner.h"

int main(void) {
    char linea[4096];
    char descripcion[2048];

    printf("Introduce declaracion:\n");
    if (!fgets(linea, sizeof(linea), stdin)) {
        fprintf(stderr, "Error leyendo linea\n");
        return 1;
    }

    // Inicializamos el scanner para leer desde la cadena 
    iniciarScannerDesdeCadena(linea);

    ResultadoParseo r = parseDcl(descripcion, sizeof(descripcion));
    if (r == OK) {
        printf("Interpretacion: %s\n", descripcion);
    } else if (r == ERROR_SINTACTICO) {
        printf("Error de sintaxis en la declaracion.\n");
    } else if (r == FALTA_MEMORIA) {
        printf("Error: memoria insuficiente para la descripcion.\n");
    } else {
        printf("Error desconocido.\n");
    }

    return (r == OK) ? 0 : 1;
}