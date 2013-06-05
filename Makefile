SRC=main.cpp
CC=clang++
LDFLAGS=-Llibjson -ljson
CCFLAGS=-framework OpenCL -framework OpenGl -framework Glut -O3 -O2 -g

oculus: $(SRC) Makefile raytracer.cl geometry.h cl.hpp
	$(CC) $(CCFLAGS) $(LDFLAGS) $(SRC) -o oculus

all: oculus

clean:
	rm -rf oculus
