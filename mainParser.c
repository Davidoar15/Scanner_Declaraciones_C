#include "parser.h"
#include "Scanner.h"

#define DIR_ENTRADA "test/"
#define DIR_SALIDA "result/"

int main(int argc, char *argv[])
{
    char linea[4096];
    char descripcion[2048];

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <nombre_archivo>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s declaraciones.txt\n", argv[0]);
        return 1;
    }

    const char* nombreArchivo = argv[1];
    char rutaEntrada[1024];
    char rutaSalida[1024];

    snprintf(rutaEntrada, sizeof(rutaEntrada), "%s%s", DIR_ENTRADA, nombreArchivo);
    snprintf(rutaSalida, sizeof(rutaSalida), "%s%s", DIR_SALIDA, nombreArchivo);

    FILE* fin = fopen(rutaEntrada, "r");
    if (!fin)
    {
        fprintf(stderr, "Error abriendo archivo de entrada: %s\n", rutaEntrada);
        perror(NULL);
        return 1;
    }

    FILE* fout = fopen(rutaSalida, "w");
    if (!fout)
    {
        fprintf(stderr, "Error abriendo archivo de salida: %s\n", rutaSalida);
        perror(NULL);
        fclose(fin);
        return 1;
    }

    while (fgets(linea, sizeof(linea), fin) != NULL)
    {
        // Si la linea esta vacia (solo ' ', '\t', '\n'), la ignoramos
        if (esLineaVacia(linea))
            continue;
        
        iniciarScannerDesdeCadena(linea);

        ResultadoParseo r = parseDcl(descripcion, sizeof(descripcion));

        if (r == OK)
            fprintf(fout, "%s\n", descripcion);
        else if (r == ERROR_SINTACTICO)
        {
            linea[strcspn(linea, "\r\n")] = 0;
            fprintf(fout, "Error sintactico en: %s\n", linea);
        }
        else if (r == FALTA_MEMORIA)
            fprintf(fout, "Error: memoria insuficiente parseando.\n");
        else
            fprintf(fout, "Error desconocido parseando.\n");
    }

    fclose(fin);
    fclose(fout);

    printf("Procesamiento completado.\n");
    printf("Entrada: %s\n", rutaEntrada);
    printf("Salida:  %s\n", rutaSalida);

    return 0;
}