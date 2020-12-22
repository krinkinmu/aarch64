#![cfg_attr(not(test), no_std)]
mod visit;
mod scanner;

use core::iter::Iterator;
use core::option::Option;
use core::result::Result;
use scanner::Scanner;
use visit::DeviceTreeVisitor;

const DEVICE_TREE_SPEC_VERSION: u32 = 17;

#[derive(Copy, Clone, Debug, PartialEq)]
struct DeviceTreeHeader {
    magic: u32,
    totalsize: u32,
    off_dt_struct: u32,
    off_dt_strings: u32,
    off_mem_rsvmap: u32,
    version: u32,
    last_comp_version: u32,
    boot_cpuid_phys: u32,
    size_dt_strings: u32,
    size_dt_struct: u32,
}

pub struct DeviceTree<'a> {
    reserved_memory: &'a [u8],
    structure: &'a [u8],
    strings: &'a [u8],
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct ReservedMemory {
    address: u64,
    size: u64,
}

#[derive(Copy, Clone)]
pub struct ReservedMemoryIterator<'a> {
    scanner: Scanner<'a>,
    done: bool,
}

impl<'a> Iterator for ReservedMemoryIterator<'a> {
    type Item = ReservedMemory;

    fn next(&mut self) -> Option<ReservedMemory> {
        if self.done {
            return None;
        }

        let address = self.scanner.consume_be64().unwrap();
        let size = self.scanner.consume_be64().unwrap();

        if address == 0 && size == 0 {
            self.done = true;
            return None;
        }

        Some(ReservedMemory { address, size })
    }
}

const FDT_BEGIN_NODE: u32 = 0x01;
const FDT_END_NODE: u32 = 0x02;
const FDT_PROP: u32 = 0x03;
const FDT_NOP: u32 = 0x04;
const FDT_END: u32 = 0x09;

impl<'a> DeviceTree<'a> {
    pub fn new(data: &[u8]) -> Result<DeviceTree, &'static str> {
        let header = DeviceTree::parse_header(data)?;

        if header.magic != 0xd00dfeed {
            return Err("Incorrect DeviceTree magic value");
        }
        if header.last_comp_version > DEVICE_TREE_SPEC_VERSION {
            return Err("DeviceTree version is too new");
        }
        if header.totalsize as usize > data.len() {
            return Err("Actual DeviceTree size is too small");
        }

        let begin = header.off_dt_struct as usize;
        let end = begin + header.size_dt_struct as usize;
        let structure = &data[begin..end];

        let begin = header.off_dt_strings as usize;
        let end = begin + header.size_dt_strings as usize;
        let strings = &data[begin..end];

        let begin = header.off_mem_rsvmap as usize;
        let reserved_memory = &data[begin..];

