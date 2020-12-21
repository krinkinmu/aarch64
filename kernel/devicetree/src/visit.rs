use core::str;

pub trait DeviceTreeVisitor {
    fn begin_node(&mut self, name: &str);
    fn end_node(&mut self);
    fn property(&mut self, name: &str, value: &[u8]);
    fn nop(&mut self);
    fn end(&mut self);
}

pub struct NoopVisitor {}

impl DeviceTreeVisitor for NoopVisitor {
    fn begin_node(&mut self, _name: &str) {}
    fn end_node(&mut self) {}
    fn property(&mut self, _name: &str, _value: &[u8]) {}
    fn nop(&mut self) {}
    fn end(&mut self) {}
}
