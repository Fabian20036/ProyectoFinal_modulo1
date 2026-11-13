/* Etapa 3: analizador de mediciones de sec_guard*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Una medición del archivo */
typedef struct {
    int categoria;      /* 0 = DISCO, 1 = PROCESOS, 2 = PERMISOS */
    int valor;
} Medicion;

/* Resumen de una categoría */
typedef struct {
    const char *nombre;
    int limite;         /* valor máximo permitido */
    int cantidad, suma, minimo, maximo, alertas;
} Categoria;

Categoria cats[3] = {
    { "DISCO",    100,  0, 0, 0, 0, 0 },
    { "PROCESOS", 2000, 0, 0, 0, 0, 0 },
    { "PERMISOS", 200,  0, 0, 0, 0, 0 }
};

/* Devuelve 0, 1 o 2 según el nombre; -1 si no existe */
int buscar_categoria(const char *nombre)
{
    for (int i = 0; i < 3; i++)
        if (strcmp(nombre, cats[i].nombre) == 0)
            return i;
    return -1;
}

/* DISCO >= 80, PROCESOS >= 30, PERMISOS > 0 */
int es_alerta(int cat, int valor)
{
    if (cat == 0) return valor >= 80;
    if (cat == 1) return valor >= 30;
    return valor > 0;
}

/* Lee el archivo. Devuelve el arreglo de mediciones, o termina el programa si hay error. */
Medicion *leer(const char *ruta, char *etiqueta, int *total)
{
    FILE *f = fopen(ruta, "r");
    if (f == NULL) {
        printf("Error: no se pudo abrir %s\n", ruta);
        exit(1);
    }

    if (fscanf(f, "%31s %d", etiqueta, total) != 2 || *total < 0 || *total > 200) {
        printf("Error: etiqueta o cantidad inválida\n");
        fclose(f);
        exit(1);
    }

    Medicion *m = NULL;
    if (*total > 0) {
        m = malloc(*total * sizeof(Medicion));
        if (m == NULL) {
            printf("Error: no hay memoria\n");
            fclose(f);
            exit(1);
        }
    }

    char nombre[16];
    int valor, leidos = 0;
    while (fscanf(f, "%15s %d", nombre, &valor) == 2) {
        int cat = buscar_categoria(nombre);
        if (leidos >= *total || cat < 0 || valor < 0 || valor > cats[cat].limite) {
            printf("Error en el registro %d: %s %d\n", leidos + 1, nombre, valor);
            free(m);
            fclose(f);
            exit(1);
        }
        m[leidos].categoria = cat;
        m[leidos].valor = valor;
        leidos++;
    }
    fclose(f);

    if (leidos != *total) {
        printf("Error: se declararon %d registros y hay %d\n", *total, leidos);
        free(m);
        exit(1);
    }
    return m;
}

/* Calcula cantidad, suma, mínimo, máximo y alertas por categoría */
void procesar(Medicion *m, int total)
{
    for (int i = 0; i < total; i++) {
        Categoria *c = &cats[m[i].categoria];
        if (c->cantidad == 0) {
            c->minimo = m[i].valor;
            c->maximo = m[i].valor;
        }
        if (m[i].valor < c->minimo) c->minimo = m[i].valor;
        if (m[i].valor > c->maximo) c->maximo = m[i].valor;
        c->cantidad++;
        c->suma += m[i].valor;
        if (es_alerta(m[i].categoria, m[i].valor)) c->alertas++;
    }
}

/* Escribe el informe. Devuelve 0 si salió bien. */
int escribir(const char *ruta, const char *etiqueta, int total)
{
    FILE *f = fopen(ruta, "w");
    if (f == NULL) {
        printf("Error: no se pudo crear %s\n", ruta);
        return 1;
    }

    int alertas = 0;
    fprintf(f, "INFORME DE MONITOREO\n\nEtiqueta: %s\nRegistros: %d\n", etiqueta, total);

    for (int i = 0; i < 3; i++) {
        fprintf(f, "\nCategoría %s\n", cats[i].nombre);
        if (cats[i].cantidad == 0) {
            fprintf(f, "  Sin datos\n");
            continue;
        }
        fprintf(f, "  Cantidad: %d\n  Suma: %d\n  Mínimo: %d\n  Máximo: %d\n",
                cats[i].cantidad, cats[i].suma, cats[i].minimo, cats[i].maximo);
        fprintf(f, "  Promedio: %.2f\n  Alertas: %d\n",
                (double) cats[i].suma / cats[i].cantidad, cats[i].alertas);
        alertas += cats[i].alertas;
    }

    fprintf(f, "\nObservaciones que cumplen condiciones de alerta: %d.\n", alertas);
    fprintf(f, "Umbrales utilizados con fines didácticos.\n");

    if (fclose(f) != 0) {
        printf("Error: no se pudo cerrar %s\n", ruta);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Uso: %s <entrada> <salida>\n", argv[0]);
        return 1;
    }

    char etiqueta[32];
    int total;

    Medicion *m = leer(argv[1], etiqueta, &total);
    procesar(m, total);
    int error = escribir(argv[2], etiqueta, total);
    free(m);

    if (error) return 1;
    printf("Informe generado en %s\n", argv[2]);
    return 0;
}
