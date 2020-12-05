#include "pl011.h"

void main(void)
{
    struct pl011 serial;

    // For HiKey960 board that I have the following parameters were found to
    // work fine:
    //
    // pl011_setup(
    //     &serial, /* base_address = */0xfff32000, /* base_clock = */19200000);
    pl011_setup(
        &serial, /* base_address = */0x9000000, /* base_clock = */24000000);
    pl011_send(&serial, "Hello, World\n", sizeof("Hello, World\n"));

    // Hang in there
    while (1) {}
}
