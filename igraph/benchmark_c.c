#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <igraph.h>

// Nombres de las 16 clases de tríadas de Holland & Leinhardt (MAN labeling)
static const char *TRIAD_NAMES[16] = {
    "003",  // 0: Vacío (sin aristas)
    "012",  // 1: 1 arista asimétrica
    "102",  // 2: 1 arista mutua
    "021D", // 3: Convergente (in-star: v->u, w->u)
    "021U", // 4: Divergente (out-star: u->v, u->w)
    "021C", // 5: Camino dirigido (u->v->w)
    "111D", // 6: Mutua + entrante
    "111U", // 7: Mutua + saliente
    "030T", // 8: Triángulo transitivo (feed-forward loop)
    "030C", // 9: Ciclo dirigido de 3
    "201",  // 10: 2 mutuas
    "120D", // 11: Mutua + 2 entrantes
    "120U", // 12: Mutua + 2 salientes
    "120C", // 13: Mutua + ciclo dirigido
    "210",  // 14: 2 mutuas + 1 asimétrica
    "300"   // 15: 3 mutuas (clique completo)
};

// Estructura temporal para aristas leídas
typedef struct {
    int u;
    int v;
} RawEdge;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <ruta_al_grafo_FormatoP>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    printf("====================================================\n");
    printf(" Benchmark C Nativo: igraph_triad_census (C Core)   \n");
    printf(" Archivo: %s\n", filename);
    printf("====================================================\n\n");

    double t_start_total = get_time_sec();

    // 1. Abrir archivo
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error al abrir archivo");
        return 1;
    }

    // 2. Leer cabecera N M
    long long num_nodes = 0;
    long long num_edges_header = 0;
    if (fscanf(f, "%lld %lld", &num_nodes, &num_edges_header) != 2) {
        fprintf(stderr, "Error al leer cabecera N M\n");
        fclose(f);
        return 1;
    }

    double t0_read = get_time_sec();

    // 3. Leer aristas dinámicamente
    size_t capacity = (num_edges_header > 0) ? (size_t)num_edges_header : 100000;
    RawEdge *raw_edges = (RawEdge *)malloc(capacity * sizeof(RawEdge));
    if (!raw_edges) {
        fprintf(stderr, "Error de memoria al alojar raw_edges\n");
        fclose(f);
        return 1;
    }

    size_t count_edges = 0;
    int min_id = 1;
    long long u = 0, v = 0, t = 0;

    while (fscanf(f, "%lld %lld %lld", &u, &v, &t) == 3) {
        if (u == v) continue; // Descartar self-loops
        
        // Regla FormatoP: solo considerar tipo 1 (u->v) y tipo 3 (u<->v)
        // Tipo 2 es el reflejo inverso ya incluido por tipo 1
        if (t == 1 || t == 3) {
            if (u < min_id) min_id = (int)u;
            if (v < min_id) min_id = (int)v;

            if (count_edges >= capacity) {
                capacity *= 2;
                RawEdge *new_ptr = (RawEdge *)realloc(raw_edges, capacity * sizeof(RawEdge));
                if (!new_ptr) {
                    fprintf(stderr, "Error de memoria al redimensionar raw_edges\n");
                    free(raw_edges);
                    fclose(f);
                    return 1;
                }
                raw_edges = new_ptr;
            }
            raw_edges[count_edges].u = (int)u;
            raw_edges[count_edges].v = (int)v;
            count_edges++;
        }
    }
    fclose(f);

    int shift = (min_id == 0) ? 0 : 1;
    double t_read = get_time_sec() - t0_read;

    printf("1. Lectura de Archivo completada en %.4f s\n", t_read);
    printf("   - Base detectada    : Base %d (min_id = %d)\n", shift, min_id);
    printf("   - Nodos (cabecera)  : %lld\n", num_nodes);
    printf("   - Aristas dirigidas : %zu\n", count_edges);

    // 4. Construir estructuras de igraph
    double t0_build = get_time_sec();
    igraph_vector_int_t edges_vec;
    igraph_vector_int_init(&edges_vec, count_edges * 2);

    for (size_t i = 0; i < count_edges; i++) {
        VECTOR(edges_vec)[2 * i]     = raw_edges[i].u - shift;
        VECTOR(edges_vec)[2 * i + 1] = raw_edges[i].v - shift;
    }
    free(raw_edges);

    igraph_t g;
    igraph_create(&g, &edges_vec, (igraph_int_t)num_nodes, IGRAPH_DIRECTED);
    igraph_vector_int_destroy(&edges_vec);

    double t_build = get_time_sec() - t0_build;
    printf("2. Construcción de grafo en igraph (C) en %.4f s\n", t_build);

    // 5. Ejecutar Censo de Tríadas en C puro
    printf("\n--- Ejecutando igraph_triad_census() [C Core] ---\n");
    double t0_census = get_time_sec();

    igraph_vector_t census_res;
    igraph_vector_init(&census_res, 0);

    igraph_error_t err = igraph_triad_census(&g, &census_res);
    if (err != IGRAPH_SUCCESS) {
        fprintf(stderr, "Error al ejecutar igraph_triad_census\n");
        igraph_vector_destroy(&census_res);
        igraph_destroy(&g);
        return 1;
    }

    double t_census = get_time_sec() - t0_census;
    printf("3. Censo de tríadas completado en %.4f s\n\n", t_census);

    // 6. Mostrar resultados de las 16 clases
    printf("Resultados del Triad Census (16 clases de Holland & Leinhardt):\n");
    printf("%-8s %-8s %-14s %22s\n", "Clase", "Nombre", "Conexidad", "Conteo");
    printf("------------------------------------------------------------\n");

    long long total_connected = 0;
    igraph_integer_t len = igraph_vector_size(&census_res);
    for (igraph_integer_t i = 0; i < len; i++) {
        long long count = (long long)VECTOR(census_res)[i];
        const char *name = (i < 16) ? TRIAD_NAMES[i] : "???";
        const char *conn = (i < 3) ? "Desconectada" : "Conexa";

        if (i >= 3) {
            total_connected += count;
        }
        printf("[%02d]     %-8s %-14s %22lld\n", (int)i, name, conn, count);
    }
    printf("------------------------------------------------------------\n");
    printf("Total subgrafos conexos de 3 nodos : %lld\n", total_connected);

    double t_total = get_time_sec() - t_start_total;
    printf("\n--- Resumen de Tiempos ---\n");
    printf("Tiempo Lectura I/O       : %.6f s\n", t_read);
    printf("Tiempo Construcción Grafo: %.6f s\n", t_build);
    printf("Tiempo Censo de Tríadas  : %.6f s\n", t_census);
    printf("Tiempo Total             : %.6f s\n", t_total);

    // Limpieza
    igraph_vector_destroy(&census_res);
    igraph_destroy(&g);

    return 0;
}
