# Benchmark de Tríadas con igraph (C Nativo y Python)

Este directorio contiene dos implementaciones de benchmark para el censo de tríadas (subgrafos de 3 nodos) utilizando la librería `igraph`:
1. **`benchmark_c.c`**: Implementación nativa en **C** enlazada contra `libigraph`.
2. **`benchmark_igraph.py`**: Implementación en **Python** utilizando `python-igraph`.

---

## 1. Requisitos de Instalación de `igraph`

Para compilar y ejecutar el benchmark en cualquier computadora:

### En Ubuntu / Debian / WSL (Recomendado):
Instalar la librería de desarrollo de `igraph` directamente desde los repositorios del sistema:

```bash
sudo apt update
sudo apt install -y libigraph-dev build-essential
```

*(Nota: Si compilaste `igraph` desde fuentes en `~/igraph`, el `Makefile` detectará automáticamente esa copia sin necesidad de instalar nada más).*

### En Windows:
* **Opción WSL (Recomendada):** Abrir la terminal de Ubuntu en WSL y seguir los pasos de Linux.
* **Opción Nativa (MSYS2 / MinGW-w64):**
  ```bash
  pacman -S mingw-w64-x86_64-igraph mingw-w64-x86_64-gcc make
  make
  ```

---

## 2. Compilación (Versión C)

El `Makefile` incluye auto-detección inteligente (soporta `pkg-config`, instalación en el sistema con `libigraph-dev` o compilación local en `~/igraph`):

```bash
cd igraph
make clean
make
```

Esto generará el ejecutable binario `benchmark_c`.

---

## 3. Ejecución

Ambos programas aceptan la ruta al archivo del grafo en `FormatoP` como argumento de línea de comandos:

```bash
# Ejecutar versión en C:
./igraph/benchmark_c FormatoP/6nodos_procesado.txt
./igraph/benchmark_c FormatoP/12nodos_grafo_doble_procesado.txt
./igraph/benchmark_c FormatoP/15nodos_Estrella_Desordenado_procesado.txt

# Ejecutar versión en Python (requiere: pip install igraph):
python3 igraph/benchmark_igraph.py FormatoP/6nodos_procesado.txt
```

---

## 4. Características y Reglas del Formato

- **Filtro de Aristas Espejo**: En `FormatoP`, cada interacción $u \to v$ aparece como tipo 1 ($u \to v$) y tipo 2 ($v \to u$, reflejo inverso). Ambas implementaciones cargan únicamente las aristas de **Tipo 1** ($u \to v$) y **Tipo 3** ($u \leftrightarrow v$), descartando Tipo 2 para evitar que `igraph` interprete erróneamente todas las aristas como bidireccionales.
- **Auto-Detección Base 0 / Base 1**: Detecta automáticamente si el grafo está indexado desde 0 (como *WikiTalk*) o desde 1 (como *TFLink*) y normaliza a base 0 para `igraph`.
- **Filtro de Self-loops**: Descarta automáticamente aristas donde $u == v$.
- **Desglose de Métricas**: Reporta las 16 clases de Holland & Leinhardt, el total de subgrafos conexos de 3 nodos (suma de clases 3 a 15) y el tiempo separado de I/O vs censo.
