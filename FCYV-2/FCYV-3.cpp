/*
 * FCYV-3.cpp — versión C++ single-thread con poda por orden descendente de adyacencias.
 *
 * Census de motivos conexos de 3 nodos en grafos dirigidos con aristas
 * etiquetadas por tipo (1,2,3), clasificados en la tabla type[a][b][c].
 *
 * Optimizaciones incluidas:
 *   1. Reetiquetado por grado descendente (counting sort).
 *   2. Adyacencias ordenadas de forma descendente por el nuevo ID del vecino.
 *   3. Poda por Early Break (romper loops al encontrar vecinos <= hub).
 *   4. Salto O(1) de vecinos si el vecino máximo es <= hub.
 *   5. CSR con aristas empaquetadas (dst<<2 | type).
 *   6. Marcado por generación (g).
 *   7. Lectura con mmap + parser propio.
 */

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <algorithm>
#include <functional>

struct NodeState {
    uint32_t hub_gen;   /* == g  si el nodo es sucesor directo del hub actual  */
    uint32_t iter;      /* == g  si ya fue extraído/procesado en este hub      */
    uint8_t  tpc;       /* tipo de la arista hub->nodo (válido si hub_gen==g)  */
    uint8_t  _pad[3];
};

#define PREFETCH_DIST 8

static int                V = 0;
static uint32_t  *__restrict__ adj = nullptr;       /* aristas empaquetadas CSR */
static int64_t   *__restrict__ adj_start = nullptr; /* offsets CSR, tamaño V+1  */
static NodeState *__restrict__ ns = nullptr;        /* estado por nodo          */
static int       *__restrict__ succ_buf = nullptr;  /* cola = sucesores del hub */

static long long          type_count[4][4][4];
static unsigned long long nexpansions = 0;

static inline long long comb2(long long n)
{
    return n < 2 ? 0 : (n * (n - 1)) >> 1;
}

static inline uint32_t parse_uint(const char *__restrict__ &p, const char *end)
{
    while (p < end && (*p < '0' || *p > '9')) ++p;
    uint32_t x = 0;
    while (p < end && *p >= '0' && *p <= '9') {
        x = x * 10u + (uint32_t)(*p - '0');
        ++p;
    }
    return x;
}

static void read_graph(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { fprintf(stderr, "No se puede abrir %s\n", filename); exit(1); }

    struct stat st;
    if (fstat(fd, &st) != 0) { perror("fstat"); exit(1); }
    size_t fsize = (size_t)st.st_size;

    char *base = (char *)mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) { perror("mmap"); exit(1); }
    madvise(base, fsize, MADV_SEQUENTIAL);

    const char *p   = base;
    const char *end = base + fsize;

    V          = (int)parse_uint(p, end);
    uint32_t M = parse_uint(p, end);
    const char *data_start = p;

    // Detectar base de indexación (0-based vs 1-based) en una muestra del archivo
    uint32_t offset = 1; // 1-based por defecto
    const char *scan_p = data_start;
    for (uint32_t i = 0; i < M && scan_p < end; ++i) {
        uint32_t src = parse_uint(scan_p, end);
        uint32_t dst = parse_uint(scan_p, end);
        parse_uint(scan_p, end);
        if (src == 0 || dst == 0) {
            offset = 0;
            break;
        }
    }
    printf("Indexacion detectada: %d-based\n", offset == 0 ? 0 : 1);

    adj_start = (int64_t *)calloc((size_t)V + 1, sizeof(int64_t));
    ns        = (NodeState *)calloc((size_t)V, sizeof(NodeState));
    succ_buf  = (int *)malloc((size_t)V * sizeof(int));
    if (!adj_start || !ns || !succ_buf) { fprintf(stderr, "calloc\n"); exit(1); }

    int64_t *outdeg = adj_start + 1;
    uint32_t valid = 0;
    p = data_start;
    for (uint32_t i = 0; i < M && p < end; ++i) {
        uint32_t src = parse_uint(p, end);
        uint32_t dst = parse_uint(p, end);
        parse_uint(p, end);
        if (src == dst) continue;
        if (src - offset >= (uint32_t)V) {
            fprintf(stderr, "Error: ID de origen %u fuera de rango para V=%d\n", src, V);
            exit(1);
        }
        ++outdeg[src - offset];
        ++valid;
    }

    for (int i = 0; i < V; ++i) adj_start[i + 1] += adj_start[i];

    adj = (uint32_t *)malloc((size_t)valid * sizeof(uint32_t));
    if (!adj) { fprintf(stderr, "malloc adj\n"); exit(1); }

    int64_t *fill = (int64_t *)malloc((size_t)V * sizeof(int64_t));
    memcpy(fill, adj_start, (size_t)V * sizeof(int64_t));
    p = data_start;
    for (uint32_t i = 0; i < M && p < end; ++i) {
        uint32_t src = parse_uint(p, end);
        uint32_t dst = parse_uint(p, end);
        uint32_t t   = parse_uint(p, end);
        if (src == dst) continue;
        src -= offset;
        dst -= offset;
        if (src >= (uint32_t)V || dst >= (uint32_t)V) {
            fprintf(stderr, "Error: ID fuera de rango en segunda pasada (src=%u, dst=%u, V=%d)\n", src, dst, V);
            exit(1);
        }
        adj[fill[src]++] = (dst << 2) | (t & 3u);
    }
    free(fill);

    munmap(base, fsize);
    close(fd);

    printf("%d %u\n", V, valid);
}

