use intrusive::{IntrusiveListLink, StructFieldOffset};
use core::cell::Cell;

#[derive(Debug)]
pub struct Page {
    pub link: IntrusiveListLink,
    state: Cell<u64>,
}

const LEVEL_MASK: u64 = 0x0ff;
const FREE_MASK: u64 = 0x100;

impl Page {
    pub fn new() -> Page {
        Page {
            link: IntrusiveListLink::new(),
            state: Cell::new(0),
        }
    }

    pub fn level(&self) -> u64 {
        self.state.get() & LEVEL_MASK
    }

    pub fn set_level(&self, level: u64) {
        assert_eq!(level & !LEVEL_MASK, 0);
        self.state.set(level | (self.state.get() & !LEVEL_MASK));
    }

    pub fn is_free(&self) -> bool {
        (self.state.get() & FREE_MASK) != 0
    }

    pub fn set_free(&self) {
        self.state.set(self.state.get() | FREE_MASK);
    }

    pub fn set_busy(&self) {
        self.state.set(self.state.get() & !FREE_MASK);
    }

    pub fn link_offset(&self) -> StructFieldOffset<Page, IntrusiveListLink> {
        StructFieldOffset::new(
            self as *const Page, &self.link as *const IntrusiveListLink)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::intrinsics;

    #[test]
    fn test_page_new() {
        let page = Page::new();
        assert_eq!(page.level(), 0);
        assert!(!page.is_free());
    }

    #[test]
    fn test_state() {
        let page = Page::new();

        page.set_level(0xff);
        page.set_free();
        assert_eq!(page.level(), 0xff);
        assert!(page.is_free());
        page.set_level(0);
        assert_eq!(page.level(), 0);
        assert!(page.is_free());
        page.set_busy();
        assert_eq!(page.level(), 0);
        assert!(!page.is_free());
    }

    #[test]
    fn test_zero_initialization() {
        unsafe { intrinsics::assert_zero_valid::<Page>(); }
    }
}
