use core::convert::TryFrom;
use core::result::Result;
use core::str;
use crate::AddressSpace;

pub struct Scanner<'a> {
    data: &'a [u8],
    offset: usize,
}

impl<'a> Scanner<'a> {
    pub fn new(data: &'a [u8]) -> Scanner<'a> {
        Scanner { data, offset: 0 }
    }

    pub fn remains(&self) -> usize {
        self.data.len() - self.offset
    }

    pub fn consume_be32(&mut self) -> Result<u32, &'static str> {
        if self.offset + 4 > self.data.len() {
            return Err("Not enough data");
        }

        let value = &self.data[self.offset..self.offset + 4];
        match <[u8; 4]>::try_from(value) {
            Ok(v) => {
                self.offset += value.len();
                Ok(u32::from_be_bytes(v))
            },
            Err(_) => Err("Error while parsing 4-byte big-endian"),
        }
    }

    pub fn consume_be64(&mut self) -> Result<u64, &'static str> {
        if self.offset + 8 > self.data.len() {
            return Err("Not enough data");
        }

        let value = &self.data[self.offset..self.offset + 8];
        match <[u8; 8]>::try_from(value) {
            Ok(v) => {
                self.offset += value.len();
                Ok(u64::from_be_bytes(v))
            },
            Err(_) => Err("Error while parsing 8-byte big-endian"),
        }
    }

    pub fn consume_address(&mut self, address_space: &AddressSpace)
        -> Result<u64, &'static str>
    {
        match address_space.address_cells {
            0 => Ok(0),
            1 => self.consume_be32().map(|x| x as u64),
            2 => self.consume_be64(),
            _ => Err("Unsupported #address-cells value."),
        }
    }

    pub fn consume_size(&mut self, address_space: &AddressSpace)
        -> Result<u64, &'static str>
    {
        match address_space.size_cells {
            0 => Ok(0),
            1 => self.consume_be32().map(|x| x as u64),
            2 => self.consume_be64(),
            _ => Err("Unsupported #address-cells value."),
        }
    }

    pub fn consume_cstr(&mut self) -> Result<&'a str, &'static str> {
        for i in self.offset.. {
            if i >= self.data.len() {
                return Err("Failed to find terminating '\0' in the data");
            }

            if self.data[i] != 0 {
                continue;
            }

            match str::from_utf8(&self.data[self.offset..i]) {
                Ok(s) => {
                    self.offset = i + 1;
                    return Ok(s);
                },
                Err(_) => return Err("Not a valid UTF8 string"),
            }
        }
        Err("Unreachable")
    }

    pub fn consume_data(&mut self, size: usize)
        -> Result<&'a [u8], &'static str>
    {
        if self.offset + size > self.data.len() {
            return Err("Not enough data");
        }

        let begin = self.offset;
        let end = begin + size;
        self.offset += size;

        Ok(&self.data[begin..end])
    }

    pub fn align_forward(&mut self, alignment: usize)
        -> Result<(), &'static str>
    {
        if alignment == 0 || self.offset % alignment == 0 {
            return Ok(());
        }

        let shift = alignment - self.offset % alignment;

        if self.offset + shift >= self.data.len() {
            return Err("Not enough data");
        }

        self.offset += shift;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_consume_be32() {
        assert!(Scanner::new(&[]).consume_be32().is_err());
        assert!(Scanner::new(&[0]).consume_be32().is_err());
        assert!(Scanner::new(&[0, 0]).consume_be32().is_err());
        assert!(Scanner::new(&[0, 0, 0]).consume_be32().is_err());
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0]).consume_be32(),
            Ok(0u32));
        assert_eq!(
            Scanner::new(&[0xff, 0, 0, 0]).consume_be32(),
            Ok(0xff000000u32));
        assert_eq!(
            Scanner::new(&[0, 0xff, 0, 0]).consume_be32(),
            Ok(0x00ff0000u32));
        assert_eq!(
            Scanner::new(&[0, 0, 0xff, 0]).consume_be32(),
            Ok(0x0000ff00u32));
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0xff]).consume_be32(),
            Ok(0x000000ffu32));
    }

    #[test]
    fn test_consume_be64() {
        assert!(Scanner::new(&[]).consume_be64().is_err());
        assert!(Scanner::new(&[0]).consume_be64().is_err());
        assert!(Scanner::new(&[0, 0]).consume_be64().is_err());
        assert!(Scanner::new(&[0, 0, 0]).consume_be64().is_err());
        assert!(Scanner::new(&[0, 0, 0, 0]).consume_be64().is_err());
        assert!(Scanner::new(&[0, 0, 0, 0, 0]).consume_be64().is_err());
        assert!(Scanner::new(&[0, 0, 0, 0, 0, 0]).consume_be64().is_err());
        assert!(Scanner::new(&[0, 0, 0, 0, 0, 0, 0]).consume_be64().is_err());
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0, 0, 0, 0, 0]).consume_be64(),
            Ok(0u64));
        assert_eq!(
            Scanner::new(&[0xff, 0, 0, 0, 0, 0, 0, 0]).consume_be64(),
            Ok(0xff00000000000000u64));
        assert_eq!(
            Scanner::new(&[0, 0xff, 0, 0, 0, 0, 0, 0]).consume_be64(),
            Ok(0x00ff000000000000u64));
        assert_eq!(
            Scanner::new(&[0, 0, 0xff, 0, 0, 0, 0, 0]).consume_be64(),
            Ok(0x0000ff0000000000u64));
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0xff, 0, 0, 0, 0]).consume_be64(),
            Ok(0x000000ff00000000u64));
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0, 0xff, 0, 0, 0]).consume_be64(),
            Ok(0x00000000ff000000u64));
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0, 0, 0xff, 0, 0]).consume_be64(),
            Ok(0x0000000000ff0000u64));
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0, 0, 0, 0xff, 0]).consume_be64(),
            Ok(0x000000000000ff00u64));
        assert_eq!(
            Scanner::new(&[0, 0, 0, 0, 0, 0, 0, 0xff]).consume_be64(),
            Ok(0x00000000000000ffu64));
    }

    #[test]
    fn test_consume_cstr() {
        assert!(Scanner::new(&[]).consume_cstr().is_err());
        assert_eq!(Scanner::new(&[0]).consume_cstr(), Ok(""));
        assert_eq!(Scanner::new(&['H' as u8, 'i' as u8, 0]).consume_cstr(), Ok("Hi"));
    }

    #[test]
    fn test_align_forward() {
        let mut scanner = Scanner::new(&[1, 2, 3, 4, 5, 6, 7, 8]);

        assert_eq!(scanner.align_forward(0), Ok(()));
        assert_eq!(scanner.offset, 0);
        assert_eq!(scanner.align_forward(1), Ok(()));
        assert_eq!(scanner.offset, 0);
        assert_eq!(scanner.align_forward(2), Ok(()));
        assert_eq!(scanner.offset, 0);
        assert_eq!(scanner.align_forward(3), Ok(()));
        assert_eq!(scanner.offset, 0);
        assert_eq!(scanner.align_forward(4), Ok(()));
        assert_eq!(scanner.offset, 0);

        assert_eq!(scanner.consume_data(1), Ok(&[1u8][..]));
        assert_eq!(scanner.align_forward(0), Ok(()));
        assert_eq!(scanner.offset, 1);
        assert_eq!(scanner.align_forward(1), Ok(()));
        assert_eq!(scanner.offset, 1);
        assert_eq!(scanner.align_forward(2), Ok(()));
        assert_eq!(scanner.offset, 2);
        assert_eq!(scanner.align_forward(2), Ok(()));
        assert_eq!(scanner.offset, 2);
        assert_eq!(scanner.align_forward(3), Ok(()));
        assert_eq!(scanner.offset, 3);
        assert_eq!(scanner.align_forward(3), Ok(()));
        assert_eq!(scanner.offset, 3);
    }
}
