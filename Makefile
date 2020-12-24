export CC := clang
export AR := llvm-ar
export LD := lld

export CFLAGS := \
	-ffreestanding -MMD -mno-red-zone -std=c11 -Ofast \
	-fPIE -target aarch64-unknown-none -Wall -Werror -pedantic

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

libbootstrap.a:
	$(MAKE) -C bootstrap
	cp bootstrap/libbootstrap.a $@

kernel.elf: libbootstrap.a libkernel.a
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: clean all default test libkernel.a libbootstrap.a

all: kernel.elf

test:
	cd kernel/pl011 ; cargo test ; cd -
	cd kernel/devicetree ; cargo test ; cd -

clean:
	cd kernel ; cargo clean ; cd -
	$(MAKE) -C bootstrap clean
	rm -rf *.elf *.o *.d *.a

