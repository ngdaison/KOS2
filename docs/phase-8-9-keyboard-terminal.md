# Phases 8 and 9: PS/2 keyboard and KOS debug terminal

These phases complete `KOS v0.1`: a QEMU-oriented kernel debug console. They
are intentionally small and synchronous; they do not create a user-space shell
or make blocking hardware work happen inside an interrupt.

## Scope and completion boundary

The supported path is an x86_64 QEMU guest with an emulated PS/2 keyboard,
set-1 scancodes, the legacy PIC, and the existing 100 Hz PIT. USB keyboards,
keyboard LEDs, international layouts, key repeat policy, cursor movement and
command history are deliberately deferred.

The phase is complete when the framebuffer and serial console both show:

```text
KOS> help
help clear meminfo uptime echo reboot

KOS> uptime
uptime: 12 seconds
```

and a typed line supports echo and Backspace.

## Design rules

1. IRQ1 does the minimum: read one controller byte, update modifier state,
   translate a make code, enqueue a character, then return.
2. IRQ1 never logs, draws, allocates, parses a command, waits for hardware, or
   executes a command. This bounds interrupt latency and avoids recursive
   console/allocator use.
3. The keyboard producer and terminal consumer communicate through a fixed,
   single-producer/single-consumer 256-byte ring buffer. A full queue drops the
   newest character and records a diagnostic count rather than corrupting data.
4. The terminal runs at normal kernel control flow and sleeps with `hlt` while
   its queue is empty. IRQ0 or IRQ1 wakes it, so it consumes no busy-loop CPU.
5. The command registry remains a small static table. Adding a command is one
   new handler and one registry entry; no string chain needs to grow.

## Phase 8 plan and implementation

### 8.1 PIC route

`KOS_PIC_IRQ_KEYBOARD` is IRQ 1. The PIC is already remapped to vectors 32–47,
so the keyboard arrives on vector 33 without colliding with CPU exceptions. It
is unmasked only after the PS/2 device is ready.

### 8.2 PS/2 controller setup

`keyboard_initialize()` follows this bounded transaction:

1. Reset local ring-buffer and modifier state, then discard stale output bytes.
2. Read the controller configuration byte (`0x20`).
3. Enable first-port interrupts and translation, and ensure the first-port
   clock is enabled.
4. Write that configuration back (`0x60`).
5. Enable the first PS/2 port (`0xAE`), discard any setup residue, and mark the
   driver initialized.

All controller waits have a finite spin limit. Failure returns `false` to the
boot coordinator, which logs an error and halts instead of enabling an unsafe
IRQ route.

### 8.3 IRQ1 path

For each IRQ1, `keyboard_interrupt()` verifies output is ready, reads port
`0x60`, handles `0xE0` extended prefixes, tracks left/right Shift make and
break codes, discards release codes, and translates supported set-1 make codes.
The initial map supports letters, digits, punctuation, Space, Enter, Backspace,
and Shift. Extended keys are intentionally ignored cleanly for this release.

The common interrupt dispatcher calls the keyboard driver then sends PIC EOI.
This ordering means the controller byte has been consumed before another
keyboard interrupt can be acknowledged.

### 8.4 Public keyboard boundary

`include/kos/keyboard.h` exposes only:

- initialization/state,
- the IRQ entry,
- non-blocking `keyboard_read_char`, and
- queue diagnostics.

It does not expose controller ports or scancodes to consumers. This keeps later
PS/2, USB-HID, and layout work behind a stable input boundary.

## Phase 9 plan and implementation

### 9.1 Line editor

`kernel/terminal.c` owns a 256-byte line buffer. Printable ASCII is echoed to
serial and framebuffer; Enter terminates and dispatches the line; Backspace
erases one visible framebuffer character and removes one buffered byte. Input
beyond 255 bytes is ignored so every submitted line remains NUL-terminated.

At initialization, it prints `KOS debug terminal ready.` followed by the exact
prompt `KOS> `. Every completed command, including an unknown or empty one,
ends by printing a fresh prompt.

### 9.2 Command registry

`kernel_command_execute()` trims leading spaces, separates one command name
from its trailing argument string, and looks up a static `{ name, handler }`
table. It returns `false` only when no entry matches. The terminal owns the
`Unknown command.` diagnostic, keeping command handlers reusable.

| Command | Result |
| --- | --- |
| `help` | Lists the six v0.1 commands. |
| `clear` | Clears the framebuffer and sends ANSI clear/home to the serial terminal. |
| `meminfo` | Shows PMM totals, reservations, and free/allocated 4 KiB frames. |
| `uptime` | Formats the monotonic PIT tick source in whole seconds. |
| `echo text` | Prints its trailing text. |
| `reboot` | Requests reset through the PS/2 controller, then safely halts if it cannot reset. |

`reboot` is not part of automated testing because it intentionally changes the
guest's execution state.

## Verification sequence

1. Build with `./scripts/build.ps1`; warnings are errors and the ELF layout is
   rechecked by the build script.
2. Run QEMU with `./scripts/run-qemu.ps1 -Display`.
3. Confirm boot reaches `KOS> ` without a panic and that `uptime` increases.
4. Type `help`, `uptime`, `echo hello`, and `meminfo`; confirm each returns a
   new prompt.
5. Type a deliberate correction such as `echo hellx`, Backspace, `o`, Enter;
   confirm `hello` is printed.
6. Test `clear` visually. Do not run `reboot` during this test session.

## Deferred improvements

The next input iteration can add Caps Lock, LEDs, key repeat, keyboard layouts,
full extended keys, a generic input-event layer, and USB HID. None belongs in
the v0.1 IRQ path.

## After v0.1: driver-developer foundation

Phase 10 should first establish a driver contract rather than immediately add
many device-specific drivers:

1. Define driver lifecycle (`name`, `init`, `shutdown`, optional IRQ hook) and
   a registration registry.
2. Put x86 port I/O, IRQ routing, MMIO mapping, and DMA constraints behind a
   small HAL boundary.
3. Adapt serial, framebuffer, PIT, and PS/2 keyboard to the common contract.
4. Enumerate PCI configuration space safely and add an `lspci` diagnostic
   command with vendor/device/class/subclass/prog-if/BAR data.
5. Target QEMU virtio devices first, then define ownership, probe, error, and
   teardown conventions for third-party driver authors.

Phase 11 follows with kernel task structures, saved CPU contexts, a
round-robin scheduler, timer-driven preemption, separate kernel stacks, and
explicit synchronization rules. User mode, processes, system calls, filesystem,
and executable loading stay after that foundation; Windows application
compatibility is far beyond this kernel milestone.
