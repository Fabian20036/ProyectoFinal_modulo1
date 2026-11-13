#!/bin/bash
# Etapa 2: normalización de las mediciones para el programa en C
# Uso: normalizar.sh [etiqueta]

umask 027

BASE="$(cd "$(dirname "$0")/.." && pwd)"
RAW="$BASE/capturas/telemetria_raw.log"
SALIDA="$BASE/preparados/mediciones_sec.txt"
TEMPORAL="$BASE/preparados/registros.tmp"
ETIQUETA="${1:-AuditoriaSecGuard}"

fallar() {
    echo "Error: $1" >&2
    exit 1
}

[ -f "$RAW" ] || fallar "No existe telemetria_raw.log"
[ "${#ETIQUETA}" -le 31 ] || fallar "La etiqueta tiene más de 31 caracteres"
[[ "$ETIQUETA" != *" "* ]] || fallar "La etiqueta no puede tener espacios"

# Quitar las capturas fallidas y dejar solo CATEGORIA VALOR
grep -v "ERROR" "$RAW" | awk '{ print $3, $4 }' > "$TEMPORAL"
TOTAL=$(wc -l < "$TEMPORAL")

FALLIDAS=$(grep -c "ERROR" "$RAW")
if [ "$FALLIDAS" -gt 0 ]; then
    echo "Aviso: se descartaron $FALLIDAS capturas fallidas" >&2
fi

# Archivo final: etiqueta, cantidad y registros
{
    echo "$ETIQUETA"
    echo "$TOTAL"
    cat "$TEMPORAL"
} > "$SALIDA"
rm "$TEMPORAL"

# Comprobar que la cantidad declarada coincide con los registros
ESCRITOS=$(tail -n +3 "$SALIDA" | wc -l)
[ "$ESCRITOS" -eq "$TOTAL" ] || fallar "Se declararon $TOTAL registros, pero hay $ESCRITOS"

echo "Generado preparados/mediciones_sec.txt: $ETIQUETA, $TOTAL registros"
