#ifndef KOS_RAMFS_H
#define KOS_RAMFS_H

#include <stdbool.h>

#include <kos/vfs.h>

bool ramfs_initialize(void);
extern const struct vfs_backend ramfs_vfs_backend;

#endif
