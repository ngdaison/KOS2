#include <stdbool.h>
#include <stdint.h>

#include <kos/cpu.h>
#include <kos/io.h>
#include <kos/timer.h>

enum {
    PIT_CHANNEL_0_DATA = 0x40,
    PIT_COMMAND = 0x43,
    PIT_INPUT_FREQUENCY_HZ = 1193182,
    PIT_COMMAND_CHANNEL_0_LOHI_MODE_3 = 0x36,
    PIT_MIN_DIVISOR = 1,
    PIT_MAX_DIVISOR = 65536,
};

static volatile uint64_t ticks;
static uint32_t frequency;
static bool initialized;

bool timer_initialize(uint32_t frequency_hz) {
    if (frequency_hz == 0) {
        return false;
    }
    uint64_t divisor = (PIT_INPUT_FREQUENCY_HZ + frequency_hz / 2) / frequency_hz;
    if (divisor < PIT_MIN_DIVISOR || divisor > PIT_MAX_DIVISOR) {
        return false;
    }

    io_out8(PIT_COMMAND, PIT_COMMAND_CHANNEL_0_LOHI_MODE_3);
    io_out8(PIT_CHANNEL_0_DATA, (uint8_t)divisor);
    io_out8(PIT_CHANNEL_0_DATA, (uint8_t)(divisor >> 8));
    __atomic_store_n(&ticks, 0, __ATOMIC_RELAXED);
    frequency = frequency_hz;
    initialized = true;
    return true;
}

bool timer_is_initialized(void) {
    return initialized;
}

void timer_interrupt(void) {
    if (initialized) {
        (void)__atomic_add_fetch(&ticks, 1, __ATOMIC_RELAXED);
    }
}

uint64_t timer_ticks(void) {
    return __atomic_load_n(&ticks, __ATOMIC_RELAXED);
}

uint64_t timer_uptime_seconds(void) {
    return initialized ? timer_ticks() / frequency : 0;
}

uint32_t timer_frequency_hz(void) {
    return frequency;
}

void timer_wait_ticks(uint64_t requested_ticks) {
    uint64_t start = timer_ticks();
    while (timer_ticks() - start < requested_ticks) {
        cpu_wait_for_interrupt();
    }
}
