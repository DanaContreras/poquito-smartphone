/*
 * Streaming de audio por UART -> MCP4725 -> PAM8403
 *
 * El PC envia muestras PCM crudas (8 bits unsigned, mono) por UART.
 * El firmware las lee y las escribe al DAC al mismo ritmo.
 * El sample rate efectivo es BAUD/10 (a 115200 baud => 11520 Hz).
 *
 * Protocolo:
 *   PC -> MCU: 0xAA                (sync request)
 *   MCU -> PC: 0x55                (sync ack)
 *   PC -> MCU: 4 bytes length LE   (cantidad de muestras a enviar)
 *   PC -> MCU: N bytes PCM         (stream continuo, ritmo dado por el baud)
 *   MCU -> PC: 0x44                (done)
 *
 * Conexiones: identicas al test_audio_tones (I2C en A4/A5).
 */

#include <util/delay.h>

#include "../../drivers/uart.h"
#include "../../drivers/twi.h"
#include "../../drivers/mcp4725.h"

#define SYNC_REQ    0xAA
#define SYNC_ACK    0x55
#define DONE_ACK    0x44
#define DAC_MIDRAIL 2048

int main(void) {
    uart_init();
    twi_init();
    _delay_ms(200);

    mcp4725_write(DAC_MIDRAIL);
    uart_print("AUDIO_STREAM_READY\r\n");

    while (1) {
        if (uart_receive() != SYNC_REQ) continue;
        uart_transmit(SYNC_ACK);

        uint32_t length = 0;
        for (uint8_t i = 0; i < 4; i++) {
            length |= ((uint32_t)uart_receive()) << (i * 8);
        }

        while (length--) {
            uint8_t sample = uart_receive();
            mcp4725_write((uint16_t)sample << 4);
        }

        mcp4725_write(DAC_MIDRAIL);
        uart_transmit(DONE_ACK);
    }

    return 0;
}
