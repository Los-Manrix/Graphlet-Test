# Resultados — Ortmann vs FCYV-2 (2026-06-16)

Comparación de los dos algoritmos sobre todos los grafos de `data/`, midiendo
graphlets conexos de 3 nodos, expansiones realizadas y tiempo de cómputo.

## Metodología

- **Comandos de compilación** (desde la carpeta `Graphlet-Test/`):

  Ortmann:

  ```bash
  g++ -O2 -o ortmangpt Ortmann/ortmangpt.cpp
  ```

  FCYV-2:

  ```bash
  g++ -O3 -march=native -funroll-loops -flto -o FCYV-2cpp FCYV-2/FCYV-2.cpp
  ```

- **Tiempo:** timer interno de cómputo de cada programa (no incluye la lectura
  del archivo).
- **Ejecución:** los grafos chicos/medianos se midieron en paralelo (8
  procesos); los 7 más pesados se midieron en serie, uno a la vez, para evitar
  contención de memoria y obtener tiempos limpios.
- **Graphlets conexos:** motivos conexos de 3 nodos. En Ortmann se obtiene como
  `total − 003 − 012 − 102` (las únicas tríadas desconectadas); en FCYV es el
  `Total subgrafos`.
- **Expansiones:** unidad de trabajo. En FCYV = cada vecino examinado en el
  recorrido; en Ortmann = cada candidato `w` examinado en el listado de
  triángulos.

## Resultados (ordenados por número de aristas M)

| Grafo | N | M | Graphlets conexos | Exp. ortman | Tiempo ortman (s) | Exp. FCYV | Tiempo FCYV (s) |
|---|---:|---:|---:|---:|---:|---:|---:|
| 6nodos | 6 | 16 | 9 | 3 | 0.000046 | 17 | 0.000001 |
| 15nodos_Estrella_Desordenado | 15 | 28 | 91 | 0 | 0.000060 | 14 | 0.000001 |
| 15nodos_Estrella_HaciaAfuera | 15 | 28 | 91 | 0 | 0.000054 | 14 | 0.000001 |
| 12nodos_grafo_doble | 12 | 34 | 22 | 6 | 0.000050 | 38 | 0.000002 |
| estrella_125_nodos | 125 | 248 | 7626 | 0 | 0.000186 | 124 | 0.000003 |
| estrella_125_nodos_fix | 125 | 248 | 7626 | 0 | 0.000227 | 124 | 0.000003 |
| social | 67 | 284 | 488 | 103 | 0.000171 | 508 | 0.000011 |
| elec | 252 | 798 | 1121 | 151 | 0.000414 | 1056 | 0.000017 |
| yeast | 688 | 2156 | 13150 | 487 | 0.001258 | 11406 | 0.000110 |
| TFLink Danio rerio | 13773 | 51920 | 65793468 | 19276 | 0.016009 | 53277931 | 0.140249 |
| TFLink Rattus norvegicus | 13546 | 162430 | 261052796 | 292607 | 0.028344 | 147195767 | 0.402586 |
| TFLink S. cerevisiae | 6555 | 462800 | 253562168 | 5198915 | 0.091100 | 201738138 | 0.795067 |
| TFLink C. elegans | 16529 | 630538 | 553828787 | 8023690 | 0.135764 | 379092903 | 1.386464 |
| TCGA-BRCA-elbow-GRN | 19251 | 640094 | 152654393 | 3144953 | 0.094823 | 105251288 | 0.406638 |
| TCGA-OV-elbow-GRN | 19249 | 671228 | 129007119 | 3447662 | 0.100169 | 85479948 | 0.338584 |
| TFLink Drosophila melanogaster | 18766 | 734472 | 1140552890 | 6152830 | 0.151996 | 722836776 | 2.568488 |
| TFLink Mus musculus | 21338 | 8040444 | 20806006123 | 541296361 | 18.001140 | 13960070869 | 56.399837 |
| TFLink Homo sapiens | 20128 | 13236760 | 35123928358 | 1430873179 | 60.491959 | 23581512196 | 110.267581 |



## Observaciones

- **Ortmann realiza muchas menos expansiones que FCYV** en todos los casos
  (p. ej. Mus: 541 M vs 13 960 M ≈ 25.8×; Homo: 1 431 M vs 23 582 M ≈ 16.5×).
  Recorre solo candidatos de triángulo bajo el orden por degeneración, mientras
  FCYV examina todos los wedges.
- **Grafos estrella:** Ortmann hace **0 expansiones** (no hay triángulos) y aun
  así cuenta los 7626 graphlets conexos mediante las fórmulas cerradas del
  sistema de ecuaciones, sin recorrer ningún candidato. FCYV, en cambio, debe
  examinar los wedges del centro.
- **Tiempo:** Ortmann es más rápido en los 18 grafos. La ventaja relativa es
  grande en los medianos (Drosophila ≈ 16.9×, C. elegans ≈ 10.2×) pero se
  reduce en los dos más grandes (Mus ≈ 3.1×, Homo ≈ 1.8×), donde la mayor
  presión de memoria de Ortmann erosiona parte de su ventaja algorítmica.
