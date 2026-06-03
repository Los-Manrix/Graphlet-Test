/*
 * BM-Triad-v2: Batagelj-Mrvar con búsquedas O(1)
 *
 * Tres optimizaciones sobre BM-Triad (que usaba binary_search):
 *
 *   1. link_out(v,w) y link_out(w,v) → O(1) via arrays de marcado con
 *      generación (mismo truco que FCYV-2 con hub_gen[]).
 *      Se mantienen 4 arrays:
 *        out_mark_v[w]==cur_v  ↔  v→w   (marcado al inicio del outer loop)
 *        in_mark_v[w] ==cur_v  ↔  w→v   (ídem)
 *        out_mark_u[w]==cur_u  ↔  u→w   (marcado al inicio del inner loop)
 *        in_mark_u[w] ==cur_u  ↔  w→u   (ídem)
 *      Tricode pasa de 6 binary_search a 6 comparaciones de enteros.
 *
 *   2. sym_link(v,w) → O(1) derivado de out_mark_v y in_mark_v,
 *      sin binary_search en la condición canónica del paso 2.1.4.
 *
 *   3. S = R̂(u)∪R̂(v)\{u,v} sin buffer explícito: se itera directamente
 *      sobre sym_adj[u] (Fase A) y luego sobre sym_adj[v] saltando los
 *      elementos ya vistos en Fase A via sym_link_u() O(1). Se elimina
 *      el merge + escritura al buffer intermedio.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

using namespace std;

/* ── Tabla de tipos (Tabla 1 del paper) ─────────────────────────────── */
static const int TRI_TYPE[64] = {
     1,  2,  2,  3,  2,  4,  6,  8,
     2,  6,  5,  7,  3,  8,  7, 11,
     2,  6,  4,  8,  5,  9,  9, 13,
     6, 10,  9, 14,  7, 14, 12, 15,
     2,  5,  6,  7,  6,  9, 10, 14,
     4,  9,  9, 12,  8, 13, 14, 15,
     3,  7,  8, 11,  7, 12, 14, 15,
     8, 14, 13, 15, 11, 15, 15, 16
};

static const char *TRI_NAME[17] = {
    "", "003","012","102","021D","021U","021C",
    "111D","111U","030T","030C","201",
    "120D","120U","120C","210","300"
};

/* ── Grafo ───────────────────────────────────────────────────────────── */
#define MAX_N 50001

static int N;
static vector<int> out_adj[MAX_N]; /* sucesores de v          */
static vector<int> in_adj [MAX_N]; /* predecesores de v       */
static vector<int> sym_adj[MAX_N]; /* R̂(v) = out ∪ in        */

/* ── Arrays de marcado con generación ───────────────────────────────── */
/*
 * cur_v se incrementa una vez por cada vértice del outer loop.
 * cur_u se incrementa una vez por cada par (v,u) del inner loop.
 *
 * Invariante al procesar el par (v,u):
 *   out_mark_v[w] == cur_v  ↔  existe arco v→w
 *   in_mark_v[w]  == cur_v  ↔  existe arco w→v
 *   out_mark_u[w] == cur_u  ↔  existe arco u→w
 *   in_mark_u[w]  == cur_u  ↔  existe arco w→u
 */
static unsigned out_mark_v[MAX_N];
static unsigned in_mark_v [MAX_N];
static unsigned out_mark_u[MAX_N];
static unsigned in_mark_u [MAX_N];

static unsigned cur_v = 0;
static unsigned cur_u = 0;

/* ── Consultas O(1) ─────────────────────────────────────────────────── */

/* Tricode(v,u,w): 6 bits de presencia/dirección de aristas en la tripla.
 * Fórmula del paper sección 2.3 — aquí sin binary_search. */
static inline int tricode_fast(int v, int u, int w)
{
    return  (int)(out_mark_v[u] == cur_v)          /* bit 0: v→u */
         | ((int)(out_mark_u[v] == cur_u) << 1)    /* bit 1: u→v */
         | ((int)(out_mark_v[w] == cur_v) << 2)    /* bit 2: v→w */
         | ((int)(in_mark_v[w]  == cur_v) << 3)    /* bit 3: w→v */
         | ((int)(out_mark_u[w] == cur_u) << 4)    /* bit 4: u→w */
         | ((int)(in_mark_u[w]  == cur_u) << 5);   /* bit 5: w→u */
}

/* ¿w es vecino de v en cualquier dirección? (paso 2.1.4: condición ¬vR̂w) */
static inline bool sym_link_v(int w)
{
    return (out_mark_v[w] == cur_v) || (in_mark_v[w] == cur_v);
}

/* ¿w es vecino de u en cualquier dirección? (dedup Fase B) */
static inline bool sym_link_u(int w)
{
    return (out_mark_u[w] == cur_u) || (in_mark_u[w] == cur_u);
}

