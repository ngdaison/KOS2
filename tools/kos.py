#!/usr/bin/env python3
"""KOS build, dependency bootstrap, and QEMU launcher.

This repository intentionally uses Python so the same commands work on native
Windows, Linux, and macOS hosts with the required tools.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
import urllib.request
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
THIRD_PARTY = ROOT / "third_party"
LIMINE_VERSION = "v12.9.0"

C_SOURCES = (
    "boot/limine_requests.c", "arch/x86_64/cpu_control.c", "arch/x86_64/descriptors.c",
    "arch/x86_64/interrupts.c", "arch/x86_64/pci.c", "arch/x86_64/pic.c",
    "drivers/driver.c", "drivers/framebuffer.c", "drivers/keyboard.c", "drivers/serial.c",
    "drivers/timer.c", "fs/elf.c", "fs/initramfs.c", "fs/vfs.c", "kernel/command.c",
    "kernel/console.c", "kernel/log.c", "kernel/main.c", "kernel/task.c",
    "kernel/terminal.c", "mm/heap.c", "mm/pmm.c", "mm/vmm.c",
)
ASM_SOURCES = (
    "boot/entry.asm", "arch/x86_64/cpu.asm", "arch/x86_64/interrupt_stubs.asm",
    "arch/x86_64/task_switch.asm",
)


def host_paths(*paths: str) -> tuple[Path, ...]:
    return tuple(Path(path) for path in paths)


def find_tool(description: str, names: tuple[str, ...], candidates: tuple[Path, ...] = ()) -> Path:
    for name in names:
        found = shutil.which(name)
        if found:
            return Path(found)
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    choices = ", ".join((*names, *(str(path) for path in candidates)))
    raise SystemExit(f"Required tool not found: {description} ({choices})")


def run(command: list[str | Path], *, capture: bool = False) -> str:
    printable = " ".join(str(part) for part in command)
    print(f"+ {printable}")
    completed = subprocess.run(
        [str(part) for part in command], cwd=ROOT, check=True, text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
    )
    return completed.stdout if capture else ""


def require_file(path: Path, message: str) -> None:
    if not path.is_file():
        raise SystemExit(f"{message}: {path}")


def directory_has_content(path: Path) -> bool:
    return path.is_dir() and any(path.iterdir())


def clone_if_missing(repository: str, destination: Path, branch: str | None = None) -> None:
    if directory_has_content(destination):
        print(f"Already available: {destination}")
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    command: list[str | Path] = [find_tool("Git", ("git",)), "clone", "--depth", "1"]
    if branch:
        command.extend(("--branch", branch))
    command.extend((repository, destination))
    run(command)


def bootstrap_limine(skip_binary: bool) -> None:
    clone_if_missing("https://github.com/limine-bootloader/limine.git", THIRD_PARTY / "limine", LIMINE_VERSION)
    clone_if_missing("https://github.com/limine-bootloader/limine-protocol.git", THIRD_PARTY / "limine-protocol")
    if skip_binary or directory_has_content(THIRD_PARTY / "limine-binary"):
        return
    release_url = f"https://github.com/limine-bootloader/limine/releases/download/{LIMINE_VERSION}/limine-binary.zip"
    with tempfile.TemporaryDirectory(prefix="kos-limine-") as temporary:
        archive = Path(temporary) / "limine-binary.zip"
        print(f"Downloading {release_url}")
        urllib.request.urlretrieve(release_url, archive)
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(temporary)
        executable = next(Path(temporary).rglob("BOOTX64.EFI"), None)
        if executable is None:
            raise SystemExit("Limine binary archive does not contain BOOTX64.EFI.")
        destination = THIRD_PARTY / "limine-binary"
        destination.mkdir(parents=True, exist_ok=True)
        for item in executable.parent.iterdir():
            target = destination / item.name
            if item.is_dir():
                shutil.copytree(item, target, dirs_exist_ok=True)
            else:
                shutil.copy2(item, target)


def bootstrap_assets() -> None:
    clone_if_missing("https://github.com/dhepper/font8x8.git", THIRD_PARTY / "font8x8")
    require_file(THIRD_PARTY / "font8x8" / "font8x8_basic.h", "font8x8 basic header is absent")


def bootstrap_nasm() -> None:
    destination = THIRD_PARTY / "tools" / "nasm-3.02" / "nasm.exe"
    if destination.is_file():
        print(f"Already available: {destination}")
        return
    url = "https://www.nasm.us/pub/nasm/releasebuilds/3.02/win64/nasm-3.02-win64.zip"
    with tempfile.TemporaryDirectory(prefix="kos-nasm-") as temporary:
        archive = Path(temporary) / "nasm.zip"
        print(f"Downloading {url}")
        urllib.request.urlretrieve(url, archive)
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(temporary)
        source = next(Path(temporary).rglob("nasm.exe"), None)
        if source is None:
            raise SystemExit("NASM archive does not contain nasm.exe.")
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def build_initramfs(kernel_elf: Path) -> Path:
    readme = ROOT / "assets" / "initramfs" / "README.TXT"
    require_file(readme, "Initramfs source is absent")
    require_file(kernel_elf, "Kernel ELF is absent")
    archive = bytearray()

    def append_entry(path: str, contents: bytes, mode: int) -> None:
        name = path.encode("ascii")
        fields = (1, mode, 0, 0, 1, 0, len(contents), 0, 0, 0, 0, len(name) + 1, 0)
        archive.extend(("070701" + "".join(f"{field:08X}" for field in fields)).encode("ascii"))
        archive.extend(name)
        archive.append(0)
        archive.extend(b"\0" * ((-len(archive)) % 4))
        archive.extend(contents)
        archive.extend(b"\0" * ((-len(archive)) % 4))

    append_entry("README.TXT", readme.read_bytes(), 0o100644)
    append_entry("KOS.ELF", kernel_elf.read_bytes(), 0o100644)
    append_entry("TRAILER!!!", b"", 0)
    output = BUILD / "initramfs.cpio"
    output.write_bytes(archive)
    return output


def build(kernel_only: bool, panic_test: bool) -> None:
    protocol_include = THIRD_PARTY / "limine-protocol" / "include"
    font_include = THIRD_PARTY / "font8x8"
    limine_binary = THIRD_PARTY / "limine-binary" / "BOOTX64.EFI"
    require_file(protocol_include / "limine.h", "Limine protocol header is absent; run bootstrap-limine")
    require_file(font_include / "font8x8_basic.h", "font asset is absent; run bootstrap-assets")

    clang = find_tool("Clang", ("clang",), host_paths(r"C:\Program Files\LLVM\bin\clang.exe"))
    lld = find_tool("LLD", ("ld.lld",), host_paths(r"C:\Program Files\LLVM\bin\ld.lld.exe"))
    nasm = find_tool("NASM", ("nasm",), host_paths(
        r"C:\Program Files\NASM\nasm.exe", str(THIRD_PARTY / "tools" / "nasm-3.02" / "nasm.exe")))
    readobj = find_tool("llvm-readobj", ("llvm-readobj",), host_paths(r"C:\Program Files\LLVM\bin\llvm-readobj.exe"))
    nm = find_tool("llvm-nm", ("llvm-nm",), host_paths(r"C:\Program Files\LLVM\bin\llvm-nm.exe"))
    objects_dir = BUILD / "obj"
    objects_dir.mkdir(parents=True, exist_ok=True)
    flags: list[str | Path] = [
        "--target=x86_64-unknown-none-elf", "-std=c17", "-ffreestanding", "-fno-stack-protector",
        "-fno-pic", "-fno-pie", "-mno-red-zone", "-mcmodel=kernel", "-mno-mmx", "-mno-sse",
        "-mno-80387", "-ffunction-sections", "-fdata-sections", "-fno-common",
        "-fno-asynchronous-unwind-tables", "-fno-unwind-tables", "-Wall", "-Wextra", "-Werror",
        "-O2", "-g", "-I", ROOT / "include", "-I", protocol_include, "-I", font_include,
    ]
    if panic_test:
        flags.append("-DKOS_PANIC_TEST")
    objects: list[Path] = []
    for relative in C_SOURCES:
        source = ROOT / relative
        require_file(source, "Kernel C source is absent")
        object_file = objects_dir / (relative.replace("/", "__").replace(".c", ".o"))
        run([clang, *flags, "-c", source, "-o", object_file])
        objects.append(object_file)
    for relative in ASM_SOURCES:
        source = ROOT / relative
        require_file(source, "Kernel Assembly source is absent")
        object_file = objects_dir / (relative.replace("/", "__").replace(".asm", ".o"))
        run([nasm, "-f", "elf64", source, "-o", object_file])
        objects.append(object_file)
    kernel_elf = BUILD / "kos.elf"
    run([lld, "-m", "elf_x86_64", "-nostdlib", "-static", "-z", "max-page-size=0x1000",
         "--gc-sections", "--build-id=none", "--fatal-warnings", "-T", ROOT / "boot" / "linker.ld",
         "-o", kernel_elf, *objects])
    headers = run([readobj, "--program-headers", kernel_elf], capture=True)
    if headers.count("Type: PT_LOAD") != 4 or headers.count("PF_X") != 1 or headers.count("PF_W") != 1:
        raise SystemExit("Kernel ELF segment verification failed.")
    symbols = run([nm, kernel_elf], capture=True)
    for symbol in ("_start", "kernel_main", "kos_boot_stack_top"):
        if not any(line.rstrip().endswith(f" {symbol}") for line in symbols.splitlines()):
            raise SystemExit(f"Required boot symbol is absent: {symbol}")
    print(f"Kernel ELF built: {kernel_elf}")
    if kernel_only:
        return
    require_file(limine_binary, "Limine UEFI binary is absent; run bootstrap-limine")
    esp = BUILD / "esp"
    efi_boot = esp / "EFI" / "BOOT"
    boot = esp / "boot"
    efi_boot.mkdir(parents=True, exist_ok=True)
    boot.mkdir(parents=True, exist_ok=True)
    initramfs = build_initramfs(kernel_elf)
    shutil.copy2(limine_binary, efi_boot / "BOOTX64.EFI")
    shutil.copy2(ROOT / "boot" / "limine.conf", efi_boot / "limine.conf")
    shutil.copy2(kernel_elf, boot / "kos.elf")
    shutil.copy2(initramfs, boot / "initramfs.cpio")
    print(f"UEFI ESP staged: {esp}")


def check_environment() -> None:
    requirements = (
        ("Git", ("git",), ()),
        ("Clang", ("clang",), host_paths(r"C:\Program Files\LLVM\bin\clang.exe")),
        ("LLD", ("ld.lld",), host_paths(r"C:\Program Files\LLVM\bin\ld.lld.exe")),
        ("NASM", ("nasm",), host_paths(r"C:\Program Files\NASM\nasm.exe", str(THIRD_PARTY / "tools" / "nasm-3.02" / "nasm.exe"))),
        ("QEMU", ("qemu-system-x86_64",), host_paths(r"C:\Program Files\qemu\qemu-system-x86_64.exe")),
    )
    missing = []
    for description, names, candidates in requirements:
        try:
            print(f"[OK] {description}: {find_tool(description, names, candidates)}")
        except SystemExit:
            print(f"[MISSING] {description}")
            missing.append(description)
    if missing:
        raise SystemExit("Missing required host tools: " + ", ".join(missing))


def launch_qemu(memory_mb: int, display: bool, gdb_wait: bool) -> None:
    esp = BUILD / "esp"
    require_file(esp / "boot" / "kos.elf", "UEFI ESP is absent; run build")
    qemu = find_tool("QEMU", ("qemu-system-x86_64",), host_paths(r"C:\Program Files\qemu\qemu-system-x86_64.exe"))
    firmware = Path(r"C:\Program Files\qemu\share\edk2-x86_64-code.fd")
    variables_template = Path(r"C:\Program Files\qemu\share\edk2-i386-vars.fd")
    require_file(firmware, "QEMU UEFI firmware is absent")
    require_file(variables_template, "QEMU UEFI variable template is absent")
    variables = BUILD / "edk2-vars.fd"
    if not variables.exists():
        shutil.copy2(variables_template, variables)
    command: list[str | Path] = [
        qemu, "-machine", "q35", "-m", str(memory_mb),
        "-drive", f"if=pflash,format=raw,readonly=on,file={firmware}",
        "-drive", f"if=pflash,format=raw,file={variables}",
        "-drive", f"format=raw,file=fat:rw:{esp}", "-serial", "stdio", "-monitor", "none",
        "-no-reboot", "-display", "default" if display else "none",
    ]
    if gdb_wait:
        command.extend(("-S", "-s"))
    run(command)


def main() -> None:
    parser = argparse.ArgumentParser(description="KOS development tool")
    commands = parser.add_subparsers(dest="command", required=True)
    limine = commands.add_parser("bootstrap-limine")
    limine.add_argument("--skip-binary", action="store_true")
    commands.add_parser("bootstrap-assets")
    commands.add_parser("bootstrap-nasm")
    commands.add_parser("check")
    build_parser = commands.add_parser("build")
    build_parser.add_argument("--kernel-only", action="store_true")
    build_parser.add_argument("--panic-test", action="store_true")
    run_parser = commands.add_parser("run")
    run_parser.add_argument("--memory-mb", type=int, default=256, choices=range(64, 4097))
    run_parser.add_argument("--display", action="store_true")
    run_parser.add_argument("--gdb-wait", action="store_true")
    arguments = parser.parse_args()
    if arguments.command == "bootstrap-limine":
        bootstrap_limine(arguments.skip_binary)
    elif arguments.command == "bootstrap-assets":
        bootstrap_assets()
    elif arguments.command == "bootstrap-nasm":
        bootstrap_nasm()
    elif arguments.command == "check":
        check_environment()
    elif arguments.command == "build":
        build(arguments.kernel_only, arguments.panic_test)
    elif arguments.command == "run":
        launch_qemu(arguments.memory_mb, arguments.display, arguments.gdb_wait)


if __name__ == "__main__":
    main()
