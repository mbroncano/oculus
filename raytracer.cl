//
//  raytracer.cl
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#define __IS_KERNEL__

#include "defs.h"
#include "geometry.h"
#include "primitives.h"
#include "random.h"
#include "ray.h"


inline bool bvh_intersect(const Ray *r, BUFFER_CONST_TYPE BVHNode *bvh)
{
	Vector sd = sign(r->d) * FLT_MAX;

// the second version is faster
//    float8 r_ = (float8)(r->o.s0, r->o.s1, r->o.s2, sd.s0, sd.s1, sd.s2, 0.f, 0.f);
//    float8 b_ = (float8)(bvh->min.s0, bvh->min.s1, bvh->min.s2, bvh->max.s0, bvh->max.s1, bvh->max.s2, 0.f, 0.f);
//    float8 m_ = min(r_, b_);
//    int8 e = (m_ == r_) + (m_ == b_);
//    return e.s0 || e.s1 || e.s2 || e.s3 || e.s4 || e.s5;

    float2 r0 = (float2)(r->o.s0, sd.s0);
    float2 r1 = (float2)(r->o.s1, sd.s1);
    float2 r2 = (float2)(r->o.s2, sd.s2);
    
    float2 b0 = (float2)(bvh->min.s0, bvh->max.s0);
    float2 b1 = (float2)(bvh->min.s1, bvh->max.s2);
    float2 b2 = (float2)(bvh->min.s2, bvh->max.s1);
    
    float2 m0 = min(r0, b0);
    float2 m1 = min(r1, b1);
    float2 m2 = min(r2, b2);
    
    int2 eq0 = (m0 == r0) + (m0 == b0);
    int2 eq1 = (m1 == r1) + (m1 == b1);
    int2 eq2 = (m2 == r2) + (m2 == b2);
    
    return eq0.x || eq0.y || eq1.x || eq1.y || eq2.x || eq2.y;
}

inline float scene_primitive_distance(BUFFER_CONST_TYPE Primitive *primitives, const Ray *r, const int index, BUFFER_CONST_TYPE Primitive **s, const float distance)
{
	BUFFER_CONST_TYPE Primitive *p = primitives + index;
	float d = min(distance, primitive_distance(p, r));
	*s = (d != distance)? p : *s;
	return d;
}

// TODO: implement shadow ray hit: leaves on the first hit which is not the light
static float scene_intersect(
    BUFFER_CONST_TYPE Primitive *primitives,
    const int numprimitives,
    const Ray *r,
    BUFFER_CONST_TYPE Primitive **s,
    BUFFER_CONST_TYPE BVHNode *bvh)
{
	float distance = FLT_MAX;
#ifdef USE_BVH
    int cur = 0;
    int end = bvh->skip;
    
    while (cur < end) {
        BUFFER_CONST_TYPE BVHNode *n = bvh + cur;
        if (bvh_intersect(r, n)) {
            if (n->pid != P_NONE)
                distance = scene_primitive_distance(primitives, r, n->pid, s, distance);
            cur ++;
        } else {
            cur = n->skip;
        }
    }
#else
    for (int i = 0; i < numprimitives; i++) {
		distance = scene_primitive_distance(primitives, r, i, s, distance);
	}
#endif
    
	return distance;
} 

inline float kdiff_lambert(const Ray *i, const Ray *o, const Vector normal)
{
	return max(0.f, dot(i->d, normal));
}

