#include <kos/console.h>
#include <kos/log.h>
#include <kos/serial.h>

static void log_write(const char *prefix, const char *message) {
    serial_write(prefix);
    serial_write(message);
    serial_write("\n");

    if (console_is_initialized()) {
        console_write(prefix);
        console_write(message);
        console_write("\n");
    }
}

static void log_write_character(char character) {
    serial_write_char(character);
    if (console_is_initialized()) {
        console_write_char(character);
    }
}

static void log_write_label(const char *label) {
    while (*label != '\0') {
        log_write_character(*label);
        ++label;
    }
}

void log_info(const char *message) {
    log_write("", message);
}

void log_error(const char *message) {
    log_write("ERROR: ", message);
}

void log_info_u64(const char *label, uint64_t value) {
    char digits[20];
    uint64_t count = 0;

    log_write_label(label);
    if (value == 0) {
        log_write_character('0');
    }
    else {
        while (value != 0) {
            digits[count] = (char)('0' + (value % 10));
            value /= 10;
            ++count;
        }
        while (count > 0) {
            --count;
            log_write_character(digits[count]);
        }
    }
    log_write_character('\n');
}

void log_info_u64_suffix(const char *label, uint64_t value, const char *suffix) {
    char digits[20];
    uint64_t count = 0;

    log_write_label(label);
    if (value == 0) {
        log_write_character('0');
    }
    else {
        while (value != 0) {
            digits[count] = (char)('0' + (value % 10));
            value /= 10;
            ++count;
        }
        while (count > 0) {
            --count;
            log_write_character(digits[count]);
        }
    }
    log_write_label(suffix);
    log_write_character('\n');
}

void log_info_hex(const char *label, uint64_t value) {
    static const char hex_digits[] = "0123456789ABCDEF";

    log_write_label(label);
    log_write_label("0x");
    for (uint64_t shift = 60;; shift -= 4) {
        log_write_character(hex_digits[(value >> shift) & 0x0f]);
        if (shift == 0) {
            break;
        }
    }
    log_write_character('\n');
}
