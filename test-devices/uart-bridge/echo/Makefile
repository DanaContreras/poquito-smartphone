# Makefile - firmware del ESP para el Test A (eco por UART de hardware).
# Usa arduino-cli; reutiliza el detector de puerto del repo.
#
# Requisitos:
#   - arduino-cli instalado (ver README)
#   - make setup-esp32 - instala el core ESP32 (una sola vez)
#   - make setup-esp8266 - instala el core ESP8266 (solo para la variante ESP8266)
#
# Para flashear el ESP: conectar SU USB a la PC y desconectar los cables del
# enlace de RX/TX (GPIO3/GPIO1). Para ESP8266: make FQBN=esp8266:esp8266:nodemcuv2 upload

SKETCH := .
FQBN   ?= esp32:esp32:esp32

ESP32_LED ?= 2
ifneq (,$(findstring esp32:,$(FQBN)))
  BUILD_PROPS := --build-property "compiler.cpp.extra_flags=-DLED_BUILTIN=$(ESP32_LED)"
endif

ESP8266_URL := https://arduino.esp8266.com/stable/package_esp8266com_index.json
ESP32_URL   := https://espressif.github.io/arduino-esp32/package_esp32_index.json

_PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../..)

# Puerto serial - auto-deteccion (override con: make PORT=/dev/ttyUSBx upload)
PORT := $(shell $(_PROJECT_ROOT)/scripts/detect_port.sh)

MON_BAUD ?= 9600

# Compilar el sketch
compile:
	arduino-cli compile --fqbn $(FQBN) $(BUILD_PROPS) $(SKETCH)

# Compilar y cargar al ESP
upload: compile
	@echo "Puerto detectado: $(PORT)"
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) $(SKETCH)

# Monitor por USB. Solo util para probar el sketch aislado (en el test el ESP
# va sin PC). Ojo: el enlace usa UART0, el mismo del USB.
monitor:
	screen $(PORT) $(MON_BAUD)

# Instalar el core ESP32 (una sola vez)
setup-esp32:
	arduino-cli core update-index --additional-urls $(ESP32_URL)
	arduino-cli core install esp32:esp32 --additional-urls $(ESP32_URL)

# Instalar el core ESP8266 (solo para la variante ESP8266)
setup-esp8266:
	arduino-cli core update-index --additional-urls $(ESP8266_URL)
	arduino-cli core install esp8266:esp8266 --additional-urls $(ESP8266_URL)

.PHONY: compile upload monitor setup-esp8266 setup-esp32
