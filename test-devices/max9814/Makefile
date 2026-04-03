# Makefile para compilar código AVR (Arduino) con avr-gcc

# Configuración del microcontrolador (Arduino UNO usa ATmega328P)
MCU = atmega328p
F_CPU = 16000000UL
BAUD = 9600

# Herramientas del compilador
CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

# Puerto serial (ajustar según tu sistema)
# Linux: /dev/ttyACM0 o /dev/ttyUSB0
# Windows: COM3, COM4, etc.
PORT = /dev/ttyUSB0

# Programador (Arduino UNO usa arduino)
PROGRAMMER = arduino

# Flags de compilación
CFLAGS = -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)
LDFLAGS = -mmcu=$(MCU)

# Archivos
TARGET = test_max9814
SRC = test_max9814.c
OBJ = $(SRC:.c=.o)
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

# Mostrar tamaño del programa
size: $(ELF)
	@echo "Tamaño del programa:"
	$(SIZE) --format=avr --mcu=$(MCU) $(ELF)

# Cargar el programa al Arduino
upload: $(HEX)
	$(AVRDUDE) -p $(MCU) -c $(PROGRAMMER) -P $(PORT) -b 115200 -U flash:w:$(HEX):i

# Limpiar archivos generados
clean:
	rm -f $(OBJ) $(ELF) $(HEX)

# Abrir monitor serial
monitor:
	screen $(PORT) $(BAUD)

.PHONY: all size upload clean monitor
