CXX := clang++
AR := llvm-ar
CXXFLAGS := \
    -MMD -mno-red-zone -std=c++17 -ffreestanding -fno-threadsafe-statics \
    -fno-exceptions -fno-rtti -Ofast -fPIE -target aarch64-unknown-none \
    -Wall -Werror -Wframe-larger-than=1024 -pedantic -I.. -I../c -I../cc

CXXSRCS := scanner.cc blob.cc
CXXOBJS := $(CXXSRCS:.cc=.o)

OBJS := $(CXXOBJS)

default: all

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

libfdt.a: $(OBJS)
	$(AR) rc $@ $^

-include $(CXXSRCS:.cc=.d)

.PHONY: clean all default

all: libfdt.a

clean:
	rm -rf *.o *.d *.a
