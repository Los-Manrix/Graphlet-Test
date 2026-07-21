/*
 * ortmann-00: esta version (00) incluye un CSR para representar el grafo;
 * el resto del codigo es igual al de ortmangpt.cpp original.
 *
 * ortmann-00: variante de ortmangpt.cpp (Ortmann-Brandes Algorithm 2) que
 * reemplaza las seis estructuras vector<vector<AdjEdge>> (Gprime_adj,
 * out_arcs, in_arcs, mutual_arcs, Gprime_out, Gprime_in) por CSR (offsets +
 * un unico array de AdjEdge contiguo). Es el UNICO cambio: el algoritmo
 * (degeneracy ordering, listado de triangulos, sistema de ecuaciones,
 * tablas Table 1 / Fig. 8) queda exactamente igual. El codigo reemplazado
 * queda comentado, no borrado. Objetivo: medir si el overhead de Ortmann
 * frente a FCYV-2/FC3R-00 viene de la estructura de datos (vector de
 * vectores, cada uno con su propia asignacion de heap) o de otra parte.
 *
 * Paper:
 *   Mark Ortmann and Ulrik Brandes,
 *   "Efficient orbit-aware triad and quad census in directed and undirected graphs",
 *   Applied Network Science 2:13, 2017.
 */

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

using namespace std;

enum TriadId {
    T003 = 1, T012, T102, T021D, T021U, T021C, T111D, T111U,
    T030T, T030C, T201, T120D, T120U, T120C, T210, T300
};

static const char *TRI_NAME[17] = {
    "",
    "003", "012", "102", "021D", "021U", "021C",
    "111D", "111U", "030T", "030C", "201",
    "120D", "120U", "120C", "210", "300"
};

/*
 * Table 1 in the paper.
 * code(u,v,w) = l(u,v) + 2l(u,w) + 4l(v,u)
 *             + 8l(v,w) + 16l(w,u) + 32l(w,v).
 */
static const int CODE_TRIAD[64] = {
    T003, T012, T012, T021D, T012, T102,  T021C, T111U,
    T012, T021C,T021U,T030T,T021D,T111U,T030T,T120U,
    T012, T021C,T102, T111U,T021U,T111D,T111D,T201,
    T021C,T030C,T111D,T120C,T030T,T120C,T120D,T210,
    T012, T021U,T021C,T030T,T021C,T111D,T030C,T120C,
    T102, T111D,T111D,T120D,T111U,T201, T120C,T210,
    T021D,T030T,T111U,T120U,T030T,T120D,T120C,T210,
    T111U,T120C,T201, T210, T120U,T210, T210, T300
};

static const int CODE_ORBIT[64][3] = {
    { 0, 0, 0}, { 2, 3, 1}, { 2, 1, 3}, {11,12,12},
    { 3, 2, 1}, { 5, 5, 4}, { 6, 7, 8}, {14,15,13},
    { 1, 2, 3}, { 7, 6, 8}, {10,10, 9}, {23,24,22},
    {12,11,12}, {15,14,13}, {24,23,22}, {26,26,25},
    { 3, 1, 2}, { 6, 8, 7}, { 5, 4, 5}, {14,13,15},
    { 9,10,10}, {17,18,16}, {17,16,18}, {20,19,19},
    { 8, 7, 6}, {21,21,21}, {18,16,17}, {30,29,31},
    {22,23,24}, {31,30,29}, {28,27,28}, {33,32,34},
    { 1, 3, 2}, {10, 9,10}, { 7, 8, 6}, {23,22,24},
    { 8, 6, 7}, {18,17,16}, {21,21,21}, {30,31,29},
    { 4, 5, 5}, {16,17,18}, {16,18,17}, {27,28,28},
    {13,14,15}, {19,20,19}, {29,30,31}, {32,33,34},
    {12,12,11}, {24,22,23}, {15,13,14}, {26,25,26},
    {22,24,23}, {28,28,27}, {31,29,30}, {33,34,32},
    {13,15,14}, {29,31,30}, {19,19,20}, {32,34,33},
    {25,26,26}, {34,33,32}, {34,32,33}, {35,35,35}
};

