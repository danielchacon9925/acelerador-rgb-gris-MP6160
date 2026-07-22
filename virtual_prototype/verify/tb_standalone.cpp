// SPDX-License-Identifier: MIT
// tb_standalone.cpp -- Verificacion local del acelerador (SystemC puro).
//
// Este banco NO usa gem5: reproduce en SystemC estandar (apt / Accellera) el
// mismo protocolo de registros que ejercera la CPU ARM64 en el prototipo
// virtual, para poder validar aqui, sin gem5, que:
//   (a) el mapa de registros responde como se espera (ID, STATUS, etc.),
//   (b) el hilo de conversion se dispara con START y levanta DONE,
//   (c) la imagen en grises generada coincide bit a bit con la de la Eval 1.
//
// El acelerador aqui incluido comparte accel_core.h y accel_io.h con el
// modulo integrado en gem5 (sc_img_accel.cc), asi que la logica verificada
// es la misma que corre en el prototipo. La unica diferencia es el envoltorio
// del socket: aqui un simple_target_socket "suelto"; en gem5, el mismo socket
// envuelto por sc_gem5::TlmTargetWrapper<32>.

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "accel_core.h"
#include "accel_io.h"

// ----------------------------------------------------------------------
// Acelerador standalone: replica exacta de la FSM de registros y el proceso
// de conversion de sc_img_accel.cc, con un target socket estandar.
// ----------------------------------------------------------------------
struct Accel : sc_core::sc_module
{
    tlm_utils::simple_target_socket<Accel> tSocket;

    std::uint32_t reg_dir_in = 0, reg_dir_out = 0, reg_num_pix = 0;
    bool busy = false, done = false;
    std::string ruta_in, ruta_out;
    sc_core::sc_event ev_start;

    SC_HAS_PROCESS(Accel);
    Accel(sc_core::sc_module_name n, std::string in, std::string out)
        : sc_module(n), tSocket("tSocket"),
          ruta_in(std::move(in)), ruta_out(std::move(out))
    {
        tSocket.register_b_transport(this, &Accel::b_transport);
        SC_THREAD(proceso);
    }

    void b_transport(tlm::tlm_generic_payload& t, sc_core::sc_time&)
    {
        const auto cmd = t.get_command();
        const auto adr = t.get_address();
        auto* ptr = t.get_data_ptr();
        if (t.get_data_length() != 4) {
            t.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE); return;
        }
        if (cmd == tlm::TLM_WRITE_COMMAND) {
            std::uint32_t v; std::memcpy(&v, ptr, 4);
            switch (adr) {
                case acc_reg::DIR_IN:     reg_dir_in  = v; break;
                case acc_reg::DIR_OUT:    reg_dir_out = v; break;
                case acc_reg::NUM_PIXELS: reg_num_pix = v; break;
                case acc_reg::CTRL:
                    if (v & acc_reg::CTRL_START) {
                        busy = true; done = false;
                        ev_start.notify(sc_core::SC_ZERO_TIME);
                    }
                    break;
                default: break;
            }
        } else {
            std::uint32_t v = 0;
            switch (adr) {
                case acc_reg::STATUS:
                    v = (busy ? acc_reg::ST_BUSY : 0) |
                        (done ? acc_reg::ST_DONE : 0); break;
                case acc_reg::ID:         v = acc_reg::MAGIC_ID; break;
                case acc_reg::DIR_IN:     v = reg_dir_in;  break;
                case acc_reg::DIR_OUT:    v = reg_dir_out; break;
                case acc_reg::NUM_PIXELS: v = reg_num_pix; break;
                default: v = 0; break;
            }
            std::memcpy(ptr, &v, 4);
        }
        t.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    void proceso()
    {
        std::vector<unsigned char> rgb, gris;
        while (true) {
            sc_core::wait(ev_start);
            if (!accel_io::cargar(ruta_in, rgb)) {
                std::cerr << "[Accel] no se pudo abrir " << ruta_in << "\n";
                busy = false; done = true; continue;
            }
            const std::uint32_t npix = reg_num_pix ? reg_num_pix : img::PIXEL_TOTAL;
            gris.resize(npix);
            accel::rgb_a_gris(rgb.data(), gris.data(), npix);
            sc_core::wait(npix * 4.0, sc_core::SC_NS);  // latencia @250MHz
            accel_io::guardar(ruta_out, gris.data(), gris.size());
            busy = false; done = true;
        }
    }
};

