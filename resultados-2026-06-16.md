# Resultados — Ortmann vs FCYV-2 (2026-06-16)

Comparación sobre todos los grafos de `data/`, midiendo graphlets conexos de 3
nodos, expansiones realizadas y tiempo de cómputo. Se comparan tres variantes:

- **ortman** — Algoritmo 2 de Ortmann-Brandes.
- **FCYV-2 (orden original)** — recorrido con el orden de id del archivo
  (versión previa, referencia).
- **FCYV-2 (orden por grado)** — versión actual: reetiqueta los nodos por grado
  no dirigido de mayor a menor (counting sort), asignando los ids más chicos a
  los nodos de mayor grado, que así se procesan primero como hub y quedan
  excluidos del resto, evitando pagar su `outdeg` enorme como sucesores.

## Metodología

- **Comandos de compilación** (desde la carpeta `Graphlet-Test/`):

  Ortmann:

  ```bash
  g++ -O2 -o ortmangpt Ortmann/ortmangpt.cpp
  ```

  FCYV-2 (versión actual, con reorden por grado):

  ```bash
  g++ -O3 -march=native -funroll-loops -flto -o FCYV-2cpp FCYV-2/FCYV-2.cpp
  ```

  La columna `FCYV-2 (orden original)` corresponde a la versión previa, sin
  paso de reorden.

- **Tiempo:** timer interno de cómputo (no incluye lectura del archivo). En la
  versión con reorden se reporta el tiempo total (reorden + búsqueda); el
  reorden es `O(n+m)` y despreciable.
- **Ejecución:** grafos chicos/medianos en paralelo (8 procesos); los 7 más
  pesados en serie para tiempos limpios.
- **Graphlets conexos:** motivos conexos de 3 nodos. En Ortmann = `total − 003
  − 012 − 102`; en FCYV = `Total subgrafos`.
- **Expansiones:** unidad de trabajo (vecinos/candidatos examinados).

## Resultados (ordenados por M)

| Grafo | N | M | Conexos | exp ort | t ort | exp FCYV orig | t FCYV orig | exp FCYV grado | t FCYV grado |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 6nodos | 6 | 16 | 9 | 3 | 0.000046 | 17 | 0.000001 | 13 | 0.000002 |
| 15nodos_Estrella_Desordenado | 15 | 28 | 91 | 0 | 0.000060 | 14 | 0.000001 | 14 | 0.000002 |
| 15nodos_Estrella_HaciaAfuera | 15 | 28 | 91 | 0 | 0.000054 | 14 | 0.000001 | 14 | 0.000001 |
| 12nodos_grafo_doble | 12 | 34 | 22 | 6 | 0.000050 | 38 | 0.000002 | 30 | 0.000002 |
| estrella_125_nodos | 125 | 248 | 7626 | 0 | 0.000186 | 124 | 0.000003 | 124 | 0.000006 |
| estrella_125_nodos_fix | 125 | 248 | 7626 | 0 | 0.000227 | 124 | 0.000003 | 124 | 0.000006 |
| social | 67 | 284 | 488 | 103 | 0.000171 | 508 | 0.000011 | 406 | 0.000011 |
| elec | 252 | 798 | 1121 | 151 | 0.000414 | 1056 | 0.000017 | 944 | 0.000030 |
| yeast | 688 | 2156 | 13150 | 487 | 0.001258 | 11406 | 0.000110 | 2295 | 0.000052 |
| TFLink Danio rerio | 13773 | 51920 | 65793468 | 19276 | 0.016009 | 53277931 | 0.140249 | 57444 | 0.001121 |
| TFLink Rattus norvegicus | 13546 | 162430 | 261052796 | 292607 | 0.028344 | 147195767 | 0.402586 | 415958 | 0.004632 |
| TFLink S. cerevisiae | 6555 | 462800 | 253562168 | 5198915 | 0.091100 | 201738138 | 0.795067 | 8349584 | 0.071626 |
| TFLink C. elegans | 16529 | 630538 | 553828787 | 8023690 | 0.135764 | 379092903 | 1.386464 | 10981597 | 0.102381 |
| TCGA-BRCA-elbow-GRN | 19251 | 640094 | 152654393 | 3144953 | 0.094823 | 105251288 | 0.406638 | 7411228 | 0.051546 |
| TCGA-OV-elbow-GRN | 19249 | 671228 | 129007119 | 3447662 | 0.100169 | 85479948 | 0.338584 | 8106411 | 0.050741 |
| TFLink Drosophila melanogaster | 18766 | 734472 | 1140552890 | 6152830 | 0.151996 | 722836776 | 2.568488 | 8767543 | 0.086531 |
| TFLink Mus musculus | 21338 | 8040444 | 20806006123 | 541296361 | 18.001140 | 13960070869 | 56.399837 | 649892307 | 6.064497 |
| TFLink Homo sapiens | 20128 | 13236760 | 35123928358 | 1430873179 | 60.491959 | 23581512196 | 110.267581 | 1737731453 | 15.768058 |

## Verificación

El número de graphlets conexos **coincide exactamente** entre las tres
variantes en los 18 grafos. Validación cruzada de correctitud.

## Observaciones

- **El reordenamiento por grado transforma a FCYV.** Las expansiones se
  desploman respecto del orden original (Mus: 13 960 M → 650 M, ~21×; Homo:
  23 582 M → 1738 M, ~14×; Drosophila ~82×), y FCYV pasa a **ganarle a Ortmann
  en tiempo en todos los grafos sesgados:** Danio ≈14×, Rattus ≈6×, Homo
  ≈3.8×, Mus ≈3×, Drosophila ≈1.8×, C. elegans ≈1.3×.
- FCYV gana en wall-time pese a hacer aún algo más de expansiones que Ortmann,
  porque su costo por expansión es menor (bucle interno simple, mejor
  localidad, sin encoding de órbitas ni sistema de ecuaciones ni el footprint
  de ~600 MB de Ortmann).
- **Grafos estrella:** sin triángulos, Ortmann hace 0 expansiones; FCYV examina
  solo los wedges del centro. El reorden no cambia el resultado.
- En grafos chicos el reorden agrega un costo `O(n+m)` despreciable.
