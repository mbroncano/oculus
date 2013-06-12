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
#include <iostream>

using std::ostream;
using std::endl;

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
    size_t primitiveIndex;
    BVHTreeNode *left, *right;
    BBox bbox;
    
    BVHTreeNode() : primitiveIndex(P_NONE), left(nullptr), right(nullptr) {}
    
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
                throw "[BVHTreeNode] Unknown primitive type";
        }
    }
    
    bool isLeaf() const { return primitiveIndex != P_NONE; }
    
};

typedef std::vector<BVHTreeNode *> node_vec_t;
typedef std::vector<BVHNode> bvh_vec_t;
    
struct BVHTree {
    std::vector<BVHTreeNode *> primitiveVec;
    BVHTreeNode *rootNode;
    bvh_vec_t bvh_vec;
    
    BVHTree(std::vector<Primitive>& primitives);
    BVHTreeNode *Build(node_vec_t &list, size_t start, size_t end) const;
    void DumpTree(BVHTreeNode *node, bvh_vec_t& list, int depth = 0) const;
};
    
#endif /* defined(__Oculus__bvhtree__) */
