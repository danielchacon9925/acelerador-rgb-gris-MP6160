#!/usr/bin/env bash
# run_vp.sh -- Corre el prototipo virtual completo (ARM64 + acelerador SystemC).
#
# Lanza gem5 con vp_arm.py, el ELF bare-metal y las rutas de imagen. La imagen
# de salida en grises la produce el acelerador SystemC durante la simulacion.
set -euo pipefail

VP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "$VP_DIR/.." && pwd)"
REPO_DIR="$VP_DIR"
GEM5_DIR="${GEM5_DIR:-$ROOT_DIR/gem5}"
GEM5_BIN="$GEM5_DIR/build/ARM/gem5.opt"
CONFIG="$GEM5_DIR/configs/example/arm/vp_arm.py"
ELF="$REPO_DIR/sw/accel_app.elf"

IMG_IN="${IMG_IN:-$ROOT_DIR/images/input/input_1080p.rgb}"
IMG_OUT="${IMG_OUT:-$ROOT_DIR/images/output/output_1080p_gray.raw}"

[ -x "$GEM5_BIN" ] || { echo "Falta $GEM5_BIN. Corre scripts/build_gem5.sh primero." >&2; exit 1; }
[ -f "$ELF" ]      || { echo "Falta $ELF. Corre scripts/build_sw.sh primero." >&2; exit 1; }
[ -f "$CONFIG" ]   || { echo "Falta $CONFIG (build_gem5.sh lo copia)." >&2; exit 1; }

mkdir -p "$(dirname "$IMG_OUT")"

echo ">> Ejecutando gem5..."
"$GEM5_BIN" "$CONFIG" \
    --kernel "$ELF" \
    --image-in "$IMG_IN" \
    --image-out "$IMG_OUT"

echo ">> Salida generada: $IMG_OUT"
ls -l "$IMG_OUT" 2>/dev/null || echo "  (no se genero la imagen; revisar la corrida)"
