use alloc::vec::Vec;
use core::cmp::{self, Ordering};
use core::iter::{ExactSizeIterator, Iterator};
use core::ops::{FnMut, Range};
use core::result::Result;
use core::slice::Iter;
use numeric;

#[derive(PartialEq, Clone, Copy, Debug)]
pub enum MemoryType {
    Regular,
    NonRegular,
}

#[derive(PartialEq, Clone, Copy, Debug)]
pub(crate) enum MemoryStatus {
    Free,
    Reserved,
    Unknown,
}

#[derive(PartialEq, Clone, Debug)]
pub struct MemoryRange {
    pub range: Range<u64>,
    pub memtype: MemoryType,
    status: MemoryStatus,
}

pub(crate) struct FreeMemoryIter<'a> {
    inner: Iter<'a, MemoryRange>,
    limits: Range<u64>,
}

pub struct ZoneIter<'a> {
    inner: Iter<'a, MemoryRange>,
}

#[derive(Clone, Debug)]
pub struct MemoryMap {
    memory: Vec<MemoryRange>,
    zones: Vec<MemoryRange>,
}

fn overlap(r1: &Range<u64>, r2: &Range<u64>) -> Range<u64> {
    cmp::max(r1.start, r2.start)..cmp::min(r1.end, r2.end)
}

fn compact(seq: &mut Vec<MemoryRange>) {
    let can_concat = |l: &MemoryRange, r: &MemoryRange| {
        l.range.end == r.range.start
            && l.memtype == r.memtype
                && l.status == r.status
    };

    let mut to = 0;
    let mut from = 1;

    while from < seq.len() {
        if can_concat(&seq[to], &seq[from]) {
            seq[to].range.end = seq[from].range.end;
        } else {
            to += 1;
            seq[to] = seq[from].clone();
        }
        from += 1;
    }
    seq.truncate(to + 1);
}

