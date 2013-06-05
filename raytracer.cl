#define __IS_KERNEL

#include "geometry.h"
#include "sphere.h"
#include "triangle.h"
#include "random.h"
#include "ray.h"

static float primitive_distance(__local const Primitive *p, const Ray *r)
{
	if (p->t == sphere) {
		return sphere_distance(&p->sphere, r);
	} else if (p->t == triangle) {
		return triangle_distance(&p->triangle, r);
	}
	return 0.f;
}

static Vector primitive_surfacepoint(__local const Primitive *p, const float a, const float b)
{
	if (p->t == sphere) {
		return sphere_surfacepoint(&p->sphere, a, b);
	}
	return vec_zero;
}

static float scene_intersect(__local const Primitive *primitives, const int numprimitives, const Ray *r, __local Primitive **s)
{
	float distance = FLT_MAX;
	for (int i = 0; i < numprimitives; i++) {
		float d = primitive_distance(primitives + i, r);
		if (d < distance) {
			*s = (__local Primitive *)(primitives + i);
			distance = d;
		}
	}
	return distance;
}

static Vector scene_illumination(
	__local const Primitive *primitives,
	const int numprimitives,
	random_state_t *rnd,
	
	__local const Primitive *s,
	const Ray *r,
	const Vector hit_point,
	const Vector normal
	)
{
	Vector illu = vec_zero;
	
	for (int i = 0; i < numprimitives; i++) {
		__local const Primitive *l = primitives + i;
		if (l->m.e != 0.f) {
			Vector light_hit = primitive_surfacepoint(l, randomf(rnd), randomf(rnd));
			
			Ray s_ray = {hit_point + normal * EPSILON, normalize(light_hit - hit_point)};

			__local Primitive *h;
			float d = scene_intersect(primitives, numprimitives, &s_ray, &h);

			if (h == l) {
				Vector emission = l->m.c * l->m.e;
				float lambert = max(0.f, dot(s_ray.d, normal));
				float blinn = pow(dot(normalize(s_ray.d + r->d), normal), 64.f);
				float attenuation = sqrt(d);

				illu = illu + emission * (lambert + blinn) * 0.5f / attenuation;
			}
		}
	}
	return illu;
}

inline Vector primitive_normal(__local const Primitive *p, Vector hit_point)
{
	if (p->t == sphere) {
		return normalize(hit_point - p->sphere.c);
	}
	return vec_zero;
}



static Vector scene_sample(
	__local const Primitive *primitives,
	const int numprimitives,
	random_state_t *rnd,
	const Ray *ray
)
{
	int depth = 6;
	bool bounce = true;
	Vector sample = vec_zero;
	Vector illum = vec_one;
	Ray r = *ray;

	while (--depth) {
		__local Primitive *s = 0;
		float distance = scene_intersect(primitives, numprimitives, &r, &s);

		if (distance == FLT_MAX) {
			return sample;
		}

		// Lights
		if (s->m.e != 0.f) {
			// HACK: only return full luminance when either hit by a primary ray or
			// after a specular bounce, as diffuse already sampled it previously
			if (bounce) { 
				sample = sample + illum * s->m.e * s->m.c;
			}
			return sample;
		} 
		
		bool leaving = false;
		Vector hit_point = r.o + r.d * distance;
		Vector normal = primitive_normal(s, hit_point);
		float cos_i = -1.f * dot(normal, normalize(r.d));
		if (cos_i < 0.f) {
			normal = normal * -1.f;
			cos_i = cos_i * -1.f;
			leaving = true;
		}

		// BDRFs, TODO: move them to functions
		if (s->m.s == Diffuse) {
			bounce = false;
			illum = illum * s->m.c;
			
			Vector light = scene_illumination(primitives, numprimitives, rnd, s, &r, hit_point, normal);
			sample = sample + illum * (light + ambient);
			
			ray_bounce(&r, hit_point, normal, rnd);
		}
		 else if (s->m.s == Specular) {
			bounce = true;
			illum = illum * s->m.c;
			
			ray_reflection(&r, hit_point, normal, cos_i);
		} 
		else if (s->m.s == Refractive) {
			bounce = true;
			illum = illum * s->m.c;
			
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
					r.o = hit_point - normal * EPSILON;
					r.d = normalize(n * r.d - (n * cos_i + cos_t) * normal);
				}
			}
		}
	}

	return sample;
}



static Ray camera_genray( __global const Camera *camera, float x, float y, int width, int height)
{
	const float fov = PI / 180.f * 45.f;
	const float fx = (float)x / width - 0.5f;
	const float fy = (float)y / height - 0.5f;
	const float zoom = 1.f;

	Vector d = normalize(camera->t - camera->o);
	Vector vx = normalize(cross(d, vec_y)) * (width * fov / height);
	Vector vy = normalize(cross(vx, d)) * fov;
	
	return (Ray){camera->o, normalize(d + zoom * vx * fx + zoom * vy * fy)};
}

kernel void raytracer(
	__global Primitive *spheres, 
	int numprimitives,
	__global const Camera *camera,
	__local const Primitive *primitives_l, 
	random_state_t seed,
	__global Vector *frame,
#ifdef INTEROP
	write_only image2d_t image,
#else
	__global Pixel *rgb,
#endif
	unsigned int samples
	)
{
	// work items and size
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(1);

	// xor seed per work items
	seed ^= (random_state_t)(x, y);

	// copy global to local memory
	event_t event = async_work_group_copy((__local char *)primitives_l, (__global char *)spheres, sizeof(Primitive)*numprimitives, 0);
	wait_group_events(1, &event);

	// antialiasing
	float dx = x + randomf(&seed) - 0.5f;
	float dy = y + randomf(&seed) - 0.5f;

	// generate primary ray and path tracing
	Ray ray = camera_genray(camera, dx, dy, width, height);
	Vector pixel = scene_sample(primitives_l, numprimitives, &seed, &ray);

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
	write_imagef(image, (int2)(x, y), (float4)(pixel.x, pixel.y, pixel.z, 0.f));
#else
	rgb[index].r = convert_uchar_sat(pixel.x * 256);
	rgb[index].g = convert_uchar_sat(pixel.y * 256);
	rgb[index].b = convert_uchar_sat(pixel.z * 256);
#endif
}
