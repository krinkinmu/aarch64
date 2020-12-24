#![no_std]
#[macro_use]
extern crate alloc;
extern crate memalloc;
extern crate runtime;

use bootstrap::ReservedRange;
use bootstrap;
use alloc::vec::Vec;
use pl011::PL011;

#[no_mangle]
pub extern "C" fn start_kernel() {
    // For HiKey960 board that I have the following parameters were found to
    // work fine:
    //
    // let serial = PL011::new(
    //     /* base_address = */0xfff32000,
    //     /* base_clock = */19200000);
    let serial = PL011::new(
        /* base_address = */0x9000000,
        /* base_clock = */24000000);
    serial.send("Hello from Rust\n");

    loop {
        let mut ranges: Vec<ReservedRange> = Vec::new();
        serial.send("Before\n");
        let msg = format!("haba-haba\n");
        serial.send("After\n");

        for range in bootstrap::reserved_range_iter() {
            ranges.push(range);
        }

        for _range in ranges {
            serial.send(msg.as_str());
        }
    }
}
