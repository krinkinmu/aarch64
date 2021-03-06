#![no_std]
#![feature(alloc_error_handler)]
extern crate alloc;

use bootstrap;
use core::alloc::{GlobalAlloc, Layout};
use core::panic::PanicInfo;


#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}

struct CustomAllocator;

unsafe impl GlobalAlloc for CustomAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        bootstrap::allocate_aligned(layout.size(), layout.align())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        bootstrap::free_aligned(ptr, layout.size(), layout.align());
    }
}

#[alloc_error_handler]
fn alloc_error_handler(_layout: Layout) -> ! {
    panic!("allocation failed!")
}

#[global_allocator]
static A: CustomAllocator = CustomAllocator;
