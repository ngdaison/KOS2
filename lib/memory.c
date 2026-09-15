#include <stdint.h>

/* Minimal freestanding C runtime routines required by compiler-generated code. */
void *memset(void *destination, int value, uint64_t size) {
    uint8_t *bytes = destination;
    for (uint64_t index = 0; index < size; ++index) {
        bytes[index] = (uint8_t)value;
    }
    return destination;
}

void *memcpy(void *destination, const void *source, uint64_t size) {
    uint8_t *target = destination;
    const uint8_t *input = source;
    for (uint64_t index = 0; index < size; ++index) {
        target[index] = input[index];
    }
    return destination;
}

void *memmove(void *destination, const void *source, uint64_t size) {
    uint8_t *target = destination;
    const uint8_t *input = source;
    if (target < input) {
        for (uint64_t index = 0; index < size; ++index) {
            target[index] = input[index];
        }
    }
    else if (target > input) {
        for (uint64_t index = size; index != 0; --index) {
            target[index - 1] = input[index - 1];
        }
    }
    return destination;
}

int memcmp(const void *left, const void *right, uint64_t size) {
    const uint8_t *first = left;
    const uint8_t *second = right;
    for (uint64_t index = 0; index < size; ++index) {
        if (first[index] != second[index]) {
            return first[index] < second[index] ? -1 : 1;
        }
    }
    return 0;
}
