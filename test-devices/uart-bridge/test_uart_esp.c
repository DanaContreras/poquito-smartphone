/*
 * ###########################################################
 * #### Test A (uart-bridge) — lado Arduino (ATmega328P) ####
 * ###########################################################
 *
 * Valida el enlace UART fisico entre el Arduino y el modulo ESP32/ESP8266 a
 * traves del conversor de nivel logico. El Arduino transmite un byte que se
 * incrementa, espera el eco que devuelve el ESP y compara:
 *
 *  - eco correcto          -> parpadeo "latido" del LED onboard (D13)
 *  - eco erroneo / timeout -> rafaga rapida de parpadeos (error)
 */

#include <util/delay.h>

#include "../../drivers/uart.h"
#include "../../drivers/registers.h"

// LED onboard (D13 = PB5 en UNO/Nano)
#define LED_BIT  PORTB5

// Espera un byte por UART hasta ~1 s. Devuelve 1 si llego, 0 si hubo timeout.
static uint8_t uart_receive_timeout(uint8_t *out) {
    for (uint16_t i = 0; i < 10000; i++) {
        if (uart_available()) {
            *out = uart_receive();
            return 1;
        }
        _delay_us(100);
    }
    return 0;
}

int main(void) {
    uint8_t tx = 0;
    uint8_t rx = 0;

    uart_init();
    DDRB |= (1 << LED_BIT);   // LED como salida

    _delay_ms(1000);

    while (1) {
        uart_transmit(tx);

        if (uart_receive_timeout(&rx) && rx == tx) {
            // Eco OK: latido invirtiendo el estado del LED
            PORTB ^= (1 << LED_BIT);
        } else {
            // Error: rafaga rapida de parpadeos
            for (uint8_t i = 0; i < 6; i++) {
                PORTB ^= (1 << LED_BIT);
                _delay_ms(80);
            }
        }

        tx++;
        _delay_ms(200);
    }

    return 0;
}