struct DirectedArc {
    int src;
    int dst;
};

struct Edge {
    int a;
    int b;
    unsigned char mask; /* bit 0: a->b, bit 1: b->a */
};

struct AdjEdge {
    int to;
    int eid;
};

/* ── ORTMANN-00: CSR generico para reemplazar vector<vector<AdjEdge>> ──────
 * start[u]..start[u+1] delimita el rango de u dentro de 'data', un unico
 * array contiguo para TODO el grafo (en vez de un vector<AdjEdge> propio,
 * con su propia asignacion de heap, por cada nodo). */
struct CSR {
    vector<int64_t> start;  /* tamaño n_vertices+1 */
    vector<AdjEdge> data;   /* aristas empaquetadas, contiguas por nodo */
};

static void csr_build(CSR &c, int n, const vector<int> &deg)
{
    c.start.assign((size_t)n + 1, 0);
    for (int u = 0; u < n; ++u) c.start[u + 1] = c.start[u] + deg[u];
    c.data.resize((size_t)c.start[n]);
}

static int n_vertices;
static int m_arcs;
static int m_mutual;
static vector<Edge> edge_info;
// static vector<vector<AdjEdge> > Gprime_adj;   // ORTMANN-00: reemplazado por CSR
// static vector<vector<AdjEdge> > out_arcs;
// static vector<vector<AdjEdge> > in_arcs;
// static vector<vector<AdjEdge> > mutual_arcs;
static CSR csr_Gprime_adj, csr_out, csr_in, csr_mutual;

static vector<array<long long, 36> > nn_orbit;
static vector<array<long long, 36> > ni_orbit;

static vector<int> node_order;
static vector<int> order_rank;
// static vector<vector<AdjEdge> > Gprime_out;   // ORTMANN-00: reemplazado por CSR
// static vector<vector<AdjEdge> > Gprime_in;
static CSR csr_Gprime_out, csr_Gprime_in;

static int orbit_to_triad[36];
static int equation_coeff[36][36];
static long long Census[17];

/* Trabajo realizado: cada candidato w examinado en el listado de triangulos. */
static unsigned long long nexpansions = 0;

static inline long long comb2(long long x)
{
    return x < 2 ? 0 : x * (x - 1) / 2;
}

static inline long long comb3(long long x)
{
    return x < 3 ? 0 : x * (x - 1) * (x - 2) / 6;
}

static inline bool has_arc(const Edge &e, int from, int to)
{
    if (from == e.a && to == e.b) return (e.mask & 1) != 0;
    if (from == e.b && to == e.a) return (e.mask & 2) != 0;
    return false;
}

static inline int code_of_triad(int u, int v, int w,
                                int eid_uv, int eid_uw, int eid_vw)
{
    const Edge &uv = edge_info[eid_uv];
    const Edge &uw = edge_info[eid_uw];
    const Edge &vw = edge_info[eid_vw];
    int code = 0;
    if (has_arc(uv, u, v)) code |= 1;
    if (has_arc(uw, u, w)) code |= 2;
    if (has_arc(uv, v, u)) code |= 4;
    if (has_arc(vw, v, w)) code |= 8;
    if (has_arc(uw, w, u)) code |= 16;
    if (has_arc(vw, w, v)) code |= 32;
    return code;
}

