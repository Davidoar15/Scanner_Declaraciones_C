#include <stdio.h>
#include "Scanner.h"

int main(void) {
    char linea[4096];

    printf("Escriba la declaracion: ");
    if (!fgets(linea, sizeof(linea), stdin)) return 0;

    iniciarScannerDesdeCadena(linea);

    while (1) {
        Token token = getNextToken();
        printToken(&token);
        if (token.tipo == TOKEN_END || token.tipo == TOKEN_ERROR) break;
    }
    
    return 0;
}