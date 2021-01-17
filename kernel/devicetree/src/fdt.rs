use alloc::string::String;
use alloc::vec::Vec;
use core::mem;
use core::result::Result;

use crate::{
    DEVICE_TREE_SPEC_VERSION, DeviceTree, DeviceTreeNode, ReservedMemory};
use crate::scanner::Scanner;

#[derive(Clone, Copy, Debug)]
struct FDTHeader {
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

pub fn parse(fdt: &[u8]) -> Result<DeviceTree, &'static str> {
    let header = parse_header(fdt)?;

    if header.magic != 0xd00dfeed {
        return Err("Incorrect FDT magic value.");
    }
    if header.last_comp_version > DEVICE_TREE_SPEC_VERSION {
        return Err("FDT version is too new and not supported.");
    }
    if header.totalsize as usize > fdt.len() {
        return Err("The FDT size is too small to fit the Device Tree.");
    }

    let reserved = parse_reserved(&fdt[header.off_mem_rsvmap as usize..])?;

    let begin = header.off_dt_struct as usize;
    let end = begin + header.size_dt_struct as usize;
    let structs = &fdt[begin..end];

    let begin = header.off_dt_strings as usize;
    let end = begin + header.size_dt_strings as usize;
    let strings = &fdt[begin..end];

    let root = parse_nodes(structs, strings)?;

    Ok(DeviceTree::new(
        reserved, root, header.last_comp_version, header.boot_cpuid_phys))
}

fn parse_header(fdt: &[u8]) -> Result<FDTHeader, &'static str> {
    let mut scanner = Scanner::new(fdt);
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

    Ok(FDTHeader{
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
    let mut state = State::new();

    loop {
        match scanner.consume_be32() {
            Ok(token) if token == FDT_BEGIN_NODE => {
                state.begin_node(scanner.consume_cstr()?)?;
                scanner.align_forward(4)?;
            },
            Ok(token) if token == FDT_END_NODE => {
                state.end_node()?;
            },
            Ok(token) if token == FDT_PROP => {
                let len = scanner.consume_be32()? as usize;
                let off = scanner.consume_be32()? as usize;
                let value = scanner.consume_data(len)?;
                let name = Scanner::new(&strings[off..]).consume_cstr()?;
                state.new_property(name, value)?;
                scanner.align_forward(4)?;
            },
            Ok(token) if token == FDT_END => {
                return state.finish();
            },
            Ok(token) if token == FDT_NOP => {},
            Err(msg) => return Err(msg),
            _ => return Err("Unknown FDT token."),
        }
    }
}

struct State<'a> {
    parents: Vec<(&'a str, DeviceTreeNode)>,
    current: DeviceTreeNode,
}

impl<'a> State<'a> {
    fn new() -> State<'a> {
        State{
            parents: Vec::new(),
            current: DeviceTreeNode::new(),
        }
    }

    fn begin_node(&mut self, name: &'a str) -> Result<(), &'static str> {
        let node = mem::replace(&mut self.current, DeviceTreeNode::new());
        self.parents.push((name, node));
        Ok(())
    }

    fn end_node(&mut self) -> Result<(), &'static str> {
        if let Some((name, parent)) = self.parents.pop() {
            let node = mem::replace(&mut self.current, parent);
            self.current.add_child(String::from(name), node);
            return Ok(());
        }
        Err("Unmatched end of node token found in FDT.")
    }

    fn new_property(&mut self, name: &str, value: &[u8])
            -> Result<(), &'static str> {
        self.current.add_property(String::from(name), Vec::from(value));
        Ok(())
    }

    fn finish(&mut self) -> Result<DeviceTreeNode, &'static str> {
        if !self.parents.is_empty() {
            return Err("Parsed FDT contains unfinished nodes.");
        }
        if let Some(root) = self.current.remove_child("") {
            return Ok(root);
        }
        Err("FDT doesn't have a root node with an empty name.")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse() {
        let dtb = include_bytes!("test.dtb");
        let dt = parse(dtb).unwrap();

        assert_eq!(
            dt.reserved_memory(),
            vec![
                ReservedMemory{addr: 0x40000000, size: 0x1000},
                ReservedMemory{addr: 0x40002000, size: 0x1000},
                ReservedMemory{addr: 0x40004000, size: 0x1000}]);

        assert_eq!(
            dt.follow("/").unwrap().property("#size-cells"),
            Some(&[0x0u8, 0x0u8, 0x0u8, 0x02u8][..]));
        assert_eq!(
            dt.follow("/").unwrap().property("#address-cells"),
            Some(&[0x0u8, 0x0u8, 0x0u8, 0x02u8][..]));

        assert_eq!(
            dt.follow("/memory@40000000").unwrap().property("device_type"),
            Some("memory\0".as_bytes()));
        assert_eq!(
            dt.follow("/memory@40000000").unwrap().property("reg"),
            Some(&[
                 0x00u8, 0x00u8, 0x00u8, 0x00u8,
                 0x40u8, 0x00u8, 0x00u8, 0x00u8,
                 0x00u8, 0x00u8, 0x00u8, 0x00u8,
                 0x08u8, 0x00u8, 0x00u8, 0x00u8][..]));

        assert_eq!(
            dt.follow("/cpus/cpu@0").unwrap().property("reg"),
            Some(&[0x00u8, 0x00u8, 0x00u8, 0x00u8][..]));
        assert_eq!(
            dt.follow("/cpus/cpu@0").unwrap().property("device_type"),
            Some("cpu\0".as_bytes()));
        assert_eq!(
            dt.follow("/cpus/cpu@0").unwrap().property("compatible"),
            Some("arm,cortex-a57\0".as_bytes()));
    }
}
