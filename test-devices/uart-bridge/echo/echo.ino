/*
 * ###########################################################
 * #### Test A (uart-bridge) - lado ESP (ESP8266 / ESP32) ####
 * ###########################################################
 *
 * Eco por UART de HARDWARE. Ambas placas se alimentan por cargador,
 * asi que UART0 (Serial) queda libre para el enlace con el Arduino:
 * recibe cada byte y lo devuelve tal cual. El LED onboard parpadea
 * por cada byte recibido, como indicador visual de trafico.
 *
 * Portabilidad: el codigo es identico para ESP8266 y ESP32 (ambos usan UART0).
 */

#define LINK_BAUD 9600

void setup() {
  Serial.begin(LINK_BAUD);            // UART0 de hardware = enlace con el Arduino
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  static bool led = false;

  if (Serial.available()) {
    int b = Serial.read();
    Serial.write((uint8_t)b);         // eco de vuelta al Arduino
    led = !led;
    digitalWrite(LED_BUILTIN, led);   // parpadeo por cada byte recibido
  }
}
