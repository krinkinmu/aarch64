#![cfg_attr(not(test), no_std)]
#![feature(core_intrinsics)]
mod buddy;
mod list;
mod page;

use buddy::BuddySystem;
use core::default::Default;
use core::intrinsics;
use core::mem;
use core::slice;
use page::{Page, PageRange};

struct Zone<'a> {
    range: PageRange<'a>,
    freelist: BuddySystem,
}

impl<'a> Zone<'a> {
    fn new(pages: &'a [Page], offset: usize) -> Zone<'a> {
        Zone {
            range: PageRange::new(pages, offset),
            freelist: BuddySystem::new(),
        }
    }

    fn allocate_pages(&mut self, order: usize) -> Option<usize> {
        self.freelist.allocate_pages(&self.range, order)
    }

    fn free_pages(&mut self, page: usize) {
        self.freelist.free_pages(&self.range, page);
    }

    fn contains(&self, page: usize) -> bool {
        self.range.contains_index(page)
    }
}

impl<'a> Default for Zone<'a> {
    fn default() -> Zone<'a> {
        Zone::new(&[], 0)
    }
}

pub struct Memory<'a> {
    zones: &'a mut [Zone<'a>],
    page_size: usize,
}

impl<'a> Memory<'a> {
    pub fn allocate_pages(&mut self, order: usize) -> Option<usize> {
        for zone in self.zones.iter_mut() {
            if let Some(index) = zone.allocate_pages(order) {
                return Some(self.page_address(index));
            }
        }
        None
    }

    pub fn free_pages(&mut self, addr: usize) {
        let page = self.address_page(addr);

        for zone in self.zones.iter_mut() {
            if zone.contains(page) {
                zone.free_pages(page);
                return;
            }
        }

        panic!()
    }

    pub fn page_size(&self) -> usize {
        self.page_size
    }

    pub fn page_address(&self, page: usize) -> usize {
        self.page_size * page
    }

    pub fn address_page(&self, addr: usize) -> usize {
        addr / self.page_size
    }
}

pub struct MemoryRange {
    addr: usize,
    size: usize,
}

impl MemoryRange {
    pub fn from(&self) -> usize {
        self.addr
    }

    pub fn to(&self) -> usize {
        self.addr + self.size
    }

    pub fn size(&self) -> usize {
        self.size
    }
}

fn align_down(x: usize, a: usize) -> usize {
    assert_eq!(a & (a - 1), 0);
    x & !(a - 1)
}

fn align_up(x: usize, a: usize) -> usize {
    align_down(x + a - 1, a)
}

fn clamp(x: usize, l: usize, r: usize) -> usize {
    if x < l {
        l
    } else if x > r {
        r
    } else {
        x
    }
}

fn find_free_space(
        from: usize,
        to: usize,
        size: usize,
        align: usize,
        reserved: &[MemoryRange]) -> Option<usize> {
    let mut addr = align_up(from, align);

    for reserve in reserved {
        if reserve.from() >= to {
            break;
        }
        if reserve.to() <= from {
            continue;
        }

        let begin = clamp(reserve.from(), addr, to);
        let end = clamp(reserve.to(), addr, to);

        if begin - addr < size {
            addr = align_up(end, align);
            continue;
        }

        return Some(addr);
    }

    None
}

unsafe fn setup_zone(
        range: &MemoryRange,
        reserved: &[MemoryRange],
        page_size: usize) -> Zone<'static> {
    let from = align_up(range.from(), page_size);
    let to = align_down(range.to(), page_size);
    let offset = from / page_size;
    let pages =  range.size() / page_size;
    let bytes_needed = pages * mem::size_of::<Page>();

    if let Some(addr) = find_free_space(
            from, to, bytes_needed, page_size, reserved) {
        let pages = {
            intrinsics::assert_zero_valid::<Page>();
            intrinsics::volatile_set_memory(addr as *mut Page, 0, pages);
            slice::from_raw_parts(addr as *const Page, pages)
        };
        Zone::new(pages, offset)
    } else {
        Zone::new(&[], offset)
    }
}

unsafe fn allocate_zones(size: usize) -> Option<&'static mut [Zone<'static>]> {
    const MAX_SIZE: usize = 32;
    static mut ZONES: Option<[Zone<'static>; MAX_SIZE]> = None;

    if size >= MAX_SIZE {
        return None;
    }

    if ZONES.is_some() {
        return None;
    }

    ZONES = Some(Default::default());
    Some(&mut ZONES.as_mut().unwrap()[..size])
}

pub fn setup_memory(memory: &[MemoryRange], reserved: &[MemoryRange])
        -> Memory<'static> {
    const PAGE_SIZE: usize = 4096;

    unsafe {
        let zones = allocate_zones(memory.len()).unwrap();
        for index in 0..zones.len() {
            zones[index] = setup_zone(&memory[index], reserved, PAGE_SIZE);
        }
        Memory { zones, page_size: PAGE_SIZE }
    }
}