static void initialize_table_1_and_figure_8()
{
    for (int i = 0; i < 36; i++) orbit_to_triad[i] = 0;
    for (int code = 0; code < 64; code++) {
        for (int p = 0; p < 3; p++) {
            orbit_to_triad[CODE_ORBIT[code][p]] = CODE_TRIAD[code];
        }
    }

    memset(equation_coeff, 0, sizeof(equation_coeff));

    /*
     * Fig. 8 is the containment system nn_i(u) = sum_j A[i][j] ni_j(u).
     * The coefficients are generated directly from Table 1: for each induced
     * triad code, enumerate all directed-edge subsets and record the orbit
     * of the same node in the non-induced subtriad.
     */
    int seen[36][36];
    memset(seen, 0, sizeof(seen));
    for (int full_code = 0; full_code < 64; full_code++) {
        for (int p = 0; p < 3; p++) {
            const int induced_orbit = CODE_ORBIT[full_code][p];
            int local_count[36];
            memset(local_count, 0, sizeof(local_count));

            int sub_code = full_code;
            while (true) {
                local_count[CODE_ORBIT[sub_code][p]]++;
                if (sub_code == 0) break;
                sub_code = (sub_code - 1) & full_code;
            }

            for (int non_induced_orbit = 0; non_induced_orbit < 36; non_induced_orbit++) {
                const int c = local_count[non_induced_orbit];
                if (c == 0) continue;
                if (!seen[non_induced_orbit][induced_orbit]) {
                    equation_coeff[non_induced_orbit][induced_orbit] = c;
                    seen[non_induced_orbit][induced_orbit] = 1;
                }
            }
        }
    }
}

static void read_graph(const char *filename, vector<DirectedArc> &arcs)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "No se puede abrir %s\n", filename);
        exit(1);
    }

    int input_m = 0;
    if (fscanf(f, "%d %d", &n_vertices, &input_m) != 2) {
        fprintf(stderr, "Cabecera invalida en %s\n", filename);
        exit(1);
    }

    arcs.clear();
    arcs.reserve(input_m);
    for (int i = 0; i < input_m; i++) {
        int src, dst, typ;
        const int got = fscanf(f, "%d %d %d", &src, &dst, &typ);
        if (got < 2) break;
        src--;
        dst--;
        if (src < 0 || dst < 0 || src >= n_vertices || dst >= n_vertices) continue;
        if (src == dst) continue;
        arcs.push_back(DirectedArc{src, dst});
    }

    fclose(f);
}

