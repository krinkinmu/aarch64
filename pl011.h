#ifndef __PL011_H__
#define __PL011_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// UART communication is defined by its speed (baudrate) and the format of the
// frame (number of data bits, parity check and the number of the stop bits).
// That's exactly what the fields of this structure describe, but I dropped the
// parity part for now and use no-parity check setting.
//
// Aside from generic UART parameters I also need base address of the PL011
// register block in memory. PL011 specification only defines offsets of the
// registers from the base address, but the base address itself can change (see
// "PrimeCell UART (PL011) Technical Reference Manual, Chapter 3 Programmers
// Model, Section 3.1 About the programmers model").
//
// Finally, "PrimeCell UART (PL011) Technical Reference Manual" doesn't actually
// define the base clock frequencey (UARTCLK in the documentation), so it has to
// be provided as well.
struct pl011 {
    uint64_t base_address;
    uint64_t base_clock;
    uint32_t baudrate;
    uint32_t data_bits;
    uint32_t stop_bits;
};

// Initialize PL011 device with the given parameters.
// Returns 0 on success.
int pl011_setup(
    struct pl011 *dev,
    uint64_t base_address,
    uint64_t base_clock);

// Reset the UART controller. That's the easiest thing to do if the controller
// got into a weird state. That being said since I don't really check for errors
// in the code it would be hard to notice that the controller got in a weird
// state.
// Returns 0 on sucess.
int pl011_reset(const struct pl011 *dev);

// Send sequence of bytes using the initialized PL011 device.
// Returns 0 on success.
int pl011_send(const struct pl011 *dev, const char *data, size_t size);

#endif  // __PL011_H__
