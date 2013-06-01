#define __CL_ENABLE_EXCEPTIONS

#include "cl.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

using namespace cl;

#include "geometry.h"


int main(void) {	
	int width = 1024;
	int height = 1024;
	
	Camera camera = (Camera) {(Vector) {50.f, 45.f, 205.6f},  (Vector) {50.f, 45.f - 0.042612f, 204.6f}};
	Sphere spheres[] = {
		(Sphere) {(Vector) {27.f, 16.5f, 47.f}, 16.5f,			(Material) {Specular, (Vector){.25f, .75f, .25f}, 0.f}},	// mirror
		(Sphere) {(Vector) {73.f, 16.5f, 78.f}, 16.5f,			(Material) {Diffuse, (Vector){.25f, .75f, .25f}, 0.f}},		// glass
		(Sphere) {(Vector) {50.f, 66.6f, 81.6f}, 7.f,			(Material) {Diffuse, (Vector){.9f, .9f, .9f}, 1.f}},		// light
		(Sphere) {(Vector) {50.f, -1e4f + 81.6f, 81.6f}, 1e4f,	(Material) {Diffuse, (Vector){.75f, .75f, .75f}, 0.f}},		// top
		(Sphere) {(Vector) {50.f, 1e4f, 81.6f}, 1e4f,			(Material) {Diffuse, (Vector){.75f, .75f, .75f}, 0.f}},		// bottom
		(Sphere) {(Vector) {1e4f + 1.f, 40.8f, 81.6f}, 1e4f,	(Material) {Diffuse, (Vector){.75f, .25f, .25f}, 0.f}},		// left
		(Sphere) {(Vector) {-1e4f + 99.f, 40.8f, 81.6f}, 1e4f,	(Material) {Diffuse, (Vector){.25f, .25f, .75f}, 0.f}},		// right
		(Sphere) {(Vector) {50.f, 40.8f, 1e4f}, 1e4f,			(Material) {Diffuse, (Vector){.75f, .75f, .75f}, 0.f}},		// back
		(Sphere) {(Vector) {50.f, 40.8f, -1e4f + 270.f}, 1e4f,	(Material) {Diffuse, (Vector){.0f, .0f, .0f}, 0.f}},		// front
	};
	int numspheres = sizeof(spheres) / sizeof(Sphere);
	
	Pixel *fb = new Pixel[width*height];
	
	
	
	cl_int err = CL_SUCCESS;
	try {
		std::vector<Platform> platforms;
		Platform::get(&platforms);

		cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};
		Context context(CL_DEVICE_TYPE_CPU, properties); 

		std::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

		std::string filename("raytracer.cl");
		std::ifstream sourceFile(filename.c_str());
		std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));
		
		Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
		
		Program program = Program(context, source);
		
		try {
			std::cout << "Building kernel: " << filename << std::endl;
			program.build();
		} catch (Error err) {
			std::cout << "Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
			std::cout << "Build Options: " << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
			std::cout << "Build Log:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
			
			throw err;
		}
		

		Kernel kernel(program, "raytracer", &err);

		CommandQueue queue(context, devices[0], 0, &err);
		
		Buffer spheres_b = Buffer(context, CL_MEM_READ_ONLY, sizeof(Sphere) * numspheres);
		Buffer camera_b = Buffer(context, CL_MEM_READ_ONLY, sizeof(Camera));
		Buffer fb_b = Buffer(context, CL_MEM_WRITE_ONLY, sizeof(Pixel) * width * height);

	 	queue.enqueueWriteBuffer(spheres_b, CL_TRUE, 0, sizeof(Sphere) * numspheres, spheres);	
	 	queue.enqueueWriteBuffer(camera_b, CL_TRUE, 0, sizeof(Camera), &camera);	

		kernel.setArg(0, spheres_b);
		kernel.setArg(1, numspheres);
		kernel.setArg(2, camera_b);
		kernel.setArg(3, fb_b);

		Event event;
		NDRange global(width, height);
		queue.enqueueNDRangeKernel(kernel, NullRange, global, NullRange, NULL, &event); 

		event.wait();

	 	queue.enqueueReadBuffer(fb_b, CL_TRUE, 0, sizeof(Pixel) * width * height, fb);
	
		std::ofstream outputFile("result.ppm");
		outputFile << "P6\n" << width << " " << height << "\n255\n";
		outputFile.write((char *)fb, sizeof(Pixel) * width * height);

	}
	catch (Error err) {
	 std::cerr 
	    << "ERROR: "
	    << err.what()
	    << "("
	    << err.err()
	    << ")"
	    << std::endl;
	
		exit(1);
	}

	return EXIT_SUCCESS;
}