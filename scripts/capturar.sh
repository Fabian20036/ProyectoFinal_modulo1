#!/bin/bash
# Etapa 2: captura de mediciones del servidor
# Uso: capturar.sh [número de capturas]

umask 027
set -o pipefail

BASE="$(cd "$(dirname "$0")/.." && pwd)"
RAW="$BASE/capturas/telemetria_raw.log"
EVIDENCIA="$BASE/capturas/evidencia_capturas.log"
CAPTURAS="${1:-3}"

> "$RAW"
> "$EVIDENCIA"

for i in $(seq 1 "$CAPTURAS"); do
    FECHA=$(date '+%Y-%m-%d %H:%M:%S')

    # Mediciones; si un comando falla, se registra ERROR en lugar de un número
    DISCO=$(df -P "$HOME" | awk 'NR == 2 { print $5 }' | tr -d '%') || DISCO="ERROR"
    PROCESOS=$(ps -u "$USER" --no-headers | wc -l) || PROCESOS="ERROR"
    PERMISOS=$(find "$BASE" -type f -perm /022 -not -path "$BASE/.git/*" | wc -l) || PERMISOS="ERROR"

    # Registro
    echo "$FECHA DISCO $DISCO" >> "$RAW"
    echo "$FECHA PROCESOS $PROCESOS" >> "$RAW"
    echo "$FECHA PERMISOS $PERMISOS" >> "$RAW"

    # Evidencia: salida original de los comandos
    echo " Captura $i ($FECHA) " >> "$EVIDENCIA"
    df -P "$HOME" >> "$EVIDENCIA"
    ps -u "$USER" >> "$EVIDENCIA"

    echo "Captura $i: DISCO=$DISCO PROCESOS=$PROCESOS PERMISOS=$PERMISOS"
    sleep 2
done

echo "Mediciones guardadas en capturas/telemetria_raw.log"
