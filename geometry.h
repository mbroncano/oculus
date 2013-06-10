#ifndef GEOMETRY_H
#define GEOMETRY_H

#define INTEROP
#define MAIN_DEVICE CL_DEVICE_TYPE_GPU
//#define DEBUG

#ifdef __IS_KERNEL

#define EPSILON 1e-2f
#define PI M_PI_F

typedef float3 Vector; // waaay faster than float4 for CPU, the same for GPU
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

#else

typedef cl_float3 Vector;
typedef struct {
	cl_uchar r, g, b;
} Pixel;

typedef cl_uint2 random_state_t;

typedef cl_float2 textcoord_t;

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

typedef struct {
	int left;
	int right;
	int pid;
	Vector min, max;
} BVH;

#ifndef __IS_KERNEL

Vector operator+(Vector &a, Vector &b) {
	return (Vector){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector operator-(Vector &a, Vector &b) {
	return (Vector){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector min(Vector &a, Vector &b)
{
	Vector r;
	r.x = std::min(a.x, b.x);
	r.y = std::min(a.y, b.y);
	r.z = std::min(a.z, b.z);
	return r;
}

Vector max(Vector &a, Vector &b)
{
	Vector r;
	r.x = std::max(a.x, b.x);
	r.y = std::max(a.y, b.y);
	r.z = std::max(a.z, b.z);
	return r;
}

Vector min(Vector &a, Vector &b, Vector &c)
{
	Vector r;
	r.x = std::min(std::min(a.x, b.x), std::min(b.x, c.x));
	r.y = std::min(std::min(a.y, b.y), std::min(b.y, c.y));
	r.z = std::min(std::min(a.z, b.z), std::min(b.z, c.z));
	return r;
}

Vector max(Vector &a, Vector &b, Vector &c)
{
	Vector r;
	r.x = std::max(std::max(a.x, b.x), std::max(b.x, c.x));
	r.y = std::max(std::max(a.y, b.y), std::max(b.y, c.y));
	r.z = std::max(std::max(a.z, b.z), std::max(b.z, c.z));
	return r;
}

std::ostream& operator<<(std::ostream& os, const Vector& a)
{
	os << setiosflags(std::ios::fixed) << std::setprecision(1);
	os << "(" << a.x << ", " << a.y << ", " << a.z << ")";
	return os;
}

std::ostream& operator<<(std::ostream& os, const BVH& a)
{
	os << "[flat] min" << a.min << " max" << a.max << " left:" << a.left << " right:" << a.right << " pid:" << a.pid;
	return os;
}

std::ostream& operator<<(std::ostream& os, const Primitive& a)
{
	((a.t == sphere)?(os << "[s] c" << a.sphere.c << " r:" << a.sphere.r):(os << "[t] p0" << a.triangle.p[0] << " p1" << a.triangle.p[1] << " p2" << a.triangle.p[2]));
	return os;
}

#endif

#endif