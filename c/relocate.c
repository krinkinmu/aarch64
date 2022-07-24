#include <stdint.h>

struct elf64_rela {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
};


const uint32_t R_AARCH64_RELATIVE = 1027;

static uint32_t rela_type(const struct elf64_rela *rela)
{
    return rela->r_info & 0xffffffff;
}

/*
 * Even PIE binary on AARCH64 might contain relocations that require runtime
 * adjustment. Apparently, it's normally done inside the CRT, but since the
 * kernel is not linked with the standard library it cannot use the CRT also,
 * so we have to handle relocations ourselves. This function is called from the
 * startup code with three parameters:
 *
 *  * difference between runtime and linktime addresses - that tells us by how
 *    much we need to correct addresses compared to what linker thought
 *  * pointers to the first and past last entries of the relocation array -
 *    gives us all the places that need to be corrected.
 *
 * In my case all the relocations I encountered are of R_AARCH64_RELATIVE type.
 * So that's the only relocation type this function handles, but it hangs if it
 * discovers an unknown/unexpected relocation type.
 */
void __relocate(int64_t diff, struct elf64_rela *begin, struct elf64_rela *end)
{
    for (struct elf64_rela *ptr = begin; ptr != end; ++ptr) {
        while (rela_type(ptr) != R_AARCH64_RELATIVE);

        *(uint64_t *)(ptr->r_offset + diff) = ptr->r_addend + diff;
    }
}
