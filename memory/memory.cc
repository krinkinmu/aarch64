#include "memory.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "common/fixed_vector.h"
#include "common/math.h"


namespace memory {

namespace {

constexpr uint64_t kPageFree = 1 << 0;

common::FixedVector<Zone, 32> AllZones;

size_t BuddyOffset(size_t offset, size_t order) {
    return offset ^ (static_cast<size_t>(1) << order);
}

}  // namespace


Zone::Zone(Page* page, size_t pages, uintptr_t from, uintptr_t to)
    : page_(page), pages_(pages), from_(from), to_(to)
{}

Page* Zone::AllocatePages(size_t order) {
    for (size_t from = order; from <= kMaxOrder; ++from) {
        if (free_[from].Empty()) {
            continue;
        }

        Page* page = free_[from].PopFront();
        available_ -= static_cast<size_t>(1) << order;
        return Split(page, from, order);
    }
    return nullptr;
}

void Zone::FreePages(Page* pages, size_t order) {
    Unite(pages, order);
    available_ += static_cast<size_t>(1) << order;
}

void Zone::FreePages(size_t addr, size_t order) {
    const size_t offset = addr >> kPageBits;
    FreePages(&page_[offset - Offset()], order);
}

size_t Zone::Offset() const { return FromAddress() >> kPageBits; }

size_t Zone::PageOffset(const Page* page) const {
    return Offset() + (page - page_);
}

uintptr_t Zone::PageAddress(const Page* page) const {
    return FromAddress() + ((page - page_) << kPageBits);
}

size_t Zone::Pages() const { return pages_; }

size_t Zone::Available() const { return available_; }

uintptr_t Zone::FromAddress() const { return from_; }

uintptr_t Zone::ToAddress() const { return to_; }

Page* Zone::Split(Page* page, size_t from, size_t to) {
    const size_t offset = Offset();
    const size_t page_offset = PageOffset(page);
    size_t order = from;

    while (order-- > to) {
        const size_t buddy_offset = BuddyOffset(page_offset, order);
        Page* buddy = &page_[buddy_offset - offset];

        buddy->order = order;
        buddy->flags |= kPageFree;
        free_[order].LinkAt(free_[order].Begin(), buddy);        
    }

    page->order = to;
    page->flags &= ~kPageFree;
    return page;
}

void Zone::Unite(Page* page, size_t from) {
    const size_t offset = Offset();
    size_t page_offset = PageOffset(page);
    size_t order = from;

    while (order < kMaxOrder) {
        const size_t buddy_offset = BuddyOffset(page_offset, order);

        if (buddy_offset < offset || buddy_offset - offset >= Pages()) {
            break;
        }

        Page* buddy = &page_[buddy_offset - offset];

        if (buddy->order != order || (buddy->flags & kPageFree) == 0) {
            break;
        }

        free_[order].Unlink(buddy);
        ++order;

        page_offset = std::min(page_offset, buddy_offset);
        page = std::min(page, buddy);
    }

    page->order = order;
    page->flags |= kPageFree;
    free_[order].LinkAt(free_[order].Begin(), page);
}


Contigous::Contigous() : zone_(nullptr), pages_(nullptr), order_(0) {}

Contigous::Contigous(nullptr_t) : Contigous() {}

Contigous::Contigous(class Zone* zone, struct Page* pages, size_t order)
    : zone_(zone), pages_(pages), order_(order) {}

Zone* Contigous::Zone() { return zone_; }

const Zone* Contigous::Zone() const { return zone_; }

Page* Contigous::Pages() { return pages_; }

const Page* Contigous::Pages() const { return pages_; }

size_t Contigous::Order() const { return order_; }

uintptr_t Contigous::FromAddress() const {
    if (pages_ == nullptr) {
        return 0;
    }
    return zone_->PageAddress(pages_);    
}

uintptr_t Contigous::ToAddress() const {
    return FromAddress() + Size();
}

size_t Contigous::Size() const {
    if (pages_ == nullptr) {
        return 0;
    }
    return static_cast<size_t>(1) << (order_ + kPageBits);
}

Contigous& Contigous::operator*() { return *this; }

Contigous* Contigous::operator->() { return this; }

Contigous::operator bool() const { return pages_ != nullptr; }

bool operator==(const Contigous& c1, const Contigous& c2) {
    return c1.Pages() == c2.Pages();
}

bool operator!=(const Contigous& c1, const Contigous& c2) {
    return c1.Pages() != c2.Pages();
}

void DeleteContigous::operator()(Contigous mem) {
    FreePhysical(mem);
}

Zone* AddressZone(uintptr_t addr) {
    for (auto it = AllZones.Begin(); it != AllZones.End(); ++it) {
        if (addr >= it->FromAddress() && addr < it->ToAddress()) {
            return &*it;
        }
    }
    return nullptr;
}

ContigousPtr AllocatePhysical(size_t size) {
    if (size == 0) {
        return ContigousPtr(nullptr);
    }

    const size_t power = 1 + common::MostSignificantBit(size - 1);
    const size_t order = std::max(power, kPageBits) - kPageBits;

    if (order > kMaxOrder) {
        return ContigousPtr(nullptr);
    }

    for (auto it = AllZones.Begin(); it != AllZones.End(); ++it) {
        Page* pages = it->AllocatePages(order);

        if (pages != nullptr) {
            return ContigousPtr(Contigous(&*it, pages, order));
        }
    }
    return ContigousPtr(nullptr);
}

void FreePhysical(Contigous mem) {
    if (mem.Size() == 0) {
        return;
    }
    mem.Zone()->FreePages(mem.Pages(), mem.Order());
}

size_t TotalPhysical() {
    size_t total = 0;

    for (auto it = AllZones.ConstBegin(); it != AllZones.ConstEnd(); ++it) {
        total += it->Pages() << kPageBits;
    }

    return total;
}

size_t AvailablePhysical() {
    size_t available = 0;

    for (auto it = AllZones.ConstBegin(); it != AllZones.ConstEnd(); ++it) {
        available += it->Available() << kPageBits;
    }

    return available;
}

namespace {

bool CreateZone(uintptr_t begin, uintptr_t end, MemoryMap* mmap) {
    begin = common::AlignUp(begin, static_cast<uintptr_t>(kPageSize));
    end = common::AlignDown(end, static_cast<uintptr_t>(kPageSize));
    if (begin >= end) {
        return true;
    }

    const size_t pages = (end - begin) >> kPageBits;
    const size_t bytes = pages * sizeof(struct Page);

    uintptr_t addr;
    if (!mmap->AllocateIn(begin, end, bytes, kPageSize, &addr)) {
        if (!mmap->Allocate(bytes, kPageSize, &addr)) {
            return false;
        }
    }

    struct Page* page = reinterpret_cast<struct Page*>(addr);
    memset(page, 0, bytes);
    return AllZones.EmplaceBack(page, pages, begin, end);    
}

bool CreateZones(MemoryMap* mmap) {
    if (mmap->ConstBegin() == mmap->ConstEnd()) {
        return true;
    }

    // We will iterate over the copy to avoid invalidating iterators.
    static MemoryMap copy = *mmap;
    auto first = copy.ConstBegin();
    uintptr_t begin = first->begin;
    uintptr_t end = first->end;

    for (auto it = ++first; it != copy.ConstEnd(); ++it) {
        if (end == it->begin) {
            end = it->end;
            continue;
        }

        if (!CreateZone(begin, end, mmap)) {
            return false;
        }

        begin = it->begin;
        end = it->end;
    }

    return CreateZone(begin, end, mmap);
}

bool FreeMemory(Zone* zone, uintptr_t begin, uintptr_t end) {
    if (begin > end) {
        return false;
    }

    if (begin < zone->FromAddress() || begin >= zone->ToAddress()) {
        return false;
    }

    if (end <= zone->FromAddress() || end > zone->ToAddress()) {
        return false;
    }

    begin = common::AlignUp(begin, static_cast<uintptr_t>(kPageSize));
    end = common::AlignDown(end, static_cast<uintptr_t>(kPageSize));
    if (begin >= end) {
        return true;
    }

    for (uintptr_t addr = begin; addr != end;) {
        const size_t offset = addr >> kPageBits;
        const size_t pages = (end - addr) >> kPageBits;

        const size_t align = common::LeastSignificantBit(offset);
        const size_t size = common::MostSignificantBit(pages);
        const size_t order = std::min(std::min(align, size), kMaxOrder);

        zone->FreePages(addr, order);
        addr += static_cast<uintptr_t>(1) << (kPageBits + order);
    }
    return true;
}

bool FreeUnusedMemory(MemoryMap* mmap) {
    auto zone = AllZones.Begin();
    auto range = mmap->ConstBegin();

    for (; range != mmap->ConstEnd(); ++range) {
        if (range->status != MemoryStatus::FREE) {
            continue;
        }

        uintptr_t begin = common::AlignUp(
            range->begin, static_cast<uintptr_t>(kPageSize));
        uintptr_t end = common::AlignDown(
            range->end, static_cast<uintptr_t>(kPageSize));
        if (begin >= end) {
            continue;
        }

        while (zone != AllZones.End() && zone->ToAddress() <= begin) {
            ++zone;
        }

        if (zone == AllZones.End()) {
            return false;
        }

        if (!FreeMemory(&*zone, begin, end)) {
            return false;
        }
    }

    return true;
}

}  // namespace

bool SetupAllocator(MemoryMap* mmap) {
    if (!CreateZones(mmap)) {
        return false;
    }
    return FreeUnusedMemory(mmap);
}

}  // namespace memory
