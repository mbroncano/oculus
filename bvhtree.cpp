//
//  bvhtree.cpp
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#include "bvhtree.h"

#define DEBUG_BVH

using std::cout;
using std::endl;

// functor to sort by axis
struct sortNodeAxis {
    int i;
    
    sortNodeAxis(int i) : i(i) {}
    bool operator()(const BVHTreeNode* a, const BVHTreeNode* b) {
        return (a->bbox.min.s[i] + a->bbox.max.s[i]) < (b->bbox.min.s[i] + b->bbox.max.s[i]);
    }
};

struct partValueAxis {
    int axis;
    float value;
    
    partValueAxis(int axis, float value) : axis(axis), value(value) {};
    bool operator()(const BVHTreeNode* a) {
        Vector ac = a->bbox.center();
        return (ac.s[axis] < value);
    }
};


BVHTree::BVHTree(std::vector<Primitive>& primitives) {
    for (int i = 0; i < primitives.size(); i ++) {
        BVHTreeNode *node = new BVHTreeNode(primitives[i], i);
        primitiveVec.push_back(node);
    }
    
    rootNode = Build(primitiveVec, 0, primitiveVec.size()-1);
    
    bvh_vec_t list;
    DumpTree(rootNode, bvh_vec);
    
    for (size_t i = 0; i<bvh_vec.size(); i ++) {
        //cout << "[" << i << "] p:" << bvh_vec[i].pid << " s: " << bvh_vec[i].skip << endl;
    }
}

BVHTreeNode *BVHTree::Build(node_vec_t &list, size_t start, size_t end) const {
    size_t d = end - start;
    
    if (d < 1) {
        return list[start];
    }
    
    // calculate the union bounding box and the mean center
    Vector mean = vec_zero;
    BVHTreeNode *node = new BVHTreeNode();
    for (size_t i = start; i < end; i ++) {
        node->bbox += list[i]->bbox;
        mean = mean + list[i]->bbox.center();
    }
    mean = mean / (float)d;
    
    // calculate the axis with more variance (where the bbs are more spread)
    Vector variance = vec_zero;
    for (size_t i = start; i < end; i ++) {
        Vector sep = list[i]->bbox.center() - mean;
        variance = variance + sep * sep; // note this is *not* the dot product
    }
    
    // chooses the axis with the biggest variance
    int axis = 0;
    if (variance.y > variance.x && variance.y > variance.z) {
        axis = 1;
    } else if (variance.z > variance.x) {
        axis = 2;
    }
    
    // creates a predicate that returns true if the element is less than the value on an axis
    partValueAxis predicate(axis, mean.s[axis]);
    
    // partitions the list over the predicate
    node_vec_t::iterator middle = std::partition(list.begin() + start, list.begin() + end, predicate);
    long split = std::distance(list.begin(), middle);
    
    // recurse on the left and right sides
    node->left = Build(list, start, split++);
    if (split <= end)
        node->right = Build(list, split, end);

    return node;
}

void BVHTree::DumpTree(BVHTreeNode *node, bvh_vec_t& list, int depth) const {
    
    if (node == nullptr)
        return;
    
    BVHNode n;
    n.pid = (unsigned int)node->primitiveIndex;
    n.min = node->bbox.min;
    n.max = node->bbox.max;
    size_t ofs = list.size();
    list.push_back(n);

    //cout << "[" << ofs << "]";
    
    //for(int i = depth; i; i--) cout << " |";

    if (node->isLeaf()) {
        //cout << " leaf: " << node->primitiveIndex << endl;
    } else {
        //cout << " node" << endl;
        DumpTree(node->left, list, depth + 1);
        DumpTree(node->right, list, depth + 1);
    }
    
    list[ofs].skip = (unsigned int)list.size();
}

