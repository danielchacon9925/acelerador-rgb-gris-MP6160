/* accel.h -- Interfaz del periferico acelerador vista desde el software.
 *
 * Los offsets de registro coinciden con systemc/img_accel_vp/accel_core.h
 * (la fuente unica de verdad del contrato HW/SW). ACCEL_BASE es la direccion
 * base del periferico en el mapa de memoria del SoC ARM64: debe coincidir con
 * el rango asignado al puente Gem5ToTlmBridge en la config de gem5
 * (gem5-config/vp_arm.py, variable ACCEL_BASE).
 */
#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>

/* Direccion base del periferico (MMIO). Cambiar aqui y en vp_arm.py juntos. */
#define ACCEL_BASE   0x2f000000UL

/* Offsets de registro (deben coincidir con accel_core.h) */
#define REG_DIR_IN     0x00
#define REG_DIR_OUT    0x04
#define REG_NUM_PIXELS 0x08
#define REG_CTRL       0x0C
#define REG_STATUS     0x10
#define REG_ID         0x14

/* Campos de bits */
#define CTRL_START     0x1u
#define ST_BUSY        0x1u
#define ST_DONE        0x2u
#define MAGIC_ID       0xACCE1111u

/* Parametros de la imagen (1080p) */
#define IMG_ANCHO      1920u
#define IMG_ALTO       1080u
#define IMG_PIXELS     (IMG_ANCHO * IMG_ALTO)   /* 2 073 600 */

/* Accesores MMIO de 32 bits. 'volatile' impide que el compilador
 * reordene o elimine los accesos al dispositivo. */
static inline void accel_wr(uint32_t off, uint32_t val)
{
    *(volatile uint32_t *)(ACCEL_BASE + off) = val;
}

static inline uint32_t accel_rd(uint32_t off)
{
    return *(volatile uint32_t *)(ACCEL_BASE + off);
}

#endif /* ACCEL_H */
