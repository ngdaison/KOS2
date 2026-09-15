#ifndef KOS_MEMORY_H
#define KOS_MEMORY_H

#include <stdint.h>

/* The base mapping/allocation granularity used by PMM, VMM, and kernel heap. */
#define KOS_PAGE_SIZE 4096ull

_Static_assert(KOS_PAGE_SIZE != 0 && (KOS_PAGE_SIZE & (KOS_PAGE_SIZE - 1)) == 0,
    "KOS page size must be a non-zero power of two");

#endif
