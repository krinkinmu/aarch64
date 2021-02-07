use core::cmp;
use core::default::Default;
use core::ops::Range;
use crate::list::List;
use crate::page::Page;

pub const LEVELS: usize = 20;

#[derive(Debug)]
pub struct BuddySystem<'a> {
    free: [List<'a>; LEVELS],
    pages: &'a [Page],
    offset: u64,
}

impl<'a> BuddySystem<'a> {
    pub fn new(pages: &'a [Page], offset: u64) -> BuddySystem {
        BuddySystem {
            free: Default::default(),
            pages: pages,
            offset: offset,
        }
    }

    pub fn page_range(&self) -> Range<u64> {
        self.offset..self.offset + self.pages.len() as u64
    }

    unsafe fn page_index(&self, page: &Page) -> u64 {
        self.offset + (page as *const Page).offset_from(
            self.pages.as_ptr()) as u64
    }

    unsafe fn page_offset(&self, index: u64) -> usize {
        (index - self.offset) as usize
    }

    pub fn allocate_pages(&mut self, order: u64) -> Option<u64> {
        unsafe {
            let order = order as usize;
            for level in order..LEVELS {
                if let Some(page) = self.free[level].pop() {
                    let index = self.page_index(page);
                    self.split_and_return(page, index, order);
                    (*page).set_busy();
                    (*page).set_level(order as u64);
                    return Some(index);
                }
            }

            None
        }
    }

    unsafe fn split_and_return(
        &mut self, page: &Page, index: u64, order: usize)
    {
        let from = page.level() as usize;
        let to = order;

        for level in (to..from).rev() {
            let buddy = &self.pages[
                self.page_offset(BuddySystem::buddy_index(index, level))];

            buddy.set_level(level as u64);
            buddy.set_free();
            self.free[level].push(buddy);
        }
    }

    pub fn free_pages(&mut self, index: u64) {
        unsafe {
            let page = &self.pages[self.page_offset(index)];
            self.free_pages_at_level(index, page.level());
        }
    }

    pub unsafe fn free_pages_at_level(&mut self, index: u64, level: u64) {
        let mut index = index;
        let mut level = level as usize;

        while level < LEVELS - 1 {
            let buddy = BuddySystem::buddy_index(index, level);

            if !self.page_range().contains(&buddy) {
                break;
            }

            let page = &self.pages[self.page_offset(buddy)];
            if !page.is_free() { break; }
            if page.level() as usize != level { break; }

            self.free[level].remove(page);
            index = cmp::min(index, buddy);
            level += 1;
        }

        let page = &self.pages[self.page_offset(index)];
        page.set_free();
        page.set_level(level as u64);
        self.free[level].push(page);
    }

    fn buddy_index(index: u64, order: usize) -> u64 {
        index ^ (1 << order)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_new() {
        let buddies = BuddySystem::new(&[], 0);
        for level in 0..LEVELS {
            assert!(buddies.free[level].is_empty());
        }
    }

    #[test]
    fn test_buddy_index() {
        assert_eq!(BuddySystem::buddy_index(0, 0), 1);
        assert_eq!(BuddySystem::buddy_index(1, 0), 0);
        assert_eq!(BuddySystem::buddy_index(2, 0), 3);
        assert_eq!(BuddySystem::buddy_index(3, 0), 2);
        assert_eq!(BuddySystem::buddy_index(4, 0), 5);
        assert_eq!(BuddySystem::buddy_index(5, 0), 4);
        assert_eq!(BuddySystem::buddy_index(6, 0), 7);
        assert_eq!(BuddySystem::buddy_index(7, 0), 6);
        assert_eq!(BuddySystem::buddy_index(0, 1), 2);
        assert_eq!(BuddySystem::buddy_index(2, 1), 0);
        assert_eq!(BuddySystem::buddy_index(4, 1), 6);
        assert_eq!(BuddySystem::buddy_index(6, 1), 4);
        assert_eq!(BuddySystem::buddy_index(0, 2), 4);
        assert_eq!(BuddySystem::buddy_index(4, 2), 0);
    }

    #[test]
    fn test_alloc_free() {
        let pages = vec![
            Page::new(), Page::new(), Page::new(), Page::new(),
            Page::new(), Page::new(), Page::new(), Page::new()
        ];
        let mut buddy = BuddySystem::new(&pages[..], 8);

        buddy.free_pages(9);
        buddy.free_pages(10);
        buddy.free_pages(11);
        assert_eq!(buddy.allocate_pages(2), None);
        buddy.free_pages(8);
        assert_eq!(buddy.allocate_pages(2), Some(8));
    }

    #[test]
    fn test_alignment() {
        let pages = vec![
            Page::new(), Page::new(), Page::new(), Page::new(),
            Page::new(), Page::new(), Page::new(), Page::new()
        ];
        let mut buddy = BuddySystem::new(&pages[..], 8);

        pages[0].set_level(3);
        buddy.free_pages(8);

        let index1 = buddy.allocate_pages(1).unwrap();
        assert_eq!(index1 & ((1 << 1) - 1), 0);

        let index2 = buddy.allocate_pages(2).unwrap();
        assert_eq!(index2 & ((1 << 2) - 1), 0);

        buddy.free_pages(index1);
        buddy.free_pages(index2);
        let index = buddy.allocate_pages(3).unwrap();
        assert_eq!(index & ((1 << 3) - 1), 0);
    }
}
