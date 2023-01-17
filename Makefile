OBJ := obj
INCLUDE := include
BIN := bin
SRC := src
CHISEL := proc_tree.lua

# targets
INCLUDES := $(INCLUDE) $(OBJ)
CHISELDIR := $(DESTDIR)/usr/share/sysdig/chisels

SRCS := $(wildcard $(SRC)/*.cpp)

MAIN = main
OBJS := $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))

# tools
CXX := clang++
CLANG ?= clang
LLVM_STRIP ?= llvm-strip
BPFTOOL := bpftool
VMLINUX := $(OBJ)/vmlinux.h
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/' | sed 's/ppc64le/powerpc/' | sed 's/mips.*/mips/')

# flags
IFLAGS := $(patsubst %,-I%,$(INCLUDES))
CXXFLAGS := -g -Wall -std=c++20 -fsanitize=address -D CHISEL="\"$(CHISEL)\""
ALL_LDFLAGS := $(LDFLAGS) $(EXTRA_LDFLAGS)

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

.PHONY: clean
clean:
	rm -rf $(OBJ) $(BIN)

.PHONY: chisel
chisel:
	cp $(SRC)/$(CHISEL) $(CHISELDIR)

$(OBJ):
	mkdir -p $(OBJ)

$(BIN):
	mkdir -p $(BIN)

$(VMLINUX): | $(OBJ)
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $(VMLINUX)

# Build BPF code
$(OBJ)/tracer.bpf.o: $(SRC)/tracer.bpf.c $(INCLUDE)/constants.h $(VMLINUX) | $(OBJ)
	$(CLANG) -g -O3 -target bpf -D__TARGET_ARCH_$(ARCH) $(IFLAGS) $(CLANG_BPF_SYS_INCLUDES) -c $(filter %.c,$^) -o $@
	$(LLVM_STRIP) -g $@ # strip useless DWARF info

# Generate BPF skeletons
$(OBJ)/tracer.skel.h: $(OBJ)/tracer.bpf.o | $(OBJ)
	$(BPFTOOL) gen skeleton $< > $@


# Build user-space code
$(OBJ)/bpf_provider.o: $(OBJ)/tracer.skel.h
$(OBJ)/$(MAIN).o: $(OBJ)/tracer.skel.h

$(OBJS): $(OBJ)/%.o: $(SRC)/%.cpp | $(OBJ)
	$(CXX) $(CXXFLAGS) $(IFLAGS) -c $< -o $@

# Build application binary
$(BIN)/$(MAIN): $(OBJS) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ $(ALL_LDFLAGS) -lbpf -lelf -lz -o $@

# delete failed targets
.DELETE_ON_ERROR:

# keep intermediate (.skel.h, .bpf.o, etc) targets
.SECONDARY:
