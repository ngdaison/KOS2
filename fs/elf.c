#include <stdbool.h>
#include <stdint.h>

#include <kos/elf.h>

enum {
    ELF_HEADER_SIZE = 64,
    ELF_PROGRAM_HEADER_MIN_SIZE = 56,
    ELF_CLASS_64 = 2,
    ELF_LITTLE_ENDIAN = 1,
    ELF_MACHINE_X86_64 = 0x3e,
    ELF_TYPE_EXECUTABLE = 2,
    ELF_TYPE_SHARED = 3,
    ELF_PROGRAM_TYPE_LOAD = 1,
};

static uint16_t read_u16(const uint8_t *address) {
    return (uint16_t)address[0] | ((uint16_t)address[1] << 8);
}

static uint32_t read_u32(const uint8_t *address) {
    return (uint32_t)address[0] | ((uint32_t)address[1] << 8) | ((uint32_t)address[2] << 16)
        | ((uint32_t)address[3] << 24);
}

static uint64_t read_u64(const uint8_t *address) {
    uint64_t value = 0;
    for (uint64_t index = 0; index < 8; ++index) {
        value |= (uint64_t)address[index] << (index * 8);
    }
    return value;
}

static bool range_is_valid(uint64_t offset, uint64_t length, uint64_t total_size) {
    return offset <= total_size && length <= total_size - offset;
}

static bool is_power_of_two(uint64_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

bool elf_inspect_image(const uint8_t *image, uint64_t image_size, struct elf_image_info *info) {
    if (image == 0 || info == 0 || image_size < ELF_HEADER_SIZE || image[0] != 0x7f
        || image[1] != 'E' || image[2] != 'L' || image[3] != 'F'
        || image[4] != ELF_CLASS_64 || image[5] != ELF_LITTLE_ENDIAN || image[6] != 1
        || (read_u16(image + 16) != ELF_TYPE_EXECUTABLE && read_u16(image + 16) != ELF_TYPE_SHARED)
        || read_u16(image + 18) != ELF_MACHINE_X86_64 || read_u32(image + 20) != 1) {
        return false;
    }
    uint64_t program_header_offset = read_u64(image + 32);
    uint16_t program_header_size = read_u16(image + 54);
    uint16_t program_header_count = read_u16(image + 56);
    if (program_header_size < ELF_PROGRAM_HEADER_MIN_SIZE || program_header_count == 0
        || !range_is_valid(program_header_offset,
            (uint64_t)program_header_count * (uint64_t)program_header_size, image_size)) {
        return false;
    }
    uint16_t loadable_segments = 0;
    for (uint16_t index = 0; index < program_header_count; ++index) {
        const uint8_t *program_header = image + program_header_offset + (uint64_t)index * program_header_size;
        if (read_u32(program_header) == ELF_PROGRAM_TYPE_LOAD) {
            uint64_t file_offset = read_u64(program_header + 8);
            uint64_t file_size = read_u64(program_header + 32);
            uint64_t memory_size = read_u64(program_header + 40);
            uint64_t alignment = read_u64(program_header + 48);
            if (memory_size < file_size || !range_is_valid(file_offset, file_size, image_size)
                || (alignment != 0 && !is_power_of_two(alignment))) {
                return false;
            }
            ++loadable_segments;
        }
    }
    if (loadable_segments == 0) {
        return false;
    }
    *info = (struct elf_image_info){
        .entry_point = read_u64(image + 24),
        .program_header_count = program_header_count,
        .loadable_segment_count = loadable_segments,
    };
    return true;
}
