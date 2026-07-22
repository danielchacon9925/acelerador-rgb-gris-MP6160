#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_DIR"

mkdir -p hls/reports

echo "[HLS TB] compilando testbench C..."
g++ -std=c++17 -O2 -Ihls/src hls/src/img_accel_hls.cpp hls/tb/img_accel_tb.cpp -o hls/reports/img_accel_tb

echo "[HLS TB] ejecutando testbench..."
./hls/reports/img_accel_tb

echo "[HLS TB] salida generada en hls/reports/output_hls.raw"

if [ -f scripts/cross_check.sh ]; then
    echo "[HLS TB] ejecutando verificacion cruzada si la salida del VP esta disponible..."
    if [ -f images/output/output_1080p_gray.raw ]; then
        ./scripts/cross_check.sh hls/reports/output_hls.raw || true
    else
        echo "[HLS TB] omitiendo cross_check porque images/output/output_1080p_gray.raw no existe"
    fi
fi
