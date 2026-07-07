#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_VERTICES 50000
#define MAX_ARISTAS  14000000

// --- STRUCT DE ARREGLOS (REPRESENTACIÓN DEL GRAFO) ---
struct Grafo {
    int cabeza[MAX_VERTICES];       // Guarda el índice de la primera arista de cada nodo
    int grado[MAX_VERTICES];        // Guarda el número de vecinos salientes de cada nodo
    int destino[MAX_ARISTAS];       // Guarda el nodo de destino de cada arista
    int siguiente[MAX_ARISTAS];     // Guarda el índice de la siguiente arista del mismo nodo
    int tipo[MAX_ARISTAS];          // Guarda el tipo de la arista (1, 2 o 3)
};

struct Grafo g; // Instancia única global del grafo

int numVertices = 0;
int numAristas = 0;

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
int order[MAX_VERTICES]; // Orden de visita de los nodos
int order_pos[MAX_VERTICES]; // Posición de visita de cada nodo (para optimizar n > hub)

// Inicializa los arreglos con valores por defecto
void inicializarGrafo(int vertices) {
    numVertices = vertices;
    numAristas = 0;
    
    for (int i = 0; i < vertices; i++) {
        g.cabeza[i] = -1; // -1 significa que el nodo no tiene aristas asignadas aún
        g.grado[i] = 0;   // El grado inicial de todo nodo es cero
    }
}

// Agrega una arista dirigida u -> v con tipo t
void agregarAristaDirigida(int u, int v, int t) {
    if (numAristas >= MAX_ARISTAS) {
        fprintf(stderr, "Error: Se excedió la capacidad máxima de aristas (%d)\n", MAX_ARISTAS);
        exit(1);
    }
    
    int ID_arista = numAristas; // Tomamos el índice libre actual
    
    g.destino[ID_arista] = v;
    g.tipo[ID_arista] = t;              // Guardamos el tipo de conexión (1, 2 o 3)
    g.siguiente[ID_arista] = g.cabeza[u]; // Apunta al índice de la que antes era la primera arista
    g.cabeza[u] = ID_arista;            // Colocamos esta nueva arista al frente de la lista
    
    g.grado[u] = g.grado[u] + 1;          // Incrementamos el grado del nodo origen
    numAristas = numAristas + 1;      // Pasamos al siguiente espacio disponible
}

// Lee el archivo del grafo (Formato P, 1-based)
void ReadGraph(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        printf("Error: No se pudo abrir el archivo %s.\n", filename);
        exit(1);
    }

    int file_vertices, file_aristas;
    if (fscanf(f, "%d %d\n", &file_vertices, &file_aristas) != 2) {
        fprintf(stderr, "Error: Formato de encabezado inválido.\n");
        fclose(f);
        exit(1);
    }

    if (file_vertices > MAX_VERTICES) {
        fprintf(stderr, "Error: El grafo excede el límite máximo de vértices (%d)\n", MAX_VERTICES);
        fclose(f);
        exit(1);
    }

    inicializarGrafo(file_vertices);

    int ori, dest, t;
    for (int i = 0; i < file_aristas; i++) {
        if (fscanf(f, "%d %d %d\n", &ori, &dest, &t) != 3) {
            break;
        }
        
        // Ajustamos la base 1 del archivo a la base 0 del lenguaje C (-1)
        int u = ori - 1;
        int v = dest - 1;
        
        if (u == v) continue; // Evitamos self-loops (lazos al mismo nodo)
        
        agregarAristaDirigida(u, v, t);
    }
    fclose(f);
}

