#define __IS_KERNEL

#include "geometry.h"

__constant const Vector vec_zero = (Vector)(0.f, 0.f, 0.f);
__constant const Vector vec_up =   (Vector)(0.f, 1.f, 0.f);
__constant const Vector vec_one =  (Vector)(1.f, 1.f, 1.f);
__constant const Vector ambient =  (Vector)(.1f, .1f, .1f);

#define EPSILON 1e-2f
#define PI M_PI_F

// optimized; assumes ray direction is normalized (so the 'a' term can be 1.f)
inline float sphereDistance(Ray r, __local const Sphere *s)
{
	// inverting this saves negating b
	Vector v = s->c - r.o;

	float b = dot(v, r.d);
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

// this is the naive implementation
/*
inline float sphereDistanceNoNormalized(Ray r, __global const Sphere *s)
{
	Vector v = r.o - s->c;

	float a = dot(r.d, r.d);
	float b = dot(v, r.d);
	float c = dot(v, v) - (s->r * s->r);
	float d = b * b - a * c;
	if (d < 0.f) {
		return FLT_MAX;
	}

	d = sqrt(d);
	float t = a;
	float t0 = (-b + d) / t;
	float t1 = (-b - d) / t;

	float s0 = min(t0, t1);
	float s1 = max(t0, t1);

	// TODO: make this branchless
	if (s1 < 0.f)
		return FLT_MAX;
	else if (s0 < 0.f)
		return s1;
	else
		return s0;
}
*/
inline float intersectRay(const Ray r, __local const Sphere *spheres, const int numspheres, __local Sphere **s)
{
	float distance = FLT_MAX;
	for (int i = 0; i < numspheres; i++) {
		float d = sphereDistance(r, spheres + i);
		if (d < distance) {
			*s = (__local Sphere *)(spheres + i);
			distance = d;
		}
	}
	return distance;
}

inline Vector sampleLights(__local const Sphere *s, const Ray r, const Vector hit_point, const Vector normal, __local const Sphere *spheres, const int numspheres)
{
	Vector illu = vec_zero;
	
	for (int i = 0; i < numspheres; i++) {
		__local const Sphere *l = spheres + i;
		if (l->m.e != 0.f) {
			Ray s_ray = {hit_point + normal * EPSILON, normalize(l->c - hit_point)};

			__local Sphere *h;
			float d = intersectRay(s_ray, spheres, numspheres, &h);

			if (h == l) {
				Vector emission = l->m.c * l->m.e;
				float lambert = max(0.f, dot(s_ray.d, normal));
				float blinn = pow(dot(normalize(s_ray.d + r.d), normal), 64.f);
				float attenuation = sqrt(d);

				illu = illu + emission * (lambert + blinn) * 0.5f / attenuation;
			}
		}
	}
	return illu;
}

inline Vector sampleRay(const Ray ray, __local const Sphere *spheres, const int numspheres)
{
	int depth = 6;
	bool bounce = true;
	Vector sample = vec_zero;
	Ray r = ray;

	while (--depth && bounce) {	
		__local Sphere *s = 0;
		float distance = intersectRay(r, spheres, numspheres, &s);

		if (distance == FLT_MAX) {
			bounce = 0;
		} else {
			Vector ill = vec_zero;
			if (s->m.e != 0.f) {
				ill = s->m.e;
				bounce = false;
			} else {
				bool leaving = false;
				Vector hit_point = r.o + r.d * distance;
				Vector normal = normalize(hit_point - s->c);
				float cos_i = -1.f * dot(normal, normalize(r.d));
				if (cos_i < 0.f) {
					normal = normal * -1.f;
					cos_i = cos_i * -1.f;
					leaving = true;
				}

				if (s->m.s == Diffuse) {
					ill = sampleLights(s, r, hit_point, normal, spheres, numspheres) + ambient;
					bounce = false;
				}
				 else if (s->m.s == Specular) {
					r.o = hit_point + normal * EPSILON;
					r.d = normalize(r.d + 2.f * cos_i * normal);
				} 
				else if (s->m.s == Refractive) {
					const float air = 1.f;
					const float glass = 1.5f;

					float n1 = leaving? glass : air;
					float n2 = leaving? air : glass;
					float n = n1 / n2;

					float cos_t2 = 1.f - pow(n, 2) * (1.f - pow(cos_i, 2));

					if (cos_t2 < 0.f) {
						r.o = hit_point + normal * EPSILON;
						r.d = normalize(r.d + 2.f * cos_i * normal);
					} else {
						float cos_t = sqrt(cos_t2);
						
						// TODO: stochastic fresnel specular component?
						
						r.o = hit_point - normal * EPSILON;
						r.d = normalize(n * r.d - (n * cos_i + cos_t) * normal);
					}
				}
			}

			sample = sample + s->m.c * ill;
		}
	}

	return sample;
}

#define MAX_STACK 10
#define MAX_DEPTH 6

typedef struct {
	Ray ray;
	int depth;
	Vector atte;
} stack_entry_t;

typedef struct {
	stack_entry_t entries[MAX_STACK];
	int size;
} stack_t;

#define push_stack(_s, _r, _d, _a) { _s.entries[_s.size].ray = _r; _s.entries[_s.size].depth = _d; _s.entries[_s.size].atte = _a; _s.size++; }
#define pop_stack(_s, _r, _d, _a) { _s.size--; _r = _s.entries[_s.size].ray; _d = _s.entries[_s.size].depth; _a = _s.entries[_s.size].atte; }

static Vector sampleRayStack(Ray ray, __local const Sphere *spheres, const int numspheres)
{
	Vector pixel = vec_zero;

	stack_t stack;
	stack.size = 0;
	push_stack(stack, ray, MAX_DEPTH, vec_one);
	
	while (stack.size > 0) {
		Ray r;
		int depth;
		Vector atte;
		Vector sample = vec_zero;
		pop_stack(stack, r, depth, atte);

		if (depth == 0)
			return pixel;

		__local Sphere *s = 0;
		float distance = intersectRay(r, spheres, numspheres, &s);

		if (distance == FLT_MAX)
			return pixel;

		Vector ill = vec_zero;
		if (s->m.e != 0.f) {
			sample = s->m.c * s->m.e;
		} else {
			bool leaving = false;
			Vector hit_point = r.o + r.d * distance;
			Vector normal = normalize(hit_point - s->c);
			float cos_i = -1.f * dot(normal, normalize(r.d));
			if (cos_i < 0.f) {
				normal = normal * -1.f;
				cos_i = cos_i * -1.f;
				leaving = true;
			}

			if (s->m.s == Diffuse) {
				ill = sampleLights(s, r, hit_point, normal, spheres, numspheres) + ambient;
				sample = s->m.c * ill * atte;
			}
			 else if (s->m.s == Specular) {
				r.o = hit_point + normal * EPSILON;
				r.d = normalize(r.d + 2.f * cos_i * normal);
				push_stack(stack, r, depth - 1, s->m.c * atte);
			} 
			else if (s->m.s == Refractive) {
				const float air = 1.f;
				const float glass = 1.5f;
				float n1 = leaving? glass : air;
				float n2 = leaving? air : glass;
				float n = n1 / n2;
				float cos_t2 = 1.f - pow(n, 2) * (1.f - pow(cos_i, 2));

				if (cos_t2 < 0.f) {
					r.o = hit_point + normal * EPSILON;
					r.d = normalize(r.d - 2.f * cos_i * normal);
					push_stack(stack, r, depth - 1, s->m.c * atte);
				} 
				else {
					float cos_t = sqrt(cos_t2);
					
					// TODO: implement Schlick approx.
					float perp = pow((n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t), 2.f);
					float para = pow((n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t), 2.f);
					float fres = (perp + para) / 2.f;
					
					Ray rr;
					rr.o = hit_point + normal * EPSILON;
					rr.d = normalize(r.d + 2.f * cos_i * normal);
					push_stack(stack, rr, depth - 1, s->m.c * atte * fres * 10.f); // HACK: remove this 10.f plz
					
					Ray rf;
					rf.o = hit_point - normal * EPSILON;
					rf.d = normalize(n * r.d - (n * cos_i + cos_t) * normal);
					push_stack(stack, rf, depth - 1, s->m.c * (1.f - fres) * atte);
					
				}
			}
		}

		pixel = pixel + sample;
	}
	
	return pixel;
}

static Ray cameraRay( __global const Camera *camera, float x, float y, int width, int height)
{
	Vector d = normalize(camera->t - camera->o);
	
	float fov = PI / 180.f * 45.f;
	Vector vx = normalize(cross(d, vec_up)) * (width * fov / height);
	Vector vy = normalize(cross(vx, d)) * fov;
	
	float fx = (float)x / width - 0.5f;
	float fy = (float)y / height - 0.5f;
	
	Ray ray = {camera->o, normalize(d + vx * fx + vy * fy)};
	
	return ray;
}

kernel void raytracer(__global Sphere *spheres, int numspheres, __global const Camera *camera, write_only image2d_t fb, __local const Sphere *spheres_l)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(1);
	
	// copy global to local memory
	event_t event = async_work_group_copy((__local char *)spheres_l, (__global char *)spheres, sizeof(Sphere)*numspheres, 0);
	wait_group_events(1, &event);

	Vector pixel = vec_zero;
	int samples = 2;
	float samples2 = samples * samples;
	
	for (int j = 0; j < samples; j ++) {
		for (int i = 0; i < samples; i ++) {
			float dx = x + 1.f * i / samples2;
			float dy = y + 1.f * j / samples2;
			
			Ray ray = cameraRay(camera, dx, dy, width, height);
			pixel = pixel + sampleRayStack(ray, spheres_l, numspheres);
//			pixel = pixel + sampleRay(ray, spheres_l, numspheres);
		}
	}
	
	pixel = clamp(pixel / samples2, 0.f, 255.f);
	
	write_imagef(fb, (int2)(x, y), (float4)(pixel.x, pixel.y, pixel.z, 0.f));
}
