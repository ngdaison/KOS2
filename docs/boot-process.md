# Planned boot process

This document describes the intended flow for Phase 2 onward. It records the
contract for the code before that code exists.

```text
QEMU
  -> UEFI firmware
    -> Limine boot manager
      -> loads the KOS ELF kernel
        -> architecture entry stub establishes the initial stack
          -> kernel_main receives boot information
            -> serial logger
            -> framebuffer console
            -> CPU tables and exception handlers
            -> memory managers
            -> timer and keyboard drivers
            -> kernel debug console
```

## Boot information required from Limine

- Framebuffer location, dimensions, pitch, and pixel format.
- UEFI memory map with usable and reserved regions.
- Kernel physical/virtual addresses and loaded image bounds.
- HHDM offset if the selected Limine protocol exposes one.

## Early constraints

Before paging and the heap are explicitly initialized, code must not assume that
dynamic memory allocation, normal C runtime initialization, or device drivers
exist. Early logs must use a static serial implementation.
