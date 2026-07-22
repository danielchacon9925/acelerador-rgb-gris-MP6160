#!/usr/bin/env bash
# build_sw.sh -- Compila el programa en C bare-metal para ARM64.
#
# Requiere un cross-compiler aarch64 (aarch64-none-elf-gcc o, en su defecto,
# aarch64-linux-gnu-gcc usado en modo freestanding). Genera sw/accel_app.elf.
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_DIR/sw"

if ! command -v aarch64-none-elf-gcc >/dev/null 2>&1 \
   && ! command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
    echo "ERROR: no se encontro un cross-compiler aarch64." >&2
    echo "  Debian/Ubuntu: sudo apt-get install gcc-aarch64-linux-gnu" >&2
    exit 1
fi

make clean
make
echo ">> Compilado: $REPO_DIR/sw/accel_app.elf"
aarch64-linux-gnu-readelf -h accel_app.elf 2>/dev/null \
    | grep -E "Machine|Entry point" || true
