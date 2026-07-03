"""Graba audio PCM 8-bit / 8 kHz recibido por UART y lo guarda como .wav.

Script compartido: sirve para cualquier test que respete el contrato de bytes
(8000 muestras/segundo, 1 byte por muestra, 115200 baud). Por defecto guarda el
.wav en el DIRECTORIO ACTUAL, es decir, en la carpeta del test desde la que se
ejecuta. Ejemplos:

    # parado en la carpeta del test en C:
    cd test-devices/microphone/test_microphone_c
    uv run ../../../scripts/record_audio.py            # -> ./grabacion.wav

    # o desde la raíz, indicando la carpeta destino:
    uv run scripts/record_audio.py -o test-devices/microphone/test_microphone_ino
"""

import argparse
import os
import subprocess
import sys
import time
import wave
from pathlib import Path

import serial

# Configuración por defecto
BAUD_RATE = 115200
SAMPLE_RATE = 8000
DURATION = 5  # Segundos a grabar
OUTPUT_FILENAME = 'grabacion.wav'


def find_arduino_port():
    """Intenta detectar el puerto serie del Arduino."""
    # detect_port.sh vive en scripts/, en la raíz del repo (dos niveles arriba).
    detect_script = Path(__file__).resolve().parents[2] / "scripts" / "detect_port.sh"
    if detect_script.exists():
        try:
            port = subprocess.check_output(['bash', str(detect_script)]).decode('utf-8').strip()
            if port:
                return port
        except Exception:
            pass

    # Fallback: puertos comunes en Linux.
    for i in range(4):
        for pattern in (f'/dev/ttyUSB{i}', f'/dev/ttyACM{i}'):
            if os.path.exists(pattern):
                return pattern
    return None


def resolve_output_path(output_arg):
    """Devuelve la ruta final del .wav.

    - Sin argumento: 'grabacion.wav' en el directorio actual (la carpeta del test
      que corre el script).
    - Si se pasa una carpeta: <carpeta>/grabacion.wav.
    - Si se pasa un archivo (.wav): se usa tal cual.
    """
    if not output_arg:
        return Path.cwd() / OUTPUT_FILENAME
    path = Path(output_arg)
    # Trata como carpeta si existe como tal o si termina en separador.
    if path.is_dir() or output_arg.endswith((os.sep, '/')):
        return path / OUTPUT_FILENAME
    return path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Graba audio (PCM 8-bit, 8 kHz, mono) por UART y lo guarda como WAV "
                    "en la carpeta actual (la del test que ejecuta el script)."
    )
    parser.add_argument('port', nargs='?', default=None,
                        help="Puerto serie (ej: /dev/ttyUSB0). Si se omite, se autodetecta.")
    parser.add_argument('-o', '--output', default=None,
                        help="Carpeta o archivo .wav de destino (por defecto: ./grabacion.wav).")
    parser.add_argument('-d', '--duration', type=int, default=DURATION,
                        help=f"Segundos a grabar (por defecto: {DURATION}).")
    return parser.parse_args()


def main():
    args = parse_args()
    port = args.port or find_arduino_port()
    output_path = resolve_output_path(args.output)
    duration = args.duration

    if not port:
        print("Error: No se detectó ningún puerto serie. Especificalo como argumento.")
        print("Uso: python record_audio.py [PUERTO] [-o CARPETA_O_ARCHIVO] [-d SEGUNDOS]")
        sys.exit(1)

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        # Abrir el puerto suele resetear el Arduino; esperamos su inicialización.
        print(f"Conectado a {port}. Esperando inicialización...")
        time.sleep(2)
        ser.reset_input_buffer()
    except Exception as e:
        print(f"Error al abrir el puerto {port}: {e}")
        sys.exit(1)

    print(f"Iniciando grabación de {duration} segundos...")
    print("Habla al micrófono ahora!")

    num_samples = SAMPLE_RATE * duration
    audio_data = bytearray()

    start_time = time.time()
    try:
        while len(audio_data) < num_samples:
            if ser.in_waiting > 0:
                # Leer lo disponible sin pasarnos del total.
                chunk = ser.read(min(ser.in_waiting, num_samples - len(audio_data)))
                audio_data.extend(chunk)

            sys.stdout.write(
                f"\rProgreso: {len(audio_data)}/{num_samples} muestras "
                f"({(len(audio_data) / num_samples) * 100:.1f}%)"
            )
            sys.stdout.flush()
    except KeyboardInterrupt:
        print("\nGrabación interrumpida por el usuario.")

    print(f"\nGrabación finalizada. Tiempo real: {time.time() - start_time:.2f}s")
    ser.close()

    if len(audio_data) == 0:
        print("Error: No se recibieron datos. ¿Está el Arduino enviando información?")
        sys.exit(1)

    # Asegurar que la carpeta destino existe.
    output_path.parent.mkdir(parents=True, exist_ok=True)

    # Guardar como archivo WAV (PCM 8-bit unsigned, mono).
    try:
        with wave.open(str(output_path), 'wb') as wf:
            wf.setnchannels(1)      # Mono
            wf.setsampwidth(1)      # 8 bits (1 byte por muestra)
            wf.setframerate(SAMPLE_RATE)
            wf.writeframes(audio_data)
        print(f"Audio guardado exitosamente en: {output_path.resolve()}")
    except Exception as e:
        print(f"Error al guardar el archivo WAV: {e}")


if __name__ == "__main__":
    main()
