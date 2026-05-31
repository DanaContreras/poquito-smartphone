/**
 * Grabación de audio con MAX9814 y Arduino UNO
 * Configurado para 8000 Hz, 8-bit PCM (Unsigned)
 * Compatible con record_audio.py
 */

const int PIN_MIC = A0;
const unsigned long INTERVALO_MUESTREO = 125; // microsegundos (1/8000 Hz = 125us)
unsigned long ultimoTiempo = 0;

void setup() {
  // Inicializar comunicación serie rápida para evitar cuellos de botella
  Serial.begin(115200);

  // Acelerar el ADC de Arduino UNO (ATmega328P)
  // El prescaler por defecto es 128 (16MHz/128 = 125kHz de reloj ADC)
  // Una conversión toma 13 ciclos de reloj ADC (~104us).
  // Para 8kHz tenemos 125us de presupuesto total, lo cual es muy justo.
  // Cambiamos el prescaler a 16 (16MHz/16 = 1MHz reloj ADC)
  // La conversión ahora tomará aproximadamente 13-15us, dándonos margen.
  
  ADCSRA &= ~(bit(ADPS0) | bit(ADPS1)); // Limpiar bits 0 y 1
  ADCSRA |= bit(ADPS2);                 // Poner bit 2 a 1 (Prescaler 16: 1 0 0)
  
  ultimoTiempo = micros();
}

void loop() {
  unsigned long tiempoActual = micros();

  // Bucle de temporización preciso basado en micros()
  if (tiempoActual - ultimoTiempo >= INTERVALO_MUESTREO) {
    // Incrementamos el tiempo de referencia para evitar derivas
    ultimoTiempo += INTERVALO_MUESTREO;

    // Leer el valor analógico (10 bits: 0 - 1023)
    int lectura = analogRead(PIN_MIC);

    // Reducir la resolución a 8 bits (0 - 255) para el formato WAV de 8 bits
    byte muestra = (byte)(lectura >> 2);

    // Enviar el byte en crudo (RAW) por el puerto serie
    Serial.write(muestra);
  }
}