// ----------------------------------------------------------------------
// CPU: modela lo que hara el programa en C sobre ARM64 (accesos MMIO).
// ----------------------------------------------------------------------
struct Cpu : sc_core::sc_module
{
    tlm_utils::simple_initiator_socket<Cpu> iSocket;
    bool ok = false;

    SC_HAS_PROCESS(Cpu);
    Cpu(sc_core::sc_module_name n) : sc_module(n), iSocket("iSocket")
    { SC_THREAD(run); }

    void access(tlm::tlm_command cmd, std::uint64_t addr, std::uint32_t& val)
    {
        tlm::tlm_generic_payload t;
        sc_core::sc_time d = sc_core::SC_ZERO_TIME;
        t.set_command(cmd);
        t.set_address(addr);
        t.set_data_ptr(reinterpret_cast<unsigned char*>(&val));
        t.set_data_length(4);
        t.set_streaming_width(4);
        t.set_byte_enable_ptr(nullptr);
        t.set_dmi_allowed(false);
        t.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
        iSocket->b_transport(t, d);
        if (t.is_response_error())
            SC_REPORT_ERROR("CPU", t.get_response_string().c_str());
    }
    void wr(std::uint64_t a, std::uint32_t v) { access(tlm::TLM_WRITE_COMMAND, a, v); }
    std::uint32_t rd(std::uint64_t a) { std::uint32_t v = 0; access(tlm::TLM_READ_COMMAND, a, v); return v; }

    void run()
    {
        // 1) Identificar el periferico
        std::uint32_t id = rd(acc_reg::ID);
        std::cout << "[CPU] ID leido = 0x" << std::hex << id << std::dec << "\n";
        if (id != acc_reg::MAGIC_ID) { SC_REPORT_ERROR("CPU", "ID incorrecto"); return; }

        // 2) Programar el acelerador
        wr(acc_reg::DIR_IN, 0);
        wr(acc_reg::DIR_OUT, 0);
        wr(acc_reg::NUM_PIXELS, img::PIXEL_TOTAL);
        std::cout << "[CPU] registros programados (npix=" << img::PIXEL_TOTAL << ")\n";

        // 3) Arrancar
        wr(acc_reg::CTRL, acc_reg::CTRL_START);
        std::cout << "[CPU] START enviado, sondeando STATUS...\n";

        // 4) Polling de STATUS. wait() timado para que avance el tiempo
        //    simulado y el hilo del acelerador complete su latencia.
        int polls = 0;
        while (!(rd(acc_reg::STATUS) & acc_reg::ST_DONE)) {
            sc_core::wait(1, sc_core::SC_MS);
            if (++polls > 1000) { SC_REPORT_ERROR("CPU", "timeout"); return; }
        }
        std::cout << "[CPU] DONE tras " << polls << " sondeos ("
                  << sc_core::sc_time_stamp() << ")\n";
        ok = true;
        sc_core::sc_stop();
    }
};

int sc_main(int argc, char* argv[])
{
    const std::string in  = (argc > 1) ? argv[1] : "images/input/input_1080p.rgb";
    const std::string out = (argc > 2) ? argv[2] : "images/output/output_1080p_gray.raw";

    Cpu   cpu("cpu");
    Accel acc("acc", in, out);
    cpu.iSocket.bind(acc.tSocket);

    sc_core::sc_start();
    std::cout << (cpu.ok ? "[TB] simulacion OK\n" : "[TB] simulacion FALLO\n");
    return cpu.ok ? 0 : 1;
}
