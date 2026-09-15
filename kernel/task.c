#include <stdbool.h>
#include <stdint.h>

#include <kos/compiler.h>
#include <kos/cpu.h>
#include <kos/heap.h>
#include <kos/memory.h>
#include <kos/log.h>
#include <kos/task.h>

enum {
    TASK_MAX_COUNT = 8,
    TASK_KERNEL_STACK_SIZE = KOS_PAGE_SIZE * 4,
    TASK_SELF_TEST_ITERATIONS = 32,
    TASK_LOG_DEMO_ITERATIONS = 5,
};

enum task_state {
    TASK_UNUSED,
    TASK_RUNNING,
    TASK_READY,
    TASK_TERMINATED,
};

struct task {
    struct task_context context;
    task_entry entry;
    void *argument;
    uint8_t *stack;
    const char *name;
    enum task_state state;
    uint64_t id;
};

struct task_self_test_counter {
    uint64_t count;
};

struct task_log_demo_state {
    const char *label;
    uint64_t count;
};

extern void task_switch(struct task_context *old, const struct task_context *next);

_Static_assert(__builtin_offsetof(struct task_context, rsp) == 48,
    "task_switch.asm expects rsp at offset 48");
_Static_assert(__builtin_offsetof(struct task_context, rip) == 56,
    "task_switch.asm expects rip at offset 56");

static struct task tasks[TASK_MAX_COUNT];
static struct task *current_task;
static uint64_t next_task_id = 1;
static bool initialized;
static volatile bool reschedule_requested;

static uint64_t task_index(const struct task *task) {
    return (uint64_t)(task - tasks);
}

static struct task *task_find_next_ready(void) {
    uint64_t start = task_index(current_task);
    for (uint64_t step = 1; step <= TASK_MAX_COUNT; ++step) {
        struct task *candidate = &tasks[(start + step) % TASK_MAX_COUNT];
        if (candidate->state == TASK_READY) {
            return candidate;
        }
    }
    return 0;
}

static bool task_release(struct task *task) {
    if (task == 0 || task == current_task || task->state == TASK_RUNNING) {
        return false;
    }
    if (task->stack != 0 && !kfree(task->stack)) {
        return false;
    }
    *task = (struct task){ .state = TASK_UNUSED };
    return true;
}

static void task_reap_terminated(void) {
    for (uint64_t index = 1; index < TASK_MAX_COUNT; ++index) {
        if (tasks[index].state == TASK_TERMINATED) {
            (void)task_release(&tasks[index]);
        }
    }
}

static KOS_NORETURN void task_start_trampoline(void) {
    current_task->entry(current_task->argument);
    task_exit();
}

bool task_initialize(void) {
    if (initialized) {
        return false;
    }
    for (uint64_t index = 0; index < TASK_MAX_COUNT; ++index) {
        tasks[index].state = TASK_UNUSED;
    }
    tasks[0].name = "bootstrap";
    tasks[0].state = TASK_RUNNING;
    tasks[0].id = next_task_id++;
    current_task = &tasks[0];
    initialized = true;
    return true;
}

bool task_create_kernel(const char *name, task_entry entry, void *argument) {
    if (!initialized || name == 0 || entry == 0) {
        return false;
    }
    task_reap_terminated();
    struct task *created = 0;
    for (uint64_t index = 1; index < TASK_MAX_COUNT; ++index) {
        if (tasks[index].state == TASK_UNUSED) {
            created = &tasks[index];
            break;
        }
    }
    if (created == 0) {
        return false;
    }
    uint8_t *stack = kmalloc(TASK_KERNEL_STACK_SIZE);
    if (stack == 0) {
        return false;
    }
    uint64_t stack_top = ((uint64_t)stack + TASK_KERNEL_STACK_SIZE) & ~0xfull;
    stack_top -= sizeof(uint64_t);
    *(uint64_t *)stack_top = 0;
    *created = (struct task){
        .context = {
            .rsp = stack_top,
            .rip = (uint64_t)task_start_trampoline,
        },
        .entry = entry,
        .argument = argument,
        .stack = stack,
        .name = name,
        .state = TASK_READY,
        .id = next_task_id++,
    };
    return true;
}

