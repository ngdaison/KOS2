# Phases 7–11: interrupt, driver, and task foundation

This document records the optimized runtime foundation added after KOS v0.1.
It keeps the boot-time design small while creating explicit boundaries for
future device drivers and user-space work.

## Execution path

```text
CPU interrupt
  -> IDT Assembly stub
  -> interrupt_dispatch
  -> per-IRQ registry handler
  -> PIC EOI

PS/2 IRQ1 -> keyboard ring buffer -> terminal -> command registry
PIT IRQ0  -> 64-bit ticks -> scheduler reschedule request
```

No handler performs logging, heap allocation, command parsing, or blocking I/O.

## Phase 7: hardened PIC/PIT interrupts

- The PIC remains remapped to vectors 32–47; IRQ0 is the 100 Hz PIT timer and
  IRQ1 is the PS/2 keyboard.
- `interrupt_dispatch` owns the PIC acknowledgement and dispatch statistics.
- Drivers use `irq_register_handler` rather than adding conditionals to the
  core dispatcher.
- Spurious IRQ7 and IRQ15 are detected from the PIC ISR. IRQ15 sends only the
  required master EOI; IRQ7 gets no EOI.
- `timer_ticks` is the monotonic 64-bit clock source. Timer IRQ asks the
  cooperative scheduler to reschedule, but never switches stacks in IRQ context.

Useful terminal diagnostic:

```text
KOS> irqinfo
```

## Phase 8: PS/2 keyboard

- Controller setup is bounded by spin limits and enables first-port IRQ plus
  set-1 translation only after local state is initialized.
- One IRQ producer and one terminal consumer use a static 256-byte ring buffer.
- A modifier bitmask tracks left and right Shift independently, so releasing
  one Shift key cannot clear the other.
- Unsupported extended keys are consumed safely. USB HID, layouts, Caps Lock,
  repeat policy, and Unicode remain future input-stack work.

Useful terminal diagnostic:

```text
KOS> kbdinfo
```

## Phase 9: kernel debug terminal

The terminal remains kernel-resident and debug-only. It has a bounded 256-byte
line editor, echo, Backspace, a command table, and serial/framebuffer output.

Base commands: `help`, `clear`, `meminfo`, `uptime`, `echo`, `reboot`.

Runtime diagnostics: `irqinfo`, `kbdinfo`, `drivers`, `lspci`, `taskinfo`, and
`tasktest`.

## Phase 10: driver framework and PCI discovery

`struct driver` provides the first stable contract:

```c
name, init, shutdown, optional IRQ handler, state
```

The boot coordinator registers serial, framebuffer, PIT, PS/2 keyboard, and
PCI. Existing early-boot hardware is adopted by the registry rather than being
reinitialized unsafely.

The x86 PCI implementation uses configuration mechanism #1 (`0xCF8/0xCFC`),
walks PCI buses reachable from bus zero, records vendor/device identity,
class/subclass/programming interface, interrupt routing, and BAR snapshots.
It never enables bus mastering, rewrites BARs, or binds a device automatically.

```text
KOS> drivers
KOS> lspci
```

The next driver-framework increment must add per-device ownership, MMIO mapping,
DMA constraints, IRQ resource claims, and virtio driver probing before external
drivers are accepted.

## Phase 11: kernel-thread foundation

The current scheduler is intentionally **cooperative**. It has a fixed task
table, separate heap-backed 16 KiB stacks, task states, saved callee-saved CPU
context, round-robin selection, and an Assembly context switch. `tasktest`
creates two independent kernel threads, each yielding 32 times, then verifies
that control returns to the terminal task.

```text
KOS> tasktest
tasktest passed: two kernel tasks each yielded 32 times.
```

The PIT records a reschedule request, which is consumed at safe kernel points.
The kernel does **not** yet preempt arbitrary code from inside an interrupt;
that requires an interrupt-return-aware context switch, per-task guard pages,
locking rules, sleep queues, and a careful IRQ-safe scheduler audit. Those are
the next Phase 11 increment, before user mode or executable loading.

## Verified QEMU result

The test guest booted to `KOS> `, discovered four QEMU PCI devices, reported
timer/keyboard IRQ dispatches without unhandled counts, and passed the two-task
context-switch test. `reboot` is intentionally excluded from automated tests.
