# Wi-Fi Full-Duplex Voice Communicator (Wi-Voice)

![License](https://img.shields.io/badge/license-MIT-blue.svg) ![Platform](https://img.shields.io/badge/platform-Arduino%20Nano%20%7C%20ESP32%20%7C%20FreeRTOS-green.svg) ![Status](https://img.shields.io/badge/status-Testing-yellow.svg)

Sistema de intercomunicación **Full-Duplex** sobre Wi-Fi. El dispositivo permite realizar llamadas de voz bidireccionales en tiempo real (VoIP) y cuenta con un sistema de señalización (timbre) para alertar sobre llamadas entrantes.

## Idea General

El objetivo es crear un "Walkie-Talkie" moderno sobre red IP local. A diferencia del half-duplex tradicional (donde solo uno habla a la vez), este sistema es **Full-Duplex**: ambos usuarios hablan y escuchan simultáneamente, como una llamada telefónica. El sistema gestiona la adquisición de audio analógico, digitalización, paquetización UDP y transmisión inalámbrica, orquestado por un RTOS para cumplir requisitos temporales.

---

## Estado Actual

**Fase: Testing de componentes aislados**

Se están validando los módulos de hardware de forma independiente. Los tests corren sobre **Arduino Nano (ATmega328P)** con toolchain AVR, usando acceso directo a los registros del microcontrolador por dirección de memoria (sin `<avr/io.h>`).

| Componente | Test | Estado |
| :--- | :--- | :--- |
| Micrófono MAX9814 | [`test-devices/max9814/`](test-devices/max9814/) | En progreso |
| DAC MCP4725 + Amplificador PAM8403 | [`test-devices/mcp4725_pam8403/`](test-devices/mcp4725_pam8403/) | En progreso |

---

## Roadmap de Desarrollo

| Fase | Checkpoints Críticos | Criterio de Exito | Tiempo Est. | Fecha Finalización |
| :--- | :--- | :--- | :--- | :--- |
| **1. Validación Mic (MAX9814)** | • Captura de audio funcional (ADC)<br>• Verificación de niveles básicos | Audio capturado visible en osciloscopio o volcado serie consistente. | ~4 Horas | [~19/04/26] |
| **2. Validación DAC (MCP4725)** | • Reproducción vía I2C<br>• Amplificación PAM8403 sin ruido | Tono generado por software audible y claro en el altavoz. | ~4 Horas | [~19/04/26] |
| **3. Wi-Fi Aislada (Tunnel)** | • Arduino Nano ↔ ESP32 (Serial/SPI)<br>• ESP32 ↔ PC vía Wi-Fi (UDP) | Envío de datos desde Arduino recibido íntegramente en la PC. | ~10 Horas | [~19/04/26] |
| **4. FreeRTOS (ATmega328P)** | • Setup FreeRTOS en 2KB RAM<br>• Gestión de tareas: Audio y Comms | Sistema multihilo estable sin desbordamientos de stack. | ~12 Horas | [-] |
| **5. Interop Desktop (Rust)** | • Software de PC en Rust para audio UDP<br>• Comunicación Nano ↔ PC | El dispositivo habla con la PC, evitando duplicar hardware inicialmente. | ~15 Horas | [-] |
| **6. Planificación de Alcance** | • Evaluación de Multicast/Broadcasting<br>• Definición de escalabilidad de red | Documento técnico de arquitectura final para múltiples nodos. | ~6 Horas | [-] |
| **7. Pipeline Audio & FSM** | • Streaming UDP 8kHz (Full-Duplex)<br>• Lógica de llamadas y señalización | Llamada completa funcional entre dos puntos (Nano y PC/Nano). | ~16 Horas | [~12/05/26] |
| **8. Prototipado Físico** | • Soldadura final (PCB/Perfboard)<br>• Modelado 3D e Impresión de carcasa | Dispositivo compacto, sin protoboard y ensamblado. | ~25 Horas | [-] |
| **9. Maestría del Sistema** | • Fine-tuning: AGC y Clipping<br>• Optimización extrema de latencia/RAM | Audio de alta fidelidad y estabilidad máxima bajo carga. | ~10 Horas | [-] |
| **10. Vitrina del Proyecto (Web)** | • Diseño web estilo "Arduino Project"<br>• Documentación visual y técnica | Sitio web funcional para presentar el proyecto al público. | ~12 Horas | [-] |

---

## Requisitos Funcionales

1. **Transmisión de Audio Full-Duplex:** Envío y recepción simultánea con latencia < 200ms.
2. **Conectividad Wi-Fi:** Conexión a WLAN mediante módulo ESP32.
3. **Señalización de Llamada:** Botón para iniciar llamada, buzzer y LED en el receptor.
4. **Gestión de Estados:** Standby, Llamando, En Llamada, Error.
5. **Calidad de Audio:** Muestreo mínimo a 8 kHz.

---

## Hardware

| Componente | Descripción | Función |
| :--- | :--- | :--- |
| **Arduino Nano** | ATmega328P (16MHz, 2KB RAM). | MCU principal. Gestiona ADC, DAC y FreeRTOS. |
| **ESP32** | SoC con Wi-Fi/BT integrado. | Módulo de comunicaciones Wi-Fi (Puente Serial-to-WiFi). |
| **MAX9814** | Micrófono con amplificador y AGC. | Captura audio. Conectado al ADC del Arduino. |
| **MCP4725** | DAC I2C de 12 bits. | Convierte audio digital a analógico. |
| **PAM8403** | Amplificador Clase D. | Maneja el altavoz. |
| **Altavoz** | 4–8 Ω. | Reproduce voz y timbres. |
| **Buzzer** | Piezoeléctrico. | Alerta de llamada entrante. |

---

## Estructura del Repositorio

```
poquito-smartphone/
├── readme.md
├── drivers/                   # Drivers reutilizables para perifericos AVR
│   ├── registers.h            # Definiciones de registros del ATmega328P
│   ├── uart.h / uart.c        # Comunicacion serial UART
│   ├── i2c.h / i2c.c          # Bus I2C/TWI
│   ├── mcp4725.h / mcp4725.c  # DAC MCP4725 (via I2C)
│   └── adc.h / adc.c          # Conversor analogico-digital
├── scripts/
│   └── detect_port.sh         # Auto-deteccion del puerto USB del Arduino
└── test-devices/
    ├── max9814/               # Test microfono MAX9814
    │   ├── test_max9814.c
    │   ├── Makefile
    │   └── README.md
    └── mcp4725_pam8403/       # Test DAC MCP4725 + amplificador PAM8403
        ├── test_mcp4725_pam8403.c
        ├── i2c_scanner.c      # Escaner de dispositivos I2C
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
make upload   # Cargar al Arduino (puerto detectado automaticamente)
make monitor  # Monitor serial (9600 baud)
make clean    # Limpiar archivos generados
```

### Auto-detección de Puerto USB

El puerto serial se detecta automáticamente al hacer `make upload` mediante `scripts/detect_port.sh`. El script busca dispositivos `/dev/ttyACM*` y `/dev/ttyUSB*`, priorizando chips conocidos (CH340, FTDI, Arduino oficial).

Para forzar un puerto manualmente:
```bash
make PORT=/dev/ttyUSB1 upload
```

### Monitor Serial

`make monitor` abre una sesión de `screen`. Para cerrarla correctamente:

| Atajo | Acción |
|-------|--------|
| `Ctrl+A` luego `k` | Cerrar sesión (confirmar con `y`) |
| `Ctrl+A` luego `d` | Detach (reconectar con `screen -r`) |

> **No uses `Ctrl+C`** — no cierra `screen`, lo deja como proceso zombie ocupando el puerto serial.
