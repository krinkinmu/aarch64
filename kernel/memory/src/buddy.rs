use core::default::Default;
use crate::list::List;
use crate::page::{Page, PageRange};

pub const LEVELS: usize = 20;

#[derive(Debug)]
pub struct BuddySystem {
    free: [List; LEVELS],
}

impl BuddySystem {
    pub fn new() -> BuddySystem {
        BuddySystem {
            free: Default::default(),
        }
    }

    pub fn allocate_pages(&mut self, range: &PageRange, order: u64)
            -> Option<u64> {
        for level in (order as usize)..LEVELS {
            if let Some(index) = self.free[level].pop(range) {
                let page = range.page(index);

                self.split_and_return(range, page, index, order);
                page.set_busy();
                page.set_level(order);
                return Some(index);
            }
        }

        None
    }

    fn split_and_return(
            &mut self,
            range: &PageRange,
            page: &Page,
            index: u64,
            order: u64)
    {
        for level in (order..page.level()).rev() {
            let buddy_index = BuddySystem::buddy_index(index, level);
            let buddy = range.page(buddy_index);

            buddy.set_level(level);
            buddy.set_free();
            self.free[level as usize].push(range, buddy_index);
        }
    }

    pub fn free_pages(&mut self, range: &PageRange, index: u64) {
        let page = range.page(index);
        let mut level = page.level();

        while level < LEVELS as u64 - 1 {
            let buddy_index = BuddySystem::buddy_index(index, level);

            if !range.contains_index(buddy_index) {
                break;
            }

            let buddy = range.page(buddy_index);
            if !buddy.is_free() {
                break;
            }

            self.free[level as usize].remove(range, buddy_index);
            level += 1;
        }

        page.set_free();
        page.set_level(level);
        self.free[level as usize].push(range, index);
    }

    fn buddy_index(index: u64, order: u64) -> u64 {
        index ^ (1 << order)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_new() {
        let buddies = BuddySystem::new();
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
        let range = PageRange::new(&pages, 8);
        let mut buddy = BuddySystem::new();

        pages[0].set_level(3);
        buddy.free_pages(&range, 8);

        for _ in 0..2 {
            let mut allocated = Vec::new();
            for _ in 0..pages.len() {
                let ret = buddy.allocate_pages(&range, 0);
                assert!(ret.is_some());
                allocated.push(ret.unwrap());
            }
            assert!(buddy.allocate_pages(&range, 0).is_none());
            allocated.sort();
            assert_eq!(allocated, &[8, 9, 10, 11, 12, 13, 14, 15]);
            for index in allocated {
                buddy.free_pages(&range, index);
            }
        }
    }

    #[test]
    fn test_alignment() {
        let pages = vec![
            Page::new(), Page::new(), Page::new(), Page::new(),
            Page::new(), Page::new(), Page::new(), Page::new()
        ];
        let range = PageRange::new(&pages, 8);
        let mut buddy = BuddySystem::new();

        pages[0].set_level(3);
        buddy.free_pages(&range, 8);

        let index = buddy.allocate_pages(&range, 1).unwrap();
        assert_eq!(index & ((1 << 1) - 1), 0);
        buddy.free_pages(&range, index);

        let index = buddy.allocate_pages(&range, 2).unwrap();
        assert_eq!(index & ((1 << 2) - 1), 0);
        buddy.free_pages(&range, index);

        let index = buddy.allocate_pages(&range, 3).unwrap();
        assert_eq!(index & ((1 << 3) - 1), 0);
        buddy.free_pages(&range, index);
    }
}
