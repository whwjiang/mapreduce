CXX      ?= g++
CXXFLAGS ?= -std=c++26 -O2 -Wall -Wextra -Werror -pedantic -I src
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
