# KOS product vision

## Product goal

KOS is intended to become a self-contained desktop operating system: it boots
its own kernel, runs its own applications, presents a graphical desktop, manages
files and devices, and provides a coherent user experience. The inspiration is
the level of usefulness and polish associated with macOS, Windows, and desktop
Linux distributions.

KOS will be its own system. It must not reuse proprietary source code, branding,
visual assets, or claim direct Windows/macOS/Linux binary compatibility.

## Why the current version is text-only

A graphical desktop depends on foundations that do not exist until the kernel can
reliably handle memory, interrupts, input, storage, processes, and protected
user-space execution. The v0.1 in-kernel command console is a development tool;
it is not the intended final interface.

```text
v0.1 kernel console
  -> user mode + processes + filesystem
    -> graphics/input/window system
      -> desktop shell + graphical applications
```

## High-level desktop architecture

```text
Applications
    |
Application framework and system services
    |
Desktop shell + window server + compositor
    |
User-space graphics, input, storage, network services
    |
System-call interface
    |
KOS kernel: scheduler, memory, drivers, IPC, filesystem primitives
```

The graphics stack, desktop shell, and normal applications should eventually run
in user mode, so an application crash cannot directly corrupt the kernel.

## Early product principles

- Start with a small, reliable feature set and improve it incrementally.
- Keep early interfaces simple enough to document and test in QEMU.
- Design original KOS APIs and visual identity.
- Treat security and crash isolation as core design requirements, not later polish.
- Prefer a usable vertical slice over a large number of unfinished subsystems.
