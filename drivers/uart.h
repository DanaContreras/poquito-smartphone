#ifndef UART_H
#define UART_H

#include <stdint.h>

#ifndef BAUD
#define BAUD 9600
#endif

#ifndef F_CPU
#error "F_CPU must be defined via compiler flag"
#endif

#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

void uart_init(void);
void uart_transmit(uint8_t data);
uint8_t uart_receive(void);
uint8_t uart_available(void);
void uart_print(const char* str);
void uart_print_int(uint16_t num);

#endif
