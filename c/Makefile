CC := clang
AR := llvm-ar

CFLAGS := \
	-MMD -mno-red-zone -ffreestanding -fno-threadsafe-statics \
	-Ofast -g -fPIE -target aarch64-unknown-none \
	-Wall -Werror -Wframe-larger-than=1024 -pedantic -I..

CSRCS := crt.c relocate.c string.c
COBJS := $(CSRCS:.c=.o)

OBJS := $(COBJS)

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

libc.a: $(OBJS)
	$(AR) rc $@ $^

-include $(CSRCS:.c=.d)

.PHONY: clean all default

all: libc.a

clean:
	rm -rf *.elf *.o *.d *.a
