#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_VERTICES 50000
#define MAX_ARISTAS  14000000

// --- STRUCT DE ARREGLOS EN FORMATO CSR ---
struct Grafo {
    int cabeza[MAX_VERTICES + 1];   // Puntos de inicio de los vecinos de cada nodo en el arreglo
    int grado[MAX_VERTICES];        // Guarda el número de vecinos salientes de cada nodo
    int destino[MAX_ARISTAS];       // Vecinos contiguos en memoria
    int tipo[MAX_ARISTAS];          // Tipos de aristas contiguos en memoria
};

struct Grafo g; // Instancia única global del grafo CSR

int numVertices = 0;
int numAristas = 0;

// Arreglos auxiliares globales para la carga de datos y re-indexado por qsort()
int grado_original[MAX_VERTICES];
int order[MAX_VERTICES];
int nuevo_id[MAX_VERTICES];
int pos_actual[MAX_VERTICES];

// Struct auxiliar para ordenar las listas de adyacencia de cada nodo de forma descendente
struct AristaAux {
    int destino;
    int tipo;
};
struct AristaAux aux_sort[50000];

// --- VARIABLES GLOBALES DE BÚSQUEDA ---
unsigned long long int nexpansions = 0;
long long int type[4][4][4]; // Matriz tridimensional para clasificar subgrafos

// Cola estática de enteros planos y estado en cola
int queue[MAX_VERTICES];
int front = 0;
int rear = 0;
char in_queue[MAX_VERTICES]; // 1 si el nodo está en la cola (vecino del hub)

void clear_queue() { front = 0; rear = 0; }
int empty_queue() { return (rear >= front); }
void queue_insert(int node_id) { 
    queue[front++] = node_id; 
    in_queue[node_id] = 1; // Marcamos que está en la cola
}
int queue_pop() { return queue[rear++]; }

// Arreglos paralelos para el estado de búsqueda de cada nodo
short color[MAX_VERTICES];
unsigned iter[MAX_VERTICES];
short tpc[MAX_VERTICES];

// Comparador para qsort: ordena de forma descendente según grado_original
int compararNodos(const void* a, const void* b) {
    int nodoA = *(const int*)a;
    int nodoB = *(const int*)b;
    return grado_original[nodoB] - grado_original[nodoA];
}

// Comparador para ordenar vecinos de forma descendente por ID de destino
int compararAristasAux(const void* a, const void* b) {
    const struct AristaAux* edgeA = (const struct AristaAux*)a;
    const struct AristaAux* edgeB = (const struct AristaAux*)b;
    return edgeB->destino - edgeA->destino;
}

// Parser de enteros rápido: misma lógica que FCYV-2, sin overhead de fscanf.
// Avanza el puntero p saltando no-dígitos, luego lee dígitos.
static inline int parse_int(const char **p, const char *end) {
    while (*p < end && (**p < '0' || **p > '9')) (*p)++;
    int x = 0;
    while (*p < end && **p >= '0' && **p <= '9') {
        x = x * 10 + (**p - '0');
        (*p)++;
    }
    return x;
}

