import serial
import wave
import time
import sys
import os

# Configuración por defecto
BAUD_RATE = 115200
SAMPLE_RATE = 8000
DURATION = 5  # Segundos a grabar
OUTPUT_FILENAME = 'grabacion.wav'

def find_arduino_port():
    """Intenta detectar el puerto serie del Arduino."""
    if os.path.exists('../../scripts/detect_port.sh'):
        import subprocess
        try:
            port = subprocess.check_output(['bash', '../../scripts/detect_port.sh']).decode('utf-8').strip()
            if port:
                return port
        except:
            pass
    
    # Fallback comunes en Linux
    for i in range(4):
        port = f'/dev/ttyUSB{i}'
        if os.path.exists(port): return port
        port = f'/dev/ttyACM{i}'
        if os.path.exists(port): return port
    return None

def main():
    port = find_arduino_port()
    
    if len(sys.argv) > 1:
        port = sys.argv[1]
    
    if not port:
        print("Error: No se detectó ningún puerto serie. Por favor especifícalo como argumento.")
        print("Uso: python record_audio.py /dev/ttyUSB0")
        sys.exit(1)

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        # El reinicio del puerto suele resetear el Arduino, esperamos un momento
        print(f"Conectado a {port}. Esperando inicialización...")
        time.sleep(2) 
        ser.reset_input_buffer()
    except Exception as e:
        print(f"Error al abrir el puerto {port}: {e}")
        sys.exit(1)

    print(f"Iniciando grabación de {DURATION} segundos...")
    print("Habla al micrófono ahora!")

    num_samples = SAMPLE_RATE * DURATION
    audio_data = bytearray()

    start_time = time.time()
    try:
        while len(audio_data) < num_samples:
            if ser.in_waiting > 0:
                # Leer lo que esté disponible sin pasarnos del total
                chunk = ser.read(min(ser.in_waiting, num_samples - len(audio_data)))
                audio_data.extend(chunk)
            
            # Mostrar progreso cada segundo
            elapsed = time.time() - start_time
            if int(elapsed * 2) % 2 == 0: # Simple indicador visual
                sys.stdout.write(f"\rProgreso: {len(audio_data)}/{num_samples} muestras ({(len(audio_data)/num_samples)*100:.1f}%)")
                sys.stdout.flush()

    except KeyboardInterrupt:
        print("\nGrabación interrumpida por el usuario.")

    print(f"\nGrabación finalizada. Tiempo real: {time.time() - start_time:.2f}s")
    ser.close()

    if len(audio_data) == 0:
        print("Error: No se recibieron datos. ¿Está el Arduino enviando información?")
        sys.exit(1)

    # Guardar como archivo WAV (PCM 8-bit Unsigned)
    try:
        with wave.open(OUTPUT_FILENAME, 'wb') as wf:
            wf.setnchannels(1)      # Mono
            wf.setsampwidth(1)      # 8 bits (1 byte por muestra)
            wf.setframerate(SAMPLE_RATE)
            wf.writeframes(audio_data)
        print(f"Audio guardado exitosamente en: {os.path.abspath(OUTPUT_FILENAME)}")
    except Exception as e:
        print(f"Error al guardar el archivo WAV: {e}")

if __name__ == "__main__":
    main()
