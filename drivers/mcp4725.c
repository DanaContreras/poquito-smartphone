
#include "mcp4725.h"
#include "twi.h"

uint8_t mcp4725_write(uint16_t value) {
    if (value > 4095) value = 4095;

    uint8_t data[2] = {
        (uint8_t)((value >> 8) & 0x0F),
        (uint8_t)(value & 0xFF),
    };

    twi_write(MCP4725_ADDR, data, 2, 0);
    twi_wait();

    return 1;
}
