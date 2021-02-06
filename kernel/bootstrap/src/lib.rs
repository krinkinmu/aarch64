#![no_std]
use core::iter::Iterator;
use core::ops::Range;
use core::ptr;
use core::slice;
use core::slice::Iter;

#[repr(C)]
struct PackedReservedRange {
    kind: u64,
    begin: u64,
    end: u64,
}

extern {
    fn reserved_ranges(
        ranges: *mut *const PackedReservedRange, size: *mut u32);
    fn devicetree(begin: *mut u64, end: *mut u64);
    fn bootstrap_heap(begin: *mut u64, end: *mut u64);

    fn bootstrap_allocator_shutdown() -> isize;
    fn bootstrap_allocate_aligned(size: usize, align: usize) -> *mut u8;
    fn bootstrap_free_aligned(ptr: *mut u8, size: usize, align: usize);
}

fn reserved_memory_ranges() -> &'static [PackedReservedRange] {
    unsafe {
        let mut ranges: *const PackedReservedRange = ptr::null();
        let mut size: u32 = 0;

        reserved_ranges(
            &mut ranges as *mut *const PackedReservedRange,
            &mut size as *mut u32);
        slice::from_raw_parts(ranges, size as usize)
    }
}

pub struct ReservedRangeIter {
    inner: Iter<'static, PackedReservedRange>,
}

impl Iterator for ReservedRangeIter {
    type Item = Range<u64>;

    fn next(&mut self) -> Option<Range<u64>> {
        match self.inner.next() {
            Some(range) => Some(range.begin..range.end),
            None => None,
        }
    }
}

pub fn reserved_range_iter() -> ReservedRangeIter {
    ReservedRangeIter { inner: reserved_memory_ranges().iter() }
}

pub fn fdt() -> &'static [u8] {
    unsafe {
        let mut begin: u64 = 0;
        let mut end: u64 = 0;

        devicetree(&mut begin as *mut u64, &mut end as *mut u64);
        if begin == end {
            return &[];
        }
        slice::from_raw_parts(begin as *const u8, (end - begin) as usize)
    }
}

pub fn heap_range() -> Range<u64> {
    unsafe {
        let mut begin: u64 = 0;
        let mut end: u64 = 0;

        bootstrap_heap(&mut begin as *mut u64, &mut end as *mut u64);
        begin..end
    }
}

pub fn allocator_shutdown() -> bool {
    unsafe {
        bootstrap_allocator_shutdown() == 0
    }
}

pub unsafe fn allocate_aligned(size: usize, align: usize) -> *mut u8 {
    bootstrap_allocate_aligned(size, align)
}

pub unsafe fn free_aligned(ptr: *mut u8, size: usize, align: usize) {
    bootstrap_free_aligned(ptr, size, align);
}
