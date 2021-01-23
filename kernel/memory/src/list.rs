use core::assert;
use core::default::Default;
use core::ptr;

#[repr(C)]
pub struct ListLink {
    next: *mut ListLink,
    prev: *mut ListLink,
}

impl ListLink {
    pub fn new() -> ListLink {
        ListLink {
            next: ptr::null_mut(),
            prev: ptr::null_mut(),
        }
    }
}

impl Default for ListLink {
    fn default() -> ListLink {
        ListLink::new()
    }
}

pub struct List {
    head: *mut ListLink,
}

impl List {
    pub fn new() -> List {
        List { head: ptr::null_mut() }
    }

    pub fn push(&mut self, link: *mut ListLink) {
        assert!(!link.is_null());
        unsafe {
            let next = self.head;

            (*link).next = next;
            (*link).prev = ptr::null_mut();

            self.head = link;
            if !next.is_null() {
                (*next).prev = link;
            }
        }
    }

    pub fn pop(&mut self) -> Option<*mut ListLink> {
        if self.head.is_null() {
            None
        } else {
            unsafe {
                let link = self.head;
                let next = (*link).next;

                self.head = next;
                if !next.is_null() {
                    (*next).prev = ptr::null_mut();
                }
                (*link).next = ptr::null_mut();
                (*link).prev = ptr::null_mut();
                Some(link)
            }
        }
    }

    pub fn remove(&mut self, link: *mut ListLink) {
        assert!(!link.is_null());
        unsafe {
            let prev = (*link).prev;
            let next = (*link).next;

            if !prev.is_null() {
                (*prev).next = next;
            }
            if !next.is_null() {
                (*next).prev = prev;
            }
            if link == self.head {
                self.head = next;
            }
            (*link).next = ptr::null_mut();
            (*link).prev = ptr::null_mut();
        }
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
    fn test_push_pop() {
        let mut l = List::new();
        let mut n1 = ListLink::new();
        let mut n2 = ListLink::new();
        let mut n3 = ListLink::new();

        assert_eq!(l.pop(), None);
        l.push(&mut n1 as *mut ListLink);
        l.push(&mut n2 as *mut ListLink);
        l.push(&mut n3 as *mut ListLink);
        assert_eq!(l.pop(), Some(&mut n3 as *mut ListLink));
        assert!(n3.prev.is_null() && n3.next.is_null());
        assert_eq!(l.pop(), Some(&mut n2 as *mut ListLink));
        assert!(n2.prev.is_null() && n2.next.is_null());
        assert_eq!(l.pop(), Some(&mut n1 as *mut ListLink));
        assert!(n1.prev.is_null() && n1.next.is_null());
        assert_eq!(l.pop(), None);
        assert!(l.head.is_null());
    }

    #[test]
    fn test_remove() {
        let mut l = List::new();
        let mut n1 = ListLink::new();
        let mut n2 = ListLink::new();
        let mut n3 = ListLink::new();

        {
            l.push(&mut n1 as *mut ListLink);
            l.push(&mut n2 as *mut ListLink);
            l.push(&mut n3 as *mut ListLink);

            l.remove(&mut n1 as *mut ListLink);
            assert!(n1.prev.is_null() && n1.next.is_null());
            assert_eq!(l.pop(), Some(&mut n3 as *mut ListLink));
            assert_eq!(l.pop(), Some(&mut n2 as *mut ListLink));
            assert_eq!(l.pop(), None);
            assert!(l.head.is_null());
        }
        {
            l.push(&mut n1 as *mut ListLink);
            l.push(&mut n2 as *mut ListLink);
            l.push(&mut n3 as *mut ListLink);

            l.remove(&mut n2 as *mut ListLink);
            assert!(n2.prev.is_null() && n2.next.is_null());
            assert_eq!(l.pop(), Some(&mut n3 as *mut ListLink));
            assert_eq!(l.pop(), Some(&mut n1 as *mut ListLink));
            assert_eq!(l.pop(), None);
            assert!(l.head.is_null());
        }
        {
            l.push(&mut n1 as *mut ListLink);
            l.push(&mut n2 as *mut ListLink);
            l.push(&mut n3 as *mut ListLink);

            l.remove(&mut n3 as *mut ListLink);
            assert!(n3.prev.is_null() && n3.next.is_null());
            assert_eq!(l.pop(), Some(&mut n2 as *mut ListLink));
            assert_eq!(l.pop(), Some(&mut n1 as *mut ListLink));
            assert_eq!(l.pop(), None);
            assert!(l.head.is_null());
        }
        {
            l.push(&mut n1 as *mut ListLink);
            l.remove(&mut n1 as *mut ListLink);
            assert!(n1.prev.is_null() && n1.next.is_null());
            assert_eq!(l.pop(), None);
            assert!(l.head.is_null());
        }
    }
}