static void transform_G_to_underlying_undirected_graph(vector<DirectedArc> &arcs)
{
    sort(arcs.begin(), arcs.end(), [](const DirectedArc &x, const DirectedArc &y) {
        if (x.src != y.src) return x.src < y.src;
        return x.dst < y.dst;
    });
    arcs.erase(unique(arcs.begin(), arcs.end(), [](const DirectedArc &x, const DirectedArc &y) {
        return x.src == y.src && x.dst == y.dst;
    }), arcs.end());

    m_arcs = (int)arcs.size();

    vector<pair<pair<int, int>, DirectedArc> > grouped;
    grouped.reserve(arcs.size());
    for (const DirectedArc &arc : arcs) {
        const int a = min(arc.src, arc.dst);
        const int b = max(arc.src, arc.dst);
        grouped.push_back(make_pair(make_pair(a, b), arc));
    }
    sort(grouped.begin(), grouped.end(),
         [](const pair<pair<int, int>, DirectedArc> &x,
            const pair<pair<int, int>, DirectedArc> &y) {
             return x.first < y.first;
         });

    /* ── ORTMANN-00: version original con vector<vector<AdjEdge>>, comentada ──
    edge_info.clear();
    Gprime_adj.assign(n_vertices, vector<AdjEdge>());
    out_arcs.assign(n_vertices, vector<AdjEdge>());
    in_arcs.assign(n_vertices, vector<AdjEdge>());
    mutual_arcs.assign(n_vertices, vector<AdjEdge>());

    for (size_t i = 0; i < grouped.size(); ) {
        const int a = grouped[i].first.first;
        const int b = grouped[i].first.second;
        unsigned char mask = 0;
        size_t j = i;
        while (j < grouped.size() && grouped[j].first.first == a && grouped[j].first.second == b) {
            const DirectedArc &arc = grouped[j].second;
            mask |= (arc.src == a) ? 1 : 2;
            j++;
        }

        const int eid = (int)edge_info.size();
        edge_info.push_back(Edge{a, b, mask});
        Gprime_adj[a].push_back(AdjEdge{b, eid});
        Gprime_adj[b].push_back(AdjEdge{a, eid});

        if (mask & 1) {
            out_arcs[a].push_back(AdjEdge{b, eid});
            in_arcs[b].push_back(AdjEdge{a, eid});
        }
        if (mask & 2) {
            out_arcs[b].push_back(AdjEdge{a, eid});
            in_arcs[a].push_back(AdjEdge{b, eid});
        }
        if (mask == 3) {
            mutual_arcs[a].push_back(AdjEdge{b, eid});
            mutual_arcs[b].push_back(AdjEdge{a, eid});
        }

        i = j;
    }

    m_mutual = 0;
    for (const Edge &e : edge_info) {
        if (e.mask == 3) m_mutual++;
    }
    ── fin version original ── */

    /* ORTMANN-00: misma logica de agrupamiento/mascara que arriba, pero en
     * dos pasadas para volcar Gprime_adj/out_arcs/in_arcs/mutual_arcs como
     * CSR (conteo de grado -> prefix sum -> volcado con cursor) en vez de
     * vector<vector<AdjEdge>>. */
    edge_info.clear();
    vector<int> deg_adj(n_vertices, 0), deg_out(n_vertices, 0), deg_in(n_vertices, 0), deg_mutual(n_vertices, 0);

    for (size_t i = 0; i < grouped.size(); ) {
        const int a = grouped[i].first.first;
        const int b = grouped[i].first.second;
        unsigned char mask = 0;
        size_t j = i;
        while (j < grouped.size() && grouped[j].first.first == a && grouped[j].first.second == b) {
            const DirectedArc &arc = grouped[j].second;
            mask |= (arc.src == a) ? 1 : 2;
            j++;
        }

        edge_info.push_back(Edge{a, b, mask});
        ++deg_adj[a]; ++deg_adj[b];
        if (mask & 1) { ++deg_out[a]; ++deg_in[b]; }
        if (mask & 2) { ++deg_out[b]; ++deg_in[a]; }
        if (mask == 3) { ++deg_mutual[a]; ++deg_mutual[b]; }

        i = j;
    }

    csr_build(csr_Gprime_adj, n_vertices, deg_adj);
    csr_build(csr_out,        n_vertices, deg_out);
    csr_build(csr_in,         n_vertices, deg_in);
    csr_build(csr_mutual,     n_vertices, deg_mutual);

    vector<int64_t> cur_adj(csr_Gprime_adj.start.begin(), csr_Gprime_adj.start.begin() + n_vertices);
    vector<int64_t> cur_out(csr_out.start.begin(),        csr_out.start.begin()        + n_vertices);
    vector<int64_t> cur_in(csr_in.start.begin(),          csr_in.start.begin()         + n_vertices);
    vector<int64_t> cur_mutual(csr_mutual.start.begin(),  csr_mutual.start.begin()     + n_vertices);

    for (int eid = 0; eid < (int)edge_info.size(); ++eid) {
        const Edge &e = edge_info[eid];
        const int a = e.a, b = e.b;
        const unsigned char mask = e.mask;

        csr_Gprime_adj.data[cur_adj[a]++] = AdjEdge{b, eid};
        csr_Gprime_adj.data[cur_adj[b]++] = AdjEdge{a, eid};

        if (mask & 1) {
            csr_out.data[cur_out[a]++] = AdjEdge{b, eid};
            csr_in.data[cur_in[b]++]   = AdjEdge{a, eid};
        }
        if (mask & 2) {
            csr_out.data[cur_out[b]++] = AdjEdge{a, eid};
            csr_in.data[cur_in[a]++]   = AdjEdge{b, eid};
        }
        if (mask == 3) {
            csr_mutual.data[cur_mutual[a]++] = AdjEdge{b, eid};
            csr_mutual.data[cur_mutual[b]++] = AdjEdge{a, eid};
        }
    }

    m_mutual = 0;
    for (const Edge &e : edge_info) {
        if (e.mask == 3) m_mutual++;
    }
}

