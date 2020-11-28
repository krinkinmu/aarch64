#include "pl011.h"

static const uint32_t DR_OFFSET = 0x000;
static const uint32_t FR_OFFSET = 0x018;
static const uint32_t IBRD_OFFSET = 0x024;
static const uint32_t FBRD_OFFSET = 0x028;
static const uint32_t LCR_OFFSET = 0x02c;
static const uint32_t CR_OFFSET = 0x030;
static const uint32_t IMSC_OFFSET = 0x038;
static const uint32_t DMACR_OFFSET = 0x048;

static const uint32_t CR_TXEN = (1 << 8);
static const uint32_t CR_UARTEN = (1 << 0);

static const uint32_t FR_BUSY = (1 << 3);

static const uint32_t LCR_FEN = (1 << 4);
static const uint32_t LCR_STP2 = (1 << 3);

static volatile uint32_t *reg(const struct pl011 *dev, uint32_t offset)
{
    const uint64_t addr = dev->base_address + offset;

    return (volatile uint32_t *)((void *)addr);
}

static void wait_tx_complete(const struct pl011 *dev)
{
    while ((*reg(dev, FR_OFFSET) & FR_BUSY) != 0) {}
}

// Calculates the baudrate divisior value from the device parameters. The
// divisor values is divided into a 16 bit integer part and 6 bit fractional
// part. The 6-bit fractional part contains the number of 1/2^6 = 1/64
// fractions.
//
// In general the divisor have to be calculated as base clock / (16 * baudrate)
// according to the "PrimeCell UART (PL011) Technical Reference Manual, Chapter
// 3 Programmers Model, Section 3.3.6 Fractional Baud Rate Register, UARTFBRD". 
static void calculate_divisors(
    const struct pl011 *dev, uint32_t *integer, uint32_t *fractional)
{
    // It's somewhat inconvenient to work with fractions, so let's first get
    // that out of the way by calculating the result multiplied by 64.
    //
    // I could multiply by a bigger number if I wanted higher precision, but
    // since I don't really need that much precision 64 is just enough.
    //
    // That changes our equation to 4 * base clock / baudrate and I only need
    // the integer part of the result.
    const uint32_t div = 4 * dev->base_clock / dev->baudrate;

    // Now the fractional part is stored in the lower 6 bits and the integer
    // part is everything except the lower 6 bits.
    *fractional = div & 0x3f;
    *integer = (div >> 6) & 0xffff;
}

int pl011_setup(
    struct pl011 *dev,
    uint64_t base_address,
    uint64_t base_clock)
{
    dev->base_address = base_address;
    dev->base_clock = base_clock;
    // Those parameters can be configurable, but this PL011 here is only for the
    // sake of establishing basic communication with the device and it's not a
    // generic UART driver, so I can be a bit lazy here.
    dev->baudrate = 115200;
    dev->data_bits = 8;
    dev->stop_bits = 1;
    return pl011_reset(dev);
}

// Rest will initialize PL011 device according to the parameters provided in the
// struct pl011 passed as an argument to the function.
// The device will be initialized in the transmit mode only, without FIFOs or
// DMA and all the interrupts masked. That basically means, that the device can
// only be used for char-by-char transmission in polling mode (I need to
// explicitly check that the transmission is complete) - that's the simplest
// mode of operation.
int pl011_reset(const struct pl011 *dev)
{
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
    uint32_t cr = *reg(dev, CR_OFFSET);
    uint32_t lcr = *reg(dev, LCR_OFFSET);
    uint32_t ibrd, fbrd;

    // steps 1 - 3 of the control register programming sequence
    *reg(dev, CR_OFFSET) = (cr & ~CR_UARTEN);
    wait_tx_complete(dev);
    *reg(dev, LCR_OFFSET) = (lcr & ~LCR_FEN);

    // while UART is disabled I will also program all other registers besides
    // the control register:
    //   1. IBRD and FBRD that control the baudrate
    //   2. LCR that controls the frame format
    //   3. IMSC responsible for setting and clearing interrupt masks
    //   4. DMACR responsible for DMA settings
    calculate_divisors(dev, &ibrd, &fbrd);
    *reg(dev, IBRD_OFFSET) = ibrd;
    *reg(dev, FBRD_OFFSET) = fbrd;

    lcr = 0x0;
    lcr |= ((dev->data_bits - 1) & 0x3) << 5;
    if (dev->stop_bits == 2)
        lcr |= LCR_STP2;
    *reg(dev, LCR_OFFSET) = lcr;

    *reg(dev, IMSC_OFFSET) = 0x7ff;

    *reg(dev, DMACR_OFFSET) = 0x0;

    // steps 4 - 5 of the control register programming sequence
    *reg(dev, CR_OFFSET) = CR_TXEN;
    *reg(dev, CR_OFFSET) = CR_TXEN | CR_UARTEN;

    return 0;
}

int pl011_send(const struct pl011 *dev, const char *data, size_t size)
{
    wait_tx_complete(dev);

    for (size_t i = 0; i < size; ++i) {
        // I either have to do it here or each and every place where the
        // function is used has to add '\r' to each '\n'.
        //
        // Given that in the Unix world plain '\n' is normally used instead of
        // '\r\n' I decided to go with the first option as more natural.
        if (data[i] == '\n') {
            *reg(dev, DR_OFFSET) = '\r';
            wait_tx_complete(dev);
        }
        *reg(dev, DR_OFFSET) = data[i];
        wait_tx_complete(dev);
    }

    return 0;
}
