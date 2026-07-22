/*
 * sc_img_accel.hh -- Acelerador RGB->gris (SystemC/TLM-2.0) adaptado como
 * periferico memory-mapped de un SoC ARM64 en gem5.
 *
 * Es el MISMO modelo de la Evaluacion 1 (misma conversion BT.601, mismos
 * registros de configuracion DIR_IN/DIR_OUT/NUM_PIXELS/CTRL/STATUS), con
 * dos adaptaciones para integrarlo al puente TLM de gem5:
 *
 *   1. En vez de un tlm_utils::simple_target_socket "suelto", el socket se
 *      envuelve con sc_gem5::TlmTargetWrapper<32>, que es lo que permite a
 *      gem5 enrutar accesos MMIO de la CPU hacia este b_transport (patron
 *      identico al ejemplo oficial util/systemc/.../sc_tlm_target.hh).
 *   2. El acceso a la RAM ("DMA") de la Evaluacion 1 se sustituye por E/S
 *      directa a archivos (accel_io): el acelerador es autonomo respecto a
 *      los datos y la CPU ARM64 solo lo controla por registros. Asi el
 *      unico sentido de trafico TLM es CPU(gem5) -> acelerador(SystemC),
 *      sin necesidad de un master transactor de vuelta.
 *
 * La logica de decodificacion de registros y la conversion viven en
 * accel_core.h (compartidas con el verificador standalone).
 */

#ifndef __SC_IMG_ACCEL_HH__
#define __SC_IMG_ACCEL_HH__

#include <tlm_utils/simple_target_socket.h>

#include <string>
#include <vector>

#include "base/trace.hh"
#include "systemc/ext/core/sc_module_name.hh"
#include "systemc/tlm_port_wrapper.hh"

#include "systemc/ext/systemc"
#include "systemc/ext/tlm"

#include "accel_core.h"
#include "accel_io.h"

// SC_MODULE clasico; toda la "magia" de convertirlo en SimObject de gem5
// la maneja la clase base generada a partir de ImgAccel.py.
SC_MODULE(ImgAccel)
{
  public:
    // Socket target: la CPU escribe/lee los registros de configuracion.
    tlm_utils::simple_target_socket<ImgAccel> tSocket;
    // Wrapper que conecta el socket TLM con el mundo de puertos de gem5.
    sc_gem5::TlmTargetWrapper<32> wrapper;

  private:
    // Registros de configuracion programados por la CPU.
    std::uint32_t reg_dir_in    = 0;
    std::uint32_t reg_dir_out   = 0;
    std::uint32_t reg_num_pix   = 0;
    bool          busy          = false;
    bool          done          = false;

    // Rutas de los archivos que modelan el almacenamiento persistente.
    std::string ruta_in;
    std::string ruta_out;

    // Evento que despierta al proceso de conversion cuando llega START.
    sc_core::sc_event ev_start;

  public:
    SC_HAS_PROCESS(ImgAccel);

    // El constructor recibe las rutas via parametros del SimObject
    // (ver ImgAccel.py / create()).
    ImgAccel(sc_core::sc_module_name name,
             const std::string& image_in,
             const std::string& image_out);

    // Requerido por gem5 para exponer el puerto TLM al resto del sistema.
    gem5::Port &gem5_getPort(const std::string &if_name, int idx=-1) override;

    // Callback de acceso a registros (target).
    virtual void b_transport(tlm::tlm_generic_payload& trans,
                             sc_core::sc_time& delay);

    // Proceso paralelo que realiza la conversion cuando se dispara START.
    void proceso();
};

#endif // __SC_IMG_ACCEL_HH__
