# 📡 Wi-Fi Full-Duplex Voice Communicator (Wi-Voice)

![License](https://img.shields.io/badge/license-MIT-blue.svg) ![Platform](https://img.shields.io/badge/platform-Arduino%20Nano%20%7C%20FreeRTOS-green.svg) ![Status](https://img.shields.io/badge/status-Development-orange.svg)

Este proyecto implementa un sistema de intercomunicación **Full-Duplex** sobre Wi-Fi utilizando un microcontrolador basado en la forma de Arduino Nano y el sistema operativo en tiempo real **FreeRTOS**. El dispositivo permite realizar llamadas de voz bidireccionales en tiempo real (VoIP) y cuenta con un sistema de señalización (timbre) para alertar sobre llamadas entrantes.

## 📖 Idea General del Proyecto

El objetivo es crear un "Walkie-Talkie" moderno que no dependa de frecuencias de radio tradicionales, sino de una red IP local. A diferencia de un Walkie-Talkie tradicional (Half-Duplex, donde solo uno habla a la vez), este sistema es **Full-Duplex**, permitiendo que ambos usuarios hablen y escuchen simultáneamente, similar a una llamada telefónica. El sistema gestiona la adquisición de audio analógico, la digitalización, la paquetización UDP y la transmisión inalámbrica, todo orquestado por un RTOS para cumplir con los requisitos temporales.

---

## 📋 Requisitos Funcionales Principales

1.  **Transmisión de Audio Full-Duplex:** Capacidad de enviar y recibir paquetes de audio simultáneamente con una latencia mínima perceptible (< 200ms).
2.  **Conectividad Wi-Fi:** El dispositivo debe conectarse a una red WLAN existente o crear un punto de acceso (AP).
3.  **Señalización de Llamada (Ringing):**
    * Capacidad de iniciar una llamada presionando un botón.
    * Alerta sonora (buzzer) y visual (LED) en el dispositivo receptor cuando recibe una solicitud de conexión.
4.  **Gestión de Estados:** Manejo de estados: *Standby*, *Llamando*, *En Llamada*, *Error*.
5.  **Calidad de Audio:** Muestreo de audio a una frecuencia mínima de 8 kHz (calidad telefonía básica).

---

## 🛠 Lista de Hardware y Descripción

> **⚠️ Nota Técnica Importante:** Aunque el factor de forma deseado es **Arduino Nano**, el chip ATmega328P clásico no tiene suficiente RAM ni velocidad para manejar Wi-Fi + Stack TCP/IP + Audio en tiempo real con FreeRTOS.
>
> **Hardware Seleccionado:** Se utiliza el **Arduino Nano 33 IoT** (o ESP32 con formato Nano). Este mantiene el tamaño del Nano pero incluye un procesador ARM Cortex-M0+ y un módulo Wi-Fi NINA-W102 nativo.

| Componente | Descripción Técnica | Función en el Proyecto |
| :--- | :--- | :--- |
| **Arduino Nano 33 IoT** | MCU ARM Cortex-M0+ (48MHz), 32KB RAM, Wi-Fi integrado. | Cerebro del sistema, ejecuta FreeRTOS y gestión de red. |
| **Módulo Micrófono (MAX9814)** | Micrófono Electret con amplificador y control automático de ganancia (AGC). | **Sensor:** Captura las ondas sonoras y entrega una señal analógica al ADC del Arduino. |
| **Amplificador de Audio (LM386)** | Amplificador operacional de baja potencia. | Acondiciona la señal PWM/DAC del Arduino para mover el altavoz. |
| **Altavoz / Auricular** | Impedancia 8Ω, 0.5W. | **Actuador:** Reproduce el audio recibido y el tono de llamada. |
| **Pulsador (Push Button)** | Botón momentáneo normalmente abierto. | **Interfaz:** Iniciar llamada / Contestar / Colgar. |
| **Buzzer Piezoeléctrico** | Zumbador pasivo o activo. | **Actuador:** Genera el sonido de "Ringing" independiente del audio de voz. |
| **LEDs Indicadores** | Rojo (Standby/Error), Verde (En llamada). | Feedback visual del estado del sistema. |
