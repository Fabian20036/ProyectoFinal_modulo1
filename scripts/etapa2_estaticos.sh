#!/bin/bash

# Etapa 2: auditoría del paquete de registros

umask 027

BASE="$(cd "$(dirname "$0")/.." && pwd)"
ESTATICOS="$BASE/capturas/estaticos"
PREP="$BASE/preparados"
PAQUETE="paquete_auditoria.tar.gz"

fallar() {
    echo "Error: $1" >&2
    exit 1
}

echo "Etapa 2: auditoría del paquete"

mkdir -p "$ESTATICOS" "$PREP" || fallar "No se pudieron crear las carpetas"

# Mover el paquete de la raíz a capturas/estaticos
if [ -f "$BASE/$PAQUETE" ]; then
    mv -f "$BASE/$PAQUETE" "$ESTATICOS/" || fallar "No se pudo mover el paquete"
    chmod 640 "$ESTATICOS/$PAQUETE"
fi
[ -f "$ESTATICOS/$PAQUETE" ] || fallar "No se encontró $PAQUETE"
echo "Paquete en capturas/estaticos"

# Validar integridad
gzip -t "$ESTATICOS/$PAQUETE" || fallar "El paquete está dañado (gzip)"
LISTA=$(tar -tzf "$ESTATICOS/$PAQUETE") || fallar "El paquete está dañado (tar)"
echo "Integridad verificada"

# Extraer
tar -xzf "$ESTATICOS/$PAQUETE" -C "$ESTATICOS" || fallar "No se pudo extraer el paquete"
TOTAL=$(echo "$LISTA" | wc -l)
[ "$TOTAL" -ge 3 ] || fallar "El paquete tiene $TOTAL archivos; se esperaban 3"
echo "Extraídos $TOTAL archivos"


# sshd_config: habilitar Port 8080 y HostKey, quitar comentarios y líneas vacías
sed -E \
    -e 's/^#?Port[[:space:]]+[0-9]+/Port 8080/' \
    -e 's/^#(HostKey[[:space:]])/\1/' \
    -e '/^[[:space:]]*#/d' \
    -e '/^[[:space:]]*$/d' \
    "$ESTATICOS/sshd_config" > "$PREP/sshd_activo.txt" || fallar "No se pudo procesar sshd_config"
echo "Generado preparados/sshd_activo.txt"


# secure.log: usuarios creados (nombre, UID, GID)
{
    echo "USUARIO UID GID"
    grep "new user:" "$ESTATICOS/secure.log" \
        | sed -E 's/.*name=([^,]+), UID=([0-9]+), GID=([0-9]+).*/\1 \2 \3/'
} | column -t > "$PREP/usuarios_creados.txt" || fallar "No se pudo procesar secure.log"
echo "Generado preparados/usuarios_creados.txt"


# net_traffic.log: inicios de conexión TCP (IP y puerto de origen y destino)
{   echo "IP_ORIGEN PUERTO_ORIGEN IP_DESTINO PUERTO_DESTINO"
    grep "Flags \[S\]," "$ESTATICOS/net_traffic.log" \
        | awk '{ print $2, $4 }' \
        | sed -E -e 's/:$//' -e 's/\.([0-9]+)( |$)/ \1\2/g'
} | column -t > "$PREP/conexiones_syn.txt" || fallar "No se pudo procesar net_traffic.log"
echo "Generado preparados/conexiones_syn.txt"
