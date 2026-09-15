# Host environment

## Required host tools

The eventual build and test loop needs the following tools on the Windows host or
inside WSL:

| Tool | Purpose |
| --- | --- |
| Git | Obtain Limine and manage the project. |
| LLVM/Clang and LLD | Compile and link the freestanding x86_64 ELF kernel. |
| NASM | Assemble x86_64 entry/interrupt code. |
| QEMU system emulator for x86_64 | Run and debug KOS. |
| PowerShell 7+ | Run KOS's native build, staging, and launch scripts. |
| xorriso | Optional: create hybrid BIOS/UEFI ISO images in a later packaging phase. |

## Recommended setup: WSL Ubuntu

WSL can provide a predictable POSIX environment for future packaging and tooling.
Install Ubuntu, then install the listed build tools through its package manager.
The compiler must be able to emit freestanding `x86_64-elf` objects; do not use
a normal Windows-targeting compiler for the kernel. The checked-in PowerShell
build scripts currently standardize on Clang and LLD; a GCC flow is possible
later, but is not yet a supported script target.

The project itself may remain in its current Windows directory, but building in a
Linux-native workspace is usually faster and avoids path/permission edge cases.

## Native Windows alternative

KOS supports a native PowerShell build flow using LLVM, NASM, and QEMU. The build
script invokes Clang with `--target=x86_64-unknown-none-elf` and LLD directly, so
it does not link against a Windows C runtime. MSYS2 remains a valid alternative
when Unix-oriented tools are preferred.

## Verify the environment

From the repository root, run:

```powershell
.\\scripts\\check-environment.ps1
```

The script only reports availability; it does not install or alter system tools.
When it passes, fetch the bootloader source with:

```powershell
.\\scripts\\bootstrap-limine.ps1
```

For the supported native Windows path, `scripts/bootstrap-toolchain.ps1` can
download the pinned portable NASM assembler after LLVM and QEMU are installed.
