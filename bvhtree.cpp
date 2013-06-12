//
//  bvhtree.cpp
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#include "bvhtree.h"

// functor to sort by axis
struct sortNodeAxis {
    int i;
    
    sortNodeAxis(int i) : i(i) {}
    bool operator()(const BVHTreeNode* a, const BVHTreeNode* b) {
        return (a->bbox.min.s[i] + a->bbox.max.s[i]) < (b->bbox.min.s[i] + b->bbox.max.s[i]);
    }
};


BVHTree::BVHTree(std::vector<Primitive>& primitives) {
    for (int i = 0; i< primitives.size(); i ++) {
        BVHTreeNode *node = new BVHTreeNode(primitives[i], i);
        primitiveVec.push_back(node);
    }
    
    rootNode = Build(primitiveVec, primitiveVec.begin(), primitiveVec.end());
    
    BVH r;
    bvh_vec.push_back(r);
}

BVHTreeNode *BVHTree::Build(node_vec_t &list, node_vec_t::iterator start, node_vec_t::iterator end, int axis) const {
    long d = std::distance(start, end);
    
    if (d < 1) {
        return *start;
    }
    
    BVHTreeNode *node = new BVHTreeNode();
    for(node_vec_t::iterator it = start; it != end; ++it) {
        node->bbox += (*it)->bbox;
    }
    //Vector center = node->bbox.center();
    
    std::sort(start, end, sortNodeAxis(axis % 3));
    axis++;
    return NULL;
}

