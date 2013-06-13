//
//  bvhtree.cpp
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#include "bvhtree.h"
#include <algorithm>

#define DEBUG_BVH

using std::cout;
using std::endl;

// functor to sort by axis
struct sortNodeAxis {
    int i;
    
    sortNodeAxis(int i) : i(i) {}
    bool operator()(const BVHTreeNode* a, const BVHTreeNode* b) {
        return a->bbox.center().s[i] < b->bbox.center().s[i];
    }
};

struct partValueAxis {
    int axis;
    Vector value;
    
    partValueAxis(int axis, Vector value) : axis(axis), value(value) {};
    bool operator()(const BVHTreeNode* a) {
        Vector center = a->bbox.center();
        bool ret = center.s[axis] < value.s[axis];
        return ret;
    }
};


BVHTree::BVHTree(std::vector<Primitive>& primitives) {
    for (int i = 0; i < primitives.size(); i ++) {
        BVHTreeNode *node = new BVHTreeNode(primitives[i], i);
        //node->bbox += 1.f;
        primitiveVec.push_back(node);
    }
    
    rootNode = Build(primitiveVec, 0, primitiveVec.size());
//    rootNode = BuildAlt(primitiveVec, 0, primitiveVec.size() - 1);
    
    bvh_vec_t list;
    cout << "bvh tree:" << endl;
    DumpTree(rootNode, bvh_vec);

#ifdef DEBUG_BVH
    cout << endl << "serialized tree:" << endl;
    for (size_t i = 0; i<bvh_vec.size(); i ++) {
        cout << "[" << std::right << std::setw(2) << i << "] skip: " << std::left << std::setw(2) << bvh_vec[i].skip;
        if (bvh_vec[i].pid != P_NONE)
            cout << " leaf: " << bvh_vec[i].pid;
        cout << endl;
    }
#endif
}

BVHTreeNode *BVHTree::BuildAlt(node_vec_t &list, size_t start, size_t end, int axis) const {
    size_t d = end - start;
    
    if (start > end)
        throw "assert";
    
    if (d == 0) {
        return list[start];
    }
    
    // calculate the union bounding box
    BVHTreeNode *node = new BVHTreeNode();
    for (size_t i = start; i <= end; i ++) {
        node->bbox += list[i]->bbox;
    }

    // sort the nodes over alternate axis by the center
    std::sort(list.begin() + start, list.begin() + end, sortNodeAxis(axis));
    axis = (axis + 1) % 3;
    
    // recurse on the left and right sides
    size_t split = start + d/2;
    node->left = BuildAlt(list, start, split, axis);
    if ((split+1) <= end)
        node->right = BuildAlt(list, split+1, end, axis);
    
    return node;
}

BVHTreeNode *BVHTree::Build(node_vec_t &list, size_t start, size_t end) const {
    size_t d = end - start;
    
    if (d < 1)
        throw "assert";
    
    if (d == 1) {
        return list[start];
    }
    
    // calculate the union bounding box and the mean center
    Vector mean = vec_zero;
    BVHTreeNode *node = new BVHTreeNode();
    for (size_t i = start; i < end; i ++) {
        node->bbox += list[i]->bbox;
        mean = mean + list[i]->bbox.center();
    }
    mean = mean / d;
    
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

    
    // sort the nodes over axis by the center
//    std::sort(list.begin() + start, list.begin() + end, sortNodeAxis(axis));
//    node_vec_t::iterator middle = std::partition(list.begin() + start, list.begin() + end, partValueAxis(axis, mean));
    
    
    // partitions the list over the predicate
    node_vec_t::iterator middle = std::partition(list.begin() + start, list.begin() + end, partValueAxis(axis, mean));
    size_t split = start + std::distance(list.begin() + start, middle);
    
    if (split == end) {
        split--;
    }
    if (split == start) {
        split ++;
    }
    
    // recurse on the left and right sides
    node->left = Build(list, start, split);
    if (split != end)
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

#ifdef DEBUG_BVH
    cout << "[" << std::setw(2) << ofs << "]";
    for(int i = depth; i; i--) cout << " |";
#endif
    
    if (node->isLeaf()) {
#ifdef DEBUG_BVH
        cout << " leaf: " << node->primitiveIndex << endl;
#endif
    } else {
#ifdef DEBUG_BVH
        cout << " node" << endl;
#endif
        DumpTree(node->left, list, depth + 1);
        DumpTree(node->right, list, depth + 1);
    }
    
    list[ofs].skip = (unsigned int)list.size();
}

