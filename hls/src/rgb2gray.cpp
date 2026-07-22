// SPDX-License-Identifier: MIT
// rgb2gray.cpp -- Acelerador RGB -> escala de grises en HLS (Vitis).
//
// Arquitectura de hardware requerida por el enunciado: pipeline con SEPARACION
// clara entre las etapas de entrada/salida de datos y la etapa de procesamiento.
// Se implementa con HLS DATAFLOW y tres funciones encadenadas por streams:
//
//     in (AXI4 Master) --> [load_rgb] --s_rgb--> [compute_gray] --s_gray--> [store_gray] --> out (AXI4 Master)
//                          E/S de datos          PROCESAMIENTO              E/S de datos
//
//   * load_rgb    : lee la imagen RGB desde memoria (3 bytes/pixel).
//   * compute_gray: convierte a gris con BT.601 entero (etapa de computo pura).
//   * store_gray  : escribe la imagen en grises a memoria (1 byte/pixel).
//
// El control (arranque/fin, direcciones base y numero de pixeles) se expone por
// AXI4-Lite; el movimiento de datos por AXI4 Master. Esto corresponde al mapa de
// registros del prototipo virtual (DIR_IN/DIR_OUT/NUM_PIXELS + ap_start/ap_done).

#include "rgb2gray.hpp"

// -----------------------------------------------------------------------
// Etapa de ENTRADA: lee la imagen RGB desde la RAM y la empuja al stream.
// -----------------------------------------------------------------------
static void load_rgb(const ap_uint<8>* in,
                     hls::stream<rgb_pixel_t>& s_rgb,
                     uint32_t num_pixels)
{
load_loop:
    for (uint32_t i = 0; i < num_pixels; ++i) {
#pragma HLS PIPELINE II=1
        rgb_pixel_t p;
        p.r = in[3 * i + 0];
        p.g = in[3 * i + 1];
        p.b = in[3 * i + 2];
        s_rgb.write(p);
    }
}

// -----------------------------------------------------------------------
// Etapa de PROCESAMIENTO: luminancia BT.601 en aritmetica entera.
// Y = (77*R + 150*G + 29*B) >> 8.  El maximo de la suma es 255*256 = 65280,
// que cabe en 16 bits sin desbordamiento.
// -----------------------------------------------------------------------
static void compute_gray(hls::stream<rgb_pixel_t>& s_rgb,
                         hls::stream<ap_uint<8> >& s_gray,
                         uint32_t num_pixels)
{
compute_loop:
    for (uint32_t i = 0; i < num_pixels; ++i) {
#pragma HLS PIPELINE II=1
        const rgb_pixel_t p = s_rgb.read();
        const ap_uint<16> acc = COEF_R * p.r + COEF_G * p.g + COEF_B * p.b;
        const ap_uint<8>  y   = acc >> 8;
        s_gray.write(y);
    }
}

// -----------------------------------------------------------------------
// Etapa de SALIDA: extrae el gris del stream y lo escribe en la RAM.
// -----------------------------------------------------------------------
static void store_gray(ap_uint<8>* out,
                       hls::stream<ap_uint<8> >& s_gray,
                       uint32_t num_pixels)
{
store_loop:
    for (uint32_t i = 0; i < num_pixels; ++i) {
#pragma HLS PIPELINE II=1
        out[i] = s_gray.read();
    }
}

// -----------------------------------------------------------------------
// Top-level. Las tres etapas corren concurrentes bajo DATAFLOW; los streams
// desacoplan la E/S del computo, dando el pipeline solicitado.
// -----------------------------------------------------------------------
// La 'depth' del m_axi es una directiva EXCLUSIVA de co-simulacion: dimensiona el
// modelo de memoria que la cosim conecta al puerto AXI. NO limita el hardware (cuyo
// bound real es num_pixels) ni cambia el RTL sintetizado. Por defecto vale la imagen
// 1080p completa; para que la cosim del RTL sea tratable se sobreescribe con el
// tamano del vector de prueba (recorte) via -DDEPTH_IN=.. -DDEPTH_OUT=.. (ver TCL).
#ifndef DEPTH_IN
#define DEPTH_IN 6220800   // 1920*1080*3
#endif
#ifndef DEPTH_OUT
#define DEPTH_OUT 2073600  // 1920*1080
#endif

void rgb2gray(const ap_uint<8>* in, ap_uint<8>* out, uint32_t num_pixels)
{
    // Datos por AXI4 Master; el offset (direccion base) se programa por
    // AXI4-Lite (offset=slave), igual que DIR_IN/DIR_OUT del prototipo virtual.
#pragma HLS INTERFACE m_axi     port=in  offset=slave bundle=gmem_in  depth=DEPTH_IN
#pragma HLS INTERFACE m_axi     port=out offset=slave bundle=gmem_out depth=DEPTH_OUT
    // Control por AXI4-Lite: escalares + registro de arranque/fin (ap_ctrl).
#pragma HLS INTERFACE s_axilite port=in         bundle=control
#pragma HLS INTERFACE s_axilite port=out        bundle=control
#pragma HLS INTERFACE s_axilite port=num_pixels bundle=control
#pragma HLS INTERFACE s_axilite port=return     bundle=control

#pragma HLS DATAFLOW
    hls::stream<rgb_pixel_t> s_rgb("s_rgb");
    hls::stream<ap_uint<8> > s_gray("s_gray");
#pragma HLS STREAM variable=s_rgb  depth=64
#pragma HLS STREAM variable=s_gray depth=64

    load_rgb(in, s_rgb, num_pixels);
    compute_gray(s_rgb, s_gray, num_pixels);
    store_gray(out, s_gray, num_pixels);
}