// http://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_shading_model
inline float kspec_blinnphong(const Ray *i, const Ray *o, const Vector normal, const Vector t)
{
	const float nshiny = 4.f;
	const Vector h = normalize(i->d + o->d);
	
	return pow(dot(normal, h), nshiny);
}
/*
inline float kspec_gaussian(const Ray *i, const Ray *o, const Vector normal, const Vector t)
{
	const float m = 0.7f;
	const Vector h = normalize(i->d + o->d);
	float a = acos(dot(normal, h));
	
	return exp(-1.f * pow(a / m, 2));
}

inline float kspec_beckmann(const Ray *i, const Ray *o, const Vector normal, const Vector t)
{
	const float m = 0.7f;
	const Vector h = normalize(i->d + o->d);
	float a = acos(dot(normal, h));
	float cos_a2 = cos(a) * cos(a);
	float e = (1.f - cos_a2) / (cos_a2 * m * m);
	
	return exp(-1.f * e) / (PI * m * m * cos_a2 * cos_a2);
}

inline float kspec_heidrich_seidel(const Ray *i, const Ray *o, const Vector normal, const Vector t)
{
	const float ka = 1e3f;
	const float a = dot(i->d, t);
	const float b = dot(o->d, t);

	return pow(sin(a)*sin(b) + cos(a)*cos(b), ka) * kspec_blinnphong(i, o, normal, t);
}
*/
static Vector scene_illumination(
	BUFFER_CONST_TYPE Primitive *primitives,
	const int numprimitives,
	random_state_t *rnd,
	
	BUFFER_CONST_TYPE Primitive *s,
	const Ray *r,
	const Vector hit_point,
	const Vector normal,
	const float cos_i,
	BUFFER_CONST_TYPE BVHNode *bvh
	)
{
	Vector illu = vec_zero;
	Vector tangent = cross(normal, normalize(r->d + 2.f * cos_i * normal));
	
	for (int i = 0; i < numprimitives; i++) {
		BUFFER_CONST_TYPE Primitive *l = primitives + i;
		if (l->m.e != 0.f) {
			Vector light_hit = primitive_surfacepoint(l, randomf(rnd), randomf(rnd));
			
			Ray s_ray = {hit_point + normal * EPSILON, normalize(light_hit - hit_point)};

			BUFFER_CONST_TYPE Primitive *h;
			float d = scene_intersect(primitives, numprimitives, &s_ray, &h, bvh);
			if (h == l) {
				Vector emission = l->m.c * l->m.e;

				// Phong illumination model http://en.wikipedia.org/wiki/Phong_reflection_model
				float attenuation = 2.f * sqrt(d);
				float kdiff = kdiff_lambert(&s_ray, r, normal);
				float kspec = kspec_blinnphong(&s_ray, r, normal, tangent);

				illu = illu + emission * (kdiff + kspec) / attenuation;
			}
		}
	}
	
	return illu + ambient;
}

