# Test de Grabación de Audio - MAX9814

Este directorio contiene los archivos necesarios para realizar una prueba de captura de audio real utilizando un micrófono MAX9814 y un Arduino UNO.

## 1. Conexiones de Hardware

Conectar el módulo MAX9814 al Arduino de la siguiente manera:

| Arduino | MAX9814 |
|---------|---------|
| A0      | OUT     |
| 5V      | VDD     |
| GND     | GND     |

---

## 2. Requisitos Previos

Se necesita tener Python instalado en tu computadora y la librería `pyserial`:

```bash
pip install pyserial
```

---

## 3. Instrucciones de Uso

### Paso A: Cargar el código al Arduino
1. Abrir el archivo `record_max9814.ino` con el IDE de Arduino.
2. Seleccionar placa (**Arduino Uno**) y el puerto serie correspondiente.
3. Hacer clic en el botón **Subir** (la flecha).

### Paso B: Ejecutar la grabación
Abrir una terminal en esta carpeta (`test-devices/max9814/record_max9814/`) y ejecutar:

```bash
python record_audio.py
```

*Nota: El script intentará detectar el puerto automáticamente. Si falla, puedes especificarlo manualmente, por ejemplo: `python record_audio.py /dev/ttyUSB0`.*

### Paso C: Resultado
El script grabará **5 segundos** de audio. Al finalizar, se verá un mensaje indicando que se ha guardado el archivo:
`grabacion.wav`

---

## 4. Escuchar el Audio

Se puede reproducir el archivo con cualquier reproductor multimedia o desde la terminal:

```bash
# En Linux
aplay grabacion.wav

# Si tienes ffmpeg instalado
ffplay grabacion.wav
```

Si el audio suena muy bajo o saturado, se puede ajustar la ganancia conectando el pin **Gain** del MAX9814 a GND (50dB) o VCC (40dB). Si se deja desconectado, la ganancia es de 60dB.
