#ifndef KOS_ELF_H
#define KOS_ELF_H

#include <stdbool.h>
#include <stdint.h>

struct elf_image_info {
    uint64_t entry_point;
    uint16_t program_header_count;
    uint16_t loadable_segment_count;
};

bool elf_inspect_image(const uint8_t *image, uint64_t image_size, struct elf_image_info *info);

#endif
