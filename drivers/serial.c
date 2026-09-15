#include <stdbool.h>
#include <stdint.h>

#include <kos/io.h>
#include <kos/serial.h>

enum {
    COM1 = 0x3f8,
    COM_DATA = 0,
    COM_INTERRUPT_ENABLE = 1,
    COM_FIFO_CONTROL = 2,
    COM_LINE_CONTROL = 3,
    COM_MODEM_CONTROL = 4,
    COM_LINE_STATUS = 5,
    COM_TRANSMITTER_EMPTY = 0x20,
    SERIAL_TRANSMIT_SPIN_LIMIT = 1000000,
};

static bool serial_initialized;

void serial_initialize(void) {
    io_out8(COM1 + COM_INTERRUPT_ENABLE, 0x00);
    io_out8(COM1 + COM_LINE_CONTROL, 0x80);
    io_out8(COM1 + COM_DATA, 0x01);
    io_out8(COM1 + COM_INTERRUPT_ENABLE, 0x00);
    io_out8(COM1 + COM_LINE_CONTROL, 0x03);
    io_out8(COM1 + COM_FIFO_CONTROL, 0xc7);
    io_out8(COM1 + COM_MODEM_CONTROL, 0x0b);
    serial_initialized = true;
}

bool serial_is_initialized(void) {
    return serial_initialized;
}

static bool serial_wait_for_transmitter(void) {
    for (uint64_t spin = 0; spin < SERIAL_TRANSMIT_SPIN_LIMIT; ++spin) {
        if ((io_in8(COM1 + COM_LINE_STATUS) & COM_TRANSMITTER_EMPTY) != 0) {
            return true;
        }
    }
    return false;
}

void serial_write_char(char character) {
    if (!serial_initialized) {
        return;
    }
    if (character == '\n') {
        serial_write_char('\r');
    }

    if (!serial_wait_for_transmitter()) {
        return;
    }
    io_out8(COM1 + COM_DATA, (uint8_t)character);
}

void serial_write(const char *text) {
    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        serial_write_char(*text);
        ++text;
    }
}
