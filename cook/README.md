# cook: Dynamic Triad Census (DTC)

Implementación en C++ del algoritmo de

> Ryan A. Cook, **"Dynamic programming for calculating the triad census"**,
> *Social Networks* 87 (2026) 77-91. doi:10.1016/j.socnet.2026.06.003 (open access)

`dtc.cpp` es una transcripción del **Algorithm 1 (`update_census`, pág. 79)**. Cada línea
del pseudocódigo aparece marcada con su número dentro de `update_census()`.

## Qué resuelve, y en qué se diferencia del resto del repo

DTC **no** es un algoritmo de censo estático. Mantiene el censo de tríadas de forma
**incremental** sobre una historia de eventos relacionales `(t, i, j, s)`, con `s = +1`
formación de arco y `s = -1` disolución. Arranca del grafo vacío, donde las `C(V,3)` tríadas
son todas 003, y ante cada evento actualiza sólo lo que cambia. Nunca recalcula el censo.

Los otros algoritmos del repo (FC3R, Ortmann, FCYV) son estáticos: reciben un grafo y
cuentan. En la terminología del paper, todos ellos son **NTC** ("naive triad census"), que es
justamente el baseline contra el que Cook compara. No compiten con DTC, están en ejes
distintos.

## Correspondencia con el pseudocódigo

| Líneas del paper | Dónde está |
|---|---|
| 1 (`A_{i,j} += 1×s`) | apertura de `update_census()` |
| 2-4 (arco redundante) | chequeo de existencia, ver "Desviaciones" |
| 5 (`these_k`) | unión de vecindarios con marcas de época |
| 6-21 (connected thirds) | bucle principal |
| 7-8 (`prev_motif`, `new_motif`) | consulta a `D` y `classify_code()` + `make_labeled()` |
| 9 (ledger) | `L.push_back(...)`, activable con `--ledger` |
| 10-19 (alta/baja de conteos) | ver "Erratas" |
| 20 (`C[new_motif] += 1`) | igual |
| 22-29 (disconnected thirds) | corrección aritmética en O(1) |

La Línea 8 implementa el Apéndice J: `classify_code()` arma la submatriz 3×3 sobre
`(i,j,k)`, le saca la diagonal, aplica `1{·>0}` y devuelve la cadena binaria de 6 bits;
`make_labeled()` la manda al motivo. `CODE_TRIAD[64]` da el tipo y `CODE_PERM[64]` da qué
nodo ocupa cada posición estructural, o sea el motivo **etiquetado** que la referencia guarda
en `D`. `CODE_TRIAD` es la de `Ortmann/ortmann-00.cpp` (ya validada contra fuerza bruta en
este repo, y usar la misma hace comparables los dos censos entrada por entrada);
`CODE_PERM` se generó desde `LabeledMotifTruthTables.csv` del repo del autor. Las dos tablas
se verificaron equivalentes a la suya en las 64 combinaciones.

## Erratas del pseudocódigo publicado

El listado del Algorithm 1 tiene dos defectos. Se implementó la versión correcta y se deja
constancia acá.

**1. Línea 12 dice `C[prev_motif] += 1`, tiene que ser `-= 1`.** La Línea 20 suma el motivo
nuevo; si además se sumara el viejo, el censo crecería sin límite y dejaría de valer
`C(V,3)`. La prosa de la Sección 3.2 confirma la intención al describir la rama contraria:
*"we must subtract 1 from its previous configuration"*.

**2. La Línea 11 (`D[ijk] ← new_motif`) está dentro de la rama `prev_motif ≠ NULL`, pero
tiene que ejecutarse también en la rama `else`.** Es justamente en esa rama donde la tríada
pasa de desconectada a conexa, o sea el caso en que hay que darla de alta en `D`. Tal como
está publicado, esas tríadas nunca entrarían al diccionario y el siguiente evento que las
tocara volvería a tratarlas como desconectadas.

## Comparación con la implementación de referencia

El autor publicó su código en <https://github.com/ryancook4/DTC> (Python y R). Se contrastó
`src/dtc.py` línea por línea contra esta implementación.

