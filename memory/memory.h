#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <cstddef>
#include <memory>

#include "common/intrusive_list.h"

#include "phys.h"


namespace memory {

constexpr size_t kMaxOrder = 20;
constexpr size_t kPageBits = 12;
constexpr size_t kPageSize = (1 << kPageBits);


struct Page : public common::ListNode<Page> {
    uint64_t flags;
    size_t order;
};


class Zone {
public:
    Zone(Page* page, size_t pages, uintptr_t from, uintptr_t to);

    Zone(const Zone&) = delete;
    Zone& operator=(const Zone&) = delete;
    Zone(Zone&&) = delete;
    Zone& operator=(Zone&&) = delete;

    Page* AllocatePages(size_t order);
    void FreePages(Page* pages, size_t order);
    void FreePages(uintptr_t addr, size_t order);

    size_t Offset() const;
    size_t PageOffset(const Page* page) const;
    uintptr_t PageAddress(const Page* page) const;
    size_t Pages() const;
    size_t Available() const;
    uintptr_t FromAddress() const;
    uintptr_t ToAddress() const;


private:
    Page* Split(Page* page, size_t from, size_t to);
    void Unite(Page* page, size_t from);

    Page* page_;
    size_t pages_;
    size_t available_;
    uintptr_t from_;
    uintptr_t to_;
    common::IntrusiveList<Page> free_[kMaxOrder + 1];
};


class Contigous {
public:
    Contigous();
    Contigous(nullptr_t);
    Contigous(Zone* zone, Page* pages, size_t order);

    Contigous(const Contigous& other) = default;
    Contigous& operator=(const Contigous& other) = default;
    Contigous(Contigous&& other) = default;
    Contigous& operator=(Contigous&& other) = default;

    class Zone* Zone();
    const class Zone* Zone() const;
    Page* Pages();
    const Page* Pages() const;
    size_t Order() const;

    uintptr_t FromAddress() const;
    uintptr_t ToAddress() const;
    size_t Size() const;

    Contigous& operator*();
    Contigous* operator->();
    explicit operator bool() const;

private:
    class Zone* zone_;
    struct Page* pages_;
    size_t order_;
};

bool operator==(const Contigous& l, const Contigous& r);
bool operator!=(const Contigous& l, const Contigous& r);


struct DeleteContigous {
    using pointer = Contigous;

    void operator()(Contigous mem);
};

using ContigousPtr = std::unique_ptr<Contigous, DeleteContigous>;


bool SetupAllocator(MemoryMap* map);

Zone* AddressZone(uintptr_t addr);
ContigousPtr AllocatePhysical(size_t size);
void FreePhysical(Contigous mem);

size_t TotalPhysical();
size_t AvailablePhysical();

}  // namespace memory

#endif  // __MEMORY_H__
