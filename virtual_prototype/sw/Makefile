# Makefile -- Compilacion cruzada del programa bare-metal ARM64.
#
# Requiere un toolchain aarch64. Funciona con el cross de Debian/Ubuntu
# (aarch64-linux-gnu-gcc, usado en modo freestanding) o con uno bare-metal
# (aarch64-none-elf-gcc). Se autodetecta cual esta disponible.

CROSS   ?= $(shell command -v aarch64-none-elf-gcc >/dev/null 2>&1 \
             && echo aarch64-none-elf- || echo aarch64-linux-gnu-)
CC       = $(CROSS)gcc
OBJCOPY  = $(CROSS)objcopy
OBJDUMP  = $(CROSS)objdump

CFLAGS  = -march=armv8-a -mgeneral-regs-only -ffreestanding -nostdlib \
          -nostartfiles -fno-stack-protector -fno-pie -Wall -Wextra -O2
LDFLAGS = -Wl,-T,link.ld -Wl,--build-id=none -static -no-pie -nostdlib

TARGET  = accel_app.elf
OBJS    = startup.o main.o

all: $(TARGET) accel_app.dump

startup.o: startup.S
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c accel.h uart.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) link.ld
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

# Volcado de ensamblador para inspeccion / entrega
accel_app.dump: $(TARGET)
	$(OBJDUMP) -d $(TARGET) > $@

clean:
	rm -f $(OBJS) $(TARGET) accel_app.dump accel_app.bin

.PHONY: all clean
