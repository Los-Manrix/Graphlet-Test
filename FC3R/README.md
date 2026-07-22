# FC3R

Census de motivos conexos de 3 nodos en grafos dirigidos con aristas
etiquetadas por tipo, vía recorrido por hubs (`EfficientGraph.c`).

**FC3R-00** es la misma implementación con un único cambio: usa un **CSR**
(offsets + arrays contiguos) para representar el grafo, en vez de la lista
enlazada (`head`/`nextEdge`) del original.

**FC3R-01** es el ordenamiento de los sucesores: FC3R-00 con un único
agregado, que ordena los sucesores de cada nodo por grado ascendente.

## Compilación

```bash
gcc EfficientGraph.c -O2 -o fc3r
gcc FC3R-00.c -O2 -o fc3r00
gcc FC3R-01.c -O2 -o fc3r01
```

## Uso

```bash
./fc3r   <archivo_grafo>
./fc3r00 <archivo_grafo>
./fc3r01 <archivo_grafo>
```

Ejemplo:

```bash
./fc3r00 ../../data/social_procesado.txt
```
