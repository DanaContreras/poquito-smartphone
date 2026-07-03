/*
 * record_max9814.c
 * Grabación de audio con MAX9814 en C bare-metal (ATmega328P).
 * 8000 Hz, 8-bit PCM unsigned. Compatible con record_audio.py.
 */

#include <avr/interrupt.h>  // ISR(), sei()
#include <util/delay.h>
#include "../../../drivers/uart.h"
#include "../../../drivers/adc.h"
//#include "../../../drivers/registers.h"

#define MIC_PIN 0

static void timer1_init_8khz(void) {
    TCCR1A = 0;                          // sin salida por pin (WGM11:10 = 00)
    TCCR1B = (1 << WGM12) | (1 << CS11); // CTC (TOP=OCR1A) + prescaler 8
    OCR1A  = 249;                        // 16MHz/8/8000 - 1 = tic cada 125 us
    TIMSK1 = (1 << OCIE1A);              // habilitar interrupcion compare-match A
}

ISR(TIMER1_COMPA_vect) {
    uint16_t lectura = adc_read(MIC_PIN);  // ~13 us con el ADC acelerado
    uart_transmit(lectura >> 2);           // ~87 us a 115200 baud  -> byte crudo
}

int main(void) {
    uart_init();
    adc_init();

    // cambiar prescaler a 16 para acelerarlo.
    ADCSRA &= ~(1 << ADPS0) & ~(1 << ADPS1);
    ADCSRA |= (1 << ADPS2);

    timer1_init_8khz();
    sei();  // habilita interrupciones globales

    while(1) {}

    return 0;
}