/*
 * dtc: Dynamic Triad Census (DTC), el algoritmo de
 *
 *   Ryan A. Cook, "Dynamic programming for calculating the triad census",
 *   Social Networks 87 (2026) 77-91.  doi:10.1016/j.socnet.2026.06.003
 *
 * Transcripcion del Algorithm 1 (update_census, pag. 79). Cada linea del
 * pseudocodigo esta marcada con su numero en el cuerpo de update_census().
 *
 * QUE HACE. Mantiene el censo de triadas de forma INCREMENTAL sobre una
 * historia de eventos relacionales (t, i, j, s), con s = +1 formacion de arco
 * y s = -1 disolucion. No recalcula el censo: parte del grafo vacio, donde
 * todas las triadas son 003, y ante cada evento actualiza solo lo que cambia.
 *
 * COMPLEJIDAD. El paper declara O(d_max) por evento y O(d_max * T * h(t)) sobre
 * la historia completa (Seccion 5). Para respetarla:
 *   - A_{i,j} se consulta en O(1)          -> hash map de pesos.
 *   - get_neighbors(v) se recorre en O(d(v)) -> conjunto de vecinos por nodo.
 *   - la union de la Linea 5 se deduplica en O(1) por elemento -> marcas con
 *     epoca, sin limpiar el arreglo entre eventos.
 *   - D se consulta y actualiza en O(1)    -> hash map con clave canonica.
 *   - los "disconnected thirds" NO se enumeran: se corrigen en O(1) por
 *     aritmetica (Lineas 22-29), que es la idea central del paper.
 * El costo por evento queda O(d(i) + d(j)), acotado por O(d_max).
 *
 * Entrada: los mismos grafos que usan los otros algoritmos del repo
 * (`data/<grafo>_procesado.txt`). Como son estaticos, se los reproduce como una
 * historia de eventos donde cada arco del archivo es una formacion. El formato
 * lista cada dyada dos veces, asi que alrededor de la mitad de los eventos son
 * arcos redundantes y ejercitan la salida temprana de la Linea 2.
 */

#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

/* ------------------------------------------------------------------ motivos */

enum TriadId {
    MOTIF_NULL = 0,
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
 * classify_motif() de la Linea 8.
 *
 * El Apendice J del paper define la clasificacion asi: se toma la submatriz
 * 3x3 de A sobre (i,j,k), se le saca la diagonal y se aplica 1{. > 0}, lo que
 * da una cadena binaria de largo 6; despues una biyeccion phi manda esas 2^6 =
 * 64 cadenas a los 16 motivos isomorfos. Esta tabla ES esa biyeccion,
 * materializada: 64 entradas indexadas por la cadena binaria.
 *
 * Es la Table 1 de Ortmann-Brandes (2017), que a su vez viene de la literatura
 * clasica del censo de triadas. Se reusa la del repo (`Ortmann/ortmann-00.cpp`)
 * porque ya esta validada contra fuerza bruta, y usar la misma hace que los
 * censos de los dos programas sean comparables entrada por entrada.
 *
 *   code(u,v,w) = l(u,v) + 2 l(u,w) + 4 l(v,u) + 8 l(v,w) + 16 l(w,u) + 32 l(w,v)
 *
 * El TIPO de triada es invariante ante permutaciones de los tres nodos, asi que
 * alcanza con ser consistente en la asignacion (u,v,w) = (i,j,k).
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

/* --------------------------------------------------------- clave de triada */

/* D esta indexado por el conjunto {i,j,k}, no por la terna ordenada: se guarda
 * siempre en orden canonico creciente (Apendice B del paper). */
struct TriadKey {
    int a, b, c;
    bool operator==(const TriadKey &o) const {
        return a == o.a && b == o.b && c == o.c;
    }
};

struct TriadHash {
    size_t operator()(const TriadKey &k) const {
        uint64_t h = 1469598103934665603ull;
        for (int v : {k.a, k.b, k.c}) {
            h ^= (uint64_t)(uint32_t)v;
            h *= 1099511628211ull;
        }
        return (size_t)h;
    }
};

static TriadKey canonical(int i, int j, int k)
{
    int a = i, b = j, c = k, t;
    if (a > b) { t = a; a = b; b = t; }
    if (b > c) { t = b; b = c; c = t; }
    if (a > b) { t = a; a = b; b = t; }
    return TriadKey{a, b, c};
}

/* ---------------------------------------------- C(n,3) sin desbordar int64 */

static long long comb3(long long x)
{
    if (x < 3) return 0;
    long long a = x, b = x - 1, c = x - 2;
    if      (a % 3 == 0) a /= 3;
    else if (b % 3 == 0) b /= 3;
    else                 c /= 3;
    if      (a % 2 == 0) a /= 2;
    else if (b % 2 == 0) b /= 2;
    else                 c /= 2;
    return a * b * c;
}

/* =========================================================================
 * Estructuras de datos de la Seccion 3.1
 * ========================================================================= */

struct DTC {
    int V = 0;                       /* fijo en el tiempo (limitacion declarada
                                      * en la Seccion 8 del paper) */

