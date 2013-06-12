//
//  opencl.cpp
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#include "opencl.h"
#include "opencl_debug.h"

#include <iostream>
#include <fstream>
#include <string>

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

OpenCL::OpenCL() {
    cl_int err = CL_SUCCESS;
    try {
        Platform::get(&platforms);
        for(int i = 0; i < platforms.size(); i++) {
            Platform p = platforms.at(i);
            platformDump(&p);
        }
        
#ifdef INTEROP
        CGLContextObj glContext = CGLGetCurrentContext();
        CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);
#endif
        
        // mac os only!
        cl_context_properties properties[] = {
#ifdef INTEROP
            CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)shareGroup,
#endif
            CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
            0 };
        context = Context(MAIN_DEVICE, properties);
        devices = context.getInfo<CL_CONTEXT_DEVICES>();
        
        std::cout << "[CL]  Number of devices: " << context.getInfo<CL_CONTEXT_NUM_DEVICES>() << std::endl;
        for(int i = 0; i < devices.size(); i++) {
            Device d = devices.at(i);
            deviceDump(&d);
        }
        
        queue = CommandQueue(context, devices[0], 0, &err);
        
    } catch (Error err) {
        errorDump(err);
        exit(1);
    }
    
    width = 1024;
    height = 768;
    width /= 1;
    height /= 1;
}

void OpenCL::createTexture() {
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glGenTextures(1, &textid);
    
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textid);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
}

	
Program *OpenCL::compileProgram(const char *f, const char *params) {
    try {
        std::string filename(f);
        std::ifstream sourceFile(filename.c_str());
        if (!sourceFile.good())
            throw "Error opening kernel file";
        
        std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
        
        Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
        program = new Program(context, source);
        
        double tick = wallclock();
        std::cout << "[CL]  Compiling: " << filename << "(" << sourceCode.length() << " bytes)" << std::endl;
        try {
            std::string kparams("-cl-strict-aliasing -cl-unsafe-math-optimizations -cl-finite-math-only ");
            if (params)
                kparams += std::string(params);
            
            program->build(kparams.c_str());
        } catch (Error err) {
            programBuildDump(program, &devices[0]);
            throw err;
        }
        programBuildDump(program, &devices[0]);
        std::cout << "[CL]  Compile time: " << f << " ("<< 1000.f * (wallclock() - tick) << " ms)" << std::endl;
    } catch (Error err) {
        errorDump(err);
        exit(1);
    }
    
    return program;
}

void OpenCL::createKernel() {
    program = compileProgram("raytracer.cl");
    
    try {
        runKernel = new Kernel(*program, "raytracer");
        kernelDump(runKernel);
    } catch (Error err) {
        errorDump(err);
        exit(1);
    }
}

void OpenCL::createBuffers() {
    try {
        prim_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Primitive) * scene->primitive_vector.size(), &scene->primitive_vector[0]);
        camera_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), &scene->camera);
        frame_b = Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Vector));
        ray_b = Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Ray));
        bvh_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(BVHNode) * scene->bvhTree->bvh_vec.size(), &scene->bvhTree->bvh_vec[0]);
        
#ifdef INTEROP
        image_b = ImageGL(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_RECTANGLE_ARB, 0, textid);
        glObjects.push_back(image_b);
#else
        rgb = new Pixel[width * height];
        rgb_b = Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Vector));
#endif
        
        samples = 0;
        
    } catch (Error err) {
        errorDump(err);
        exit(1);
    }
}

void OpenCL::executeKernel() {
    try {
        int argc = 0;
        
        runKernel->setArg(argc++, prim_b);
        runKernel->setArg(argc++, scene->primitive_vector.size());
        runKernel->setArg(argc++, camera_b);
        runKernel->setArg(argc++, (random_state_t){(cl_uint)random(), (cl_uint)random()});
        runKernel->setArg(argc++, frame_b);
#ifdef INTEROP
        runKernel->setArg(argc++, image_b);
#else
        runKernel->setArg(argc++, rgb_b);
#endif
        runKernel->setArg(argc++, samples++);
        runKernel->setArg(argc++, bvh_b);
        runKernel->setArg(argc++, scene->bvhTree->bvh_vec.size());
        
#ifdef INTEROP
        queue.enqueueAcquireGLObjects(&glObjects);
#endif
        Event event;
        NDRange global(width, height);
        queue.enqueueNDRangeKernel(*runKernel, NullRange, global, NullRange, NULL, &event);
        event.wait();
        
#ifdef INTEROP
        queue.enqueueReleaseGLObjects(&glObjects);
#else
        queue.enqueueReadBuffer(rgb_b, CL_TRUE, 0, width * height * sizeof(Pixel), rgb);
#endif			
    }
    catch (Error err) {
        errorDump(err);
        exit(1);
    }
}
