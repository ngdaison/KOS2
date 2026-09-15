# Coding conventions

## General

- Use English for identifiers, code comments, logs, commit messages, and docs.
- Use C17-style C without hosted-library assumptions.
- Keep hardware-specific code under `arch/x86_64` or `drivers`.
- Put public declarations in `include/kos`; avoid leaking private globals.
- Prefer fixed-width integer types and explicit units in names, such as `size_bytes`.

## Safety rules

- No libc, exceptions, RTTI, or implicit heap allocation.
- Check every address range against the boot memory map before it enters a memory manager.
- Interrupt handlers must acknowledge their interrupt controller source before returning.
- Avoid dynamic allocation inside interrupt handlers.
- Every unrecoverable condition calls `panic` with a diagnostic message.

## Testing rule

Each subsystem earns a QEMU test case and a serial-log expectation before the
next subsystem is introduced.
