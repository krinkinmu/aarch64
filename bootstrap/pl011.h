#ifndef __BOOTSTRAP_PL011_H__
#define __BOOTSTRAP_PL011_H__

#include <cstddef>
#include <cstdint>

#include "common/stream.h"

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

class PL011OutputStream final : public common::OutputStream {
public:
    PL011OutputStream(PL011 *dev);
    ~PL011OutputStream() override;

    PL011OutputStream(const PL011OutputStream&) = delete;
    PL011OutputStream& operator=(const PL011OutputStream&) = delete;

    PL011OutputStream(PL011OutputStream&&) = default;
    PL011OutputStream& operator=(PL011OutputStream&&) = default;

    int PutN(const char *data, int size) override;
    int Put(char c) override;

private:
    PL011 *dev_;
};


#endif  // __BOOTSTRAP_PL011_H__
