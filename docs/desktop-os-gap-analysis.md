# KOS core-kernel gap analysis

## Honest current position

KOS is a bootable x86_64 kernel foundation, not yet a general-purpose desktop
operating system. It has working early-kernel mechanisms: UEFI boot, a serial
and framebuffer debug console, CPU exception reporting, physical and virtual
memory primitives, a kernel heap, legacy timer/keyboard interrupts, a bounded
kernel terminal, PCI discovery, cooperative kernel tasks, and a read-only CPIO
initramfs.

That is a sound start. It is not enough to run ordinary applications, use a
real disk, connect to a network, present a desktop, or safely isolate users.
The project must not describe an inspection-only ELF parser as an application
loader, a framebuffer text console as a GUI, or PCI enumeration as a driver
ecosystem.

## Core-kernel capability audit

This audit intentionally excludes individual hardware drivers (GPU, USB, NIC,
audio, storage-controller drivers). It evaluates only the kernel mechanisms
that those drivers and future user applications need.

| Core capability | KOS status | What is still required |
| --- | --- | --- |
| Boot/init kernel, serial logging, panic | Implemented | Init process, boot-service handoff rules, recovery boot policy, and crash persistence. |
| CPU exceptions, IDT, legacy timer interrupt | Implemented foundation | ACPI tables, Local APIC/IOAPIC, MSI/MSI-X, watchdog, NMI policy, and SMP bring-up. |
| Multicore CPU management | Missing | CPU discovery, AP startup, per-CPU data/stacks, TLB shootdown, atomic scheduling, and CPU hotplug policy. |
| Kernel thread/context switch | Cooperative implementation | Interrupt-return preemption, per-thread states, FPU/SIMD state, sleep/wait states, priorities, and CPU affinity. |
| Scheduler | Cooperative round-robin only | Preemption, fairness, deadlines, idle task, load balancing, priority inheritance, and resource accounting. |
| PMM, paging, heap | Implemented foundation | Per-process page tables, `mmap`, shared memory, copy-on-write, page cache, guard pages, ASLR, and memory limits. |
| Process model | Missing | Process IDs, parent/child relation, handles, exit/wait, process table, executable image ownership, and quotas. |
| User/kernel boundary | Missing | Ring 3 entry, TSS `rsp0`, user pointer validation, user-fault containment, NX/SMEP/SMAP policy, and ASLR. |
| Syscall ABI | Missing | `syscall`/`sysret` or controlled `int` entry, ABI versioning, validation, error model, and tracing. |
| IPC, events, signals | Missing | Pipes/channels, event objects, wakeups, signals, cancellation, and wait-many semantics. |
| Mutex/semaphore/spinlock | IRQ-safe spinlock implemented | Sleeping mutexes, semaphores, condition variables, lock ordering, priority inheritance, and deadlock diagnostics. |
| Identity, permissions, sandbox | Missing | User/group IDs, capabilities, ACL policy, process credentials, namespaces, containers, and resource quotas. |
| VFS and mounts | Read-only single initramfs mount | File descriptors/handles, directories, mount namespace, path resolver, buffer/page cache, writable policy, and mount/unmount. |
| Block abstraction, partitions | Missing | Generic block request API, GPT/MBR parser, partition objects, I/O queueing, and cache/buffer layer. |
| Filesystem implementations | CPIO reader only | FAT32 first; later a writable native filesystem with crash-recovery design. |
| Networking core | UDP loopback socket core | Packet buffers, Ethernet/ARP, IPv4/IPv6, routing, external UDP, TCP, DNS/DHCP, firewall, and network namespaces. |
| Executable loading | Validated ELF inspection only | ELF `PT_LOAD` mapping, user stack, process start/exit, PE parsing only if compatibility work begins. |
| Dynamic linking | Missing | Dynamic loader is primarily user space; kernel supplies mappings, relocations policy, TLS support, and shared-object cache rules. |
| Kernel modules/update/recovery | Missing | Signed module format, dependency/lifetime management, safe update/rollback, recovery image, and compatibility policy. |
| Time, random, logging, tracing | Timer/logging implemented | RTC/UTC/timezone policy, entropy pool/cryptographic RNG, persistent logs, crash dump, tracing, profiling, and audit log. |
| System information API | Implemented in kernel | Expose it through a validated user syscall after Ring 3 exists. |

