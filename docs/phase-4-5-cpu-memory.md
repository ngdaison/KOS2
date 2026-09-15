# Phases 4 and 5: CPU safety and physical memory

## Phase 4 objective: controlled CPU failures

The kernel installs its own GDT and IDT before enabling future interrupt-driven
subsystems. The GDT includes a 64-bit TSS and a dedicated IST stack for a
double-fault emergency path. The IDT registers handlers for divide-by-zero,
invalid opcode, page fault, double fault, and all architecturally error-code
exceptions. Every one of the 256 IDT entries has an Assembly stub carrying its
real vector number; error-code vectors use a distinct normalized frame. Each
handler saves the general-purpose register frame in Assembly,
reports the exception name/vector/error code/RIP/RFLAGS to both COM1 and the
framebuffer console, then disables interrupts and halts.

```text
CPU exception
  -> IDT gate
    -> Assembly register-save stub
      -> C panic dispatcher
        -> serial + framebuffer diagnostic
          -> cli; hlt loop
```

The normal kernel build does not intentionally fault. `tools\kos-tool.exe build
--panic-test` enables the deliberate divide-by-zero validation build.

The normal build uses `-O2` while retaining debug symbols. The artificial
divide instruction is specifically marked non-inline/non-optimised so the panic
test remains reliable.

The panic dispatcher has a re-entry latch. If reporting itself faults, KOS
halts immediately instead of recursively corrupting the diagnostic path. Page
fault reporting also decodes reserved-bit, protection-key, shadow-stack, and
SGX-related error flags when supplied by the CPU.

## Phase 5 objective: 4 KiB physical frames

Limine supplies a memory map describing usable RAM, bootloader-owned regions,
the kernel image, framebuffer, ACPI data, and reserved ranges. The physical
memory manager uses two static bitmaps for physical addresses below 64 GiB:

- `usable_bitmap`: frames which originated in a `LIMINE_MEMMAP_USABLE` range.
- `free_bitmap`: currently allocatable usable frames.

All non-usable types remain unavailable. Frame zero is always retained as
invalid. Allocation clears a free bit; freeing restores it only for a tracked,
usable, previously allocated 4 KiB frame.

`meminfo` separately reports Limine's kernel/module, framebuffer, and
bootloader-reclaimable ranges, so these reserved areas remain visible instead
of being confused with allocatable RAM.

The reported total excludes address-space holes and non-RAM reserved mappings;
it includes usable RAM plus RAM occupied by firmware/ACPI, the loaded kernel,
the bootloader, and the framebuffer.

The 64 GiB tracking ceiling keeps the bootstrap manager deterministic without a
heap or virtual-memory allocator. Bitmap range marking and allocation scanning
operate a 64-bit word at a time, using set-bit/popcount CPU operations rather
than visiting every 4 KiB frame. An allocation hint avoids repeatedly scanning
from physical frame zero. A later paging/heap phase can replace this static
bootstrap limit with dynamically allocated metadata.

## Work sequence

1. Load a GDT with null, kernel-code, and kernel-data descriptors.
2. Build an IDT with 256 present gates and explicit handlers for #DE, #UD, and
   #PF.
3. Add structured panic reporting and a `-PanicTest` build switch.
4. Request Limine's memory map.
5. Mark only aligned, usable 4 KiB frames allocatable in the dual bitmap.
6. Test allocate/free/recount during boot without touching physical RAM.
7. Register and run `meminfo`, which logs total, usable, tracked, free, and
   allocated-frame metrics. Keyboard input will invoke this same command later.

## Completion checks

| Check | Expected evidence |
| --- | --- |
| Default boot | CPU tables and PMM initialize; memory metrics appear in serial and framebuffer. |
| Panic build | `KERNEL PANIC`, `Divide-by-zero (#DE)`, and RIP diagnostics appear instead of an invisible hang. |
| PMM self-test | 65 distinct frames are allocated and freed across a bitmap-word boundary; the free-frame count returns to its original value. |
| Reserved memory | No non-`USABLE` memory-map range is marked free. |

`meminfo` is now a registered command and is executed during boot for
verification. It cannot be typed interactively until keyboard/input arrives in
a later phase, but that phase will call the same command registry.

## Verified result

The ordinary QEMU boot completed the PMM self-test and reported:

```text
meminfo
  Total RAM bytes: 267218944
  Usable RAM bytes: 209920000
  PMM tracked bytes: 209915904
  Kernel/modules reserved bytes: 4300800
  Framebuffer reserved bytes: 3145728
  Bootloader reclaimable bytes: 47673344
  Free frames: 51249
  Allocated frames: 0
```

The deliberate `tools\kos-tool.exe build --panic-test` build produced a visible and
serialised `Divide-by-zero (#DE)` panic with the vector, error code, RIP, and
RFLAGS, then halted as designed. The normal non-panic build was restored after
the test.
