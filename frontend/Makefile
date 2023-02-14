CXX := g++
CXXFLAGS := -std=c++20 -Wall -Iinclude
LDLIBS := -lfmt

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
EXAMPLES_DIR := examples

SRCS := $(shell find $(SRC_DIR) -name "*.cpp")
OBJS := $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(SRCS:.cpp=.o))
DEPS := $(OBJS:.o=.d)
EXAMPLE_SRCS := $(shell find $(EXAMPLES_DIR) -name "*.cpp")
EXAMPLE_BINS := $(patsubst $(EXAMPLES_DIR)/%,$(BIN_DIR)/%,$(EXAMPLE_SRCS:.cpp=))

.PHONY: all
all: $(EXAMPLE_BINS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

$(EXAMPLE_BINS): $(BIN_DIR)/%: $(EXAMPLES_DIR)/%.cpp $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< $(OBJS) $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
