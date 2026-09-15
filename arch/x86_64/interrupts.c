#include <stdint.h>

#include <kos/compiler.h>
#include <kos/console.h>
#include <kos/cpu.h>
#include <kos/interrupts.h>
#include <kos/log.h>
#include <kos/pic.h>
#include <kos/serial.h>

enum {
    IDT_ENTRY_COUNT = 256,
    IDT_INTERRUPT_GATE = 0x8e,
};

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct interrupt_context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t vector;
    uint64_t error_code;
    uint64_t instruction_pointer;
    uint64_t code_segment;
    uint64_t rflags;
};

extern void idt_load(const struct idt_pointer *pointer);
extern void (*isr_stub_table[])(void);

static struct idt_entry idt_entries[IDT_ENTRY_COUNT] KOS_ALIGNED(16);
static irq_handler irq_handlers[16];
static volatile uint64_t irq_counts[16];
static volatile uint64_t irq_unhandled_counts[16];

static void idt_set_gate(uint8_t vector, void (*handler)(void), uint8_t ist) {
    uint64_t address = (uint64_t)handler;
    idt_entries[vector] = (struct idt_entry){
        .offset_low = (uint16_t)address,
        .selector = KOS_KERNEL_CODE_SELECTOR,
        .ist = ist,
        .type_attributes = IDT_INTERRUPT_GATE,
        .offset_middle = (uint16_t)(address >> 16),
        .offset_high = (uint32_t)(address >> 32),
        .reserved = 0,
    };
}

void idt_initialize(void) {
    for (uint16_t vector = 0; vector < IDT_ENTRY_COUNT; ++vector) {
        uint8_t ist = vector == 8 ? 1 : 0;
        idt_set_gate((uint8_t)vector, isr_stub_table[vector], ist);
    }

    const struct idt_pointer pointer = {
        .limit = sizeof(idt_entries) - 1,
        .base = (uint64_t)idt_entries,
    };
    idt_load(&pointer);
}

static const char *exception_name(uint64_t vector) {
    switch (vector) {
        case 0: return "Divide-by-zero (#DE)";
        case 8: return "Double fault (#DF)";
        case 6: return "Invalid opcode (#UD)";
        case 10: return "Invalid TSS (#TS)";
        case 11: return "Segment not present (#NP)";
        case 12: return "Stack-segment fault (#SS)";
        case 13: return "General protection fault (#GP)";
        case 14: return "Page fault (#PF)";
        case 17: return "Alignment check (#AC)";
        case 21: return "Control protection (#CP)";
        case 29: return "VMM communication (#VC)";
        case 30: return "Security exception (#SX)";
        default: return "Unhandled CPU exception";
    }
}

static void report_page_fault(uint64_t error_code) {
    log_info_hex("CR2: ", cpu_read_cr2());
    if ((error_code & 1) != 0) {
        log_error("Page fault cause: protection violation.");
    }
    else {
        log_error("Page fault cause: non-present page.");
    }
    if ((error_code & 2) != 0) {
        log_error("Page fault access: write.");
    }
    else {
        log_error("Page fault access: read.");
    }
    if ((error_code & 4) != 0) {
        log_error("Page fault mode: user.");
    }
    else {
        log_error("Page fault mode: supervisor.");
    }
    if ((error_code & 16) != 0) {
        log_error("Page fault access: instruction fetch.");
    }
    if ((error_code & 8) != 0) {
        log_error("Page fault cause: reserved-bit violation.");
    }
    if ((error_code & 32) != 0) {
        log_error("Page fault cause: protection key.");
    }
    if ((error_code & 64) != 0) {
        log_error("Page fault cause: shadow stack.");
    }
    if ((error_code & 0x8000) != 0) {
        log_error("Page fault cause: SGX violation.");
    }
}

static bool vector_is_pic_irq(uint64_t vector) {
    return vector >= KOS_PIC_MASTER_VECTOR && vector < KOS_PIC_MASTER_VECTOR + 16;
}

bool irq_register_handler(uint8_t irq, irq_handler handler) {
    if (irq >= 16 || handler == 0 || irq_handlers[irq] != 0) {
        return false;
    }
    irq_handlers[irq] = handler;
    return true;
}

bool irq_unregister_handler(uint8_t irq, irq_handler handler) {
    if (irq >= 16 || handler == 0 || irq_handlers[irq] != handler) {
        return false;
    }
    irq_handlers[irq] = 0;
    return true;
}

uint64_t irq_dispatch_count(uint8_t irq) {
    return irq < 16 ? irq_counts[irq] : 0;
}

uint64_t irq_unhandled_count(uint8_t irq) {
    return irq < 16 ? irq_unhandled_counts[irq] : 0;
}

void interrupt_dispatch(const struct interrupt_context *context) {
    if (vector_is_pic_irq(context->vector)) {
        uint8_t irq = (uint8_t)(context->vector - KOS_PIC_MASTER_VECTOR);
        if (pic_is_spurious_irq(irq)) {
            pic_send_spurious_eoi(irq);
            return;
        }
        ++irq_counts[irq];
        irq_handler handler = irq_handlers[irq];
        if (handler != 0) {
            handler(irq, (void *)context);
        }
        else {
            ++irq_unhandled_counts[irq];
        }
        pic_send_eoi(irq);
        return;
    }

    static volatile uint8_t panic_in_progress;
    if (panic_in_progress != 0) {
        cpu_halt();
    }
    panic_in_progress = 1;
    log_error("KERNEL PANIC");
    log_error(exception_name(context->vector));
    log_info_hex("Vector: ", context->vector);
    log_info_hex("Error code: ", context->error_code);
    log_info_hex("RIP: ", context->instruction_pointer);
    log_info_hex("RFLAGS: ", context->rflags);
    if (context->vector == 14) {
        report_page_fault(context->error_code);
    }
    log_error("System halted.");
    cpu_halt();
}
