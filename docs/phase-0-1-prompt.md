# Prompt: establish KOS Phase 0 and Phase 1

```text
You are setting up the foundation of an educational x86_64 operating-system
project named KOS. Implement only Phase 0 (scope) and Phase 1 (repository and
host-environment preparation). Do not write a bootable kernel, boot assembly,
memory manager, drivers, terminal, filesystem, or user-space code.

Use these fixed decisions:
- Target: x86_64, UEFI, QEMU.
- Bootloader: Limine, obtained as a vendored third-party dependency rather than
  written from scratch.
- Intended implementation language: freestanding C with minimal x86_64 Assembly.
- Kernel style: small monolithic kernel.
- First functional milestone: boot -> text output -> keyboard input -> command
  console -> basic RAM management -> timer/interrupts.
- The initial in-kernel console commands will be: help, clear, meminfo, uptime,
  and reboot.
- Out of scope for KOS v0.1: networking, USB, GUI, audio, SMP, Linux
  compatibility, persistent filesystems, ELF loading, user mode, and a separate
  shell.

Create this repository layout with placeholders where source code will later go:
arch/x86_64, boot, mm, drivers, kernel, lib, include, docs, scripts, third_party.

Write concise English documentation that includes:
1. README with project goal, target, layout, and current status.
2. A roadmap with phases, scope, exclusions, and observable completion checks.
3. An architecture document covering the bootloader boundary, monolithic layering,
   debug console status, memory policy, and panic policy.
4. A boot-process diagram and required boot information.
5. Environment instructions for WSL and native Windows, plus a list of tools:
   Git, an ELF-capable x86_64 freestanding toolchain, NASM, a build tool, QEMU,
   and xorriso.
6. Coding conventions appropriate for freestanding kernel C.

Also create PowerShell scripts that:
- Check tool availability without modifying the system.
- Clone Limine from its official repository into third_party/limine only when
  explicitly run, and refuse to overwrite an existing non-empty destination.

Add a .gitignore for build artifacts, IDE state, and downloaded Limine sources.
The result must not claim that the kernel builds or boots yet. End with a concise
summary of created files and unavailable host dependencies.
```
