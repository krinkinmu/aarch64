use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};

pub struct Mutex<T> {
    data: UnsafeCell<T>,
}

impl<T> Mutex<T> {
    pub fn new(data: T) -> Mutex<T> {
        Mutex { data: UnsafeCell::new(data) }
    }

    pub fn lock(&self) -> Lock<'_, T> {
        unsafe { Lock::new(self) }
    }
}

pub struct Lock<'a, T> {
    mutex: &'a Mutex<T>,
}

impl<'a, T> Lock<'a, T> {
    unsafe fn new(mutex: &'a Mutex<T>) -> Lock<'a, T> {
        Lock { mutex }
    }
}

impl<'a, T> Deref for Lock<'a, T> {
    type Target = T;

    fn deref(&self) -> &T {
        unsafe { &*self.mutex.data.get() }
    }
}

impl<'a, T> DerefMut for Lock<'a, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.mutex.data.get() }
    }
}

// There is nothing to do in Drop in a placeholder Lock because it's not a
// real lock, but in a real one we'd have to actually drop the lock.
impl<'a, T> Drop for Lock<'a, T> {
    fn drop(&mut self) {}
}

#[cfg(test)]
mod tests {
    use super::*;

    struct Data {
        data: isize,
    }

    #[test]
    fn test_mutex() {
        let mutex = Mutex::new(Data { data: 0 });
        {
            let lock = mutex.lock();
            assert_eq!(lock.data, 0);
        }
        {
            let mut lock = mutex.lock();
            lock.data = 42;
        }
        {
            let lock = mutex.lock();
            assert_eq!(lock.data, 42);
        }
        {
            let mut lock = mutex.lock();
            *lock = Data { data: 0 };
        }
        {
            let lock = mutex.lock();
            assert_eq!(lock.data, 0);
        }
    }
}