static void reorder_by_degree()
{
    const int64_t m = adj_start[V];

    int *udeg = (int *)calloc((size_t)V, sizeof(int));
    for (int u = 0; u < V; ++u)
        for (int64_t k = adj_start[u]; k < adj_start[u + 1]; ++k) {
            const int v = (int)(adj[k] >> 2);
            ++udeg[u];
            ++udeg[v];
        }

    int md = 0;
    for (int u = 0; u < V; ++u) if (udeg[u] > md) md = udeg[u];

    int *fillp = (int *)calloc((size_t)md + 1, sizeof(int));
    for (int u = 0; u < V; ++u) ++fillp[udeg[u]];
    int pos = 0;
    for (int d = md; d >= 0; --d) { const int c = fillp[d]; fillp[d] = pos; pos += c; }

    int *newid = (int *)malloc((size_t)V * sizeof(int));
    for (int u = 0; u < V; ++u) newid[u] = fillp[udeg[u]]++;

    free(udeg);
    free(fillp);

    int64_t *nstart = (int64_t *)calloc((size_t)V + 1, sizeof(int64_t));
    for (int u = 0; u < V; ++u)
        nstart[newid[u] + 1] = adj_start[u + 1] - adj_start[u];
    for (int u = 0; u < V; ++u) nstart[u + 1] += nstart[u];

    uint32_t *nadj  = (uint32_t *)malloc((size_t)m * sizeof(uint32_t));
    int64_t  *nfill = (int64_t *)malloc((size_t)V * sizeof(int64_t));
    memcpy(nfill, nstart, (size_t)V * sizeof(int64_t));
    for (int u = 0; u < V; ++u) {
        const int nu = newid[u];
        for (int64_t k = adj_start[u]; k < adj_start[u + 1]; ++k) {
            const uint32_t e = adj[k];
            const int olddst = (int)(e >> 2);
            const uint32_t t = e & 3u;
            nadj[nfill[nu]++] = ((uint32_t)newid[olddst] << 2) | t;
        }
    }
    free(nfill);
    free(newid);

    // NUEVO: Ordenar los vecinos de cada nodo de forma descendente por su nuevo ID
    for (int nu = 0; nu < V; ++nu) {
        std::sort(nadj + nstart[nu], nadj + nstart[nu + 1], std::greater<uint32_t>());
    }

    free(adj);       adj = nadj;
    free(adj_start); adj_start = nstart;
}

static inline void search_motif(int hub, uint32_t g)
{
    const int64_t b = adj_start[hub];
    const int64_t e = adj_start[hub + 1];

    int n1 = 0, n2 = 0, n3 = 0;
    int qn = 0;

    for (int64_t k = b; k < e; ++k) {
        const uint32_t edge = adj[k];
        const int dst = (int)(edge >> 2);
        if (dst <= hub) break; // PODA: como está ordenado descendente, todos los siguientes son <= hub
        const int t = (int)(edge & 3u);
        ++nexpansions;
        NodeState &x = ns[dst];
        x.hub_gen = g;
        x.tpc     = (uint8_t)t;
        if      (t == 1) ++n1;
        else if (t == 2) ++n2;
        else             ++n3;
        succ_buf[qn++] = dst;
    }

    type_count[1][0][1] += comb2(n1);
    type_count[2][0][2] += comb2(n2);
    type_count[3][0][3] += comb2(n3);
    type_count[1][0][2] += (long long)n1 * n2;
    type_count[1][0][3] += (long long)n1 * n3;
    type_count[2][0][3] += (long long)n2 * n3;

    for (int qi = 0; qi < qn; ++qi) {
        const int s = succ_buf[qi];
        ns[s].iter = g;
        const int ts = ns[s].tpc;

        const int64_t sb = adj_start[s];
        const int64_t se = adj_start[s + 1];

        // PODA O(1): si el vecino máximo de s es <= hub, todos sus vecinos lo son
        if (sb < se) {
            const int max_neighbor = (int)(adj[sb] >> 2);
            if (max_neighbor <= hub) continue;
        }

        for (int64_t k = sb; k < se; ++k) {
            if (k + PREFETCH_DIST < se)
                __builtin_prefetch(&ns[adj[k + PREFETCH_DIST] >> 2], 0, 1);

            const uint32_t edge = adj[k];
            const int n = (int)(edge >> 2);
            if (n <= hub) break; // PODA: si este es <= hub, todos los que siguen también lo son
            const NodeState &x = ns[n];
            if (x.iter == g) continue;
            const int tn = (int)(edge & 3u);
            ++nexpansions;
            if (x.hub_gen == g) {
                const int nt = x.tpc;
                int ta = ts, tb = nt;
                if (ta > tb) { const int tmp = ta; ta = tb; tb = tmp; }
                --type_count[ta][0][tb];
                ++type_count[ts][tn][nt];
            } else {
                ++type_count[ts][tn][0];
            }
        }
    }
}

static void search_motif_driver()
{
    memset(type_count, 0, sizeof(type_count));
    uint32_t g = 0;
    for (int hub = 0; hub < V; ++hub) {
        ++g;
        search_motif(hub, g);
    }
}

static void print_types()
{
    long long total = 0;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) {
                printf("[%d][%d][%d] : %lld\n", i, j, k, type_count[i][j][k]);
                total += type_count[i][j][k];
            }
    printf("Total subgrafos: %lld\n", total);
}

static double elapsed(const struct timespec &a, const struct timespec &b)
{
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) * 1e-9;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }

    read_graph(argv[1]);

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

    free(adj);
    free(adj_start);
    free(ns);
    free(succ_buf);
    return 0;
}
