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
#define U2X0    1
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

/* Timer1 (16-bit) - 0x80 */

#define TCCR1A  REG8(0x80)
#define TCCR1B  REG8(0x81)
#define TCCR1C  REG8(0x82)
#define TCNT1   REG16(0x84)
#define ICR1    REG16(0x86)
#define OCR1A   REG16(0x88)
#define OCR1B   REG16(0x8A)

#define TIMSK1  REG8(0x6F)   /* mascara de interrupciones del timer 1 */
#define TIFR1   REG8(0x36)   /* banderas del timer 1 (polling)   */

/* bits de TCCR1A (salida PWM; no se usan en CTC del micro) */
#define WGM10   0
#define WGM11   1
#define COM1B1  5
#define COM1A1  7

/* bits de TCCR1B */
#define WGM12   3
#define WGM13   4
#define CS10    0
#define CS11    1
#define CS12    2

/* bits de TIMSK1 / TIFR1 */
#define TOIE1   0   /* overflow interrupt enable */
#define OCIE1A  1   /* compare-match A interrupt enable */
#define OCF1A   1   /* bandera compare-match A (polling) */

#endif
