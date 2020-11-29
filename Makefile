CC := clang
LD := lld

CFLAGS := \
	-ffreestanding -MMD -mno-red-zone -std=c11 \
	-target aarch64-unknown-eabi -Wall -Werror -pedantic
LDFLAGS := \
	-flavor ld -e main

SRCS := pl011.c main.c

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: pl011.o main.o
	$(LD) $(LDFLAGS) $^ -o $@

-include $(SRCS:.c=.d)

.PHONY: clean all default

all: kernel.elf

clean:
	rm -rf *.elf *.o *.d

