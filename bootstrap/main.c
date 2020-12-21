#include <stddef.h>
#include <stdint.h>

#include "pl011.h"


// We get a list of these from the UEFI bootloader. It describes modules that
// the bootloader loaded in memory for us. It's up to the bootstrap code to
// convert this list into struct boot_info below that the kernel expects.
struct data {
    const char *name;
    uint64_t begin;
    uint64_t end;
};

enum reserve_type {
    INVALID = 0x0,
    // For anything that doesn't fit one of the defined purposes before we can
    // use UNKNOWN
    UNKNOWN = 0x1,
    // Memory occupied by the kernel image itself.
    KERNEL_IMAGE = 0x2,
    // Memory range occupied by the flattened device tree.
    DEVICE_TREE = 0x3,
};

// This structure describes a reserved range of memory that cannot be allocated
// for any other use. For example, the memory where the kernel image lives
// cannot be reused for anything else, otherwise we can overwrite the actual
// kernel data and/or code. We can also reserve some memory ranges for other
// reasons.
struct reserved_memory {
    uint64_t type;
    uint64_t begin;
    uint64_t end;
};

// This is the structure that we pass to the actual kernel. The goal of the
// bootstrap code is to combine information from various sources into this
// structure to pass it to the kernel.
struct boot_info {
    uint64_t devicetree_begin;
    uint64_t devicetree_end;
    struct reserved_memory reserve[32];
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

static void setup_boot_info(
    struct data *data, size_t size, struct boot_info *boot)
{
    for (size_t i = 0; i < size; ++i) {
        boot->reserve[i].begin = data[i].begin;
        boot->reserve[i].end = data[i].end;
        boot->reserve[i].type = UNKNOWN;

        if (strcmp(data[i].name, "kernel") == 0) {
            boot->reserve[i].type = KERNEL_IMAGE;
        } else if (strcmp(data[i].name, "dtb") == 0) {
            boot->reserve[i].type = DEVICE_TREE;
            boot->devicetree_begin = data[i].begin;
            boot->devicetree_end = data[i].end;
        }
    }
}

void start_kernel(struct boot_info *);

void main(struct data *data, size_t size)
{
    static struct boot_info boot;
    struct pl011 serial;

    // For HiKey960 board that I have the following parameters were found to
    // work fine:
    //
    // pl011_setup(
    //     &serial, /* base_address = */0xfff32000, /* base_clock = */19200000);
    pl011_setup(
        &serial, /* base_address = */0x9000000, /* base_clock = */24000000);
    pl011_send(&serial, "Bootstraping...\n", sizeof("Bootstraping...\n"));

    if (size > 32) {
        pl011_send(
            &serial,
            "Too many reserved memory ranges\n",
            sizeof("Too many reserved memory ranges\n"));
        while (1);
    }

    setup_boot_info(data, size, &boot);
    start_kernel(&boot);

    while (1);
}
