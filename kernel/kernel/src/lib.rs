#![no_std]
extern crate devicetree;
extern crate runtime;
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
    serial.send("Hello, from Rust\n");
    loop {}
}
