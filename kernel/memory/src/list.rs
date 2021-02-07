use core::default::Default;
use core::mem;
use core::ptr;
use crate::page::Page;

#[derive(Debug)]
pub struct List {
    head: *const Page,
}

impl List {
    pub const fn new() -> List {
        List { head: ptr::null() }
    }

    #[cfg(test)]
    pub fn is_empty(&self) -> bool {
        self.head == ptr::null()
    }

    pub unsafe fn push(&mut self, page: *const Page) {
        assert_eq!((*page).next.get(), ptr::null());
        assert_eq!((*page).prev.get(), ptr::null());
        let next = mem::replace(&mut self.head, page);

        (*page).next.set(next);
        (*page).prev.set(ptr::null());

        if next != ptr::null() {
            (*next).prev.set(page);
        }
    }

    pub unsafe fn pop(&mut self) -> Option<*const Page> {
        if self.head == ptr::null() {
            return None;
        }

        let page = self.head;
        let next = (*page).next.get();

        self.head = next;
        if next != ptr::null() {
            (*next).prev.set(ptr::null());
        }

        (*page).prev.set(ptr::null());
        (*page).next.set(ptr::null());
        Some(page)
    }

    pub unsafe fn remove(&mut self, page: *const Page) {
        assert_ne!(page, ptr::null());
        let prev = (*page).prev.get();
        let next = (*page).next.get();

        if prev != ptr::null() {
            (*prev).next.set(next);
        }
        if next != ptr::null() {
            (*next).prev.set(prev);
        }
        if self.head == page {
            self.head = next;
        }
        (*page).prev.set(ptr::null());
        (*page).next.set(ptr::null());
    }
}

impl Default for List {
    fn default() -> List {
        List::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_new() {
        let l = List::new();
        assert_eq!(l.head, ptr::null());
    }

    #[test]
    fn test_is_empty() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            assert!(l.is_empty());
            l.push(&pages[0] as *const Page);
            assert!(!l.is_empty());
            l.push(&pages[1] as *const Page);
            assert!(!l.is_empty());
            l.push(&pages[2] as *const Page);
            assert!(!l.is_empty());
        }
    }

    #[test]
    fn test_push_pop() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            assert_eq!(l.head, ptr::null());
            assert_eq!(l.pop(), None);
            l.push(&pages[0] as *const Page);
            l.push(&pages[1] as *const Page);
            l.push(&pages[2] as *const Page);
            assert_eq!(l.pop(), Some(&pages[2] as *const Page));
            assert_eq!(pages[2].next.get(), ptr::null());
            assert_eq!(pages[2].prev.get(), ptr::null());
            assert_eq!(l.pop(), Some(&pages[1] as *const Page));
            assert_eq!(pages[1].next.get(), ptr::null());
            assert_eq!(pages[1].prev.get(), ptr::null());
            assert_eq!(l.pop(), Some(&pages[0] as *const Page));
            assert_eq!(pages[0].next.get(), ptr::null());
            assert_eq!(pages[0].prev.get(), ptr::null());
            assert_eq!(l.pop(), None);
            assert_eq!(l.head, ptr::null());
        }
    }

    #[test]
    fn test_remove() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            l.push(&pages[0] as *const Page);
            l.push(&pages[1] as *const Page);
            l.push(&pages[2] as *const Page);
            l.remove(&pages[0] as *const Page);
            assert_eq!(pages[0].next.get(), ptr::null());
            assert_eq!(pages[0].prev.get(), ptr::null());
            assert_eq!(l.pop(), Some(&pages[2] as *const Page));
            assert_eq!(l.pop(), Some(&pages[1] as *const Page));
            assert_eq!(l.pop(), None);
            assert_eq!(l.head, ptr::null());
        }
        unsafe {
            l.push(&pages[0] as *const Page);
            l.push(&pages[1] as *const Page);
            l.push(&pages[2] as *const Page);
            l.remove(&pages[1] as *const Page);
            assert_eq!(pages[1].next.get(), ptr::null());
            assert_eq!(pages[1].prev.get(), ptr::null());
            assert_eq!(l.pop(), Some(&pages[2] as *const Page));
            assert_eq!(l.pop(), Some(&pages[0] as *const Page));
            assert_eq!(l.pop(), None);
            assert_eq!(l.head, ptr::null());
        }
        unsafe {
            l.push(&pages[0] as *const Page);
            l.push(&pages[1] as *const Page);
            l.push(&pages[2] as *const Page);
            l.remove(&pages[2] as *const Page);
            assert_eq!(pages[2].next.get(), ptr::null());
            assert_eq!(pages[2].prev.get(), ptr::null());
            assert_eq!(l.pop(), Some(&pages[1] as *const Page));
            assert_eq!(l.pop(), Some(&pages[0] as *const Page));
            assert_eq!(l.pop(), None);
            assert_eq!(l.head, ptr::null());
        }
        unsafe {
            l.push(&pages[0] as *const Page);
            l.remove(&pages[0] as *const Page);
            assert_eq!(pages[0].next.get(), ptr::null());
            assert_eq!(pages[0].prev.get(), ptr::null());
            assert_eq!(l.pop(), None);
            assert_eq!(l.head, ptr::null());
        }
    }
}