static void calculate_n0_to_n20()
{
    nn_orbit.assign(n_vertices, array<long long, 36>());
    ni_orbit.assign(n_vertices, array<long long, 36>());
    for (int u = 0; u < n_vertices; u++) {
        nn_orbit[u].fill(0);
        ni_orbit[u].fill(0);
    }

    for (int u = 0; u < n_vertices; u++) {
        /* ── ORTMANN-00: version original (vector<vector<AdjEdge>>), comentada ──
        const long long dp = (long long)out_arcs[u].size();
        const long long dm = (long long)in_arcs[u].size();
        const long long db = (long long)mutual_arcs[u].size();
        ── fin version original ── */
        const long long dp = csr_out.start[u + 1]    - csr_out.start[u];
        const long long dm = csr_in.start[u + 1]     - csr_in.start[u];
        const long long db = csr_mutual.start[u + 1] - csr_mutual.start[u];

        nn_orbit[u][0]  = comb2((long long)n_vertices - 1);
        nn_orbit[u][1]  = (long long)m_arcs - dp - dm;
        nn_orbit[u][2]  = dp * ((long long)n_vertices - 2);
        nn_orbit[u][3]  = dm * ((long long)n_vertices - 2);
        nn_orbit[u][4]  = (long long)m_mutual - db;
        nn_orbit[u][5]  = db * ((long long)n_vertices - 2);
        nn_orbit[u][6]  = dm * dp - db;
        nn_orbit[u][9]  = comb2(dm);
        nn_orbit[u][11] = comb2(dp);
        nn_orbit[u][14] = db * (dp - 1);
        nn_orbit[u][17] = db * (dm - 1);
        nn_orbit[u][20] = comb2(db);

        /* ── ORTMANN-00: version original, comentada ──
        for (const AdjEdge &uv : out_arcs[u]) {
            const int v = uv.to;
            const bool vu = has_arc(edge_info[uv.eid], v, u);
            nn_orbit[u][7]  += (long long)out_arcs[v].size() - (vu ? 1 : 0);
            nn_orbit[u][10] += (long long)in_arcs[v].size() - 1;
            nn_orbit[u][16] += (long long)mutual_arcs[v].size() - (vu ? 1 : 0);
        }

        for (const AdjEdge &vu : in_arcs[u]) {
            const int v = vu.to;
            const bool uv = has_arc(edge_info[vu.eid], u, v);
            nn_orbit[u][8]  += (long long)in_arcs[v].size() - (uv ? 1 : 0);
            nn_orbit[u][12] += (long long)out_arcs[v].size() - 1;
            nn_orbit[u][13] += (long long)mutual_arcs[v].size() - (uv ? 1 : 0);
        }

        for (const AdjEdge &uv : mutual_arcs[u]) {
            const int v = uv.to;
            nn_orbit[u][15] += (long long)out_arcs[v].size() - 1;
            nn_orbit[u][18] += (long long)in_arcs[v].size() - 1;
            nn_orbit[u][19] += (long long)mutual_arcs[v].size() - 1;
        }
        ── fin version original ── */

        for (int64_t k = csr_out.start[u]; k < csr_out.start[u + 1]; ++k) {
            const AdjEdge &uv = csr_out.data[k];
            const int v = uv.to;
            const bool vu = has_arc(edge_info[uv.eid], v, u);
            nn_orbit[u][7]  += (csr_out.start[v + 1]     - csr_out.start[v])     - (vu ? 1 : 0);
            nn_orbit[u][10] += (csr_in.start[v + 1]      - csr_in.start[v])      - 1;
            nn_orbit[u][16] += (csr_mutual.start[v + 1]  - csr_mutual.start[v])  - (vu ? 1 : 0);
        }

        for (int64_t k = csr_in.start[u]; k < csr_in.start[u + 1]; ++k) {
            const AdjEdge &vu = csr_in.data[k];
            const int v = vu.to;
            const bool uv = has_arc(edge_info[vu.eid], u, v);
            nn_orbit[u][8]  += (csr_in.start[v + 1]      - csr_in.start[v])      - (uv ? 1 : 0);
            nn_orbit[u][12] += (csr_out.start[v + 1]     - csr_out.start[v])     - 1;
            nn_orbit[u][13] += (csr_mutual.start[v + 1]  - csr_mutual.start[v])  - (uv ? 1 : 0);
        }

        for (int64_t k = csr_mutual.start[u]; k < csr_mutual.start[u + 1]; ++k) {
            const AdjEdge &uv = csr_mutual.data[k];
            const int v = uv.to;
            nn_orbit[u][15] += (csr_out.start[v + 1]    - csr_out.start[v])    - 1;
            nn_orbit[u][18] += (csr_in.start[v + 1]     - csr_in.start[v])     - 1;
            nn_orbit[u][19] += (csr_mutual.start[v + 1] - csr_mutual.start[v]) - 1;
        }
    }
}

