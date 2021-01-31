use core::cell::Cell;

pub struct Page {
    pub next: Cell<u64>,
    pub prev: Cell<u64>,
    state: Cell<u64>,
}

pub const NULL_PAGE: u64 = 0;

const LEVEL_MASK: u64 = 0x0ff;
const FREE_MASK: u64 = 0x100;

impl Page {
    #[cfg(test)]
    pub fn new() -> Page {
        Page {
            next: Cell::new(0),
            prev: Cell::new(0),
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
}

pub struct PageRange<'a> {
    pages: &'a [Page],
    offset: u64,
}

impl<'a> PageRange<'a> {
    pub fn new(pages: &'a [Page], offset: u64) -> PageRange<'a> {
        PageRange { pages, offset }
    }

    pub fn page(&self, index: u64) -> &Page {
        if !self.contains_index(index) {
            panic!();
        }

        &self.pages[(index - self.offset) as usize]
    }

    pub fn contains_index(&self, index: u64) -> bool {
        index >= self.offset
            && index < self.offset + self.pages.len() as u64
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::intrinsics;

    #[test]
    fn test_page_new() {
        let page = Page::new();
        assert_eq!(page.next.get(), NULL_PAGE);
        assert_eq!(page.prev.get(), NULL_PAGE);
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
    fn test_contains_index() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 42);

        assert!(!range.contains_index(41));
        assert!(range.contains_index(42));
        assert!(range.contains_index(43));
        assert!(range.contains_index(44));
        assert!(!range.contains_index(45));
    }

    #[test]
    fn test_page() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 42);

        assert_eq!(&pages[0] as *const Page, range.page(42) as *const Page);
        range.page(42).next.set(42);
        assert_eq!(range.page(42).next.get(), 42);
        assert_eq!(&pages[1] as *const Page, range.page(43) as *const Page);
        range.page(43).prev.set(42);
        assert_eq!(range.page(43).prev.get(), 42);
        assert_eq!(&pages[2] as *const Page, range.page(44) as *const Page);
        range.page(44).set_level(42);
        assert_eq!(range.page(44).level(), 42);
    }

    #[test]
    fn test_zero_initialization() {
        unsafe { intrinsics::assert_zero_valid::<Page>(); }
    }
}
