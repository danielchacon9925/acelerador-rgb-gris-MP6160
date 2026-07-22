// SPDX-License-Identifier: MIT
// accel_core.h  --  Nucleo puro del acelerador (sin SystemC ni gem5)
//
// Este header concentra TODO lo que no depende de un framework de
// simulacion: el mapa de registros del periferico, las dimensiones de la
// imagen y la conversion RGB -> escala de grises. Lo incluyen tanto el
// modulo integrado en gem5 (sc_img_accel.hh) como el banco de pruebas
// standalone (verify/tb_standalone.cpp), de modo que la logica verificada
// aqui es EXACTAMENTE la misma que corre en el prototipo virtual.

#pragma once

#include <cstdint>

namespace img
{
// --- Parametros de la imagen (identicos a la Evaluacion 1) -------------
constexpr std::uint32_t ANCHO       = 1920;
constexpr std::uint32_t ALTO        = 1080;
constexpr std::uint32_t BPP_RGB     = 3;   // bytes por pixel de entrada
constexpr std::uint32_t BPP_GRIS    = 1;   // bytes por pixel de salida

constexpr std::uint32_t PIXEL_TOTAL = ANCHO * ALTO;        // 2 073 600
constexpr std::uint32_t BYTES_RGB   = PIXEL_TOTAL * BPP_RGB;  // 6 220 800
constexpr std::uint32_t BYTES_GRIS  = PIXEL_TOTAL * BPP_GRIS; // 2 073 600
}  // namespace img

// -----------------------------------------------------------------------
// Mapa de registros del periferico acelerador (AXI4-Lite style, 32 bits).
// Estos offsets son relativos a la direccion base del periferico en el
// bus del SoC (ACCEL_BASE, definida en la config de gem5). El programa en
// C que corre en la CPU ARM64 accede a estos offsets por MMIO; cada acceso
// se convierte en una transaccion TLM-2.0 a traves del puente de gem5.
// -----------------------------------------------------------------------
namespace acc_reg
{
constexpr std::uint64_t DIR_IN     = 0x00;  // RW  base de la imagen de entrada
constexpr std::uint64_t DIR_OUT    = 0x04;  // RW  base de la imagen de salida
constexpr std::uint64_t NUM_PIXELS = 0x08;  // RW  cantidad de pixeles a procesar
constexpr std::uint64_t CTRL       = 0x0C;  // WO  bit0 = START
constexpr std::uint64_t STATUS     = 0x10;  // RO  bit0 = BUSY, bit1 = DONE
constexpr std::uint64_t ID         = 0x14;  // RO  identificador magico

constexpr std::uint32_t CTRL_START = 0x1;
constexpr std::uint32_t ST_BUSY    = 0x1;
constexpr std::uint32_t ST_DONE    = 0x2;

constexpr std::uint32_t MAGIC_ID   = 0xACCE1111u;  // "ACCEl" + version
}  // namespace acc_reg

// -----------------------------------------------------------------------
// Conversion RGB -> escala de grises, luminancia BT.601 en aritmetica
// entera:  Y = (77*R + 150*G + 29*B) >> 8  ~=  0.299R + 0.587G + 0.114B
// Es identica a la de la Evaluacion 1 para garantizar que el prototipo
// virtual produce el mismo resultado bit a bit que el modelo SystemC
// original y que la implementacion HLS.
// -----------------------------------------------------------------------
namespace accel
{
inline void rgb_a_gris(const unsigned char* rgb,
                       unsigned char* gris,
                       std::uint32_t npix)
{
    for (std::uint32_t i = 0; i < npix; ++i)
    {
        const std::uint32_t r = rgb[3 * i + 0];
        const std::uint32_t g = rgb[3 * i + 1];
        const std::uint32_t b = rgb[3 * i + 2];
        gris[i] = static_cast<unsigned char>((77 * r + 150 * g + 29 * b) >> 8);
    }
}
}  // namespace accel
