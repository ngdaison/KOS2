# Prompt: implement KOS Phase 3

```text
Implement Phase 3 of KOS: a framebuffer text console for an x86_64 Limine
kernel. Preserve the existing serial boot path and use it for all early errors.

Add a Limine framebuffer request and validate that the first response is a
non-null 32-bit RGB framebuffer with 8-bit red, green, and blue masks. Create a
framebuffer driver that respects width, height, pitch, and channel shifts; it
must draw pixels, fill rectangles, clear the screen, and scroll safely.

Build a kernel console around a public-domain 8x8 bitmap font rendered as 8x16
cells. It must support printable ASCII, newline, carriage return, tab,
backspace, wrapping, a visible underline cursor, screen clear, and scrolling.
Create a logger that always writes to COM1 and mirrors to the framebuffer once
the console initializes. On successful boot, visibly print exactly these lines:
`KOS booting...` and `Framebuffer initialized.`

Do not add keyboard input, command parsing, timer interrupts, memory managers,
user mode, windows, or GUI controls. Build and boot QEMU to verify the serial
output and the visual framebuffer result.
```
