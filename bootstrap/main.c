#include <stddef.h>
#include <stdint.h>

#include "alloc.h"
#include "config.h"
#include "pl011.h"

// We get a list of these from the UEFI bootloader. It describes modules that
// the bootloader loaded in memory for us. It's up to the bootstrap code to
// convert this list into struct boot_info below that the kernel expects.
struct data {
    const char *name;
    uint64_t begin;
    uint64_t end;
};

static int strcmp(const char *l, const char *r)
{
    while (*l == *r && *l != '\0') {
        ++l;
        ++r;
    }

    if (*l < *r) return -1;
    if (*l > *r) return 1;
    return 0;
}

static void reserve_ranges(const struct data *data, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (strcmp(data[i].name, "kernel") == 0)
            reserve_range(data[i].begin, data[i].end, RESERVE_KERNEL);
        else if (strcmp(data[i].name, "dtb") == 0)
            reserve_range(data[i].begin, data[i].end, RESERVE_DEVICETREE);
        else
            reserve_range(data[i].begin, data[i].end, RESERVE_OTHER);
    }
}

void start_kernel(void);

// I'm using this cont variable to pause the kernel early during the boot
// process to have time to connect to QEMU with GDB for debugging. Just
// set it to 0 and the system will hang in the infinite loop inside main.
// To break the infinite loop, just write to the cont variable a non-zero
// value using GDB memory write.
static volatile int cont = 1;

void main(struct data *data, size_t size)
{
    struct pl011 serial;
    uint64_t heap_begin, heap_end;

    while (!cont);

    // For HiKey960 board that I have the following parameters were found to
    // work fine:
    //
    // pl011_setup(
    //     &serial, /* base_address = */0xfff32000, /* base_clock = */19200000);
    pl011_setup(
        &serial, /* base_address = */0x9000000, /* base_clock = */24000000);
    pl011_send(&serial, "Bootstraping...\n", sizeof("Bootstraping...\n"));

    bootstrap_allocator_setup();
    reserve_ranges(data, size);
    bootstrap_heap(&heap_begin, &heap_end);
    bootstrap_allocator_add_range(heap_begin, heap_end);

    // Uncomment to generate an exception to test the interrupt/exception setup
    // cont = *((volatile int *)0xffffffffffffffffull);

    start_kernel();

    while (1);
}
