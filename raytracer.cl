#define __IS_KERNEL

#include "geometry.h"

static float sphereDistance(Ray r, __global const Sphere *s)
{
	Vector v = r.o - s->c;

	float a = dot(r.d, r.d);
	float b = 2.f * dot(v, r.d);
	float c = dot(v, v) - (s->r * s->r);
	float d = b * b - 4.f * a * c;
	if (d < 0.f) {
		return INFINITY;
	}

	d = sqrt(d);
	float t = 2.f * a;
	float t0 = (-b + d) / t;
	float t1 = (-b - d) / t;

	float s0 = min(t0, t1);
	float s1 = max(t0, t1);

	// TODO: make this branchless
	if (s1 < 0.f)
		return INFINITY;
	else if (s0 < 0.f)
		return s1;
	else
		return s0;
}

static float intersectRay(const Ray r, __global const Sphere *spheres, const int numspheres, __global const Sphere **s)
{
	float distance = INFINITY;
	for (int i = 0; i < numspheres; i++) {
		float d = sphereDistance(r, spheres + i);
		if (d < distance) {
			*s = spheres + i;
			distance = d;
		}
	}
	return distance;
}

static Vector sampleLights(__global const Sphere *s, const Ray r, const Vector hit_point, const Vector normal, __global const Sphere *spheres, const int numspheres)
{
	Vector illu = vec_zero;
	
	for (int i = 0; i < numspheres; i++) {
		__global const Sphere *l = spheres + i;
		if (l->m.e != 0.f) {
			Ray s_ray = {hit_point + normal * EPSILON, l->c - hit_point};

			__global const Sphere *h;
			float dist_shadow = intersectRay(s_ray, spheres, numspheres, &h);

			if (h == l) {
				Vector emission = l->m.c * l->m.e;
				float lambert = max(0.f, dot(normalize(s_ray.d), normal));
				float blinn = pow(dot(normalize(s_ray.d + r.d), normal), 64.f);
				float attenuation = sqrt(length(s_ray.d));

				illu = illu + emission * (lambert + blinn) * 0.5f / attenuation;
			} else {
			//	illu = vec_one * dist_shadow; // fake soft shadows!
			}
		}
	}
	return illu;
}

static Vector sampleRay(const Ray ray, __global const Sphere *spheres, const int numspheres)
{
	int depth = 6;
	bool bounce = true;
	Vector sample = vec_zero;
	Ray r = ray;
	
	while (--depth && bounce) {	
		/*if (--depth == 0) {
			return (Vector)(.75f, 0.f, .75f);
		}*/
		
		__global Sphere *s = 0;
		float distance = intersectRay(r, spheres, numspheres, &s);

		if (distance == INFINITY) {
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
					Vector ambient = (Vector)(.2f, .2f, .2f);
					ill = sampleLights(s, r, hit_point, normal, spheres, numspheres) + ambient;
					bounce = false;
				} else if (s->m.s == Specular) {
					r.o = hit_point + normal * EPSILON;
					r.d = r.d - 2.f * cos_i * normal;
				} else if (s->m.s == Refractive) {
					float n1 = 1.f;
					float n2 = 1.5f;
					float n = leaving ? (n2 / n1) : (n1 / n2);
					float cos_t2 = 1.f - pow(n, 2) * (1.f - pow(cos_i, 2));
					
					if (cos_t2 < 0.f) {
						r.o = hit_point + normal * EPSILON;
						r.d = r.d + 2.f * cos_i * normal;
					} else {
						r.o = hit_point - normal * EPSILON;
						r.d = n * r.d + (n * cos_i - sqrt(cos_t2)) * normal;
					}
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
	
	float fov = PI / 180.f * 45.f;
	Vector vx = normalize(cross(d, vec_up)) * (width * fov / height);
	Vector vy = normalize(cross(vx, d)) * fov;
	
	float fx = (float)x / width - 0.5f;
	float fy = (float)y / height - 0.5f;
	
	Ray ray = {camera->o, d + vx * fx + vy * fy};
	
	return ray;
}

__kernel void raytracer(__global const Sphere *spheres, int numspheres, __global const Camera *camera, __global Pixel *fb)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int width = get_global_size(0);
	const int height = get_global_size(1);

	Ray ray = cameraRay(camera, x, y, width, height);

	Vector sample = sampleRay(ray, spheres, numspheres);
	
	fb[(y * width + x)].r = convert_uchar_sat(sample.x * 256);
	fb[(y * width + x)].g = convert_uchar_sat(sample.y * 256);
	fb[(y * width + x)].b = convert_uchar_sat(sample.z * 256);
}
