# BM-Triad-v2

Implementación optimizada del algoritmo de Batagelj-Mrvar (2001) para
triad census en grafos dirigidos. Todas las consultas de adyacencia son
O(1) mediante arrays de marcado con generación.

## Compilación

```bash
g++ -O2 -std=c++17 -o BM-Triad-v2 BM-Triad-v2.cpp
```

## Uso

```bash
./BM-Triad-v2 <archivo_grafo>
```

## Formato de entrada

Archivo de texto plano con la siguiente estructura:

```
n m
src1 dst1 [tipo]
src2 dst2 [tipo]
...
```

| Campo | Descripción |
|---|---|
| `n` | Número de vértices |
| `m` | Número de arcos |
| `src` `dst` | Vértices origen y destino (indexados desde 1) |
| `tipo` | Etiqueta de arista — **opcional, se ignora** |

- Los self-loops (`src == dst`) se descartan automáticamente.
- Los arcos duplicados se eliminan en la construcción de las listas.

## Grafos de prueba disponibles

Todos en `data/`:

| Archivo | n | m | Descripción |
|---|---|---|---|
| `6nodos_procesado.txt` | 6 | 16 | Grafo pequeño de prueba |
| `12nodos_grafo_doble_procesado.txt` | 12 | — | Grafo doble de 12 nodos |
| `15nodos_Estrella_HaciaAfuera_procesado.txt` | 15 | 28 | Estrella dirigida hacia afuera |
| `15nodos_Estrella_Desordenado_procesado.txt` | 15 | 28 | Estrella con arcos desordenados |
| `yeast_procesado.txt` | 688 | 2156 | Red de levadura (pequeña) |
| `social_procesado.txt` | — | — | Red social |
| `elec_procesado.txt` | — | — | Red eléctrica |
| `estrella_125_nodos_procesado.txt` | — | — | Estrella de 125 nodos |
| `TFLink_Saccharomyces_cerevisiae_...txt` | 6555 | 462800 | TFLink levadura |
| `TFLink_Danio_rerio_...txt` | 13773 | 51920 | TFLink pez cebra |
| `TFLink_Rattus_norvegicus_...txt` | 13546 | 162430 | TFLink rata |
| `TFLink_Caenorhabditis_elegans_...txt` | 16529 | 630538 | TFLink gusano |
| `TFLink_Drosophila_melanogaster_...txt` | 18766 | 734472 | TFLink mosca |
| `TFLink_Mus_musculus_...txt` | 21338 | 8040444 | TFLink ratón |
| `TFLink_Homo_sapiens_...txt` | 20128 | 13236760 | TFLink humano |

## Salida

```
Vértices: 6555   Arcos válidos: 462800

Triad Census:
   1 - 003   : 45662144392
   2 - 012   : 0
  ...
  16 - 300   : 3107344

Total contado : 46921085485
C(N,3)        : 46921085485
Diferencia    : 0  (debe ser 0)
Tiempo        : 1.394686 s
```

`Diferencia` debe ser siempre 0 — verifica que todos los C(n,3) tripletes
fueron clasificados exactamente una vez.

## Límites

| Parámetro | Valor máximo |
|---|---|
| Vértices (`MAX_N`) | 50 000 |
| Arcos | ilimitado (uso de `vector`) |

Para grafos más grandes modificar `#define MAX_N` en el fuente.

## Tiempos de referencia (Intel Core, -O2)

| Grafo | Tiempo |
|---|---|
| S.cerevisiae (6K nodos, 462K arcos) | ~1.4 s |
| D.rerio (13K nodos, 52K arcos) | ~0.3 s |
| R.norvegicus (13K nodos, 162K arcos) | ~1.2 s |
| C.elegans (16K nodos, 630K arcos) | ~3.0 s |
| D.melanogaster (18K nodos, 734K arcos) | ~6.4 s |
