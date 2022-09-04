#include "space.h"

namespace memory {

namespace impl {

PageTable::PageTable(uintptr_t address)
    : descriptors_(reinterpret_cast<uint64_t*>(address))
{}

uintptr_t PageTable::Address() const {
    return reinterpret_cast<uintptr_t>(descriptors_);
}

bool PageTable::IsClear(size_t entry) const {
    return (descriptors_[entry] & kPresent) == 0;
}

bool PageTable::IsTable(size_t entry) const {
    const uint64_t mask = kPresent | kTable;
    return (descriptors_[entry] & mask) == mask; 
}

bool PageTable::IsMemory(size_t entry) const {
    const uint64_t mask = kPresent | kTable;
    return (descriptors_[entry] & mask) == kPresent;
}

void PageTable::Clear(size_t entry) {
    descriptors_[entry] = 0;
}

void PageTable::SetTable(size_t entry, const PageTable& table) {
    descriptors_[entry] = table.Address() | kPresent | kTable;
}

void PageTable::SetMemory(size_t entry, const Memory& memory) {
    descriptors_[entry] = memory.addr | memory.attr | kPresent;
}

Memory PageTable::GetMemory(size_t entry) const {
    addr = descriptors_[entry] & kAddressMask;
    attr = descriptors_[entry] & kAttributesMask;
    return Memory{
        .addr = addr,
        .attr = attr,
    };
}

PageTable PageTable::GetTable(size_t entry) const {
    return PageTable(descriptors_[entry] & kAddressMask);
}

}  // namespace impl

bool SetupMapping() {
    // AArch64 System Register Descriptions, D13.2 General system control
    // registers, D13.2.85 MAIR_EL2, Memory Attribute Indirection Register (EL2)
    // for the encoding of the MAIR_EL2.
    constexpr uint64_t kNormalOuterWriteBackNonTransient = 0xc0;
    constexpr uint64_t kOuterReadAllocate = 0x20;
    constexpr uint64_t kOuterWriteAllocate = 0x10;

    constexpr uint64_t kNormalInnerWriteBackNonTransient = 0xc;
    constexpr uint64_t kInnerReadAllocate = 0x2;
    constexpr uint64_t kInnerWriteAllocate = 0x1;

    constexpr uint64_t mair =
        kNormalOuterWriteBackNonTransient |
        kOuterReadAllocate | kOuterWriteAllocate |
        kNormalInnerWriteBackNonTransient |
        kInnerReadAllocate | kInnerWriteAllocate;

    SetMairEl2(mair);
    return true;
}

}  // namespace memory
