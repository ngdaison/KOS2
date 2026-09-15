# Phase 6: virtual memory and kernel heap

## Objective

Take controlled ownership of new kernel virtual mappings without replacing the
working Limine bootstrap address space. KOS keeps Limine's mappings for the
loaded kernel, boot stack, and framebuffer, validates that each is present,
then maps a dedicated kernel heap window with read/write and no-execute (NX)
permissions.

## Four-level page-table design

KOS reads the active CR3 root and uses Limine's Higher Half Direct Map (HHDM)
to access physical page-table frames. To map a new virtual page it walks:

```text
PML4 -> PDPT -> page directory -> page table -> PTE
```

Missing intermediate tables are allocated through the PMM, zeroed through
HHDM, and connected with present/write permissions. Heap PTEs use `PRESENT |
WRITABLE | NX`, so data allocated from `kmalloc` is not executable. `invlpg`
flushes the local TLB entry after every new mapping or unmapping. Page-table
queries calculate effective write/NX permissions across every page-table level,
not merely from the final PTE.

Mapping is transactional: if a walk cannot reach a new leaf, any intermediate
tables allocated for that attempt are released. KOS tags its private leaf and
table mappings with software-reserved page-table bits; only a KOS-created leaf
can be unmapped through the public API. This prevents a heap or later subsystem
from accidentally tearing down a Limine bootstrap mapping.

KOS can also adjust the permissions at a 4 KiB, 2 MiB, or 1 GiB leaf mapping
without disturbing its physical address or caching attributes. At boot it uses
this to harden the framebuffer mapping to `WRITABLE | NX`; some Limine/QEMU
bootstrap configurations expose that mapping as a writable 2 MiB leaf without
NX by default.

The initial heap window begins at `0xffffffffc0000000` and can grow to 128 MiB.
It is deliberately separate from the kernel image at `0xffffffff80000000` and
from the HHDM region.

## Kernel heap design

`kmalloc` is a 16-byte-aligned, first-fit linked-block allocator. It grows the
virtual heap by mapping PMM frames, splits large free blocks, and coalesces
adjacent blocks after `kfree`. `kfree` rejects null, unknown, and already-free
pointers. When coalescing produces a page-aligned free block at the end of the
heap, KOS unmaps its pages, returns their frames to the PMM, and also releases
now-empty page-table frames that KOS created. This prevents temporary heap use
from permanently consuming physical memory.

## Boot verification

The normal boot validates that Limine mapped the kernel code, data probe, boot
stack, and framebuffer. It also checks effective permissions: code is
executable but not writable; kernel data, BSS, stack, and framebuffer are
writable and NX. It then allocates 64 bytes and 9,000 bytes, writes and reads
values across multiple pages, frees both allocations, and confirms both the
heap's mapped-page count and the PMM free-frame count return to their initial
values.
The heap self-test additionally verifies the first and last pages used by its
allocations have effective `WRITABLE | NX` permissions.

`kmalloc` checks alignment and size arithmetic for overflow. `kfree` first
validates that a supplied address lies inside the active heap window before
walking metadata, rejecting arbitrary low or out-of-window pointers safely.

QEMU verified:

```text
Four-level virtual memory manager initialized.
Kernel heap self-test and page reclamation passed.
```

`meminfo` then confirms the temporary heap data and its empty page-table
metadata have returned to the PMM.

## Boundaries

This phase does not yet replace Limine's complete page table, map user-space
processes, enable demand paging, or implement copy-on-write. Those require
later process and scheduler work.
