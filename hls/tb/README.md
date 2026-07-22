# Testbench HLS

Aquí irán los testbench en C/C++ usados para la co-simulación.

## Convención de salida

El testbench debe escribir la salida generada por el kernel en:

```text
hls/reports/output_hls.raw
```

Este archivo debe contener exactamente un byte por píxel en escala de grises.