        Ok(DeviceTree { reserved_memory, structure, strings })
    }

    fn parse_header(data: &[u8]) -> Result<DeviceTreeHeader, &'static str> {
        let mut scanner = Scanner::new(data);
        let magic = scanner.consume_be32()?;
        let totalsize = scanner.consume_be32()?;
        let off_dt_struct = scanner.consume_be32()?;
        let off_dt_strings = scanner.consume_be32()?;
        let off_mem_rsvmap = scanner.consume_be32()?;
        let version = scanner.consume_be32()?;
        let last_comp_version = scanner.consume_be32()?;
        let boot_cpuid_phys = scanner.consume_be32()?;
        let size_dt_strings = scanner.consume_be32()?;
        let size_dt_struct = scanner.consume_be32()?;

        Ok(DeviceTreeHeader {
            magic,
            totalsize,
            off_dt_struct,
            off_dt_strings,
            off_mem_rsvmap,
            version,
            last_comp_version,
            boot_cpuid_phys,
            size_dt_strings,
            size_dt_struct,
        })
    }

    pub fn reserved_memory_iter(&self) -> ReservedMemoryIterator {
        ReservedMemoryIterator {
            scanner: Scanner::new(self.reserved_memory),
            done: false,
        }
    }

    pub fn parse<V>(&self, visitor: &mut V) -> Result<(), &'static str>
        where V: DeviceTreeVisitor
    {
        let mut scanner = Scanner::new(self.structure);

        loop {
            match scanner.consume_be32() {
                Ok(token) if token == FDT_BEGIN_NODE => {
                    let name = scanner.consume_cstr()?;
                    scanner.align_forward(4)?;
                    visitor.begin_node(name);
                },
                Ok(token) if token == FDT_END_NODE => visitor.end_node(),
                Ok(token) if token == FDT_PROP => {
                    let len = scanner.consume_be32()?;
                    let off = scanner.consume_be32()?;
                    let value = scanner.consume_data(len as usize)?;
                    let name = Scanner::new(
                        &self.strings[off as usize..]).consume_cstr()?;
                    scanner.align_forward(4)?;
                    visitor.property(name, value);
                },
                Ok(token) if token == FDT_NOP => visitor.nop(),
                Ok(token) if token == FDT_END => {
                    visitor.end();
                    break;
                },
                Err(msg) => return Err(msg),
                _ => return Err("Invalid Device Tree token"),
            }
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem;
    use std::slice;
    use std::iter::FromIterator;
    use std::vec::Vec;

    #[derive(Copy, Clone, Debug)]
    #[repr(C)]
    struct PackedDeviceTreeHeader {
        magic: u32,
        totalsize: u32,
        off_dt_struct: u32,
        off_dt_strings: u32,
        off_mem_rsvmap: u32,
        version: u32,
        last_comp_version: u32,
        boot_cpuid_phys: u32,
        size_dt_strings: u32,
        size_dt_struct: u32,
    }

    fn make_header(magic: u32, version: u32, size: u32) -> Vec<u8> {
        let header = PackedDeviceTreeHeader {
            magic: magic.to_be(),
            totalsize: size.to_be(),
            off_dt_struct: 0x0u32.to_be(),
            off_dt_strings: 0x0u32.to_be(),
            off_mem_rsvmap: 0x0u32.to_be(),
            version: version.to_be(),
            last_comp_version: version.to_be(),
            boot_cpuid_phys: 0x0u32.to_be(),
            size_dt_strings: 0x0u32.to_be(),
            size_dt_struct: 0x0u32.to_be(),
        };
        let data = unsafe {
            slice::from_raw_parts(
                (&header as *const PackedDeviceTreeHeader) as *const u8,
                mem::size_of::<PackedDeviceTreeHeader>())
        };

        Vec::<u8>::from(data)
    }

    #[test]
    fn test_new() {
        let header_size = mem::size_of::<PackedDeviceTreeHeader>() as u32;
        assert!(DeviceTree::new(
            make_header(0xd00dfeed, 17, header_size).as_slice()).is_ok());
        assert!(DeviceTree::new(
            make_header(0xd00dfee, 17, header_size).as_slice()).is_err());
        assert!(DeviceTree::new(
            make_header(0xd00dfeed, 18, header_size).as_slice()).is_err());
        assert!(DeviceTree::new(
            make_header(0xd00dfeed, 17, header_size + 1).as_slice()).is_err());
    }

    #[test]
    fn test_parse_header() {
        assert!(DeviceTree::parse_header(&[]).is_err());

        let header = PackedDeviceTreeHeader {
            magic: 0xd00dfeedu32.to_be(),
            totalsize: 0x1u32.to_be(),
            off_dt_struct: 0x2u32.to_be(),
            off_dt_strings: 0x3u32.to_be(),
            off_mem_rsvmap: 0x4u32.to_be(),
            version: 0x5u32.to_be(),
            last_comp_version: 0x6u32.to_be(),
            boot_cpuid_phys: 0x7u32.to_be(),
            size_dt_strings: 0x8u32.to_be(),
            size_dt_struct: 0x9u32.to_be(),
        };
        let data = unsafe {
            slice::from_raw_parts(
                (&header as *const PackedDeviceTreeHeader) as *const u8,
                mem::size_of::<PackedDeviceTreeHeader>())
        };

        assert_eq!(
            DeviceTree::parse_header(data),
            Ok(DeviceTreeHeader {
                magic: 0xd00dfeedu32,
                totalsize: 0x1u32,
                off_dt_struct: 0x2u32,
                off_dt_strings: 0x3u32,
                off_mem_rsvmap: 0x4u32,
                version: 0x5u32,
                last_comp_version: 0x6u32,
                boot_cpuid_phys: 0x7u32,
                size_dt_strings: 0x8u32,
                size_dt_struct: 0x9u32,
            }));
    }

    #[test]
    fn test_reserved_memory_iter() {
        let dtb = include_bytes!("test.dtb");
        let dt = DeviceTree::new(&dtb[..]).unwrap();
        let mut got = Vec::<ReservedMemory>::from_iter(
            dt.reserved_memory_iter());
        got.sort();
        let mut want = vec![
            ReservedMemory { address: 0x40000000, size: 0x1000, },
            ReservedMemory { address: 0x40002000, size: 0x1000, },
            ReservedMemory { address: 0x40004000, size: 0x1000, },
        ];
        want.sort();
        assert_eq!(got, want);
    }

    #[derive(Debug, PartialEq, Eq)]
    struct DeviceTreeNode {
        name: String,
        nodes: Vec<DeviceTreeNode>,
    }

    struct DeviceTreeParser {
        parents: Vec<DeviceTreeNode>,
        current: DeviceTreeNode,
    }

    impl DeviceTreeParser {
        fn new() -> DeviceTreeParser {
            DeviceTreeParser {
                parents: Vec::new(),
                current: DeviceTreeNode{
                    name: String::from(""),
                    nodes: Vec::new(),
                },
            }
        }
    }

    impl DeviceTreeVisitor for DeviceTreeParser {
        fn begin_node(&mut self, node_name: &str) {
            let current = mem::replace(
                &mut self.current,
                DeviceTreeNode {
                    name: String::from(node_name),
                    nodes: Vec::new(),
                });
            self.parents.push(current);
        }

        fn end_node(&mut self) {
            let parent = self.parents.pop().unwrap();
            let current = mem::replace(&mut self.current, parent);
            self.current.nodes.push(current);
        }
    }

    #[test]
    fn test_parse() {
        let dtb = include_bytes!("test.dtb");
        let dt = DeviceTree::new(&dtb[..]).unwrap();
        let mut parser = DeviceTreeParser::new();

        assert_eq!(dt.parse(&mut parser), Ok(()));
        assert_eq!(
            parser.current.nodes[0],
            DeviceTreeNode {
                name: String::from(""),
                nodes: vec![
                    DeviceTreeNode {
                        name: String::from("memory@40000000"),
                        nodes: Vec::new(),
                    },
                    DeviceTreeNode {
                        name: String::from("cpus"),
                        nodes: vec![
                            DeviceTreeNode {
                                name: String::from("cpu@0"),
                                nodes: Vec::new(),
                            },
                        ],
                    },
                ],
            });
    }
}
