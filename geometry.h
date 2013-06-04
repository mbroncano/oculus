#ifndef GEOMETRY_H
#define GEOMETRY_H

#define INTEROP


#ifdef __IS_KERNEL
typedef float3 Vector; // waaay faster than float4 for CPU, the same for GPU
typedef struct {
	uchar r, g, b;
} Pixel; 

typedef uint2 random_state_t;


#else

typedef cl_float3 Vector;
typedef struct {
	cl_uchar r, g, b;
} Pixel;

typedef cl_uint2 random_state_t;

#endif

typedef struct {
	Vector o, d;
} Ray;

typedef enum {
	Diffuse, Specular, Refractive
} Surface;

typedef struct {
	Surface s;
	Vector c;
	float e;
} Material;

typedef struct {
	Vector c;
	float r;
	Material m;
} Sphere;

typedef struct {
	Vector o, t;
} Camera;

#endif