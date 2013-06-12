//
//  scene.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef __Oculus__scene__
#define __Oculus__scene__

#include "geometry.h"
#include "bvhtree.h"
#include "parson.h"
#include <map>
#include <vector>
#include <string>
#include <iostream>

struct Scene {
	Camera camera;
	std::map<std::string, Material> material_map;
	std::vector<Primitive> primitive_vector;
	BVHTree *bvhTree;
    
    void buildBVH();
    void testScene();
    Vector getVector(JSON_Array *vector_array);
    void loadJson(const char *f);
};

#endif /* defined(__Oculus__scene__) */
