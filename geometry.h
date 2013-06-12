//
//  geometry.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef Oculus_geometry_h
#define Oculus_geometry_h

#define P_NONE UINT_MAX

#ifdef __IS_KERNEL__

#define EPSILON 1e-2f
#define PI M_PI_F

typedef float3 Vector;

typedef struct {
	uchar r, g, b;
} Pixel;

typedef float2 textcoord_t;

typedef uint2 random_state_t;

__constant const Vector vec_zero =	(Vector)(0.f, 0.f, 0.f);
__constant const Vector vec_y =		(Vector)(0.f, 1.f, 0.f);
__constant const Vector vec_x =		(Vector)(1.f, 0.f, 0.f);
__constant const Vector vec_one =	(Vector)(1.f, 1.f, 1.f);
__constant const Vector ambient =	(Vector)(.1f, .1f, .1f);

typedef struct {
    uint pid;    // primitive index
    uint skip;   // the distance to the right node
    Vector min, max;
} BVHNode;


#else

#include <OpenCL/OpenCL.h>

typedef cl_float3 Vector;

typedef struct {
	cl_uchar r, g, b;
} Pixel;

typedef cl_uint2 random_state_t;

typedef cl_float2 textcoord_t;

const Vector vec_zero = (Vector){{0.f, 0.f, 0.f}};

typedef struct {
    cl_uint pid;    // primitive index
    cl_uint skip;   // the distance to the right node
    Vector min, max;
} BVHNode;

#endif

typedef struct {
	Vector o, d;
} Ray;

typedef enum {
	Diffuse, Specular, Dielectric, Metal
} Surface;

typedef struct {
	Surface s;
	Vector c;
	float e;
} Material;

typedef struct {
	Vector c;
	float r;
} Sphere;

typedef struct {
	Vector p[3];
} Triangle;

typedef struct {
	Vector o, t;
} Camera;

typedef enum {
	sphere, triangle
} PrimitiveType;

typedef struct {
	union {
		Sphere sphere;
		Triangle triangle;
	};
	Material m;
	PrimitiveType t;
} Primitive;

#endif