## Required implementation order

### Release A — safe multitasking core

1. Replace cooperative-only scheduling with interrupt-return preemption.
2. Add explicit no-preempt sections and use the implemented IRQ-safe spin locks
   for every preemptible shared kernel structure.
3. Add sleep queues and monotonic deadline timers.
4. Discover ACPI tables, then migrate from PIC/PIT to Local APIC/IOAPIC where
   QEMU support is validated.
5. Add scheduler and lock stress tests that keep keyboard input responsive.

**Acceptance:** two CPU-bound kernel workers are time-sliced without calling
`task_yield`, while the terminal remains responsive and heap/PMM invariants
hold.

### Release B — storage core (without choosing hardware drivers)

1. Define block requests, buffer ownership, completion, and error interfaces.
2. Add an in-memory block-device test backend so the storage core is tested
   independently of any controller driver.
3. Add a GPT/MBR parser and FAT32 read-only mount, then directories and a
   multi-mount VFS.
4. Add a carefully tested writable filesystem path only after block-cache and
   error handling are in place.
5. Connect a real block driver only after the generic kernel storage API passes
   its in-memory tests.

**Acceptance:** the VFS mounts a tested block backend, lists FAT32 directories,
and reads an executable without corrupting the initramfs or kernel memory.

### Release C — protected user applications

1. Add user-visible page permissions and process-specific page tables.
2. Add a Ring 3 transition with a per-process Ring 0 stack in the TSS.
3. Implement only four initial syscalls: `write`, `exit`, `yield`, and
   `uptime`.
4. Validate every user pointer and convert invalid user faults into process
   termination rather than a kernel panic.
5. Replace ELF inspection with a bounded user ELF loader and execute one
   statically linked test program.
6. Move the normal command shell out of the kernel.

**Acceptance:** a Ring 3 `hello` program can print, call `uptime`, exit, and
cannot write kernel memory or crash the kernel with a bad pointer.

### Release D — usable developer OS

1. Add files, pipes, handles, process wait/exit status, and a service manager.
2. Add kernel socket objects, packet buffers, routing, and a small sockets API;
   connect a NIC driver only after those interfaces are tested.
3. Define package signing, updates, system configuration, logging, crash dump,
   and debugger conventions.
4. Publish a versioned SDK and build an example user application and example
   driver in the KOS source tree.

**Acceptance:** a developer can install a signed program, start it as an
unprivileged process, inspect logs, access a network service, and recover from
an application failure without rebooting.

### Release E — graphical desktop

1. Add a composited pixel display server before a full desktop shell.
2. Add input focus, window surfaces, font rendering, clipboard, image loading,
   and accessibility primitives.
3. Add a virtio-gpu path in QEMU, then define real-GPU driver requirements.
4. Add audio only after an event loop and stable user-space IPC exist.

**Acceptance:** multiple unprivileged applications draw independent windows,
receive routed keyboard/mouse input, and recover when one application exits.

### Release F — application compatibility

Windows compatibility is a user-space product after Release D/E, not a kernel
feature. A practical path is a Wine-like compatibility subsystem: PE loading,
selected Win32 APIs, registry/configuration compatibility, and translation to
KOS graphics, audio, filesystem, networking, and input services. Office, CAD,
and Adobe-class applications require a far larger API and GPU compatibility
surface than basic utilities. macOS binary compatibility is not a project goal;
its software and platform interfaces are proprietary and hardware-specific.

## Non-negotiable engineering rules

1. Keep the kernel C plus minimal x86_64 Assembly; no application compatibility
   code is placed in kernel mode.
2. Add each subsystem with a QEMU acceptance test and a failure-path test.
3. Do not make a third-party driver ABI stable before storage, DMA, interrupts,
   and memory ownership are tested under preemption.
4. Make real-hardware support a separate validation track from QEMU support.
5. Add security boundaries before network, desktop, packages, or untrusted apps.
