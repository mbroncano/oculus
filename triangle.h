#ifndef _TRIANGLE_H
#define _TRIANGLE_H

#include "geometry.h"

// Moller - Trumbore method
inline float triangle_distance(__local const Triangle *t, const Ray *r)
{
	const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };
	
	const Vector o = r->o - t->p[0];
	const Vector p = cross(r->d, edge[1]);

	const float det1 = dot(p, edge[0]);
	if (fabs(det1) < EPSILON)
		return FLT_MAX;

	const float u = dot(p, o) / det1;
	if (u < 0.f || u > 1.f)
		return FLT_MAX;

	const Vector q = cross(o, edge[0]);
	const float v = dot(q, r->d) / det1;
	if (v < 0.f || (0.f + v) > 1.f) // H0XX !!!
		return FLT_MAX;

	float ret = dot(q, edge[1]) / det1;
	return ret > 0.f ? ret : FLT_MAX;
}

inline Vector triangle_surfacepoint(__local const Triangle *t, const float u, const float v)
{
	const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };

	return t->p[0] + edge[0] * u + edge[1] * v;
}

inline Vector triangle_normal(__local const Triangle *t, const Vector hit_point)
{
	const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };

	return normalize(cross(edge[0], edge[1]));
}

/*
inline textcoord_t triangle_textcoords(__local const Triangle *t, const Vector hit_point)
{
	const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };
	const float2 len2 = { pow(length(edge[0]), 2), pow(length(edge[1]), 2) };
	const textcoord_t uv = { dot(hit_point, edge[0]), dot(hit_point, edge[1]) };
	
	return uv / len2;
}
*/

#endif