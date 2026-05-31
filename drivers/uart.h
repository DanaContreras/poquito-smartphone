#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifndef BAUD
#define BAUD 9600
#endif

#ifndef F_CPU
#error "F_CPU must be defined via compiler flag"
#endif

/* Modo double-speed (U2X): divisor 8 en vez de 16. Reduce el error de baud
 * a 16 MHz / 115200 de +8.5% (UBRR truncado) a +2.1%. */
#define UBRR_VALUE ((F_CPU / (8UL * BAUD)) - 1)

void uart_init(void);
void uart_transmit(uint8_t data);
uint8_t uart_receive(void);
uint8_t uart_available(void);
void uart_print(const char* str);
void uart_print_int(uint16_t num);

#endif
