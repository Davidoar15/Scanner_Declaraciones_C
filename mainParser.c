#include "parser.h"

#define DIR_ENTRADA "test/"
#define DIR_SALIDA "result/"
#define ERROR -1
#define SUCCESS 0

int main(int argc, char *argv[])
{
    char linea[4096];
    char descripcion[2048];

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s <nombre_archivo>\n", argv[0]);
        fprintf(stderr, "Ejemplo: %s declaraciones.txt\n", argv[0]);
        return ERROR;
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
        return ERROR;
    }

    FILE* fout = fopen(rutaSalida, "w");
    if (!fout)
    {
        fprintf(stderr, "Error abriendo archivo de salida: %s\n", rutaSalida);
        perror(NULL);
        fclose(fin);
        return ERROR;
    }

    while (fgets(linea, sizeof(linea), fin) != NULL)
    {   
        iniciarScannerDesdeCadena(linea);

        ResultadoParseo result = parseDcl(descripcion, sizeof(descripcion));

        switch (result)
        {
            case OK:
                fprintf(fout, "%s\n", descripcion);
                break;

            case ERROR_LINEA_VACIA:
                break;

            case ERROR_SINTACTICO:
                linea[strcspn(linea, "\r\n")] = 0;
                fprintf(fout, "Error sintactico en: %s\n", linea);
                break;

            case FALTA_MEMORIA:
                fprintf(fout, "Error: memoria insuficiente parseando.\n");
                break;

            default:
                fprintf(fout, "Error desconocido parseando.\n");
                break;
        }   
    }

    fclose(fin);
    fclose(fout);

    printf("Procesamiento completado.\n");
    printf("Entrada: %s\n", rutaEntrada);
    printf("Salida:  %s\n", rutaSalida);

    return SUCCESS;
}