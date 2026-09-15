#include <stdbool.h>
#include <stdint.h>

#include <kos/vfs.h>

static const struct vfs_backend *root_backend;

bool vfs_mount_root(const struct vfs_backend *backend) {
    if (backend == 0 || backend->read_file == 0 || backend->file_count == 0 || backend->file_at == 0
        || root_backend != 0) {
        return false;
    }
    root_backend = backend;
    return true;
}

bool vfs_is_initialized(void) {
    return root_backend != 0;
}

bool vfs_read_file(const char *path, const uint8_t **data, uint64_t *size) {
    return root_backend != 0 && root_backend->read_file(path, data, size);
}

uint64_t vfs_file_count(void) {
    return root_backend != 0 ? root_backend->file_count() : 0;
}

const struct vfs_file *vfs_file_at(uint64_t index) {
    return root_backend != 0 ? root_backend->file_at(index) : 0;
}
