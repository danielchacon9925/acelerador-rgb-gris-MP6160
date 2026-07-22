# vp_arm.py -- Prototipo virtual: SoC ARM64 (gem5) + acelerador SystemC.
#
# Construye un sistema ARM64 bare-metal (plataforma VExpress_GEM5) que corre
# el programa en C (sw/accel_app.elf) y expone el acelerador RGB->gris como
# periferico memory-mapped. Los accesos MMIO de la CPU al rango del acelerador
# se convierten en transacciones TLM-2.0 mediante Gem5ToTlmBridge32 y llegan al
# modulo SystemC ImgAccel (systemc/img_accel_vp/).
#
# El sistema corre bajo el kernel de SystemC de gem5 (SystemC_Kernel), que es
# lo que permite que modulos SystemC y SimObjects de gem5 coexistan y avancen
# con el mismo planificador de eventos (patron de util/systemc/.../systemc_gem5_tlm).
#
# COMO EJECUTAR (desde la raiz del arbol de gem5, tras compilar con EXTRAS):
#   build/ARM/gem5.opt <ruta>/vp_arm.py --kernel <ruta>/sw/accel_app.elf
#
# Este script debe poder importar 'devices' y 'workloads' del directorio
# configs/example/arm de gem5; run_vp.sh se encarga de ubicarlo/enlazarlo ahi.

import argparse
import os

import m5
from m5.objects import *
from m5.util import addToPath

# Permitir importar los helpers ARM de gem5 (devices.py, workloads.py, common/)
addToPath("../..")            # -> configs/
addToPath(".")                # -> configs/example/arm/ (donde vive este script)

import devices
import workloads
from common import ObjectList

# ---------------------------------------------------------------------------
# Direccion base del periferico acelerador en el mapa de memoria del SoC.
# DEBE coincidir con ACCEL_BASE en sw/accel.h. Se elige un rango libre por
# encima del espacio de perifericos on-chip de VExpress_GEM5 y por debajo de
# la DRAM (0x80000000).
# ---------------------------------------------------------------------------
ACCEL_BASE = 0x2F000000
ACCEL_SIZE = 0x1000  # 4 KiB de ventana de registros


def parse_args():
    p = argparse.ArgumentParser(description="Prototipo virtual ARM64 + acelerador SystemC")
    p.add_argument("--kernel", required=True,
                   help="ELF bare-metal ARM64 (sw/accel_app.elf)")
    p.add_argument("--cpu", default="atomic", choices=["atomic", "minor"],
                   help="tipo de CPU")
    p.add_argument("--mem-size", default="512MiB")
    p.add_argument("--image-in", default="images/input/input_1080p.rgb")
    p.add_argument("--image-out", default="images/output/output_1080p_gray.raw")
    return p.parse_args()


def build_system(args):
    cpu_types = {
        "atomic": (AtomicSimpleCPU, None, None, None),
        "minor":  (MinorCPU, devices.L1I, devices.L1D, devices.L2),
    }
    cpu_cls = cpu_types[args.cpu][0]
    mem_mode = cpu_cls.memory_mode()
    want_caches = mem_mode == "timing"

    platform = ObjectList.platform_list.get("VExpress_GEM5_V2")

    # Sistema ARM base (RealView: incluye GIC, UART PL011 en 0x1c090000, etc.)
    system = devices.SimpleSystem(
        want_caches,
        args.mem_size,
        platform=platform(),
        mem_mode=mem_mode,
    )

    # Memoria principal (DRAM) sobre los rangos que define la plataforma.
    system.mem_ctrls = [
        SimpleMemory(range=r, port=system.membus.mem_side_ports)
        for r in system.mem_ranges
    ]

    # Cablear el sistema (bridges de IO, UART, GIC...).
    system.connect()

    # Cluster de CPU ARM64.
    system.cpu_cluster = [
        devices.ArmCpuCluster(
            system, 1, "1GHz", "1.0V", *cpu_types[args.cpu],
        )
    ]
    system.addCaches(want_caches, last_cache_level=2)

    # Bootloader minimo de gem5 y ejecucion en AArch64.
    system.auto_reset_addr = True
    system.highest_el_is_64 = True
    if hasattr(system.realview.gic, "gicv4"):
        system.realview.gic.gicv4 = False

    # Workload bare-metal: carga el ELF y arranca en su punto de entrada.
    system.workload = workloads.ArmBaremetal(args.kernel, system)

    # Que gem5 termine la simulacion cuando el UART reciba EOT (0x04),
    # que es lo que envia startup.S al volver de main().
    for uart in system.realview.uart:
        uart.end_on_eot = True

    # -----------------------------------------------------------------------
    # Periferico acelerador (SystemC) + puente gem5 -> TLM.
    #   iobus (mem_side) --Gem5ToTlmBridge32--> tSocket del ImgAccel
    # El bridge declara el rango [ACCEL_BASE, ACCEL_BASE+SIZE) para que el
    # iobus enrute ahi los accesos MMIO de la CPU. Debe colgarse del iobus (bus
    # de perifericos) y NO del membus: alli el iobridge de VExpress ya cubre
    # ese rango y gem5 aborta por doble respondedor.
    # -----------------------------------------------------------------------
    system.accel = ImgAccel(
        image_in=args.image_in,
        image_out=args.image_out,
    )
    system.tlm_bridge = Gem5ToTlmBridge32(gem5=system.iobus.mem_side_ports)
    system.tlm_bridge.addr_ranges = [AddrRange(ACCEL_BASE, size=ACCEL_SIZE)]
    system.tlm_bridge.tlm = system.accel.tlm

    return system


def main():
    args = parse_args()
    system = build_system(args)

    # Correr bajo el kernel de SystemC de gem5.
    kernel = SystemC_Kernel(system=system)
    root = Root(full_system=True, systemc_kernel=kernel)

    m5.instantiate(None)

    print("== Prototipo virtual ARM64 + acelerador SystemC: iniciando ==")
    event = m5.simulate()
    print(f"== Simulacion terminada @ tick {m5.curTick()}: {event.getCause()} ==")


main()
