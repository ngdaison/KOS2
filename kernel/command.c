#include <stdbool.h>

#include <kos/command.h>
#include <kos/console.h>
#include <kos/cpu.h>
#include <kos/driver.h>
#include <kos/elf.h>
#include <kos/interrupts.h>
#include <kos/keyboard.h>
#include <kos/log.h>
#include <kos/pci.h>
#include <kos/pic.h>
#include <kos/pmm.h>
#include <kos/serial.h>
#include <kos/timer.h>
#include <kos/task.h>
#include <kos/vfs.h>

typedef void (*command_handler)(const char *arguments);

struct command_entry {
    const char *name;
    command_handler handler;
};

static void command_help(const char *arguments) {
    (void)arguments;
    log_info("help clear meminfo uptime echo reboot irqinfo kbdinfo drivers lspci taskinfo tasktest taskdemo ls cat elfinfo");
}

static void command_clear(const char *arguments) {
    (void)arguments;
    console_clear();
    serial_write("\x1b[2J\x1b[H");
}

static void command_meminfo(const char *arguments) {
    (void)arguments;
    log_info("meminfo");
    log_info_u64("  Total RAM bytes: ", pmm_total_memory_bytes());
    log_info_u64("  Usable RAM bytes: ", pmm_usable_memory_bytes());
    log_info_u64("  PMM tracked bytes: ", pmm_tracked_memory_bytes());
    log_info_u64("  Kernel/modules reserved bytes: ", pmm_kernel_memory_bytes());
    log_info_u64("  Framebuffer reserved bytes: ", pmm_framebuffer_memory_bytes());
    log_info_u64("  Bootloader reclaimable bytes: ", pmm_bootloader_reclaimable_bytes());
    log_info_u64("  Frame size bytes: ", pmm_frame_size());
    log_info_u64("  Free frames: ", pmm_free_frame_count());
    log_info_u64("  Allocated frames: ", pmm_allocated_frame_count());
}

static void command_uptime(const char *arguments) {
    (void)arguments;
    log_info_u64_suffix("uptime: ", timer_uptime_seconds(), " seconds");
}

static void command_echo(const char *arguments) {
    log_info(arguments);
}

static void command_reboot(const char *arguments) {
    (void)arguments;
    log_info("Rebooting through the PS/2 controller.");
    cpu_reboot();
}

static void command_irqinfo(const char *arguments) {
    (void)arguments;
    log_info("irqinfo");
    log_info_u64("  IRQ0 timer dispatches: ", irq_dispatch_count(KOS_PIC_IRQ_TIMER));
    log_info_u64("  IRQ0 timer unhandled: ", irq_unhandled_count(KOS_PIC_IRQ_TIMER));
    log_info_u64("  IRQ1 keyboard dispatches: ", irq_dispatch_count(KOS_PIC_IRQ_KEYBOARD));
    log_info_u64("  IRQ1 keyboard unhandled: ", irq_unhandled_count(KOS_PIC_IRQ_KEYBOARD));
}

static void command_kbdinfo(const char *arguments) {
    (void)arguments;
    log_info("kbdinfo");
    log_info_u64("  Pending characters: ", keyboard_pending_count());
    log_info_u64("  Dropped characters: ", keyboard_dropped_char_count());
}

static void command_drivers(const char *arguments) {
    (void)arguments;
    log_info("drivers");
    for (uint64_t index = 0; index < driver_count(); ++index) {
        const struct driver *driver = driver_at(index);
        if (driver != 0) {
            log_info(driver->name);
        }
    }
}

static void command_lspci(const char *arguments) {
    (void)arguments;
    log_info("lspci");
    for (uint64_t index = 0; index < pci_device_count(); ++index) {
        const struct pci_device *device = pci_device_at(index);
        if (device == 0) {
            continue;
        }
        uint64_t bdf = ((uint64_t)device->bus << 16) | ((uint64_t)device->device << 8)
            | device->function;
        uint64_t class_info = ((uint64_t)device->class_code << 16)
            | ((uint64_t)device->subclass << 8) | device->programming_interface;
        log_info_hex("  BDF: ", bdf);
        log_info_hex("    Vendor ID: ", device->vendor_id);
        log_info_hex("    Device ID: ", device->device_id);
        log_info_hex("    Class/Subclass/ProgIF: ", class_info);
        log_info_u64("    IRQ line: ", device->interrupt_line);
    }
}

