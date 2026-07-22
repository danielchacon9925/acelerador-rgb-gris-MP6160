# Implementación en HLS — Acelerador RGB → escala de grises

Implementación en **Vitis HLS** del acelerador de la Evaluación 2, sintetizable
para la **AMD Kria KV260** (SOM K26, `xck26-sfvc784-2LV-c`) a **250 MHz**. Es la
contraparte en hardware del [prototipo virtual](../virtual_prototype/): ambos
derivan del mismo modelo de referencia (Eval 1) y producen la **misma imagen de
salida bit a bit**.

> **Versión de la herramienta.** El enunciado indica Vitis 2024.1; este flujo se
> desarrolló y verificó con **Vitis HLS 2024.2** (la versión instalada). El flujo
> TCL y el código son idénticos entre ambas versiones.

---

## 1. Organización

```
hls/
├── README.md                 Este documento
├── src/
│   ├── rgb2gray.hpp          Contrato: constantes, coeficientes BT.601, firma
│   └── rgb2gray.cpp          Kernel sintetizable (DATAFLOW: load→compute→store)
├── tb/
│   ├── tb_rgb2gray.cpp       Testbench en C para co-simulación (compara vs. referencia)
│   └── vectors/             Vectores de co-simulación (recorte de 8192 px)
│       ├── input_crop.rgb   Primeros 8192 px de la entrada RGB
│       └── ref_crop.raw     Primeros 8192 px de la referencia en grises
├── scripts/
│   └── run_hls.tcl           Flujo completo: csim → csynth → cosim → export
└── reports/                  Reportes reales de síntesis/co-simulación (generados)
    └── rgb2gray_csynth.rpt
```

## 2. Cómo compilar y ejecutar el flujo

```bash
# 1) Cargar el entorno de Vitis HLS
source /ruta/a/Vitis_HLS/2024.2/settings64.sh

# 2) Ejecutar el flujo completo desde la carpeta hls/
cd hls
vitis_hls -f scripts/run_hls.tcl
```

El script ejecuta, en orden:

| Paso | Comando | Qué produce |
|---|---|---|
| Simulación C | `csim_design` | Verifica la función en C; compara contra la referencia |
| Síntesis | `csynth_design` | RTL + reporte de timing, recursos y latencia |
| Co-simulación | `cosim_design` | Verifica el RTL generado con el mismo testbench |
| Exportar IP | `export_design` | Empaqueta el IP para Vivado / integración en la KV260 |

El testbench escribe la imagen de salida en
[`../images/output/output_1080p_gray_hls.raw`](../images/output/), lista para la
[verificación cruzada](../scripts/cross_check.sh).

## 3. Arquitectura del acelerador

Pipeline con **separación explícita entre E/S de datos y procesamiento**, tal como
exige el enunciado, implementado con `#pragma HLS DATAFLOW` y tres etapas
concurrentes encadenadas por `hls::stream`:

```
 in  ──(AXI4 Master)──▶ [ load_rgb ] ──s_rgb──▶ [ compute_gray ] ──s_gray──▶ [ store_gray ] ──(AXI4 Master)──▶ out
                        ── E/S ──                ── PROCESAMIENTO ──                ── E/S ──
```

- **`load_rgb`** — lee la imagen RGB desde la RAM del sistema (3 B/px) y la empuja al stream.
- **`compute_gray`** — conversión BT.601 **entera**: `Y = (77·R + 150·G + 29·B) >> 8`.
- **`store_gray`** — extrae el gris del stream y lo escribe en la RAM (1 B/px).

### Interfaces

| Puerto | Interfaz | Rol |
|---|---|---|
| `in`  | **AXI4 Master** (`m_axi`, offset=slave) | Lectura de la imagen RGB en RAM |
| `out` | **AXI4 Master** (`m_axi`, offset=slave) | Escritura de la imagen gris en RAM |
| `in`, `out`, `num_pixels` | **AXI4-Lite** (`s_axilite`) | Direcciones base y nº de píxeles |
| control de bloque | **AXI4-Lite** (`ap_ctrl_hs`) | `ap_start` / `ap_done` / `ap_idle` |