    /* A_t: matriz de adyacencia pesada. El paper la guarda densa en O(V^2)
     * (Apendice A). Aca se usa un hash map, que da las mismas consultas en
     * O(1) pero con memoria O(|E|), y por lo tanto no afecta la complejidad
     * declarada, solo la hace utilizable en los grafos grandes del repo. */
    unordered_map<uint64_t, int> Aw;

    /* Vecinos NO dirigidos de cada nodo: N_t(v) del paper. Necesario para que
     * get_neighbors() de la Linea 5 cueste O(d(v)) y no O(V). */
    vector<unordered_set<int> > nbr;

    /* C_t: el censo, 16 conteos. */
    long long C[17];

    /* D_t: hashmap de triadas CONEXAS -> motivo actual. */
    unordered_map<TriadKey, int, TriadHash> D;
    bool use_dict = true;

    /* L: ledger de transiciones (t, i, j, k, motivo previo, motivo nuevo). */
    struct Entry { int t, i, j, k, prev, next; };
    vector<Entry> L;
    bool keep_ledger = false;

    /* Deduplicacion de la Linea 5 en O(1) por elemento. */
    vector<int> mark;
    int epoch = 0;
    vector<int> these_k;

    /* Instrumentacion (no es parte del algoritmo). */
    unsigned long long n_events = 0, n_redundant = 0, n_connected_thirds = 0;

    /* ---------------------------------------------------------- caso base */
    void init(int v)
    {
        V = v;
        nbr.assign(V, unordered_set<int>());
        mark.assign(V, -1);
        memset(C, 0, sizeof(C));
        /* Seccion 4.1: en t = 0 el grafo esta vacio, asi que las C(V,3) triadas
         * son todas 003. */
        C[T003] = comb3(V);
    }

    /* ------------------------------------------------------- acceso a A_t */
    inline uint64_t key(int i, int j) const { return (uint64_t)i * (uint64_t)V + (uint64_t)j; }

    inline int weight(int i, int j) const
    {
        auto it = Aw.find(key(i, j));
        return it == Aw.end() ? 0 : it->second;
    }

    inline bool arc(int i, int j) const { return weight(i, j) > 0; }

    /* Linea 8: classify_motif(i, j, k, A_t) */
    inline int classify_motif(int i, int j, int k) const
    {
        int code = 0;
        if (arc(i, j)) code |= 1;
        if (arc(i, k)) code |= 2;
        if (arc(j, i)) code |= 4;
        if (arc(j, k)) code |= 8;
        if (arc(k, i)) code |= 16;
        if (arc(k, j)) code |= 32;
        return CODE_TRIAD[code];
    }

