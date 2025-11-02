CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -lstdc++fmt -Wall -Wextra -pedantic -I src
LDFLAGS  ?= -pthread
BUILD    := build

$(BUILD)/wordcount: examples/wordcount.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(BUILD):
	mkdir -p $@

.PHONY: all clean
all: $(BUILD)/wordcount
clean:
	rm -rf $(BUILD)