# Makefile para compilar codigo AVR (Arduino) con avr-gcc
# Test MAX9814 Microfono

# Configuracion del microcontrolador (Arduino UNO usa ATmega328P)
MCU = atmega328p
F_CPU = 16000000UL
BAUD = 115200

# Herramientas del compilador
CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

_PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)
_DRIVERS_DIR := $(_PROJECT_ROOT)/drivers

# Puerto serial - auto-deteccion (override con: make PORT=/dev/ttyUSBx upload)
PORT := $(shell $(_PROJECT_ROOT)/scripts/detect_port.sh)

# Programador (Arduino UNO usa arduino)
PROGRAMMER = arduino

# Flags de compilacion
CFLAGS = -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I$(_DRIVERS_DIR)
LDFLAGS = -mmcu=$(MCU)

# Archivos del test
TARGET = record_max9814
TEST_SRC = record_max9814.c

# Drivers necesarios
DRIVER_SRC = $(_DRIVERS_DIR)/uart.c $(_DRIVERS_DIR)/adc.c

# Objeto
OBJ = $(TEST_SRC:.c=.o) $(notdir $(DRIVER_SRC:.c=.o))
vpath %.c $(dir $(DRIVER_SRC))

HEX = $(TARGET).hex
ELF = $(TARGET).elf

# Regla por defecto
all: $(HEX) size

# Compilar archivo .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Enlazar archivo .o a .elf
$(ELF): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

# Convertir .elf a .hex
$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Mostrar tamano del programa
size: $(ELF)
	@echo "Tamano del programa:"
	$(SIZE) --format=avr --mcu=$(MCU) $(ELF)

# Cargar el programa al Arduino
upload: $(HEX)
	@echo "Puerto detectado: $(PORT)"
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b 115200 -U flash:w:$(HEX):i

# Limpiar archivos generados
clean:
	rm -f $(OBJ) $(ELF) $(HEX)

# Abrir monitor serial
monitor:
	screen $(PORT) $(BAUD)

.PHONY: all size upload clean monitor
