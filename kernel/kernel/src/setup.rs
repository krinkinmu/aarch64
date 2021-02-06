use bootstrap;
use core::option::Option;
use devicetree::DeviceTree;
use log::{self, LoggerSink};
use memory::{Memory, MemoryMap, MemoryType};
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

pub fn setup_memory(dt: &DeviceTree) -> Memory<'static> {
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

pub fn setup_logger() {
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
        log::setup_logger_sink(SINK.as_ref().unwrap() as &dyn LoggerSink);
    }
}
