#ifndef GEOMETRY_H
#define GEOMETRY_H

#ifdef __IS_KERNEL
typedef float3 Vector; 
typedef struct {
	uchar r, g, b;
} Pixel; 

__constant const Vector vec_zero = (Vector)(0.f, 0.f, 0.f);
__constant const Vector vec_up = (Vector)(0.f, -1.f, 0.f);
__constant const Vector vec_one = (Vector)(1.f, 1.f, 1.f);

#define EPSILON 1e-2f
#define PI M_PI_F

#else
#include <math.h>

typedef cl_float3 Vector;
typedef struct {
	cl_uchar r, g, b;
} Pixel;

/*
float length(Vector a) {
	return sqrt(a.s[0] * a.s[0] + a.s[1] * a.s[1] + a.s[2] * a.s[2]);
}

Vector smul(Vector a, float s) {
	Vector ret;
	ret.s[0] = a.s[0] * s;
	ret.s[1] = a.s[1] * s;
	ret.s[2] = a.s[2] * s;

	return ret;
}

Vector normalize(Vector a) {
	return smul(a, 1.f / length(a));
}

Vector cross(Vector a, Vector b) {
	Vector ret;
	ret.s[0] = a.s[1] * a.s[2] - a.s[2] * a.s[1];
	ret.s[1] = a.s[2] * a.s[0] - a.s[0] * a.s[2];
	ret.s[2] = a.s[0] * a.s[1] - a.s[1] * a.s[0];

	return ret;
}
*/
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