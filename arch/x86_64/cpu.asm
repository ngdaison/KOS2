bits 64
default rel

global gdt_load
global idt_load
global tss_load

section .text

; rdi points to a packed { uint16_t limit; uint64_t base; } descriptor.
gdt_load:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    push qword 0x08
    lea rax, [rel .reload_code_segment]
    push rax
    retfq
.reload_code_segment:
    ret

idt_load:
    lidt [rdi]
    ret

tss_load:
    mov ax, di
    ltr ax
    ret
