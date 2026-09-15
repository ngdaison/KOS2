#ifndef KOS_VFS_H
#define KOS_VFS_H

#include <stdbool.h>
#include <stdint.h>

struct vfs_file {
    const char *path;
    const uint8_t *data;
    uint64_t size;
};

struct vfs_backend {
    bool (*read_file)(const char *path, const uint8_t **data, uint64_t *size);
    uint64_t (*file_count)(void);
    const struct vfs_file *(*file_at)(uint64_t index);
};

bool vfs_mount_root(const struct vfs_backend *backend);
bool vfs_is_initialized(void);
bool vfs_read_file(const char *path, const uint8_t **data, uint64_t *size);
uint64_t vfs_file_count(void);
const struct vfs_file *vfs_file_at(uint64_t index);

#endif
