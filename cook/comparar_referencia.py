#!/usr/bin/env python3
"""
Compara esta implementacion en C++ contra la implementacion de referencia del
autor (Python), https://github.com/ryancook4/DTC.

Comprueba dos cosas por cada grafo:
  1. el censo de 16 tipos, entrada por entrada
  2. el contenido de D: mismas triadas conexas y mismo motivo ETIQUETADO
     (tipo mas que nodo ocupa cada posicion estructural)

Ademas verifica el camino de disolucion, reproduciendo en Python el mismo
generador pseudoaleatorio que usa --disolver-parcial en el C++, de modo que los
dos disuelvan exactamente los mismos arcos.

Uso:
    git clone https://github.com/ryancook4/DTC.git
    ./comparar_referencia.py /ruta/a/DTC [directorio_de_datos]

Requiere numpy y pandas (no networkx: eso solo lo usa el baseline del autor).
"""
import os
import subprocess
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
CPP = os.path.join(AQUI, 'dtc')

ORDEN = ['003', '012', '102', '021D', '021U', '021C', '111D', '111U',
         '030T', '030C', '201', '120D', '120U', '120C', '210', '300']

GRAFOS = ['6nodos', '12nodos_grafo_doble', 'social', 'elec', 'yeast']


def leer_grafo(path):
    """Mismo lector que dtc.cpp y que Ortmann/ortmann-00.cpp."""
    tok = open(path).read().split()
    V, m = int(tok[0]), int(tok[1])
    arcos, p = [], 2
    for _ in range(m):
        if p + 2 >= len(tok) + 1:
            break
        src, dst, typ = int(tok[p]) - 1, int(tok[p + 1]) - 1, int(tok[p + 2])
        p += 3
        if not (0 <= src < V) or not (0 <= dst < V) or src == dst:
            continue
        if typ == 1:
            arcos.append((src, dst))
        elif typ == 2:
            arcos.append((dst, src))
        elif typ == 3:
            arcos.append((src, dst))
            arcos.append((dst, src))
    return V, arcos


def corridas_disolucion(arcos, seed):
    """Reproduce el LCG de --disolver-parcial del C++ (uint32 con wraparound)."""
    rng = (seed * 2654435761 + 1) % (2 ** 32)
    fuera = []
    for idx in range(len(arcos) - 1, -1, -1):
        rng = (rng * 1664525 + 1013904223) % (2 ** 32)
        if (rng >> 16) % 100 < 30:
            fuera.append(idx)
    return fuera


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    ref_repo = os.path.abspath(sys.argv[1])
    data = sys.argv[2] if len(sys.argv) > 2 else '/home/mat/claude/graph_motifs/data'

    os.chdir(ref_repo)          # DTC lee 'src/LabeledMotifTruthTables.csv' relativo al cwd
    sys.path.insert(0, ref_repo)
    from src.dtc import DTC

    tmp = '/tmp/cook_D_cpp.txt'
    fallos = 0
    print(f"{'grafo':<24}{'censo':<10}{'D (etiquetado)':<18}{'censo c/disolucion':<20}")
    print(f"{'-'*24}{'-'*10}{'-'*18}{'-'*20}")

    for g in GRAFOS:
        f = os.path.join(data, g + '_procesado.txt')
        if not os.path.exists(f):
            continue
        V, arcos = leer_grafo(f)

        # ---- referencia, solo formaciones ----
        d = DTC(None, V)
        for t, (i, j) in enumerate(arcos):
            d.update((t, i, j, 1))
        censo_ref = [str(int(d.C.get(n, 0))) for n in ORDEN]
        D_ref = {}
        for key, val in d.D.items():
            motif, orden = val.split('_')
            if motif in ('003', '012', '102'):
                continue                      # no conexas: no viven en D
            D_ref[tuple(sorted(int(x) for x in key.split('-')))] = \
                (motif, tuple(int(x) for x in orden.split('-')))

        # ---- C++, solo formaciones ----
        out = subprocess.run([CPP, f, '--volcar-D', tmp],
                             capture_output=True, text=True, check=True).stdout
        censo_cpp = [ln.split()[4] for ln in out.splitlines()
                     if ln.strip().startswith(tuple('0123456789')) and ' - ' in ln]
        D_cpp = {}
        for ln in open(tmp):
            a, b, c, motif, n1, n2, n3 = ln.split()
            D_cpp[(int(a), int(b), int(c))] = (motif, (int(n1), int(n2), int(n3)))

        r1 = 'OK' if censo_ref == censo_cpp else 'FALLA'
        r2 = 'OK' if D_ref == D_cpp else 'FALLA'

        # ---- con disolucion parcial ----
        d2 = DTC(None, V)
        t = 0
        for (i, j) in arcos:
            d2.update((t, i, j, 1)); t += 1
        for idx in corridas_disolucion(arcos, 7):
            i, j = arcos[idx]
            d2.update((t, i, j, -1)); t += 1
        censo_ref2 = [str(int(d2.C.get(n, 0))) for n in ORDEN]

        out2 = subprocess.run([CPP, f, '--disolver-parcial', '7', '/tmp/cook_rest.txt'],
                              capture_output=True, text=True, check=True).stdout
        censo_cpp2 = [ln.split()[4] for ln in out2.splitlines()
                      if ln.strip().startswith(tuple('0123456789')) and ' - ' in ln]
        r3 = 'OK' if censo_ref2 == censo_cpp2 else 'FALLA'

        for r in (r1, r2, r3):
            if r == 'FALLA':
                fallos += 1
        print(f"{g:<24}{r1:<10}{r2:<18}{r3:<20}")

    print()
    print("TODO OK" if fallos == 0 else f"{fallos} comprobacion(es) fallaron")
    return fallos


if __name__ == '__main__':
    sys.exit(main())
