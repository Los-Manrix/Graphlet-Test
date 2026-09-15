# Benchmark de Tríadas y Motivos con python-igraph

Este directorio contiene la implementación del censo de tríadas (subgrafos de 3 nodos) utilizando la librería `igraph` de alto rendimiento desde Python.

## Requisitos

Tener instalado `igraph` en el entorno:

```bash
pip install igraph
```

## Uso

El script acepta la ruta de cualquier grafo en `FormatoP` como argumento de terminal:

```bash
python3 igraph/benchmark_igraph.py <ruta_al_grafo>
```

### Ejemplos de Ejecución

```bash
# Probar grafo pequeño de control (6 nodos)
python3 igraph/benchmark_igraph.py FormatoP/6nodos_procesado.txt

# Probar Danio rerio
python3 igraph/benchmark_igraph.py FormatoP/TFLink_Danio_rerio_interactions_LS_simpleFormat_v1.0_procesado.txt

# Probar Drosophila melanogaster
python3 igraph/benchmark_igraph.py FormatoP/TFLink_Drosophila_melanogaster_interactions_LS_simpleFormat_v1.0_procesado.txt

# Probar WikiTalk (Base 0, 2.39M nodos)
python3 igraph/benchmark_igraph.py FormatoP/WikiTalk_procesado.txt
```

## Características Técnicas

1. **Reconstrucción Fidedigna de Aristas**:
   - En `FormatoP`, cada interacción dirigida $u \to v$ aparece duplicada como `u v 1` (saliente) y `v u 2` (reflejo entrante).
   - El script carga exclusivamente aristas de **Tipo 1** ($u \to v$) y **Tipo 3** ($u \leftrightarrow v$), descartando el Tipo 2 para que `igraph` no considere todas las aristas como bidireccionales.
2. **Auto-Detección de Base 0 / Base 1**:
   - Detecta si el grafo contiene el nodo 0 (como en WikiTalk) o comienza en 1 (como en TFLink) y ajusta los índices automáticamente.
3. **Descarte de Self-Loops**:
   - Filtra aristas donde $u = v$ para mantener paridad estricta con las versiones en C (`test_lectura`, `ImprovedOrmann`, etc.).
4. **Desglose de Tiempos**:
   - Mide por separado el tiempo de I/O y construcción de CSR nativo vs el tiempo de cómputo del censo de tríadas.
