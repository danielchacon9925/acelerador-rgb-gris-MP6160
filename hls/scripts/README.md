# Scripts del flujo HLS

Aquí se agrupan los scripts TCL y utilidades para automatizar el flujo de Vitis HLS.

## Punto de entrada

- scripts/run_hls.tcl: flujo de proyecto, C-simulation, synthesis, cosimulation y exportación de IP.
- scripts/run_tb.sh: compila y ejecuta el testbench C local, genera la salida RAW y ejecuta la verificación cruzada.
- scripts/run_cosim.sh: invoca Vitis HLS con el script TCL si el entorno está disponible.

## Comandos de uso

```bash
cd hls
./scripts/run_tb.sh
./scripts/run_cosim.sh
```

## Variables opcionales

El script TCL acepta variables de entorno para ajustar el proyecto:

```bash
HLS_PROJECT_DIR=/ruta/al/proyecto \
HLS_PART=xcu50-fsvh2104-2-e \
HLS_CLOCK_NS=4 \
vitis_hls -f scripts/run_hls.tcl
```
