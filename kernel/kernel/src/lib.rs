#![no_std]
extern crate alloc;
extern crate bootstrap;
extern crate devicetree;
extern crate interrupt;
extern crate log;
extern crate memory;
extern crate numeric;
extern crate pl011;
extern crate runtime;
extern crate sync;

mod setup;

use log::Logger;
use memory::Memory;

pub struct Kernel {
    memory: Memory<'static>,
    logger: Logger<'static>,
}

#[no_mangle]
pub extern "C" fn start_kernel() {
    let kernel = setup::from_devicetree();
    kernel.logger.log("Hello from Rust\n");
    loop {}
}
