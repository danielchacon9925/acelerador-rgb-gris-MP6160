/* main.c -- Programa en C que interactua con el periferico acelerador.
 *
 * Corre bare-metal sobre la CPU ARM64 simulada en gem5. Cada acceso a los
 * registros del acelerador (accel_wr / accel_rd) es un acceso MMIO que el
 * puente Gem5ToTlmBridge convierte en una transaccion TLM-2.0 hacia el
 * modulo SystemC. El flujo reproduce la orquestacion que en la Evaluacion 1
 * hacia el "CPU" de SystemC, ahora ejecutada por software real sobre ARM64:
 *
 *   1. Identificar el periferico (leer REG_ID).
 *   2. Programar DIR_IN, DIR_OUT y NUM_PIXELS.
 *   3. Escribir START en CTRL.
 *   4. Sondear STATUS hasta ver DONE.
 *
 * El movimiento de los pixeles (carga de la imagen y escritura del
 * resultado) lo realiza el propio acelerador contra el almacenamiento
 * persistente (archivos del host), de modo que el trafico TLM del prototipo
 * se concentra en el control del dispositivo.
 */

#include "accel.h"
#include "uart.h"

int main(void)
{
    uart_puts("\n=== Prototipo Virtual: acelerador RGB->gris (ARM64) ===\n");

    /* 1) Identificar el periferico -------------------------------------- */
    uint32_t id = accel_rd(REG_ID);
    uart_puts("[SW] REG_ID = ");
    uart_puthex(id);
    uart_puts("\n");
    if (id != MAGIC_ID) {
        uart_puts("[SW] ERROR: periferico no encontrado en ACCEL_BASE\n");
        return 1;
    }

    /* 2) Programar los registros de configuracion ----------------------- */
    accel_wr(REG_DIR_IN,     0);            /* base logica de la entrada  */
    accel_wr(REG_DIR_OUT,    0);            /* base logica de la salida   */
    accel_wr(REG_NUM_PIXELS, IMG_PIXELS);   /* 1920 x 1080 = 2 073 600    */
    uart_puts("[SW] registros programados (npix = ");
    uart_putdec(IMG_PIXELS);
    uart_puts(")\n");

    /* 3) Arrancar el acelerador ----------------------------------------- */
    accel_wr(REG_CTRL, CTRL_START);
    uart_puts("[SW] START enviado, esperando DONE...\n");

    /* 4) Sondear STATUS hasta que el acelerador termine ----------------- */
    uint32_t status;
    do {
        status = accel_rd(REG_STATUS);
    } while (!(status & ST_DONE));

    uart_puts("[SW] DONE. Imagen en grises generada por el periferico.\n");
    uart_puts("=== Fin ===\n");
    return 0;
}
