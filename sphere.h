#ifndef _SPHERE_H
#define _SPHERE_H

#include "geometry.h"

// optimized; assumes ray direction is normalized (so the 'a' term can be 1.f)
inline float sphere_distance(__local const Sphere *s, const Ray *ray)
{
	// inverting this saves negating b
	Vector v = s->c - ray->o;

	float b = dot(v, ray->d);
	float d = b * b - dot(v, v) + s->r * s->r;
	if (d < 0.f) {
		return FLT_MAX;
	}

	d = sqrt(d);
	float t = b - d;
	if (t > 0.f)
		return t;
	t = b + d;
	if (t > 0.f)
		return t;
	return FLT_MAX;
}

inline Vector sphere_surfacepoint(__local const Sphere *s, const float u, const float v)
{
	const float z = 1.f - 2.f * u;				// z in [-1, 1] -> z = cos w
	const float r = sqrt(max(0.f, 1 - z * z));	// sqrt(1 - x^2) -> sin w
	const float p = 2 * PI * v;					// p in [0, 2pi]
	const float x = r * cos(p);
	const float y = r * sin(p);
	
	return s->c + s->r * (Vector)(x, y, x);
}

inline Vector sphere_normal(__local const Sphere *s, const Vector hit_point)
{
	return normalize(hit_point - s->c);
}

/*
inline Vector sphere_tangent(__local const Sphere *s, const Vector hit_point)
{
	Vector u = hit_point - s->c;
	
	if (u.x < u.y && u.x < u.z)
		return cross(normalize((Vector)(0.f, -u.z, u.y)), u);
	else if (u.y < u.x && u.y < u.z)
		return cross(normalize((Vector)(-u.z, 0.f, u.x)), u);
	else
		return cross(normalize((Vector)(-u.y, u.x, 0.f)), u);
	
}
*/

#endif