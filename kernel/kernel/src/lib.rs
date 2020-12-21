#![no_std]
extern crate runtime;

use core::slice;
use devicetree::DeviceTree;
use pl011::PL011;

#[repr(C)]
struct ReservedMemory {
    memory_type: u64,
    begin: u64,
    end: u64,
}

#[repr(C)]
pub struct BootInfo {
    devicetree_begin: u64,
    devicetree_end: u64,
    reserve: [ReservedMemory; 32],
}

fn get_device_tree(boot_info: &BootInfo) -> DeviceTree {
    let addr = boot_info.devicetree_begin;
    let size = boot_info.devicetree_end - boot_info.devicetree_begin;

    unsafe {
        let data = slice::from_raw_parts(addr as *const u8, size as usize);

        DeviceTree::new(data).unwrap()
    }
}

#[no_mangle]
pub extern "C" fn start_kernel(boot_info: &'static BootInfo) {
    let _dt = get_device_tree(boot_info);

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

    loop {}
}
