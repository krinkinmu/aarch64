#![no_std]
extern crate sync;

use core::option::Option;
use sync::placeholder::Mutex;

pub trait LoggerSink {
    fn log(&self, msg: &str);
}

static SINK: Mutex<Option<&'static dyn LoggerSink>> = Mutex::new(None);

pub fn setup_logger_sink(sink: &'static dyn LoggerSink) {
    let mut locked = SINK.lock();
    *locked = Some(sink);
}

pub fn log(msg: &str) {
    let locked = SINK.lock();
    if let Some(sink) = *locked {
        sink.log(msg);
    }
}
