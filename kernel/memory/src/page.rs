use core::cell::Cell;

pub struct Page {
    pub next: Cell<usize>,
    pub prev: Cell<usize>,
    state: Cell<u64>,
}

pub const NULL_PAGE: usize = 0;

const LEVEL_MASK: u64 = 0x0ff;
const FREE_MASK: u64 = 0x100;

impl Page {
    pub const fn new() -> Page {
        Page {
            next: Cell::new(NULL_PAGE),
            prev: Cell::new(NULL_PAGE),
            state: Cell::new(0u64),
        }
    }

    pub fn level(&self) -> usize {
        (self.state.get() & LEVEL_MASK) as usize
    }

    pub fn set_level(&self, level: usize) {
        assert_eq!((level as u64) & !LEVEL_MASK, 0);
        self.state.set((level as u64) | (self.state.get() & !LEVEL_MASK));
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
    offset: usize,
}

impl<'a> PageRange<'a> {
    pub fn new(pages: &'a [Page], offset: usize) -> PageRange<'a> {
        PageRange { pages, offset }
    }

    pub fn page(&self, index: usize) -> &Page {
        if !self.contains_index(index) {
            panic!();
        }

        &self.pages[index - self.offset]
    }

    pub fn index(&self, page: &Page) -> usize {
        if !self.contains_page(page) {
            panic!();
        }
        let shift = unsafe {
            (page as *const Page).offset_from(self.pages.as_ptr()) as usize
        };
        self.offset + shift
    }

    pub fn contains_index(&self, index: usize) -> bool {
        index >= self.offset && index < self.offset + self.pages.len()
    }

    pub fn contains_page(&self, page: &Page) -> bool {
        self.pages.as_ptr_range().contains(&(page as *const Page))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

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
    fn test_contains_page() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 42);
        let dummy = Page::new();

        assert!(range.contains_page(&pages[0]));
        assert!(range.contains_page(range.page(42)));
        assert!(range.contains_page(&pages[1]));
        assert!(range.contains_page(range.page(43)));
        assert!(range.contains_page(&pages[2]));
        assert!(range.contains_page(range.page(44)));
        assert!(!range.contains_page(&dummy));
    }

    #[test]
    fn test_index() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 42);

        assert_eq!(range.index(&pages[0]), 42);
        assert_eq!(range.index(&pages[1]), 43);
        assert_eq!(range.index(&pages[2]), 44);
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
}
