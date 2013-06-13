//
//  primitives.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/13/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef Oculus_primitives_h
#define Oculus_primitives_h

#ifdef DEBUG
static void dump_primitives(BUFFER_CONST_TYPE Primitive *primitives, int num)
{
	for (int i = 0; i < num; i ++) {
		BUFFER_CONST_TYPE Primitive *p = primitives + i;
		
		if (p->t == sphere) {
			printf("[%d] s: (%.2f, %.2f, %.2f), %.2f\n", i, p->sphere.c.x, p->sphere.c.y ,p->sphere.c.z, p->sphere.r);
		} else if (p->t == triangle) {
			Vector n = triangle_normal(&p->triangle, vec_zero);
			printf("[%d] t: (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), n[%.2f, %.2f, %.2f]\n", i,
                   p->triangle.p[0].x, p->triangle.p[0].y, p->triangle.p[0].z,
                   p->triangle.p[1].x, p->triangle.p[1].y, p->triangle.p[1].z,
                   p->triangle.p[2].x, p->triangle.p[2].y, p->triangle.p[2].z,
                   n.x, n.y, n.z
                   );
		}
	}
}
#endif

// optimized; assumes ray direction is normalized (so the 'a' term can be 1.f)
inline float sphere_distance(BUFFER_CONST_TYPE Sphere *s, const Ray *ray)
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

inline Vector sphere_surfacepoint(BUFFER_CONST_TYPE Sphere *s, const float u, const float v)
{
	const float z = 1.f - 2.f * u;				// z in [-1, 1] -> z = cos w
	const float r = sqrt(max(0.f, 1 - z * z));	// sqrt(1 - x^2) -> sin w
	const float p = 2 * PI * v;					// p in [0, 2pi]
	const float x = r * cos(p);
	const float y = r * sin(p);
	
	return s->c + s->r * (Vector)(x, y, x);
}

inline Vector sphere_normal(BUFFER_CONST_TYPE Sphere *s, const Vector hit_point)
{
	return normalize(hit_point - s->c);
}

//inline Vector sphere_tangent(BUFFER_CONST_TYPE Sphere *s, const Vector hit_point)
//{
//    Vector u = hit_point - s->c;
//    
//    if (u.x < u.y && u.x < u.z)
//        return cross(normalize((Vector)(0.f, -u.z, u.y)), u);
//    else if (u.y < u.x && u.y < u.z)
//        return cross(normalize((Vector)(-u.z, 0.f, u.x)), u);
//    else
//        return cross(normalize((Vector)(-u.y, u.x, 0.f)), u);
//    
//}

// Moller - Trumbore method
inline float triangle_distance(BUFFER_CONST_TYPE Triangle *t, const Ray *r)
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

inline Vector triangle_surfacepoint(BUFFER_CONST_TYPE Triangle *t, const float u, const float v)
{
	const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };
    
	// TODO: use mad() ?
	return t->p[0] + edge[0] * u + edge[1] * v;
}

inline Vector triangle_normal(BUFFER_CONST_TYPE Triangle *t, const Vector hit_point)
{
	const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };
    
	return normalize(cross(edge[0], edge[1]));
}


//inline textcoord_t triangle_textcoords(BUFFER_CONST_TYPE Triangle *t, const Vector hit_point)
//{
//    const Vector edge[2] = { (t->p[1] - t->p[0]), (t->p[2] - t->p[0]) };
//    const float2 len2 = { pow(length(edge[0]), 2), pow(length(edge[1]), 2) };
//    const textcoord_t uv = { dot(hit_point, edge[0]), dot(hit_point, edge[1]) };
//    
//    return uv / len2;
//}

static float primitive_distance(BUFFER_CONST_TYPE Primitive *p, const Ray *r)
{
	if (p->t == sphere) {
		return sphere_distance(&p->sphere, r);
	} else if (p->t == triangle) {
		return triangle_distance(&p->triangle, r);
	}
	return 0.f;
}

static Vector primitive_surfacepoint(BUFFER_CONST_TYPE Primitive *p, const float a, const float b)
{
	if (p->t == sphere) {
		return sphere_surfacepoint(&p->sphere, a, b);
	} else if (p->t == triangle) {
		return triangle_surfacepoint(&p->triangle, a, b);
	}
	return vec_zero;
}

static Vector primitive_normal(BUFFER_CONST_TYPE Primitive *p, const Vector hit_point)
{
	if (p->t == sphere) {
		return sphere_normal(&p->sphere, hit_point);
	} else if(p->t == triangle) {
		return triangle_normal(&p->triangle, hit_point);
	}
	return vec_zero;
}

#endif
