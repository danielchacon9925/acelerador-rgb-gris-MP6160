/* startup.S -- Arranque bare-metal AArch64 para gem5.
 *
 * gem5 entrega el control en la direccion de entrada del ELF con la MMU
 * desactivada (direccionamiento fisico), justo lo que necesitamos para el
 * MMIO al acelerador. Aqui solo preparamos el stack, limpiamos .bss y
 * saltamos a main(). Al volver, enviamos EOT (0x04) por el UART para que
 * gem5 termine la simulacion limpiamente (uart.end_on_eot = True).
 */
    .section .text.boot
    .global _start
_start:
    /* Configurar el puntero de pila */
    ldr     x0, =_stack_top
    mov     sp, x0

    /* Limpiar la seccion .bss */
    ldr     x1, =__bss_start
    ldr     x2, =__bss_end
1:  cmp     x1, x2
    b.ge    2f
    str     xzr, [x1], #8
    b       1b

2:  /* Saltar a main() */
    bl      main

    /* Enviar EOT (0x04) al PL011 (0x1c090000) para cerrar la simulacion */
    ldr     x3, =0x1c090000
    mov     w4, #4
    str     w4, [x3]

3:  wfi
    b       3b
