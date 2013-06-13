//
//  scene.cpp
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#include "scene.h"

void Scene::buildBVH() {
    bvhTree = new BVHTree(primitive_vector);
    
}

Vector Scene::getVector(JSON_Array *vector_array) {
    if (json_array_get_count(vector_array) != 3) {
        throw "reading vector ";
    }
    Vector v;
    for (int i = 0; i < 3; i ++)
        v.s[i] = json_array_get_number(vector_array, i);
    
    return v;
}

void Scene::testScene() {
    int n = 10;
    int r = 100/n/2 - 1;
    int ofs = 100/n;
    int cofs = ofs/2;
    for (int i = 0; i < n; i ++)
        for (int j = 0; j < n; j ++)
            for (int k = 0; k < n; k ++) {
                Primitive s;
                s.sphere.c = (Vector){{static_cast<cl_float>(cofs + i * ofs), static_cast<cl_float>(cofs + j * ofs), static_cast<cl_float>(cofs + k * ofs)}};
                s.sphere.r = r;
                s.t = sphere;
                s.m.c = (Vector){{0.9f * i / n, 0.9f * j / n, 0.9f * k / n}};
                s.m.s = Diffuse;
                s.m.e = 0.f;
                primitive_vector.push_back(s);
            }
    
    Primitive t;
    t.triangle.p[0] = (Vector){{40.f, 180.f, 40.f}};
    t.triangle.p[1] = (Vector){{40.f, 180.f, 60.f}};
    t.triangle.p[2] = (Vector){{60.f, 180.f, 40.f}};
    t.t = triangle;
    t.m.c = (Vector){{0.9f, 0.9f, 0.9f}};
    t.m.s = Diffuse;
    t.m.e = 12.f;
    primitive_vector.push_back(t);
    
    camera.o = (Vector){{100.f, 200.f, 200.f}};
    camera.t = (Vector){{50.f, 50.f, 50.f}};
    
}

void Scene::loadJson(const char *f) {
    try {
        JSON_Value *_root = json_parse_file(f);
        if (!json_value_get_type(_root)) {
            throw "error parsing file";
        }
        
        if (json_value_get_type(_root) != JSONObject) {
            throw "missing root";
        }
        
        JSON_Object *_scene = json_object_get_object(json_value_get_object(_root), "scene");
        if(!_scene) {
            throw "missing scene";
        }
        
        camera.o = getVector(json_object_dotget_array(_scene, "camera.origin"));
        camera.t = getVector(json_object_dotget_array(_scene, "camera.target"));
        
        JSON_Array *_materials = json_object_get_array(_scene, "materials");
        if (!_materials) {
            throw "missing materials";
        }
        
        for(int i = 0; i < json_array_get_count(_materials); i ++) {
            JSON_Object *_material = json_array_get_object(_materials, i);
            std::string name = json_object_get_string(_material, "name");
            
            Material material;
            
            std::string type = json_object_get_string(_material, "type");
            if (type == "diffuse") {
                material.s = Diffuse;
            } else if (type == "specular") {
                material.s = Specular;
            } else if (type == "dielectric") {
                material.s = Dielectric;
            } else if (type == "metal") {
                material.s = Metal;
            } else {
                throw "uknown material type";
            }
            
            material.c = getVector(json_object_get_array(_material, "color"));
            material.e = json_object_get_number(_material, "emission");
            
            material_map[name] = material;
        }
        
        JSON_Array *_primitives = json_object_get_array(_scene, "primitives");
        if (!_primitives) {
            throw "missing primitives";
        }
        
        for(int i = 0; i < json_array_get_count(_primitives); i ++) {
            JSON_Object *_primitive = json_array_get_object(_primitives, i);
            std::string name = json_object_get_string(_primitive, "name");
            
            Primitive primitive;
            
            std::string type = json_object_get_string(_primitive, "type");
            if (type == "sphere") {
                primitive.t = sphere;
                primitive.sphere.c = getVector(json_object_get_array(_primitive, "center"));
                primitive.sphere.r =json_object_get_number(_primitive, "radius");
            } else if (type == "triangle") {
                primitive.t = triangle;
                JSON_Array *_points = json_object_get_array(_primitive, "points");
                for(int j = 0; j < json_array_get_count(_points); j ++) {
                    //JSON_Object *_point = json_array_get_object(_points, j);
                    primitive.triangle.p[j] = getVector(json_array_get_array(_points, j));
                }
                
            } else {
                throw "unknown primitive type";
            };
            
            std::string mat_string = json_object_get_string(_primitive, "material");
            Material material = material_map[mat_string];
            /*if (material) {
             std::cout << mat_string;
             throw "material for primitive not found";
             }*/
            primitive.m = material;
            
            primitive_vector.push_back(primitive);
        }
        
        json_value_free(_root);
    } catch (const char *e) {
        std::cout << "[Scene] Exception: " << e << std::endl;
        exit(1);
    }
}