Esto corresponde con el mapa de registros del prototipo virtual: `DIR_IN`/`DIR_OUT`
→ punteros AXI4 Master (offset por AXI4-Lite), `NUM_PIXELS` → escalar AXI4-Lite,
`CTRL`/`STATUS` → `ap_start`/`ap_done` (ver §10.2 y §10.4 del README raíz).

## 4. Equivalencia bit a bit

La conversión usa **exactamente los mismos coeficientes enteros** que
[`accel_core.h`](../virtual_prototype/systemc/img_accel_vp/accel_core.h) del
prototipo virtual. No se emplea `float` ni `ap_fixed`: cualquiera de ellos
introduciría diferencias de redondeo (±1 en ~47 % de los píxeles). El testbench
compara la salida contra `docs/reference_output_gray.raw` y la co-simulación
vuelve a verificar el RTL con el mismo criterio.

Verificación cruzada de las tres implementaciones:

```bash
./scripts/cross_check.sh   # desde la raíz del repo, tras correr el flujo HLS
```

## 5. Resultados de síntesis (Vitis HLS 2024.2, KV260, 250 MHz)

Reporte completo en [`reports/rgb2gray_csynth.rpt`](reports/rgb2gray_csynth.rpt).

| Métrica | Valor | Nota |
|---|---|---|
| Reloj objetivo | 4.00 ns (250 MHz) | Requisito del enunciado |
| Reloj estimado | 2.920 ns | **Fmax ≈ 342 MHz** — cumple con holgura |
| BRAM_18K | 5 / 288 | 1 % |
| DSP | 2 / 1248 | ~0 % (los 3 multiplicadores BT.601) |
| FF | 2 645 / 234 240 | 1 % |
| LUT | 3 434 / 117 120 | 2 % |

**Initiation Interval por etapa:** `compute_gray` = 1, `store_gray` = 1, `load_rgb` = 3.
La etapa de carga es el cuello de botella porque lee los 3 bytes RGB con accesos
byte a byte sobre un puerto AXI de 8 bits; bajo `DATAFLOW`, el pipeline procesa un
píxel cada ~3 ciclos. Detalle e implicación en la latencia en la
[§10.3 del README raíz](../README.md#103-calibración-del-modelo-de-latencia).

**Verificación:** csim con salida bit-exacta sobre la imagen 1080p completa (0
errores) y co-simulación C/RTL **PASS** sobre un recorte de 8192 píxeles (simular la
imagen completa en `xsim` es impracticable). La cosim mide **24 671 ciclos** para
8192 px = 3,01 ciclos/px, lo que confirma el `II=3` de la etapa de carga y proyecta
≈ 24,98 ms para los 1080p completos (ver
[§10.3 del README raíz](../README.md#103-calibración-del-modelo-de-latencia)). Reporte
en [`reports/rgb2gray_cosim.rpt`](reports/rgb2gray_cosim.rpt).

## 6. Nota de optimización

El cuello de botella es `load_rgb` (`II=3`): accede a la memoria en granularidad de
byte (bus AXI de 8 bits), lo que es simple y **bit-exacto** pero no maximiza el ancho
de banda. Una mejora directa —sin alterar el resultado— es ensanchar el puerto
`m_axi` de entrada (p. ej. leer 64 bits por transacción y desempaquetar los canales
RGB) para llevar la carga a `II≈1` y triplicar la tasa de píxeles por ciclo. Se
documenta como trabajo futuro; el diseño actual cumple los requisitos funcionales, de
frecuencia (250 MHz) y de equivalencia bit a bit.

## 7. Declaración de uso de IA

Ver [§11.2 del README raíz](../README.md#11-declaración-sobre-el-uso-de-inteligencia-artificial).
