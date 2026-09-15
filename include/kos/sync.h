#ifndef KOS_SYNC_H
#define KOS_SYNC_H

#include <stdbool.h>
#include <stdint.h>

#include <kos/cpu.h>

/*
 * Raw spin locks are only for short, non-sleeping kernel critical sections.
 * The irqsave form prevents a local interrupt handler from deadlocking by
 * taking a lock held by the code it interrupted. Sleeping mutexes and
 * semaphores require the preemptive scheduler and are intentionally separate.
 */
struct kos_spinlock {
    volatile uint32_t state;
};

#define KOS_SPINLOCK_INITIALIZER { .state = 0 }

static inline void spinlock_lock(struct kos_spinlock *lock) {
    for (;;) {
        if (__atomic_exchange_n(&lock->state, 1, __ATOMIC_ACQUIRE) == 0) {
            return;
        }
        while (__atomic_load_n(&lock->state, __ATOMIC_RELAXED) != 0) {
            cpu_pause();
        }
    }
}

static inline bool spinlock_try_lock(struct kos_spinlock *lock) {
    return __atomic_exchange_n(&lock->state, 1, __ATOMIC_ACQUIRE) == 0;
}

static inline void spinlock_unlock(struct kos_spinlock *lock) {
    __atomic_store_n(&lock->state, 0, __ATOMIC_RELEASE);
}

static inline uint64_t spinlock_lock_irqsave(struct kos_spinlock *lock) {
    uint64_t flags = cpu_interrupt_save_disable();
    spinlock_lock(lock);
    return flags;
}

static inline void spinlock_unlock_irqrestore(struct kos_spinlock *lock, uint64_t flags) {
    spinlock_unlock(lock);
    cpu_interrupt_restore(flags);
}

#endif
