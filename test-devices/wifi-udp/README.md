# Test B — wifi-udp (ESP ↔ PC por Wi-Fi UDP)

Valida el **enlace de red** entre el módulo ESP32/ESP8266 y la PC por **Wi-Fi UDP**, sin
Arduino de por medio. Aísla el dominio de **red/configuración** (credenciales, IP, puertos,
firewall, stack) del dominio eléctrico ya cubierto por el [Test A (uart-bridge)](../uart-bridge/README.md).
Es la segunda etapa del enlace (Fase 3 "Wi-Fi Aislada (Tunnel)" del [README principal](../../README.md)).

El ESP se conecta a la WLAN (modo estación), imprime su IP por Serial y hace **eco UDP**: un
script Python en la PC le manda unos paquetes y cuenta cuántos ecos vuelven. Así se prueba el
camino completo **PC → ESP → PC** en un round-trip.

> ⚠️ **En este test el ESP va conectado a la PC por USB** (a diferencia del Test A, donde iba
> por cargador). Serial se usa solo para descubrir la IP; el canal bajo prueba es el **Wi-Fi**,
> no el UART. No hace falta el conversor de nivel ni ningún cableado entre placas.

## Componentes

| Componente | Descripción |
|------------|-------------|
| ESP32 (DevKit) | Módulo Wi-Fi (lógica 3.3V). Portable a ESP8266 |
| PC | En la **misma WLAN** que el ESP; corre `udp_test.py` |
| Cable USB | Alimenta el ESP y lleva el Serial (para leer la IP) |
| Router / AP | Ambos en la misma red **2.4 GHz** (el ESP8266 no soporta 5 GHz) |

## Firmwares y scripts

| Archivo | Dónde corre | Rol |
|---|---|---|
| `esp_udp_echo/esp_udp_echo.ino` | ESP (arduino-cli) | Conecta al Wi-Fi, imprime IP, ecoa cada datagrama UDP. LED onboard parpadea por paquete |
| `udp_test.py` | PC (`uv run`) | Manda 5 paquetes UDP y cuenta los ecos. Auto-descubre la IP del ESP por broadcast (o se le pasa a mano) |

## Uso

> Requisitos previos: **arduino-cli** para flashear el ESP (instalar desde
> <https://arduino.github.io/arduino-cli/latest/installation/>). El script `udp_test.py` usa
> solo la librería `socket` de stdlib (sin dependencias); se corre con `uv run` desde la raíz
> del repo, la convención del proyecto.

### 1. Configurar credenciales Wi-Fi

```bash
cd esp_udp_echo
cp secrets.h.example secrets.h   # editar secrets.h con tu SSID y clave
```

`secrets.h` está **gitignoreado**: tu clave no se versiona. Se versiona solo la plantilla
`secrets.h.example`.

### 2. Flashear el ESP

```bash
# desde test-devices/wifi-udp/esp_udp_echo/
make setup-esp32   # una sola vez: instala el core ESP32
make upload        # con el ESP conectado por USB
```

> El board genérico `esp32:esp32:esp32` no define `LED_BUILTIN`; el `Makefile` lo inyecta
> (GPIO2, el LED onboard de la mayoría de las DevKit). Si tu placa usa otro pin:
> `make ESP32_LED=<pin> upload`.

> Si el flasheo falla con `Failed to connect to ESP32: No serial data received`, mantené
> presionado el botón **BOOT** al iniciar el `upload`.

**Para ESP8266**, indicar el FQBN (una sola vez el `setup`):

```bash
make setup-esp8266                          # una sola vez: instala el core ESP8266
make FQBN=esp8266:esp8266:nodemcuv2 upload
```

### 3. Correr el test desde la PC

```bash
# desde test-devices/wifi-udp/
uv run udp_test.py                 # auto-descubre la IP del ESP por broadcast
uv run udp_test.py 192.168.x.y     # o pasando la IP a mano (ver paso 4)
```

### 4. Ver la IP del ESP (solo si el broadcast no llega)

Si tu router aísla a los clientes (*client isolation*), el broadcast no llega y hay que pasar
la IP a mano. Se lee por Serial:

```bash
cd esp_udp_echo
make monitor   # abre screen a 115200; el ESP imprime "IP: 192.168.x.y"
```

Salir de `screen`: `Ctrl-A` luego `K`, confirmar con `y`.

## Qué observar

| Señal | Comportamiento esperado | Significado |
|---|---|---|
| **Serial del ESP** | imprime `IP: ...` y `Escuchando UDP en el puerto 4210` | conectó al AP y está a la escucha |
| **`udp_test.py`** | `5/5 ecos OK -> enlace funcionando` | round-trip PC↔ESP correcto |
| **LED del ESP (onboard)** | parpadea por cada paquete | el ESP está recibiendo/ecoando datagramas |

## Solución de Problemas

**El ESP nunca imprime la IP (se queda en `Conectando ....`)**
- SSID/clave incorrectos en `secrets.h`.
- Red de **5 GHz**: el ESP8266 solo va en 2.4 GHz (el ESP32 también prefiere 2.4 GHz).
- AP fuera de alcance o MAC filtering.

**El script dice `No se encontro el ESP por broadcast`**
- **Aislamiento de clientes (client isolation)** en el AP: bloquea el broadcast entre
  dispositivos. Solución: pasar la IP a mano (`uv run udp_test.py <IP>`, ver paso 4).
- La PC en otra subred/VLAN o en otra red Wi-Fi distinta a la del ESP.

**El script da `0/5 ecos OK` (no vuelve ningún eco)**
- IP equivocada: confirmá la que el ESP imprime por Serial (paso 4).
- **Firewall del host** bloqueando UDP entrante: permitir el puerto 4210 o bajar el firewall
  temporalmente para probar.
- Client isolation también puede romper el unicast: probar cableando la PC al router.

**Ecos parciales (ej. `3/5`)**
- Wi-Fi congestionado o señal débil. En LAN estable debería ser 5/5. Volvé a correr el script.

## Referencias

- [arduino-cli — instalación](https://arduino.github.io/arduino-cli/latest/installation/)
- ESP8266 Arduino core: <https://arduino-esp8266.readthedocs.io/>
- ESP32 Arduino core: <https://docs.espressif.com/projects/arduino-esp32/>
- [Test A — uart-bridge](../uart-bridge/README.md) · [README principal](../../README.md)
