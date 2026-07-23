
#include "rgb2gray.hpp"

// el enunciado pide separar la entrada/salida de datos del procesamiento
// entonces parti el acelerador en 3 funciones conectadas por streams
// con DATAFLOW corren las 3 al mismo tiempo y queda el pipeline

// etapa de entrada: lee cada pixel rgb de la RAM y lo mete al stream
// cada pixel son 3 bytes seguidos por eso el 3*i
static void load_rgb(const ap_uint<8>* in,
                     hls::stream<rgb_pixel_t>& s_rgb,
                     uint32_t num_pixels)
{
load_loop:
  for (uint32_t i = 0; i < num_pixels; ++i)
  {
#pragma HLS PIPELINE II=1
    rgb_pixel_t p;
    p.r = in[3*i+0];
    p.g = in[3*i+1];
    p.b = in[3*i+2];
    s_rgb.write(p);
  }
}

// etapa de computo: convierte rgb a gris con BT.601
// multiplico los pesos por 256 para evitar decimales
// y al final divido con >>8 que es igual a dividir por 256
// con 16 bits alcanza para la suma, lo revise a mano
static void compute_gray(hls::stream<rgb_pixel_t>& s_rgb,
                         hls::stream<ap_uint<8> >& s_gray,
                         uint32_t num_pixels)
{
compute_loop:
  for (uint32_t i = 0; i < num_pixels; ++i)
  {
#pragma HLS PIPELINE II=1
    const rgb_pixel_t p = s_rgb.read();
    const ap_uint<16> acc = COEF_R * p.r + COEF_G * p.g + COEF_B * p.b;
    const ap_uint<8> y = acc >> 8;
    s_gray.write(y);
  }
}

// etapa de salida: saca el gris del stream y lo escribe a la RAM
// aqui es 1 byte por pixel nada mas
static void store_gray(ap_uint<8>* out,
                       hls::stream<ap_uint<8> >& s_gray,
                       uint32_t num_pixels)
{
store_loop:
  for (uint32_t i = 0; i < num_pixels; ++i)
  {
#pragma HLS PIPELINE II=1
    out[i] = s_gray.read();
  }
}

// el depth solo lo usa la cosim para armar su memoria, no cambia el hardware
// el tcl lo baja al tamano del recorte porque si no la cosim dura horas
#ifndef DEPTH_IN
#define DEPTH_IN 6220800   // 1920*1080*3
#endif
#ifndef DEPTH_OUT
#define DEPTH_OUT 2073600  // 1920*1080
#endif

// funcion top del acelerador
// los datos van por AXI master y el control por AXI lite
// igual que los registros DIR_IN/DIR_OUT/NUM_PIXELS del prototipo virtual
void rgb2gray(const ap_uint<8>* in, ap_uint<8>* out, uint32_t num_pixels)
{
#pragma HLS INTERFACE m_axi     port=in  offset=slave bundle=gmem_in  depth=DEPTH_IN
#pragma HLS INTERFACE m_axi     port=out offset=slave bundle=gmem_out depth=DEPTH_OUT
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
