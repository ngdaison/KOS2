# Phase 7: PIT timer and hardware interrupts

## Objective

Turn KOS's existing exception-only IDT into a safe interrupt foundation. The
first source is the legacy x86 Programmable Interval Timer (PIT), routed through
the 8259 Programmable Interrupt Controller (PIC). This is deliberately the
smallest path that is reliable in QEMU; Local APIC and SMP timing are later
work.

The phase completes only when IRQ0 arrives at a KOS IDT stub, increments a
monotonic 64-bit tick counter, acknowledges the PIC, returns through `iretq`,
and continues running without a panic.

## Fixed choices

| Item | Choice | Reason |
| --- | --- | --- |
| Interrupt controller | Dual 8259 PIC | Simple, deterministic, available in QEMU. |
| PIC vectors | Master `32-39`, slave `40-47` | Keeps hardware IRQs away from CPU exceptions `0-31`. |
| Timer device | PIT channel 0 | Available before APIC and ACPI work. |
| Rate | 100 Hz | 10 ms granularity with negligible interrupt cost for a debug kernel. |
| Time source | 64-bit tick counter | Does not wrap during realistic kernel testing. |
| Interrupt state | Disabled until all boot self-tests finish | PMM/VMM/heap startup remains deterministic. |

## Boot order

```text
entry.asm: cli
  -> framebuffer + serial
  -> GDT + 256-entry IDT
  -> PMM + VMM + heap self-tests
  -> PIC remap; mask every IRQ
  -> PIT program at 100 Hz
  -> unmask only IRQ0
  -> sti
  -> wait for one second of timer ticks
  -> enter sti; hlt idle loop
```

The keyboard IRQ remains masked until Phase 8. No other device can unexpectedly
enter the interrupt dispatcher during this phase.

## IRQ flow

```text
PIT channel 0
  -> PIC IRQ0
    -> IDT vector 32
      -> isr_stub_32
        -> interrupt_dispatch(context)
          -> timer_interrupt(): ++ticks
          -> pic_send_eoi(0)
        -> iretq
```

Every IDT vector has its own Assembly stub. The common dispatcher distinguishes
PIC vectors (`32-47`) from CPU exceptions. Exceptions preserve the existing
panic path; PIC IRQs return normally after acknowledgement.

## Components and API

| File | Responsibility |
| --- | --- |
| `include/kos/io.h` | Shared x86 port-I/O helpers used by serial, PIC, and PIT. |
| `arch/x86_64/pic.c` | PIC initialization, IRQ masking, and master/slave EOI. |
| `drivers/timer.c` | PIT divisor programming and the monotonic tick API. |
| `arch/x86_64/interrupts.c` | Routes IRQ0 to the timer without affecting exception panic handling. |
| `arch/x86_64/cpu_control.c` | Controlled `sti`, `hlt`, and interrupt-enabled idle loop. |

Public timer API:

```c
bool timer_initialize(uint32_t frequency_hz);
void timer_interrupt(void);
uint64_t timer_ticks(void);
uint64_t timer_uptime_seconds(void);
uint32_t timer_frequency_hz(void);
void timer_wait_ticks(uint64_t ticks);
```

`timer_wait_ticks()` is only used after interrupts are enabled. It sleeps with
`hlt` between ticks, so the Phase 7 self-test does not busy-spin for a second.

## PIC details

PIC initialization sends ICW1–ICW4 to both controllers, maps their vectors to
32 and 40, configures the master/slave cascade line, enters 8086 mode, and then
masks all sixteen IRQs. `pic_unmask_irq(0)` opens only the timer line. For a
slave IRQ in a later phase, KOS automatically opens the master cascade IRQ2 and
sends EOI to slave first, then master.

## Correctness and safety rules

- Timer code only increments a `volatile uint64_t`; it does not log, allocate,
  take locks, or draw on the framebuffer from IRQ context.
- The EOI is sent after the device-specific handler, before returning with
  `iretq`.
- `cpu_halt()` is reserved for panic/error paths and disables interrupts.
  Normal idle uses `sti; hlt`, so IRQ0 continues to wake the CPU.
- The 64-bit tick counter is naturally aligned and atomically read/written on
  this x86_64 uniprocessor target. SMP-safe timekeeping is deferred.
- A timer setup failure keeps interrupts disabled and halts with a visible
  kernel error rather than entering a partially initialized IRQ state.

## Verification

Build and run:

```text
python tools/kos.py build
python tools/kos.py run
```

Observed QEMU serial output includes:

```text
PIT timer interrupts initialized at 100 Hz.
Timer self-test passed; monotonic uptime is running.
Phase 7 timer and hardware interrupt foundation complete.
```

The normal kernel remains in an interrupt-enabled idle loop after this output.
Phase 9 will expose the same `timer_uptime_seconds()` value through the
interactive `uptime` command.

## Deliberately deferred

- PS/2 keyboard IRQ1 and scancode decoding (Phase 8).
- PIC spurious-IRQ checks and dynamic IRQ routing.
- Local APIC, I/O APIC, HPET, TSC calibration, SMP, and scheduler preemption.
- Sleeping queues and timers for tasks; Phase 7 only supplies raw monotonic
  ticks.
