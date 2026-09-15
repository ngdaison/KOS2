# KOS v0.1 architecture

## Design intent

KOS is an educational, command-line-first kernel. Early development favors
observability and simple, correct boundaries over feature count. Every subsystem
must be able to emit diagnostic logs to the serial console.

## Platform decisions

| Concern | Decision | Reason |
| --- | --- | --- |
| CPU | x86_64 | Well documented and fully supported by QEMU. |
| Firmware | UEFI | Modern, deterministic virtual firmware path. |
| Bootloader | Limine | It handles firmware/ELF loading and passes framebuffer and memory-map data. |
| Kernel model | Monolithic | Keeps the first kernel small and reduces early IPC complexity. |
| Primary language | Freestanding C | Direct control of hardware and a small runtime surface. |
| Assembly | Minimal x86_64 Assembly | Used only where C cannot safely express the operation. |
| Main test target | QEMU | Enables fast, reproducible testing before real hardware. |

## Layering

```text
QEMU + UEFI firmware
        |
     Limine
        |
  boot protocol boundary
        |
 kernel core ---- arch/x86_64
   |       \         |
 memory    drivers   interrupts/timer
   |          |
   +---- kernel debug console
```

The debug console belongs to the kernel in v0.1. It is deliberately not treated
as user space. A later version may replace it with a terminal emulator and shell.

## Memory policy for v0.1

- Page size: 4 KiB.
- The physical memory manager owns only frames marked usable by the boot memory map.
- Kernel image, bootloader-owned regions, framebuffer, and firmware-reserved areas
  are never allocated as free frames.
- Dynamic allocations are kernel-only; no user address space exists in v0.1.

## Error policy

An unrecoverable kernel condition enters `panic`: write a concise reason and any
available CPU state to serial and framebuffer, then halt. Silent hangs are bugs.
