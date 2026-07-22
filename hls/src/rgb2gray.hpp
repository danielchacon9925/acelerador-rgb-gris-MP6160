// SPDX-License-Identifier: MIT
// rgb2gray.hpp -- Contrato del acelerador HLS RGB -> escala de grises.
//
// Concentra las constantes compartidas y la firma del kernel sintetizable.
// Los coeficientes de conversion son EXACTAMENTE los mismos que usa el
// prototipo virtual en virtual_prototype/systemc/img_accel_vp/accel_core.h:
//
//     Y = (77*R + 150*G + 29*B) >> 8      (luminancia BT.601, entero)
//
// Mantener aritmetica entera (sin float / ap_fixed) es lo que garantiza la
// equivalencia bit a bit con el modelo de referencia (Eval 1) y con el VP.

#pragma once

#include <ap_int.h>
#include <hls_stream.h>
#include <cstdint>

// --- Parametros de la imagen (identicos a accel_core.h) ----------------
constexpr int      IMG_ANCHO  = 1920;
constexpr int      IMG_ALTO   = 1080;
constexpr uint32_t IMG_PIXELS = IMG_ANCHO * IMG_ALTO;  // 2 073 600

// --- Coeficientes BT.601 en entero (77,150,29 = round(coef * 256)) ------
constexpr int COEF_R = 77;
constexpr int COEF_G = 150;
constexpr int COEF_B = 29;

// Pixel RGB empaquetado que viaja por el stream interno del pipeline.
struct rgb_pixel_t {
    ap_uint<8> r;
    ap_uint<8> g;
    ap_uint<8> b;
};

// Kernel top-level. 'in' y 'out' son punteros a la RAM del sistema
// (interfaz AXI4 Master); 'num_pixels' es un escalar de control (AXI4-Lite).
void rgb2gray(const ap_uint<8>* in, ap_uint<8>* out, uint32_t num_pixels);
