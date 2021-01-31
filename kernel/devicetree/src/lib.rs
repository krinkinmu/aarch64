#![cfg_attr(not(test), no_std)]
extern crate alloc;
mod scanner;
pub mod fdt;

use alloc::collections::btree_map::{BTreeMap, Iter};
use alloc::string::String;
use alloc::vec::Vec;
use core::iter::Iterator;
use core::option::Option;


const DEVICE_TREE_SPEC_VERSION: u32 = 17;

#[derive(Clone, Debug, PartialEq)]
pub struct DeviceTreeNode {
    children: BTreeMap<String, DeviceTreeNode>,
    properties: BTreeMap<String, Vec<u8>>,
}

impl DeviceTreeNode {
    pub fn child(&self, name: &str) -> Option<&DeviceTreeNode> {
        self.children.get(name)
    }

    pub fn children(&self) -> Children {
        Children{ inner: self.children.iter() }
    }

    pub fn property(&self, name: &str) -> Option<&[u8]> {
        self.properties.get(name).map(|v| v.as_slice())
    }

    pub fn properties(&self) -> Properties {
        Properties{ inner: self.properties.iter() }
    }

    fn new() -> DeviceTreeNode {
        DeviceTreeNode{
            children: BTreeMap::new(),
            properties: BTreeMap::new(),
        }
    }

    fn add_child(&mut self, name: String, child: DeviceTreeNode) {
        self.children.insert(name, child);
    }

    fn remove_child(&mut self, name: &str) -> Option<DeviceTreeNode> {
        self.children.remove(name)
    }

    fn add_property(&mut self, name: String, value: Vec<u8>) {
        self.properties.insert(name, value);
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

#[derive(Copy, Clone, Debug, PartialEq)]
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

impl DeviceTree {
    pub fn new(
            reserved: Vec<ReservedMemory>,
            root: DeviceTreeNode,
            last_comp_version: u32,
            boot_cpuid_phys: u32) -> DeviceTree {
        DeviceTree{ reserved, root, last_comp_version, boot_cpuid_phys }
    }

    pub fn reserved_memory(&self) -> &[ReservedMemory] {
        self.reserved.as_slice()
    }

    pub fn follow(&self, path: &str) -> Option<&DeviceTreeNode> {
        let mut current = &self.root;

        if path == "/" {
            return Some(current);
        }

        for name in path[1..].split("/") {
            if let Some(node) = current.child(name) {
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


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_compatible() {
        let dt = DeviceTree::new(Vec::new(), DeviceTreeNode::new(), 10, 0);

        assert!(dt.compatible(10));
        assert!(dt.compatible(DEVICE_TREE_SPEC_VERSION));
        assert!(!dt.compatible(9));
        assert!(!dt.compatible(DEVICE_TREE_SPEC_VERSION + 1));
    }

    #[test]
    fn test_follow() {
        let mut root = DeviceTreeNode::new();
        let mut root_node1 = DeviceTreeNode::new();
        let root_node2 = DeviceTreeNode::new();
        let root_node1_node3 = DeviceTreeNode::new();

        root_node1.add_child(
            String::from("node3"), root_node1_node3.clone());
        root.add_child(String::from("node1"), root_node1.clone());
        root.add_child(String::from("node2"), root_node2.clone());

        let dt = DeviceTree::new(Vec::new(), root.clone(), 0, 0);

        assert_eq!(dt.follow("/"), Some(&root));
        assert_eq!(dt.follow("/node1"), Some(&root_node1));
        assert_eq!(dt.follow("/node2"), Some(&root_node2));
        assert_eq!(dt.follow("/node1/node3"), Some(&root_node1_node3));
        assert_eq!(dt.follow("/node4"), None);
    }
}
