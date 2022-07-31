#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <cstddef>
#include <memory>

#include "memory/phys.h"


namespace memory {

class Zone;
struct Page;

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
ContigousPtr AllocatePhysical(size_t size);
void FreePhysical(Contigous mem);

size_t TotalPhysical();
size_t AvailablePhysical();

}  // namespace memory

#endif  // __MEMORY_H__
