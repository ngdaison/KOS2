# Prompt: implement KOS Phase 6

```text
Implement Phase 6 of KOS: four-level x86_64 virtual-memory management and a
kernel heap. Preserve the known-working Limine boot mappings rather than
rebuilding the complete kernel address space immediately.

Request Limine HHDM information, read CR3, and use HHDM to walk/extend the
active PML4 -> PDPT -> page-directory -> page-table chain. Allocate missing
page tables through the PMM, clear them before use, and invalidate TLB entries
after mappings. Validate that the kernel code, writable data, boot stack, and
framebuffer mappings already supplied by Limine are present.

Reserve a separate higher-half kernel heap virtual window. Map its pages with
present, writable, and NX permissions. Implement 16-byte-aligned `kmalloc` and
checked `kfree` using a first-fit linked free list. Split oversized blocks and
coalesce adjacent free blocks. Do not use libc or host allocators.

Add a boot self-test that allocates a small block and a 9,000-byte block,
writes/reads values crossing page boundaries, frees the blocks, and verifies
that no active heap allocations remain. Boot in QEMU and report the observed
serial result. Do not add user mode, process address spaces, demand paging, or
filesystem work.
```
