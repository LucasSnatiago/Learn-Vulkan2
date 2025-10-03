CXXFLAGS=-std=c++17 -O2 -I./include $(shell pkg-config --cflags glfw3 fmt)
LDFLAGS=$(shell pkg-config --libs glfw3 fmt) -lvulkan
INCLUDES=-I./include
SRC:=$(wildcard ./src/*.cpp)
CXX=g++
BIN=VulkanTest

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: test clean

test: $(BIN)
	./$(BIN)

clean:
	$(RM) $(BIN)
