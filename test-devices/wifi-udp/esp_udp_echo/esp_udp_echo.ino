/*
 * ###########################################################
 * #### Test B (wifi-udp) - lado ESP (ESP8266 / ESP32)    ####
 * ###########################################################
 *
 * Eco por Wi-Fi UDP. El ESP se conecta a la WLAN (modo STA), imprime su IP
 * por Serial (USB) y escucha en UDP_PORT: cada datagrama que recibe lo
 * devuelve tal cual al emisor. El LED onboard parpadea por cada paquete.
 */

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif
#include <WiFiUdp.h>

#include "secrets.h"   // define WIFI_SSID y WIFI_PASS

#define UDP_PORT     4210   // puerto de escucha
#define RX_BUF_SIZE  512    // tamano maximo de datagrama que ecoamos

WiFiUDP udp;
static uint8_t rxBuf[RX_BUF_SIZE];

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);                    // estacion (cliente), no access point
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("\nConectando a ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Escuchando UDP en el puerto ");
  Serial.println(UDP_PORT);

  udp.begin(UDP_PORT);
}

void loop() {
  static bool led = false;

  if (udp.parsePacket() <= 0) return;

  int n = udp.read(rxBuf, RX_BUF_SIZE);
  if (n <= 0) return;

  // Eco: responder al mismo emisor (su IP/puerto). Esto permite que la PC
  // descubra la IP del ESP a partir del origen del datagrama de respuesta.
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  udp.write(rxBuf, n);
  udp.endPacket();

  led = !led;
  digitalWrite(LED_BUILTIN, led);         // parpadeo por cada paquete recibido
}
