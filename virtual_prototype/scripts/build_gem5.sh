#!/usr/bin/env bash
# build_gem5.sh -- Construye gem5 (ARM) con el acelerador SystemC integrado.
#
# Compila el modulo systemc/img_accel_vp dentro de gem5 mediante EXTRAS y deja
# la config vp_arm.py accesible en configs/example/arm/. Ejecutar desde
# cualquier lugar; usa rutas relativas al repo.
#
# Variables opcionales:
#   GEM5_DIR   ruta a un clon existente de gem5 (por defecto: ./gem5 junto al repo)
#   JOBS       paralelismo de compilacion (por defecto: nproc)
set -euo pipefail

VP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROOT_DIR="$(cd "$VP_DIR/.." && pwd)"
REPO_DIR="$VP_DIR"
GEM5_DIR="${GEM5_DIR:-$ROOT_DIR/gem5}"
JOBS="${JOBS:-$(nproc)}"
EXTRAS_DIR="$REPO_DIR/systemc/img_accel_vp"

echo ">> Repo:   $REPO_DIR"
echo ">> gem5:   $GEM5_DIR"
echo ">> EXTRAS: $EXTRAS_DIR"

# 1) Obtener gem5 si no existe.
if [ ! -d "$GEM5_DIR" ]; then
    echo ">> Clonando gem5 (stable)..."
    git clone https://github.com/gem5/gem5.git "$GEM5_DIR"
fi

# 2) Dependencias tipicas (Debian/Ubuntu). Descomentar si hace falta:
# sudo apt-get install -y build-essential scons python3-dev git m4 \
#     zlib1g-dev libprotobuf-dev protobuf-compiler libboost-all-dev \
#     libhdf5-dev pkg-config

# 3) Compilar gem5.opt para ARM con el acelerador como EXTRAS.
cd "$GEM5_DIR"
echo ">> Compilando build/ARM/gem5.opt (esto tarda)..."
scons "build/ARM/gem5.opt" EXTRAS="$EXTRAS_DIR" -j"$JOBS"

# 4) Poner la config al alcance de los imports de gem5 (devices, workloads).
cp "$REPO_DIR/gem5-config/vp_arm.py" "$GEM5_DIR/configs/example/arm/vp_arm.py"

echo ">> Listo. Binario: $GEM5_DIR/build/ARM/gem5.opt"
echo ">> Config:        $GEM5_DIR/configs/example/arm/vp_arm.py"
