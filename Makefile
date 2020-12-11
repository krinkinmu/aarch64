CC := clang
LD := lld

CFLAGS := \
	-ffreestanding -MMD -mno-red-zone -std=c11 \
	-target aarch64-unknown-none -Wall -Werror -pedantic
LDFLAGS := \
	-flavor ld -e main

SRCS := main.c pl011.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libstart.a:
	cd kernel ; cargo build --release --target=aarch64-unknown-none ; cd -
	cp kernel/target/aarch64-unknown-none/release/libstart.a $@

kernel.elf: main.o pl011.o libstart.a
	$(LD) $(LDFLAGS) $^ -o $@

-include $(SRCS:.c=.d)

.PHONY: clean all default test

all: kernel.elf

test:
	cd kernel/pl011 ; cargo test --target=x86_64-unknown-linux-gnu ; cd -

clean:
	cd kernel ; cargo clean ; cd -
	rm -rf *.elf *.o *.d *.a

