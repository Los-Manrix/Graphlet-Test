#!/bin/bash
# Valida la implementacion de DTC contra ortmann-00, que en este repo ya esta
# verificado contra fuerza bruta (ver Ortmann/validacion/).
#
# Por cada grafo comprueba tres cosas:
#   1. el censo de 16 tipos, entrada por entrada, contra ortmann-00
#   2. que el modo --no-dict (sin el hashmap D) de exactamente el mismo censo
#   3. que al disolver todos los arcos en orden inverso el censo vuelva al caso
#      base (todas las triadas en 003), que es lo que ejercita el camino s = -1
#
# Uso:  ./validar.sh [directorio_de_datos]
# Var:  ORTMANN=/ruta/al/binario  para usar un ortmann-00 ya compilado
#       (util si el fuente del repo tiene cambios sin commitear)

set -u
DATA="${1:-/home/mat/claude/graph_motifs/data}"
HERE="$(cd "$(dirname "$0")" && pwd)"
DTC="$HERE/dtc"
ORT="${ORTMANN:-$HERE/../Ortmann/ortmann-00}"

[ -x "$DTC" ] || { echo "Falta $DTC. Compilar con: g++ -O2 -o dtc dtc.cpp"; exit 1; }
if [ ! -x "$ORT" ]; then
    echo "Compilando ortmann-00 de referencia..."
    g++ -O2 -o "$ORT" "$HERE/../Ortmann/ortmann-00.cpp" || exit 1
fi

censo () { grep -E '^ +[0-9]+ - ' | awk '{print $3, $5}'; }

# Grafos de menor a mayor. Los grandes se omiten porque el hashmap D del paper
# no entra en memoria; ver README, "Limites practicos".
GRAFOS="
6nodos_procesado.txt
12nodos_grafo_doble_procesado.txt
15nodos_Estrella_HaciaAfuera_procesado.txt
15nodos_Estrella_Desordenado_procesado.txt
estrella_125_nodos_procesado.txt
social_procesado.txt
elec_procesado.txt
yeast_procesado.txt
TFLink_Danio_rerio_interactions_LS_simpleFormat_v1.0_procesado.txt
"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

fallos=0
printf "%-40s %-11s %-9s %-11s %-11s\n" \
       "grafo" "vs-ortmann" "no-dict" "disol-total" "disol-parcial"
printf "%-40s %-11s %-9s %-11s %-11s\n" \
       "----------------------------------------" "-----------" "---------" \
       "-----------" "-------------"

for g in $GRAFOS; do
    [ -f "$DATA/$g" ] || continue
    nombre=$(echo "$g" | sed 's/_procesado\.txt$//' | cut -c1-40)

    # 1. censo contra ortmann-00
    a=$("$DTC" "$DATA/$g"            2>/dev/null | censo)
    b=$("$ORT" "$DATA/$g" /dev/null  2>/dev/null | censo)
    if [ -n "$a" ] && [ "$a" = "$b" ]; then r1=OK; else r1=FALLA; fallos=$((fallos+1)); fi

    # 2. el modo sin diccionario da el mismo censo
    c=$("$DTC" "$DATA/$g" --no-dict  2>/dev/null | censo)
    if [ -n "$c" ] && [ "$c" = "$a" ]; then r2=OK; else r2=FALLA; fallos=$((fallos+1)); fi

    # 3. disolver todo devuelve el censo al caso base
    if "$DTC" "$DATA/$g" --disolver 2>/dev/null | grep -q "Disolucion total .*: OK"; then
        r3=OK
    else
        r3=FALLA; fallos=$((fallos+1))
    fi

    # 4. disolver un subconjunto y comparar contra un censo estatico del grafo
    #    remanente. Es el unico test fuerte del camino s = -1: disolver TODO
    #    puede pasar aunque haya errores intermedios que se cancelen.
    d=$("$DTC" "$DATA/$g" --disolver-parcial 7 "$TMP/rest.txt" 2>/dev/null | censo)
    e=$("$ORT" "$TMP/rest.txt" /dev/null 2>/dev/null | censo)
    if [ -n "$d" ] && [ "$d" = "$e" ]; then r4=OK; else r4=FALLA; fallos=$((fallos+1)); fi

    printf "%-40s %-11s %-9s %-11s %-11s\n" "$nombre" "$r1" "$r2" "$r3" "$r4"
done

echo
if [ "$fallos" -eq 0 ]; then
    echo "TODO OK"
else
    echo "$fallos comprobacion(es) fallaron"
fi
exit "$fallos"
