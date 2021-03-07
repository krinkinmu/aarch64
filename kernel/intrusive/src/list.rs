use core::cell::Cell;
use core::option::Option;
use core::ptr;
use crate::StructFieldOffset;

#[derive(Copy, Clone, Debug)]
struct IntrusiveListNode {
    next: *const IntrusiveListLink,
    prev: *const IntrusiveListLink,
}

#[derive(Debug)]
pub struct IntrusiveListLink {
    link: Cell<IntrusiveListNode>,
}

impl IntrusiveListLink {
    pub fn new() -> IntrusiveListLink {
        IntrusiveListLink {
            link: Cell::new(IntrusiveListNode {
                next: ptr::null(),
                prev: ptr::null(),
            }),
        }
    }

    fn next(&self) -> *const IntrusiveListLink {
        self.link.get().next
    }

    fn set_next(&self, next: *const IntrusiveListLink) {
        let mut link = self.link.get();
        link.next = next;
        self.link.set(link);
    }

    fn prev(&self) -> *const IntrusiveListLink {
        self.link.get().prev
    }

    fn set_prev(&self, prev: *const IntrusiveListLink) {
        let mut link = self.link.get();
        link.prev = prev;
        self.link.set(link);
    }

    fn set(&self, node: IntrusiveListNode) {
        self.link.set(node);
    }
}

#[derive(Debug)]
pub struct IntrusiveList<S> {
    head: *const IntrusiveListLink,
    tail: *const IntrusiveListLink,
    offset: StructFieldOffset<S, IntrusiveListLink>,
}

impl<S> IntrusiveList<S> {
    pub fn new(offset: StructFieldOffset<S, IntrusiveListLink>)
        -> IntrusiveList<S>
    {
        IntrusiveList {
            head: ptr::null(),
            tail: ptr::null(),
            offset: offset,
        }
    }

    pub fn is_empty(&self) -> bool {
        self.head == ptr::null()
    }

    pub unsafe fn push(&mut self, item: *const S) {
        self.push_link(self.offset.field(item));
    }

    pub unsafe fn pop(&mut self) -> Option<*const S> {
        self.pop_link().map(|link| self.offset.container_of(link))
    }

    pub unsafe fn remove(&mut self, item: *const S) {
        self.remove_link(self.offset.field(item));
    }

    unsafe fn push_link(&mut self, ptr: *const IntrusiveListLink) {
        (*ptr).set(IntrusiveListNode {
            next: self.head,
            prev: ptr::null(),
        });

        if self.head == ptr::null() {
            self.tail = ptr;
        } else {
            (*self.head).set_prev(ptr);
        }
        self.head = ptr;
    }

    unsafe fn pop_link(&mut self) -> Option<*const IntrusiveListLink> {
        if self.head == ptr::null() {
            return None;
        }

        let ptr = self.head;
        let node = &*ptr;

        if self.head == self.tail {
            self.head = ptr::null();
            self.tail = ptr::null();
        } else {
            self.head = node.next();
            (*node.next()).set_prev(ptr::null());
        }

        node.set(IntrusiveListNode {
            next: ptr::null(),
            prev: ptr::null(),
        });
        Some(ptr)
    }

    unsafe fn remove_link(&mut self, ptr: *const IntrusiveListLink) {
        let node = &*ptr;
        let prev = node.prev();
        let next = node.next();

        node.set(IntrusiveListNode {
            next: ptr::null(),
            prev: ptr::null(),
        });

        if prev != ptr::null() { (*prev).set_next(next); }
        if next != ptr::null() { (*next).set_prev(prev); }

        if self.head == ptr { self.head = next; }
        if self.tail == ptr { self.tail = prev; }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    struct ListNode {
        _placeholder: i32,
        link: IntrusiveListLink,
    }

    impl ListNode {
        fn new() -> ListNode {
            ListNode {
                _placeholder: 0,
                link: IntrusiveListLink::new(),
            }
        }

        fn offset(&self) -> StructFieldOffset<ListNode, IntrusiveListLink> {
            StructFieldOffset::new(self as *const ListNode, &self.link as *const IntrusiveListLink)
        }
    }

    #[test]
    fn test_push_pop() {
        unsafe {
            let nodes = [ListNode::new(), ListNode::new(), ListNode::new()];
            let mut list = IntrusiveList::new(nodes[0].offset());

            assert_eq!(list.pop(), None);

            list.push(&nodes[0] as *const ListNode);
            assert_eq!(list.pop(), Some(&nodes[0] as *const ListNode));
            assert_eq!(list.pop(), None);

            list.push(&nodes[0] as *const ListNode);
            list.push(&nodes[1] as *const ListNode);
            assert_eq!(list.pop(), Some(&nodes[1] as *const ListNode));
            assert_eq!(list.pop(), Some(&nodes[0] as *const ListNode));
            assert_eq!(list.pop(), None);

            list.push(&nodes[0] as *const ListNode);
            list.push(&nodes[1] as *const ListNode);
            list.push(&nodes[2] as *const ListNode);
            assert_eq!(list.pop(), Some(&nodes[2] as *const ListNode));
            assert_eq!(list.pop(), Some(&nodes[1] as *const ListNode));
            assert_eq!(list.pop(), Some(&nodes[0] as *const ListNode));
            assert_eq!(list.pop(), None);
        }
    }

    #[test]
    fn test_remove() {
        unsafe {
            let nodes = [ListNode::new(), ListNode::new(), ListNode::new()];
            let mut list = IntrusiveList::new(nodes[0].offset());

            {
                assert_eq!(list.pop(), None);
                list.push(&nodes[0] as *const ListNode);
                list.push(&nodes[1] as *const ListNode);
                list.push(&nodes[2] as *const ListNode);
                list.remove(&nodes[0] as *const ListNode);
                assert_eq!(list.pop(), Some(&nodes[2] as *const ListNode));
                assert_eq!(list.pop(), Some(&nodes[1] as *const ListNode));
                assert_eq!(list.pop(), None);
            }
            {
                assert_eq!(list.pop(), None);
                list.push(&nodes[0] as *const ListNode);
                list.push(&nodes[1] as *const ListNode);
                list.push(&nodes[2] as *const ListNode);
                list.remove(&nodes[1] as *const ListNode);
                assert_eq!(list.pop(), Some(&nodes[2] as *const ListNode));
                assert_eq!(list.pop(), Some(&nodes[0] as *const ListNode));
                assert_eq!(list.pop(), None);
            }
            {
                assert_eq!(list.pop(), None);
                list.push(&nodes[0] as *const ListNode);
                list.push(&nodes[1] as *const ListNode);
                list.push(&nodes[2] as *const ListNode);
                list.remove(&nodes[2] as *const ListNode);
                assert_eq!(list.pop(), Some(&nodes[1] as *const ListNode));
                assert_eq!(list.pop(), Some(&nodes[0] as *const ListNode));
                assert_eq!(list.pop(), None);
            }
        }
    }
}
