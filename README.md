## Cambios recientes

1. **Corrección del bug de `iter`**: se incorporó `iter++` dentro del loop principal de `search_motif_driver`. Este era el error crítico que impedía detectar correctamente los triángulos: al mantenerse `iter` siempre en 1, los nodos procesados en iteraciones anteriores eran incorrectamente descartados por la condición `n->iter != iter`.

2. **Directivas de compilación `VERBOSE` y `DEBUG`**: se agregaron dos defines en la cabecera del archivo:
   - `VERBOSE`: si vale `1`, imprime los mensajes de traza interna (nodos popeados, ajustes a la matriz, conteos parciales). Si vale `0`, el programa corre en silencio.
   - `DEBUG`: si vale `1`, ejecuta `getchar()` como pausa interactiva en puntos clave del algoritmo. Si vale `0`, el programa corre sin interrupciones.

3. **Ruta del grafo como argumento**: el archivo del grafo de entrada ya no está hardcodeado. Se pasa como argumento al ejecutar el programa.

## Compilar

```bash
gcc FCYV-1.c -o main.o
```

## Ejecutar

```bash
./main.o <RUTA_DEL_GRAFO_EN_FORMATO_P>
```

Ejemplo:

```bash
./main.o ../../data/6nodos_procesado.txt
```