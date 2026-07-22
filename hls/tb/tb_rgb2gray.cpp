// SPDX-License-Identifier: MIT
// tb_rgb2gray.cpp -- Testbench en C para co-simulacion (C/RTL) del acelerador.
//
// Reproduce el flujo del enunciado desde el lado del "CPU":
//   1. Carga la imagen RAW RGB de entrada desde disco a un buffer (la RAM).
//   2. Invoca el kernel rgb2gray (equivale a arrancarlo por AXI4-Lite).
//   3. Escribe la imagen en grises resultante a disco (para cross_check.sh).
//   4. Compara byte a byte contra la referencia de la Eval 1; el resultado del
//      testbench (return 0/1) es lo que evalua la co-simulacion.
//
// Uso (argv opcional; hay defaults relativos al repo):
//   tb  <input.rgb>  <reference_gray.raw>  <output_gray.raw>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "rgb2gray.hpp"

static bool cargar(const char* ruta, std::vector<unsigned char>& out)
{
    FILE* f = std::fopen(ruta, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? static_cast<size_t>(n) : 0);
    const size_t leidos = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    std::fclose(f);
    return leidos == out.size();
}

static bool guardar(const char* ruta, const unsigned char* d, size_t n)
{
    FILE* f = std::fopen(ruta, "wb");
    if (!f) return false;
    const size_t esc = std::fwrite(d, 1, n, f);
    std::fclose(f);
    return esc == n;
}

int main(int argc, char** argv)
{
    const char* ruta_in  = (argc > 1) ? argv[1] : "../../../../images/input/input_1080p.rgb";
    const char* ruta_ref = (argc > 2) ? argv[2] : "../../../../docs/reference_output_gray.raw";
    const char* ruta_out = (argc > 3) ? argv[3] : "output_1080p_gray_hls.raw";

    // 1) Cargar la imagen RGB de entrada.
    std::vector<unsigned char> rgb;
    if (!cargar(ruta_in, rgb)) {
        std::printf("[TB] ERROR: no se pudo abrir la entrada %s\n", ruta_in);
        return 1;
    }
    const uint32_t npix = static_cast<uint32_t>(rgb.size() / 3);
    if (npix != IMG_PIXELS)
        std::printf("[TB] AVISO: npix=%u (esperado %u)\n", npix, IMG_PIXELS);

    // 2) Invocar el kernel (la RAM del sistema se modela con los buffers).
    std::vector<unsigned char> gris(npix, 0);
    rgb2gray(reinterpret_cast<const ap_uint<8>*>(rgb.data()),
             reinterpret_cast<ap_uint<8>*>(gris.data()),
             npix);

    // 3) Guardar el resultado (para verificacion cruzada con el VP / Eval 1).
    if (!guardar(ruta_out, gris.data(), gris.size()))
        std::printf("[TB] AVISO: no se pudo escribir %s\n", ruta_out);
    else
        std::printf("[TB] salida escrita: %s (%zu bytes)\n", ruta_out, gris.size());

    // 4) Comparar contra la referencia (criterio bit a bit).
    std::vector<unsigned char> ref;
    if (!cargar(ruta_ref, ref)) {
        std::printf("[TB] ERROR: no se pudo abrir la referencia %s\n", ruta_ref);
        return 1;
    }
    if (ref.size() != gris.size()) {
        std::printf("[TB] FALLO: tamanos distintos (out=%zu ref=%zu)\n",
                    gris.size(), ref.size());
        return 1;
    }

    size_t difs = 0;
    int max_abs = 0;
    for (size_t i = 0; i < gris.size(); ++i) {
        const int d = std::abs(int(gris[i]) - int(ref[i]));
        if (d) { ++difs; if (d > max_abs) max_abs = d; }
    }

    if (difs == 0) {
        std::printf("[TB] OK: salida identica bit a bit a la referencia (%zu bytes).\n",
                    gris.size());
        return 0;
    }
    std::printf("[TB] FALLO: %zu pixeles distintos (%.4f %%), error abs max = %d\n",
                difs, 100.0 * difs / gris.size(), max_abs);
    return 1;
}
