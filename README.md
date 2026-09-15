# KOS

KOS is an independent x86_64 desktop operating-system project. Its long-term
goal is a polished graphical OS in the broad category of macOS, Windows, and
desktop Linux; it is not a clone of, nor binary-compatible with, any of them.

## Current milestone

The repository currently completes Phases 0 through 10, the cooperative
kernel-thread foundation of Phase 11, and the initramfs/VFS foundation of Phase
12: reproducible host setup,
Limine boot, serial and framebuffer diagnostics, CPU exception handling,
physical/virtual memory management, a reclaiming kernel heap, hardened PIT/PIC
interrupt dispatch, PS/2 keyboard input, a kernel debug terminal, PCI discovery,
round-robin kernel threads, and a read-only initramfs VFS.

The first bootable milestone (`KOS v0.1`) is a kernel foundation, not the final
desktop experience:

```text
boot -> text output -> keyboard input -> command console -> basic RAM management -> timer/interrupts
```

The initial kernel-console commands are `help`, `clear`, `meminfo`, `uptime`,
`sysinfo`, `netinfo`, `udptest`, `echo`, and `reboot`.

## Target

- Architecture: x86_64
- Firmware: UEFI
- Emulator: QEMU
- Bootloader: Limine (vendored by the bootstrap script)
- Kernel language: freestanding C with small, architecture-specific Assembly
- Kernel style: small monolithic kernel

The longer-term product direction is recorded in [docs/product-vision.md](docs/product-vision.md).

## Repository layout

```text
arch/x86_64/  CPU-specific code: entry, descriptor tables, interrupts, paging
boot/         Boot protocol boundary and linker configuration
drivers/      Serial, framebuffer, timer, and keyboard drivers
include/      Public kernel headers
kernel/       Kernel entry and high-level coordination
lib/          Freestanding utility routines
mm/           Physical memory, virtual memory, and heap
docs/         Design decisions and project documentation
tools/        C environment, build, staging, and QEMU launcher
third_party/  Downloaded dependencies; not committed
```

## Build and run

Compile the C development tool once from the repository root:

```text
clang -std=c17 -O2 -Wall -Wextra -Werror tools/kos.c -o tools/kos-tool.exe
tools\kos-tool.exe check
```

See [docs/environment.md](docs/environment.md) for setup choices. Once the host
tools are ready, obtain Limine and its protocol/header dependency with:

```text
tools\kos-tool.exe bootstrap-limine
tools\kos-tool.exe bootstrap-assets
```

Build the KOS kernel ELF and prepare a UEFI ESP directory with:

```text
tools\kos-tool.exe build
```

Run it in QEMU with:

```text
tools\kos-tool.exe run
```

The expected serial log begins with `KOS: kernel started` and ends at the
interactive `KOS> ` prompt. To also open QEMU's framebuffer window and inspect
the text console, use:

```text
tools\kos-tool.exe run --display
```

## Documentation

- [Scope and roadmap](docs/roadmap.md)
- [Product vision](docs/product-vision.md)
- [Desktop-OS capability audit and implementation order](docs/desktop-os-gap-analysis.md)
- [VFS volumes and drive-letter paths](docs/vfs-volumes.md)
- [Networking status and path to curl](docs/networking-roadmap.md)
- [Architecture](docs/architecture.md)
- [Boot process](docs/boot-process.md)
- [Environment setup](docs/environment.md)
- [Coding conventions](docs/coding-conventions.md)
- [Phase 0-1 implementation prompt](docs/phase-0-1-prompt.md)
- [Phase 2 boot plan](docs/phase-2-boot.md)
- [Phase 3 text-console plan](docs/phase-3-text-console.md)
- [Phase 1-3 optimization notes](docs/phase-1-3-optimization.md)
- [Phase 4-5 CPU and memory plan](docs/phase-4-5-cpu-memory.md)
- [Phase 6 virtual memory and kernel heap plan](docs/phase-6-virtual-memory.md)
- [Phase 7 timer and hardware interrupt plan](docs/phase-7-timer-interrupts.md)
- [Phases 8-9 keyboard and kernel-terminal plan](docs/phase-8-9-keyboard-terminal.md)
- [Phases 7-11 runtime foundation](docs/phase-7-11-runtime-foundation.md)
- [Phases 11-12 multitasking and filesystem plan](docs/phase-11-12-multitasking-files.md)
- [Optimized master roadmap, phases 0-12](docs/optimized-master-roadmap.md)
- [Phase 4-6 optimization notes](docs/phase-4-6-optimization.md)
