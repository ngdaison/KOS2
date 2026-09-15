#ifndef KOS_TIMER_H
#define KOS_TIMER_H

#include <stdbool.h>
#include <stdint.h>

bool timer_initialize(uint32_t frequency_hz);
bool timer_is_initialized(void);
void timer_interrupt(void);
uint64_t timer_ticks(void);
uint64_t timer_uptime_seconds(void);
uint32_t timer_frequency_hz(void);
void timer_wait_ticks(uint64_t ticks);

#endif
