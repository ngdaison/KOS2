# Phases 11 and 12: kernel tasks, VFS, and program-loading path

## Phase 11 — Basic kernel multitasking

### Implemented foundation

KOS has a cooperative, single-core kernel scheduler with:

```text
task table (8 slots)
  -> one bootstrap task
  -> heap-backed 16 KiB stack for each created task
  -> saved callee-saved CPU context
  -> Assembly context switch
  -> round-robin ready-task selection
```

The PIT IRQ invokes `task_timer_tick`, which sets a reschedule request. The
terminal consumes that request at a safe kernel point. Context switches are not
performed inside the interrupt handler, so interrupt stack state is never used
as a task stack.

Two diagnostics demonstrate the result:

```text
KOS> tasktest
tasktest passed: two kernel tasks each yielded 32 times.

KOS> taskdemo
task-A count: 1
task-B count: 1
...
task-A count: 5
task-B count: 5
```

Keyboard IRQ remains enabled throughout `taskdemo`; the terminal consumes its
queued input after the demo returns.

### Phase 11B plan: preemptive scheduling

Do not enable preemption until all of these are complete:

1. Save/restore the complete interrupt-return context instead of only the
   callee-saved C ABI context.
2. Move scheduling to the interrupt-return path so a task cannot resume using
   another task's interrupt frame.
3. Add per-task guard pages through VMM-backed stack allocation.
4. Add IRQ-safe locks and an explicit no-preempt critical-section counter.
5. Add `BLOCKED` and `SLEEPING` states with timer wake queues.
6. Test a CPU-bound task that never calls `task_yield`, plus concurrent keyboard
   input and timer uptime.

Only then should the timer enforce quanta on arbitrary kernel code. SMP is not
part of this milestone.

## Phase 12 — Files and KOS programs

### 12A implemented: VFS and initramfs

The first filesystem is a read-only CPIO `newc` initramfs supplied to the
kernel as a Limine module. The build creates it from `assets/initramfs/`, stages
it as `boot/initramfs.cpio`, and the kernel validates the archive before mounting
it as the VFS root.

The VFS interface deliberately contains only the stable read-only operations:

```c
mount_root(backend)
read_file(path, data, size)
file_count()
file_at(index)
```

Current commands:

```text
KOS> ls
/
README.TXT
KOS.ELF

KOS> cat README.TXT
```

`KOS.ELF` is included solely to test the parser; it is the kernel image, not a
user application.

### 12B next: FAT32 backend

1. Add block-device abstraction before exposing ATA/AHCI or virtio-blk.
2. Mount FAT32 as a second VFS backend, initially read-only.
3. Support path lookup and directories before write support.
4. Keep initramfs as the immutable recovery root while FAT32 is under test.
5. Add a VFS mount table rather than replacing the root-backend pointer.

### 12C next: user-mode virtual memory

1. Add page-table flag `USER` and isolated user address spaces.
2. Map kernel pages supervisor-only; map program code RX, data RW+NX, user stack
   RW+NX, and leave guard pages unmapped.
3. Extend GDT/TSS with Ring 3 code/data selectors and a trusted Ring 0 stack for
   privilege transitions.
4. Use a tiny fixed-address user test program before supporting general ELF.

### 12D next: syscall boundary

1. Begin with a small `int 0x80` or `syscall/sysret` ABI, not both.
2. Validate user pointers, buffer ranges, and lengths on every call.
3. Start with `write`, `exit`, `yield`, and `uptime` only.
4. Route errors through return values; user code must never call kernel helpers
   directly.

### 12E next: actual ELF loader

KOS already validates ELF64 headers and loadable program headers through:

```text
KOS> elfinfo KOS.ELF
```

The real loader must additionally validate segment bounds and alignment, allocate
user pages, copy `PT_LOAD` data, zero BSS, apply page permissions, create the
initial user stack/argv, and enter Ring 3 at the ELF entry point. The current
`elfinfo` parser does **not** execute an ELF image.

### 12F next: shell outside the kernel

After a user ELF can run, move command parsing out of `kernel/terminal.c` into
an initramfs user program named `kos-shell`. The kernel terminal remains only as
a recovery/debug console. This is the point where KOS starts running programs
of its own instead of executing all commands in Ring 0.
