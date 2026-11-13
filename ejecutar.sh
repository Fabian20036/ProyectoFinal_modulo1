#!/bin/bash
# Etapa 4: ejecuta todo el proyecto de principio a fin

set -e
umask 027
cd "$(dirname "$0")"
trap 'echo " Flujo detenido: hubo un error en el paso anterior "' ERR

echo " Limpieza "
rm -f resultados/informe_seguridad.txt fuentes/sec_engine

echo " Etapa 1 "
./scripts/etapa1_entorno.sh

echo " Etapa 2 "
./scripts/etapa2_estaticos.sh
./scripts/capturar.sh "${1:-3}"
./scripts/normalizar.sh "${2:-AuditoriaSecGuard}"

echo " Etapa 3 "
gcc -o fuentes/sec_engine fuentes/sec_engine.c
./fuentes/sec_engine preparados/mediciones_sec.txt resultados/informe_seguridad.txt

echo " Etapa 4: proceso de prueba "
sleep 300 &
PID=$!
{   echo "Proceso: sleep 300, PID $PID"
    echo ""
    echo "Iniciado (S = dormido):"
    ps -o pid,stat,cmd -p $PID
    kill -STOP $PID
    sleep 1
    echo ""
    echo "Suspendido con kill -STOP (T = detenido):"
    ps -o pid,stat,cmd -p $PID
    kill -CONT $PID
    sleep 1
    echo ""
    echo "Reanudado con kill -CONT (S = dormido):"
    ps -o pid,stat,cmd -p $PID
    kill -TERM $PID
    wait $PID || true
    echo ""
    echo "Terminado con kill -TERM:"
    ps -p $PID || echo "El proceso $PID ya no existe"
} > evidencias/pruebas/admin_process.txt
echo "Registro en evidencias/pruebas/admin_process.txt"

echo " Historial "
cp resultados/informe_seguridad.txt "resultados/informe_$(date +%Y%m%d_%H%M%S).txt"
ls resultados

echo ""
cat resultados/informe_seguridad.txt
