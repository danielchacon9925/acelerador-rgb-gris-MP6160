/* uart.h -- Driver minimo del UART PL011 (solo TX por polling).
 *
 * gem5 con la plataforma VExpress_GEM5 expone un ARM PL011 en 0x1c090000,
 * conectado a un Terminal. Escribir bytes en el registro de datos (DR)
 * imprime en la consola de gem5. Con esto el programa bare-metal reporta su
 * progreso sin necesidad de un sistema operativo.
 */
#ifndef UART_H
#define UART_H

#include <stdint.h>

#define PL011_BASE   0x1c090000UL
#define PL011_DR     0x00          /* Data Register            */
#define PL011_FR     0x18          /* Flag Register            */
#define PL011_FR_TXFF (1u << 5)    /* Transmit FIFO Full       */

static inline void uart_putc(char c)
{
    volatile uint32_t *fr = (volatile uint32_t *)(PL011_BASE + PL011_FR);
    volatile uint32_t *dr = (volatile uint32_t *)(PL011_BASE + PL011_DR);
    while (*fr & PL011_FR_TXFF) { }   /* esperar espacio en la FIFO */
    *dr = (uint32_t)c;
}

static inline void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

/* Imprime un uint32_t en hexadecimal (0x........). */
static inline void uart_puthex(uint32_t v)
{
    const char *hex = "0123456789abcdef";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4)
        uart_putc(hex[(v >> i) & 0xF]);
}

/* Imprime un uint32_t en decimal. */
static inline void uart_putdec(uint32_t v)
{
    char buf[11];
    int i = 0;
    if (v == 0) { uart_putc('0'); return; }
    while (v) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i--) uart_putc(buf[i]);
}

#endif /* UART_H */
