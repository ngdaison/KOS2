#ifndef KOS_LOG_H
#define KOS_LOG_H

#include <stdint.h>

void log_info(const char *message);
void log_error(const char *message);
void log_info_u64(const char *label, uint64_t value);
void log_info_u64_suffix(const char *label, uint64_t value, const char *suffix);
void log_info_hex(const char *label, uint64_t value);

#endif
