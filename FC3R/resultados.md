# Resultados — FC3R vs FC3R-00 vs Ortmann (todos los grafos TFLink)

Tiempo de ejecución (wall-clock, `/usr/bin/time`), timeout de 300 s por corrida.
**FC3R-00** es FC3R con el CSR para representar el grafo; el resto del código es igual al original.

| Organismo | N (nodos) | M (aristas) | FC3R | FC3R-00 | Ortmann |
|---|---:|---:|---:|---:|---:|
| Danio rerio | 13,773 | 51,920 | 0.01 s | 0.01 s | 0.03 s |
| Rattus norvegicus | 13,546 | 162,430 | 0.03 s | 0.05 s | 0.09 s |
| Saccharomyces cerevisiae | 6,555 | 462,800 | 0.56 s | 0.23 s | 0.30 s |
| Caenorhabditis elegans | 16,529 | 630,538 | 1.00 s | 0.31 s | 0.41 s |
| Drosophila melanogaster | 18,766 | 734,472 | 0.55 s | 0.31 s | 0.55 s |
| Mus musculus | 21,338 | 8,040,444 | 171.87 s | 11.02 s | 24.36 s |
| **Homo sapiens** | 20,128 | 13,236,760 | **TIMEOUT (>300 s)** | **26.96 s** | **63.52 s** |

## Observaciones

- **FC3R original no logra terminar Homo sapiens en 300 s.** Su lista enlazada (arrays `toEdge`/`nextEdge` recorridos por índice, sin contigüidad por nodo) escala muy mal con el tamaño del grafo: de 0.55 s en Drosophila (734 mil aristas) a 171.87 s en Mus musculus (8 millones) y timeout en Homo sapiens (13.2 millones).
- **FC3R-00 (mismo algoritmo, con CSR) escala de forma casi lineal con M** y termina Homo sapiens en 26.96 s — más rápido que Ortmann (63.52 s) en el grafo más grande, igual que ya se había visto en Mus musculus.
- El único cambio entre FC3R y FC3R-00 es la estructura de datos del grafo (lista enlazada → CSR); el algoritmo, el orden de hubs y el dedup dinámico son idénticos en ambos.
