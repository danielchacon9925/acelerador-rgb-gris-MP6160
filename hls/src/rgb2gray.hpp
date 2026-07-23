
#pragma once

#include <ap_int.h>
#include <hls_stream.h>
#include <cstdint>

// dimensiones de la imagen, las mismas de accel_core.h del prototipo
// para que los dos trabajen con la misma imagen
constexpr int IMG_ANCHO = 1920;
constexpr int IMG_ALTO = 1080;
constexpr uint32_t IMG_PIXELS = IMG_ANCHO * IMG_ALTO;

// coeficientes de BT.601 por 256 para no usar decimales
// son los mismos del acelerador del prototipo virtual
constexpr int COEF_R = 77;
constexpr int COEF_G = 150;
constexpr int COEF_B = 29;

// un pixel rgb completo, es lo que viaja por el stream
struct rgb_pixel_t
{
  ap_uint<8> r;
  ap_uint<8> g;
  ap_uint<8> b;
};

// funcion top que sintetiza vitis
// in y out van a la RAM por AXI master y num_pixels por AXI lite
void rgb2gray(const ap_uint<8>* in, ap_uint<8>* out, uint32_t num_pixels);
