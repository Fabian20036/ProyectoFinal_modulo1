/* Etapa 3: analizador de mediciones de sec_guard
   Uso: sec_engine <archivo_entrada> <archivo_salida> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ETIQUETA  31
#define MAX_REGISTROS 200
#define NUM_CAT       3

/* Una medición leída del archivo */
typedef struct {
    int categoria;      /* 0 = DISCO, 1 = PROCESOS, 2 = PERMISOS */
    int valor;
} Medicion;

/* Resumen de una categoría */
typedef struct {
    const char *nombre;
    int limite;         /* valor máximo permitido */
    int cantidad;
    long suma;
    int minimo;
    int maximo;
    int alertas;
} Categoria;


/* Devuelve 0, 1 o 2 según el nombre, o -1 si no existe */
int buscar_categoria(const char *nombre)
{   if (strcmp(nombre, "DISCO") == 0)    return 0;
    if (strcmp(nombre, "PROCESOS") == 0) return 1;
    if (strcmp(nombre, "PERMISOS") == 0) return 2;
    return -1;
}

/* Condiciones de alerta: DISCO >= 80, PROCESOS >= 30, PERMISOS > 0 */
int es_alerta(int categoria, int valor)
{   if (categoria == 0) return valor >= 80;
    if (categoria == 1) return valor >= 30;
    return valor > 0;
}

/* Muestra el error, libera la memoria, cierra el archivo y devuelve 0 */
int error_lectura(FILE *archivo, Medicion *mediciones, int linea, const char *mensaje)
{
    fprintf(stderr, "Error en la línea %d: %s\n", linea, mensaje);
    free(mediciones);
    fclose(archivo);
    return 0;
}


/* Lee y valida el archivo. Devuelve 1 si todo es correcto y 0 si hay error. */
int leer_archivo(const char *ruta, char etiqueta[], int *total,
                 Medicion **mediciones, Categoria cats[])
{
    char linea[128];
    int num_linea = 0;
    int leidos = 0;

    *mediciones = NULL;

    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        fprintf(stderr, "Error: no se pudo abrir %s\n", ruta);
        return 0;
    }

    /* Línea 1: etiqueta, sin espacios y de hasta 31 caracteres */
    num_linea++;
    if (fgets(linea, sizeof linea, archivo) == NULL)
        return error_lectura(archivo, NULL, num_linea, "el archivo está vacío");
    linea[strcspn(linea, "\r\n")] = '\0';
    if (strlen(linea) == 0 || strlen(linea) > MAX_ETIQUETA || strchr(linea, ' ') != NULL)
        return error_lectura(archivo, NULL, num_linea, "etiqueta inválida (sin espacios, de 1 a 31 caracteres)");
    strcpy(etiqueta, linea);

    /* Línea 2: cantidad de registros, entre 0 y 200 */
    num_linea++;
    char sobrante;
    if (fgets(linea, sizeof linea, archivo) == NULL
        || sscanf(linea, "%d %c", total, &sobrante) != 1
        || *total < 0 || *total > MAX_REGISTROS)
        return error_lectura(archivo, NULL, num_linea, "cantidad de registros inválida (0 a 200)");

    /* Memoria dinámica para las mediciones */
    if (*total > 0) {
        *mediciones = malloc(*total * sizeof(Medicion));
        if (*mediciones == NULL)
            return error_lectura(archivo, NULL, num_linea, "no se pudo reservar memoria");
    }

    /* Registros: CATEGORIA VALOR */
    while (fgets(linea, sizeof linea, archivo) != NULL) {
        num_linea++;
        char nombre[16];
        int valor;

        if (strspn(linea, " \t\r\n") == strlen(linea))
            continue;   /* línea en blanco */

        if (leidos >= *total)
            return error_lectura(archivo, *mediciones, num_linea, "hay más registros que los declarados");

        if (sscanf(linea, "%15s %d %c", nombre, &valor, &sobrante) != 2)
            return error_lectura(archivo, *mediciones, num_linea, "se esperaba CATEGORIA VALOR");

        int cat = buscar_categoria(nombre);
        if (cat < 0)
            return error_lectura(archivo, *mediciones, num_linea, "categoría desconocida");

        if (valor < 0 || valor > cats[cat].limite)
            return error_lectura(archivo, *mediciones, num_linea, "valor fuera de intervalo");

        (*mediciones)[leidos].categoria = cat;
        (*mediciones)[leidos].valor = valor;
        leidos++;
    }

    if (leidos < *total)
        return error_lectura(archivo, *mediciones, num_linea, "hay menos registros que los declarados");

    fclose(archivo);
    return 1;
}


