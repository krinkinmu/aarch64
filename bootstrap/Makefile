CC := clang
CXX := clang
AR := llvm-ar

AFLAGS := \
	-fPIE -target aarch64-unknown-none -Wall -Werror
CXXFLAGS := \
    -MMD -mno-red-zone -std=c++17 -ffreestanding -fno-threadsafe-statics \
    -fno-exceptions -fno-rtti -Ofast -g -fPIE -target aarch64-unknown-none \
    -Wall -Werror -Wframe-larger-than=1024 -pedantic -I.. -I../c -I../cc

ASRCS := start.S interrupts.S
AOBJS := $(ASRCS:.S=.o)

CXXSRCS := main.cc pl011.cc memory.cc
CXXOBJS := $(CXXSRCS:.cc=.o)

OBJS := $(AOBJS) $(CXXOBJS)

default: all

%.o: %.S
	$(CC) $(AFLAGS) -c $< -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

libbootstrap.a: $(OBJS)
	$(AR) rc $@ $^

-include $(ASRCS:.S=.d)
-include $(CXXSRCS:.cc=.d)

.PHONY: clean all default

all: libbootstrap.a

clean:
	rm -rf *.elf *.o *.d *.a
