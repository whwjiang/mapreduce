CXX      ?= g++
CXXFLAGS ?= -std=c++26 -O2 -Wall -Wextra -Werror -pedantic -I src
LDFLAGS  ?= -pthread
BUILD    := build
LIB      := $(BUILD)/libmapreduce.a
OBJS     := $(BUILD)/ThreadPool.o

EXAMPLE_SRCS := examples/wordcount/wordcount.cpp

$(BUILD)/wordcount: $(EXAMPLE_SRCS) $(LIB) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(EXAMPLE_SRCS) -o $@ $(LIB) $(LDFLAGS)

$(LIB): $(OBJS) | $(BUILD)
	ar rcs $@ $^

$(BUILD)/ThreadPool.o: src/ThreadPool.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

.PHONY: all clean
all: $(BUILD)/wordcount
clean:
	rm -rf $(BUILD)
