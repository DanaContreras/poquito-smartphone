/*
 * Test basico para DAC MCP4725 + Amplificador PAM8403
 *
 * Conexiones Arduino Nano -> MCP4725:
 * A4  -> SDA
 * A5  -> SCL
 * 5V  -> VCC
 * GND -> GND
 *
 * Conexiones MCP4725 -> PAM8403:
 * OUT -> L (entrada izquierda)
 * OUT -> R (entrada derecha)
 * GND -> GND (tierra comun)
 *
 * Este codigo genera diferentes formas de onda para probar el altavoz.
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define F_CPU 4000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

// Direccion I2C del MCP4725 (por defecto)
#define MCP4725_ADDR 0x60

// Comandos MCP4725
#define MCP4725_CMD_WRITEDAC 0x40    // Escribir al DAC (sin EEPROM)
#define MCP4725_CMD_WRITEDACEEPROM 0x60  // Escribir al DAC y EEPROM

// Frecuencia I2C ~100kHz con F_CPU=16MHz
#define TWI_FREQ 400000UL
#define TWBR_VALUE ((F_CPU / TWI_FREQ - 16) / 2)

// Tabla de seno (64 muestras, valores 0-4095 para 12 bits)
// Generada con: sin(2*PI*i/64) * 2047 + 2048
const uint16_t sine_table[64] PROGMEM = {
    2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
    3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
    4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
    3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
    2048, 1847, 1648, 1453, 1264, 1082,  910,  749,
     600,  465,  345,  242,  156,   88,   39,   10,
       0,   10,   39,   88,  156,  242,  345,  465,
     600,  749,  910, 1082, 1264, 1453, 1648, 1847
};

// Variables globales
volatile uint8_t wave_type = 0;  // 0=seno, 1=cuadrada, 2=triangular, 3=diente sierra
volatile uint16_t delay_us = 500;  // Delay entre muestras (controla frecuencia)

// ============== UART ==============

void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

uint8_t uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

uint8_t uart_available(void) {
    return (UCSR0A & (1 << RXC0));
}

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

void uart_print_int(uint16_t num) {
    char buffer[6];
    int i = 0;

    if (num == 0) {
        uart_transmit('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i > 0) {
        uart_transmit(buffer[--i]);
    }
}

// ============== I2C/TWI ==============

void i2c_init(void) {
    // Configurar velocidad I2C
    TWSR = 0;  // Prescaler = 1
    TWBR = (uint8_t)TWBR_VALUE;
    // Habilitar TWI
    TWCR = (1 << TWEN);
}

uint8_t i2c_start(void) {
    // Enviar condicion START
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    // Esperar a que se complete
    while (!(TWCR & (1 << TWINT)));
    // Verificar estado (0x08 = START, 0x10 = REPEATED START)
    uint8_t status = TWSR & 0xF8;
    return (status == 0x08 || status == 0x10);
}

void i2c_stop(void) {
    // Enviar condicion STOP
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    // Esperar a que se libere el bus
    while (TWCR & (1 << TWSTO));
}

uint8_t i2c_write(uint8_t data) {
    // Cargar dato
    TWDR = data;
    // Iniciar transmision
    TWCR = (1 << TWINT) | (1 << TWEN);
    // Esperar a que se complete
    while (!(TWCR & (1 << TWINT)));
    // Verificar ACK (0x18 = SLA+W ACK, 0x28 = Data ACK)
    uint8_t status = TWSR & 0xF8;
    return (status == 0x18 || status == 0x28);
}

// ============== MCP4725 ==============

uint8_t mcp4725_write(uint16_t value) {
    // Limitar a 12 bits
    if (value > 4095) value = 4095;

    if (!i2c_start()) return 0;

    // Enviar direccion + Write
    if (!i2c_write((MCP4725_ADDR << 1) | 0)) {
        i2c_stop();
        return 0;
    }

    // Enviar comando y datos (Fast Mode: 2 bytes)
    // Byte 1: [C2 C1 PD1 PD0 D11 D10 D9 D8]
    // Byte 2: [D7 D6 D5 D4 D3 D2 D1 D0]
    if (!i2c_write((value >> 8) & 0x0F)) {
        i2c_stop();
        return 0;
    }

    if (!i2c_write(value & 0xFF)) {
        i2c_stop();
        return 0;
    }

    i2c_stop();
    return 1;
}

// ============== Generadores de Onda ==============

uint16_t get_sine_value(uint8_t index) {
    return pgm_read_word(&sine_table[index & 0x3F]);
}

uint16_t get_square_value(uint8_t index) {
    // Onda cuadrada: 0 o maximo
    return (index < 32) ? 4095 : 0;
}

uint16_t get_triangle_value(uint8_t index) {
    // Onda triangular
    if (index < 32) {
        return (uint16_t)index * 128;  // Subida
    } else {
        return (uint16_t)(63 - index) * 128;  // Bajada
    }
}

uint16_t get_sawtooth_value(uint8_t index) {
    // Diente de sierra
    return (uint16_t)index * 64;
}

uint16_t get_wave_value(uint8_t wave, uint8_t index) {
    switch (wave) {
        case 0: return get_sine_value(index);
        case 1: return get_square_value(index);
        case 2: return get_triangle_value(index);
        case 3: return get_sawtooth_value(index);
        default: return 2048;  // Silencio (punto medio)
    }
}

// ============== Menu ==============

void print_menu(void) {
    uart_print("\r\n=== Menu de Control ===\r\n");
    uart_print("Tipo de onda:\r\n");
    uart_print("  1 - Senoidal\r\n");
    uart_print("  2 - Cuadrada\r\n");
    uart_print("  3 - Triangular\r\n");
    uart_print("  4 - Diente de sierra\r\n");
    uart_print("  5 - Silencio\r\n");
    uart_print("Frecuencia:\r\n");
    uart_print("  + - Aumentar frecuencia\r\n");
    uart_print("  - - Disminuir frecuencia\r\n");
    uart_print("Tests:\r\n");
    uart_print("  t - Test de barrido de frecuencias\r\n");
    uart_print("  w - Test de todas las ondas\r\n");
    uart_print("========================\r\n\r\n");
}

void print_status(void) {
    uart_print("Onda: ");
    switch (wave_type) {
        case 0: uart_print("Senoidal"); break;
        case 1: uart_print("Cuadrada"); break;
        case 2: uart_print("Triangular"); break;
        case 3: uart_print("Diente de sierra"); break;
        default: uart_print("Silencio"); break;
    }
    uart_print(" | Delay: ");
    uart_print_int(delay_us);
    uart_print("us\r\n");
}

// ============== Tests ==============

void test_frequency_sweep(void) {
    uart_print("\r\n--- Test de barrido de frecuencias ---\r\n");
    uart_print("Barriendo de grave a agudo...\r\n");

    uint8_t index = 0;
    uint16_t sweep_delay;

    // Barrido de frecuencias (de grave a agudo)
    for (sweep_delay = 2000; sweep_delay >= 100; sweep_delay -= 50) {
        // Generar algunos ciclos a esta frecuencia
        for (uint8_t cycles = 0; cycles < 20; cycles++) {
            for (index = 0; index < 64; index++) {
                mcp4725_write(get_sine_value(index));
                for (uint16_t d = 0; d < sweep_delay / 10; d++) {
                    _delay_us(10);
                }
            }
        }
    }

    // Silencio al final
    mcp4725_write(2048);
    uart_print("Barrido completado.\r\n");
}

void test_all_waves(void) {
    uart_print("\r\n--- Test de todas las ondas ---\r\n");

    const char* wave_names[] = {"Senoidal", "Cuadrada", "Triangular", "Diente de sierra"};
    uint8_t index = 0;

    for (uint8_t w = 0; w < 4; w++) {
        uart_print("Reproduciendo: ");
        uart_print(wave_names[w]);
        uart_print("...\r\n");

        // Reproducir cada onda por ~2 segundos
        for (uint16_t t = 0; t < 2000; t++) {
            mcp4725_write(get_wave_value(w, index));
            index = (index + 1) & 0x3F;
            _delay_us(500);
        }

        // Pausa entre ondas
        mcp4725_write(2048);
        _delay_ms(500);
    }

    uart_print("Test completado.\r\n");
}

// ============== Main ==============

int main(void) {
    uint8_t index = 0;
    uint8_t cmd;
    uint8_t was_silence = 0;

    // Inicializar perifericos
    uart_init();
    i2c_init();

    _delay_ms(500);

    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Test MCP4725 DAC + PAM8403 Amplifier\r\n");
    uart_print("========================================\r\n\r\n");

    // Verificar comunicacion con MCP4725
    uart_print("Verificando MCP4725... ");
    if (mcp4725_write(2048)) {
        uart_print("OK!\r\n");
    } else {
        uart_print("ERROR!\r\n");
        uart_print("Verifica las conexiones I2C.\r\n");
        uart_print("Direccion esperada: 0x");
        uart_print_int(MCP4725_ADDR);
        uart_print("\r\n");
        while (1);  // Detener si no hay DAC
    }

    print_menu();
    print_status();

    uart_print("\r\nIniciando generacion de audio...\r\n\r\n");

    while (1) {
        // Generar forma de onda
        if (wave_type < 4) {
            mcp4725_write(get_wave_value(wave_type, index));
            index = (index + 1) & 0x3F;  // Ciclar 0-63
            was_silence = 0;
        } else {
            // Silencio - punto medio
            if (!was_silence) {
                mcp4725_write(2048);
                was_silence = 1;
            }
        }

        // Delay para controlar frecuencia
        for (uint16_t d = 0; d < delay_us / 10; d++) {
            _delay_us(10);
        }

        // Verificar comandos del usuario (no bloqueante)
        if (uart_available()) {
            cmd = uart_receive();

            switch (cmd) {
                case '1':
                    wave_type = 0;
                    print_status();
                    break;
                case '2':
                    wave_type = 1;
                    print_status();
                    break;
                case '3':
                    wave_type = 2;
                    print_status();
                    break;
                case '4':
                    wave_type = 3;
                    print_status();
                    break;
                case '5':
                    wave_type = 4;
                    print_status();
                    break;
                case '+':
                case '=':
                    if (delay_us > 100) {
                        delay_us -= 100;
                    }
                    print_status();
                    break;
                case '-':
                case '_':
                    if (delay_us < 2000) {
                        delay_us += 100;
                    }
                    print_status();
                    break;
                case 't':
                case 'T':
                    test_frequency_sweep();
                    print_status();
                    break;
                case 'w':
                case 'W':
                    test_all_waves();
                    print_status();
                    break;
                case 'h':
                case 'H':
                case '?':
                    print_menu();
                    break;
            }
        }
    }

    return 0;
}
