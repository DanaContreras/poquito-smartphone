/*
 * Test basico para modulo MAX9814
 *
 * Conexiones:
 * Arduino A0  -> MAX9814 OUT
 * Arduino 5V  -> MAX9814 VDD
 * Arduino GND -> MAX9814 GND
 *
 * Este codigo lee continuamente el valor analogico del microfono
 * y muestra los valores en el monitor serial.
 */

#include <util/delay.h>

#include "../../drivers/uart.h"
#include "../../drivers/adc.h"

#define MIC_PIN 0

int main(void) {
    uint16_t raw_value;
    uint16_t signal_max;
    uint16_t signal_min;
    uint16_t peak_to_peak;
    uint8_t i;

    uart_init();
    adc_init();

    _delay_ms(1000);

    uart_print("=== Test MAX9814 Microphone Module ===\r\n");
    uart_print("Iniciando lectura del microfono...\r\n\r\n");

    while (1) {
        raw_value = adc_read(MIC_PIN);

        signal_max = 0;
        signal_min = 1023;

        for (i = 0; i < 50; i++) {
            uint16_t sample = adc_read(MIC_PIN);

            if (sample > signal_max) {
                signal_max = sample;
            }
            if (sample < signal_min) {
                signal_min = sample;
            }

            _delay_ms(1);
        }

        peak_to_peak = signal_max - signal_min;

        uart_print("Raw: ");
        uart_print_int(raw_value);
        uart_print(" | Min: ");
        uart_print_int(signal_min);
        uart_print(" | Max: ");
        uart_print_int(signal_max);
        uart_print(" | Peak-to-Peak: ");
        uart_print_int(peak_to_peak);
        uart_print("\r\n");

        _delay_ms(100);
    }

    return 0;
}
