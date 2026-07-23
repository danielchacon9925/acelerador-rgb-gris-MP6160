
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "rgb2gray.hpp"

// testbench para csim y cosim
// hace lo mismo que el CPU del prototipo: carga la imagen, llama al kernel,
// guarda el resultado y lo compara contra la referencia de la eval 1
// si algo no calza retorna 1 y la cosim falla
//
// uso: tb <input.rgb> <reference_gray.raw> <output_gray.raw>

// lee un archivo completo a un vector
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

// escribe un buffer completo a un archivo
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
  // si no pasan argumentos se usan las rutas del repo
  const char* ruta_in = (argc > 1) ? argv[1] : "../../../../images/input/input_1080p.rgb";
  const char* ruta_ref = (argc > 2) ? argv[2] : "../../../../docs/reference_output_gray.raw";
  const char* ruta_out = (argc > 3) ? argv[3] : "output_1080p_gray_hls.raw";

  // 1) cargar la imagen rgb de entrada
  std::vector<unsigned char> rgb;
  if (!cargar(ruta_in, rgb))
  {
    std::printf("[TB] ERROR: no se pudo abrir la entrada %s\n", ruta_in);
    return 1;
  }
  const uint32_t npix = static_cast<uint32_t>(rgb.size() / 3);
  if (npix != IMG_PIXELS)
    std::printf("[TB] AVISO: npix=%u (esperado %u)\n", npix, IMG_PIXELS);

  // 2) llamar al kernel, los buffers hacen de RAM
  std::vector<unsigned char> gris(npix, 0);
  rgb2gray(reinterpret_cast<const ap_uint<8>*>(rgb.data()),
           reinterpret_cast<ap_uint<8>*>(gris.data()),
           npix);

  // 3) guardar el resultado para revisarlo contra el prototipo
  if (!guardar(ruta_out, gris.data(), gris.size()))
    std::printf("[TB] AVISO: no se pudo escribir %s\n", ruta_out);
  else
    std::printf("[TB] salida escrita: %s (%zu bytes)\n", ruta_out, gris.size());

  // 4) comparar contra la referencia, tiene que dar identico bit a bit
  std::vector<unsigned char> ref;
  if (!cargar(ruta_ref, ref))
  {
    std::printf("[TB] ERROR: no se pudo abrir la referencia %s\n", ruta_ref);
    return 1;
  }
  if (ref.size() != gris.size())
  {
    std::printf("[TB] FALLO: tamanos distintos (out=%zu ref=%zu)\n",
                gris.size(), ref.size());
    return 1;
  }

  size_t difs = 0;
  int max_abs = 0;
  for (size_t i = 0; i < gris.size(); ++i)
  {
    const int d = std::abs(int(gris[i]) - int(ref[i]));
    if (d) { ++difs; if (d > max_abs) max_abs = d; }
  }

  if (difs == 0)
  {
    std::printf("[TB] OK: salida identica a la referencia (%zu bytes)\n",
                gris.size());
    return 0;
  }
  std::printf("[TB] FALLO: %zu pixeles distintos (%.4f %%), error abs max = %d\n",
              difs, 100.0 * difs / gris.size(), max_abs);
  return 1;
}
