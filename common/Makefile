CXX := clang
AR := llvm-ar

CXXFLAGS := \
    -MMD -mno-red-zone -std=c++17 -ffreestanding -fno-threadsafe-statics \
    -fno-exceptions -fno-rtti -Ofast -g -fPIE -target aarch64-unknown-none \
    -Wall -Werror -Wframe-larger-than=1024 -pedantic -I.. -I../c -I../cc

CXXSRCS := stream.cc logging.cc string_view.cc intrusive_list.cc math.cc endian.cc
CXXOBJS := $(CXXSRCS:.cc=.o)

OBJS := $(CXXOBJS)

default: all

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

libcommon.a: $(OBJS)
	$(AR) rc $@ $^

-include $(CXXSRCS:.cc=.d)

.PHONY: clean all default

all: libcommon.a

clean:
	rm -rf *.elf *.o *.d *.a
