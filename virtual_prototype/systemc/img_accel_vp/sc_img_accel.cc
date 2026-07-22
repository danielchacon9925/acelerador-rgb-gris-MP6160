/*
 * sc_img_accel.cc -- Implementacion del acelerador RGB->gris para gem5.
 *
 * El puente gem5->TLM (Gem5ToTlmBridge32) entrega aqui las transacciones
 * generadas por los accesos MMIO de la CPU ARM64. Cada acceso es de 4 bytes
 * (un registro). El proceso de conversion corre en un SC_THREAD para que la
 * escritura de START retorne de inmediato y el sondeo de STATUS por parte de
 * la CPU pueda avanzar la simulacion (mismo patron que la Evaluacion 1).
 */

#include <iostream>

#include "base/trace.hh"
#include "params/ImgAccel.hh"
#include "sc_img_accel.hh"

#include "systemc/ext/systemc"
#include "systemc/ext/tlm"

ImgAccel::ImgAccel(sc_core::sc_module_name name,
                   const std::string& image_in,
                   const std::string& image_out)
    : sc_core::sc_module(name),
      tSocket("tSocket"),
      wrapper(tSocket, std::string(name) + ".tlm", gem5::InvalidPortID),
      ruta_in(image_in),
      ruta_out(image_out)
{
    tSocket.register_b_transport(this, &ImgAccel::b_transport);
    SC_THREAD(proceso);
    std::cout << "[ImgAccel] periferico en linea (in=" << ruta_in
              << ", out=" << ruta_out << ")\n";
}

void
ImgAccel::b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
{
    const tlm::tlm_command cmd = trans.get_command();
    const sc_dt::uint64    adr_raw = trans.get_address();
    // El puente puede entregar la direccion completa o el offset local segun
    // la version de gem5. La ventana es de 4 KiB, asi que enmascarar a 12 bits
    // hace que el decodificado funcione en ambos casos.
    const sc_dt::uint64    adr = adr_raw & 0xFFF;
    unsigned char*         ptr = trans.get_data_ptr();
    const unsigned int     len = trans.get_data_length();

    // Registros de 32 bits: solo aceptamos accesos alineados de 4 bytes.
    if (len != 4) {
        trans.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }

    if (cmd == tlm::TLM_WRITE_COMMAND) {
        std::uint32_t val;
        std::memcpy(&val, ptr, 4);
        switch (adr) {
            case acc_reg::DIR_IN:     reg_dir_in  = val; break;
            case acc_reg::DIR_OUT:    reg_dir_out = val; break;
            case acc_reg::NUM_PIXELS: reg_num_pix = val; break;
            case acc_reg::CTRL:
                if (val & acc_reg::CTRL_START) {
                    busy = true;
                    done = false;
                    ev_start.notify(sc_core::SC_ZERO_TIME);
                }
                break;
            default: break;  // registros RO o reservados: se ignora la escritura
        }
    } else {  // TLM_READ_COMMAND
        std::uint32_t val = 0;
        switch (adr) {
            case acc_reg::STATUS:
                val = (busy ? acc_reg::ST_BUSY : 0) |
                      (done ? acc_reg::ST_DONE : 0);
                break;
            case acc_reg::ID:         val = acc_reg::MAGIC_ID; break;
            case acc_reg::DIR_IN:     val = reg_dir_in;  break;
            case acc_reg::DIR_OUT:    val = reg_dir_out; break;
            case acc_reg::NUM_PIXELS: val = reg_num_pix; break;
            default:                  val = 0; break;
        }
        std::memcpy(ptr, &val, 4);
    }

    trans.set_response_status(tlm::TLM_OK_RESPONSE);
}

void
ImgAccel::proceso()
{
    std::vector<unsigned char> buf_rgb;
    std::vector<unsigned char> buf_gris(img::BYTES_GRIS);

    while (true) {
        sc_core::wait(ev_start);

        std::cout << "[ImgAccel] START recibido: "
                  << "dir_in=0x" << std::hex << reg_dir_in
                  << " dir_out=0x" << reg_dir_out
                  << " npix=" << std::dec << reg_num_pix << "\n";

        // 1) Cargar la imagen RGB desde el almacenamiento persistente.
        if (!accel_io::cargar(ruta_in, buf_rgb)) {
            std::cerr << "[ImgAccel] ERROR: no se pudo abrir " << ruta_in << "\n";
            busy = false; done = true;
            continue;
        }

        // 2) Convertir a escala de grises (BT.601 entero).
        const std::uint32_t npix =
            reg_num_pix ? reg_num_pix : img::PIXEL_TOTAL;
        buf_gris.resize(npix);
        accel::rgb_a_gris(buf_rgb.data(), buf_gris.data(), npix);

        // Modela la latencia del pipeline HW: ~1 pixel/ciclo @ 250 MHz
        // (periodo de reloj = 4 ns), coherente con la implementacion HLS.
        sc_core::wait(npix * 4.0, sc_core::SC_NS);

        // 3) Escribir el resultado al almacenamiento persistente.
        if (!accel_io::guardar(ruta_out, buf_gris.data(), buf_gris.size()))
            std::cerr << "[ImgAccel] ERROR: no se pudo escribir " << ruta_out << "\n";
        else
            std::cout << "[ImgAccel] imagen en grises escrita: " << ruta_out << "\n";

        busy = false;
        done = true;  // la CPU lo detecta por polling de STATUS
    }
}

// Puente entre la configuracion Python (parametros del SimObject) y el
// objeto SystemC: gem5 llama a create() para instanciar el modulo.
namespace gem5
{
ImgAccel *
ImgAccelParams::create() const
{
    return new ImgAccel(name.c_str(), image_in, image_out);
}
}  // namespace gem5

gem5::Port &
ImgAccel::gem5_getPort(const std::string &if_name, int idx)
{
    return wrapper;
}
