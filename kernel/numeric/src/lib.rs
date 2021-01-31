#![no_std]

pub fn lsb(mut x: u64) -> u32 {
    if x == 0 {
        return u32::MAX;
    }

    let mut r = 0;
    if (x & 0xffffffff) == 0 {
        x >>= 32;
        r += 32;
    }
    if (x & 0xffff) == 0 {
        x >>= 16;
        r += 16;
    }
    if (x & 0xff) == 0 {
        x >>= 8;
        r += 8;
    }
    if (x & 0xf) == 0 {
        x >>= 4;
        r += 4;
    }
    if (x & 0x3) == 0 {
        x >>= 2;
        r += 2;
    }
    if (x & 0x1) == 0 {
        r += 1;
    }
    r
}

pub fn msb(mut x: u64) -> u32 {
    if x == 0 {
        return 0;
    }

    let mut r = 64;
    if (x & 0xffffffff00000000) == 0 {
        x <<= 32;
        r -= 32;
    }
    if (x & 0xffff000000000000) == 0 {
        x <<= 16;
        r -= 16;
    }
    if (x & 0xff00000000000000) == 0 {
        x <<= 8;
        r -= 8;
    }
    if (x & 0xf000000000000000) == 0 {
        x <<= 4;
        r -= 4;
    }
    if (x & 0xc000000000000000) == 0 {
        x <<= 2;
        r -= 2;
    }
    if (x & 0x8000000000000000) == 0 {
        r -= 1;
    }
    r
}

pub fn ilog2(x: u64) -> u32 {
    assert_ne!(x, 0);
    msb(x) - 1
}

pub fn align_down(x: u64, align: u64) -> u64 {
    assert_eq!(align & (align - 1), 0);
    x & !(align - 1)
}

pub fn align_up(x: u64, align: u64) -> u64 {
    align_down(x + align - 1, align)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_align_down() {
        assert_eq!(align_down(0, 2), 0);
        assert_eq!(align_down(1, 2), 0);
        assert_eq!(align_down(2, 2), 2);
        assert_eq!(align_down(3, 2), 2);
        assert_eq!(align_down(4, 2), 4);
    }

    #[test]
    fn test_align_up() {
        assert_eq!(align_up(0, 2), 0);
        assert_eq!(align_up(1, 2), 2);
        assert_eq!(align_up(2, 2), 2);
        assert_eq!(align_up(3, 2), 4);
        assert_eq!(align_up(4, 2), 4);
    }

    #[test]
    fn test_msb() {
        assert_eq!(msb(0), 0);
        assert_eq!(msb(1), 1);
        assert_eq!(msb(2), 2);
        assert_eq!(msb(3), 2);
        assert_eq!(msb(4), 3);
        assert_eq!(msb(5), 3);
        assert_eq!(msb(6), 3);
        assert_eq!(msb(7), 3);
        assert_eq!(msb(8), 4);
    }

    #[test]
    fn test_ilog2() {
        assert_eq!(ilog2(1), 0);
        assert_eq!(ilog2(2), 1);
        assert_eq!(ilog2(3), 1);
        assert_eq!(ilog2(4), 2);
        assert_eq!(ilog2(5), 2);
        assert_eq!(ilog2(6), 2);
        assert_eq!(ilog2(7), 2);
        assert_eq!(ilog2(8), 3);
    }
}
