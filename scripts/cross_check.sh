#!/usr/bin/env bash
# cross_check.sh -- Verificacion cruzada HLS <-> Prototipo Virtual <-> Eval 1.
#
# Compara la imagen en escala de grises producida por las tres implementaciones.
# Si son identicas, la equivalencia funcional queda demostrada. Si no lo son,
# reporta cuantos pixeles difieren y el error absoluto maximo, para poder
# documentar la discrepancia (tipicamente causada por usar punto flotante en HLS
# en vez de la aritmetica entera BT.601 del modelo de referencia).
#
# Uso:
#   ./scripts/cross_check.sh <salida_del_testbench_HLS.raw>
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

REF="$REPO_DIR/docs/reference_output_gray.raw"
VP="$REPO_DIR/images/output/output_1080p_gray.raw"
HLS="${1:-}"

if [ -z "$HLS" ]; then
    echo "Uso: $0 <salida_del_testbench_HLS.raw>" >&2
    echo "" >&2
    echo "Compara la salida del HLS contra la referencia (Eval 1) y la del" >&2
    echo "prototipo virtual. Las tres deben ser identicas bit a bit." >&2
    exit 1
fi

fail=0
for f in "$REF" "$VP" "$HLS"; do
    if [ ! -f "$f" ]; then
        echo "ERROR: no existe $f" >&2
        [ "$f" = "$VP" ] && echo "       (corre ./scripts/run_vp.sh primero)" >&2
        fail=1
    fi
done
[ "$fail" -eq 0 ] || exit 1

echo "=============================================================="
echo " Verificacion cruzada: Eval 1  <->  Prototipo Virtual  <->  HLS"
echo "=============================================================="
echo ""
echo "Tamanos (esperado: 2073600 bytes = 1920 x 1080 x 1):"
for f in "$REF" "$VP" "$HLS"; do
    printf "  %10s  %s\n" "$(stat -c%s "$f")" "$(basename "$f")"
done
echo ""

echo "MD5:"
md5sum "$REF" "$VP" "$HLS" | sed 's/^/  /'
echo ""

# --- Comparaciones por pares -------------------------------------------
compara() {
    local a="$1" b="$2" etiqueta="$3"
    if cmp -s "$a" "$b"; then
        echo "  [OK]   $etiqueta : identicas bit a bit"
        return 0
    fi
    echo "  [DIFF] $etiqueta : NO son identicas"
    # Reportar magnitud de la diferencia con python (byte a byte)
    python3 - "$a" "$b" <<'PY'
import sys
a = open(sys.argv[1], 'rb').read()
b = open(sys.argv[2], 'rb').read()
n = min(len(a), len(b))
dif = [abs(a[i] - b[i]) for i in range(n) if a[i] != b[i]]
if dif:
    print(f"         pixeles distintos : {len(dif)} de {n} "
          f"({100.0*len(dif)/n:.4f} %)")
    print(f"         error absoluto max: {max(dif)}")
    print(f"         error absoluto med: {sum(dif)/len(dif):.4f}")
if len(a) != len(b):
    print(f"         AVISO: tamanos distintos ({len(a)} vs {len(b)})")
PY
    return 1
}

echo "Comparaciones:"
ok=0
compara "$REF" "$VP"  "Eval 1  vs  Prototipo Virtual" || ok=1
compara "$REF" "$HLS" "Eval 1  vs  HLS             " || ok=1
compara "$VP"  "$HLS" "Prot.Virtual  vs  HLS       " || ok=1
echo ""

if [ "$ok" -eq 0 ]; then
    echo "=============================================================="
    echo " RESULTADO: las tres implementaciones son BIT-EXACTAS."
    echo " Equivalencia funcional demostrada."
    echo "=============================================================="
else
    echo "=============================================================="
    echo " RESULTADO: hay diferencias."
    echo ""
    echo " Causa mas probable: la implementacion HLS no usa la misma"
    echo " aritmetica entera que el modelo de referencia. Debe ser:"
    echo ""
    echo "     gris = (77*R + 150*G + 29*B) >> 8;"
    echo ""
    echo " Si el HLS usa float/double/ap_fixed o los coeficientes"
    echo " 0.299/0.587/0.114, habra diferencias de redondeo."
    echo "=============================================================="
    exit 1
fi