static void order_nodes_by_successively_removing_min_degree()
{
    vector<int> degree(n_vertices);
    vector<vector<int> > bucket(n_vertices);
    vector<char> removed(n_vertices, 0);

    for (int v = 0; v < n_vertices; v++) {
        /* ORTMANN-00: degree[v] = (int)Gprime_adj[v].size(); (original, comentada) */
        degree[v] = (int)(csr_Gprime_adj.start[v + 1] - csr_Gprime_adj.start[v]);
        bucket[degree[v]].push_back(v);
    }

    node_order.assign(n_vertices, -1);
    order_rank.assign(n_vertices, -1);

    int min_degree = 0;
    for (int i = 0; i < n_vertices; i++) {
        int v = -1;
        while (v == -1) {
            while (min_degree < n_vertices && bucket[min_degree].empty()) min_degree++;
            if (min_degree >= n_vertices) {
                fprintf(stderr, "Error interno al ordenar por grado minimo\n");
                exit(1);
            }
            const int candidate = bucket[min_degree].back();
            bucket[min_degree].pop_back();
            if (!removed[candidate] && degree[candidate] == min_degree) v = candidate;
        }

        removed[v] = 1;
        node_order[i] = v;
        order_rank[v] = i;

        /* ── ORTMANN-00: version original, comentada ──
        for (const AdjEdge &vw : Gprime_adj[v]) {
            const int w = vw.to;
            if (removed[w]) continue;
            degree[w]--;
            bucket[degree[w]].push_back(w);
            if (degree[w] < min_degree) min_degree = degree[w];
        }
        ── fin version original ── */
        for (int64_t k = csr_Gprime_adj.start[v]; k < csr_Gprime_adj.start[v + 1]; ++k) {
            const int w = csr_Gprime_adj.data[k].to;
            if (removed[w]) continue;
            degree[w]--;
            bucket[degree[w]].push_back(w);
            if (degree[w] < min_degree) min_degree = degree[w];
        }
    }
}

