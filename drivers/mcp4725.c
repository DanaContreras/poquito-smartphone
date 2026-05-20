
#include "mcp4725.h"
#include "i2c.h"

uint8_t mcp4725_write(uint16_t value) {
    if (value > 4095) value = 4095;

    if (!i2c_start()) return 0;

    if (!i2c_write((MCP4725_ADDR << 1) | 0)) {
        i2c_stop();
        return 0;
    }

    if (!i2c_write((value >> 8) & 0x0F)) {
        i2c_stop();
        return 0;
    }

    if (!i2c_write(value & 0xFF)) {
        i2c_stop();
        return 0;
    }

    i2c_stop();
    return 1;
}
