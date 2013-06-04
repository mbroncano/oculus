#define __IS_KERNEL

#include "geometry.h"

__constant const Vector vec_zero = (Vector)(0.f, 0.f, 0.f);
__constant const Vector vec_y =    (Vector)(0.f, 1.f, 0.f);
__constant const Vector vec_x =    (Vector)(1.f, 0.f, 0.f);
__constant const Vector vec_one =  (Vector)(1.f, 1.f, 1.f);
__constant const Vector ambient =  (Vector)(.1f, .1f, .1f);

#define EPSILON 1e-2f
#define PI M_PI_F

static float randomf(random_state_t *state)
{
	union {
		float f;
		unsigned int ui;
	} ret;

	const uint a = 22222;
	const uint b = 55555;
	*state = (random_state_t)(a * ((state->x) & 0xFFFF) + ((state->x) >> 16), b * ((state->y) & 0xFFFF) + ((state->y) >> 16));
	
	ret.ui = (((state->x) << 16) + (state->y) & 0x007fffff) | 0x40000000;
	
	return (ret.f - 2.f) / 2.f;
}

// optimized; assumes ray direction is normalized (so the 'a' term can be 1.f)
static float sphereDistance(Ray r, __local const Sphere *s)
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

static Vector unitSphereSample(const float u, const float v)
{
	const float z = 1.f - 2.f * u;				// z in [-1, 1] -> z = cos w
	const float r = sqrt(max(0.f, 1 - z * z));	// sqrt(1 - x^2) -> sin w
	const float p = 2 * PI * v;					// p in [0, 2pi]
	const float x = r * cos(p);
	const float y = r * sin(p);
	
	return (Vector)(x, y, x);
}

static Vector hemiSphereSample(const float s0, const float s1, const Vector normal)
{
	float r1 = 2 * PI * s0;
	float r2 = s1;
	float r2s = sqrt(r2);
	
	Vector w = normal;
	Vector a;
	if (fabs(w.x) > .1f)
		a = vec_y;
	else
		a = vec_x;
	Vector u = normalize(cross(a, w));
	Vector v = cross(w, u);
	
	Vector dir = u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1 - r2);
	
	return dir;
}

static float intersectRay(const Ray r, __local const Sphere *spheres, const int numspheres, __local Sphere **s)
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

static Vector sampleLights(
	__local const Sphere *s,
	const Ray r,
	const Vector hit_point,
	const Vector normal,
	__local const Sphere *spheres,
	const int numspheres,
	random_state_t *rnd)
{
	Vector illu = vec_zero;
	
	for (int i = 0; i < numspheres; i++) {
		__local const Sphere *l = spheres + i;
		if (l->m.e != 0.f) {
			Vector light_hit = l->c + l->r * unitSphereSample(randomf(rnd), randomf(rnd));
			
			Ray s_ray = {hit_point + normal * EPSILON, normalize(light_hit - hit_point)};

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

inline Vector sampleRay(const Ray ray, __local const Sphere *spheres, const int numspheres, random_state_t *rnd)
{
	int depth = 6;
	bool bounce = true;
	Vector sample = vec_zero;
	Vector illum = vec_one;
	Ray r = ray;

	while (--depth) {
		__local Sphere *s = 0;
		float distance = intersectRay(r, spheres, numspheres, &s);

		if (distance == FLT_MAX) {
			return sample;
		}

		if (s->m.e != 0.f) {
			if (bounce) { 
				sample = sample + illum * s->m.e;
			}
			return sample;
		} 
		
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
			bounce = false;
			illum = illum * s->m.c;
			
			Vector light = sampleLights(s, r, hit_point, normal, spheres, numspheres, rnd);
			sample = sample + illum * (light + ambient);
			
			r.o = hit_point + normal * EPSILON;
			r.d = hemiSphereSample(randomf(rnd), randomf(rnd), normal);
		}
		 else if (s->m.s == Specular) {
			bounce = true;
			illum = illum * s->m.c;

			// shadows over mirrors?
			//Vector light = sampleLights(s, r, hit_point, normal, spheres, numspheres, rnd);
			//sample = sample + illum * (light + ambient) * normalize(vec_one - s->m.c);
			
			r.o = hit_point + normal * EPSILON;
			r.d = normalize(r.d + 2.f * cos_i * normal);
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
				r.o = hit_point + normal * EPSILON;
				r.d = normalize(r.d + 2.f * cos_i * normal);
			} else {
				float cos_t = sqrt(cos_t2);

				// TODO: implement Schlick approx.
				float perp = pow((n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t), 2.f);
				float para = pow((n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t), 2.f);
				float fres = (perp + para) / 2.f;

				if (randomf(rnd) < fres) {
					r.o = hit_point + normal * EPSILON;
					r.d = normalize(r.d + 2.f * cos_i * normal);
				} else {
					r.o = hit_point - normal * EPSILON;
					r.d = normalize(n * r.d - (n * cos_i + cos_t) * normal);
				}
			}
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

// todo: boundaries check
#define push_stack(_s, _r, _d, _a) { _s.entries[_s.size].ray = _r; _s.entries[_s.size].depth = _d; _s.entries[_s.size].atte = _a; _s.size++; }
#define pop_stack(_s, _r, _d, _a) { _s.size--; _r = _s.entries[_s.size].ray; _d = _s.entries[_s.size].depth; _a = _s.entries[_s.size].atte; }

static Vector sampleRayStack(Ray ray, __local const Sphere *spheres, const int numspheres, random_state_t *rnd)
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
				ill = sampleLights(s, r, hit_point, normal, spheres, numspheres, rnd) + ambient;
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
	Vector vx = normalize(cross(d, vec_y)) * (width * fov / height);
	Vector vy = normalize(cross(vx, d)) * fov;
	
	float fx = (float)x / width - 0.5f;
	float fy = (float)y / height - 0.5f;
	float zoom = 1.f;
	
	Ray ray = {camera->o, normalize(d + zoom * vx * fx + zoom * vy * fy)};
	
	return ray;
}

kernel void raytracer(
	__global Sphere *spheres, 
	int numspheres,
	__global const Camera *camera,
	__local const Sphere *spheres_l, 
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
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(1);

	// xor seed per work items
	seed ^= (random_state_t)(x, y);
	//printf("%d, %d\n", seed.x, seed.y);
	// copy global to local memory
	event_t event = async_work_group_copy((__local char *)spheres_l, (__global char *)spheres, sizeof(Sphere)*numspheres, 0);
	wait_group_events(1, &event);

	Vector pixel = vec_zero;
	int sdir = 1;

	float sdir2 = sdir * sdir;
	
	for (int j = 0; j < sdir; j ++) {
		for (int i = 0; i < sdir; i ++) {
			float dx = x + randomf(&seed) - 0.5f;
			float dy = y + randomf(&seed) - 0.5f;

			//float dx = x + 1.f * i / sdir;
			//float dy = y + 1.f * j / sdir;
			
			Ray ray = cameraRay(camera, dx, dy, width, height);
//			pixel = pixel + sampleRayStack(ray, spheres_l, numspheres, &seed);
			pixel = pixel + sampleRay(ray, spheres_l, numspheres, &seed);
		}
	}
	// average number of samples
	uint index = y * width + x;
	pixel = pixel / sdir2;
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
