/*
 * Test básico para módulo MAX9814
 *
 * Conexiones:
 * Arduino A0  -> MAX9814 OUT
 * Arduino 5V  -> MAX9814 VDD
 * Arduino GND -> MAX9814 GND
 *
 * Este código lee continuamente el valor analógico del micrófono
 * y muestra los valores en el monitor serial.
 */

#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 16000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

#define MIC_PIN 0  // ADC0 (pin A0 en Arduino)

// Inicializar UART
void uart_init(void) {
    // Configurar baud rate
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;

    // Habilitar receptor y transmisor
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // Formato: 8 bits de datos, 1 bit de stop
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Transmitir un byte
void uart_transmit(uint8_t data) {
    // Esperar a que el buffer de transmisión esté vacío
    while (!(UCSR0A & (1 << UDRE0)));

    // Poner dato en el buffer y enviar
    UDR0 = data;
}

// Transmitir una cadena
void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

// Transmitir un número entero
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

// Inicializar ADC
void adc_init(void) {
    // Referencia de voltaje AVcc, pin ADC0
    ADMUX = (1 << REFS0);

    // Habilitar ADC, prescaler = 128 (16MHz/128 = 125kHz)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Leer valor del ADC
uint16_t adc_read(uint8_t channel) {
    // Seleccionar canal (mantener REFS0)
    ADMUX = (1 << REFS0) | (channel & 0x07);

    // Iniciar conversión
    ADCSRA |= (1 << ADSC);

    // Esperar a que termine la conversión
    while (ADCSRA & (1 << ADSC));

    // Retornar valor ADC (10 bits)
    return ADC;
}

int main(void) {
    uint16_t raw_value;
    uint16_t signal_max;
    uint16_t signal_min;
    uint16_t peak_to_peak;
    uint8_t i;

    // Inicializar periféricos
    uart_init();
    adc_init();

    _delay_ms(1000);

    uart_print("=== Test MAX9814 Microphone Module ===\r\n");
    uart_print("Iniciando lectura del microfono...\r\n\r\n");

    while (1) {
        // Lectura simple
        raw_value = adc_read(MIC_PIN);

        // Muestreo para detectar picos (50 muestras)
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

        // Mostrar resultados
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
