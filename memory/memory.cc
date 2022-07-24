#include "memory/memory.h"

#include "util/stdint.h"
#include "util/string.h"
#include "util/intrusive_list.h"
#include "util/fixed_vector.h"
#include "util/math.h"


namespace memory {

namespace {

constexpr size_t kMaxOrder = 20;
constexpr size_t kPageBits = 12;
constexpr size_t kPageSize = (1 << kPageBits);
constexpr uint64_t kPageFree = 1 << 0;

}  // namespace


struct Page : public util::ListNode<Page> {
    uint64_t flags;
    size_t order;
};


class Zone {
public:
    Zone(Page* page, size_t pages, uintptr_t from, uintptr_t to)
        : page_(page), pages_(pages), from_(from), to_(to) {}

    Zone(const Zone& other) = delete;
    Zone& operator=(const Zone& other) = delete;
    Zone(Zone&& other) = delete;
    Zone& operator=(Zone&& other) = delete;

    Page* AllocatePages(size_t order) {
        for (size_t from = order; from <= kMaxOrder; ++from) {
            if (free_[from].Empty()) {
                continue;
            }

            Page* page = &(*free_[from].Begin());
            free_[from].PopFront();
            available_ -= static_cast<size_t>(1) << order;
            return Split(page, from, order);
        }
        return nullptr;
    }

    void FreePages(Page* pages, size_t order) {
        Unite(pages, order);
        available_ += static_cast<size_t>(1) << order;
    }

    void FreePages(size_t offset, size_t order) {
        FreePages(&page_[offset - Offset()], order);
    }

    size_t Offset() const { return FromAddress() >> kPageBits; }
    size_t PageOffset(const Page* page) const {
        return Offset() + (page - page_);
    }
    uintptr_t PageAddress(const Page* page) const {
        return FromAddress() + ((page - page_) << kPageBits);
    }
    size_t Pages() const { return pages_; }
    size_t Available() const { return available_; }
    uintptr_t FromAddress() const { return from_; }
    uintptr_t ToAddress() const { return to_; }

private:
    Page* Split(Page* page, size_t from, size_t to);
    void Unite(Page* page, size_t from);

    Page* page_;
    size_t pages_;
    size_t available_;
    uintptr_t from_;
    uintptr_t to_;
    util::IntrusiveList<Page> free_[kMaxOrder + 1];
};


namespace {

util::FixedVector<Zone, 32> AllZones;


size_t BuddyOffset(size_t offset, size_t order) {
    return offset ^ (static_cast<size_t>(1) << order);
}

}  // namespace


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

        page_offset = util::Min(page_offset, buddy_offset);
        page = util::Min(page, buddy);
    }

    page->order = order;
    page->flags |= kPageFree;
    free_[order].LinkAt(free_[order].Begin(), page);
}


Contigous::Contigous() : zone_(nullptr), pages_(nullptr), order_(0) {}

Contigous::Contigous(class Zone* zone, struct Page* pages, size_t order)
    : zone_(zone), pages_(pages), order_(order) {}

Contigous::~Contigous() {
    zone_ = nullptr;
    pages_ = nullptr;
    order_ = 0;
}

Contigous::Contigous(Contigous&& other)
        : zone_(other.zone_), pages_(other.pages_), order_(other.order_) {
    other.zone_ = nullptr;
    other.pages_ = nullptr;
    other.order_ = 0;
}

Contigous& Contigous::operator=(Contigous&& other) {
    if (this == &other) {
        return *this;
    }
    zone_ = other.zone_;
    pages_ = other.pages_;
    order_ = other.order_;
    other.zone_ = nullptr;
    other.pages_ = nullptr;
    other.order_ = 0;
    return *this;
}

Zone* Contigous::Zone() { return zone_; }

Page* Contigous::Pages() { return pages_; }

size_t Contigous::Order() { return order_; }

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


Contigous AllocatePhysical(size_t size) {
    if (size == 0) {
        return Contigous();
    }

    const size_t power = 1 + util::MostSignificantBit(size - 1);
    const size_t order = util::Max(power, kPageBits) - kPageBits;

    if (order > kMaxOrder) {
        return Contigous();
    }

    for (auto it = AllZones.Begin(); it != AllZones.End(); ++it) {
        Page* pages = it->AllocatePages(order);

        if (pages != nullptr) {
            return Contigous(&*it, pages, order);
        }
    }
    return Contigous();
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
    begin = util::AlignUp(begin, static_cast<uintptr_t>(kPageSize));
    end = util::AlignDown(end, static_cast<uintptr_t>(kPageSize));
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
    util::memset(page, 0, bytes);
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

    begin = util::AlignUp(begin, static_cast<uintptr_t>(kPageSize));
    end = util::AlignDown(end, static_cast<uintptr_t>(kPageSize));
    if (begin >= end) {
        return true;
    }

    for (uintptr_t addr = begin; addr != end;) {
        const size_t offset = addr >> kPageBits;
        const size_t pages = (end - addr) >> kPageBits;

        const size_t align = util::LeastSignificantBit(offset);
        const size_t size = util::MostSignificantBit(pages);
        const size_t order = util::Min(util::Min(align, size), kMaxOrder);

        zone->FreePages(offset, order);
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

        uintptr_t begin = util::AlignUp(
            range->begin, static_cast<uintptr_t>(kPageSize));
        uintptr_t end = util::AlignDown(
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