static Vector scene_sample(
	BUFFER_CONST_TYPE Primitive *primitives,
	const int numprimitives,
	random_state_t *rnd,
	const Ray *ray,
	BUFFER_CONST_TYPE BVHNode *bvh
)
{
	int depth = 6;
	bool bounce = true;
	Vector sample = vec_zero;
	Vector illum = vec_one;
	Ray r = *ray;

	while (--depth) {
		BUFFER_CONST_TYPE Primitive *s = 0;
		float distance = scene_intersect(primitives, numprimitives, &r, &s, bvh);
		if (distance == FLT_MAX) {
			return sample;
		}
		
		// Lights
		if (s->m.e != 0.f) {
			// HACK?: only return full luminance when either hit by a primary ray or
			// after a specular bounce, as diffuse already sampled it previously
			return bounce ? (sample + illum * s->m.e * s->m.c) : sample;
		} 

		// intersection
		Vector hit_point = r.o + r.d * distance;
		Vector normal = primitive_normal(s, hit_point);
		
		// correct normals, simt style
		float cos_i = -1.f * dot(normal, normalize(r.d));
		float csign = sign(cos_i);

		normal = normal * csign;
		cos_i = cos_i * csign;
		bool leaving = (csign < 0.f);
		Surface material = s->m.s;
		
		illum = illum * s->m.c;

		// BRDFs, TODO: move them to functions
		// Avoiding switch decreases 8% frame time!
		if (material == Diffuse) {
			bounce = false;
		
			sample = sample + illum * scene_illumination(primitives, numprimitives, rnd, s, &r, hit_point, normal, cos_i, bvh);
		
			ray_bounce(&r, hit_point, normal, rnd);
		} 
		else if (material == Metal) {
			bounce = true;
			sample = sample + illum * scene_illumination(primitives, numprimitives, rnd, s, &r, hit_point, normal, cos_i, bvh);

			ray_reflection(&r, hit_point, normal, cos_i);
		}
		else if (material == Specular) {
			bounce = true;
		
			ray_reflection(&r, hit_point, normal, cos_i);
		}
		else if (material == Dielectric) {
			bounce = true;
		
			const float air = 1.f;
			const float glass = 1.5f;

			float n1 = leaving? glass : air;
			float n2 = leaving? air : glass;
			float n = n1 / n2;

			float cos_t2 = 1.f - pow(n, 2) * (1.f - pow(cos_i, 2));

			if (cos_t2 < 0.f) {
				ray_reflection(&r, hit_point, normal, cos_i);
			} else {
				float cos_t = sqrt(cos_t2);

				// TODO: implement Schlick approx.
				float perp = pow((n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t), 2.f);
				float para = pow((n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t), 2.f);
				float fres = (perp + para) / 2.f;
				
				if (randomf(rnd) < fres) {
					ray_reflection(&r, hit_point, normal, cos_i);
				} else {
					ray_refraction(&r, hit_point, normal, cos_i, cos_t, n);
				}
			}
			
		} 

	}


	return sample;
}

static Ray camera_genray( BUFFER_CONST_TYPE Camera *camera, float x, float y, int width, int height)
{
	const float fov = radians(45.f);
	const float fx = (float)x / width - 0.5f;
	const float fy = (float)y / height - 0.5f;
	const float zoom = 1.f;

	Vector d = normalize(camera->t - camera->o);
	Vector vx = normalize(cross(d, vec_y)) * (width * fov / height);
	Vector vy = normalize(cross(vx, d)) * fov;
	
	return (Ray){camera->o, normalize(d + zoom * vx * fx + zoom * vy * fy)};
}

__kernel void raytracer(
	BUFFER_CONST_TYPE Primitive *primitives,
	int numprimitives,
	BUFFER_CONST_TYPE Camera *camera,
	random_state_t seed,
	__global Vector *frame,
#ifdef INTEROP
	write_only image2d_t image,
#else
	__global Pixel *rgb,
#endif
	unsigned int samples,
	BUFFER_CONST_TYPE BVHNode *bvh,
	int numbvh
	)
{
	// work items and size
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(1);

	// xor seed per work items
	seed ^= (random_state_t)(x, y);

#ifdef DEBUG
	if (samples != 0 && x ==0 && y ==0) {
		dump_primitives(primitives_l, numprimitives);
	}
#endif

	// antialiasing
	float dx = x + randomf(&seed) - 0.5f;
	float dy = y + randomf(&seed) - 0.5f;

	// generate primary ray and path tracing
	Ray ray = camera_genray(camera, dx, dy, width, height);
	Vector pixel = scene_sample(primitives, numprimitives, &seed, &ray, bvh);

	// averages the pixel, except for the first
	uint index = y * width + x;
	if (samples > 1) {
		const float k1 = samples;
		const float k2 = 1.f / (samples + 1.f);
		pixel = (frame[index] * k1 + pixel) * k2;
	}
	frame[index] = pixel;
	
	// return RGBA image
#ifdef INTEROP
	// it does the clamp by itself it seems
	write_imagef(image, (int2)(x, y), (float4)(pixel.x, pixel.y, pixel.z, 0.f));
#else
	rgb[index].r = convert_uchar_sat(pixel.x * 256);
	rgb[index].g = convert_uchar_sat(pixel.y * 256);
	rgb[index].b = convert_uchar_sat(pixel.z * 256);
#endif
}
