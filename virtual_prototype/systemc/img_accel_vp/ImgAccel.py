# ImgAccel.py -- SimObject que envuelve el modulo SystemC del acelerador.
#
# Al compilar gem5 con EXTRAS apuntando al directorio 'systemc/' de este
# repo, gem5 genera params/ImgAccel.hh a partir de esta descripcion y lo
# usa el constructor del modulo (ver sc_img_accel.cc :: create()).

from m5.objects.SystemC import SystemC_ScModule
from m5.objects.Tlm import TlmTargetSocket
from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject


class ImgAccel(SystemC_ScModule):
    """Acelerador RGB->gris de la Evaluacion 1, adaptado como periferico
    TLM-2.0 memory-mapped del SoC ARM64 simulado en gem5."""

    type = "ImgAccel"
    cxx_class = "ImgAccel"
    cxx_header = "img_accel_vp/sc_img_accel.hh"

    # Socket TLM target de 32 bits: por aqui entran los accesos MMIO de la
    # CPU a los registros de configuracion, via Gem5ToTlmBridge32.
    tlm = TlmTargetSocket(32, "socket TLM de registros de configuracion")

    # Rutas del almacenamiento persistente (carpeta del host).
    image_in = Param.String(
        "images/input/input_1080p.rgb", "imagen RAW RGB de entrada"
    )
    image_out = Param.String(
        "images/output/output_1080p_gray.raw", "imagen en grises de salida"
    )

    system = Param.System(Parent.any, "system")
