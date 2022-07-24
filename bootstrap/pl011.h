#ifndef __BOOTSTRAP_PL011_H__
#define __BOOTSTRAP_PL011_H__

#include "util/stddef.h"
#include "util/stdint.h"
#include "util/string_view.h"

/*
 * UART communication is defined by its speed (baudrate) and the format of the
 * frame (number of data bits, parity check and the number of the stop bits).
 * That's exactly what the fields of this structure describe, but I dropped the
 * parity part for now and use no-parity check setting.
 *
 * Aside from generic UART parameters I also need base address of the PL011
 * register block in memory. PL011 specification only defines offsets of the
 * registers from the base address, but the base address itself can change (see
 * "PrimeCell UART (PL011) Technical Reference Manual, Chapter 3 Programmers
 * Model, Section 3.1 About the programmers model").
 *
 * Finally, "PrimeCell UART (PL011) Technical Reference Manual" doesn't actually
 * define the base clock frequencey (UARTCLK in the documentation), so it has to
 * be provided as well.
 */
class PL011 {
public:
    static PL011 Serial(uintptr_t base_address, uint64_t base_clock); 

    PL011(const PL011 &other) = delete;
    PL011& operator=(const PL011 &other) = delete;

    PL011(PL011 &&other) = default;
    PL011& operator=(PL011 &&other) = default;

    void Reset();
    void Send(const uint8_t *data, size_t size);

private:
    PL011(volatile uint32_t *regs, uint32_t divisor)
            : registers_(regs), divisor_(divisor) {}

    volatile uint32_t *registers_;
    uint32_t divisor_;

    static const uint32_t kBaudrate = 115200;
    static const uint32_t kDataBits = 8;
    static const uint32_t kStopBits = 1;
};

void FormatAddress(PL011 &dev, uintptr_t addr);
void FormatNumber(PL011 &dev, intmax_t num);
void FormatString(PL011 &dev, const char *str);
void FormatString(PL011 &dev, util::StringView str);

#endif  // __BOOTSTRAP_PL011_H__
