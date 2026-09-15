#ifndef KOS_SERIAL_H
#define KOS_SERIAL_H

#include <stdbool.h>

void serial_initialize(void);
bool serial_is_initialized(void);
void serial_write(const char *text);
void serial_write_char(char character);

#endif
