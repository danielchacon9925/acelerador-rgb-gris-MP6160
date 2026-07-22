# Instrucciones para ejecutar la implementación HLS en Vitis

Este documento resume cómo ejecutar el flujo HLS del proyecto desde un servidor que tenga Vitis HLS instalado.

## 1. Cargar el entorno de Vitis

Si tu servidor usa módulos, prueba:

```bash
module load Xilinx/Vitis/2024.1
```

Si no usa módulos, localiza Vitis y carga su ambiente:

```bash
which vitis_hls
source /ruta/a/Vitis/2024.1/settings64.sh
```

## 2. Entrar al repositorio

```bash
cd /home/andres/maestria/tarea2_repo
```

## 3. Validar primero el testbench C local

Antes de correr Vitis, puedes verificar que el kernel genere la salida correcta:

```bash
./hls/scripts/run_tb.sh
```

Esto compila el testbench, ejecuta el kernel y genera:

```text
hls/reports/output_hls.raw
```

## 4. Ejecutar el flujo completo de Vitis HLS

Desde la carpeta HLS:

```bash
cd hls
./scripts/run_cosim.sh
```

Ese script invoca:

```bash
vitis_hls -f scripts/run_hls.tcl
```

## 5. Ejecutar Vitis HLS directamente

Si prefieres ejecutarlo de forma manual:

```bash
cd /home/andres/maestria/tarea2_repo/hls
vitis_hls -f scripts/run_hls.tcl
```

## 6. Ajustar partición y reloj si es necesario

El script TCL acepta variables de entorno opcionales:

```bash
HLS_PROJECT_DIR=/ruta/al/proyecto \
HLS_PART=xcu50-fsvh2104-2-e \
HLS_CLOCK_NS=4 \
vitis_hls -f scripts/run_hls.tcl
```

## 7. Archivos relevantes

- [hls/scripts/run_hls.tcl](hls/scripts/run_hls.tcl)
- [hls/scripts/run_tb.sh](hls/scripts/run_tb.sh)
- [hls/scripts/run_cosim.sh](hls/scripts/run_cosim.sh)
- [hls/src/img_accel_hls.cpp](hls/src/img_accel_hls.cpp)
- [hls/tb/img_accel_tb.cpp](hls/tb/img_accel_tb.cpp)

## 8. Resultados esperados

Al finalizar, revisa:

- [hls/reports](hls/reports)
- [hls/build](hls/build)
- [hls/reports/output_hls.raw](hls/reports/output_hls.raw)