    /* =====================================================================
     * Algorithm 1: update_census()
     * ===================================================================== */
    void update_census(int t, int i, int j, int s)
    {
        ++n_events;

        /* --- Linea 1: A_{i,j} += (1 x s) -------------------------------- */
        const bool tie_before = arc(i, j);
        const uint64_t kij = key(i, j);
        int &w = Aw[kij];
        w += s;
        if (w < 0) w = 0;                 /* disolver un arco inexistente no hace nada */
        const bool tie_now = (w > 0);

        /* --- Lineas 2-4: arco redundante, salida temprana en O(1) --------
         *
         * El paper escribe `if A_{i,j} > 1 then return`, que es exactamente
         * este chequeo para el caso de formacion (s = +1): el arco ya existia
         * y el censo no cambia. Para disolucion ese test literal no sirve
         * (bajar el peso de 2 a 1 dejaria pasar el evento aunque el arco siga
         * existiendo), asi que se usa la condicion equivalente y simetrica:
         * el censo solo cambia si cambia la EXISTENCIA del arco, no su peso.
         * Es lo que argumenta la Seccion 4.2.1 del paper. */
        if (tie_before == tie_now) {
            ++n_redundant;
            return;
        }

        /* Mantener N_t(.) coherente: j es vecino no dirigido de i si hay arco
         * en cualquiera de las dos direcciones. */
        if (tie_now || arc(j, i)) {
            nbr[i].insert(j);
            nbr[j].insert(i);
        } else {
            nbr[i].erase(j);
            nbr[j].erase(i);
        }

        /* --- Linea 5: these_k <- unique(N(i) u N(j)) \ {i,j} -------------
         * O(d(i) + d(j)), con deduplicacion O(1) por elemento. */
        ++epoch;
        these_k.clear();
        for (int k : nbr[i])
            if (k != i && k != j && mark[k] != epoch) { mark[k] = epoch; these_k.push_back(k); }
        for (int k : nbr[j])
            if (k != i && k != j && mark[k] != epoch) { mark[k] = epoch; these_k.push_back(k); }

        /* Que la triada {i,j,k} sea conexa depende de la dyada NO DIRIGIDA
         * i-j, no del arco i->j: si j->i ya existia, i y j ya eran vecinos y la
         * triada ya era conexa antes de este evento. El arco reverso no lo toca
         * el evento, asi que vale para el antes y el despues. */
        const bool rev = arc(j, i);
        const bool dyad_before = tie_before || rev;
        const bool dyad_now    = tie_now    || rev;

        /* --- Lineas 6-21: los "connected thirds" ------------------------ */
        for (size_t idx = 0; idx < these_k.size(); ++idx) {
            const int k = these_k[idx];
            ++n_connected_thirds;

            /* La triada es conexa si k toca a los dos focales, o si existe la
             * dyada i-j. Como el evento solo toco esa dyada, las adyacencias de
             * k son las mismas antes y despues. */
            const bool k_adj_i = arc(i, k) || arc(k, i);
            const bool k_adj_j = arc(j, k) || arc(k, j);
            const bool both    = k_adj_i && k_adj_j;
            const bool conn_before = both || dyad_before;
            const bool conn_now    = both || dyad_now;

            const TriadKey key3 = canonical(i, j, k);

            /* --- Linea 7: prev_motif <- D['ijk'] ------------------------ */
            int prev_motif = MOTIF_NULL;
            if (use_dict) {
                auto it = D.find(key3);
                if (it != D.end()) prev_motif = it->second;
            } else {
                /* Sin diccionario: el motivo previo se deriva en O(1) del
                 * actual, porque el unico bit que cambio es l(i,j), que es el
                 * bit 0 de la codificacion. Da exactamente lo mismo que leer D
                 * (ver README, "Observacion"), pero no guarda la pertenencia
                 * por triada, que es un entregable del paper. */
                const int code_now = classify_code(i, j, k);
                const int code_before = (code_now & ~1) | (tie_before ? 1 : 0);
                /* Solo cuenta como "estaba en D" si la triada era conexa. */
                prev_motif = conn_before ? CODE_TRIAD[code_before] : MOTIF_NULL;
            }

            /* --- Linea 8 ------------------------------------------------ */
            const int new_motif = classify_motif(i, j, k);

            /* --- Linea 9 ------------------------------------------------ */
            if (keep_ledger) L.push_back(Entry{t, i, j, k, prev_motif, new_motif});

            /* --- Lineas 10-19 ------------------------------------------- */
            if (prev_motif != MOTIF_NULL) {
                /* El paper escribe `C[prev_motif] += 1` en la Linea 12. Es una
                 * errata: hay que RESTAR el motivo viejo, porque la Linea 20
                 * suma el nuevo. Sumando los dos el censo crecería sin limite y
                 * dejaria de valer C(V,3). Ver README, "Erratas". */
                C[prev_motif] -= 1;
            } else {
                /* Lineas 14-18: la triada no era conexa, asi que estaba
                 * contada como 012 o 102 segun si la unica dyada existente era
                 * mutua. */
                const bool mutual = (arc(k, i) && arc(i, k)) || (arc(k, j) && arc(j, k));
                C[mutual ? T102 : T012] -= 1;
            }

            /* --- Linea 20 ----------------------------------------------- */
            C[new_motif] += 1;

            /* --- Linea 11, sacada afuera del if -------------------------
             * El paper la deja adentro de la rama prev_motif != NULL, pero
             * tambien hay que guardar la triada cuando pasa de desconectada a
             * conexa (que es justo la rama else). Y en disolucion puede dejar
             * de ser conexa, en cuyo caso sale de D. Ver README, "Erratas". */
            if (use_dict) {
                if (conn_now) D[key3] = new_motif;
                else          D.erase(key3);
            }
        }

        /* --- Lineas 22-29: los "disconnected thirds", en agregado --------
         * Aca esta el nucleo del paper: los k* que no tocan ni a i ni a j no se
         * enumeran, se corrigen con aritmetica en O(1). */
        const long long kstar = (long long)V - 2 - (long long)these_k.size();

        if (arc(j, i)) {                    /* Linea 23: A_{j,i} >= 1 */
            C[T012] -= kstar * s;           /* Linea 24 */
            C[T102] += kstar * s;           /* Linea 25 */
        } else {                            /* Linea 26 */
            C[T003] -= kstar * s;           /* Linea 27 */
            C[T012] += kstar * s;           /* Linea 28 */
        }
        /* Linea 30: (A, C, D, L quedan actualizados in situ) */
    }

