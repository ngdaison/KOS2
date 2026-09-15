#ifndef KOS_INTERRUPTS_H
#define KOS_INTERRUPTS_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*irq_handler)(uint8_t irq, void *context);

bool irq_register_handler(uint8_t irq, irq_handler handler);
bool irq_unregister_handler(uint8_t irq, irq_handler handler);
uint64_t irq_dispatch_count(uint8_t irq);
uint64_t irq_unhandled_count(uint8_t irq);

#endif
