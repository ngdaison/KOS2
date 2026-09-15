#include <stdint.h>

#include <kos/compiler.h>
#include <kos/cpu.h>

struct descriptor_table_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

extern void gdt_load(const struct descriptor_table_pointer *pointer);
extern void tss_load(uint16_t selector);

struct task_state_segment {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t io_map_base;
} __attribute__((packed));

enum {
    DOUBLE_FAULT_STACK_SIZE = 16384,
};

static uint64_t gdt_entries[5] KOS_ALIGNED(16) = {
    0x0000000000000000ull,
    0x00af9a000000ffffull,
    0x00cf92000000ffffull,
};
static struct task_state_segment tss KOS_ALIGNED(16);
static uint8_t double_fault_stack[DOUBLE_FAULT_STACK_SIZE] KOS_ALIGNED(16);

static void gdt_configure_tss_descriptor(void) {
    uint64_t base = (uint64_t)&tss;
    uint64_t limit = sizeof(tss) - 1;

    gdt_entries[3] = (limit & 0xffff)
        | ((base & 0xffffff) << 16)
        | (0x89ull << 40)
        | ((limit & 0xf0000) << 32)
        | (((base >> 24) & 0xff) << 56);
    gdt_entries[4] = base >> 32;
}

void gdt_initialize(void) {
    tss.ist1 = (uint64_t)(double_fault_stack + DOUBLE_FAULT_STACK_SIZE);
    tss.io_map_base = sizeof(tss);
    gdt_configure_tss_descriptor();

    const struct descriptor_table_pointer gdt_pointer = {
        .limit = sizeof(gdt_entries) - 1,
        .base = (uint64_t)gdt_entries,
    };
    gdt_load(&gdt_pointer);
    tss_load(KOS_KERNEL_TSS_SELECTOR);
}
