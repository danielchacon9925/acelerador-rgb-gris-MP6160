# Acelerador de Conversión RGB a Escala de Grises

**Evaluación Corta 2 — Diseño de Alto Nivel (MP6160)**
II Cuatrimestre 2026 · Tecnológico de Costa Rica
Profesor: Luis G. León-Vega, Ph.D.

Implementación de un acelerador de procesamiento de imagen que convierte una imagen RAW RGB de
1080p a escala de grises, desarrollado en dos vertientes complementarias a partir del modelo a
nivel de transacciones de la evaluación anterior:

1. **Implementación en HLS** — Vitis 2024.1, 250 MHz, AMD Kria KV260, interfaces AXI4 / AXI4-Lite.
2. **Prototipo virtual** — SoC ARM64 simulado en gem5, con el acelerador SystemC integrado
   mediante TLM-2.0 y controlado por un programa en C.

| Parte | Estado |
|---|---|
| Prototipo virtual | ✅ Completo y verificado |
| Implementación HLS | ⬜ En desarrollo — ver [`hls/README.md`](hls/README.md) |
| Verificación cruzada | ◐ Parcial — pendiente la salida del testbench de HLS |

---

## Tabla de contenido

1. [Descripción del sistema](#1-descripción-del-sistema)
2. [Instrucciones de compilación y ejecución](#2-instrucciones-de-compilación-y-ejecución)
3. [Organización del repositorio](#3-organización-del-repositorio)
4. [Organización de los módulos](#4-organización-de-los-módulos)
5. [Diagrama de bloques](#5-diagrama-de-bloques)
6. [Diagrama de secuencias](#6-diagrama-de-secuencias)
7. [Formato de las transacciones](#7-formato-de-las-transacciones)
8. [Mapa de memoria y registros](#8-mapa-de-memoria-y-registros)
9. [Resultados obtenidos](#9-resultados-obtenidos)
10. [Correspondencia entre ambas implementaciones](#10-correspondencia-entre-ambas-implementaciones)
11. [Declaración sobre el uso de Inteligencia Artificial](#11-declaración-sobre-el-uso-de-inteligencia-artificial)

---

## 1. Descripción del sistema

### Flujo funcional

1. Cargar una imagen desde un almacenamiento persistente.
2. La imagen está en formato RAW RGB, resolución 1080p (1920 × 1080).
3. Indicar al acelerador la dirección base de entrada, la de salida y la cantidad de píxeles.
4. El acelerador convierte la imagen a escala de grises.
5. El resultado se almacena nuevamente en disco.

### Conversión implementada

Luminancia BT.601 evaluada en **aritmética entera**, idéntica en ambas implementaciones:

```
Y = (77·R + 150·G + 29·B) >> 8
```

Los coeficientes 77, 150 y 29 se obtienen al redondear 0.299×256, 0.587×256 y 0.114×256.
El uso de aritmética entera —y no de punto flotante— es requisito para que ambas
implementaciones sean equivalentes bit a bit (ver [sección 10](#10-correspondencia-entre-ambas-implementaciones)).

### Parámetros de los datos

| Parámetro | Valor |
|---|---|
| Resolución | 1920 × 1080 |
| Total de píxeles | 2 073 600 |
| Imagen de entrada (RGB, 3 B/px) | 6 220 800 bytes |
| Imagen de salida (gris, 1 B/px) | 2 073 600 bytes |

---

## 2. Instrucciones de compilación y ejecución

### 2.1 Prototipo virtual

#### Requisitos

Probado en Debian 13 (Trixie); en Ubuntu funciona igual.

```bash
# SystemC
sudo apt install -y libsystemc-dev g++
#   (si se usa una instalación propia:  export SYSTEMC_HOME=/usr/local/systemc)

# Cross-compiler ARM64
sudo apt install -y gcc-aarch64-linux-gnu

# Dependencias de gem5
sudo apt install -y build-essential scons python3-dev git m4 \
    zlib1g-dev libprotobuf-dev protobuf-compiler libboost-all-dev pkg-config
```

#### Procedimiento

```bash
cd virtual_prototype

# 1) Verificar el acelerador (SystemC puro, sin gem5) — segundos
./scripts/verify.sh

# 2) Compilar el programa en C  ->  sw/accel_app.elf
./scripts/build_sw.sh

# 3) Compilar gem5 con el acelerador integrado — 20 a 40 min
yes | ./scripts/build_gem5.sh

# 4) Ejecutar el prototipo virtual completo
./scripts/run_vp.sh
```

El paso 3 clona gem5 (~2 GB) en la raíz del repositorio y lo compila mediante el mecanismo
`EXTRAS`, que incorpora el acelerador dentro del binario del simulador sin modificar su árbol
de fuentes:

```bash
scons build/ARM/gem5.opt EXTRAS=<repo>/virtual_prototype/systemc/img_accel_vp -j$(nproc)
```

Si ya se dispone de un clon de gem5, se indica con `GEM5_DIR=/ruta/a/gem5`. Si la compilación
agota la memoria, se reduce el paralelismo con `JOBS=4`; `scons` retoma donde quedó.

#### Verificación del resultado

```bash
md5sum images/output/output_1080p_gray.raw docs/reference_output_gray.raw
```

Ambas sumas deben coincidir en `45041cfcf4f0f1f0d8cdc2230acee01b`.

### 2.2 Implementación en HLS

La implementación HLS ya tiene un flujo base operativo.

#### Ejecución local del testbench

```bash
cd hls
./scripts/run_tb.sh
```

Este comando compila el testbench, ejecuta el kernel y genera la salida RAW en:

```text
hls/reports/output_hls.raw
```

#### Ejecución del flujo de Vitis HLS

Si Vitis HLS está instalado en el sistema:

```bash
cd hls
./scripts/run_cosim.sh
```

El script invoca:

```bash
vitis_hls -f scripts/run_hls.tcl
```

Para una referencia detallada, ver [hls/README.md](hls/README.md).

### 2.3 Verificación cruzada

Una vez disponible la salida del testbench de HLS:

```bash
./scripts/cross_check.sh <ruta/a/la/salida/del/testbench/HLS.raw>
```

---

## 3. Organización del repositorio

```
acelerador-rgb-gris-MP6160/
├── README.md                          Este documento
│
├── hls/                               IMPLEMENTACIÓN EN HLS  (pendiente)
│   ├── README.md                      Guía de lo que debe entregarse
│   ├── src/                           Código fuente sintetizable
│   ├── tb/                            Testbench en C para co-simulación
│   ├── scripts/                       Scripts TCL del flujo de HLS
│   │   └── run_hls.tcl                Plantilla del flujo completo
│   └── reports/                       Reportes de síntesis y co-simulación
│
├── virtual_prototype/                 PROTOTIPO VIRTUAL  (completo)
│   ├── systemc/img_accel_vp/          Modelo SystemC del acelerador
│   │   ├── accel_core.h               Mapa de registros y conversión BT.601
│   │   ├── accel_io.h                 E/S del almacenamiento persistente
│   │   ├── sc_img_accel.hh/.cc        Módulo SystemC integrado a gem5
│   │   ├── ImgAccel.py                Descripción como SimObject de gem5
│   │   └── SConscript                 Regla de compilación (EXTRAS)
│   ├── sw/                            Programa en C bare-metal (ARM64)
│   │   ├── main.c                     Control del periférico por MMIO
│   │   ├── accel.h                    Mapa de registros (lado software)
│   │   ├── uart.h                     Driver del UART PL011
│   │   ├── startup.S                  Arranque en ensamblador AArch64
│   │   ├── link.ld                    Linker script (carga en 0x80000000)
│   │   └── Makefile                   Compilación cruzada
│   ├── gem5-config/vp_arm.py          Construcción del SoC ARM64 + acelerador
│   ├── verify/tb_standalone.cpp       Banco de pruebas en SystemC puro
│   └── scripts/                       Automatización
│       ├── verify.sh                  Verifica el acelerador sin gem5
│       ├── build_sw.sh                Compila el programa en C
│       ├── build_gem5.sh              Compila gem5 con el acelerador
│       └── run_vp.sh                  Ejecuta el prototipo completo
│
├── images/                            COMPARTIDO por ambas implementaciones
│   ├── input/input_1080p.rgb          Imagen de entrada RAW RGB
│   └── output/output_1080p_gray.raw   Imagen de salida generada
│
├── docs/
│   ├── diagrama_bloques.txt           Diagrama de bloques del prototipo virtual
│   ├── reference_output_gray.raw      Salida de referencia (Evaluación 1)
│   └── reporte/                       Reporte técnico en LaTeX
│       ├── reporte_MP6160.tex
│       ├── COMO_COMPILAR.txt
│       └── figuras/
│
└── scripts/
    └── cross_check.sh                 Verificación cruzada HLS ↔ VP ↔ Eval 1
```

La carpeta `images/` es deliberadamente compartida: **ambas implementaciones deben procesar
la misma imagen de entrada** para que la verificación cruzada tenga sentido.

---

## 4. Organización de los módulos

### 4.1 Acelerador SystemC — `virtual_prototype/systemc/img_accel_vp/`

Organizado en tres capas funcionales:

| Archivo | Capa | Función |
|---|---|---|
| `accel_core.h` | Contrato | Constantes puras: mapa de registros, dimensiones y la función de conversión. Sin dependencias de SystemC ni de gem5. |
| `accel_io.h` | Contrato | Lectura y escritura de archivos: modela el almacenamiento persistente. |
| `sc_img_accel.hh` | Modelo | Declaración del `SC_MODULE`: socket TLM, registros internos, evento de arranque. |
| `sc_img_accel.cc` | Modelo | `b_transport()` decodifica registros; el `SC_THREAD proceso()` realiza la conversión. |
| `ImgAccel.py` | Envoltorio | Descripción del acelerador como *SimObject* de gem5 (socket TLM, parámetros de imagen). |
| `SConscript` | Envoltorio | Regla de compilación para el mecanismo `EXTRAS`. |

La separación entre *contrato* y *modelo* permite que el banco de pruebas independiente incluya
el mismo `accel_core.h`: lo verificado fuera de gem5 es literalmente el mismo código que se
ejecuta dentro del prototipo.

**Comportamiento del módulo.** `b_transport()` atiende cada transacción y retorna de inmediato;
al recibir el bit `START` únicamente notifica un evento. El procesamiento se realiza en un
`SC_THREAD` independiente, de modo que la CPU puede continuar ejecutando y sondear el estado
—reproduciendo el comportamiento de un periférico real que opera en paralelo con el procesador.

### 4.2 Programa en C — `virtual_prototype/sw/`

Software *bare-metal*: sin sistema operativo, sin biblioteca estándar y con la MMU desactivada.

| Archivo | Función |
|---|---|
| `accel.h` | Dirección base, offsets de registro (espejo de `accel_core.h`) y accesores MMIO. |
| `uart.h` | Driver del UART PL011 (0x1c090000): permite imprimir sin sistema operativo. |
| `main.c` | Identifica el periférico, lo configura, lo arranca y sondea hasta la terminación. |
| `startup.S` | Fija el puntero de pila, limpia `.bss`, invoca `main()` y cierra la simulación. |
| `link.ld` | Ubica el binario en 0x80000000 (base de DRAM del SoC). |
| `Makefile` | Compilación cruzada a AArch64 con opciones *freestanding* y estáticas. |

El acceso al periférico se realiza mediante punteros calificados con `volatile`, requisito
indispensable: sin él, el compilador extraería la lectura del registro de estado fuera del
bucle de espera, convirtiéndolo en un bucle infinito.

### 4.3 Sistema — `virtual_prototype/gem5-config/vp_arm.py`

Construye un SoC ARM64 *bare-metal* sobre la plataforma `VExpress_GEM5_V2`, le conecta la DRAM
y el UART, y cuelga el acelerador del bus de periféricos (`iobus`) a través de un
`Gem5ToTlmBridge32` en el rango `0x2F000000`. Todo se ejecuta bajo `SystemC_Kernel`, que
permite que los módulos SystemC y los componentes de gem5 avancen con el mismo planificador de
eventos.

### 4.4 Implementación en HLS — `hls/`

> ⬜ Pendiente. Ver [`hls/README.md`](hls/README.md).

---

## 5. Diagrama de bloques

### 5.1 Prototipo virtual

```
   +===========================================================================+
   |                        gem5  (SystemC_Kernel)                             |
   |   +---------------------+                                                 |
   |   |   CPU ARM64 (ARMv8) |   ejecuta sw/accel_app.elf (bare-metal, C)      |
   |   +----------+----------+                                                 |
   |              | acceso MMIO                                                |
   |        +-----v---------------------------------------------------+        |
   |        |                 membus  (SystemXBar)                    |        |
   |        +--+-------------------+-------------------------+--------+        |
   |           v                   v                         v                 |
   |   +-------+------+    +-------+--------+     +----------+-----------+      |
   |   | DRAM         |    | UART PL011     |     | iobridge --> iobus   |      |
   |   | 0x80000000   |    | 0x1c090000     |     |          |           |      |
   |   +--------------+    +----------------+     +----------v-----------+      |
   |                                              | Gem5ToTlmBridge32    |      |
   |                                              | rango: 0x2F000000    |      |
   |                                              +----------+-----------+      |
   |                                                         | TLM-2.0          |
   |. . . . . . . . . . frontera gem5 <-> SystemC . . . . . .v . . . . . . . . .|
   |                                              +----------+-----------+      |
   |                                              |   ImgAccel (SystemC) |      |
   |                                              |  tSocket + wrapper   |      |
   |                                              |  b_transport()       |      |
   |                                              |  SC_THREAD proceso() |      |
   |                                              +----+-------------+---+      |
   +===================================================|=============|==========+
                                              lee      v             v  escribe
                                          +------------+--+   +------+---------+
                                          | input .rgb    |   | output .raw    |
                                          +---------------+   +----------------+
                                            Almacenamiento persistente (host)
```

Versión ampliada en [`docs/diagrama_bloques.txt`](docs/diagrama_bloques.txt).

El acelerador constituye un ***target* puro**: el tráfico TLM circula siempre en sentido
CPU → acelerador. El movimiento de los píxeles lo efectúa el propio acelerador contra archivos
del anfitrión, por lo que no se requiere un *transactor* maestro de retorno hacia gem5.

### 5.2 Implementación en HLS

> ⬜ Pendiente: insertar el diagrama de bloques de la arquitectura en hardware.

---

## 6. Diagrama de secuencias

```
 CPU ARM64 (main.c)      membus        Gem5ToTlmBridge32       ImgAccel (SystemC)
     |                     |                  |                        |
     |  MMIO RD 0x2F...14  |                  |                        |
     |-------------------->|--- TLM READ ---->|----- b_transport ----->|
     |                     |                  |     ID = 0xACCE1111    |
     |<--------------------|<-----------------|<-----------------------|
     |  (verifica ID)      |                  |                        |
     |                     |                  |                        |
     |  MMIO WR DIR_IN / DIR_OUT / NUM_PIXELS |                        |
     |-------------------->|--- TLM WRITE --->|----- b_transport ----->| (guarda)
     |                     |                  |                        |
     |  MMIO WR CTRL=START |                  |                        |
     |-------------------->|--- TLM WRITE --->|----- b_transport ----->| notify()
     |<-- OK --------------|<-----------------|<-----------------------|    |
     |                     |                  |          +-------------v-----------+
     |  bucle:             |                  |          | SC_THREAD proceso():    |
     |   MMIO RD STATUS    |                  |          |  cargar input .rgb      |
     |-------------------->|--- TLM READ ---->|--------->|  rgb_a_gris (BT.601)    |
     |<-- BUSY/DONE -------|<-----------------|<---------|  wait(npix*4 ns)        |
     |   (repite hasta DONE)                  |          |  guardar output .raw    |
     |                     |                  |          |  done = true            |
     |                     |                  |          +-------------------------+
     |  STATUS & DONE == 1 |                  |                        |
     |  -> EOT al UART -> gem5 finaliza       |                        |
```

---

## 7. Formato de las transacciones

Cada acceso MMIO de la CPU a la ventana del acelerador genera una transacción
`tlm::tlm_generic_payload` de 4 bytes, entregada por el `Gem5ToTlmBridge32` al método
`b_transport()` del módulo SystemC.

| Campo del *payload* | Valor | Descripción |
|---|---|---|
| `command` | `TLM_READ_COMMAND` / `TLM_WRITE_COMMAND` | Según sea lectura o escritura |
| `address` | Offset dentro de `0x2F000000` | El puente entrega el offset local |
| `data_ptr` | Puntero a 4 bytes | Valor del registro |
| `data_length` | `4` | Registro de 32 bits |
| `streaming_width` | `4` | Sin *streaming* |
| `byte_enable_ptr` | `nullptr` | No se emplean *byte enables* |
| `response_status` | `TLM_OK_RESPONSE` / `TLM_BURST_ERROR_RESPONSE` | Error si `data_length` ≠ 4 |

### Secuencia de transacciones generada por el programa en C

| Paso | Comando | Registro (offset) | Valor | Sentido |
|---|---|---|---|---|
| Identificar | READ | `ID` (0x14) | `0xACCE1111` | acel. → CPU |
| Programar | WRITE | `DIR_IN` (0x00) | `0` | CPU → acel. |
| Programar | WRITE | `DIR_OUT` (0x04) | `0` | CPU → acel. |
| Programar | WRITE | `NUM_PIXELS` (0x08) | `2 073 600` | CPU → acel. |
| Arrancar | WRITE | `CTRL` (0x0C) | `0x1` (START) | CPU → acel. |
| Sondear | READ | `STATUS` (0x10) | `BUSY` / `DONE` | acel. → CPU |

---

## 8. Mapa de memoria y registros

### Mapa de memoria del SoC simulado

| Rango | Periférico | Función |
|---|---|---|
| `0x1C090000` | UART PL011 | Consola del programa en C |
| `0x2F000000` – `0x2F000FFF` | `ImgAccel` | Ventana de registros del acelerador |
| `0x80000000` … | DRAM | Memoria principal; carga y ejecuta el ELF |

### Registros del acelerador (base `0x2F000000`)

| Offset | Nombre | Acceso | Descripción |
|---|---|---|---|
| `0x00` | `DIR_IN` | RW | Base lógica de la imagen de entrada |
| `0x04` | `DIR_OUT` | RW | Base lógica de la imagen de salida |
| `0x08` | `NUM_PIXELS` | RW | Píxeles a procesar (2 073 600) |
| `0x0C` | `CTRL` | WO | bit 0 = `START` |
| `0x10` | `STATUS` | RO | bit 0 = `BUSY`, bit 1 = `DONE` |
| `0x14` | `ID` | RO | Identificador `0xACCE1111` |

> La dirección base está definida en dos ubicaciones que deben mantenerse consistentes:
> `virtual_prototype/sw/accel.h` (software) y `virtual_prototype/gem5-config/vp_arm.py`
> (sistema). El registro de identificación permite al programa detectar una eventual
> inconsistencia entre ambas.

### Mapa AXI4-Lite de la implementación en HLS

> ⬜ Pendiente: completar con el mapa generado por Vitis.

---

## 9. Resultados obtenidos

### 9.1 Entorno de ejecución

| Componente | Versión |
|---|---|
| Sistema operativo | Debian 13 (Trixie) |
| gem5 | 25.1.0.1 |
| SystemC | 3.0.2 (Accellera) |
| Cross-compiler | `aarch64-linux-gnu-gcc` 14.2.0 |
| CPU simulada | `AtomicSimpleCPU`, ARMv8-A, 1 GHz |
| Plataforma | `VExpress_GEM5_V2` |

### 9.2 Verificación del acelerador de forma independiente

```
        SystemC 3.0.2-Accellera
[CPU] ID leido = 0xacce1111
[CPU] registros programados (npix=2073600)
[CPU] START enviado, sondeando STATUS...
[CPU] DONE tras 9 sondeos (9 ms)
[TB] simulacion OK
>> Comparando con la referencia...
OK: la salida es identica bit a bit a la referencia.
```

El acelerador respondió con el identificador esperado, aceptó la configuración, inició la
operación al recibir `START` y señalizó su terminación tras nueve sondeos del registro de estado.

### 9.3 Compilación del programa en C

| Atributo | Valor |
|---|---|
| Formato / Arquitectura | ELF64 / AArch64 |
| Tipo | `EXEC` (ejecutable, *bare-metal*) |
| Punto de entrada | `0x80000000` |
| Intérprete dinámico | Ninguno |

La ausencia de intérprete dinámico confirma que el binario es efectivamente *bare-metal* y
puede ser cargado directamente por gem5.

### 9.4 Ejecución del prototipo virtual completo

```
gem5 Simulator System.  https://www.gem5.org
gem5 version 25.1.0.1
...
info: kernel located at: .../sw/accel_app.elf
[ImgAccel] periferico en linea (in=.../input_1080p.rgb,
                                out=.../output_1080p_gray.raw)
== Prototipo virtual ARM64 + acelerador SystemC: iniciando ==
== Simulacion terminada @ tick 1312000: UART received EOT ==
>> Salida generada: .../images/output/output_1080p_gray.raw
-rw-r--r-- 1 daniel daniel 2073600 .../output_1080p_gray.raw
```

La simulación finalizó de forma controlada mediante el byte `EOT` (0x04) que la rutina de
arranque envía al UART al retornar de `main()`. Ello confirma que el programa completó su
ejecución: alcanzó el final del bucle de sondeo, lo que implica que el acelerador señalizó su
terminación.

### 9.5 Comprobación de resultados

```
$ md5sum images/output/output_1080p_gray.raw docs/reference_output_gray.raw

45041cfcf4f0f1f0d8cdc2230acee01b  images/output/output_1080p_gray.raw
45041cfcf4f0f1f0d8cdc2230acee01b  docs/reference_output_gray.raw
```

| Criterio | Resultado | Estado |
|---|---|---|
| Tamaño de la salida | 2 073 600 bytes | ✅ |
| Suma MD5 | Coincide con la referencia | ✅ |
| Bytes discrepantes | 0 de 2 073 600 | ✅ |
| Terminación | `UART received EOT` | ✅ |

La coincidencia de las sumas de comprobación indica que los 2 073 600 bytes de ambas imágenes
son idénticos: la migración del modelo a una arquitectura ARM64 con comunicación TLM-2.0
preservó íntegramente la funcionalidad del acelerador original.

### 9.6 Análisis

**Correctitud funcional.** La equivalencia bit a bit constituye el criterio más estricto de
verificación disponible: no admite tolerancias. Su cumplimiento demuestra que las adaptaciones
introducidas —el envoltorio del socket para el puente de gem5 y la modificación del camino de
datos— no alteraron la función de transferencia del acelerador.

**Modelo temporal.** El acelerador modela su latencia como `t = N_px × 4 ns`, correspondiente a
un píxel por ciclo a 250 MHz. Para la imagen completa: `t = 2 073 600 × 4 ns = 8,29 ms`. Este
valor asume un *Initiation Interval* unitario, suposición que se contrasta con los resultados
de síntesis en la [sección 10](#103-calibración-del-modelo-de-latencia).

**Validez del prototipo.** El programa en C no contiene ninguna construcción específica de la
simulación: únicamente accede a direcciones de memoria mediante punteros `volatile`. En
consecuencia, el mismo código fuente, recompilado, sería válido para controlar el IP sintetizado
sobre la KV260, modificando únicamente la dirección base del periférico.

### 9.7 Problemas de integración encontrados

| Problema | Causa | Solución |
|---|---|---|
| `fatal: Failed to open file /lib/ld-linux-aarch64.so.1` | GCC 14 genera ejecutables PIE por defecto, que requieren un intérprete dinámico inexistente en *bare-metal*. Las opciones `-nostdlib` y `-nostartfiles` **no** lo desactivan. | Añadir `-fno-pie -no-pie -static`. Verificar con `readelf -l` la ausencia de segmento `INTERP`. |
| `fatal: membus has two ports responding within range [0x2f000000:0x80000000]` | El puente se conectó al `membus`, cuyo rango ya cubre el `iobridge` de la plataforma VExpress. | Conectar el puente al `iobus`, destinado a periféricos *memory-mapped*. |

### 9.8 Resultados de síntesis (HLS)

> ⬜ Pendiente. Ver [`hls/README.md`](hls/README.md).

---

## 10. Correspondencia entre ambas implementaciones

Ambas implementaciones derivan del mismo modelo de referencia, por lo que deben resultar
funcionalmente equivalentes.

```
                     Modelo de referencia (Evaluación 1)
                      acelerador SystemC — BT.601 entero
                                    |
                +-------------------+-------------------+
                |                                       |
       Implementación HLS                      Prototipo Virtual
    (Vitis 2024.1 → RTL → KV260)          (gem5 ARM64 + SystemC/TLM-2.0)
                |                                       |
                +-------------------+-------------------+
                                    |
                    Verificación cruzada: misma imagen de salida
```

### 10.1 Equivalencia funcional

```bash
./scripts/cross_check.sh <salida_del_testbench_HLS.raw>
```

| Implementación | Suma MD5 | Estado |
|---|---|---|
| Modelo de referencia (Eval. 1) | `45041cfcf4f0f1f0d8cdc2230acee01b` | ✅ |
| Prototipo virtual | `45041cfcf4f0f1f0d8cdc2230acee01b` | ✅ |
| Implementación HLS | — | ⬜ Pendiente |

**Requisito de aritmética entera.** La equivalencia bit a bit sólo se cumple si el HLS emplea la
misma aritmética entera. El uso de `float`, `double` o `ap_fixed` introduce diferencias de
redondeo. Interpretación del diagnóstico que emite `cross_check.sh`:

| Error absoluto máximo | Diagnóstico | Acción |
|---|---|---|
| 0 | Equivalencia bit a bit | Ninguna |
| 1 | Diferencia de redondeo: se emplea punto flotante o coma fija. La lógica es correcta. | Migrar a aritmética entera |
| 2 – 5 | Coeficientes o política de redondeo distintos | Revisar coeficientes y desplazamiento |
| > 10 | Error lógico: canales intercambiados, fórmula distinta o desbordamiento | Depurar el núcleo |

Como referencia, una implementación en punto flotante con coeficientes decimales difiere del
modelo entero en aproximadamente el **47 % de los píxeles, pero siempre en ±1**. Una proporción
elevada de píxeles discrepantes no implica necesariamente un error lógico.

### 10.2 Correspondencia de los mapas de registros

Ambas implementaciones exponen el mismo dispositivo desde la perspectiva del software; sólo
difiere el mecanismo de transporte.

| Función | Prototipo Virtual (TLM-2.0) | HLS (AXI4-Lite) |
|---|---|---|
| Iniciar la operación | `CTRL` (0x0C), bit 0 | `ap_start` |
| Consultar terminación | `STATUS` (0x10), bit 1 | `ap_done` / `ap_idle` |
| Indicar ocupado | `STATUS` (0x10), bit 0 | `ap_ready` / `ap_idle` |
| Dirección base de entrada | `DIR_IN` (0x00) | Puerto AXI4 Master |
| Dirección base de salida | `DIR_OUT` (0x04) | Puerto AXI4 Master |
| Cantidad de píxeles | `NUM_PIXELS` (0x08) | Argumento escalar AXI4-Lite |
| Identificación | `ID` (0x14) | No aplica |

El protocolo de control es equivalente: escribir un bit de arranque y sondear un bit de
terminación. El programa en C del prototipo virtual constituye, conceptualmente, el mismo
controlador que se requeriría para operar el IP sobre la KV260.

### 10.3 Calibración del modelo de latencia

| Parámetro | Modelo del VP | Síntesis HLS |
|---|---|---|
| Frecuencia | 250 MHz | ⬜ Pendiente |
| *Initiation Interval* | 1 | ⬜ Pendiente |
| Latencia estimada | 8,29 ms | ⬜ Pendiente |

Si el *Initiation Interval* difiere de la unidad, el modelo del prototipo virtual debe ajustarse
según `t = N_px · II · T_clk` en `sc_img_accel.cc`.

### 10.4 Diferencia arquitectónica en el camino de datos

| | Implementación HLS | Prototipo Virtual |
|---|---|---|
| Control | AXI4-Lite | MMIO → TLM-2.0 |
| Datos | AXI4 Master: el IP accede a la memoria del sistema | E/S directa del acelerador contra archivos del anfitrión |
| Sockets | Dos: control (esclavo) y datos (maestro) | Uno: el acelerador es *target* puro |

**Justificación.** En el prototipo virtual el sistema ARM64 se ejecuta en modo *bare-metal*, sin
sistema de archivos, por lo que la CPU no puede cargar la imagen desde disco hacia la memoria
sin habilitar *semihosting*. Se optó por modelar el almacenamiento persistente como
entrada/salida del propio acelerador —de forma coherente con el rol que ya desempeñaba el módulo
de almacenamiento en la evaluación anterior—, lo que concentra el tráfico TLM en el control del
dispositivo. El enunciado autoriza expresamente adaptaciones del modelo, y el resultado es
funcionalmente idéntico según demuestra la verificación de la [sección 9.5](#95-comprobación-de-resultados).

---

## 11. Declaración sobre el uso de Inteligencia Artificial

De acuerdo con lo establecido en el enunciado, se declara el uso de herramientas de Inteligencia
Artificial durante el desarrollo de esta evaluación.

### 11.1 Parte de prototipo virtual

Se utilizó **Claude (Anthropic)** a través de su interfaz de conversación, con los siguientes
propósitos:

- **Consulta de conceptos y de API de integración.** Se consultó el procedimiento para integrar
  un módulo SystemC como periférico TLM-2.0 dentro de un SoC ARM64 en gem5.
  *Prompt representativo:* «cómo conectar un *target* SystemC TLM-2.0 al bus de un sistema ARM64
  en gem5 mediante `Gem5ToTlmBridge` y `SystemC_Kernel`».

- **Generación de código base.** A partir del acelerador de la evaluación anterior, se generó con
  asistencia el andamiaje del módulo integrado a gem5 (`sc_img_accel.hh/.cc`, `ImgAccel.py`,
  `SConscript`), el programa en C *bare-metal* (`main.c`, `startup.S`, `link.ld`), la
  configuración de gem5 y los scripts de automatización.
  *Prompt representativo:* «adaptar el acelerador RGB a grises de SystemC para que una CPU ARM64
  lo controle por MMIO, y escribir el programa en C que lo maneja».

- **Depuración.** Se utilizó asistencia para diagnosticar los dos problemas de integración
  documentados en la sección 9.7: la generación de un ejecutable con enlace dinámico y el
  conflicto de rangos de direcciones entre el `membus` y el `iobridge`.

- **Generación de diagramas y mejora de la redacción.** Los diagramas de bloques y de secuencias
  en formato de texto, así como la redacción de esta documentación técnica, se elaboraron con
  asistencia a partir del código del proyecto.

La totalidad del código fue revisada, integrada y verificada por el equipo. La fórmula de
conversión y el mapa de registros provienen del trabajo propio desarrollado en la evaluación
anterior.

### 11.2 Parte de implementación en HLS

> ⬜ **Pendiente — obligatorio.** El equipo encargado de la implementación en HLS debe completar
> esta subsección indicando si empleó herramientas de Inteligencia Artificial, con qué propósito
> y los *prompts* representativos. El enunciado establece que **la omisión de esta declaración
> implica la aplicación de la normativa de plagio**; si no se utilizaron dichas herramientas,
> debe indicarse explícitamente.

---

## Reporte técnico

El reporte en LaTeX se encuentra en [`docs/reporte/`](docs/reporte/). Para compilarlo:

```bash
cd docs/reporte
pdflatex reporte_MP6160.tex && pdflatex reporte_MP6160.tex
```

Los bloques pendientes se localizan con:

```bash
grep -n "pendiente{" docs/reporte/reporte_MP6160.tex
```
