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
| 7-8 (`prev_motif`, `new_motif`) | consulta a `D` y `classify_motif()` |
| 9 (ledger) | `L.push_back(...)`, activable con `--ledger` |
| 10-19 (alta/baja de conteos) | ver "Erratas" |
| 20 (`C[new_motif] += 1`) | igual |
| 22-29 (disconnected thirds) | corrección aritmética en O(1) |

`classify_motif()` (Línea 8) implementa el Apéndice J: submatriz 3×3 sobre `(i,j,k)`, se le
saca la diagonal, se aplica `1{·>0}` y queda una cadena binaria de 6 bits; una biyección
manda esas 64 cadenas a los 16 motivos. La tabla `CODE_TRIAD[64]` **es** esa biyección
materializada. Se reusa la de `Ortmann/ortmann-00.cpp` porque ya está validada contra fuerza
bruta en este repo, y usar la misma hace que los dos censos sean comparables entrada por
entrada.

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

## Desviaciones deliberadas

**Chequeo de arco redundante (Líneas 2-4).** El paper escribe `if A_{i,j} > 1 then return`,
que es correcto para formación pero no para disolución: bajar el peso de 2 a 1 dejaría pasar
el evento aunque el arco siga existiendo. Se usa la condición equivalente y simétrica: el
censo cambia sólo si cambia la **existencia** del arco, no su peso. Es lo que argumenta la
Sección 4.2.1, y para `s = +1` coincide exactamente con la línea publicada.

**Conectividad de la tríada.** Que `{i,j,k}` sea conexa depende de la díada **no dirigida**
`i-j`, no del arco `i→j`. Si `j→i` ya existía, `i` y `j` ya eran vecinos y la tríada ya era
conexa antes del evento. El paper no lo explicita porque su Línea 7 delega en `D`, que lo
resuelve solo; acá hace falta ser explícito para poder mantener `D` con precisión y para que
el modo `--no-dict` sea equivalente.

**Disolución.** El paper dice que `s = -1` "sigue por simetría" y no la desarrolla en el
listado. Falta el caso en que la tríada deja de ser conexa: hay que sacarla de `D` y devolver
su conteo a 012 o 102. Está implementado y **validado** (ver abajo).

**Almacenamiento de `A`.** El paper guarda la matriz densa en `O(V²)` (Apéndice A). Acá se
usa un hash map: mismas consultas en `O(1)`, memoria `O(|E|)`. No cambia la complejidad
declarada, sólo hace usables los grafos del repo.

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
