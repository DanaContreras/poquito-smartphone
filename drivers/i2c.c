#include "i2c.h"
#include "registers.h"

void i2c_init(void) {
    TWSR = 0;
    TWBR = 0;
    TWCR = (1 << TWEN);
}

uint8_t i2c_start(void) {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    uint8_t status = TWSR & 0xF8;
    return (status == 0x08 || status == 0x10);
}

void i2c_stop(void) {
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    while (TWCR & (1 << TWSTO));
}

uint8_t i2c_write(uint8_t data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
    uint8_t status = TWSR & 0xF8;
    return (status == 0x18 || status == 0x28);
}
