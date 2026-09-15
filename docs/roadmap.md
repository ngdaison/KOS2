# KOS roadmap

KOS v0.1 is deliberately a small kernel milestone. It exists to establish the
technical base for KOS as a future desktop operating system, rather than define
the final product boundary.

## v0.1: kernel foundation

**Goal:** Boot a small x86_64 kernel in QEMU and provide a kernel-resident debug
console. This is not yet a general-purpose operating system or a user-space shell.

### Included capabilities

1. Boot from UEFI through Limine into a freestanding x86_64 kernel.
2. Serial logging and framebuffer text output.
3. CPU exception handling through GDT and IDT.
4. A physical-frame allocator and a small kernel heap.
5. Timer interrupts and monotonic uptime.
6. PS/2 keyboard input.
7. An in-kernel line console with the following commands:
   - `help`
   - `clear`
   - `meminfo`
   - `uptime`
   - `reboot`

### Explicitly out of scope

- Networking
- USB support
- GUI, mouse support, audio, and graphics acceleration
- Symmetric multiprocessing (multiple CPU cores)
- Linux binary compatibility or Linux drivers
- Persistent filesystem, ELF loading, user mode, and a separate shell

## Delivery order

| Phase | Deliverable | Completion check |
| --- | --- | --- |
| 0 | Scope and architecture decisions | This documentation is approved. |
| 1 | Host environment and repository layout | Environment script reports the required tools. |
| 2 | Kernel entry and serial log | QEMU displays `KOS: kernel started`. |
| 3 | Framebuffer console | Text appears in the QEMU display. |
| 4 | CPU exception diagnostics | A deliberate divide-by-zero produces a readable panic. |
| 5 | Physical memory manager | `meminfo` can report usable memory and free frames. |
| 6 | Paging and kernel heap | Kernel allocations work without page faults. |
| 7 | Timer interrupt | PIT IRQ0 passes a one-second self-test and backs the monotonic uptime API. |
| 8 | Keyboard driver | PS/2 IRQ1 input, Shift, Enter, and Backspace work. |
| 9 | Kernel debug console | All six initial commands work and a fresh prompt follows each line. |
| 10 | Driver framework and PCI | Built-in drivers share a lifecycle registry; `lspci` inventories QEMU PCI devices. |
| 11A | Cooperative kernel threads | Separate task stacks and verified round-robin context switches work. |
| 12A | Initramfs VFS | Limine loads a read-only CPIO archive; `ls`, `cat`, and ELF inspection work. |

Work advances one phase at a time; a phase is not considered complete merely
because its code compiles.

## Beyond v0.1: desktop OS direction

After v0.1, KOS progresses through these product milestones:

| Milestone | Outcome |
| --- | --- |
| v0.2 | Finish preemptive scheduling, then add user mode, system calls, executable loading, and an initial filesystem. |
| v0.3 | User-space shell, package/application layout, processes, permissions, and basic device storage. |
| v0.4 | Window server, compositor, input stack, fonts, desktop shell, and bundled graphical utilities. |
| v0.5 | Stable application API, installer/update path, networking, stronger security boundaries, and hardware-driver expansion. |

The aim is comparable **category and capability**, not copying another OS's code,
brand, visual assets, APIs, or proprietary binary formats.
