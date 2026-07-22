#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_DIR"

if ! command -v vitis_hls >/dev/null 2>&1; then
    echo "[HLS COSIM] vitis_hls no encontrado en PATH. Se requiere Vitis HLS para ejecutar la co-simulación." >&2
    exit 2
fi

echo "[HLS COSIM] lanzando flujo de Vitis HLS..."
vitis_hls -f hls/scripts/run_hls.tcl

echo "[HLS COSIM] flujo completado. Revisar hls/reports y hls/build"
