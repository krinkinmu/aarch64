#include "phys.h"

#include "util/algorithms.h"
#include "util/math.h"

namespace memory {

namespace {

MemoryRange* Compact(MemoryRange* begin, MemoryRange* end) {
    if (begin == end) {
        return end;
    }

    MemoryRange* pos = begin;
    for (MemoryRange* it = pos + 1; it != end; ++it) {
        if (it->begin == pos->end && it->status == pos->status) {
            pos->end = it->end;
            continue;
        }

        if (pos->begin == pos->end) {
            *pos = *it;
        } else {
            ++pos;
            *pos = *it;
        }
    }

    if (pos->begin == pos->end) {
        return pos;
    }
    return pos + 1;
}

}  // namespace

bool MemoryMap::SetStatus(uintptr_t begin, uintptr_t end, MemoryStatus status) {
    MemoryRange range;
    range.begin = begin;
    range.end = end;
    range.status = status;

    auto less = [](const MemoryRange& l, const MemoryRange& r) -> bool {
        return l.end <= r.begin;
    };

    MemoryRange *from = util::LowerBound(
        ranges_.Begin(), ranges_.End(), range, less);
    MemoryRange *to = util::UpperBound(
        ranges_.Begin(), ranges_.End(), range, less);

    if (from == to) {
        return true;
    }

    if (from->begin < range.begin) {
        MemoryRange head;
        head.begin = from->begin;
        head.end = range.begin;
        head.status = from->status;

        from->begin = head.end;
        if (!ranges_.Insert(from, head)) {
            return false;
        }

        ++from;
        ++to;
    }

    if ((to - 1)->end > range.end) {
        MemoryRange tail;
        tail.begin = range.end;
        tail.end = (to - 1)->end;
        tail.status = (to - 1)->status;

        (to - 1)->end = tail.begin;
        if (!ranges_.Insert(to, tail)) {
            return false;
        }
    }

    for (MemoryRange *it = from; it != to; ++to) {
        it->status = status;
    }

    ranges_.Erase(
        Compact(ranges_.Begin(), ranges_.End()),
        ranges_.End());
    return true;
}

bool MemoryMap::Register(uintptr_t begin, uintptr_t end, MemoryStatus status) {
    MemoryRange range;
    range.begin = begin;
    range.end = end;
    range.status = status;

    auto less = [](const MemoryRange& l, const MemoryRange& r) -> bool {
        return l.end <= r.begin;
    };

    auto from = util::LowerBound(
        ranges_.Begin(), ranges_.End(), range, less);
    auto to = util::UpperBound(
        ranges_.Begin(), ranges_.End(), range, less);

    if (from == to) {
        return ranges_.Insert(from, range);
    }

    for (auto it = from; it != to; ++it) {
        if (it->status != range.status) {
            return false;
        }
    }

    range.begin = util::Min(range.begin, from->begin);
    range.end = util::Max(range.end, (to - 1)->end);

    ranges_.Insert(ranges_.Erase(from, to), range);
    ranges_.Erase(
        Compact(ranges_.Begin(), ranges_.End()),
        ranges_.End());
    return true;
}

bool MemoryMap::Reserve(uintptr_t begin, uintptr_t end) {
    return SetStatus(begin, end, MemoryStatus::RESERVED);
}

bool MemoryMap::Release(uintptr_t begin, uintptr_t end) {
    return SetStatus(begin, end, MemoryStatus::FREE);
}

bool MemoryMap::FindIn(
        uintptr_t begin, uintptr_t end,
        size_t size, size_t alignment, MemoryStatus status,
        uintptr_t* ret) {
    MemoryRange range;
    range.begin = begin;
    range.end = end;
    range.status = status;
    
    auto less = [](const MemoryRange& l, const MemoryRange& r) -> bool {
        return l.end <= r.begin;
    };

    MemoryRange *from = util::LowerBound(
        ranges_.Begin(), ranges_.End(), range, less);
    MemoryRange *to = util::UpperBound(
        ranges_.Begin(), ranges_.End(), range, less);

    for (MemoryRange *it = from; it != to; ++it) {
        MemoryRange r = *it;

        if (r.status != status) {
            continue;
        }

        r.begin = util::Clamp(r.begin, begin, end);
        r.end = util::Clamp(r.end, begin, end);

        const uintptr_t addr = util::AlignUp(
            r.begin, static_cast<uintptr_t>(alignment));
        if (addr + size <= r.end) {
            *ret = addr;
            return true;
        }
    }
    return false;
}

bool MemoryMap::AllocateIn(
        uintptr_t begin, uintptr_t end,
        size_t size, size_t alignment,
        uintptr_t* ret)
{
    uintptr_t addr;

    if (!FindIn(begin, end, size, alignment, MemoryStatus::FREE, &addr)) {
        return false;
    }

    if (!Reserve(addr, addr + size)) {
        return false;
    }

    *ret = addr;
    return true;
}

bool MemoryMap::Allocate(size_t size, size_t alignment, uintptr_t* ret) {
    return AllocateIn(0, ~static_cast<uintptr_t>(0), size, alignment, ret);
}

}  // namespace memory
