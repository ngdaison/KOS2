# KOS: optimized master roadmap, phases 0–12

This is the single status document for the whole kernel path. A phase marked
**implemented** has a QEMU acceptance check; a phase marked **foundation** is a
real, working base but deliberately does not claim its later product features.

| Phase | Optimized responsibility | Status | Acceptance check |
| --- | --- | --- | --- |
| 0 | Scope, x86_64/UEFI/QEMU boundary, no GUI or compatibility claims | Implemented | Scope is explicit and testable. |
| 1 | Reproducible cross-build, source layout, QEMU tooling | Implemented | `tools\kos-tool.exe build` stages a bootable ESP. |
| 2 | Limine entry, owned bootstrap stack, serial-first diagnostics | Implemented | `KOS: kernel started`. |
| 3 | Framebuffer text console and shared log output | Implemented | Text is visible in QEMU and serial. |
| 4 | GDT, IDT, exception report, safe halt | Implemented | Deliberate divide-by-zero prints a panic. |
| 5 | 4 KiB physical-frame allocator and memory-map accounting | Implemented | `meminfo` reports RAM and frames. |
| 6 | Existing 4-level mappings, permission checks, reclaiming heap | Implemented | Heap self-test passes without page fault. |
| 7 | PIC/PIT, IRQ registry, spurious IRQ handling, monotonic ticks | Implemented | IRQ0 runs at 100 Hz with no unhandled timer IRQ. |
| 8 | Bounded PS/2 setup, SPSC input ring, modifier state | Implemented | Shift, Enter, and Backspace work. |
| 9 | Bounded kernel terminal and table-based debug commands | Implemented | Every command returns to `KOS> `. |
| 10 | Driver lifecycle registry and read-only PCI discovery | Implemented foundation | `drivers` and `lspci` work in QEMU. |
| 11 | Separate task stacks, Assembly context switch, round-robin tasks | Implemented cooperative foundation | `tasktest` reclaims both worker stacks. |
| 12 | Read-only VFS, CPIO initramfs, bounded ELF64 inspection | Implemented foundation | `ls`, `cat`, and `elfinfo` work. |

## Cross-phase engineering rules

1. **Early boot stays dependency-free.** Serial, framebuffer, GDT/IDT, PMM and
   VMM bootstrap before heap, task, VFS, or driver-framework code is used.
2. **Interrupt handlers are bounded.** They acknowledge hardware, update small
   state, and queue work. They never allocate, log, parse commands, or wait.
3. **Every external structure is validated.** Limine responses, CPIO bounds,
   CPIO paths, and ELF program-header ranges are checked before use.
4. **All mutable lifetime has an owner.** The PMM owns physical frames; VMM owns
   mappings it created; heap owns allocations; task code releases terminated
   worker stacks; the VFS owns only its mounted backend pointer.
5. **A feature must have an observable diagnostic.** `meminfo`, `irqinfo`,
   `kbdinfo`, `lspci`, `taskinfo`, `ls`, and `elfinfo` make early failures visible.
6. **Never overstate a foundation as a finished desktop subsystem.** PCI
   discovery is not a storage driver; cooperative tasks are not preemption;
   ELF inspection is not execution; initramfs is not a persistent filesystem.

## High-value optimization changes now applied

- IRQ registration removes device-specific branches from the dispatcher and
  handles legacy PIC spurious IRQ7/15 correctly.
- Keyboard uses independent left/right Shift bits and a fixed SPSC ring buffer.
- Driver names must be unique; reverse-order shutdown is available for later
  reset/unload paths.
- Task demos and tests free their completed worker stacks. Failed second-task
  creation rolls back the first one instead of leaving a hidden runnable task.
- The initramfs selects its Limine module by `module_string`, so future modules
  do not break mounting. CPIO paths reject absolute/traversal names and duplicate
  entries; only regular files are exposed.
- ELF inspection verifies ELF64 identity, type, machine, header bounds,
  `PT_LOAD` file ranges, `memsz >= filesz`, and alignment sanity.

## Next implementation order

1. **11B — preemptive scheduler:** interrupt-return context switching, guard
   pages, no-preempt guards, sleep queue, then a CPU-bound preemption test.
2. **12B — block/VFS/FAT32:** block-device API, read-only virtio-blk or AHCI,
   FAT32 path lookup and directories, then a multi-mount VFS.
3. **12C — user mode:** USER mappings, isolated page tables, TSS/Ring 0 stack,
   and a single fixed user-mode smoke test.
4. **12D — syscalls:** a small validated ABI (`write`, `exit`, `yield`,
   `uptime`) and user-pointer checking.
5. **12E/F — program loader and shell:** map validated ELF `PT_LOAD` segments,
   enter Ring 3, then move the normal shell out of the kernel.

Windows application compatibility, GUI, GPU acceleration, and persistent
application installation remain after those foundations; they are not kernel
optimizations that can safely be skipped ahead to.
