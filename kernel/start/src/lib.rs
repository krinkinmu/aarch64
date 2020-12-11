#![no_std]
extern crate runtime;
use pl011::PL011;

#[no_mangle]
pub extern "C" fn start_kernel() {
    let serial = PL011::new(
        /* base_address = */0x9000000,
        /* base_clock = */24000000);
    serial.send("Hello, from Rust\n");
    loop {}
}
