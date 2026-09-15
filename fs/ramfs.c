#include <stdbool.h>
#include <stdint.h>

#include <kos/ramfs.h>

static const uint8_t readme[] =
    "KOS Data volume\n"
    "This is an independent in-memory D: volume.\n"
    "It proves the VFS volume namespace before a disk/FAT32 backend is added.\n";

static const struct vfs_file files[] = {
    { .path = "README.TXT", .data = readme, .size = sizeof(readme) - 1 },
};

static bool initialized;

static bool strings_equal(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == *right;
}

static bool ramfs_read_file(const char *path, const uint8_t **data, uint64_t *size) {
    if (!initialized || path == 0 || data == 0 || size == 0) {
        return false;
    }
    for (uint64_t index = 0; index < sizeof(files) / sizeof(files[0]); ++index) {
        if (strings_equal(path, files[index].path)) {
            *data = files[index].data;
            *size = files[index].size;
            return true;
        }
    }
    return false;
}

static uint64_t ramfs_file_count(void) {
    return initialized ? sizeof(files) / sizeof(files[0]) : 0;
}

static const struct vfs_file *ramfs_file_at(uint64_t index) {
    return initialized && index < sizeof(files) / sizeof(files[0]) ? &files[index] : 0;
}

const struct vfs_backend ramfs_vfs_backend = {
    .read_file = ramfs_read_file,
    .file_count = ramfs_file_count,
    .file_at = ramfs_file_at,
};

bool ramfs_initialize(void) {
    if (initialized) {
        return false;
    }
    initialized = true;
    return true;
}