/* ── Lectura ─────────────────────────────────────────────────────────── */
static void read_graph(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "No se puede abrir %s\n", filename); exit(1); }

    int m = 0;
    fscanf(f, "%d %d", &N, &m);

    char line[256];
    fgets(line, sizeof(line), f);

    int arc_count = 0;
    for (int i = 0; i < m; i++) {
        if (!fgets(line, sizeof(line), f)) break;
        int src, dst;
        if (sscanf(line, "%d %d", &src, &dst) < 2) { i--; continue; }
        src--; dst--;
        if (src < 0 || dst < 0 || src >= N || dst >= N || src == dst) continue;

        out_adj[src].push_back(dst);
        in_adj [dst].push_back(src);   /* ← necesario para in_mark */
        sym_adj[src].push_back(dst);
        sym_adj[dst].push_back(src);
        arc_count++;
    }
    fclose(f);

    for (int i = 0; i < N; i++) {
        sort(out_adj[i].begin(), out_adj[i].end());
        out_adj[i].erase(unique(out_adj[i].begin(), out_adj[i].end()), out_adj[i].end());
        sort(in_adj[i].begin(), in_adj[i].end());
        in_adj[i].erase(unique(in_adj[i].begin(), in_adj[i].end()), in_adj[i].end());
        sort(sym_adj[i].begin(), sym_adj[i].end());
        sym_adj[i].erase(unique(sym_adj[i].begin(), sym_adj[i].end()), sym_adj[i].end());
    }

    printf("Vértices: %d   Arcos válidos: %d\n", N, arc_count);
}

/* ── Algoritmo de Batagelj-Mrvar ─────────────────────────────────────
 *
 * Estructura idéntica al paper (sección 2.4); solo cambian los
 * mecanismos internos de consulta y la construcción de S.
 * ──────────────────────────────────────────────────────────────────── */
static long long Census[17];

static void triad_census(void)
{
    memset(Census,    0, sizeof(Census));
    memset(out_mark_v,0, sizeof(out_mark_v));
    memset(in_mark_v, 0, sizeof(in_mark_v));
    memset(out_mark_u,0, sizeof(out_mark_u));
    memset(in_mark_u, 0, sizeof(in_mark_u));
    cur_v = cur_u = 0;

    /* paso 2: for each v ∈ V */
    for (int v = 0; v < N; v++) {

        /* ── Marcar vecinos de v (una vez por outer loop) ──────────
         * Optimización 1+2: establece los invariantes de cur_v.    */
        ++cur_v;
        for (int w : out_adj[v]) out_mark_v[w] = cur_v;
        for (int w : in_adj[v])  in_mark_v[w]  = cur_v;

        const vector<int> &sv = sym_adj[v];

        /* paso 2.1: for each u ∈ R̂(v), if v < u */
        for (int u : sv) {
            if (u <= v) continue;

            /* ── Marcar vecinos de u (una vez por par (v,u)) ───────
             * Optimización 1: establece los invariantes de cur_u.  */
            ++cur_u;
            for (int w : out_adj[u]) out_mark_u[w] = cur_u;
            for (int w : in_adj[u])  in_mark_u[w]  = cur_u;

            /* ── paso 2.1.2: tipo de díada — O(1) ──────────────── */
            const int dtype =
                ((out_mark_v[u] == cur_v) && (out_mark_u[v] == cur_u)) ? 3 : 2;

            /* ── paso 2.1.1+2.1.4: S sin buffer explícito ──────────
             *
             * Optimización 3: en lugar de construir S = R̂(u)∪R̂(v)\{u,v}
             * como merge explícito, se itera en dos fases directamente
             * sobre las listas de vecinos.
             *
             * Fase A — w ∈ R̂(u) \ {v,u}:
             *   Todos estos w están en S. Condición canónica completa.
             *
             * Fase B — w ∈ R̂(v) \ (R̂(u) ∪ {u,v}):
             *   w ∈ R̂(v) ⟹ sym_link(v,w)=true ⟹ ¬sym_link(v,w)=false.
             *   La condición canónica se reduce a w > u.
             *   Dedup: se salta w si ya está en R̂(u) (sym_link_u O(1)).
             * ──────────────────────────────────────────────────── */
            int sz_S = 0;

            /* Fase A */
            for (int w : sym_adj[u]) {
                if (w == v || w == u) continue;
                ++sz_S;
                const bool in_Rv = sym_link_v(w);          /* O(1) opt.2 */
                if (w > u || (w > v && !in_Rv)) {
                    Census[TRI_TYPE[tricode_fast(v, u, w)]]++; /* O(1) opt.1 */
                }
            }

            /* Fase B */
            for (int w : sv) {
                if (w == u || w == v) continue;
                if (sym_link_u(w)) continue;               /* O(1) dedup */
                ++sz_S;
                if (w > u) {
                    Census[TRI_TYPE[tricode_fast(v, u, w)]]++;
                }
            }

            /* ── paso 2.1.3: díadas/nulas ─────────────────────── */
            Census[dtype] += (long long)(N - sz_S - 2);
        }
    }

    /* ── paso 3: triadas nulas 1-003 ── */
    long long sum = 0;
    for (int i = 2; i <= 16; i++) sum += Census[i];
    Census[1] = (long long)N * (N - 1) * (N - 2) / 6 - sum;
}

/* ── Main ─────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }

    read_graph(argv[1]);

    clock_t t0 = clock();
    triad_census();
    clock_t t1 = clock();

    printf("\nTriad Census:\n");
    long long total = 0;
    for (int i = 1; i <= 16; i++) {
        printf("  %2d - %-5s : %lld\n", i, TRI_NAME[i], Census[i]);
        total += Census[i];
    }

    const long long cn3 = (long long)N * (N - 1) * (N - 2) / 6;
    printf("\nTotal contado : %lld\n", total);
    printf("C(N,3)        : %lld\n",   cn3);
    printf("Diferencia    : %lld  (debe ser 0)\n", cn3 - total);
    printf("Tiempo        : %.6f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);

    return 0;
}
