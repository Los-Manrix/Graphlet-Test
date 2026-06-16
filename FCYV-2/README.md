# FCYV-2 (C++)

Census de motivos conexos de 3 nodos en grafos dirigidos con aristas
etiquetadas por tipo (1, 2, 3). Los conteos se reportan en la tabla
`type[a][b][c]` y en el total de subgrafos conexos.

## Compilación

```bash
g++ -O3 -march=native -funroll-loops -flto -o FCYV-2cpp FCYV-2.cpp
```

## Uso

```bash
./FCYV-2cpp <archivo_grafo>
```

Ejemplo (con un grafo de la carpeta `data/`):

```bash
./FCYV-2cpp ../../data/social_procesado.txt
```

Formato de entrada (índices base 1):

```
N  M
src dst type
...        (M líneas)
```

## Optimizaciones incluidas

1. **Dedup estático `n <= hub`.** Como los hubs se procesan en orden de id,
   "el nodo `n` ya fue hub" equivale exactamente a `n <= hub`. Esto reemplaza
   el arreglo `color` mutable por una simple comparación, eliminando sus
   lecturas y escrituras.
2. **CSR con aristas empaquetadas.** Cada arista se guarda como
   `(dest << 2) | type` en un único `uint32_t`. La adyacencia queda en arreglos
   planos contiguos: la mitad de bytes y recorrido secuencial amigable al
   prefetch del hardware.
3. **Estado por nodo en AoS.** `hub_gen`, `iter` y `tpc` viven en un mismo
   `struct` (una línea de cache), de modo que cada vecino requiere un solo
   acceso aleatorio a memoria en vez de varios arreglos separados.
4. **Marcado por generación.** Un contador `g` que avanza por cada hub evita
   tener que limpiar los arreglos de marcado entre hubs (`O(1)` por hub en vez
   de `O(V)`).
5. **Prefetch software.** En el bucle caliente se precarga el estado del
   próximo vecino para ocultar la latencia de los accesos aleatorios a memoria.
6. **Lectura con `mmap` + parser de enteros propio.** Evita el costo de
   `fscanf`, relevante en los grafos grandes.
7. **Memoria dinámica.** Las estructuras se dimensionan a los `N` y `M` reales
   del grafo; no hay límites fijos de nodos ni de aristas.
8. **Reetiquetado por grado.** Antes de procesar se ordenan los nodos por grado
   no dirigido de mayor a menor (counting/bucket sort, `O(n+m)`), asignando los
   ids más chicos a los nodos de mayor grado. El costo dominante de FCYV es
   `Σ_s (#in-vecinos de s con id menor) · outdeg(s)`; al dar id chico a los
   nodos de alto grado, estos se procesan primero como hub y quedan excluidos
   del resto (`n <= hub`), evitando pagar su `outdeg` enorme como sucesores.
   Reduce las expansiones ~10–70× en grafos sesgados. El census es invariante
   al reetiquetado (cuenta motivos por tipo de arista, no por id).

## Salida

- Tabla completa `[i][j][k] : conteo` para los tipos 0–3.
- `Total subgrafos`: total de motivos conexos contados.
- `nexpansions`: número de expansiones realizadas (métrica de trabajo).
- `Tiempo reorden`: tiempo del reetiquetado por grado.
- `Tiempo busqueda`: tiempo del cómputo del census.
- `Tiempo total`: reorden + búsqueda.