// -----------------------------------------------------------------------
// PASADA 1 + 2: mmap + parser manual (igual que FCYV-2).
// I/O + re-indexado por grado. El sort de vecinos queda
// para reorder_by_degree() (que se cronometra por separado).
// -----------------------------------------------------------------------
void ReadGraph(const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error: No se pudo abrir el archivo %s.\n", filename);
        exit(1);
    }

    struct stat st;
    fstat(fd, &st);
    size_t fsize = (size_t)st.st_size;

    // Mapear el archivo completo en memoria del SO en una sola syscall
    char *base = (char *)mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) { perror("mmap"); exit(1); }
    madvise(base, fsize, MADV_SEQUENTIAL); // Avisa al OS que leeremos en orden
    close(fd);

    const char *p   = base;
    const char *end = base + fsize;

    int file_vertices = parse_int(&p, end);
    int file_aristas  = parse_int(&p, end);

    if (file_vertices > MAX_VERTICES) {
        fprintf(stderr, "Error: El grafo excede el límite máximo de vértices (%d)\n", MAX_VERTICES);
        munmap(base, fsize);
        exit(1);
    }

    numVertices = file_vertices;

    // Inicializar grados en cero
    for (int i = 0; i < numVertices; i++) {
        grado_original[i] = 0;
        order[i] = i;
    }

    // Guardar posición justo después del encabezado para la PASADA 2
    const char *data_start = p;

    // PASADA 1: Contar grado de salida de cada nodo
    for (int i = 0; i < file_aristas && p < end; i++) {
        int u = parse_int(&p, end) - 1;
        int v = parse_int(&p, end) - 1;
        parse_int(&p, end); // consumir tipo (no necesario en pasada 1)
        if (u == v) continue;
        grado_original[u]++;
    }

    // Ordenar nodos por grado descendente y crear mapeo ID original -> nuevo ID
    qsort(order, numVertices, sizeof(int), compararNodos);
    for (int i = 0; i < numVertices; i++) {
        nuevo_id[order[i]] = i;
    }

    // Construir offsets del CSR en el nuevo orden
    g.cabeza[0] = 0;
    for (int i = 0; i < numVertices; i++) {
        int orig = order[i];
        g.grado[i]    = grado_original[orig];
        g.cabeza[i+1] = g.cabeza[i] + g.grado[i];
        pos_actual[i] = g.cabeza[i];
    }

    // PASADA 2: Re-leer aristas con los nuevos IDs (desde el puntero guardado)
    p = data_start;
    numAristas = 0;
    for (int i = 0; i < file_aristas && p < end; i++) {
        int u = parse_int(&p, end) - 1;
        int v = parse_int(&p, end) - 1;
        int t = parse_int(&p, end);
        if (u == v) continue;

        int u_new = nuevo_id[u];
        int v_new = nuevo_id[v];
        int idx = pos_actual[u_new]++;
        g.destino[idx] = v_new;
        g.tipo[idx]    = t;
        numAristas++;
    }

    munmap(base, fsize);
}



// -----------------------------------------------------------------------
// REORDEN: Ordena los vecinos de cada nodo en forma descendente.
// Esto habilita la poda temprana (early break) en search_motif.
// Se cronometra por separado, igual que en FCYV-2.
// -----------------------------------------------------------------------
void reorder_by_degree() {
    for (int i = 0; i < numVertices; i++) {
        int start = g.cabeza[i];
        int len   = g.grado[i];
        if (len < 2) continue;

        for (int k = 0; k < len; k++) {
            aux_sort[k].destino = g.destino[start + k];
            aux_sort[k].tipo    = g.tipo[start + k];
        }
        qsort(aux_sort, len, sizeof(struct AristaAux), compararAristasAux);
        for (int k = 0; k < len; k++) {
            g.destino[start + k] = aux_sort[k].destino;
            g.tipo[start + k]    = aux_sort[k].tipo;
        }
    }
}

// Muestra el grafo leído por pantalla de forma limpia
void mostrarGrafo() {
    int max_print = (numVertices > 20) ? 10 : numVertices;
    printf("\n--- Estructura del Grafo (mostrando primeros %d vértices de %d) ---\n", max_print, numVertices);
    
    for (int i = 0; i < max_print; i++) {
        printf("Vértice %d (Grado: %d) -> Vecinos: [", i, g.grado[i]);
        
        int start = g.cabeza[i];
        int end = g.cabeza[i + 1];
        for (int e = start; e < end; e++) {
            printf(" %d(tipo:%d)", g.destino[e], g.tipo[e]);
        }
        printf(" ]\n");
    }
    
    if (numVertices > 20) {
        printf("... (se omiten los restantes %d vértices para no saturar la pantalla) ...\n", numVertices - 10);
    }
}

// --- FUNCIONES AUXILIARES PARA EL CENSO DE MOTIVOS ---

void print_types() {
    long long total = 0;
    printf("\n--- Resultados del Censo de Motivos ---\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                if (type[i][j][k] != 0) {
                    printf("[%d][%d][%d] : %lld\n", i, j, k, type[i][j][k]);
                    total += type[i][j][k];
                }
            }
        }
    }
    printf("Total subgrafos conexos de 3 nodos: %lld\n", total);
}

