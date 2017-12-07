SRC=main.cpp scene.cpp bvhtree.cpp opencl.cpp
HEADERS=defs.h geometry.h cl.hpp util.h scene.h bvhtree.h opencl.h opencl_debug.h
CPP=clang++
CC=clang
LDFLAGS=
CCFLAGS=-framework OpenCL -framework OpenGl -framework Glut -O3 -O2 -g -Wno-deprecated-declarations -std=gnu++11

all: parson.o oculus

parson.o: parson.c
	$(CC) parson.c -c -o parson.o

oculus: $(SRC) Makefile parson.o
	$(CPP) $(CCFLAGS) $(LDFLAGS) $(SRC) parson.o -o oculus

clean:
	rm -rf oculus parson.o
