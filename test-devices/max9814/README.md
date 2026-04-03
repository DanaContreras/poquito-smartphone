# Test MAX9814 - Módulo de Micrófono

Proyecto de prueba para el módulo de micrófono MAX9814 con control automático de ganancia (AGC).

## Conexiones

| Arduino | MAX9814 |
|---------|---------|
| A0      | OUT     |
| 5V      | VDD     |
| GND     | GND     |

## Verificación de Conexiones ✓

Las conexiones especificadas son **correctas**:
- **OUT → A0**: El pin OUT entrega la señal analógica que debe leerse con un pin ADC
- **VDD → 5V**: El módulo acepta voltaje de 2.7V a 5.5V
- **GND → GND**: Conexión a tierra común

## Sobre el Módulo MAX9814

- **Tipo**: Amplificador de micrófono electret con AGC (Auto Gain Control)
- **Salida**: Señal analógica 0-2.45V (promedio 1.25V en silencio)
- **Ganancia**: Ajustable 40dB, 50dB o 60dB (pin Gain)
- **Respuesta**: 20Hz - 20kHz
- **Característica importante**: Tiene offset de 1.25V en silencio (no es cero)

## Instalación de Herramientas

### Linux (Debian/Ubuntu)
```bash
sudo apt-get update
sudo apt-get install gcc-avr avr-libc avrdude make
```

### Arch Linux
```bash
sudo pacman -S avr-gcc avr-libc avrdude make
```

### macOS
```bash
brew tap osx-cross/avr
brew install avr-gcc avrdude
```

## Compilación

Para compilar el código:
```bash
make
```

Esto generará el archivo `.hex` listo para cargar al Arduino.

## Cargar al Arduino

1. Conecta tu Arduino al puerto USB
2. Verifica/ajusta el puerto en el Makefile (variable PORT)
3. Carga el programa:
```bash
make upload
```

## Monitorear la Salida

Para ver los datos del micrófono en tiempo real:
```bash
make monitor
```

O usa el monitor serial de Arduino IDE a 9600 baudios.

## Qué Esperar

El programa mostrará en el monitor serial:
- **Raw**: Valor ADC instantáneo (0-1023)
- **Min/Max**: Valores mínimo y máximo durante la ventana de muestreo
- **Peak-to-Peak**: Diferencia entre pico máximo y mínimo

### Valores Típicos:
- **Silencio**: Raw ~512 (1.25V), Peak-to-Peak bajo (5-20)
- **Hablar normal**: Peak-to-Peak 50-200
- **Sonido fuerte**: Peak-to-Peak 200-500+

Si el Raw está alrededor de 512 (±50) en silencio, el módulo funciona correctamente.

## Ajustes Opcionales del Módulo

Si tu módulo tiene pines adicionales:

- **Pin Gain** (ganancia máxima):
  - Flotante: 60dB
  - GND: 50dB
  - VDD: 40dB

- **Pin AR** (attack/release ratio):
  - Flotante: 1:4000
  - VDD: 1:2000
  - GND: 1:500

## Limpieza

Para eliminar archivos compilados:
```bash
make clean
```

## Referencias

- [Datasheet MAX9814](https://www.analog.com/media/en/technical-documentation/data-sheets/max9814.pdf)
- [Adafruit MAX9814 Guide](https://learn.adafruit.com/adafruit-agc-electret-microphone-amplifier-max9814)
