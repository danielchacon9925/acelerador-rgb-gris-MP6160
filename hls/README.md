# Implementación HLS — fase 0

Este directorio concentra la implementación HLS del acelerador RGB → gris.

## Estado actual

La fase 0 deja preparada la estructura base para el flujo de HLS:

- carpetas de código fuente, testbench, scripts y reportes,
- una convención de salida para los archivos RAW generados por el testbench,
- una guía inicial para ejecutar el flujo de Vitis HLS.

## Estructura esperada

```text
hls/
├── README.md
├── src/                # fuentes sintetizables en HLS
├── tb/                 # testbench C/C++ para co-simulación
├── scripts/            # scripts TCL y utilidades de automatización
└── reports/            # reportes, logs y salidas RAW
```

## Convención de salida

El testbench de HLS debe generar el archivo RAW final en:

```text
hls/reports/output_hls.raw
```

Esta ruta está pensada para ser consumida por el script de verificación cruzada:

```bash
./scripts/cross_check.sh hls/reports/output_hls.raw
```

## Ejecución actual

### Testbench C local

```bash
cd hls
./scripts/run_tb.sh
```

Esto compila el testbench, ejecuta el kernel y genera la salida RAW en:

```text
hls/reports/output_hls.raw
```

### Co-simulación con Vitis HLS

Si Vitis HLS está instalado:

```bash
cd hls
./scripts/run_cosim.sh
```

El script invoca el flujo de Vitis HLS descrito en scripts/run_hls.tcl.

## Criterios de aceptación de la fase 0

- Existe la carpeta hls/ con las subcarpetas src, tb, scripts y reports.
- El README documenta la convención de salida para el archivo RAW.
- El flujo de HLS tiene un punto de entrada inicial en scripts/run_hls.tcl.
