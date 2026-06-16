# Ortmann (ortmangpt, C++)

Implementación del Algoritmo 2 de:

> Mark Ortmann y Ulrik Brandes, *"Efficient orbit-aware triad and quad census
> in directed and undirected graphs"*, Applied Network Science 2:13, 2017.

Calcula el census de tríadas dirigidas (los 16 tipos MAN: `003`, `012`, …,
`300`) de forma orbit-aware: lista los triángulos con orden por degeneración y
resuelve el sistema de ecuaciones (Fig. 8) para recuperar las tríadas dispersas
sin enumerarlas. Reporta además el número de motivos conexos.

## Compilación

```bash
g++ -O2 -o ortmangpt ortmangpt.cpp
```

## Uso

```bash
./ortmangpt <archivo_grafo>
```

Ejemplo (con un grafo de la carpeta `data/`):

```bash
./ortmangpt ../../data/social_procesado.txt
```

Formato de entrada (índices base 1):

```
N  M
src dst [type]
...           (M líneas; el campo type es opcional y se ignora)
```

## Salida

- `Triad Census`: conteo de cada uno de los 16 tipos de tríada.
- `Motifs conexos`: total de tríadas conexas (todas menos `003`, `012` y `102`).
- `Total contado` y `C(N,3)`: verificación de que el census suma exactamente
  `C(N,3)` (la diferencia debe ser 0).
- `nexpansions`: trabajo realizado, contado como cada candidato `w` examinado
  en el listado de triángulos (métrica comparable a la de FCYV).
- `Tiempo`: tiempo de cómputo en segundos.
