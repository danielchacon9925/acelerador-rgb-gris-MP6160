// SPDX-License-Identifier: MIT
// accel_io.h  --  Almacenamiento persistente modelado como archivos del host
//
// La Evaluacion 1 modela el "disco" como un modulo SystemC (storage.h) que
// lee/escribe una carpeta del computador. En el prototipo virtual el
// acelerador es autonomo respecto a los datos: al recibir START carga la
// imagen de entrada desde el archivo RAW RGB y, al terminar, escribe la
// imagen en grises a otro archivo. La CPU ARM64 solo orquesta el periferico
// por MMIO (no mueve los 6 MB de pixeles), lo cual mantiene el trafico TLM
// del prototipo enfocado en el control del dispositivo.
//
// Este header no depende de SystemC ni de gem5: son utilidades de archivo.

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace accel_io
{
// Carga un archivo binario completo en un vector. Devuelve false si falla.
inline bool cargar(const std::string& ruta, std::vector<unsigned char>& out)
{
    std::ifstream f(ruta, std::ios::binary | std::ios::ate);
    if (!f) return false;
    const std::streamsize tam = f.tellg();
    f.seekg(0);
    out.resize(static_cast<std::size_t>(tam));
    f.read(reinterpret_cast<char*>(out.data()), tam);
    return static_cast<bool>(f);
}

// Escribe un buffer completo a disco (trunca si existe).
inline bool guardar(const std::string& ruta,
                    const unsigned char* data, std::size_t len)
{
    std::ofstream f(ruta, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(data),
            static_cast<std::streamsize>(len));
    return static_cast<bool>(f);
}
}  // namespace accel_io