void task_yield(void) {
    if (!initialized) {
        return;
    }
    struct task *next = task_find_next_ready();
    __atomic_store_n(&reschedule_requested, false, __ATOMIC_RELAXED);
    if (next == 0) {
        return;
    }
    struct task *previous = current_task;
    if (previous->state == TASK_RUNNING) {
        previous->state = TASK_READY;
    }
    next->state = TASK_RUNNING;
    current_task = next;
    task_switch(&previous->context, &next->context);
}

KOS_NORETURN void task_exit(void) {
    if (!initialized || current_task == &tasks[0]) {
        cpu_halt();
    }
    current_task->state = TASK_TERMINATED;
    struct task *previous = current_task;
    struct task *next = task_find_next_ready();
    if (next == 0) {
        cpu_halt();
    }
    next->state = TASK_RUNNING;
    current_task = next;
    task_switch(&previous->context, &next->context);
    cpu_halt();
}

void task_timer_tick(void) {
    __atomic_store_n(&reschedule_requested, true, __ATOMIC_RELAXED);
}

void task_reschedule_if_needed(void) {
    if (__atomic_exchange_n(&reschedule_requested, false, __ATOMIC_RELAXED)) {
        task_yield();
    }
}

uint64_t task_count(void) {
    uint64_t count = 0;
    for (uint64_t index = 0; index < TASK_MAX_COUNT; ++index) {
        if (tasks[index].state != TASK_UNUSED) {
            ++count;
        }
    }
    return count;
}

uint64_t task_ready_count(void) {
    uint64_t count = 0;
    for (uint64_t index = 0; index < TASK_MAX_COUNT; ++index) {
        if (tasks[index].state == TASK_READY) {
            ++count;
        }
    }
    return count;
}

static void task_self_test_entry(void *argument) {
    struct task_self_test_counter *counter = argument;
    for (uint64_t index = 0; index < TASK_SELF_TEST_ITERATIONS; ++index) {
        ++counter->count;
        task_yield();
    }
}

bool task_run_self_test(void) {
    static struct task_self_test_counter first;
    static struct task_self_test_counter second;

    if (!initialized || task_ready_count() != 0) {
        return false;
    }
    first.count = 0;
    second.count = 0;
    if (!task_create_kernel("selftest-A", task_self_test_entry, &first)) {
        return false;
    }
    if (!task_create_kernel("selftest-B", task_self_test_entry, &second)) {
        (void)task_release(&tasks[1]);
        return false;
    }
    while (task_ready_count() != 0) {
        task_yield();
    }
    bool passed = first.count == TASK_SELF_TEST_ITERATIONS && second.count == TASK_SELF_TEST_ITERATIONS;
    task_reap_terminated();
    return passed;
}

static void task_log_demo_entry(void *argument) {
    struct task_log_demo_state *state = argument;
    for (uint64_t index = 0; index < TASK_LOG_DEMO_ITERATIONS; ++index) {
        ++state->count;
        log_info_u64(state->label, state->count);
        task_yield();
    }
}

bool task_run_log_demo(void) {
    static struct task_log_demo_state first = { .label = "task-A count: " };
    static struct task_log_demo_state second = { .label = "task-B count: " };

    if (!initialized || task_ready_count() != 0) {
        return false;
    }
    first.count = 0;
    second.count = 0;
    if (!task_create_kernel("log-demo-A", task_log_demo_entry, &first)) {
        return false;
    }
    if (!task_create_kernel("log-demo-B", task_log_demo_entry, &second)) {
        (void)task_release(&tasks[1]);
        return false;
    }
    while (task_ready_count() != 0) {
        task_yield();
    }
    bool passed = first.count == TASK_LOG_DEMO_ITERATIONS && second.count == TASK_LOG_DEMO_ITERATIONS;
    task_reap_terminated();
    return passed;
}