    /* Igual que classify_motif pero devolviendo el codigo crudo de 6 bits. */
    inline int classify_code(int i, int j, int k) const
    {
        int code = 0;
        if (arc(i, j)) code |= 1;
        if (arc(i, k)) code |= 2;
        if (arc(j, i)) code |= 4;
        if (arc(j, k)) code |= 8;
        if (arc(k, i)) code |= 16;
        if (arc(k, j)) code |= 32;
        return code;
    }

    long long total() const
    {
        long long s = 0;
        for (int m = T003; m <= T300; m++) s += C[m];
        return s;
    }
};

/* =========================================================================
 * Lectura de los grafos del repo y armado de la historia de eventos
 * ========================================================================= */

struct Arc { int src, dst; };

/* Formato de `data/<grafo>_procesado.txt`: cabecera "n m", despues m lineas
 * "src dst typ" con ids en base 1 y typ = 1 (src->dst), 2 (dst->src) o
 * 3 (mutua). Cada dyada aparece listada en las dos direcciones. Es el mismo
 * lector que usa Ortmann/ortmann-00.cpp, para que los censos sean comparables. */
static int read_graph(const char *filename, vector<Arc> &arcs)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "No se puede abrir %s\n", filename);
        exit(1);
    }

    int n_vertices = 0, input_m = 0;
    if (fscanf(f, "%d %d", &n_vertices, &input_m) != 2) {
        fprintf(stderr, "Cabecera invalida en %s\n", filename);
        exit(1);
    }

    arcs.clear();
    arcs.reserve((size_t)input_m);
    for (int e = 0; e < input_m; e++) {
        int src, dst, typ;
        const int got = fscanf(f, "%d %d %d", &src, &dst, &typ);
        if (got < 2) break;
        if (got < 3) typ = 1;
        src--; dst--;
        if (src < 0 || dst < 0 || src >= n_vertices || dst >= n_vertices) continue;
        if (src == dst) continue;
        if (typ == 1) {
            arcs.push_back(Arc{src, dst});
        } else if (typ == 2) {
            arcs.push_back(Arc{dst, src});
        } else if (typ == 3) {
            arcs.push_back(Arc{src, dst});
            arcs.push_back(Arc{dst, src});
        } else {
            fprintf(stderr, "Tipo de arco desconocido (%d) en %s\n", typ, filename);
            exit(1);
        }
    }
    fclose(f);
    return n_vertices;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr,
                "Uso: %s <archivo_grafo> [opciones]\n"
                "  --no-dict        no mantener el hashmap D de triadas conexas.\n"
                "                   El censo es identico y la complejidad tambien,\n"
                "                   pero se pierde la pertenencia por triada. Necesario\n"
                "                   en los grafos grandes, donde D no entra en memoria.\n"
                "  --ledger ARCH    volcar el ledger L de transiciones.\n"
                "  --disolver       despues de formar todo, disolver todo en orden\n"
                "                   inverso y verificar que el censo vuelve al caso base.\n"
                "  --disolver-parcial SEED SALIDA\n"
                "                   disuelve ~30 %% de los arcos y escribe el grafo\n"
                "                   remanente, para comparar contra un censo estatico.\n",
                argv[0]);
        return 1;
    }

    const char *path = argv[1];
    bool no_dict = false, disolver = false;
    const char *ledger_path = NULL;
    int parcial_seed = -1;
    const char *parcial_out = NULL;
    for (int a = 2; a < argc; a++) {
        if (!strcmp(argv[a], "--no-dict")) no_dict = true;
        else if (!strcmp(argv[a], "--disolver")) disolver = true;
        else if (!strcmp(argv[a], "--ledger") && a + 1 < argc) ledger_path = argv[++a];
        else if (!strcmp(argv[a], "--disolver-parcial") && a + 2 < argc) {
            parcial_seed = atoi(argv[++a]);
            parcial_out = argv[++a];
        }
        else { fprintf(stderr, "Opcion desconocida: %s\n", argv[a]); return 1; }
    }

    vector<Arc> arcs;
    const int V = read_graph(path, arcs);

    DTC dtc;
    dtc.use_dict = !no_dict;
    dtc.keep_ledger = (ledger_path != NULL);
    dtc.init(V);

    /* La historia de eventos: cada arco del archivo es una formacion. El
     * cronometro cubre solo el replay, no la lectura del archivo. */
    const auto t0 = chrono::steady_clock::now();
    int t = 0;
    for (const Arc &a : arcs) dtc.update_census(t++, a.src, a.dst, +1);
    const auto t1 = chrono::steady_clock::now();

    /* Test diferencial del camino s = -1. La prueba de disolver TODO es debil:
     * el estado final es el grafo vacio y errores intermedios pueden
     * cancelarse. Aca se disuelve un subconjunto aleatorio y se escribe el
     * grafo remanente, para comparar el censo de DTC contra un censo estatico
     * de ese mismo grafo. El remanente se toma de la propia A de DTC, asi que
     * respeta los pesos (un arco listado dos veces sobrevive a una disolucion). */
    if (parcial_out) {
        unsigned rng = (unsigned)parcial_seed * 2654435761u + 1u;
        for (size_t idx = arcs.size(); idx-- > 0; ) {
            rng = rng * 1664525u + 1013904223u;
            if ((rng >> 16) % 100u < 30u)
                dtc.update_census(t++, arcs[idx].src, arcs[idx].dst, -1);
        }
        vector<Arc> rest;
        for (const auto &kv : dtc.Aw)
            if (kv.second > 0)
                rest.push_back(Arc{(int)(kv.first / (uint64_t)V), (int)(kv.first % (uint64_t)V)});
        FILE *o = fopen(parcial_out, "w");
        if (!o) { fprintf(stderr, "No se puede escribir %s\n", parcial_out); return 1; }
        fprintf(o, "%d %zu\n", V, rest.size());
        for (const Arc &a : rest) fprintf(o, "%d %d 1\n", a.src + 1, a.dst + 1);
        fclose(o);
        fprintf(stderr, "grafo remanente (%zu arcos) -> %s\n", rest.size(), parcial_out);
    }

    printf("Grafo         : %s\n", path);
    printf("Vertices      : %d   Eventos: %zu\n", V, arcs.size());
    printf("Tiempo        : %.6f s\n",
           chrono::duration<double>(t1 - t0).count());

    printf("\nTriad Census:\n");
    long long total = 0;
    for (int m = T003; m <= T300; m++) {
        printf("  %2d - %-5s : %lld\n", m, TRI_NAME[m], dtc.C[m]);
        total += dtc.C[m];
    }

    const long long connected = total - dtc.C[T003] - dtc.C[T012] - dtc.C[T102];
    printf("\nMotifs conexos: %lld\n", connected);

    const long long cn3 = comb3(V);
    printf("\nTotal contado : %lld\n", total);
    printf("C(N,3)        : %lld\n", cn3);
    printf("Diferencia    : %lld  (debe ser 0)\n", cn3 - total);

    printf("\nEventos       : %llu   redundantes: %llu (%.1f %%)\n",
           dtc.n_events, dtc.n_redundant,
           dtc.n_events ? 100.0 * dtc.n_redundant / dtc.n_events : 0.0);
    printf("Connected thirds visitados: %llu\n", dtc.n_connected_thirds);
    if (dtc.use_dict) printf("Triadas conexas en D      : %zu\n", dtc.D.size());

    if (ledger_path) {
        FILE *out = fopen(ledger_path, "w");
        if (!out) { fprintf(stderr, "No se puede escribir %s\n", ledger_path); return 1; }
        fprintf(out, "# t i j k motivo_previo motivo_nuevo  (nodos en base 1)\n");
        for (const DTC::Entry &e : dtc.L)
            fprintf(out, "%d %d %d %d %s %s\n", e.t, e.i + 1, e.j + 1, e.k + 1,
                    e.prev == MOTIF_NULL ? "NULL" : TRI_NAME[e.prev], TRI_NAME[e.next]);
        fclose(out);
        printf("Ledger        : %zu transiciones -> %s\n", dtc.L.size(), ledger_path);
    }

    /* Chequeo de la simetria formacion/disolucion: al deshacer todos los
     * eventos en orden inverso el censo tiene que volver al caso base. */
    if (disolver) {
        for (size_t idx = arcs.size(); idx-- > 0; )
            dtc.update_census(t++, arcs[idx].src, arcs[idx].dst, -1);
        const bool ok = (dtc.C[T003] == cn3) && (dtc.total() == cn3);
        printf("\nDisolucion total -> 003 = %lld (esperado %lld): %s\n",
               dtc.C[T003], cn3, ok ? "OK" : "FALLA");
        if (!ok) return 1;
    }

    return (cn3 - total) == 0 ? 0 : 1;
}
