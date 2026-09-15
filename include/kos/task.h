#ifndef KOS_TASK_H
#define KOS_TASK_H

#include <stdbool.h>
#include <stdint.h>

#include <kos/compiler.h>

struct task_context {
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rsp;
    uint64_t rip;
};

typedef void (*task_entry)(void *argument);

bool task_initialize(void);
bool task_create_kernel(const char *name, task_entry entry, void *argument);
void task_yield(void);
KOS_NORETURN void task_exit(void);
void task_timer_tick(void);
void task_reschedule_if_needed(void);
uint64_t task_count(void);
uint64_t task_ready_count(void);
bool task_run_self_test(void);
bool task_run_log_demo(void);

#endif
