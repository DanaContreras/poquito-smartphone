/*
 * registers.h
 *
 * Registros y bits del ATmega328P utilizados en el proyecto.
 * Acceso directo por direcciones de memoria.
 */

#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#define REG8(addr)  (*((volatile uint8_t *)(addr)))
#define REG16(addr) (*((volatile uint16_t *)(addr)))

/* USART0 - 0xC0 */

#define UCSR0A  REG8(0xC0)
#define UCSR0B  REG8(0xC1)
#define UCSR0C  REG8(0xC2)
#define UBRR0L  REG8(0xC4)
#define UBRR0H  REG8(0xC5)
#define UDR0    REG8(0xC6)

#define RXC0    7
#define UDRE0   5
#define RXEN0   4
#define TXEN0   3
#define UCSZ01  2
#define UCSZ00  1

/* TWI (I2C) - 0xB8 */

#define TWBR    REG8(0xB8)
#define TWSR    REG8(0xB9)
#define TWDR    REG8(0xBB)
#define TWCR    REG8(0xBC)

#define TWINT   7
#define TWSTA   5
#define TWSTO   4
#define TWEN    2

/* GPIO - PORTC - 0x26 */

#define PORTC   REG8(0x26)

#define PORTC4  4
#define PORTC5  5

/* ADC - 0x78 */

#define ADC     REG16(0x78)
#define ADCSRA  REG8(0x7A)
#define ADMUX   REG8(0x7C)

#define REFS0   6
#define ADEN    7
#define ADSC    6
#define ADPS2   2
#define ADPS1   1
#define ADPS0   0

#endif
