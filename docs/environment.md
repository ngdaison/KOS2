# Host environment

## Required host tools

The build and test loop needs the following tools on the host:

| Tool | Purpose |
| --- | --- |
| Git | Obtain Limine and manage the project. |
| LLVM/Clang and LLD | Compile and link the freestanding x86_64 ELF kernel. |
| NASM | Assemble x86_64 entry/interrupt code. |
| QEMU system emulator for x86_64 | Run and debug KOS. |
| Python 3.10+ | Run KOS's cross-platform build, staging, and launch tool. |
| xorriso | Optional: create hybrid BIOS/UEFI ISO images in a later packaging phase. |

`tools/kos.py` invokes Clang with `--target=x86_64-unknown-none-elf`, NASM, and
LLD directly; it does not link against a host C runtime. The same Python command
interface is used on native Windows, Linux, and macOS, subject to the available
QEMU firmware path.

## Verify the environment

From the repository root, run:

```text
python tools/kos.py check
```

The script only reports availability; it does not install or alter system tools.
When it passes, fetch the bootloader source with:

```text
python tools/kos.py bootstrap-limine
python tools/kos.py bootstrap-assets
```

On Windows, `python tools/kos.py bootstrap-nasm` downloads the pinned portable
NASM assembler after LLVM and QEMU are installed.
