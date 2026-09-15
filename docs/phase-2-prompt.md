# Prompt: implement KOS Phase 2

```text
Implement only Phase 2 of KOS, an independent x86_64 desktop operating-system
project. The long-term product goal is a graphical OS, but this phase must stay
strictly focused on a safe, observable kernel boot path.

Use Limine on UEFI in QEMU. Build a freestanding ELF64 x86_64 kernel in C with
minimal NASM Assembly. Add a linker script, Limine request delimiters and base
revision request, an Assembly `_start` symbol, a dedicated aligned 64 KiB boot
stack, and a noreturn `kernel_main()` C function. `_start` must disable
interrupts, clear the direction flag, install the KOS stack, align `rsp` for
the System V ABI, and call `kernel_main`.

Implement a polling COM1 serial driver using port I/O only. In `kernel_main`,
initialize COM1, print the exact first line `KOS: kernel started`, verify that
Limine accepted the base revision, print useful completion logs, and halt
cleanly. Do not add framebuffer, keyboard, timer, heap, filesystem, processes,
user mode, or GUI code.

Provide reproducible PowerShell build/run scripts. The build must target
`x86_64-unknown-none-elf`, use no host C runtime or libc, assemble with NASM,
link with LLD and a custom linker script, and stage a UEFI FAT ESP directory
containing Limine `BOOTX64.EFI`, `limine.conf`, and `boot/kos.elf`. The run
script must launch QEMU with UEFI firmware and expose COM1 in the terminal.

Verify the resulting ELF symbols and architecture, then boot it in QEMU. Report
the exact serial output and any unavailable host dependency. Do not claim a
successful boot unless the serial line was actually observed.
```
