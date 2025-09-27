CFLAGS=-std=c++17 -O2
# Release
#LDFLAGS=-Wl,-Bstatic -lfmt -lglfw \
#        -Wl,-Bdynamic -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
LDFLAGS=-lfmt -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
CC=gcc
CXX=g++
BIN=VulkanTest

$(BIN): main.cpp
	$(CXX) $(CFLAGS) -o $(BIN) main.cpp $(LDFLAGS)

.PHONY: test clean

test: $(BIN)
	./$(BIN)

clean:
	$(RM) $(BIN)