// Muestra el grafo leído por pantalla de forma limpia
/*
void mostrarGrafo() {
    int max_print = (numVertices > 20) ? 10 : numVertices;
    printf("\n--- Estructura del Grafo (mostrando primeros %d vértices de %d) ---\n", max_print, numVertices);
    
    for (int i = 0; i < max_print; i++) {
        printf("Vértice %d (Grado: %d) -> Vecinos: [", i, g.grado[i]);
        
        int e = g.cabeza[i];
        while (e != -1) {
            printf(" %d(tipo:%d)", g.destino[e], g.tipo[e]);
            e = g.siguiente[e];
        }
        printf(" ]\n");
    }
    
    if (numVertices > 20) {
        printf("... (se omiten los restantes %d vértices para no saturar la pantalla) ...\n", numVertices - 10);
    }
}

// --- FUNCIONES AUXILIARES PARA EL CENSO DE MOTIVOS ---

*/
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

    int hub_pos = order_pos[hub];

    // 1. Contar vecinos del hub con order_pos[n] > order_pos[hub]
    int e = g.cabeza[hub];
    while (e != -1) {
        int n = g.destino[e];
        if (order_pos[n] > hub_pos) {
            nexpansions++;
            tpc[n] = g.tipo[e];
            if (tpc[n] == 1) n1++;
            if (tpc[n] == 2) n2++;
            if (tpc[n] == 3) n3++;
            queue_insert(n); // Llama internamente a in_queue[n] = 1
        }
        e = g.siguiente[e];
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

        int e_s = g.cabeza[s];
        while (e_s != -1) {
            int n = g.destino[e_s];
            if (order_pos[n] > hub_pos && iter[n] != iter_val) {
                nexpansions++;
                if (in_queue[n]) { // Triángulo hub--s--n (está en la cola)
                    int ta = tpc[s];
                    int tb = tpc[n];
                    if (ta > tb) { int tmp = ta; ta = tb; tb = tmp; }
                    type[ta][0][tb]--;
                    type[tpc[s]][g.tipo[e_s]][tpc[n]]++;
                } else { // Camino abierto hub->s->n
                    type[tpc[s]][g.tipo[e_s]][0]++;
                }
            }
            e_s = g.siguiente[e_s];
        }
    }

    // 3. Limpieza de in_queue para el siguiente hub
    for (int i = 0; i < front; i++) {
        in_queue[queue[i]] = 0;
    }
    clear_queue();
}

// Funciones auxiliares para QuickSort (orden descendente según grado)
void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int partition(int arr[], int low, int high) {
    int pivot = g.grado[arr[high]];
    int i = (low - 1);
    
    for (int j = low; j <= high - 1; j++) {
        if (g.grado[arr[j]] > pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

void iniciar_types(){
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                type[i][j][k] = 0;
            }
        }
    }
}

void search_motif_driver() {
    iniciar_types();
    nexpansions = 0;
    
    // Inicializar estado de los nodos y el arreglo de orden de visita
    for (int i = 0; i < numVertices; i++) {
        color[i] = 0;
        iter[i] = 0;
        in_queue[i] = 0;
        tpc[i] = 0;
        order[i] = i;
    }

    // Ordenar nodos por grado descendente usando QuickSort
    quickSort(order, 0, numVertices - 1);

    // Poblar el mapeo de posiciones ordenadas de visita (order_pos)
    for (int i = 0; i < numVertices; i++) {
        order_pos[order[i]] = i;
    }

    int iter_val = 1;
    for (int i = 0; i < numVertices; i++) {
        int target_node = order[i];
        search_motif(target_node, iter_val);
        iter_val++;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    
    printf("Leyendo el grafo desde: %s ...\n", filename);
    ReadGraph(filename);
    
    printf("Lectura completada. Vértices leídos: %d, Aristas válidas añadidas: %d\n", numVertices, numAristas);
    
    //mostrarGrafo();
    
    // Censo de Motivos de 3 Nodos
    printf("\nEjecutando censo de motivos conexos de 3 nodos...\n");
    clock_t t_inicio = clock();
    search_motif_driver();
    clock_t t_fin = clock();
    
    print_types();
    printf("nexpansions: %llu\n", nexpansions);
    printf("Tiempo busqueda: %.6f segundos\n", (double)(t_fin - t_inicio) / CLOCKS_PER_SEC);
    
    return 0;
}
