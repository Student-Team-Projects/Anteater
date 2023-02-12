OBJ := obj
INCLUDE := include
BIN := bin
SRC := src
CHISEL := proc_tree.lua

CHISELDIR := $(DESTDIR)/usr/share/sysdig/chisels
LOGSDIR := /tmp/debugger/logs


INCLUDES := $(INCLUDE) $(OBJ)
SRCS := $(wildcard $(SRC)/*.cpp)
OBJS := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))
MAIN = main

# BPF variables
BPF_SRC := $(SRC)/bpf
BPF_SRCS := $(wildcard $(BPF_SRC)/*.bpf.c)
BPF_INCLUDES := $(INCLUDES)
BPF_OBJS := $(patsubst $(BPF_SRC)/%.bpf.c,$(OBJ)/%.bpf.o,$(BPF_SRCS))
BPF_TRACER := tracer.bpf.o
BPF_SKELETON := tracer.skel.h

# tools
CXX := clang++
CLANG ?= clang
LLVM_STRIP ?= llvm-strip
BPFTOOL := bpftool
VMLINUX := vmlinux.h
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/' | sed 's/ppc64le/powerpc/' | sed 's/mips.*/mips/')

# flags
IFLAGS := $(patsubst %,-I%,$(INCLUDES))
CXXFLAGS := -Wall -std=c++20 -D CHISEL="\"$(CHISEL)\"" -D LOGSDIR="\"$(LOGSDIR)\""
DEBUG_CXXFLAGS := -g -fsanitize=address
# to add debug flags, uncomment the next line
# CXXFLAGS += DEBUG_CXXFLAGS
ALL_LDFLAGS := $(LDFLAGS) $(EXTRA_LDFLAGS)
BPF_IFLAGS := $(patsubst %,-I%,$(BPF_INCLUDES))

# Get Clang's default includes on this system. We'll explicitly add these dirs
# to the includes list when compiling with `-target bpf` because otherwise some
# architecture-specific dirs will be "missing" on some architectures/distros -
# headers such as asm/types.h, asm/byteorder.h, asm/socket.h, asm/sockios.h,
# sys/cdefs.h etc. might be missing.
#
# Use '-idirafter': Don't interfere with include mechanics except where the
# build would have failed anyways.
CLANG_BPF_SYS_INCLUDES = $(shell $(CLANG) -v -E - </dev/null 2>&1 \
	| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p }')

.PHONY: all
all: $(BIN)/$(MAIN)

# cleanup
.PHONY: clean
clean:
	rm -rf $(OBJ) $(BIN)

# installs chisel, so that sysdig can find it
.PHONY: chisel
chisel:
	cp $(SRC)/$(CHISEL) $(CHISELDIR)

# create directory for intermediate objects
$(OBJ):
	mkdir -p $(OBJ)

# create directory for binaries
$(BIN):
	mkdir -p $(BIN)

# create vmlinux.h
$(OBJ)/$(VMLINUX): | $(OBJ)
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

# Build BPF code
$(BPF_OBJS): $(OBJ)/%.o: $(BPF_SRC)/%.c $(BPF_INCLUDES) $(OBJ)/$(VMLINUX) | $(OBJ)
	$(CLANG) -g -O3 -target bpf -D__TARGET_ARCH_$(ARCH) $(BPF_IFLAGS) $(CLANG_BPF_SYS_INCLUDES) -c $(filter %.c,$^) -o $@
	$(LLVM_STRIP) -g $@ # strip useless DWARF info

# Combine BPF objects
$(OBJ)/$(BPF_TRACER): $(BPF_OBJS) | $(OBJ)
	$(BPFTOOL) gen object $@ $^

# Generate BPF skeleton
$(OBJ)/$(BPF_SKELETON): $(OBJ)/$(BPF_TRACER) | $(OBJ)
	$(BPFTOOL) gen skeleton $< > $@

# Add BPF skeleton to BPF provider dependencies
$(OBJ)/bpf_provider.o: $(OBJ)/$(BPF_SKELETON)

# Generic rule to create objects from cpp files
$(OBJS): $(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# Build application binary
$(BIN)/$(MAIN): $(OBJS) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ $(ALL_LDFLAGS) -lbpf -lelf -lfmt -lz -o $@

# delete failed targets
.DELETE_ON_ERROR:

# keep intermediate (.skel.h, .bpf.o, etc) targets
.SECONDARY:
