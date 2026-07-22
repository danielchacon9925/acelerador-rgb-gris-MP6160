/* link.ld -- Ubica la imagen bare-metal en la base de DRAM de gem5 (ARM).
 *
 * La plataforma VExpress_GEM5 mapea la memoria principal en 0x80000000; el
 * ELF se carga y ejecuta desde ahi. El punto de entrada es _start (startup.S).
 */
ENTRY(_start)

SECTIONS
{
    . = 0x80000000;

    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }

    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }

    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    . = ALIGN(16);
    __bss_end = .;

    /* Pila de 64 KiB */
    . = ALIGN(16);
    . = . + 0x10000;
    _stack_top = .;
}
