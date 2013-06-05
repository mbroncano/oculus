#ifndef _RAY_H
#define _RAY_H

#include "geometry.h"
#include "random.h"

inline Vector vector_rotate(const Vector normal, const float s0, const float s1)
{
	float r1 = 2 * PI * s0;
	float r2 = s1;
	float r2s = sqrt(r2);
	
	Vector w = normal;
	Vector a;
	// HACK: ???
	if (fabs(w.x) > .1f)
		a = vec_y;
	else
		a = vec_x;
	Vector u = normalize(cross(a, w));
	Vector v = cross(w, u);
	
	Vector dir = u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2);
	
	return dir;
}

inline void ray_reflection(Ray *ray, const Vector hit_point, const Vector normal, const float cos_i)
{
	ray->o = hit_point + normal * EPSILON;
	ray->d = normalize(ray->d + 2.f * cos_i * normal);
}

inline void ray_refraction(Ray *ray, const Vector hit_point, const Vector normal, const float cos_i, const float cos_t, const float n)
{
	ray->o = hit_point - normal * EPSILON;
	ray->d = normalize(n * ray->d - (n * cos_i + cos_t) * normal);
}

inline void ray_bounce(Ray *ray, const Vector hit_point, const Vector normal, random_state_t *rnd)
{
	ray->o = hit_point + normal * EPSILON;
	ray->d = vector_rotate(normal, randomf(rnd), randomf(rnd));
}

#endif