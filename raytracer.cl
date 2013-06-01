#define __IS_KERNEL

#include "geometry.h"

static float sphereDistance(Ray r, __global const Sphere *s)
{
	Vector v = r.o - s->c;

	float a = dot(r.d, r.d);
	float b = 2 * dot(v, r.d);
	float c = dot(v, v) - (s->r * s->r);
	float d = b * b - 4 * a * c;
	if (d < 0.f) {
		return FLT_MAX;
	}

	d = sqrt(d);
	float s0 = -b + d;
	float s1 = -b - d;
	float t = 2 * a;

	// TODO: optimize for SIMT
	if (s0 < 0) {
		return s1 / t;
	} else if (s1 < 0) {
		return s0 / t;
	} else if (s0 > s1) {
		return s1 / t;
	} else {
		return s0 / t;
	}
}

static Vector sampleRay(Ray r, __global const Sphere *spheres, int numspheres)
{
	int depth = 6;
	bool bounce = 1;
	Vector sample = vec_zero;
	
	while (depth-- && bounce) {	
		float distance = INFINITY;	
		__global const Sphere *s = 0;
		for (int i = 0; i < numspheres; i++) {
			float d = sphereDistance(r, spheres + i);
			if (d < distance) {
				s = spheres + i;
				distance = d;
			}
		}

		if (distance != INFINITY) {
			Vector ill = vec_zero;
			if (s->m.e != 0.f) {
				ill = s->m.e;
				bounce = 0;
			} else {
				Vector hit_point = r.o + r.d * (distance + FLT_EPSILON);
				Vector normal = normalize(hit_point - s->c);
				float cos_i = dot(normal, r.d);
				if (cos_i > 0.f) {
					normal = normal * -1.f;
				}

				if (s->m.s == Diffuse) {
					for (int i = 0; i < numspheres; i++) {
						__global const Sphere *l = spheres + i;
						if (l->m.e == 0.f)
							continue;
				
						Ray s_ray;
						s_ray.o = hit_point;
						s_ray.d = normalize(l->c - hit_point);

						float dist_shadow = FLT_MAX;
						for (int j = 0; j < numspheres; j++) {
							float d = sphereDistance(r, spheres + j);
							if (d < dist_shadow) {
								dist_shadow = d;
							}
						}
						if (dist_shadow == FLT_MAX)
							continue;	

						float lambert = max(0.f, dot(s_ray.d, normal));
						float attenuation = length(s_ray.d);
			
						ill = ill + (l->m.c * l->m.e) * lambert / attenuation;
					}
					bounce = 0;
				} else if (s->m.s == Specular) {
					r.o = hit_point;
					r.d = r.d + 2.f * cos_i * normal;
				}
			}
		
			sample = sample + s->m.c * ill;
		}
	}
	
	return sample;
}

static Ray cameraRay( __global const Camera *camera, int x, int y, int width, int height)
{
	Vector d = normalize(camera->t - camera->o);
	
	float fov = M_PI_2 / 180.f * 45.f;
	Vector vx = normalize(cross(d, vec_up)) * (width * fov / height);
	Vector vy = normalize(cross(vx, d)) * fov;
	
	float fx = 2.f * (float)x / width - 1.f;
	float fy = 2.f * (float)y / height - 1.f;
	
	Ray ray;
	ray.o = camera->o;
	ray.d = d + vx * fx + vy * fy;
	
	return ray;
}

__kernel void raytracer(__global const Sphere *spheres, int numspheres, __global const Camera *camera, __global Pixel *fb)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(0);

	Ray ray = cameraRay(camera, x, y, width, height);
	
	Vector sample = sampleRay(ray, spheres, numspheres);
	
	fb[(y * width + x)].r = convert_uchar_sat(sample.x * 256);
	fb[(y * width + x)].g = convert_uchar_sat(sample.y * 256);
	fb[(y * width + x)].b = convert_uchar_sat(sample.z * 256);
}