#include <stdio.h>
#include "Scanner.h"

int main(void) {
    char linea[4096];

    printf("Escriba la declaracion: ");
    if (!fgets(linea, sizeof(linea), stdin)) return 0;

    ScannerInitFromString(linea);

    while (1) {
        Token token = GetNextToken();
        PrintToken(&token);
        if (token.tipo == TOKEN_END || token.tipo == TOKEN_ERROR) {
            TokenFree(&token);
            break;
        }
        TokenFree(&token);
    }
    return 0;
}