fn lower_bound<'a, T, F>(seq: &'a [T], mut cmp: F) -> usize
where
    F: FnMut(&'a T) -> Ordering
{
    for i in 0..seq.len() {
        match cmp(&seq[i]) {
            Ordering::Less => continue,
            _ => return i,
        }
    }
    seq.len()
}

fn upper_bound<'a, T, F>(seq: &'a [T], mut cmp: F) -> usize
where
    F: FnMut(&'a T) -> Ordering
{
    for i in 0..seq.len() {
        match cmp(&seq[i]) {
            Ordering::Greater => return i,
            _ => continue,
        }
    }
    seq.len()
}

fn equal_range<'a, T, F>(seq: &'a [T], mut cmp: F) -> Range<usize>
where
    F: FnMut(&'a T) -> Ordering
{
    lower_bound(seq, &mut cmp)..upper_bound(seq, &mut cmp)
}

fn replace<T: Clone>(seq: &mut Vec<T>, range: Range<usize>, items: &[T]) {
    assert!(range.end <= seq.len());
    let smaller = range.start;
    let larger = seq.len() - range.end;

    seq.rotate_left(range.end);
    seq.truncate(smaller + larger);
    for item in items {
        seq.push(item.clone());
    }
    seq.rotate_left(larger);
}

fn add_memory(
    seq: &mut Vec<MemoryRange>,
    mem: MemoryRange) -> Result<(), &'static str>
{
    if mem.range.is_empty() { return Ok(()); }

    let cmp = |l: &MemoryRange| {
        if l.range.end <= mem.range.start { Ordering::Less }
        else if l.range.start >= mem.range.end { Ordering::Greater }
        else { Ordering::Equal }
    };

    let mut before = MemoryRange {
        range: mem.range.start..mem.range.start,
        memtype: mem.memtype,
        status: mem.status,
    };
    let mut after = MemoryRange {
        range: mem.range.end..mem.range.end,
        memtype: mem.memtype,
        status: mem.status,
    };
    let pos = equal_range(&seq[..], cmp);
    for i in pos.clone() {
        if seq[i].memtype != mem.memtype {
            return Err("Memory types of overlapping ranges don't match.");
        }
        if (seq[i].status, mem.status) ==
            (MemoryStatus::Reserved, MemoryStatus::Free)
        {
            return Err("Reserved memory cannot be freed.");
        }

        if seq[i].range.start < mem.range.start {
            before = MemoryRange {
                range: seq[i].range.start..mem.range.start,
                memtype: seq[i].memtype,
                status: seq[i].status,
            }
        }
        if seq[i].range.end > mem.range.end {
            after = MemoryRange {
                range: mem.range.end..seq[i].range.end,
                memtype: seq[i].memtype,
                status: seq[i].status,
            }
        }
    }

    replace(seq, pos, &[before, mem, after]);
    compact(seq);

    Ok(())
}

impl MemoryMap {
    pub fn new() -> MemoryMap {
        MemoryMap {
            memory: Vec::new(),
            zones: Vec::new(),
        }
    }

    pub fn zones(&self) -> ZoneIter {
        ZoneIter {
            inner: self.zones.iter(),
        }
    }

    pub(crate) fn free_memory_in_range(
        &self, range: Range<u64>) -> FreeMemoryIter
    {
        FreeMemoryIter {
            inner: self.memory.iter(),
            limits: range,
        }
    }

    pub fn add_memory(
        &mut self,
        range: Range<u64>,
        memtype: MemoryType) -> Result<(), &'static str>
    {
        add_memory(&mut self.memory, MemoryRange {
            range: range.clone(),
            memtype: memtype,
            status: MemoryStatus::Free,
        }).and(add_memory(&mut self.zones, MemoryRange {
            range: range,
            memtype: memtype,
            status: MemoryStatus::Unknown,
        }))
    }

    pub fn reserve_memory(
        &mut self,
        range: Range<u64>,
        memtype: MemoryType) -> Result<(), &'static str>
    {
        add_memory(&mut self.memory, MemoryRange {
            range: range.clone(),
            memtype: memtype,
            status: MemoryStatus::Reserved,
        }).and(add_memory(&mut self.zones, MemoryRange {
            range: range,
            memtype: memtype,
            status: MemoryStatus::Unknown,
        }))
    }

    pub(crate) fn allocate(&mut self, size: u64, align: u64) -> Option<u64> {
        self.allocate_in_range(u64::MIN..u64::MAX, size, align)
    }

    pub(crate) fn allocate_with_hint(
        &mut self, hint: Range<u64>, size: u64, align: u64) -> Option<u64>
    {
        self.allocate_in_range(hint.clone(), size, align)
            .or_else(|| self.allocate(size, align))
    }

    fn allocate_in_range(
        &mut self, limit: Range<u64>, size: u64, align: u64) -> Option<u64>
    {
        if let Some(mem) = self.find_free_memory_in_range(limit, size, align) {
            add_memory(&mut self.memory, MemoryRange {
                range: mem.range.clone(),
                memtype: mem.memtype,
                status: MemoryStatus::Reserved,
            }).unwrap();
            return Some(mem.range.start);
        }

        None
    }

    fn find_free_memory_in_range(
        &self,
        limit: Range<u64>,
        size: u64,
        align: u64) -> Option<MemoryRange>
    {
        for mem in self.free_memory_in_range(limit) {
            if mem.memtype != MemoryType::Regular { continue; }

            let from = numeric::align_up(mem.range.start, align);
            let to = mem.range.end;

            if from + size > to { continue; }

            return Some(MemoryRange {
                range: from..from + size,
                memtype: mem.memtype,
                status: mem.status,
            });
        }

        None
    }
}

impl<'a> Iterator for FreeMemoryIter<'a> {
    type Item = MemoryRange;

    fn next(&mut self) -> Option<MemoryRange> {
        while let Some(r) = self.inner.next() {
            let range = overlap(&r.range, &self.limits);
            if r.status != MemoryStatus::Free { continue; }
            if range.is_empty() { continue; }
            return Some(MemoryRange {
                range: range,
                memtype: r.memtype,
                status: MemoryStatus::Free,
            });
        }
        None
    }
}

impl<'a> Iterator for ZoneIter<'a> {
    type Item = MemoryRange;

    fn next(&mut self) -> Option<MemoryRange> {
        self.inner.next().cloned()
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        self.inner.size_hint()
    }
}

impl<'a> ExactSizeIterator for ZoneIter<'a> {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_lower_bound() {
        assert_eq!(lower_bound(&[], |x: &i32| x.cmp(&0)), 0);
        assert_eq!(lower_bound(&[1, 2, 3], |x| x.cmp(&0)), 0);
        assert_eq!(lower_bound(&[1, 2, 3], |x| x.cmp(&1)), 0);
        assert_eq!(lower_bound(&[1, 2, 3], |x| x.cmp(&2)), 1);
        assert_eq!(lower_bound(&[1, 2, 3], |x| x.cmp(&3)), 2);
        assert_eq!(lower_bound(&[1, 2, 3], |x| x.cmp(&4)), 3);
    }

    #[test]
    fn test_upper_bound() {
        assert_eq!(upper_bound(&[], |x: &i32| x.cmp(&0)), 0);
        assert_eq!(upper_bound(&[1, 2, 3], |x| x.cmp(&0)), 0);
        assert_eq!(upper_bound(&[1, 2, 3], |x| x.cmp(&1)), 1);
        assert_eq!(upper_bound(&[1, 2, 3], |x| x.cmp(&2)), 2);
        assert_eq!(upper_bound(&[1, 2, 3], |x| x.cmp(&3)), 3);
        assert_eq!(upper_bound(&[1, 2, 3], |x| x.cmp(&4)), 3);
    }

    #[test]
    fn test_equal_bound() {
        assert_eq!(equal_range(&[], |x: &i32| x.cmp(&0)), 0usize..0usize);
        assert_eq!(
            equal_range(&[1, 2, 2, 2, 3], |x| x.cmp(&0)), 0usize..0usize);
        assert_eq!(
            equal_range(&[1, 2, 2, 2, 3], |x| x.cmp(&1)), 0usize..1usize);
        assert_eq!(
            equal_range(&[1, 2, 2, 2, 3], |x| x.cmp(&2)), 1usize..4usize);
        assert_eq!(
            equal_range(&[1, 2, 2, 2, 3], |x| x.cmp(&3)), 4usize..5usize);
        assert_eq!(
            equal_range(&[1, 2, 2, 2, 3], |x| x.cmp(&4)), 5usize..5usize);
    }

    #[test]
    fn test_replace() {
        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 0..0, &[0]);
        assert_eq!(&seq[..], &[0, 0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 0..0, &[0, 0]);
        assert_eq!(&seq[..], &[0, 0, 0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 1..1, &[0]);
        assert_eq!(&seq[..], &[0, 0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 1..1, &[1, 0]);
        assert_eq!(&seq[..], &[0, 1, 0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 2..2, &[0]);
        assert_eq!(&seq[..], &[0, 1, 0, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 2..2, &[0, 0]);
        assert_eq!(&seq[..], &[0, 1, 0, 0, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 5..5, &[0]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 0, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 5..5, &[0, 0]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 0, 0, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 6..6, &[0]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5, 0]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 6..6, &[0, 0]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5, 0, 0]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 0..2, &[0]);
        assert_eq!(&seq[..], &[0, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 0..2, &[0, 1]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 1..3, &[0]);
        assert_eq!(&seq[..], &[0, 0, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 1..3, &[1, 2]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 2..4, &[0]);
        assert_eq!(&seq[..], &[0, 1, 0, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 2..4, &[2, 3]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 3..5, &[0]);
        assert_eq!(&seq[..], &[0, 1, 2, 0, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 3..5, &[3, 4]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 4..6, &[0]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 0]);

        let mut seq = vec![0, 1, 2, 3, 4, 5];
        replace(&mut seq, 4..6, &[4, 5]);
        assert_eq!(&seq[..], &[0, 1, 2, 3, 4, 5]);
    }

    #[test]
    fn test_compact() {
        let mut rs = Vec::new();
        compact(&mut rs);
        assert_eq!(&rs[..], &[]);

        let mut rs = vec![
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::NonRegular,
                status: MemoryStatus::Free,
            },
        ];
        compact(&mut rs);
        assert_eq!(&rs[..], &[
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::NonRegular,
                status: MemoryStatus::Free,
            },
        ]);

        let mut rs = vec![
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Reserved,
            },
        ];
        compact(&mut rs);
        assert_eq!(&rs[..], &[
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Reserved,
            },
        ]);

        let mut rs = vec![
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 2..3,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
        ];
        compact(&mut rs);
        assert_eq!(&rs[..], &[
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 2..3,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free
            },
        ]);

        let mut rs = vec![
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
        ];
        compact(&mut rs);
        assert_eq!(&rs[..], &[
            MemoryRange {
                range: 0..2,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
        ]);
    }

    #[test]
    fn test_add_memory() {
        let mut mem = Vec::new();

        assert!(add_memory(&mut mem, MemoryRange {
            range: 0..1,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Free,
        }).is_ok());
        assert_eq!(&mem[..], &[MemoryRange {
            range: 0..1,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Free,
        }]);

        assert!(add_memory(&mut mem, MemoryRange {
            range: 0..1,
            memtype: MemoryType::NonRegular,
            status: MemoryStatus::Free,
        }).is_err());

        assert!(add_memory(&mut mem, MemoryRange {
            range: 0..1,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Reserved,
        }).is_ok());
        assert_eq!(&mem[..], &[MemoryRange {
            range: 0..1,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Reserved,
        }]);

        assert!(add_memory(&mut mem, MemoryRange {
            range: 0..1,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Free,
        }).is_err());

        assert!(add_memory(&mut mem, MemoryRange {
            range: 1..2,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Free,
        }).is_ok());
        assert_eq!(&mem[..], &[
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Reserved,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Free,
            },
        ]);

        assert!(add_memory(&mut mem, MemoryRange {
            range: 1..2,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Reserved,
        }).is_ok());
        assert_eq!(&mem[..], &[MemoryRange {
            range: 0..2,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Reserved,
        }]);

        let mut mem = Vec::new();

        assert!(add_memory(&mut mem, MemoryRange {
            range: 0..4,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Free,
        }).is_ok());
        assert!(add_memory(&mut mem, MemoryRange {
            range: 1..3,
            memtype: MemoryType::Regular,
            status: MemoryStatus::Reserved,
        }).is_ok());
        assert_eq!(
            &mem[..],
            &[
                MemoryRange {
                    range: 0..1,
                    memtype: MemoryType::Regular,
                    status: MemoryStatus::Free,
                },
                MemoryRange {
                    range: 1..3,
                    memtype: MemoryType::Regular,
                    status: MemoryStatus::Reserved,
                },
                MemoryRange {
                    range: 3..4,
                    memtype: MemoryType::Regular,
                    status: MemoryStatus::Free,
                },
            ]);
    }

    #[test]
    fn test_zones() {
        let mut mmap = MemoryMap::new();

        assert!(mmap.add_memory(0..1, MemoryType::Regular).is_ok());
        assert!(mmap.add_memory(1..2, MemoryType::NonRegular).is_ok());
        assert!(mmap.add_memory(3..4, MemoryType::Regular).is_ok());
        assert!(mmap.reserve_memory(4..5, MemoryType::Regular).is_ok());

        let it = mmap.zones();
        assert_eq!(it.len(), 3);
        assert_eq!(it.collect::<Vec<_>>(), vec![
            MemoryRange {
                range: 0..1,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Unknown,
            },
            MemoryRange {
                range: 1..2,
                memtype: MemoryType::NonRegular,
                status: MemoryStatus::Unknown,
            },
            MemoryRange {
                range: 3..5,
                memtype: MemoryType::Regular,
                status: MemoryStatus::Unknown,
            },
        ]);
    }

    #[test]
    fn test_free_memory() {
        let mut mmap = MemoryMap::new();

        assert!(mmap.add_memory(0..1, MemoryType::Regular).is_ok());
        assert!(mmap.add_memory(1..3, MemoryType::NonRegular).is_ok());
        assert!(mmap.add_memory(4..5, MemoryType::Regular).is_ok());
        assert!(mmap.reserve_memory(5..6, MemoryType::Regular).is_ok());

        assert_eq!(
            mmap.free_memory_in_range(u64::MIN..u64::MAX).collect::<Vec<_>>(),
            vec![
                MemoryRange {
                    range: 0..1,
                    memtype: MemoryType::Regular,
                    status: MemoryStatus::Free,
                },
                MemoryRange {
                    range: 1..3,
                    memtype: MemoryType::NonRegular,
                    status: MemoryStatus::Free,
                },
                MemoryRange {
                    range: 4..5,
                    memtype: MemoryType::Regular,
                    status: MemoryStatus::Free,
                },
            ]);
        assert_eq!(
            mmap.free_memory_in_range(0..0).collect::<Vec<_>>(),
            Vec::new());
        assert_eq!(
            mmap.free_memory_in_range(3..4).collect::<Vec<_>>(),
            Vec::new());
        assert_eq!(
            mmap.free_memory_in_range(5..6).collect::<Vec<_>>(),
            Vec::new());
        assert_eq!(
            mmap.free_memory_in_range(0..1).collect::<Vec<_>>(),
            vec![
                MemoryRange {
                    range: 0..1,
                    memtype: MemoryType::Regular,
                    status: MemoryStatus::Free,
                },
            ]);
        assert_eq!(
            mmap.free_memory_in_range(1..2).collect::<Vec<_>>(),
            vec![
                MemoryRange {
                    range: 1..2,
                    memtype: MemoryType::NonRegular,
                    status: MemoryStatus::Free,
                },
            ]);
    }
}
