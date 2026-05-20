#ifndef MCP4725_H
#define MCP4725_H

#include <stdint.h>

#define MCP4725_ADDR 0x60

uint8_t mcp4725_write(uint16_t value);

#endif
