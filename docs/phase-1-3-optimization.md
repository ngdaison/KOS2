# Phase 1-3 optimization pass

This pass improves the completed early boot path without changing its public
boot protocol or requiring later subsystems.

## Phase 1: reproducible builds and QEMU use

- C compilation now emits one code/data section per symbol, allowing LLD's
  existing garbage collection to remove unreachable code.
- Object filenames include their source directory, avoiding collisions as the
  kernel grows.
- The build checks that the ELF has exactly four load segments, with one
  executable and one writable segment, before staging the UEFI ESP.
- Build output reports only the useful verification result instead of the full
  symbol table.
- `run-qemu.ps1 -Display` opens the framebuffer window. The default continues
  to use serial-only mode, which is better for repeatable debugging.

## Phase 2: controlled early output

The entry path continues to establish a 64 KiB, 16-byte-aligned stack before
calling C. COM1 now tracks initialization, ignores null strings, and uses a
bounded polling loop for every byte. A bad serial device can therefore lose a
diagnostic message, but cannot turn the kernel into an invisible infinite wait.

## Phase 3: framebuffer-console efficiency and validation

Framebuffer initialization now rejects malformed row pitch, invalid 8-bit RGB
channel positions, and overlapping colour masks. The driver writes contiguous
32-bit rows directly when filling or scrolling. The text renderer groups each
glyph row into equal-colour runs, reducing framebuffer calls substantially while
preserving the existing public-domain 8x8 font, 16x16 evenly-scaled cell
geometry, cursor, and ASCII behaviour. Equal horizontal/vertical scaling avoids
the visibly stretched 8x16 appearance of the original bootstrap console.

The font remains a compact bitmap development font, not the eventual desktop UI
font. A scalable UI text system belongs after input, processes, and graphics
compositing exist.