long long comb2(int n) {
    if (n < 2) return 0;
    return ((long long)n * (n - 1)) >> 1;
}

// Núcleo de búsqueda: explora los motivos de 3 nodos conexos desde un hub
void search_motif(int hub, int iter_val) {
    int n1 = 0, n2 = 0, n3 = 0;
    color[hub] = 1; // 1 significa red (visitado)

    // 1. Contar vecinos del hub con n > hub
    int start = g.cabeza[hub];
    int end = g.cabeza[hub + 1];
    for (int i = start; i < end; i++) {
        int n = g.destino[i];
        if (n <= hub) break; // PODA TEMPRANA: Al estar ordenado descendente, todos los siguientes serán <= hub!

        nexpansions++;
        tpc[n] = g.tipo[i];
        if (tpc[n] == 1) n1++;
        if (tpc[n] == 2) n2++;
        if (tpc[n] == 3) n3++;
        queue_insert(n); // Llama internamente a in_queue[n] = 1
    }

    // Combinatoria cerrada simplificada para los abanicos (fans)
    type[1][0][1] += comb2(n1);
    type[2][0][2] += comb2(n2);
    type[3][0][3] += comb2(n3);
    type[1][0][2] += (long long)n1 * n2;
    type[1][0][3] += (long long)n1 * n3;
    type[2][0][3] += (long long)n2 * n3;

    // 2. Caminos y triángulos conexos
    while (!empty_queue()) {
        int s = queue_pop();
        iter[s] = iter_val;

        int s_start = g.cabeza[s];
        int s_end = g.cabeza[s + 1];
        for (int i = s_start; i < s_end; i++) {
            int n = g.destino[i];
            if (n <= hub) break; // PODA TEMPRANA: Al estar ordenado descendente, todos los siguientes serán <= hub!

            if (iter[n] != iter_val) {
                nexpansions++;
                if (in_queue[n]) { // Triángulo hub--s--n (está en la cola)
                    int ta = tpc[s];
                    int tb = tpc[n];
                    if (ta > tb) { int tmp = ta; ta = tb; tb = tmp; }
                    type[ta][0][tb]--;
                    type[tpc[s]][g.tipo[i]][tpc[n]]++;
                } else { // Camino abierto hub->s->n
                    type[tpc[s]][g.tipo[i]][0]++;
                }
            }
        }
    }

    // 3. Limpieza de in_queue para el siguiente hub
    for (int i = 0; i < front; i++) {
        in_queue[queue[i]] = 0;
    }
    clear_queue();
}

void search_motif_driver() {
    memset(type, 0, sizeof(type));
    nexpansions = 0;
    
    // Inicializar estado de los nodos
    for (int i = 0; i < numVertices; i++) {
        color[i] = 0;
        iter[i] = 0;
        in_queue[i] = 0;
        tpc[i] = 0;
    }

    // Como los nodos se renombraron físicamente en la carga de mayor a menor grado,
    // el orden de visita como hub es simplemente la secuencia ordenada de enteros 0..V-1.
    int iter_val = 1;
    for (int i = 0; i < numVertices; i++) {
        search_motif(i, iter_val);
        iter_val++;
    }
}

// Helper: diferencia entre dos timespec en segundos (igual que FCYV-2)
static double elapsed(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    printf("Leyendo el grafo desde: %s ...\n", filename);
    ReadGraph(filename);
    printf("Lectura completada. Vértices leídos: %d, Aristas válidas añadidas: %d\n",
           numVertices, numAristas);

    // Cronometrar reorden + búsqueda por separado, igual que FCYV-2
    struct timespec t0, t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    reorder_by_degree();
    clock_gettime(CLOCK_MONOTONIC, &t1);
    search_motif_driver();
    clock_gettime(CLOCK_MONOTONIC, &t2);

    print_types();
    printf("nexpansions: %llu\n", nexpansions);
    printf("Tiempo reorden : %.6f segundos\n", elapsed(t0, t1));
    printf("Tiempo busqueda: %.6f segundos\n", elapsed(t1, t2));
    printf("Tiempo total   : %.6f segundos\n", elapsed(t0, t2));

    return 0;
}
