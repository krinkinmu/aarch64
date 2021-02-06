#![no_std]
extern crate alloc;
extern crate bootstrap;
extern crate devicetree;
extern crate interrupt;
extern crate memory;
extern crate log;
extern crate pl011;
extern crate runtime;
extern crate sync;

mod setup;

#[no_mangle]
pub extern "C" fn start_kernel() {
    setup::setup_logger();
    let dt = devicetree::fdt::parse(bootstrap::fdt()).unwrap();
    let _memory = setup::setup_memory(&dt);
    log::log("Hello from Rust\n");
    loop {}
}
