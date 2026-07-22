# run_hls.tcl -- Flujo completo de Vitis HLS para el acelerador RGB -> gris.
#
# Target : AMD Kria KV260 (SOM K26, part xck26-sfvc784-2LV-c)
# Reloj  : 250 MHz  (periodo = 4 ns)
# Flujo  : csim (C, 1080p) -> csynth (sintesis) -> cosim (C/RTL) -> export (IP)
#
# Nota sobre la co-simulacion: la cosim C/RTL de la imagen 1080p completa (~2 M
# pixeles => millones de ciclos en xsim) es impracticablemente lenta, asi que se
# co-simula un RECORTE de 8192 pixeles (hls/tb/vectors/). La cosim valida que el RTL
# generado coincide con el modelo en C; no requiere la imagen completa para ello.
# La csim si cubre los 1080p completos (comparacion bit a bit con la referencia).
#
# La 'depth' de los puertos m_axi es una directiva EXCLUSIVA de cosim (dimensiona el
# modelo de memoria) y NO cambia el RTL ni los reportes de sintesis. Aqui se
# sobreescribe (-DDEPTH_IN/-DDEPTH_OUT) al tamano del recorte para que la cosim no
# lea fuera del buffer. La csim ignora 'depth', por lo que sigue procesando 1080p.
#
# Ejecutar:
#   source <ruta>/Vitis_HLS/2024.x/settings64.sh
#   cd hls && vitis_hls -f scripts/run_hls.tcl

# --- Rutas (relativas a la ubicacion de este script) -------------------
set SCRIPT_DIR [file dirname [file normalize [info script]]]
set HLS_DIR    [file normalize [file join $SCRIPT_DIR ..]]
set REPO_DIR   [file normalize [file join $HLS_DIR ..]]

set INC "-I[file join $HLS_DIR src]"

# Vectores: imagen 1080p completa (csim) y recorte de 8192 px (cosim).
set IMG_IN   [file join $REPO_DIR images input  input_1080p.rgb]
set IMG_REF  [file join $REPO_DIR docs reference_output_gray.raw]
set IMG_OUT  [file join $REPO_DIR images output output_1080p_gray_hls.raw]
set CROP_IN  [file join $HLS_DIR tb vectors input_crop.rgb]
set CROP_REF [file join $HLS_DIR tb vectors ref_crop.raw]
set CROP_OUT [file join $HLS_DIR tb vectors out_crop_hls.raw]

# --- Proyecto y solucion -----------------------------------------------
open_project -reset rgb2gray_hls
set_top rgb2gray
# depth del m_axi reducida al recorte (solo afecta cosim; el RTL es el mismo).
add_files     [file join $HLS_DIR src rgb2gray.cpp] -cflags "$INC -DDEPTH_IN=24576 -DDEPTH_OUT=8192"
add_files -tb [file join $HLS_DIR tb  tb_rgb2gray.cpp] -cflags $INC

open_solution -reset "solution1" -flow_target vivado
set_part {xck26-sfvc784-2LV-c}
create_clock -period 4 -name default   ;# 250 MHz

# --- 1) Simulacion C (funcional, imagen 1080p completa, bit-exacta) ----
csim_design -argv "$IMG_IN $IMG_REF $IMG_OUT"

# --- 2) Sintesis de alto nivel -----------------------------------------
csynth_design

# --- 3) Co-simulacion C/RTL (recorte de 8192 px, tratable) -------------
cosim_design -argv "$CROP_IN $CROP_REF $CROP_OUT"

# --- 4) Exportar el IP para Vivado / integracion en la KV260 -----------
export_design -format ip_catalog -rtl verilog

exit
