use core::default::Default;
use core::marker::PhantomData;
use core::option::Option;
use crate::page::Page;
use intrusive::IntrusiveList;

#[derive(Debug)]
pub struct PageList<'a> {
    list: IntrusiveList<Page>,
    _marker: PhantomData<&'a Page>,
}

impl<'a> PageList<'a> {
    pub fn new() -> PageList<'a> {
        let page = Page::new();

        PageList {
            list: IntrusiveList::new(page.link_offset()),
            _marker: PhantomData,
        }
    }

    pub unsafe fn push(&mut self, page: &'a Page) {
        self.list.push(page as *const Page);
    }

    pub unsafe fn pop(&mut self) -> Option<&'a Page> {
        self.list.pop().map(|ptr| { &*ptr })
    }

    pub unsafe fn remove(&mut self, page: &'a Page) {
        self.list.remove(page as *const Page);
    }
}

impl<'a> Default for PageList<'a> {
    fn default() -> PageList<'a> {
        PageList::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_push_pop() {
        let pages = [Page::new(), Page::new(), Page::new()];
        let mut l = PageList::new();

        unsafe {
            assert!(l.pop().is_none());
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            assert!(l.pop().is_none());
        }
    }

    #[test]
    fn test_remove() {
        let pages = [Page::new(), Page::new(), Page::new()];
        let mut l = PageList::new();

        unsafe {
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            l.remove(&pages[0]);
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            assert!(l.pop().is_none());
        }
        unsafe {
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            l.remove(&pages[1]);
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[2] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            assert!(l.pop().is_none());
        }
        unsafe {
            l.push(&pages[0]);
            l.push(&pages[1]);
            l.push(&pages[2]);
            l.remove(&pages[2]);
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[1] as *const Page));
            assert_eq!(
                l.pop().map(|x| x as *const Page),
                Some(&pages[0] as *const Page));
            assert!(l.pop().is_none());
        }
        unsafe {
            l.push(&pages[0]);
            l.remove(&pages[0]);
            assert!(l.pop().is_none());
        }
    }
}
