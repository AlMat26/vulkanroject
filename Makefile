
CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

vulkanProject: main.cpp
	g++ $(CFLAGS) -o vulkanproject main.cpp $(LDFLAGS)

.PHONY: test clean

test: vulkanProject
	vulkanproject

clean:
	rm -f vulkanproject
