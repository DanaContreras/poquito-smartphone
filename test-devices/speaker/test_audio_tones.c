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
 * Reproduce una melodia (Oda a la Alegria) generando cada nota como una onda
 * cuadrada sobre el DAC. Sirve para probar que la cadena I2C -> MCP4725 ->
 * PAM8403 -> parlante funciona, como paso previo al streaming de audio real.
 */

#include <util/delay.h>
#include <avr/pgmspace.h>

#include "../../drivers/uart.h"
#include "../../drivers/twi.h"
#include "../../drivers/mcp4725.h"

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

// Compensacion del tiempo de una escritura I2C al DAC, en us. El Makefile
// compila a 400 kHz, donde una escritura ronda los ~80 us. Es solo un knob de
// afinacion aproximada: subilo/bajalo si en el hardware real la melodia desafina.
#define I2C_WRITE_US 80

// Genera una nota como onda cuadrada: alterna el DAC entre sus dos extremos
// (0 y 4095) cada medio periodo. Una sola escritura I2C por medio periodo.
void play_note(uint16_t freq_hz, uint16_t duration_ms) {
    if (freq_hz == 0) {                       // silencio
        mcp4725_write(2048);
        for (uint16_t i = 0; i < duration_ms; i++) _delay_ms(1);
        return;
    }

    uint16_t half_us = 500000UL / freq_hz;    // medio periodo de la onda
    uint16_t gap = (half_us > I2C_WRITE_US) ? half_us - I2C_WRITE_US : 0;
    uint32_t toggles = (uint32_t)duration_ms * 1000UL / half_us;

    for (uint32_t t = 0; t < toggles; t++) {
        mcp4725_write((t & 1) ? 0 : 4095);    // alterna entre los dos extremos
        for (uint16_t d = 0; d < gap; d++) _delay_us(1);
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

// ============== Main ==============

int main(void) {
    uart_init();
    twi_init();

    _delay_ms(500);

    uart_print("\r\n=== Test MCP4725 + PAM8403 (melodia) ===\r\n");
    uart_print("Verificando MCP4725.");
    if (mcp4725_write(2048)) {
        uart_print("Escritura de prueba completada.\r\n");
    } else {
        uart_print("ERROR: revisa las conexiones I2C.\r\n");
        while (1);
    }

    while (1) {
        play_ode_to_joy();
        _delay_ms(2000);
    }

    return 0;
}