/* Calcula cantidad, suma, mínimo, máximo y alertas por categoría */
void procesar(const Medicion mediciones[], int total, Categoria cats[])
{
    for (int i = 0; i < total; i++) {
        Categoria *c = &cats[mediciones[i].categoria];
        int valor = mediciones[i].valor;

        if (c->cantidad == 0) {        /* primera medición de la categoría */
            c->minimo = valor;
            c->maximo = valor;
        }
        if (valor < c->minimo) c->minimo = valor;
        if (valor > c->maximo) c->maximo = valor;

        c->cantidad++;
        c->suma += valor;
        if (es_alerta(mediciones[i].categoria, valor))
            c->alertas++;
    }
}

/* Escribe el informe. Devuelve 1 si se escribió completo y 0 si hubo error. */
int escribir_informe(const char *ruta, const char *etiqueta, int total, const Categoria cats[])
{
    FILE *salida = fopen(ruta, "w");
    if (salida == NULL) {
        fprintf(stderr, "Error: no se pudo crear %s\n", ruta);
        return 0;
    }

    int total_alertas = 0;

    fprintf(salida, "INFORME DE MONITOREO\n\n");
    fprintf(salida, "Etiqueta: %s\n", etiqueta);
    fprintf(salida, "Registros: %d\n", total);

    for (int i = 0; i < NUM_CAT; i++) {
        fprintf(salida, "\nCategoría %s\n", cats[i].nombre);

        if (cats[i].cantidad == 0) {
            fprintf(salida, "  Sin datos\n");
            continue;
        }

        double promedio = (double) cats[i].suma / cats[i].cantidad;
        fprintf(salida, "  Cantidad: %d\n", cats[i].cantidad);
        fprintf(salida, "  Suma: %ld\n", cats[i].suma);
        fprintf(salida, "  Mínimo: %d\n", cats[i].minimo);
        fprintf(salida, "  Máximo: %d\n", cats[i].maximo);
        fprintf(salida, "  Promedio: %.2f\n", promedio);
        fprintf(salida, "  Alertas: %d\n", cats[i].alertas);
        total_alertas += cats[i].alertas;
    }

    fprintf(salida, "\nObservaciones que cumplen condiciones de alerta: %d.\n", total_alertas);
    fprintf(salida, "Umbrales utilizados con fines didácticos.\n");

    /* Comprobar que la escritura y el cierre funcionaron */
    int error = ferror(salida);
    if (fclose(salida) != 0 || error) {
        fprintf(stderr, "Error: no se pudo escribir %s\n", ruta);
        remove(ruta);
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_entrada> <archivo_salida>\n", argv[0]);
        return 1;
    }

    Categoria cats[NUM_CAT] = {
        { "DISCO",    100,  0, 0, 0, 0, 0 },
        { "PROCESOS", 2000, 0, 0, 0, 0, 0 },
        { "PERMISOS", 200,  0, 0, 0, 0, 0 }
    };
    char etiqueta[MAX_ETIQUETA + 1];
    int total;
    Medicion *mediciones;

    if (!leer_archivo(argv[1], etiqueta, &total, &mediciones, cats))
        return 1;

    procesar(mediciones, total, cats);

    int ok = escribir_informe(argv[2], etiqueta, total, cats);
    free(mediciones);

    if (!ok)
        return 1;

    printf("Informe generado en %s (%d registros)\n", argv[2], total);
    return 0;
}
