#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../src/img_accel_hls.h"

int main(int argc, char** argv)
{
    const std::string input_path = (argc > 1) ? argv[1] : "images/input/input_1080p.rgb";
    const std::string output_path = (argc > 2) ? argv[2] : "hls/reports/output_hls.raw";
    const std::string reference_path = (argc > 3) ? argv[3] : "docs/reference_output_gray.raw";

    std::ifstream in(input_path, std::ios::binary | std::ios::ate);
    if (!in) {
        std::cerr << "No se pudo abrir la imagen de entrada: " << input_path << "\n";
        return 1;
    }

    const std::streamsize bytes = in.tellg();
    in.seekg(0);
    std::vector<unsigned char> rgb(static_cast<std::size_t>(bytes));
    in.read(reinterpret_cast<char*>(rgb.data()), bytes);

    if (bytes % 3 != 0) {
        std::cerr << "Tamanio de entrada invalido: " << bytes << " bytes\n";
        return 2;
    }

    const unsigned int num_pixels = static_cast<unsigned int>(bytes / 3);
    std::vector<unsigned char> gray(num_pixels);

    rgb_to_gray_top(rgb.data(), gray.data(), num_pixels);

    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "No se pudo escribir la salida HLS: " << output_path << "\n";
        return 3;
    }
    out.write(reinterpret_cast<const char*>(gray.data()), gray.size());
    out.close();

    std::ifstream ref(reference_path, std::ios::binary | std::ios::ate);
    if (!ref) {
        std::cerr << "No se encontro la referencia: " << reference_path << "\n";
        std::cout << "Salida generada en " << output_path << "\n";
        return 0;
    }

    const std::streamsize ref_bytes = ref.tellg();
    ref.seekg(0);
    std::vector<unsigned char> ref_data(static_cast<std::size_t>(ref_bytes));
    ref.read(reinterpret_cast<char*>(ref_data.data()), ref_bytes);

    if (ref_bytes != static_cast<std::streamsize>(gray.size())) {
        std::cerr << "Tamanio de referencia distinto: " << ref_bytes << " vs " << gray.size() << "\n";
        return 4;
    }

    std::size_t mismatches = 0;
    for (std::size_t i = 0; i < gray.size(); ++i) {
        if (gray[i] != ref_data[i]) {
            ++mismatches;
        }
    }

    if (mismatches == 0) {
        std::cout << "[TB] Salida HLS coincidente con la referencia.\n";
        return 0;
    }

    std::cerr << "[TB] Se encontraron " << mismatches << " diferencias de " << gray.size() << " bytes.\n";
    return 5;
}
