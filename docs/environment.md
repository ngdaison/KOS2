# Host environment

## Required host tools

The build and test loop needs the following tools on the host:

| Tool | Purpose |
| --- | --- |
| Git | Obtain Limine and manage the project. |
| LLVM/Clang and LLD | Compile and link the freestanding x86_64 ELF kernel. |
| NASM | Assemble x86_64 entry/interrupt code. |
| QEMU system emulator for x86_64 | Run and debug KOS. |
| A hosted C17 compiler | Compile KOS's native C build, staging, and launch tool. |
| xorriso | Optional: create hybrid BIOS/UEFI ISO images in a later packaging phase. |

`tools/kos.c` is a hosted C17 program for the Windows development host. It invokes
Clang with `--target=x86_64-unknown-none-elf`, NASM, and LLD directly; the kernel
it builds does not link against a host C runtime.

## Verify the environment

From the repository root, run:

```text
clang -std=c17 -O2 -Wall -Wextra -Werror tools/kos.c -o tools/kos-tool.exe
tools\kos-tool.exe check
```

The script only reports availability; it does not install or alter system tools.
When it passes, fetch the bootloader source with:

```text
tools\kos-tool.exe bootstrap-limine
tools\kos-tool.exe bootstrap-assets
```

Install NASM 3.02 or newer and add it to `PATH`, or put `nasm.exe` at
`third_party/tools/nasm-3.02/nasm.exe`. Place Limine's UEFI `BOOTX64.EFI` at
`third_party/limine-binary/BOOTX64.EFI` before building.
