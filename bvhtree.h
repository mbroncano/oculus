//
//  bvhtree.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef __Oculus__bvhtree__
#define __Oculus__bvhtree__

#include "geometry.h"
#include "util.h"
#include <vector>

struct BBox {
    Vector min, max;
    
    BBox() : min(vec_zero), max(vec_zero) {};
    
    BBox& operator+=(const BBox &b) {
        min = fmin(min, b.max);
        max = fmax(max, b.max);
        return *this;
    }
    
    Vector center() const {
        Vector ret;
        ret.x = (min.x + max.x) / 2.f;
        ret.y = (min.y + max.y) / 2.f;
        ret.z = (min.z + max.z) / 2.f;
        return ret;
    }
};
    
struct BVHTreeNode {
    int primitiveIndex;
    BVHTreeNode *left, *right;
    BBox bbox;
    
    BVHTreeNode() : primitiveIndex(0), left(0), right(0) {}
    
    BVHTreeNode(Primitive p, int i) : primitiveIndex(i), left(0), right(0) {
        switch(p.t) {
            case sphere:
                bbox.min = p.sphere.c - p.sphere.r;
                bbox.max = p.sphere.c + p.sphere.r;
                break;
            case triangle:
                bbox.min = fmin(p.triangle.p[0], p.triangle.p[1], p.triangle.p[2]);
                bbox.max = fmax(p.triangle.p[0], p.triangle.p[1], p.triangle.p[2]);
                break;
            default:
                throw "[BVHTreeNode] Unknown primtive type";
        }
    }
};

typedef std::vector<BVHTreeNode *> node_vec_t;
    
struct BVHTree {
    std::vector<BVHTreeNode *> primitiveVec;
    BVHTreeNode *rootNode;
    std::vector<BVH> bvh_vec;
    
    BVHTree(std::vector<Primitive>& primitives);
    BVHTreeNode *Build(node_vec_t &list, node_vec_t::iterator start, node_vec_t::iterator end, int axis = 0) const;
};
    
#endif /* defined(__Oculus__bvhtree__) */
