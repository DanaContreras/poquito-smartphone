#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#ifndef F_CPU
#error "F_CPU must be defined via compiler flag"
#endif

void i2c_init(void);
uint8_t i2c_start(void);
void i2c_stop(void);
uint8_t i2c_write(uint8_t data);

#endif
