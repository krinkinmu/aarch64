#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "util/stddef.h"
#include "memory/phys.h"


namespace memory {

class Zone;
struct Page;

class Contigous {
public:
    Contigous();
    Contigous(Zone* zone, Page* pages, size_t order);
    ~Contigous();

    Contigous(const Contigous& other) = delete;
    Contigous& operator=(const Contigous& other) = delete;

    Contigous(Contigous&& other);
    Contigous& operator=(Contigous&& other);

    Zone* Zone();
    Page* Pages();
    size_t Order();

    uintptr_t FromAddress() const;
    uintptr_t ToAddress() const;
    size_t Size() const;

private:
    class Zone* zone_;
    struct Page* pages_;
    size_t order_;
};


bool SetupAllocator(MemoryMap* map);
Contigous AllocatePhysical(size_t size);
void FreePhysical(Contigous mem);
size_t TotalPhysical();
size_t AvailablePhysical();

}  // namespace memory

#endif  // __MEMORY_H__
