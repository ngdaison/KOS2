# Phase 4-6 optimization pass

## CPU exceptions

KOS now generates a dedicated IDT stub for every CPU vector. Diagnostics retain
the true vector even for an unplanned exception, and vectors that receive a CPU
error code preserve the ABI-normalized stack frame. A panic re-entry latch
protects the emergency reporting path from recursion.

## Physical memory

The PMM still owns only `LIMINE_MEMMAP_USABLE` frames below its explicit 64 GiB
bootstrap ceiling. Initialization marks bitmap ranges 64 bits at a time, while
allocation finds the next free set bit with CPU bit operations. The boot
self-test allocates 65 distinct frames, crossing a bitmap-word boundary, then
verifies the original free-frame count returns.

## Virtual memory and heap

VMM rejects unsupported mapping flags, validates page alignment, rolls back
new intermediate tables when a map cannot complete, and prevents public unmap
of bootstrap mappings. Heap allocation now checks integer-overflow boundaries;
its grow rollback uses its exact count of successfully mapped pages, and `kfree`
rejects addresses outside the heap before interpreting a metadata header.

## Shared contract

`KOS_PAGE_SIZE` is defined once in `include/kos/memory.h` and consumed by PMM,
VMM, and heap. This prevents a future edit from silently giving the allocator
and page-table code different mapping granularities.