static void command_taskinfo(const char *arguments) {
    (void)arguments;
    log_info("taskinfo");
    log_info_u64("  Known tasks: ", task_count());
    log_info_u64("  Ready tasks: ", task_ready_count());
    log_info("  Scheduler: cooperative round-robin; timer requests reschedule.");
}

static void command_tasktest(const char *arguments) {
    (void)arguments;
    if (task_run_self_test()) {
        log_info("tasktest passed: two kernel tasks each yielded 32 times.");
    }
    else {
        log_error("tasktest failed: scheduler is busy or context switch failed.");
    }
}

static void command_taskdemo(const char *arguments) {
    (void)arguments;
    if (task_run_log_demo()) {
        log_info("taskdemo passed: keyboard IRQ remained enabled while tasks alternated.");
    }
    else {
        log_error("taskdemo failed: scheduler is busy or task creation failed.");
    }
}

static void command_ls(const char *arguments) {
    (void)arguments;
    log_info("/");
    for (uint64_t index = 0; index < vfs_file_count(); ++index) {
        const struct vfs_file *file = vfs_file_at(index);
        if (file != 0) {
            log_info(file->path);
        }
    }
}

static void command_cat(const char *arguments) {
    const uint8_t *data;
    uint64_t size;
    if (*arguments == '\0' || !vfs_read_file(arguments, &data, &size)) {
        log_error("cat: file not found.");
        return;
    }
    for (uint64_t index = 0; index < size; ++index) {
        char character = data[index] >= ' ' && data[index] <= '~' ? (char)data[index]
            : data[index] == '\n' ? '\n' : '?';
        serial_write_char(character);
        console_write_char(character);
    }
    if (size == 0 || data[size - 1] != '\n') {
        serial_write_char('\n');
        console_write_char('\n');
    }
}

static void command_elfinfo(const char *arguments) {
    const uint8_t *data;
    uint64_t size;
    struct elf_image_info info;
    if (*arguments == '\0' || !vfs_read_file(arguments, &data, &size)) {
        log_error("elfinfo: file not found.");
        return;
    }
    if (!elf_inspect_image(data, size, &info)) {
        log_error("elfinfo: not a supported x86_64 ELF image.");
        return;
    }
    log_info_hex("ELF entry: ", info.entry_point);
    log_info_u64("ELF program headers: ", info.program_header_count);
    log_info_u64("ELF loadable segments: ", info.loadable_segment_count);
}

static const struct command_entry command_registry[] = {
    { "help", command_help },
    { "clear", command_clear },
    { "meminfo", command_meminfo },
    { "uptime", command_uptime },
    { "echo", command_echo },
    { "reboot", command_reboot },
    { "irqinfo", command_irqinfo },
    { "kbdinfo", command_kbdinfo },
    { "drivers", command_drivers },
    { "lspci", command_lspci },
    { "taskinfo", command_taskinfo },
    { "tasktest", command_tasktest },
    { "taskdemo", command_taskdemo },
    { "ls", command_ls },
    { "cat", command_cat },
    { "elfinfo", command_elfinfo },
};

bool kernel_command_execute(const char *command) {
    if (command == 0) {
        return false;
    }
    while (*command == ' ') {
        ++command;
    }
    const char *name = command;
    while (*command != '\0' && *command != ' ') {
        ++command;
    }
    uint64_t name_length = (uint64_t)(command - name);
    while (*command == ' ') {
        ++command;
    }

    for (uint64_t index = 0; index < sizeof(command_registry) / sizeof(command_registry[0]); ++index) {
        const char *registered_name = command_registry[index].name;
        uint64_t registered_length = 0;
        while (registered_name[registered_length] != '\0') {
            ++registered_length;
        }
        if (registered_length == name_length) {
            bool matches = true;
            for (uint64_t character = 0; character < name_length; ++character) {
                if (name[character] != registered_name[character]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                command_registry[index].handler(command);
                return true;
            }
        }
    }
    return false;
}
