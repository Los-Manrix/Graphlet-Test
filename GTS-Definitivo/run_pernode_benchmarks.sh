#!/bin/bash
# ==============================================================================
# Script de Automatización de Benchmarks para FC3RperNode
# Ejecuta Versión 1 y Versión 2 de SearchGraphletsTotalPerNode para cada grafo.
# ==============================================================================

set -e

# Directorio base del script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuración de compilación
SRC="FC3RperNode.c"
EXEC="./FC3RperNode"
CFLAGS="-O3 -march=native"

# Directorio de los grafos (configurable por argumento $1 o auto-detectado)
USER_DATA_DIR="$1"
CSV_OUT="resultados_pernode.csv"

# Lista de grafos según 'Experimentos.xlsx'
GRAPHS=(
    "TCGA-BRCA-elbow-GRN"
    "TCGA-OV-elbow-GRN"
    "TFLink_Caenorhabditis_elegans_interactions_LS_simpleFormat_v1.0"
    "TFLink_Danio_rerio_interactions_LS_simpleFormat_v1.0"
    "TFLink_Drosophila_melanogaster_interactions_LS_simpleFormat_v1.0"
    "TFLink_Homo_sapiens_interactions_LS_simpleFormat_v1.0"
    "WikiTalk"
    "Amazon0302"
    "Amazon0312"
    "Email-EuAll"
    "Amazon0505"
    "Amazon0601"
    "soc-pokec-relationships"
    "web-BerkStan"
    "web-NotreDame"
    "web-Stanford"
    "wiki-topcats"
)

# 1. Compilación del código con optimización -O3
echo "=============================================================================="
echo " [1/3] Compilando $SRC con: gcc $CFLAGS $SRC -o $EXEC"
echo "=============================================================================="
gcc $CFLAGS "$SRC" -o "$EXEC"
echo "Compilación exitosa."
echo ""

# 2. Localización del directorio de grafos
resolve_data_dir() {
    if [ -n "$USER_DATA_DIR" ] && [ -d "$USER_DATA_DIR" ]; then
        echo "$USER_DATA_DIR"
        return 0
    fi
    for candidate in \
        "../experimentos/FormatoP" \
        "../../experimentos/FormatoP" \
        "../Investigacion-Graphlets/DATA/FormatoP" \
        "./Benchmarks/outs" \
        "./graphs" \
        "./data" \
        "."
    do
        if [ -d "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done
    echo "."
}

DATA_DIR=$(resolve_data_dir)
echo "=============================================================================="
echo " [2/3] Directorio de grafos detectado: $DATA_DIR"
echo "=============================================================================="
echo ""

# Función para encontrar el archivo de un grafo por variaciones de nombre/extensión
find_graph_file() {
    local base="$1"
    for candidate in \
        "$DATA_DIR/$base" \
        "$DATA_DIR/${base}.txt" \
        "$DATA_DIR/${base}_procesado.txt" \
        "$DATA_DIR/${base}.tsv_procesado.txt" \
        "$DATA_DIR/${base}.tsv" \
        "$DATA_DIR/${base}_FormatoP.txt" \
        "$DATA_DIR/${base/-/_}_procesado.txt" \
        "$DATA_DIR/${base/_/-}_procesado.txt"
    do
        if [ -f "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done

    # Búsqueda insensible a mayúsculas/minúsculas
    local found
    found=$(find "$DATA_DIR" -maxdepth 2 -type f -iname "${base}*" 2>/dev/null | head -n 1)
    if [ -n "$found" ] && [ -f "$found" ]; then
        echo "$found"
        return 0
    fi

    return 1
}

# 3. Inicializar archivo CSV de resultados
echo "Grafo,Version,Tiempo_Segundos,Triangulos,Nodos_Tocados,Ruta_Archivo" > "$CSV_OUT"

echo "=============================================================================="
echo " [3/3] Iniciando ejecuciones (Versión 1 y Versión 2 por separado)"
echo " Salida guardada en: $CSV_OUT"
echo "=============================================================================="
printf "%-32s | %-7s | %-15s | %-12s\n" "GRAFO" "VERSIÓN" "TIEMPO (s)" "TRIÁNGULOS"
echo "------------------------------------------------------------------------------"

for graph_name in "${GRAPHS[@]}"; do
    file_path=$(find_graph_file "$graph_name" || true)

    if [ -z "$file_path" ] || [ ! -f "$file_path" ]; then
        echo "[-] Grafo no encontrado: $graph_name (buscado en $DATA_DIR)"
        echo "$graph_name,1,NO_ENCONTRADO,0,0," >> "$CSV_OUT"
        echo "$graph_name,2,NO_ENCONTRADO,0,0," >> "$CSV_OUT"
        continue
    fi

    # Ejecutar Versión 1 (SearchGraphletsTotalPerNode)
    out_v1=$($EXEC "$file_path" 1 asc 2>&1 || true)
    time_v1=$(echo "$out_v1" | sed -n 's/.*tiempo_transcurrido:\([0-9.]*\).*/\1/p')
    tri_v1=$(echo "$out_v1" | sed -n 's/.*ntriagles:\([0-9]*\).*/\1/p')
    touch_v1=$(echo "$out_v1" | sed -n 's/.*touchNodes:\([0-9]*\).*/\1/p')

    if [ -n "$time_v1" ]; then
        printf "%-32s | %-7s | %-15s | %-12s\n" "$graph_name" "V1" "$time_v1" "$tri_v1"
        echo "$graph_name,1,$time_v1,$tri_v1,$touch_v1,$file_path" >> "$CSV_OUT"
    else
        printf "%-32s | %-7s | %-15s | %-12s\n" "$graph_name" "V1" "ERROR / CRASH" "N/A"
        echo "$graph_name,1,ERROR,0,0,$file_path" >> "$CSV_OUT"
    fi

    # Ejecutar Versión 2 (SearchGraphletsTotalPerNode2)
    out_v2=$($EXEC "$file_path" 2 asc 2>&1 || true)
    time_v2=$(echo "$out_v2" | sed -n 's/.*tiempo_transcurrido:\([0-9.]*\).*/\1/p')
    tri_v2=$(echo "$out_v2" | sed -n 's/.*ntriagles:\([0-9]*\).*/\1/p')
    touch_v2=$(echo "$out_v2" | sed -n 's/.*touchNodes:\([0-9]*\).*/\1/p')

    if [ -n "$time_v2" ]; then
        printf "%-32s | %-7s | %-15s | %-12s\n" "$graph_name" "V2" "$time_v2" "$tri_v2"
        echo "$graph_name,2,$time_v2,$tri_v2,$touch_v2,$file_path" >> "$CSV_OUT"
    else
        printf "%-32s | %-7s | %-15s | %-12s\n" "$graph_name" "V2" "ERROR / CRASH" "N/A"
        echo "$graph_name,2,ERROR,0,0,$file_path" >> "$CSV_OUT"
    fi

    echo "------------------------------------------------------------------------------"
done

echo ""
echo "=============================================================================="
echo " Benchmarks finalizados con éxito."
echo " Archivo de resultados generado: $(pwd)/$CSV_OUT"
echo "=============================================================================="
