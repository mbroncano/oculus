SRC=main.cpp
CPP=clang++
CC=clang
LDFLAGS=
CCFLAGS=-framework OpenCL -framework OpenGl -framework Glut -O3 -O2 -g

all: parson.o oculus

parson.o: parson/parson.c
	$(CC) parson/parson.c -c -o parson.o

oculus: $(SRC) Makefile raytracer.cl geometry.h cl.hpp parson.o
	$(CPP) $(CCFLAGS) $(LDFLAGS) $(SRC) parson.o -o oculus

clean:
	rm -rf oculus parson.o
