# FCYV-3: Censo de Motivos Conexos de 3 Nodos

Esta carpeta contiene la versión **FCYV-3**, que introduce una optimización en la velocidad de búsqueda ordenando los vecinos de los nodos y deteniendo los bucles antes de tiempo.

---

## 1. Compilación y Ejecución

*Ejecuta los comandos desde la raíz del proyecto (`Graphlet-Test/`):*

### Compilar
```bash
g++ -O3 -march=native -funroll-loops -flto -o FCYV-3cpp FCYV-3/FCYV-3.cpp
```

### Ejecutar
```bash
./FCYV-3cpp <ruta_del_grafo>
```

*Ejemplo:*
```bash
./FCYV-3cpp FormatoP/TFLink_Homo_sapiens_interactions_LS_simpleFormat_v1.0.tsv_procesado.txt
```

---

## 2. Explicación de la Poda

Para no contar dos veces el mismo grupo de 3 nodos (motivo), el algoritmo siempre empieza a contar desde el nodo con el ID más chico. 

Si estamos parados en el nodo `hub`:
*   Cualquier vecino con un ID menor o igual al `hub` ya fue visitado en iteraciones anteriores, por lo que se descarta para evitar duplicados.

**La mejora de FCYV-3 consiste en:**
1.  **Ordenar los vecinos:** La lista de vecinos de cada nodo se ordena de mayor a menor ID.
2.  **Detener el bucle antes (`break`):** Al recorrer la lista de vecinos, tan pronto como encontramos un nodo con ID menor o igual al `hub`, detenemos el bucle inmediatamente. Como la lista está ordenada de mayor a menor, sabemos con certeza que todos los vecinos que quedan por revisar también serán menores o iguales, por lo que no hace falta perder tiempo revisándolos.
