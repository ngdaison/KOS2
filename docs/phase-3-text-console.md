# Phase 3: framebuffer text console

## Objective

Request a 32-bit RGB framebuffer from Limine, draw an original text console in
the QEMU display, and retain serial output as the independent debug channel.

The visual completion output is:

```text
KOS booting...
Framebuffer initialized.
```

## Design

| Component | Responsibility |
| --- | --- |
| `boot/limine_requests.c` | Requests the framebuffer and exposes the response. |
| `drivers/framebuffer.c` | Validates the framebuffer layout, channel masks, and pitch; safely fills and scrolls 32-bit pixel rows. |
| `kernel/console.c` | Renders ASCII glyph runs, tracks cursor position, wraps, clears, scrolls, and handles control characters. |
| `kernel/log.c` | Mirrors messages to COM1 and the framebuffer console. |
| `third_party/font8x8` | Public-domain 8x8 bitmap glyph data, scaled evenly to 16x16 cells. |

## Work sequence

1. Request and validate a 32-bit RGB Limine framebuffer.
2. Convert RGB values using the channel shifts supplied by Limine.
3. Render public-domain 8x8 glyphs with even 2x scaling to a 16x16 console cell.
   Adjacent pixels of the same colour are drawn as a run rather than by issuing
   one framebuffer operation per source pixel.
4. Draw a two-pixel underline cursor at the active cell.
5. Implement newline, carriage return, tab, backspace, wrapping, clear, and
   pixel-level scrolling.
6. Route log messages to serial first and then to the console when initialized.
7. Boot QEMU and verify both serial and visible framebuffer output.

## Limits of this phase

The console is kernel-only and ASCII-only. It is not a GUI, terminal emulator,
keyboard input system, window manager, font rasterizer, or user-space shell.

## Verification

- The serial terminal includes `KOS booting...` and `Framebuffer initialized.`
- The QEMU display shows the same messages with a visible cursor.
- The framebuffer is validated before any memory writes.
- Scrolling moves pixels only within the reported framebuffer geometry.
- Run `scripts/run-qemu.ps1 -Display` to inspect the framebuffer window; the
  default run remains serial-only for automated debugging.
