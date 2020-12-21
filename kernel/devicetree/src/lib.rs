#![cfg_attr(not(test), no_std)]
mod visit;

use core::result::Result;
use visit::DeviceTreeVisitor;

pub struct DeviceTree<'a> {
    data: &'a [u8],
}

#[derive(Copy, Clone, Debug, PartialEq)]
#[repr(C)]
pub struct ReservedMemory {
    address: u64,
    size: u64,
}

impl<'a> DeviceTree<'a> {
    pub fn new(data: &[u8]) -> Result<DeviceTree, &'static str> {
        Ok(DeviceTree { data })
    }

    pub fn reserved_memory(&self) -> &[ReservedMemory] {
        &[]
    }

    pub fn parse<V>(&self, _visitor: &mut V) -> Result<(), &'static str>
        where V: DeviceTreeVisitor
    {
        Err("A disaster")
    }

    pub fn get_data(&self) -> &[u8] {
        self.data
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use visit::NoopVisitor;

    #[test]
    fn test_basics() {
        assert!(DeviceTree::new(&[]).is_ok());
        assert_eq!(DeviceTree::new(&[]).unwrap().get_data(), &[]);
        assert_eq!(DeviceTree::new(&[]).unwrap().reserved_memory(), &[]);
        assert!(DeviceTree::new(&[]).unwrap().parse(&mut NoopVisitor{}).is_err());
    }
}
