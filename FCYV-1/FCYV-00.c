#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define MAX_NODES 50000
#define MAX_ARCS  14000000
#define MAX_QUEUE 50000
#define DEBUG 0
#define VERBOSE 0
#define DBG_PAUSE if(DEBUG) getchar()
#define VRB_PRINT(...) if(VERBOSE) printf(__VA_ARGS__)

unsigned long long int nexpansions = 0;

struct snode;
typedef struct snode snode;

struct snode
{
  int id;
  short color;
  unsigned iter;
  unsigned nsuccs;
  short tpc;
  snode *parent;
};

/* Grafo en formato CSR: adj_dest y adj_type almacenan los destinos y tipos
   de todas las aristas de forma contigua; adj_start[i] indica donde
   empiezan los sucesores del nodo i dentro de esos arrays. */
static int adj_dest[MAX_ARCS];
static int adj_type_arr[MAX_ARCS];
static int adj_start[MAX_NODES + 1];
static int V;

snode* searchNodes[MAX_NODES];
snode* queue[MAX_QUEUE];

int front = 0;
int rear  = 0;

void clear_queue() { front = 0; rear = 0; }
int  empty_queue() { return (rear >= front); }
void queue_insert(snode* node) { queue[front++] = node; }
snode* queue_pop()             { return queue[rear++]; }

snode* new_SearchNode(int id) {
    snode* node = (snode*)malloc(sizeof(snode));
    node->id     = id;
    node->color  = 0;
    node->iter   = 0;
    node->tpc    = 0;
    node->nsuccs = 0;
    node->parent = NULL;
    return node;
}

void ReadGraph(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) { printf("Cannot open file %s.\n", filename); exit(1); }

    int num_arcs;
    fscanf(f, "%d %d", &V, &num_arcs);
    fscanf(f, "\n");
    printf("%d %d %d\n", V, num_arcs, INT_MAX);
    DBG_PAUSE;

    /* Lectura temporal de aristas para luego construir el CSR */
    static int tmp_src[MAX_ARCS], tmp_dst[MAX_ARCS], tmp_t[MAX_ARCS];
    static int deg[MAX_NODES];
    memset(deg, 0, sizeof(int) * V);

    int valid = 0;
    for (int i = 0; i < num_arcs; i++) {
        int ori, dest, t;
        fscanf(f, "%d %d %d\n", &ori, &dest, &t);
        if (ori - 1 == dest - 1) continue;
        tmp_src[valid] = ori - 1;
        tmp_dst[valid] = dest - 1;
        tmp_t[valid]   = t;
        deg[ori - 1]++;
        valid++;
    }
    fclose(f);

    /* Construir adj_start a partir de los grados */
    adj_start[0] = 0;
    for (int i = 0; i < V; i++)
        adj_start[i + 1] = adj_start[i] + deg[i];

    /* Llenar adj_dest y adj_type_arr */
    static int fill[MAX_NODES];
    memcpy(fill, adj_start, sizeof(int) * V);
    for (int i = 0; i < valid; i++) {
        int pos          = fill[tmp_src[i]]++;
        adj_dest[pos]    = tmp_dst[i];
        adj_type_arr[pos] = tmp_t[i];
    }

    /* Inicializar nodos de búsqueda */
    for (int i = 0; i < V; i++) {
        searchNodes[i]         = new_SearchNode(i);
        searchNodes[i]->nsuccs = deg[i];
    }
}

long long int type[4][4][4];
void initialize_type() {
    int i, j, k;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            for (k = 0; k < 4; k++)
                type[i][j][k] = 0;
}

void print_types() {
    int i, j, k;
    long long int total = 0;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            for (k = 0; k < 4; k++) {
                printf("[%d][%d][%d] : %lld\n", i, j, k, type[i][j][k]);
                total += type[i][j][k];
            }
    printf("Total subgrafos: %lld\n", total);
}

long long comb2(int n) {
    if (n < 2) return 0;
    return ((long long)n * (n - 1)) >> 1;
}

void search_motif(snode* node, int iter) {
    int n1 = 0, n2 = 0, n3 = 0;
    node->parent = NULL;
    node->color  = 1;

    for (int i = adj_start[node->id]; i < adj_start[node->id + 1]; i++) {
        snode* succ = searchNodes[adj_dest[i]];
        if (succ->color != 1) {
            nexpansions++;
            succ->tpc    = adj_type_arr[i];
            succ->parent = node;
            if (succ->tpc == 1) n1++;
            if (succ->tpc == 2) n2++;
            if (succ->tpc == 3) n3++;
            queue_insert(succ);
        }
    }

    type[1][0][1] += comb2(n1);
    type[2][0][2] += comb2(n2);
    type[3][0][3] += comb2(n3);
    type[1][0][2] += comb2(n1 + n2) - comb2(n1) - comb2(n2);
    type[1][0][3] += comb2(n1 + n3) - comb2(n1) - comb2(n3);
    type[2][0][3] += comb2(n2 + n3) - comb2(n2) - comb2(n3);

    VRB_PRINT("n1:%d comb2(n1):%lld n2:%d n3:%d - %lld %lld %lld %lld %lld %lld\n",
              n1, comb2(n1), n2, n3,
              type[1][0][1], type[2][0][2], type[3][0][3],
              type[1][0][2], type[1][0][3], type[2][0][3]);
    DBG_PAUSE;

    while (!(empty_queue())) {
        snode* s = queue_pop();
        s->iter  = iter;
        VRB_PRINT("  Pop node %d\n", s->id);

        for (int i = adj_start[s->id]; i < adj_start[s->id + 1]; i++) {
            snode* n = searchNodes[adj_dest[i]];
            if (n->color != 1 && n->iter != iter) {
                nexpansions++;
                if (n->parent == s->parent) {
                    int ta = s->tpc, tb = n->tpc;
                    if (ta > tb) { int tmp = ta; ta = tb; tb = tmp; }
                    VRB_PRINT("Se resta 1 a [%d][%d][%d]\n", ta, 0, tb);
                    VRB_PRINT("Se agrega 1 a [%d][%d][%d]\n", s->tpc, adj_type_arr[i], n->tpc);
                    type[ta][0][tb]--;
                    type[s->tpc][adj_type_arr[i]][n->tpc]++;
                    DBG_PAUSE;
                } else {
                    type[s->tpc][adj_type_arr[i]][0]++;
                }
            }
        }
    }
}

void search_motif_driver() {
    int i, iter = 1;
    initialize_type();
    int maxnumbersucc = 0;
    for (i = 0; i < V; i++) {
        VRB_PRINT("%d: nsuccs:%d %lld\n", i, searchNodes[i]->nsuccs, type[1][0][1]);
        if (maxnumbersucc < (int)searchNodes[i]->nsuccs)
            maxnumbersucc = searchNodes[i]->nsuccs;
        search_motif(searchNodes[i], iter);
        iter++;
        clear_queue();
    }
    print_types();
    printf("maxnumbersucc:%d\n", maxnumbersucc);
}

int main(int argc, char *argv[]) {
    printf("comb2(1): %lld\n", comb2(1));
    DBG_PAUSE;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }

    ReadGraph(argv[1]);

    clock_t t_inicio = clock();
    search_motif_driver();
    clock_t t_fin = clock();

    printf("nexpansions: %llu\n", nexpansions);
    printf("Tiempo busqueda: %.6f segundos\n", (double)(t_fin - t_inicio) / CLOCKS_PER_SEC);
    return 0;
}
