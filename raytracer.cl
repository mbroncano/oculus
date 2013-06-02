#define __IS_KERNEL

#include "geometry.h"

// optimized; assumes ray direction is normalized (so the 'a' term can be 1.f)
inline float sphereDistance(Ray r, __global const Sphere *s)
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
inline float intersectRay(const Ray r, __global const Sphere *spheres, const int numspheres, __global Sphere **s)
{
	float distance = FLT_MAX;
	for (int i = 0; i < numspheres; i++) {
		float d = sphereDistance(r, spheres + i);
		if (d < distance) {
			*s = (__global Sphere *)(spheres + i);
			distance = d;
		}
	}
	return distance;
}

inline Vector sampleLights(__global const Sphere *s, const Ray r, const Vector hit_point, const Vector normal, __global const Sphere *spheres, const int numspheres)
{
	Vector illu = vec_zero;
	
	for (int i = 0; i < numspheres; i++) {
		__global const Sphere *l = spheres + i;
		if (l->m.e != 0.f) {
			Ray s_ray = {hit_point + normal * EPSILON, normalize(l->c - hit_point)};

			__global Sphere *h;
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

inline Vector sampleRay(const Ray ray, __global const Sphere *spheres, const int numspheres)
{
	int depth = 6;
	bool bounce = true;
	Vector sample = vec_zero;
	Ray r = ray;
	
	while (--depth && bounce) {	
		__global Sphere *s = 0;
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
				float cos_i = dot(normal, normalize(r.d));
				if (cos_i > 0.f) {
					normal = normal * -1.f;
					cos_i = cos_i * -1.f;
					leaving = true;
				}

				if (s->m.s == Diffuse) {
					ill = sampleLights(s, r, hit_point, normal, spheres, numspheres) + ambient;
					bounce = false;
				} else if (s->m.s == Specular) {
					r.o = hit_point + normal * EPSILON;
					r.d = normalize(r.d - 2.f * cos_i * normal);
				} else if (s->m.s == Refractive) {
					float n1 = 1.f;
					float n2 = 1.5f;
					float n = leaving ? (n2 / n1) : (n1 / n2);
					float cos_t2 = 1.f - pow(n, 2) * (1.f - pow(cos_i, 2));
					
					if (cos_t2 < 0.f) {
						r.o = hit_point + normal * EPSILON;
						r.d = normalize(r.d - 2.f * cos_i * normal);
					} else {
						r.o = hit_point - normal * EPSILON;
						r.d = normalize(n * r.d + (n * cos_i - sqrt(cos_t2)) * normal);
					}
				}
			}
		
			sample = sample + s->m.c * ill;
		}
	}
	
	return sample;
}

inline Ray cameraRay( __global const Camera *camera, float x, float y, int width, int height)
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

__kernel void raytracer(__global const Sphere *spheres, int numspheres, __global const Camera *camera, __global Pixel *fb)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(1);

	Vector pixel = vec_zero;
	int samples = 2;
	float samples2 = samples * samples;
	
	for (int j = 0; j < samples; j ++) {
		for (int i = 0; i < samples; i ++) {
			float dx = x + 1.f * i / samples2;
			float dy = y + 1.f * j / samples2;
			
			Ray ray = cameraRay(camera, dx, dy, width, height);
			pixel = pixel + sampleRay(ray, spheres, numspheres);
		}
	}
	
	pixel = pixel / samples2;
	
	fb[(y * width + x)].r = convert_uchar_sat(pixel.x * 256);
	fb[(y * width + x)].g = convert_uchar_sat(pixel.y * 256);
	fb[(y * width + x)].b = convert_uchar_sat(pixel.z * 256);
}
