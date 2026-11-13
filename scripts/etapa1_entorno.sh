#!/bin/bash
# Etapa 1: estructura, permisos, enlaces y metadatos

umask 027

BASE="$(cd "$(dirname "$0")/.." && pwd)"
EVID="$BASE/evidencias"
PRUEBAS="$EVID/pruebas"

fallar() {
    echo "Error: $1" >&2
    exit 1
}

echo "Etapa 1"


# Estructura de directorios
mkdir -p "$BASE"/{capturas/estaticos,preparados,fuentes,scripts,resultados,evidencias/pruebas} \
    || fallar "No se pudo crear la estructura de directorios"
echo "Estructura creada"

# Limpieza de la prueba anterior
rm -f "$PRUEBAS"/* "$BASE/enlace_pruebas" "$EVID/link_record.txt" \
    || fallar "No se pudo limpiar la prueba anterior"


# Enlace simbólico al directorio de pruebas
ln -s evidencias/pruebas "$BASE/enlace_pruebas" || fallar "No se pudo crear el enlace al directorio"

# Archivo de prueba y enlaces
cd "$PRUEBAS" || fallar "No se pudo entrar a evidencias/pruebas"
uname -a > prueba_doc.txt || fallar "No se pudo crear prueba_doc.txt"
ln prueba_doc.txt hardlink.txt || fallar "No se pudo crear el enlace duro"
ln -s prueba_doc.txt simbolico_doc.txt || fallar "No se pudo crear el enlace simbólico"
echo "Enlaces creados"

# Metadatos antes del cambio
{ echo " ANTES DE RENOMBRAR "
    stat prueba_doc.txt hardlink.txt simbolico_doc.txt "$BASE/enlace_pruebas"
} > "$EVID/link_record.txt" || fallar "No se pudo escribir link_record.txt"

# Cambio de nombre y modificación del original
mv prueba_doc.txt prueba_doc_original.txt || fallar "No se pudo renombrar prueba_doc.txt"
echo "Linea agregada al original" >> prueba_doc_original.txt || fallar "No se pudo modificar el original"

# Metadatos después del cambio
{ echo " DESPUES DE RENOMBRAR Y MODIFICAR (prueba_doc.txt -> prueba_doc_original.txt) "
    stat prueba_doc_original.txt hardlink.txt simbolico_doc.txt "$BASE/enlace_pruebas"
} >> "$EVID/link_record.txt" || fallar "No se pudo completar link_record.txt"
echo "Metadatos registrados en evidencias/link_record.txt"

cd "$BASE" || fallar "No se pudo regresar a sec_guard"



# Permisos de directorios: 750
chmod 750 "$BASE" "$BASE"/{capturas,capturas/estaticos,preparados,fuentes,scripts,resultados,evidencias,evidencias/pruebas} \
    || fallar "No se pudieron asignar permisos a los directorios"

# Permisos de archivos de datos: 640 (los .sh y el binario no se tocan)
chmod 640 "$BASE/README.md" || fallar "No se pudo asignar permiso a README.md"
find "$BASE"/{capturas,preparados,fuentes,resultados,evidencias} -type f ! -name "sec_engine" -exec chmod 640 {} + \
    || fallar "No se pudieron asignar permisos a los archivos"
echo "Permisos asignados"

# Evidencia de permisos
{   echo "PERMISOS DE sec_guard"
    echo "La justificación está en README.md"
    echo ""
    ls -ld "$BASE"
    ls -lR "$BASE"
} > "$EVID/permissions.txt" || fallar "No se pudo escribir permissions.txt"
echo "Permisos documentados en evidencias/permissions.txt"

# Árbol de directorios
echo ""
cd "$BASE" && find . -not -path "./.git*" | sort | sed -e "s|[^/]*/|  |g" | tee "$EVID/arbol_directorios.txt"
