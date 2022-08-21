export MAKEFLAGS += -r
export CC := clang
export CXX := clang
export AR := llvm-ar
export LD := lld

LDFLAGS := \
	-flavor ld -m aarch64elf \
	--pie --static --nostdlib --script=kernel.lds
CARGO_TARGET := \
	-Zbuild-std -Zbuild-std-features=compiler-builtins-mem \
	--target=$(shell pwd)/aarch64-unknown-custom.json

default: all

libkernel.a:
	cd kernel ; cargo build $(CARGO_TARGET) --release ; cd -
	cp kernel/target/aarch64-unknown-custom/release/libkernel.a $@

libmemory.a:
	$(MAKE) -C memory
	cp memory/libmemory.a $@

libfdt.a:
	$(MAKE) -C fdt
	cp fdt/libfdt.a $@

libbootstrap.a:
	$(MAKE) -C bootstrap
	cp bootstrap/libbootstrap.a $@

libc.a:
	$(MAKE) -C c
	cp c/libc.a $@

libcc.a:
	$(MAKE) -C cc
	cp cc/libcc.a $@

libcc.a:
	$(MAKE) -C cc
	cp cc/libcc.a $@

libcommon.a:
	$(MAKE) -C common
	cp common/libcommon.a $@

kernel.elf: libc.a libcc.a libcommon.a libmemory.a libfdt.a libbootstrap.a #libkernel.a
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: clean all default test libcommon.a libc.a libcc.a libkernel.a libbootstrap.a libmemory.a libfdt.a

all: kernel.elf

test:
	cd kernel/pl011 ; cargo test ; cd -
	cd kernel/devicetree ; cargo test ; cd -
	cd kernel/memory ; cargo test ; cd -
	cd kernel/numeric ; cargo test ; cd -
	cd kernel/sync ; cargo test ; cd -

bench:
	cd kernel/pl011 ; cargo bench ; cd -
	cd kernel/devicetree ; cargo bench ; cd -
	cd kernel/memory ; cargo bench ; cd -
	cd kernel/numeric ; cargo bench ; cd -
	cd kernel/sync ; cargo bench ; cd -

clean:
	cd kernel ; cargo clean ; cd -
	$(MAKE) -C bootstrap clean
	$(MAKE) -C memory clean
	$(MAKE) -C fdt clean
	$(MAKE) -C c clean
	$(MAKE) -C cc clean
	$(MAKE) -C common clean
	rm -rf *.elf *.o *.d *.a

