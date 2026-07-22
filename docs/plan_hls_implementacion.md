# Plan de trabajo para completar la implementacion HLS

## Alcance
Este plan se enfoca solo en la implementacion HLS (no en extender el prototipo virtual).

## Verificacion de sintetizabilidad del codigo SystemC actual

### Conclusiones rapidas
- No todo el codigo SystemC actual es sintetizable en HLS.
- El nucleo aritmetico de conversion si es reutilizable para HLS.
- El modulo integrado con gem5/TLM debe separarse del nucleo HLS.

### Archivo por archivo
1. `virtual_prototype/systemc/img_accel_vp/accel_core.h`
   - `accel::rgb_a_gris(...)` es logica C++ apta para HLS como base funcional.
   - El mapa de registros definido aqui sirve como contrato funcional de referencia.

2. `virtual_prototype/systemc/img_accel_vp/accel_io.h`
   - No sintetizable: usa `std::ifstream`, `std::ofstream`, `std::string`, `std::vector`.
   - Esto es solo para modelar almacenamiento persistente en host.

3. `virtual_prototype/systemc/img_accel_vp/sc_img_accel.hh` y `sc_img_accel.cc`
   - No sintetizable: dependencias de gem5/SystemC/TLM (`SC_MODULE`, sockets TLM, `sc_event`, `SC_THREAD`, `wait`).
   - No sintetizable: E/S por archivos (`accel_io::cargar/guardar`), `std::vector`, `std::string`, `iostream`.
   - Se debe usar solo como referencia de comportamiento y protocolo de control.

4. `virtual_prototype/verify/tb_standalone.cpp`
   - No sintetizable: es testbench de simulacion SystemC/TLM.

## Como arreglar la falta de sintetizabilidad

1. Separar claramente 3 capas
   - Capa 1 (golden funcional): conversion BT.601 entera.
   - Capa 2 (top HLS): interfaz AXI y movimiento de datos memoria->kernel->memoria.
   - Capa 3 (simulacion VP): wrapper SystemC/TLM y E/S a archivos.

2. No migrar TLM/SystemC a HLS
   - El top HLS debe ser C/C++ plano con pragmas de Vitis.
   - El control TLM y la E/S host se quedan en VP para verificacion de sistema.

3. Reusar formula exacta
   - Mantener: `gris = (77*R + 150*G + 29*B) >> 8`.
   - Evitar float/double/ap_fixed para conservar bit-exactitud con VP y referencia.

## Estado real del repositorio
- La carpeta `hls/` ya existe y contiene estructura base, kernel funcional, testbench y scripts de ejecución.
- La fase 0, 1 y 2 quedan verificadas localmente y la salida HLS coincide bit a bit con la referencia del proyecto.
- La fase 3 queda parcialmente implementada: el flujo TCL base existe, pero su ejecución en Vitis HLS requiere un entorno instalado y posiblemente ajustes específicos del dispositivo/partición.

## Plan de implementacion HLS (fases)

### Fase 0: Preparacion de estructura
1. Crear `hls/src`, `hls/tb`, `hls/scripts`, `hls/reports`.
2. Agregar `hls/README.md` con pasos de ejecucion y criterios de aceptacion.
3. Definir convencion de salidas para que `scripts/cross_check.sh` pueda consumir el RAW de HLS.

### Fase 1: Kernel HLS minimo funcional
1. Implementar top HLS con interfaz:
   - AXI4 master para entrada RGB.
   - AXI4 master para salida gris.
   - AXI4-Lite para control (bases y numero de pixeles).
2. Integrar pragmas minimos:
   - interfaces m_axi/s_axilite.
   - pipeline en bucle de pixeles (objetivo II=1).
3. Asegurar que la formula sea identica a la referencia entera.

### Fase 2: Testbench C y co-simulacion
1. Testbench que:
   - Carga `images/input/input_1080p.rgb`.
   - Ejecuta el top HLS.
   - Guarda RAW de salida en ruta conocida.
2. Validacion local:
   - Comparar byte a byte con `docs/reference_output_gray.raw`.
3. C/RTL cosim:
   - Confirmar equivalencia C-sim vs RTL cosim.

### Fase 3: Flujo TCL automatizado
1. Crear `hls/scripts/run_hls.tcl` con:
   - apertura/creacion de proyecto,
   - add_files (src/tb),
   - set_top,
   - C simulation,
   - C synthesis,
   - C/RTL cosimulation,
   - export IP.
2. Fijar target 250 MHz (clock period 4 ns).
3. Dejar logs/reportes en `hls/reports` y/o carpeta de proyecto HLS.

Estado actual:
- El script TCL base ya existe y está orientado a un proyecto de Vitis HLS.
- Se ha dejado un flujo inicial con `csim_design`, `synth_design`, `cosim_design` y `export_design`.
- Queda pendiente validar el script en un entorno con Vitis HLS y ajustar el `set_part`/ruta de proyecto si el entorno local difiere.

### Fase 4: Cierre tecnico y trazabilidad
1. Ejecutar `scripts/cross_check.sh <salida_hls.raw>`.
2. Completar en README/reporte:
   - diagrama de bloques HLS,
   - diagrama de secuencia HLS,
   - mapa AXI4-Lite real exportado por Vitis,
   - recursos (LUT/FF/BRAM/DSP), latencia e II.
3. Documentar correspondencia de control VP vs HLS (`CTRL/STATUS` <-> `ap_start/ap_done/ap_idle`).

## Checklist de aceptacion
- [x] Existe carpeta `hls/` funcional con codigo, testbench y TCL.
- [x] C-sim local pasa y genera la salida HLS esperada.
- [x] Salida HLS coincide bit a bit con referencia.
- [x] Script cruzado reporta equivalencia con VP y Eval 1.
- [ ] Reporte y README actualizados con evidencias (interfaces, mapa, latencia, recursos) de la síntesis real en Vitis HLS.

## Riesgos y mitigaciones
1. Riesgo: diferencias de redondeo.
   - Mitigacion: usar solo aritmetica entera exacta.
2. Riesgo: no cumplir 250 MHz/II=1.
   - Mitigacion: pragmas de pipeline y revision temprana de reportes de scheduling.
3. Riesgo: inconsistencia entre documentacion y repo.
   - Mitigacion: crear `hls/` real y actualizar README conforme a estado verdadero.