| | referencia (`src/dtc.py`) | esta implementación |
|---|---|---|
| `A` | `np.zeros((V,V))`, densa `O(V²)` | hash map, `O(E)`, consultas `O(1)` |
| `get_neighbors(u)` | `argwhere(A[u,:]>0)` + `argwhere(A[:,u]>0)`: **`O(V)`** | conjunto de vecinos: **`O(d(u))`** |
| deduplicación de `these_k` | `set(...)` | marcas de época, `O(1)` por elemento |
| arco redundante | `(A>1 and s==1) or (A>=1 and s==-1)` | `tie_before == tie_now`, equivalente |
| `classify_triad` | CSV de 64 filas, bits `(ij,ik,jk,ji,ki,kj)` | tabla de 64, otra codificación de bits |
| `D` | clave `"a-b-c"` → `"021D_3-7-5"` (motivo **etiquetado**) | clave canónica → `{motivo, nodos[3]}` |
| baja de `D` | nunca borra; detecta desconectado por el nombre del motivo | borra cuando deja de ser conexa |
| ledger | `(t, clave, motivo_prev, motivo_curr)` | igual, con motivo etiquetado |

**Las dos erratas están confirmadas por el propio código del autor.** `src/dtc.py:107` hace
`self.C[prev_main_motif] -= 1`, o sea **resta**, no suma como dice la Línea 12 publicada. Y
`src/dtc.py:103` hace `self.D[this_key] = curr_motif_id` **antes e independientemente** del
`if/else`, no dentro de la rama `prev_motif ≠ NULL` como aparece en la Línea 11. También su
chequeo de arco redundante contempla los dos signos, cosa que la Línea 2 publicada no hace.
O sea que el listado del paper tiene erratas de transcripción y el código real coincide con lo
que se implementó acá.

### La referencia no alcanza la complejidad que declara el paper

`get_neighbors()` barre una fila y una columna enteras de la matriz densa:

```python
return np.concatenate( (np.argwhere( self.A[u, :]>0),
                        (np.argwhere( self.A[:, u]>0))) ).reshape(-1)
```

Eso es `O(V)` por llamada, dos llamadas por evento. La Sección 5.1 del paper afirma que el
acceso a vecinos es `O(1)` amortizado *"since nodes have canonical ordering and hashing"*, y
de ahí deriva el `O(Δ̂)` por evento. La implementación publicada no hace eso.

Medido, con 800 eventos aleatorios sobre un grafo casi vacío (grado medio 0,1), donde un
algoritmo `O(Δ̂)` debería costar prácticamente lo mismo para cualquier `V`:

| `V` | referencia (µs/evento) | esta implementación (µs/evento) |
|---:|---:|---:|
| 2 000 | 51,0 | 0,6 |
| 4 000 | 89,0 | 0,5 |
| 8 000 | 203,9 | 0,3 |
| 16 000 | 626,0 | 0,3 |

El costo de la referencia se multiplica por 12 al multiplicar `V` por 8; el de acá se mantiene
plano. El factor constante entre Python y C++ no es lo relevante: lo que importa es la
**forma** de la curva, lineal en `V` contra plana. Esto además explica una observación del
propio paper (Apéndice F): que la ventaja de DTC sobre el baseline *disminuye* al pasar de
`V = 1e2` a `V = 1e3`, algo que un algoritmo genuinamente `O(Δ̂)` no debería mostrar.

A esto se suma que la matriz densa cuesta `O(V²)` de memoria: con `V = 16 000` son 2 GB en
float64, lo que en la práctica pone el techo de tamaño mucho antes que el tiempo.

### Validación cruzada contra la referencia

```bash
git clone https://github.com/ryancook4/DTC.git
./comparar_referencia.py /ruta/a/DTC
```

Compara censo, contenido de `D` (incluido el motivo etiquetado, o sea qué nodo ocupa cada
posición estructural) y censo con disolución parcial, reproduciendo en Python el mismo
generador pseudoaleatorio del C++ para que los dos disuelvan exactamente los mismos arcos.

