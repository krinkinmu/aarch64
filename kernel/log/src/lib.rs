#![no_std]
pub trait LoggerSink {
    fn log(&self, msg: &str);
}

pub struct Logger<'a> {
    sink: &'a dyn LoggerSink,
}

impl<'a> Logger<'a> {
    pub fn new(sink: &dyn LoggerSink) -> Logger {
        Logger { sink }
    }

    pub fn log(&self, msg: &str) {
        self.sink.log(msg)
    }
}

