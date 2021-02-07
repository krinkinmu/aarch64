use core::default::Default;
use core::option::Option;
use core::ptr;
use crate::page::Page;

#[derive(Debug)]
pub struct List<'a> {
    head: Option<&'a Page>,
}

impl<'a> List<'a> {
    pub const fn new() -> List<'a> {
        List { head: None }
    }

    #[cfg(test)]
    pub fn is_empty(&self) -> bool {
        self.head.is_none()
    }

    pub unsafe fn push(&mut self, page: &'a Page) {
        page.prev.set(ptr::null());

        if let Some(next) = self.head {
            next.prev.set(page as *const Page);
            page.next.set(next as *const Page);
        } else {
            page.next.set(ptr::null());
        }
        self.head = Some(page);
    }

    pub unsafe fn pop(&mut self) -> Option<&'a Page> {
        self.head.map(|head| {
            let next = head.next.get();

            head.next.set(ptr::null());
            head.prev.set(ptr::null());

            if next != ptr::null() {
                (*next).prev.set(ptr::null());
            }
            self.head = Some(&*next);

            head
        })
    }

    pub unsafe fn remove(&mut self, page: &'a Page) {
        let prev = page.prev.get();
        let next = page.next.get();

        page.prev.set(ptr::null());
        page.next.set(ptr::null());

        if prev != ptr::null() {
            (*prev).next.set(next);
        }
        if next != ptr::null() {
            (*next).prev.set(prev);
        }

        if self.head.unwrap() as *const Page == page as *const Page {
            self.head = if next == ptr::null() {
                None
            } else {
                Some(&*next)
            };
        }
    }
}

impl<'a> Default for List<'a> {
    fn default() -> List<'a> {
        List::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_new() {
        let l = List::new();
        assert!(l.head.is_none());
    }

    #[test]
    fn test_is_empty() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            assert!(l.is_empty());
            l.push(&pages[0]);
            assert!(!l.is_empty());
            l.push(&pages[1]);
            assert!(!l.is_empty());
            l.push(&pages[2]);
            assert!(!l.is_empty());
        }
    }

    #[test]
    fn test_push() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            assert!(l.head.is_none());
            l.push(&pages[0]);
            assert_eq!(
                l.head.map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            l.push(&pages[1]);
            assert_eq!(
                l.head.map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            l.push(&pages[2]);
            assert_eq!(
                l.head.map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
        }
    }

    #[test]
    fn test_pop() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            assert!(l.head.is_none());
            assert!(l.pop().is_none());
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
            assert_eq!(pages[2].next.get(), ptr::null());
            assert_eq!(pages[2].prev.get(), ptr::null());
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            assert_eq!(pages[1].next.get(), ptr::null());
            assert_eq!(pages[1].prev.get(), ptr::null());
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            assert_eq!(pages[0].next.get(), ptr::null());
            assert_eq!(pages[0].prev.get(), ptr::null());
            assert!(l.pop().is_none());
            assert!(l.head.is_none());
        }
    }

    #[test]
    fn test_remove() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let mut l = List::new();

        unsafe {
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            l.remove(&pages[0]);
            assert_eq!(pages[0].next.get(), ptr::null());
            assert_eq!(pages[0].prev.get(), ptr::null());
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            assert!(l.pop().is_none());
            assert!(l.head.is_none());
        }
        unsafe {
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            l.remove(&pages[1]);
            assert_eq!(pages[1].next.get(), ptr::null());
            assert_eq!(pages[1].prev.get(), ptr::null());
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            assert!(l.pop().is_none());
            assert!(l.head.is_none());
        }
        unsafe {
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            l.remove(&pages[2]);
            assert_eq!(pages[2].next.get(), ptr::null());
            assert_eq!(pages[2].prev.get(), ptr::null());
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            assert!(l.pop().is_none());
            assert!(l.head.is_none());
        }
        unsafe {
            l.push(&pages[0]);
            l.remove(&pages[0]);
            assert_eq!(pages[0].next.get(), ptr::null());
            assert_eq!(pages[0].prev.get(), ptr::null());
            assert!(l.pop().is_none());
            assert!(l.head.is_none());
        }
    }
}
