#![cfg_attr(not(test), no_std)]
extern crate alloc;

mod scanner;

use alloc::collections::btree_map::{BTreeMap, Iter};
use alloc::string::String;
use alloc::vec::Vec;
use core::iter::Iterator;
use core::mem;
use core::option::Option;
use core::result::Result;
use scanner::Scanner;

const DEVICE_TREE_SPEC_VERSION: u32 = 17;

#[derive(Clone, Debug)]
pub struct DeviceTreeNode {
    children: BTreeMap<String, DeviceTreeNode>,
    properties: BTreeMap<String, Vec<u8>>,
}

impl DeviceTreeNode {
    fn new() -> DeviceTreeNode {
        DeviceTreeNode{
            children: BTreeMap::new(),
            properties: BTreeMap::new(),
        }
    }

    fn add_child(&mut self, name: String, child: DeviceTreeNode) {
        self.children.insert(name, child);
    }

    pub fn find_child(&self, name: &str) -> Option<&DeviceTreeNode> {
        self.children.get(name)
    }

    pub fn children(&self) -> Children {
        Children{ inner: self.children.iter() }
    }

    fn add_property(&mut self, name: String, value: Vec<u8>) {
        self.properties.insert(name, value);
    }

    pub fn properties(&self) -> Properties {
        Properties{ inner: self.properties.iter() }
    }
}

#[derive(Clone, Debug)]
pub struct Children<'a> {
    inner: Iter<'a, String, DeviceTreeNode>,
}

impl<'a> Iterator for Children<'a> {
    type Item = (&'a str, &'a DeviceTreeNode);

    fn next(&mut self) -> Option<(&'a str, &'a DeviceTreeNode)> {
        if let Some((name, node)) = self.inner.next() {
            Some((name.as_str(), node))
        } else {
            None
        }
    }
}

#[derive(Clone, Debug)]
pub struct Properties<'a> {
    inner: Iter<'a, String, Vec<u8>>,
}

impl<'a> Iterator for Properties<'a> {
    type Item = (&'a str, &'a [u8]);

    fn next(&mut self) -> Option<(&'a str, &'a [u8])> {
        if let Some((name, value)) = self.inner.next() {
            Some((name.as_str(), value.as_slice()))
        } else {
            None
        }
    }
}

#[derive(Copy, Clone, Debug)]
pub struct ReservedMemory {
    addr: u64,
    size: u64,
}

impl ReservedMemory {
    pub fn begin(&self) -> u64 {
        self.addr
    }

    pub fn end(&self) -> u64 {
        self.addr + self.size
    }

    pub fn size(&self) -> u64 {
        self.size
    }
}


#[derive(Clone, Debug)]
pub struct DeviceTree {
    reserved: Vec<ReservedMemory>,
    root: DeviceTreeNode,
    last_comp_version: u32,
    boot_cpuid_phys: u32,
}

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

impl DeviceTree {
    pub fn new(data: &[u8]) -> Result<DeviceTree, &'static str> {
        let header = DeviceTree::parse_header(data)?;

        if header.magic != 0xd00dfeed {
            return Err("Incorrect DeviceTree magic value.");
        }
        if header.last_comp_version > DEVICE_TREE_SPEC_VERSION {
            return Err("DeviceTree version is too new and not supported.");
        }
        if header.totalsize as usize > data.len() {
            return Err("The data blob size is too small to fit Device Tree.");
        }

        let reserved = DeviceTree::parse_reserved(
            &data[header.off_mem_rsvmap as usize..])?;

        let begin = header.off_dt_struct as usize;
        let end = begin + header.size_dt_struct as usize;
        let structs = &data[begin..end];

        let begin = header.off_dt_strings as usize;
        let end = begin + header.size_dt_strings as usize;
        let strings = &data[begin..end];

        let root = DeviceTree::parse_nodes(structs, strings)?;

        Ok(DeviceTree{
            reserved,
            root,
            last_comp_version: header.last_comp_version,
            boot_cpuid_phys: header.boot_cpuid_phys,
        })
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

        Ok(DeviceTreeHeader{
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

    fn parse_reserved(data: &[u8])
            -> Result<Vec<ReservedMemory>, &'static str> {
        let mut scanner = Scanner::new(data);
        let mut reserved = Vec::new();

        loop {
            let addr = scanner.consume_be64()?;
            let size = scanner.consume_be64()?;

            if addr == 0 && size == 0 {
                break;
            }
            reserved.push(ReservedMemory{ addr, size });
        }

        Ok(reserved)
    }

    fn parse_nodes(structs: &[u8], strings: &[u8])
            -> Result<DeviceTreeNode, &'static str> {
        const FDT_BEGIN_NODE: u32 = 0x01;
        const FDT_END_NODE: u32 = 0x02;
        const FDT_PROP: u32 = 0x03;
        const FDT_NOP: u32 = 0x04;
        const FDT_END: u32 = 0x09;

        let mut scanner = Scanner::new(structs);
        let mut parents = Vec::new();
        let mut current = DeviceTreeNode::new();

        loop {
            match scanner.consume_be32() {
                Ok(token) if token == FDT_BEGIN_NODE => {
                    let name = scanner.consume_cstr()?;
                    let node = mem::replace(&mut current, DeviceTreeNode::new());
                    parents.push((String::from(name), node));
                    scanner.align_forward(4)?;
                },
                Ok(token) if token == FDT_END_NODE => {
                    if let Some((name, parent)) = parents.pop() {
                        let node = mem::replace(&mut current, parent);
                        current.add_child(name, node);
                    }
                    return Err("Unmatched end of the node token found.");
                },
                Ok(token) if token == FDT_PROP => {
                    let len = scanner.consume_be32()? as usize;
                    let off = scanner.consume_be32()? as usize;
                    let value = scanner.consume_data(len)?;
                    let name = Scanner::new(&strings[off..]).consume_cstr()?;
                    current.add_property(String::from(name), Vec::from(value));
                    scanner.align_forward(4)?;
                },
                Ok(token) if token == FDT_NOP => {
                },
                Ok(token) if token == FDT_END => {
                    if !parents.is_empty() {
                        return Err("Unexpected END token found inside Device Tree node.");
                    }
                    if let None = current.find_child("") {
                        return Err("Device Tree doesn't have the mandatory root node.");
                    }
                    return Ok(current);
                },
                Err(msg) => return Err(msg),
                _ => return Err("Unknown Device Tree token."),
            }
        }
    }

    pub fn reserved_memory(&self) -> &[ReservedMemory] {
        self.reserved.as_slice()
    }

    pub fn follow(&self, path: &str) -> Option<&DeviceTreeNode> {
        let mut current = &self.root;

        if path == "/" {
            return current.find_child("");
        }

        for name in path.split("/") {
            if let Some(node) = current.find_child(name) {
                current = node;
            } else {
                return None;
            }
        }

        Some(current)
    }

    pub fn compatible(&self, version: u32) -> bool {
        version <= DEVICE_TREE_SPEC_VERSION
            && version >= self.last_comp_version
    }

    pub fn boot_cpuid(&self) -> u32 {
        self.boot_cpuid_phys
    }
}

