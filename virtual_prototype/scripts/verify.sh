#!/usr/bin/env bash
# verify.sh -- Verificacion local del acelerador SIN gem5 (SystemC estandar).
#
# Compila y corre verify/tb_standalone.cpp, que reproduce el protocolo de
# registros que ejercera la CPU ARM64 y genera la imagen en grises. Compara el
# resultado con la referencia de la Evaluacion 1 (docs/reference_output_gray.raw).
#
# Requiere SystemC (libsystemc-dev). SYSTEMC_HOME opcional si no esta en /usr.
set -euo pipefail

VP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "$VP_DIR/.." && pwd)"
REPO_DIR="$VP_DIR"
cd "$REPO_DIR"

INC="-I systemc/img_accel_vp"
LIB="-lsystemc"
if [ -n "${SYSTEMC_HOME:-}" ]; then
    INC="$INC -I $SYSTEMC_HOME/include"
    LIB="-L $SYSTEMC_HOME/lib -L $SYSTEMC_HOME/lib-linux64 $LIB"
    export LD_LIBRARY_PATH="$SYSTEMC_HOME/lib:$SYSTEMC_HOME/lib-linux64:${LD_LIBRARY_PATH:-}"
fi

echo ">> Compilando verificador..."
g++ -std=c++17 $INC verify/tb_standalone.cpp -o verify/tb_standalone $LIB

echo ">> Ejecutando..."
mkdir -p "$ROOT_DIR/images/output"
./verify/tb_standalone "$ROOT_DIR/images/input/input_1080p.rgb" "$ROOT_DIR/images/output/output_1080p_gray.raw"

echo ">> Comparando con la referencia..."
if cmp -s "$ROOT_DIR/images/output/output_1080p_gray.raw" "$ROOT_DIR/docs/reference_output_gray.raw"; then
    echo "OK: la salida es identica bit a bit a la referencia."
else
    echo "DIFERENCIA respecto a la referencia:" >&2
    cmp "$ROOT_DIR/images/output/output_1080p_gray.raw" "$ROOT_DIR/docs/reference_output_gray.raw" >&2 || true
    exit 1
fi
