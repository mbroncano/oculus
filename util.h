//
//  util.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef Oculus_util_h
#define Oculus_util_h

#include <sys/time.h>
#include <algorithm>
#include <iomanip>

static double wallclock() {
	struct timeval t;
	gettimeofday(&t, NULL);
    
	return t.tv_sec + t.tv_usec / 1000000.0;
}

static Vector operator+(const Vector &a, const Vector &b) {
	return (Vector){{a.x + b.x, a.y + b.y, a.z + b.z}};
}

static Vector operator+(const Vector &a, const float &b) {
	return (Vector){{a.x + b, a.y + b, a.z + b}};
}

static Vector operator-(const Vector &a, const Vector &b) {
	return (Vector){{a.x - b.x, a.y - b.y, a.z - b.z}};
}

static Vector operator-(const Vector &a, const float &b) {
	return (Vector){{a.x - b, a.y - b, a.z - b}};
}

static Vector operator*(const Vector &a, const Vector &b) {
	return (Vector){{a.x * b.x, a.y * b.x, a.z * b.x}};
}

static Vector operator*(const Vector &a, const float &b) {
	return (Vector){{a.x * b, a.y * b, a.z * b}};
}

static Vector operator/(const Vector &a, const float &b) {
	return (Vector){{a.x / b, a.y / b, a.z / b}};
}

static Vector fmin(const Vector &a, const Vector &b)
{
	Vector r;
	r.x = std::min(a.x, b.x);
	r.y = std::min(a.y, b.y);
	r.z = std::min(a.z, b.z);
	return r;
}

static Vector fmax(const Vector &a, const Vector &b)
{
	Vector r;
	r.x = std::max(a.x, b.x);
	r.y = std::max(a.y, b.y);
	r.z = std::max(a.z, b.z);
	return r;
}

static Vector fmin(const Vector &a, const Vector &b, const Vector &c)
{
	Vector r;
	r.x = std::min(std::min(a.x, b.x), std::min(b.x, c.x));
	r.y = std::min(std::min(a.y, b.y), std::min(b.y, c.y));
	r.z = std::min(std::min(a.z, b.z), std::min(b.z, c.z));
	return r;
}

static Vector fmax(const Vector &a, const Vector &b, const Vector &c)
{
	Vector r;
	r.x = std::max(std::max(a.x, b.x), std::max(b.x, c.x));
	r.y = std::max(std::max(a.y, b.y), std::max(b.y, c.y));
	r.z = std::max(std::max(a.z, b.z), std::max(b.z, c.z));
	return r;
}

static std::ostream& operator<<(std::ostream& os, const Vector& a)
{
	os << std::setiosflags(std::ios::fixed) << std::setprecision(1);
	os << "(" << a.x << ", " << a.y << ", " << a.z << ")";
	return os;
}

static std::ostream& operator<<(std::ostream& os, const Primitive& a)
{
    switch(a.t) {
        case sphere:
            os << "[s] c" << a.sphere.c << " r:" << a.sphere.r;
            break;
        case triangle:
            os << "[t] p0" << a.triangle.p[0] << " p1" << a.triangle.p[1] << " p2" << a.triangle.p[2];
            break;
        default:
            throw "Unknown primitive type";
    }

    return os;
}
    

#endif
