#include "pl011.h"

#include "util/string_view.h"

namespace {

constexpr uint32_t DR = (0x000 / 4);
constexpr uint32_t FR = (0x018 / 4);
constexpr uint32_t IBRD = (0x024 / 4);
constexpr uint32_t FBRD = (0x028 / 4);
constexpr uint32_t LCR = (0x02c / 4);
constexpr uint32_t CR = (0x030 / 4);
constexpr uint32_t IMSC = (0x038 / 4);
constexpr uint32_t ICR = (0x044 / 4);
constexpr uint32_t DMACR = (0x048 / 4);

constexpr uint32_t CR_TXEN = (1 << 8);
constexpr uint32_t CR_UARTEN = (1 << 0);
constexpr uint32_t FR_BUSY = (1 << 3);
constexpr uint32_t LCR_FEN = (1 << 4);


void WaitTxComplete(volatile uint32_t *fr_reg) {
    while ((*fr_reg & FR_BUSY) != 0) {}
}

// Calculates the baudrate divisior value from the device parameters. The
// divisor values is divided into a 16 bit integer part and 6 bit fractional
// part. The 6-bit fractional part contains the number of 1/2^6 = 1/64
// fractions.
//
// In general the divisor have to be calculated as base clock / (16 * baudrate)
// according to the "PrimeCell UART (PL011) Technical Reference Manual, Chapter
// 3 Programmers Model, Section 3.3.6 Fractional Baud Rate Register, UARTFBRD". 
uint32_t Divisor(uint64_t base_clock, uint32_t baudrate) {
    // It's somewhat inconvenient to work with fractions, so let's first get
    // that out of the way by calculating the result multiplied by 64.
    //
    // I could multiply by a bigger number if I wanted higher precision, but
    // since I don't really need that much precision 64 is just enough.
    //
    // That changes our equation to 4 * base clock / baudrate and I only need
    // the integer part of the result.
    //
    // Now the fractional part is stored in the lower 6 bits and the integer
    // part is everything except the lower 6 bits.
    return 4 * base_clock / baudrate;
}

}  // namespace

PL011 PL011::Serial(uintptr_t base_address, uint64_t base_clock) {
    volatile uint32_t *regs = reinterpret_cast<volatile uint32_t*>(
        base_address);
    uint32_t divisor = Divisor(base_clock, PL011::kBaudrate);

    WaitTxComplete(&regs[FR]);
    return static_cast<PL011&&>(PL011(regs, divisor));
}

void PL011::Reset() {
    // According to "PrimeCell UART (PL011) Technical Reference Manual, Chapter
    // 3 Programmers Model, Section 3.3.8 Control Register, UARTCR" the correct
    // sequence involing programmin of the control register (CR) is:
    //   1. disable UART
    //   2. wait for any ongoing transmissions and receives to complete
    //   3. flush the FIFO
    //   4. program the control register
    //   5. enable UART.
    //
    // The sequence is slightly confusing because both enabling and disabling
    // UART involves writing to the control register. So I interpret it as
    // writing the UARTEN bit of the control register should be done separately
    // from everything else.
    //
    // The other part that is confusing me is that while I can check for any
    // outgoing transmissions by checking the BUSY bit of the FR register, I
    // didn't really figure out what does it mean to wait for any ongoing
    // receives to complete and how I can do it. So that part I just ignored.
    const uint32_t cr = registers_[CR];
    const uint32_t lcr = registers_[LCR];

    // steps 1 - 3 of the control register programming sequence
    registers_[CR] = (cr & ~CR_UARTEN);
    WaitTxComplete(&registers_[FR]);
    registers_[LCR] = (lcr & ~LCR_FEN);

    registers_[IMSC] = 0x7ff;

    registers_[ICR] = 0x7ff;

    registers_[DMACR] = 0x0;

    // while UART is disabled I will also program all other registers besides
    // the control register:
    //   1. IBRD and FBRD that control the baudrate
    //   2. LCR that controls the frame format
    //   3. IMSC responsible for setting and clearing interrupt masks
    //   4. DMACR responsible for DMA settings
    registers_[IBRD] = divisor_ >> 6;
    registers_[FBRD] = divisor_ & 0x3F;
    registers_[LCR] = ((kDataBits - 1) & 0x3) << 5;

    // steps 4 - 5 of the control register programming sequence
    registers_[CR] = CR_TXEN;
    registers_[CR] = CR_TXEN | CR_UARTEN;
}

void PL011::Send(const uint8_t *data, size_t size) {
    WaitTxComplete(&registers_[FR]);

    for (size_t i = 0; i < size; ++i) {
        // I either have to do it here or each and every place where the
        // function is used has to add '\r' to each '\n'.
        //
        // Given that in the Unix world plain '\n' is normally used instead of
        // '\r\n' I decided to go with the first option as more natural.
        if (data[i] == '\n') {
            registers_[DR] = '\r';
            WaitTxComplete(&registers_[FR]);
        }
        registers_[DR] = data[i];
        WaitTxComplete(&registers_[FR]);
    }
}

PL011OutputStream::PL011OutputStream(PL011 *dev) : dev_(dev) {}

PL011OutputStream::~PL011OutputStream() {}

int PL011OutputStream::PutN(const char *data, int n) {
    dev_->Send(reinterpret_cast<const uint8_t*>(data), n);
    return n;
}

int PL011OutputStream::Put(char c) {
    return PutN(&c, 1);
}
