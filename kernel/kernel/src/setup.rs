use bootstrap;
use core::option::Option;
use crate::Kernel;
use devicetree::{self, DeviceTree};
use log::{self, Logger, LoggerSink};
use memory::{Memory, MemoryMap, MemoryType};
use numeric;
use pl011::PL011;


fn mmap_from_devicetree(dt: &DeviceTree) -> MemoryMap {
    let mut mmap = MemoryMap::new();
    let root = dt.follow("/").unwrap();

    for (unitname, node) in root.children() {
        if unitname != "memory" && !unitname.starts_with("memory@") {
            continue;
        }
        let mut reg = node.property("reg").unwrap();
        while reg.remains() > 0 {
            let addr = reg.consume_address(&root.address_space()).unwrap();
            let size = reg.consume_size(&root.address_space()).unwrap();

            mmap.add_memory(addr..addr + size, MemoryType::Regular).unwrap();
        }
    }

    for reserved in dt.reserved_memory().iter().cloned() {
        mmap.reserve_memory(reserved, MemoryType::Regular).unwrap();
    }
    for reserved in bootstrap::reserved_range_iter() {
        mmap.reserve_memory(reserved, MemoryType::Regular).unwrap();
    }

    mmap
}

fn memory_from_devicetree(dt: &DeviceTree) -> Memory<'static> {
    let mmap = mmap_from_devicetree(dt);
    unsafe { Memory::new(&mmap) }
}

struct SerialSink {
    serial: PL011,
}

impl LoggerSink for SerialSink {
    fn log(&self, msg: &str) {
        self.serial.send(msg);
        self.serial.send("\n");
    }
}

fn serial_logger() -> Logger<'static> {
    static mut SINK: Option<SerialSink> = None;

    // For HiKey960 board that I have the following parameters were found to
    // work fine:
    //
    // let serial = PL011::new(
    //     /* base_address = */0xfff32000,
    //     /* base_clock = */19200000);
    let serial = PL011::new(
        /* base_address = */0x9000000,
        /* base_clock = */24000000);

    unsafe {
        assert!(SINK.is_none());
        SINK = Some(SerialSink { serial });
        Logger::new(SINK.as_ref().unwrap() as &dyn LoggerSink)
    }
}

pub fn from_devicetree() -> Kernel {
    let mut memory = {
        let dt = devicetree::fdt::parse(bootstrap::fdt()).unwrap();
        memory_from_devicetree(&dt)
    };
    let logger = serial_logger();

    assert!(bootstrap::allocator_shutdown());
    let heap = bootstrap::heap_range();

    let mut addr = numeric::align_up(heap.start, memory.page_size());
    let to = numeric::align_down(heap.end, memory.page_size());
    while addr < to {
        memory.free_pages(addr);
        addr += memory.page_size();
    }

    Kernel { memory, logger }
}
