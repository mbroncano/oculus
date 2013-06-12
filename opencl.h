//
//  opencl.h
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#ifndef __Oculus__opencl__
#define __Oculus__opencl__

#define __CL_ENABLE_EXCEPTIONS

#include "defs.h"
#include "scene.h"
#include "cl.hpp"
#include <vector>

using namespace cl;

struct OpenCL {
	std::vector<Platform> platforms;
	std::vector<Device> devices;
	Context context;
	Program *program;
	Kernel *initKernel;
	Kernel *runKernel;
	CommandQueue queue;
    
	int width;
	int height;
	int samples;
	Scene *scene;
	
	GLuint textid;
	Buffer prim_b, camera_b, random_b, frame_b, ray_b, bvh_b;
    
#ifdef INTEROP
	ImageGL image_b;
	std::vector<Memory> glObjects;
#else
	Pixel *rgb;
	Buffer rgb_b;
#endif
    
    OpenCL();
    void createTexture();
   	Program *compileProgram(const char *f, const char *params = NULL);
	void createKernel();
    void createBuffers();
	void executeKernel();
    
};

#endif /* defined(__Oculus__opencl__) */
