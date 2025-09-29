CFLAGS=-std=c++17 -O2
# Release
#LDFLAGS=-Wl,-Bstatic -lfmt -lglfw \
#        -Wl,-Bdynamic -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
LDFLAGS=-lfmt -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
# Adding native support for wayland
# LDFLAGS+=-lwayland-client -lwayland-cursor -lwayland-egl
INCLUDES=-I./include
SRC=./src/*.cpp
CC=gcc
CXX=g++
BIN=VulkanTest

$(BIN): $(SRC)
	$(CXX) $(INCLUDES) $(CFLAGS) -o $(BIN) $(SRC) $(LDFLAGS)

.PHONY: test clean

test: $(BIN)
	./$(BIN)

clean:
	$(RM) $(BIN)
