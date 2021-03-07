#![no_std]
mod list;

use core::marker::PhantomData;
pub use list::{IntrusiveList, IntrusiveListLink};

#[derive(Copy, Clone, Debug)]
pub struct StructFieldOffset<S, F> {
    offset: usize,
    _sm: PhantomData<*const S>,
    _fm: PhantomData<*const F>,
}

impl<S, F> StructFieldOffset<S, F> {
    pub fn new(s: *const S, f: *const F) -> StructFieldOffset<S, F> {
        StructFieldOffset {
            offset: f as usize - s as usize,
            _sm: PhantomData,
            _fm: PhantomData,
        }
    }

    pub fn field(&self, s: *const S) -> *const F {
        (s as usize + self.offset) as *const F
    }

    pub fn field_mut(&self, s: *mut S) -> *mut F {
        (s as usize + self.offset) as *mut F
    }

    pub fn container_of(&self, f: *const F) -> *const S {
        (f as usize - self.offset) as *const S
    }

    pub fn container_of_mut(&self, f: *mut F) -> *mut S {
        (f as usize - self.offset) as *mut S
    }
}

