#!/usr/bin/env python3
"""
Benchmark de Tríadas y Motivos usando python-igraph.

Soporta los datasets en FormatoP:
- Detecta y descarta la cabecera 'N M'.
- Detecta automáticamente indexación Base 0 o Base 1 (shift).
- Descarta self-loops (u == v).
- Maneja la codificación espejo: carga aristas tipo 1 (u -> v) y tipo 3 (u <-> v),
  descartando aristas tipo 2 (que son el reflejo inverso de tipo 1).
"""

import sys
import time
import igraph as ig

# Nombres de las 16 clases estándar de Holland & Leinhardt (MAN labeling)
TRIAD_NAMES = [
    "003",  # 0: Vacio (sin aristas)
    "012",  # 1: 1 arista asimetrica
    "102",  # 2: 1 arista mutua
    "021D", # 3: V -> U, W -> U (convergente / in-star)
    "021U", # 4: U -> V, U -> W (divergente / out-star)
    "021C", # 5: U -> V -> W (camino dirigido)
    "111D", # 6: U <-> V, W -> U
    "111U", # 7: U <-> V, U -> W
    "030T", # 8: Triangulo transitivo (feed-forward loop)
    "030C", # 9: Triangulo ciclico (3-cycle)
    "201",  # 10: U <-> V <-> W
    "120D", # 11: 120D (mutua + 2 entrantes)
    "120U", # 120U (mutua + 2 salientes)
    "120C", # 120C (mutua + ciclo dirigido)
    "210",  # 14: 210 (2 mutuas + 1 asimetrica)
    "300"   # 15: 300 (3 mutuas, clique completo)
]

def load_graph(filepath):
    """
    Carga el grafo desde el archivo FormatoP optimizando la construccion del CSR nativo de igraph.
    """
    print(f"Cargando archivo: {filepath} ...")
    t0 = time.time()
    
    edges = []
    num_nodes = 0
    num_edges = 0
    shift = 1 # Asume base 1 por defecto
    
    with open(filepath, "r") as f:
        # 1. Leer cabecera N M
        first_line = f.readline().split()
        if len(first_line) >= 2:
            num_nodes = int(first_line[0])
            num_edges = int(first_line[1])
        
        # 2. Comprobar si contiene nodo 0 (para auto-detectar Base 0 vs Base 1)
        data_pos = f.tell()
        for line in f:
            p = line.split()
            if len(p) >= 2:
                u, v = int(p[0]), int(p[1])
                if u == 0 or v == 0:
                    shift = 0
                    break
        f.seek(data_pos)
        
        # 3. Leer aristas: solo tipo 1 y tipo 3, descartando self-loops
        for line in f:
            p = line.split()
            if len(p) >= 3:
                u, v, t = int(p[0]), int(p[1]), int(p[2])
                if u == v:
                    continue # Descartar self-loop
                
                # Tipo 1 (asimetrica u->v) o Tipo 3 (mutua u<->v)
                if t == 1 or t == 3:
                    edges.append((u - shift, v - shift))
    
    # 4. Crear grafo nativo directo con C-core
    g = ig.Graph(n=num_nodes, edges=edges, directed=True)
    t_load = time.time() - t0
    
    print(f"Grafo cargado exitosamente en {t_load:.4f} segundos.")
    print(f"  - Base detectada    : Base {shift}")
    print(f"  - Vertices          : {g.vcount():,}")
    print(f"  - Aristas dirigidas : {g.ecount():,}")
    
    return g, t_load

def run_benchmark(filepath):
    g, t_io = load_graph(filepath)
    
    print("\n--- Ejecutando Censo de Triadas (igraph.triad_census) ---")
    t0 = time.time()
    raw_census = list(g.triad_census())
    t_census = time.time() - t0
    
    # Indices 3 a 15 corresponden a las 13 triadas conexas (motivos conexos de 3 nodos)
    connected_subgraphs = sum(raw_census[3:])
    
    print("\nResultados del Triad Census (16 clases de Holland & Leinhardt):")
    print(f"{'Clase':<8} {'Nombre':<8} {'Conexidad':<12} {'Conteo':>20}")
    print("-" * 52)
    for idx, count in enumerate(raw_census):
        name = TRIAD_NAMES[idx]
        conn = "Desconectada" if idx < 3 else "Conexa"
        print(f"[{idx:02d}]     {name:<8} {conn:<12} {count:>20,}")
    print("-" * 52)
    
    t_total = t_io + t_census
    print(f"\nTotal subgrafos conexos de 3 nodos: {connected_subgraphs:,}")
    print(f"Tiempo Lectura e I/O : {t_io:.6f} segundos")
    print(f"Tiempo Censo Triadas : {t_census:.6f} segundos")
    print(f"Tiempo Total         : {t_total:.6f} segundos")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Uso: python3 {sys.argv[0]} <ruta_al_grafo>")
        sys.exit(1)
    
    run_benchmark(sys.argv[1])