Resultado: **15 de 15 comprobaciones OK** sobre 6nodos, 12nodos, social, elec y yeast. El
contenido de `D` coincide entrada por entrada, con las mismas tríadas y el mismo orden de
nodos etiquetado. Las dos tablas de clasificación (la CSV del autor y la de 64 entradas usada
acá) se verificaron equivalentes en las **64** combinaciones binarias.

## Desviaciones deliberadas

**Chequeo de arco redundante (Líneas 2-4).** El paper escribe `if A_{i,j} > 1 then return`,
que es correcto para formación pero no para disolución: bajar el peso de 2 a 1 dejaría pasar
el evento aunque el arco siga existiendo. Se usa la condición equivalente y simétrica: el
censo cambia sólo si cambia la **existencia** del arco, no su peso. Es lo que argumenta la
Sección 4.2.1, y coincide con lo que hace el código del autor
(`(A>1 and s==1) or (A>=1 and s==-1)`), que también contempla los dos signos.

**Conectividad de la tríada.** Que `{i,j,k}` sea conexa depende de la díada **no dirigida**
`i-j`, no del arco `i→j`. Si `j→i` ya existía, `i` y `j` ya eran vecinos y la tríada ya era
conexa antes del evento. El paper no lo explicita porque su Línea 7 delega en `D`, que lo
resuelve solo; acá hace falta ser explícito para poder mantener `D` con precisión y para que
el modo `--no-dict` sea equivalente.

**Disolución.** El paper dice que `s = -1` "sigue por simetría" y no la desarrolla en el
listado. Falta el caso en que la tríada deja de ser conexa: hay que sacarla de `D` y devolver
su conteo a 012 o 102. Está implementado y **validado** (ver abajo).

**Almacenamiento de `A` y acceso a vecinos.** El paper guarda la matriz densa en `O(V²)`
(Apéndice A) y la referencia obtiene los vecinos barriendo fila y columna, lo que cuesta
`O(V)`. Acá `A` es un hash map (consultas `O(1)`, memoria `O(E)`) y cada nodo tiene su
conjunto de vecinos no dirigidos, con lo que `get_neighbors` cuesta `O(d(u))`. Es la única
desviación que cambia el comportamiento asintótico, y es deliberada: es lo que hace falta
para que se cumpla el `O(Δ̂)` por evento que el paper declara. Ver la comparación de arriba.

## Observación: `D` no hace falta para el censo

El motivo previo se puede derivar en `O(1)` del actual, porque el único bit que cambia es
`l(i,j)`, que es el bit 0 de la codificación:

```
code_before = (code_now & ~1) | (existía i→j antes)
```

Con eso, las Líneas 13-19 del paper resultan ser un caso particular de "clasificar la tríada
sobre el estado anterior de `A`". La bandera `--no-dict` usa esta derivación y produce **el
censo idéntico** (verificado en los 9 grafos). Lo que se pierde es la pertenencia por tríada,
que sí es un entregable del paper: es lo que habilita el análisis de brokerage y usar los
motivos como covariables en un REM. Por eso `D` queda activo por omisión.

## Uso

```bash
g++ -O2 -o dtc dtc.cpp

./dtc ../../data/social_procesado.txt                 # censo, modo fiel al paper
./dtc <grafo> --no-dict                               # sin D: mismo censo, mucho más liviano
./dtc <grafo> --ledger transiciones.txt               # volcar el ledger L
./dtc <grafo> --volcar-D D.txt                        # volcar D con motivo etiquetado
./dtc <grafo> --disolver                              # formar todo y después disolver todo
./dtc <grafo> --disolver-parcial 7 remanente.txt      # disolver ~30 % y escribir el resto
```

Lee los mismos grafos que el resto del repo (`data/*_procesado.txt`). Como son estáticos, se
los reproduce como historia de eventos donde cada arco del archivo es una formación. El
formato lista cada díada dos veces, así que alrededor de la mitad de los eventos son arcos
redundantes y ejercitan la salida temprana de la Línea 2.

## Validación

