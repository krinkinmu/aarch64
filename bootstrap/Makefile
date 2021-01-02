CC ?= clang
AR ?= llvm-ar
CFLAGS ?= \
	-ffreestanding -MMD -mno-red-zone -std=c11 -Ofast \
	-fPIE -target aarch64-unknown-none -Wall -Werror -pedantic

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) -fPIE -target aarch64-unknown-none -Wall -Werror -c $< -o $@

libbootstrap.a: main.o pl011.o alloc.o config.o relocate.o start.o ints.o
	$(AR) rc $@ $^

-include $(SRCS:.c=.d)

.PHONY: clean all default

all: libbootstrap.a

clean:
	rm -rf *.elf *.o *.d *.a
