#![cfg_attr(not(test), no_std)]
mod buddy;
mod list;
mod page;

use buddy::BuddySystem;
use core::mem::MaybeUninit;
use core::pin::Pin;
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

static mut MEMORY: Option<Memory<'static>> = None;

pub fn memory() -> Option<Pin<&'static mut Memory<'static>>> {
    unsafe {
        if let Some(m) = MEMORY.as_mut() {
            Some(Pin::new(m))
        } else {
            None
        }
    }
}

pub fn setup_memory() {
    static mut ZONES: MaybeUninit<[Zone<'static>; 32]> = MaybeUninit::uninit();

    unsafe {
        assert!(&MEMORY.is_none());
        let memory = Memory { zones: &mut [], page_size: 4096 };
        MEMORY.replace(memory);
    }
}