```bash
./validar.sh                       # o: ORTMANN=/ruta/al/binario ./validar.sh
```

Compara contra `ortmann-00`, que en este repo ya está verificado contra fuerza bruta
(`Ortmann/validacion/`). Cuatro comprobaciones por grafo: censo de 16 tipos entrada por
entrada, equivalencia del modo `--no-dict`, disolución total, y disolución parcial contra un
censo estático del grafo remanente.

Esta última es la importante para el camino `s = -1`: disolver **todo** es una prueba débil
porque el estado final es el grafo vacío y los errores intermedios pueden cancelarse. De
hecho, una versión anterior con un bug real la pasaba igual. La disolución parcial lo detecta.

Resultado: **36 de 36 comprobaciones OK** sobre 9 grafos (6nodos, 12nodos, las dos estrellas
de 15, estrella_125, social, elec, yeast y Danio rerio).

## Límites prácticos (medidos, no estimados)

El hashmap `D` guarda una entrada por tríada conexa, que es `O(V³)` en el peor caso. El propio
paper lo declara en el Apéndice A. En la práctica es lo que decide hasta dónde se llega:

| grafo | nodos | arcos | con `D` | `--no-dict` |
|---|---:|---:|---|---|
| Danio rerio | 13 773 | 51 920 | 53,8 s / **3,9 GB** | 7,1 s / 9 MB |
| Rattus norvegicus | 13 546 | 162 430 | 4 510 s / **15,5 GB** | 34,6 s / 16 MB |
| Saccharomyces | 6 555 | 462 800 | no medido (inviable) | 81,8 s / 37 MB |
| C. elegans | 16 529 | 630 538 | no medido (inviable) | 215,0 s / 49 MB |
| Drosophila | 18 766 | 734 472 | inviable | no termina en 600 s |

Dos conclusiones:

1. **Con `D` sólo se llega a los grafos chicos.** Rattus ya pide 15,5 GB para un grafo de
   apenas 81 mil díadas. Mus musculus y Homo sapiens están fuera de discusión: sólo sus
   triángulos son 444 M y 1 271 M respectivamente, y `D` guarda además todas las tríadas
   conexas abiertas, que son muchas más.
2. **Aun sin `D`, DTC se pone lento en los grafos densos.** Es esperable y no es un defecto de
   la implementación: el costo es la suma sobre eventos de `d(i) + d(j)`, así que reproducir
   un grafo denso como historia de eventos es el peor caso para un algoritmo dinámico. DTC
   está pensado para *event logs* donde el censo se consulta muchas veces a lo largo del
   tiempo, no para contar una vez sobre un grafo ya armado.

## Comparación contra FC3R y Ortmann sobre el censo estático

Los tres calculan el mismo censo y **coinciden en todos los grafos**: el censo de 16 tipos de
DTC es idéntico al de `ortmann-00`, y el total de motivos conexos de FC3R-01 coincide con el
de los otros dos. Lo que cambia es el tiempo.

Tiempo de algoritmo, sin la lectura del archivo en ninguno de los tres (medianas de 3
corridas; 1 corrida en los dos más lentos). DTC en modo `--no-dict`, que es su caso más
favorable:

| grafo | n | arcos | ortmann-00 | FC3R-01 | DTC | DTC / ortmann |
|---|---:|---:|---:|---:|---:|---:|
| 6nodos | 6 | 16 | 0,022 ms | < 0,01 ms | 0,006 ms | 0,3 × |
| 12nodos | 12 | 34 | 0,027 ms | < 0,01 ms | 0,010 ms | 0,4 × |
| social | 67 | 284 | 0,090 ms | < 0,01 ms | 0,120 ms | 1,3 × |
| estrella_125 | 125 | 248 | 0,109 ms | < 0,01 ms | 0,372 ms | 3,4 × |
| elec | 252 | 798 | 0,258 ms | < 0,01 ms | 0,208 ms | 0,8 × |
| yeast | 688 | 2 156 | 0,571 ms | < 0,01 ms | 1,218 ms | 2,1 × |
| Danio rerio | 13 773 | 51 920 | 12,3 ms | 2,0 ms | 5 798 ms | **473 ×** |
| Rattus | 13 546 | 162 430 | 21,6 ms | 9,0 ms | 31 954 ms | **1 479 ×** |
| Saccharomyces | 6 555 | 462 800 | 100,1 ms | 70,0 ms | 79 984 ms | **799 ×** |
| C. elegans | 16 529 | 630 538 | 141,4 ms | 94,9 ms | 200 999 ms | **1 421 ×** |
| Drosophila | 18 766 | 734 472 | 160,1 ms | 87,1 ms | > 600 000 ms | **> 3 700 ×** |

