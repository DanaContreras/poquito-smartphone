#include "uart.h"
#include "registers.h"

void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;
    UCSR0A = (1 << U2X0);
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