static void orient_Gprime_and_sort_adjacencies_according_node_ordering()
{
    /* ── ORTMANN-00: version original, comentada ──
    Gprime_out.assign(n_vertices, vector<AdjEdge>());
    Gprime_in.assign(n_vertices, vector<AdjEdge>());

    for (int eid = 0; eid < (int)edge_info.size(); eid++) {
        int a = edge_info[eid].a;
        int b = edge_info[eid].b;
        if (order_rank[a] > order_rank[b]) swap(a, b);
        Gprime_out[a].push_back(AdjEdge{b, eid});
        Gprime_in[b].push_back(AdjEdge{a, eid});
    }

    const auto by_rank = [](const AdjEdge &x, const AdjEdge &y) {
        return order_rank[x.to] < order_rank[y.to];
    };
    for (int v = 0; v < n_vertices; v++) {
        sort(Gprime_out[v].begin(), Gprime_out[v].end(), by_rank);
        sort(Gprime_in[v].begin(), Gprime_in[v].end(), by_rank);
    }
    ── fin version original ── */

    /* ORTMANN-00: mismo criterio de orientacion (por order_rank), pero
     * construyendo Gprime_out/Gprime_in como CSR: conteo de grado orientado
     * -> prefix sum -> volcado con cursor -> sort por rango (igual que
     * FCYV-2 ordena cada slice de su adyacencia por id). */
    vector<int> deg_out(n_vertices, 0), deg_in(n_vertices, 0);
    for (int eid = 0; eid < (int)edge_info.size(); eid++) {
        int a = edge_info[eid].a;
        int b = edge_info[eid].b;
        if (order_rank[a] > order_rank[b]) swap(a, b);
        ++deg_out[a];
        ++deg_in[b];
    }

    csr_build(csr_Gprime_out, n_vertices, deg_out);
    csr_build(csr_Gprime_in,  n_vertices, deg_in);

    vector<int64_t> cur_out(csr_Gprime_out.start.begin(), csr_Gprime_out.start.begin() + n_vertices);
    vector<int64_t> cur_in(csr_Gprime_in.start.begin(),   csr_Gprime_in.start.begin()   + n_vertices);

    for (int eid = 0; eid < (int)edge_info.size(); eid++) {
        int a = edge_info[eid].a;
        int b = edge_info[eid].b;
        if (order_rank[a] > order_rank[b]) swap(a, b);
        csr_Gprime_out.data[cur_out[a]++] = AdjEdge{b, eid};
        csr_Gprime_in.data[cur_in[b]++]   = AdjEdge{a, eid};
    }

    const auto by_rank = [](const AdjEdge &x, const AdjEdge &y) {
        return order_rank[x.to] < order_rank[y.to];
    };
    for (int v = 0; v < n_vertices; v++) {
        sort(csr_Gprime_out.data.begin() + csr_Gprime_out.start[v],
             csr_Gprime_out.data.begin() + csr_Gprime_out.start[v + 1], by_rank);
        sort(csr_Gprime_in.data.begin() + csr_Gprime_in.start[v],
             csr_Gprime_in.data.begin() + csr_Gprime_in.start[v + 1], by_rank);
    }
}

static void list_K3_and_increment_orbits_wrt_encoding()
{
    vector<int> mark(n_vertices, -1);

    for (int i = 1; i < n_vertices; i++) {
        const int u = node_order[i];

        /* ── ORTMANN-00: version original, comentada ──
        for (const AdjEdge &uv : Gprime_in[u]) {
            mark[uv.to] = uv.eid;
        }

        for (const AdjEdge &uv : Gprime_in[u]) {
            const int v = uv.to;
            const int eid_uv = uv.eid;
            mark[v] = -1;

            for (const AdjEdge &vw : Gprime_out[v]) {
                const int w = vw.to;
                if (order_rank[w] >= order_rank[u]) break;
                ++nexpansions;
                const int eid_uw = mark[w];
                if (eid_uw == -1) continue;

                const int eid_vw = vw.eid;
                const int code = code_of_triad(u, v, w, eid_uv, eid_uw, eid_vw);
                ni_orbit[u][CODE_ORBIT[code][0]]++;
                ni_orbit[v][CODE_ORBIT[code][1]]++;
                ni_orbit[w][CODE_ORBIT[code][2]]++;
            }
        }

        for (const AdjEdge &uv : Gprime_in[u]) {
            mark[uv.to] = -1;
        }
        ── fin version original ── */

        for (int64_t k = csr_Gprime_in.start[u]; k < csr_Gprime_in.start[u + 1]; ++k) {
            mark[csr_Gprime_in.data[k].to] = csr_Gprime_in.data[k].eid;
        }

        for (int64_t k = csr_Gprime_in.start[u]; k < csr_Gprime_in.start[u + 1]; ++k) {
            const AdjEdge &uv = csr_Gprime_in.data[k];
            const int v = uv.to;
            const int eid_uv = uv.eid;
            mark[v] = -1;

            for (int64_t k2 = csr_Gprime_out.start[v]; k2 < csr_Gprime_out.start[v + 1]; ++k2) {
                const AdjEdge &vw = csr_Gprime_out.data[k2];
                const int w = vw.to;
                if (order_rank[w] >= order_rank[u]) break;
                ++nexpansions;
                const int eid_uw = mark[w];
                if (eid_uw == -1) continue;

                const int eid_vw = vw.eid;
                const int code = code_of_triad(u, v, w, eid_uv, eid_uw, eid_vw);
                ni_orbit[u][CODE_ORBIT[code][0]]++;
                ni_orbit[v][CODE_ORBIT[code][1]]++;
                ni_orbit[w][CODE_ORBIT[code][2]]++;
            }
        }

        for (int64_t k = csr_Gprime_in.start[u]; k < csr_Gprime_in.start[u + 1]; ++k) {
            mark[csr_Gprime_in.data[k].to] = -1;
        }
    }
}

