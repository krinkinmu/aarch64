#![no_std]
#[macro_use]
extern crate alloc;

use pl011::PL011;

#[derive(Copy, Clone, Debug)]
#[repr(C)]
pub struct InterruptFrame {
    x0: u64,
    x1: u64,
    x2: u64,
    x3: u64,
    x4: u64,
    x5: u64,
    x6: u64,
    x7: u64,
    x8: u64,
    x9: u64,
    x10: u64,
    x11: u64,
    x12: u64,
    x13: u64,
    x14: u64,
    x15: u64,
    x16: u64,
    x17: u64,
    x18: u64,
    fp: u64,
    lr: u64,
    sp: u64,
    esr: u64,
    far: u64,
}

#[no_mangle]
pub extern "C" fn interrupt(frame: *mut InterruptFrame) {
    unsafe { safe_interrupt(&mut *frame) }
}

fn safe_interrupt(frame: &mut InterruptFrame) {
    let serial = PL011::new(
        /* base_address = */0x9000000,
        /* base_clock = */24000000);
    serial.send(format!("{:?}", frame).as_str());
}

#[no_mangle]
pub extern "C" fn exception(frame: *mut InterruptFrame) {
    unsafe { safe_exception(&mut *frame) }
}

fn safe_exception(frame: &mut InterruptFrame) {
    let serial = PL011::new(
        /* base_address = */0x9000000,
        /* base_clock = */24000000);
    serial.send(format!("{:?}", frame).as_str());
    loop {}
}

