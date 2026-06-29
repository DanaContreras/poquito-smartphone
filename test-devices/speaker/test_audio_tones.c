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

#include <util/delay.h>
#include <avr/pgmspace.h>

#include "../../drivers/uart.h"
#include "../../drivers/twi.h"
#include "../../drivers/mcp4725.h"

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

// Notas musicales: frecuencia en Hz
#define NOTE_REST 0
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659

// Ode to Joy - Beethoven (fragmento principal)
// {nota, duracion_ms}
const uint16_t ode_to_joy[][2] PROGMEM = {
    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 400},
    {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D5, 400},
    {NOTE_C5, 400}, {NOTE_C5, 400}, {NOTE_D5, 400}, {NOTE_E5, 400},
    {NOTE_E4, 600}, {NOTE_E4, 200}, {NOTE_REST, 400},

    {NOTE_E4, 400}, {NOTE_E4, 400}, {NOTE_F4, 400}, {NOTE_G4, 400},
    {NOTE_G4, 400}, {NOTE_F4, 400}, {NOTE_E4, 400}, {NOTE_D5, 400},
    {NOTE_C5, 400}, {NOTE_C5, 400}, {NOTE_D5, 400}, {NOTE_E5, 400},
    {NOTE_D5, 600}, {NOTE_D5, 200}, {NOTE_REST, 400},

    {NOTE_D5, 400}, {NOTE_D5, 400}, {NOTE_E5, 400}, {NOTE_C5, 400},
    {NOTE_C5, 400}, {NOTE_D5, 400}, {NOTE_E4, 400}, {NOTE_F4, 400},
    {NOTE_G4, 400}, {NOTE_G4, 400}, {NOTE_E4, 400}, {NOTE_C5, 400},
    {NOTE_C5, 400}, {NOTE_D5, 400}, {NOTE_D5, 400}, {NOTE_REST, 400},
    {0, 0}
};

// Variables globales
volatile uint8_t wave_type = 0;  // 0=seno, 1=cuadrada, 2=triangular, 3=diente sierra
volatile uint16_t delay_us = 500;  // Delay entre muestras (controla frecuencia)

// ============== Generadores de Onda ==============

uint16_t get_sine_value(uint8_t index) {
    return pgm_read_word(&sine_table[index & 0x3F]);
}

uint16_t get_square_value(uint8_t index) {
    return (index < 32) ? 4095 : 0;
}

uint16_t get_triangle_value(uint8_t index) {
    if (index < 32) {
        return (uint16_t)index * 128;
    } else {
        return (uint16_t)(63 - index) * 128;
    }
}

uint16_t get_sawtooth_value(uint8_t index) {
    return (uint16_t)index * 64;
}

uint16_t get_wave_value(uint8_t wave, uint8_t index) {
    switch (wave) {
        case 0: return get_sine_value(index);
        case 1: return get_square_value(index);
        case 2: return get_triangle_value(index);
        case 3: return get_sawtooth_value(index);
        default: return 2048;
    }
}

// ============== Melodia ==============

#define I2C_WRITE_US 180

void play_note(uint16_t freq_hz, uint16_t duration_ms) {
    if (freq_hz == 0) {
        mcp4725_write(2048);
        for (uint16_t i = 0; i < duration_ms; i++) _delay_ms(1);
        return;
    }

    uint32_t period_us = 1000000UL / freq_hz;

    uint8_t samples = 32;
    while (samples > 4 && (period_us / samples) < (uint32_t)(I2C_WRITE_US + 20)) {
        samples /= 2;
    }

    uint16_t sample_time = (uint16_t)(period_us / samples);
    uint16_t extra_delay = 0;
    if (sample_time > I2C_WRITE_US) {
        extra_delay = sample_time - I2C_WRITE_US;
    }

    uint16_t num_cycles = (uint32_t)duration_ms * 1000UL / period_us;

    for (uint16_t c = 0; c < num_cycles; c++) {
        for (uint8_t s = 0; s < samples; s++) {
            uint8_t idx = (uint16_t)s * 64 / samples;
            mcp4725_write(pgm_read_word(&sine_table[idx & 0x3F]));
            for (uint16_t d = 0; d < extra_delay; d++) {
                _delay_us(1);
            }
        }
    }

    mcp4725_write(2048);
}

void play_ode_to_joy(void) {
    uart_print("\r\n--- Reproduciendo: Oda a la Alegria (Beethoven) ---\r\n");

    for (uint8_t i = 0; ; i++) {
        uint16_t note = pgm_read_word(&ode_to_joy[i][0]);
        uint16_t dur  = pgm_read_word(&ode_to_joy[i][1]);
        if (note == 0 && dur == 0) break;
        play_note(note, dur);
        _delay_ms(30);
    }

    uart_print("Melodia completada.\r\n");
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
    uart_print("  m - Reproducir melodia (Oda a la Alegria)\r\n");
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

    for (sweep_delay = 2000; sweep_delay >= 100; sweep_delay -= 50) {
        for (uint8_t cycles = 0; cycles < 20; cycles++) {
            for (index = 0; index < 64; index++) {
                mcp4725_write(get_sine_value(index));
                for (uint16_t d = 0; d < sweep_delay / 10; d++) {
                    _delay_us(10);
                }
            }
        }
    }

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

        for (uint16_t t = 0; t < 2000; t++) {
            mcp4725_write(get_wave_value(w, index));
            index = (index + 1) & 0x3F;
            _delay_us(500);
        }

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

    uart_init();
    twi_init();

    _delay_ms(500);

    uart_print("\r\n");
    uart_print("========================================\r\n");
    uart_print("  Test MCP4725 DAC + PAM8403 Amplifier\r\n");
    uart_print("========================================\r\n\r\n");

    uart_print("Verificando MCP4725... ");
    if (mcp4725_write(2048)) {
        uart_print("OK!\r\n");
    } else {
        uart_print("ERROR!\r\n");
        uart_print("Verifica las conexiones I2C.\r\n");
        uart_print("Direccion esperada: 0x");
        uart_print_int(MCP4725_ADDR);
        uart_print("\r\n");
        while (1);
    }

    print_menu();
    print_status();

    uart_print("\r\nIniciando generacion de audio...\r\n\r\n");

    while (1) {
        if (wave_type < 4) {
            mcp4725_write(get_wave_value(wave_type, index));
            index = (index + 1) & 0x3F;
            was_silence = 0;
        } else {
            if (!was_silence) {
                mcp4725_write(2048);
                was_silence = 1;
            }
        }

        for (uint16_t d = 0; d < delay_us / 10; d++) {
            _delay_us(10);
        }

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
                case 'm':
                case 'M':
                    play_ode_to_joy();
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
