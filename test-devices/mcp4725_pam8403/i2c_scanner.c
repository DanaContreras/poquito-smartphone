/*
 * I2C Scanner - Detecta dispositivos en el bus I2C
 * Version con log detallado y timeout
 */

#include <avr/io.h>
#include <util/delay.h>

#define F_CPU 4000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)

#define TIMEOUT_MAX 10000  // Timeout para operaciones I2C

// UART
void uart_init(void) {
    UBRR0H = (uint8_t)(UBRR_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR_VALUE;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_transmit(uint8_t data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
}

void uart_print(const char* str) {
    while (*str) uart_transmit(*str++);
}

void uart_print_hex(uint8_t val) {
    const char hex[] = "0123456789ABCDEF";
    uart_transmit('0');
    uart_transmit('x');
    uart_transmit(hex[(val >> 4) & 0x0F]);
    uart_transmit(hex[val & 0x0F]);
}

// I2C con timeout
void i2c_init(void) {
    // Activar resistencias pull-up internas para SDA (A4/PC4) y SCL (A5/PC5)
    PORTC |= (1 << PORTC4) | (1 << PORTC5);
    
    TWSR = 0;
    TWBR = 12;  // 100kHz con F_CPU=4MHz
    TWCR = (1 << TWEN);
}

// Retorna: 1=OK, 0=NACK, 2=TIMEOUT
uint8_t i2c_start_timeout(void) {
    uint16_t timeout = 0;

    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT))) {
        timeout++;
        if (timeout > TIMEOUT_MAX) {
            return 2;  // TIMEOUT
        }
    }

    uint8_t status = TWSR & 0xF8;
    return (status == 0x08 || status == 0x10) ? 1 : 0;
}

// Retorna: 1=ACK, 0=NACK, 2=TIMEOUT
uint8_t i2c_write_timeout(uint8_t data) {
    uint16_t timeout = 0;

    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    while (!(TWCR & (1 << TWINT))) {
        timeout++;
        if (timeout > TIMEOUT_MAX) {
            return 2;  // TIMEOUT
        }
    }

    uint8_t status = TWSR & 0xF8;
    return (status == 0x18 || status == 0x28) ? 1 : 0;
}

void i2c_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    uint16_t timeout = 0;
    while ((TWCR & (1 << TWSTO)) && timeout < TIMEOUT_MAX) {
        timeout++;
    }
}

int main(void) {
    uint8_t addr;
    uint8_t found = 0;
    uint8_t result;

    uart_init();
    i2c_init();

    _delay_ms(500);

    uart_print("\r\n=== I2C Scanner (con log) ===\r\n");
    uart_print("Escaneando direcciones 0x01-0x7F...\r\n\r\n");

    for (addr = 1; addr < 128; addr++) {
        // Mostrar que direccion estamos probando
        uart_print("Probando ");
        uart_print_hex(addr);
        uart_print(": ");

        result = i2c_start_timeout();

        if (result == 2) {
            uart_print("TIMEOUT en START\r\n");
            // Reset del bus I2C
            TWCR = 0;
            _delay_ms(10);
            i2c_init();
            continue;
        }

        if (result == 0) {
            uart_print("START fallo\r\n");
            i2c_stop();
            continue;
        }

        // START ok, intentar escribir direccion
        result = i2c_write_timeout(addr << 1);

        if (result == 2) {
            uart_print("TIMEOUT en WRITE\r\n");
            TWCR = 0;
            _delay_ms(10);
            i2c_init();
            continue;
        }

        if (result == 1) {
            uart_print("ENCONTRADO!\r\n");
            found++;
        } else {
            uart_print("no responde\r\n");
        }

        i2c_stop();
        _delay_ms(5);
    }

    uart_print("\r\n=== Escaneo completo ===\r\n");
    uart_print("Dispositivos encontrados: ");
    uart_transmit('0' + found);
    uart_print("\r\n");

    while (1);
    return 0;
}