**Esto no es un defecto de DTC ni de esta implementación: es su peor caso por construcción.**
Reproducir un grafo estático como historia de eventos y mirar el resultado una sola vez es
exactamente el escenario donde toda su contabilidad incremental es puro sobrecosto. DTC está
diseñado para lo contrario: un *event log* donde el censo se consulta muchas veces a lo largo
del tiempo y nunca se recalcula.

Los contadores confirman que el costo es el que la complejidad predice, no algo cuadrático:
en Rattus visita 261 M *connected thirds* en 162 430 eventos, o sea unos 1 600 por evento, a
117 ns cada uno. El promedio de 1 600 (contra un grado medio de 12) es puro efecto de los
hubs: el costo por evento es `O(d(i) + d(j))` y en estos grafos `d` está muy sesgado.

### Dónde estaría el punto de equilibrio

La pregunta correcta no es cuál gana en una foto, sino a partir de cuántas fotos conviene DTC.
Recalcular estáticamente en `T` instantes cuesta `T ×` un censo estático; DTC cuesta el replay
completo una sola vez. El cruce está en:

| grafo | vs ortmann-00 | vs FC3R-01 |
|---|---:|---:|
| Danio rerio | 473 instantes | 2 899 instantes |
| Saccharomyces | 799 | 1 143 |
| C. elegans | 1 421 | 2 118 |
| Rattus | 1 479 | 3 550 |

O sea que en estos grafos hacen falta del orden de **500 a 3 500 instantes de tiempo** antes de
que DTC compense. Por debajo de eso conviene recalcular con FC3R o con Ortmann. Es un número
concreto y defendible para el paper: delimita el terreno de cada enfoque sin descalificar a
ninguno.

Una salvedad honesta: esta implementación de DTC no está optimizada. Cada *third* hace del
orden de diez consultas al hash map de `A` (conectividad, clasificación y el chequeo de
mutualidad), y reusando valores se podrían recortar a la mitad. Un factor 2 de mejora no
cambiaría ninguna de las conclusiones de arriba, pero conviene no leer los 473-1 479 × como si
todo fuera intrínseco al algoritmo.

## Complejidad

El paper declara `O(Δ̂)` por evento y `O(Δ̂·T·h(t))` sobre la historia completa (Sección 5),
con `Δ̂` el grado máximo y `h(t)` la tasa de arcos redundantes que puede saltear. Para
respetarla:

- `A_{i,j}` se consulta en `O(1)` (hash map de pesos).
- `get_neighbors(v)` se recorre en `O(d(v))` (conjunto de vecinos por nodo).
- La unión de la Línea 5 se deduplica en `O(1)` por elemento con marcas de época, sin limpiar
  el arreglo entre eventos.
- `D` se consulta y actualiza en `O(1)` (hash map con clave canónica creciente, Apéndice B).
- Los "disconnected thirds" **no se enumeran**: se corrigen en `O(1)` por aritmética
  (Líneas 22-29). Ésta es la idea central del paper.

El costo por evento queda `O(d(i) + d(j))`, acotado por `O(Δ̂)`.

## Limitaciones heredadas del paper

`V` es fijo en el tiempo (Sección 8: los actores que entran o salen tienen que estar todos
incluidos desde el principio, porque cambiar `V` invalida el caso base y toda la tabulación
previa). No se admiten self-loops ni multiplexidad: un solo tipo de arco entre cada par.
