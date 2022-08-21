#ifndef __MEMORY_ARCH_H__
#define __MEMORY_ARCH_H__

#include <cstdint>

namespace memory {

inline void SetMairEl2(uint64_t mair) {
    asm volatile("msr MAIR_EL2, %0" : : "r"(mair));
}

inline uint64_t GetMairEl2() {
    uint64_t mair;
    asm volatile("mrs %0, MAIR_EL2" : "=r"(mair));
    return mair;
}

inline void SetTtbar0El2(uintptr_t ttbr) {
    asm volatile("msr TTBR0_EL2, %0" : : "r"(ttbr));
}

inline uintptr_t GetTtbar0El2() {
    uintptr_t ttbr;
    asm volatile("mrs %0, TTBR0_EL2" : "=r"(ttbr));
    return ttbr;
}

}  // namespace memory

#endif  // __MEMORY_ARCH_H__