static void solve_system_of_linear_equations()
{
    for (int u = 0; u < n_vertices; u++) {
        for (int i = 20; i >= 0; i--) {
            long long value = nn_orbit[u][i];
            for (int j = i + 1; j < 36; j++) {
                value -= (long long)equation_coeff[i][j] * ni_orbit[u][j];
            }
            ni_orbit[u][i] = value;
        }
    }
}

static void derive_graph_level_triad_census_from_node_orbits()
{
    memset(Census, 0, sizeof(Census));

    for (int u = 0; u < n_vertices; u++) {
        for (int orbit = 0; orbit < 36; orbit++) {
            Census[orbit_to_triad[orbit]] += ni_orbit[u][orbit];
        }
    }

    for (int t = 1; t <= 16; t++) {
        Census[t] /= 3;
    }
}

static void triad_census_algorithm_2()
{
    initialize_table_1_and_figure_8();

    calculate_n0_to_n20();
    order_nodes_by_successively_removing_min_degree();
    orient_Gprime_and_sort_adjacencies_according_node_ordering();
    list_K3_and_increment_orbits_wrt_encoding();
    solve_system_of_linear_equations();
    derive_graph_level_triad_census_from_node_orbits();
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo_grafo>\n", argv[0]);
        return 1;
    }

    vector<DirectedArc> arcs;
    read_graph(argv[1], arcs);
    transform_G_to_underlying_undirected_graph(arcs);

    printf("Vertices: %d   Arcos validos unicos: %d   Dyadas subyacentes: %zu\n",
           n_vertices, m_arcs, edge_info.size());

    const clock_t t0 = clock();
    triad_census_algorithm_2();
    const clock_t t1 = clock();

    printf("\nTriad Census:\n");
    long long total = 0;
    for (int i = 1; i <= 16; i++) {
        printf("  %2d - %-5s : %lld\n", i, TRI_NAME[i], Census[i]);
        total += Census[i];
    }

    /*
     * Motivos conexos = tríadas cuyo grafo subyacente es conexo (>= 2 aristas).
     * Las desconectadas son 003 (vacia), 012 y 102 (una sola dyada + nodo
     * aislado). El resto (021* ... 300) son conexas. Este total coincide con
     * el "Total subgrafos" que reporta FCYV.
     */
    const long long connected = total - Census[T003] - Census[T012] - Census[T102];
    printf("\nMotifs conexos: %lld\n", connected);

    const long long cn3 = comb3(n_vertices);
    printf("\nTotal contado : %lld\n", total);
    printf("C(N,3)        : %lld\n", cn3);
    printf("Diferencia    : %lld  (debe ser 0)\n", cn3 - total);
    printf("nexpansions   : %llu\n", nexpansions);
    printf("Tiempo        : %.6f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);

    return 0;
}
