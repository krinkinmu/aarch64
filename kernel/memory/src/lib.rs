#![cfg_attr(not(test), no_std)]
#![feature(core_intrinsics)]
extern crate alloc;
mod buddy;
mod list;
mod memory_map;
mod page;

use buddy::BuddySystem;
use core::cmp;
use core::intrinsics;
use core::mem;
use core::ops::Range;
use core::ptr;
use core::slice;
use numeric;
use page::{Page, PageRange};
use sync::placeholder::Mutex;

pub use memory_map::MemoryMap;

struct Zone<'a> {
    range: PageRange<'a>,
    freelist: Mutex<BuddySystem>,
    space: Range<u64>,
    page_size: u64,
}

impl<'a> Zone<'a> {
    fn new(pages: &'a [Page], space: Range<u64>, page_size: u64) -> Zone<'a> {
        Zone {
            range: PageRange::new(pages, space.start / page_size),
            freelist: Mutex::new(BuddySystem::new()),
            space,
            page_size,
        }
    }

    fn allocate_pages(&self, order: u64) -> Option<u64> {
        self.freelist.lock().allocate_pages(&self.range, order)
    }

    fn free_pages(&self, page: u64) {
        self.freelist.lock().free_pages(&self.range, page);
    }

    unsafe fn free_pages_at_level(&self, page: u64, level: u64) {
        assert_eq!(page & ((1u64 << level) - 1), 0);
        self.range.page(page).set_level(level);
        self.free_pages(page);
    }

    unsafe fn free_space(&self, mmap: &MemoryMap) {
        for mem in mmap.free_memory_in_range(self.space.clone()) {
            let mut start = numeric::align_up(
                mem.range.start, self.page_size) / self.page_size;
            let end = numeric::align_down(
                mem.range.end, self.page_size) / self.page_size;

            while start < end {
                let order = cmp::min(
                    buddy::LEVELS as u32 - 1,
                    cmp::min(
                        numeric::ilog2(end - start),
                        numeric::lsb(start) - 1)) as u64;
                self.free_pages_at_level(start, order);
                start += 1 << order;
            }
        }
    }

    fn contains(&self, page: u64) -> bool {
        self.range.contains_index(page)
    }
}

pub struct Memory<'a> {
    zones: &'a [Zone<'a>],
    page_size: u64,
}

impl<'a> Memory<'a> {
    pub fn allocate_pages(&mut self, order: u64) -> Option<u64> {
        for zone in self.zones {
            if let Some(index) = zone.allocate_pages(order) {
                return Some(self.page_address(index));
            }
        }
        None
    }

    pub fn free_pages(&mut self, addr: u64) {
        let page = self.address_page(addr);

        for zone in self.zones {
            if zone.contains(page) {
                zone.free_pages(page);
                return;
            }
        }

        panic!()
    }

    pub fn page_size(&self) -> u64 {
        self.page_size
    }

    pub fn page_address(&self, page: u64) -> u64 {
        self.page_size * page
    }

    pub fn address_page(&self, addr: u64) -> u64 {
        addr / self.page_size
    }
}

unsafe fn create_pages(
    zone: Range<u64>,
    page_size: u64,
    malloc: &mut MemoryMap) -> Option<&'static [Page]>
{
    let pages = ((zone.end - zone.start) / page_size) as usize;
    let bytes = pages * mem::size_of::<Page>();

    assert!((page_size & (page_size - 1)) == 0);
    assert!(mem::align_of::<Page>() <= page_size as usize);

    malloc.allocate_with_hint(zone, bytes as u64, page_size)
        .map(|addr| {
            let ptr = addr as *mut Page;

            intrinsics::assert_zero_valid::<Page>();
            intrinsics::volatile_set_memory(ptr, 0, pages);
            slice::from_raw_parts(ptr as *const Page, pages)
        })
}

unsafe fn create_zones(
    page_size: u64,
    mmap: &MemoryMap,
    malloc: &mut MemoryMap) -> Option<&'static mut [Zone<'static>]>
{
    let zones = mmap.zones().len();
    let bytes = zones * mem::size_of::<Zone>();

    assert!((page_size & (page_size - 1)) == 0);
    assert!(mem::align_of::<Zone>() <= page_size as usize);

    malloc.allocate(bytes as u64, page_size)
        .map(|addr| {
            let ptr = addr as *mut Zone;
            for (index, zone) in mmap.zones().enumerate() {
                let start = numeric::align_up(zone.range.start, page_size);
                let end = numeric::align_down(zone.range.end, page_size);
                let pages = create_pages(
                    start..end, page_size, malloc).unwrap();

                ptr::write(
                    ptr.offset(index as isize),
                    Zone::new(pages, start..end, page_size)); 
            }
            slice::from_raw_parts_mut(ptr, zones)
        })
}

impl Memory<'static> {
    pub unsafe fn new(mmap: &MemoryMap) -> Memory<'static> {
        const PAGE_SIZE: u64 = 4096;

        let mut malloc = mmap.clone();
        let zones = create_zones(PAGE_SIZE, mmap, &mut malloc).unwrap();

        for zone in zones.iter() {
            zone.free_space(&malloc);
        }

        Memory { zones, page_size: PAGE_SIZE }
    }
}
