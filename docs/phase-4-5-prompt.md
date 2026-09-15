# Prompt: implement KOS Phases 4 and 5

```text
Implement Phases 4 and 5 of KOS, preserving the existing Limine, serial, and
framebuffer console boot path.

Phase 4: install a 64-bit GDT with kernel code/data selectors and reload all
segment registers safely. Install a 256-entry IDT, with proper interrupt gates
and Assembly stubs for divide-by-zero (#DE), invalid opcode (#UD), and page
fault (#PF). The stub must normalize error-code and no-error-code frames, save
general-purpose registers, call a C dispatcher with ABI-safe stack alignment,
then be able to restore and iretq. The C dispatcher must show a kernel panic on
both COM1 and framebuffer, including exception name, vector, error code, RIP,
and RFLAGS, then disable interrupts and halt. Add a build-only deliberate
divide-by-zero test; never make it enabled in the normal build.

Phase 5: request Limine's memory map. Implement a bootstrap physical-memory
manager for 4 KiB frames below 4 GiB using static usable/free bitmaps. Mark only
aligned LIMINE_MEMMAP_USABLE frames free; retain frame zero and every other map
type. Expose allocate/free/statistics APIs. During normal boot, allocate and
free two frames as a self-test, verify the free count is restored, and log
total/usable/tracked/free metrics to serial and framebuffer. Explain that the
interactive `meminfo` command awaits keyboard/command phases, but uses these
statistics later.

Build and test both a normal boot and the panic-test boot in QEMU. Do not add
timer interrupts, keyboard input, paging, heap, user mode, filesystem, GUI, or
SMP support.
```
