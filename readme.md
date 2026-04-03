# Wi-Fi Full-Duplex Voice Communicator (Wi-Voice)

![License](https://img.shields.io/badge/license-MIT-blue.svg) ![Platform](https://img.shields.io/badge/platform-Arduino%20Nano%20%7C%20FreeRTOS-green.svg) ![Status](https://img.shields.io/badge/status-Testing-yellow.svg)

Sistema de intercomunicación **Full-Duplex** sobre Wi-Fi. El dispositivo permite realizar llamadas de voz bidireccionales en tiempo real (VoIP) y cuenta con un sistema de señalización (timbre) para alertar sobre llamadas entrantes.

## Idea General

El objetivo es crear un "Walkie-Talkie" moderno sobre red IP local. A diferencia del half-duplex tradicional (donde solo uno habla a la vez), este sistema es **Full-Duplex**: ambos usuarios hablan y escuchan simultáneamente, como una llamada telefónica. El sistema gestiona la adquisición de audio analógico, digitalización, paquetización UDP y transmisión inalámbrica, orquestado por un RTOS para cumplir requisitos temporales.

---

## Estado Actual

**Fase: Testing de componentes aislados**

Se están validando los módulos de hardware de forma independiente antes de la integración final. Los tests corren sobre **Arduino Nano (ATmega328P)** con toolchain AVR puro (sin RTOS).

| Componente | Test | Estado |
| :--- | :--- | :--- |
| Micrófono MAX9814 | [`test-devices/max9814/`](test-devices/max9814/) | En progreso |
| DAC MCP4725 + Amplificador PAM8403 | [`test-devices/mcp4725_pam8403/`](test-devices/mcp4725_pam8403/) | En progreso |

**Próxima fase:** Integración de todos los componentes sobre **Arduino Nano 33 IoT** con **FreeRTOS**, Wi-Fi y stack UDP.

---

## Requisitos Funcionales

1. **Transmisión de Audio Full-Duplex:** Envío y recepción simultánea con latencia < 200ms.
2. **Conectividad Wi-Fi:** Conexión a WLAN existente o modo AP.
3. **Señalización de Llamada:** Botón para iniciar llamada, buzzer y LED en el receptor.
4. **Gestión de Estados:** Standby, Llamando, En Llamada, Error.
5. **Calidad de Audio:** Muestreo mínimo a 8 kHz.

---

## Hardware

> **Nota sobre el MCU:** Los tests de componentes usan **Arduino Nano (ATmega328P)**. La integración final usará **Arduino Nano 33 IoT** (ARM Cortex-M0+, 48MHz, 32KB RAM, Wi-Fi integrado), ya que el ATmega328P no tiene suficiente RAM ni velocidad para manejar Wi-Fi + stack TCP/IP + audio en tiempo real con FreeRTOS.

| Componente | Descripción | Función |
| :--- | :--- | :--- |
| **Arduino Nano 33 IoT** | ARM Cortex-M0+ (48MHz), 32KB RAM, Wi-Fi NINA-W102. | MCU principal en integración final. Ejecuta FreeRTOS y gestión de red. |
| **MAX9814** | Micrófono electret con amplificador y AGC. Salida analógica 0–2.45V. | Captura audio. Conectado al ADC del Arduino. |
| **MCP4725** | DAC I2C de 12 bits (4096 niveles, 0–VCC). Dirección por defecto 0x60. | Convierte muestras digitales a señal analógica para el amplificador. |
| **PAM8403** | Amplificador estéreo Clase D, 3W+3W, alimentación 2.5–5.5V. | Amplifica la señal del DAC para mover el altavoz. |
| **Altavoz** | 4–8 Ω. | Reproduce el audio recibido y el tono de llamada. |
| **Pulsador** | Botón momentáneo normalmente abierto. | Iniciar / contestar / colgar llamada. |
| **Buzzer piezoeléctrico** | Pasivo o activo. | Tono de llamada entrante (independiente del canal de voz). |
| **LEDs indicadores** | Rojo (Standby/Error), Verde (En llamada). | Feedback visual del estado. |

---

## Estructura del Repositorio

```
poquito-smartphone/
├── readme.md
└── test-devices/
    ├── max9814/               # Test micrófono MAX9814
    │   ├── test_max9814.c
    │   ├── Makefile
    │   └── README.md
    └── mcp4725_pam8403/       # Test DAC MCP4725 + amplificador PAM8403
        ├── test_mcp4725_pam8403.c
        ├── Makefile
        └── README.md
```

---

## Compilar y Cargar un Test

Cada subdirectorio en `test-devices/` tiene su propio `Makefile`. Requisitos:

**Linux (Debian/Ubuntu)**
```bash
sudo apt-get install gcc-avr avr-libc avrdude make screen
```

**Arch Linux**
```bash
sudo pacman -S avr-gcc avr-libc avrdude make screen
```

**macOS**
```bash
brew tap osx-cross/avr && brew install avr-gcc avrdude
```

Luego, dentro del directorio del test:
```bash
make          # Compilar
make upload   # Cargar al Arduino (ajustar PORT en el Makefile)
make monitor  # Monitor serial (9600 baud)
make clean    # Limpiar archivos generados
```
