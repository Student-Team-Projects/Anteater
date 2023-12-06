OBJ_DIR := obj
BIN_DIR := bin

VMLINUX := $(OBJ_DIR)/include/vmlinux.h
TRACER_SKEL := $(OBJ_DIR)/include/tracer.skel.h

INCLUDE := common $(OBJ_DIR)/include include
INCLUDE_FLAGS := $(patsubst %,-I%,$(INCLUDE))

# We exclude main.cpp from SRCS because otherwise we would have
# to filter object lists that gets compiled into test binary
SRC_DIR := src
SRCS := $(shell find $(SRC_DIR) ! -name "main.cpp" -name "*.cpp")
OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o, $(SRCS))
TARGET := $(BIN_DIR)/main

all: $(TARGET)

$(TARGET): $(TRACER_SKEL) $(VMLINUX) $(OBJS)
	@mkdir -p $(dir $@)
	clang++ -std=c++20 $(OBJS) $(SRC_DIR)/main.cpp $(INCLUDE_FLAGS) -lbpf -lelf -o $@
	sudo chown root $(TARGET)
	sudo chmod 4755 $(TARGET)

$(OBJS) : $(OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(dir $@)
	clang++ -std=c++20 $(INCLUDE_FLAGS) -c $< -o $@

# This throws warnings due to clash with previous command.
# This is intentional because bpf_provider depends on the skeleton (unlike other sources)
$(OBJ_DIR)/$(SRC_DIR)/bpf_provider.o : $(SRC_DIR)/bpf_provider.cpp $(TRACER_SKEL)
	@mkdir -p $(dir $@)
	clang++ -std=c++20 -Wno-c99-designator $(INCLUDE_FLAGS) -c $< -o $@


$(OBJ_DIR)/$(SRC_DIR)/tracer.bpf.o : $(VMLINUX) $(SRC_DIR)/backend/tracer.bpf.c
	@mkdir -p $(dir $@)
# -g flag is really important here, not sure why
	clang -g -I$(OBJ_DIR)/include -O3 -target bpf -D__TARGET_ARCH_x86_64 -c $(SRC_DIR)/backend/tracer.bpf.c -o $@


$(TRACER_SKEL): $(OBJ_DIR)/$(SRC_DIR)/tracer.bpf.o
	@mkdir -p $(dir $@)
	bpftool gen skeleton $(OBJ_DIR)/$(SRC_DIR)/tracer.bpf.o name tracer > $@

$(VMLINUX):
	@mkdir -p $(dir $@)
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > $@

# Tests
TEST_SRC_DIR := test/tests
TEST_SRCS := $(shell find $(TEST_SRC_DIR) -name "*.cpp")
TEST_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o, $(TEST_SRCS))
TEST_TARGET := $(BIN_DIR)/test

# Test programss
PROGRAM_PATH := $(BIN_DIR)/programs
PROGRAM_SRC_DIR := test/programs
PROGRAM_SRCS := $(shell find $(PROGRAM_SRC_DIR) -name "*.cpp")
PROGRAM_TARGETS := $(patsubst $(PROGRAM_SRC_DIR)/%.cpp,$(PROGRAM_PATH)/%, $(PROGRAM_SRCS))
PATH_DEFINES := -DPATH_TO_PROGRAMS="\"$(PROGRAM_PATH)\"" -DPATH_TO_DEBUGGER="\"$(TARGET)\""

test : $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET) : $(TEST_OBJS) $(OBJS) $(TARGET) $(PROGRAM_TARGETS)
	@mkdir -p $(dir $@)
	clang++ -std=c++20 $(TEST_OBJS) $(OBJS) -lbpf -lelf -lgtest -lgtest_main -pthread -o $@

$(TEST_OBJS) : $(OBJ_DIR)/%.o : %.cpp
	@mkdir -p $(dir $@)
	clang++ -std=c++20 $(INCLUDE_FLAGS) $(PATH_DEFINES) -c $< -o $@

$(PROGRAM_TARGETS) : $(BIN_DIR)/programs/% : $(PROGRAM_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	clang++ -std=c++20 $< -o $@

.PHONY: clean clean_fast test all
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Both TRACER_SKEL and VMLINUX take a long time to finish, so this
# options cleans everything expect their result (and tracer.bpf.o)
clean_fast:
	rm -rf $(BIN_DIR)
	find $(OBJ_DIR) -type f ! -name "*.h" ! -name "tracer.bpf.o" -exec rm -rf {} \;
