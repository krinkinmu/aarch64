use core::default::Default;
use crate::page::{NULL_PAGE, PageRange};

pub struct List {
    head: usize,
}

impl List {
    pub fn new() -> List {
        List { head: NULL_PAGE }
    }

    #[cfg(test)]
    pub fn is_empty(&self) -> bool {
        self.head == NULL_PAGE
    }

    pub fn push(&mut self, range: &PageRange, index: usize) {
        assert_ne!(index, NULL_PAGE);
        let page = range.page(index);
        let next = self.head;

        page.next.set(next);
        page.prev.set(NULL_PAGE);
        self.head = index;

        if next != NULL_PAGE {
            range.page(next).prev.set(index);
        }
    }

    pub fn pop(&mut self, range: &PageRange) -> Option<usize> {
        if self.head == NULL_PAGE {
            return None;
        }

        let page_index = self.head;
        let page = range.page(page_index);
        let next_index = page.next.get();

        self.head = next_index;
        if next_index != NULL_PAGE {
            range.page(next_index).prev.set(NULL_PAGE);
        }

        page.prev.set(NULL_PAGE);
        page.next.set(NULL_PAGE);
        Some(page_index)
    }

    pub fn remove(&mut self, range: &PageRange, index: usize) {
        assert_ne!(index, NULL_PAGE);
        let page = range.page(index);
        let prev_index = page.prev.get();
        let next_index = page.next.get();

        if prev_index != NULL_PAGE {
            range.page(prev_index).next.set(next_index);
        }
        if next_index != NULL_PAGE {
            range.page(next_index).prev.set(prev_index);
        }
        if self.head == index {
            self.head = next_index;
        }
        page.prev.set(NULL_PAGE);
        page.next.set(NULL_PAGE);
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
    use crate::page::Page;

    #[test]
    fn test_new() {
        let l = List::new();
        assert_eq!(l.head, NULL_PAGE);
    }

    #[test]
    fn test_is_empty() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 1);
        let mut l = List::new();

        assert!(l.is_empty());
        l.push(&range, 1);
        assert!(!l.is_empty());
        l.push(&range, 2);
        assert!(!l.is_empty());
        l.push(&range, 3);
        assert!(!l.is_empty());
    }

    #[test]
    fn test_push_pop() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 1);
        let mut l = List::new();

        assert_eq!(l.head, NULL_PAGE);
        assert_eq!(l.pop(&range), None);
        l.push(&range, 1);
        l.push(&range, 2);
        l.push(&range, 3);
        assert_eq!(l.pop(&range), Some(3));
        assert_eq!(range.page(3).next.get(), NULL_PAGE);
        assert_eq!(range.page(3).prev.get(), NULL_PAGE);
        assert_eq!(l.pop(&range), Some(2));
        assert_eq!(range.page(2).next.get(), NULL_PAGE);
        assert_eq!(range.page(2).prev.get(), NULL_PAGE);
        assert_eq!(l.pop(&range), Some(1));
        assert_eq!(range.page(1).next.get(), NULL_PAGE);
        assert_eq!(range.page(1).prev.get(), NULL_PAGE);
        assert_eq!(l.pop(&range), None);
        assert_eq!(l.head, NULL_PAGE);
    }

    #[test]
    fn test_remove() {
        let pages = vec![Page::new(), Page::new(), Page::new()];
        let range = PageRange::new(pages.as_slice(), 1);
        let mut l = List::new();

        {
            l.push(&range, 1);
            l.push(&range, 2);
            l.push(&range, 3);
            l.remove(&range, 1);
            assert_eq!(range.page(1).next.get(), NULL_PAGE);
            assert_eq!(range.page(1).prev.get(), NULL_PAGE);
            assert_eq!(l.pop(&range), Some(3));
            assert_eq!(l.pop(&range), Some(2));
            assert_eq!(l.pop(&range), None);
            assert_eq!(l.head, NULL_PAGE);
        }
        {
            l.push(&range, 1);
            l.push(&range, 2);
            l.push(&range, 3);
            l.remove(&range, 2);
            assert_eq!(range.page(2).next.get(), NULL_PAGE);
            assert_eq!(range.page(2).prev.get(), NULL_PAGE);
            assert_eq!(l.pop(&range), Some(3));
            assert_eq!(l.pop(&range), Some(1));
            assert_eq!(l.pop(&range), None);
            assert_eq!(l.head, NULL_PAGE);
        }
        {
            l.push(&range, 1);
            l.push(&range, 2);
            l.push(&range, 3);
            l.remove(&range, 3);
            assert_eq!(range.page(3).next.get(), NULL_PAGE);
            assert_eq!(range.page(3).prev.get(), NULL_PAGE);
            assert_eq!(l.head, 2);
            assert_eq!(l.pop(&range), Some(2));
            assert_eq!(l.pop(&range), Some(1));
            assert_eq!(l.pop(&range), None);
            assert_eq!(l.head, NULL_PAGE);
        }
        {
            l.push(&range, 1);
            l.remove(&range, 1);
            assert_eq!(range.page(1).next.get(), NULL_PAGE);
            assert_eq!(range.page(1).prev.get(), NULL_PAGE);
            assert_eq!(l.pop(&range), None);
            assert_eq!(l.head, NULL_PAGE);
        }
    }
}
