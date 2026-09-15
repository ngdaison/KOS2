# Phase 2: bootable kernel plan

## Objective

Produce a minimal x86_64 ELF kernel which Limine loads through UEFI, transfers
to an Assembly entry point, gives an explicit 64 KiB kernel stack, and then
hands control to `kernel_main()` in freestanding C. The first observable result
must be this serial line:

```text
KOS: kernel started
```

## Implementation breakdown

1. **Boot protocol contract** — `boot/limine_requests.c` places the Limine base
   revision and bootloader-information requests in a dedicated, kept ELF segment.
2. **ELF memory layout** — `boot/linker.ld` creates a higher-half kernel with
   separate read-only, executable, and writable load segments.
3. **Architecture entry** — `boot/entry.asm` clears direction flags, disables
   interrupts during earliest initialization, installs KOS's own 64 KiB stack,
   and calls C using the x86_64 System V ABI.
4. **Freestanding C entry** — `kernel/main.c` initializes COM1, checks that
   Limine accepted the requested protocol revision, logs progress, then halts.
5. **Serial driver** — `drivers/serial.c` implements polling output for the
   QEMU COM1 device without libc, heap allocation, or interrupts. A bounded
   transmitter wait prevents an absent or wedged serial device from freezing
   the whole kernel during a diagnostic write.
6. **UEFI boot media** — build scripts stage `BOOTX64.EFI`, `limine.conf`, and
   `kos.elf` into a FAT-backed EFI System Partition directory used directly by
   QEMU.

## Completion checks

| Check | Evidence |
| --- | --- |
| Kernel is an ELF64 x86_64 executable | `llvm-readobj --file-headers build/kos.elf` reports `ELF64` and `EM_X86_64`. |
| Entry is linked | `llvm-nm build/kos.elf` contains `_start` and `kernel_main`. |
| UEFI can find Limine | QEMU reaches the KOS boot entry. |
| Serial path works | QEMU terminal shows `KOS: kernel started`. |
| Kernel remains controlled | It prints its phase completion line then halts rather than falling through. |

## Verified result

The native Windows build was verified with LLVM 22.1.8, NASM 3.02, Limine
12.9.0, and QEMU 11.1.0. `llvm-readobj` reported an `elf64-x86-64` executable
with `EM_X86_64`; QEMU then emitted:

```text
KOS: kernel started
KOS booting...
Serial COM1 online.
```

## Not implemented by this phase

The boot foundation is now consumed by later completed CPU and memory phases.
Keyboard input, timer interrupts, command parsing beyond the bootstrap command,
filesystems, processes, and GUI remain later work.
