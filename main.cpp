#define __CL_ENABLE_EXCEPTIONS

#include "cl.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <sys/time.h>
#include <GLUT/glut.h>
#include <exception>
#include <algorithm>
#include "parson/parson.h"

using namespace cl;

#include "geometry.h"

double wallclock() {
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
}

struct Scene {
	Camera camera;
	std::map<std::string, Material> material_map;
	std::vector<Primitive> primitive_vector;

	Vector getVector(JSON_Array *vector_array) {
		if (json_array_get_count(vector_array) != 3) {
			throw "reading vector ";
		}
		Vector v;
		for (int i = 0; i < 3; i ++)
			v.s[i] = json_array_get_number(vector_array, i); 

		return v;
	}

	void loadJson(const char *f) {
		try {
			JSON_Value *_root = json_parse_file(f);
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
						JSON_Object *_point = json_array_get_object(_points, j);
						primitive.triangle.p[j] = getVector(json_array_get_array(_points, j));

						std::cout << "p" << j << " ["
									<< primitive.triangle.p[j].s[0] << ", "
									<< primitive.triangle.p[j].s[1] << ", "
									<< primitive.triangle.p[j].s[2] << "]" << std::endl;
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
	
};


struct OpenCL {
	std::vector<Platform> platforms;
	std::vector<Device> devices;
	Context context;
	Kernel kernel;
	CommandQueue queue;

	int width;
	int height;	
	int samples;	
	Scene *scene;
	
	GLuint textid;
	

	void platformDump(Platform p) {
		std::cout 
			<< "[OpenCL] Platform: " << p.getInfo<CL_PLATFORM_NAME>() << std::endl
			<< "[OpenCL] * Vendor: " << p.getInfo<CL_PLATFORM_VENDOR>() << std::endl
			<< "[OpenCL] * Version: " << p.getInfo<CL_PLATFORM_VERSION>() << std::endl
			<< "[OpenCL] * Profile: " << p.getInfo<CL_PLATFORM_PROFILE>() << std::endl
			<< "[OpenCL] * Extensions: " << p.getInfo<CL_PLATFORM_EXTENSIONS>() << std::endl;
	}
	
	void deviceDump(Device d) {
		std::cout
			<< "[OpenCL] Device: " << d.getInfo<CL_DEVICE_NAME>() << std::endl
			<< "[OpenCL] * Type: " << d.getInfo<CL_DEVICE_TYPE>() << std::endl
			<< "[OpenCL] * Version: " << d.getInfo<CL_DEVICE_VERSION>() << std::endl
			<< "[OpenCL] * Extensions: " << d.getInfo<CL_DEVICE_EXTENSIONS>() << std::endl
			<< "[OpenCL] * Compute units: " << d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl
			<< "[OpenCL] * Global mem size: " << d.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl
			<< "[OpenCL] * Local mem size: " << d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl
			<< "[OpenCL] * Work groups: " << d.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl
			<< "[OpenCL] * Image support: " << d.getInfo<CL_DEVICE_IMAGE_SUPPORT>() << std::endl;
	}
	
	OpenCL() {
		cl_int err = CL_SUCCESS;
		try {
			Platform::get(&platforms);
			std::cout << "[OpenCL] Number of platforms: " << platforms.size() << std::endl;
			for(int i = 0; i < platforms.size(); i++) {
				Platform p = platforms.at(i);
				platformDump(p);
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
			context = Context(CL_DEVICE_TYPE_GPU, properties); 

			devices = context.getInfo<CL_CONTEXT_DEVICES>();
			std::cout << "[OpenCL] Number of devices: " << devices.size() << std::endl;
			for(int i = 0; i < devices.size(); i++) {
				Device d = devices.at(i);
				deviceDump(d);
			}
			
			queue = CommandQueue(context, devices[0], 0, &err);

		} catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
		
		width = 1024;
		height = 768;
	}
	
	void createTexture() {
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textid);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	
	void createKernel(const char *f, const char *k) {
		cl_int err = CL_SUCCESS;
		double tick;

		try {
			std::string filename(f);
			std::ifstream sourceFile(filename.c_str());
			std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));

			Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
			Program program = Program(context, source);

			tick = wallclock();
			try {
				std::cout << "[OpenCL] Building kernel: " << filename << std::endl;
				program.build("-cl-strict-aliasing -cl-unsafe-math-optimizations -cl-finite-math-only");
			} catch (Error err) {
				std::cerr << "[OpenCL] Error! " << std::endl;
				std::cerr << "[OpenCL] Build Status: " << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
				std::cerr << "[OpenCL] Build Options: " << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
				std::cerr << "[OpenCL] Build Log:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;

				throw err;
			}
			std::cout << "[OpenCL] Build Log:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
			printf("[OpenCL] Compile: %.2fms\n", 1000.f * (wallclock() - tick));

			kernel = Kernel(program, "raytracer", &err);
			
			std::cout << "[OpenCL] Kernel: " << k << std::endl;
			std::cout << "[OpenCL] * Private mem size: " << kernel.getWorkGroupInfo<CL_KERNEL_PRIVATE_MEM_SIZE>(devices[0]) << std::endl;
			std::cout << "[OpenCL] * Local mem size: " << kernel.getWorkGroupInfo<CL_KERNEL_LOCAL_MEM_SIZE>(devices[0]) << std::endl;
			
		} catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
	}
	
	Buffer spheres_b, camera_b, random_b, frame_b;
	
#ifdef INTEROP
	ImageGL image_b;
	std::vector<Memory> glObjects;
#else
	Pixel *rgb;
	Buffer rgb_b;
#endif
	
	void createBuffers() {
		try {
			spheres_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Primitive) * scene->primitive_vector.size(), &scene->primitive_vector[0]);
			camera_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), &scene->camera);
			
			// HACK: initializes buffer, as CL_MEM_ALLOC_HOST_PTR doesn't seem to do so
			// TODO: do this in opencl
			Vector *frame = new Vector[width * height];
			frame_b = Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, width * height * sizeof(Vector), frame);
			delete frame;
			

#ifdef INTEROP
			image_b = ImageGL(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_RECTANGLE_ARB, 0, textid);
			glObjects.push_back(image_b);
#else
			rgb = new Pixel[width * height];
			rgb_b = Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Vector));
#endif
			samples = 0;
			
		} catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
	}
	
	void executeKernel() {
		cl_int err = CL_SUCCESS;
		double tick;

		try {
			kernel.setArg(0, spheres_b);
			kernel.setArg(1, scene->primitive_vector.size());
			kernel.setArg(2, camera_b);
			kernel.setArg(3, sizeof(Primitive) * scene->primitive_vector.size(), NULL);

			random_state_t seed = {random(), random()};
			kernel.setArg(4, seed);

			kernel.setArg(5, frame_b);
#ifdef INTEROP
			kernel.setArg(6, image_b);
#else
			kernel.setArg(6, rgb_b);
#endif
			kernel.setArg(7, samples++);

			tick = wallclock();
#ifdef INTEROP
			queue.enqueueAcquireGLObjects(&glObjects);
#endif

			tick = wallclock();
			Event event;
			NDRange global(width, height);
			queue.enqueueNDRangeKernel(kernel, NullRange, global, NullRange, NULL, &event); 

			event.wait();

			tick = wallclock();
#ifdef INTEROP
			queue.enqueueReleaseGLObjects(&glObjects);
#else
			queue.enqueueReadBuffer(rgb_b, CL_TRUE, 0, width * height * sizeof(Pixel), rgb);
#endif			
		}
		catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
	}
};

OpenCL * openCL;

// GLUT, OpenGL related functions
char label[256];
int screen_w, screen_h;

void reshape(int width, int height) {
	screen_w = width;
	screen_h = height;
	glViewport(0, 0, width, height);
}

void display() {
	glLoadIdentity();
	glOrtho(0.f, screen_w - 1.f, 0.f, screen_h - 1.f, -1.f, 1.f);

	glColor3f(1.f, 1.f, 1.f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, openCL->textid);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(openCL->width - 1.0f, 0.0f); glVertex2f(screen_w - 1.0f, 0.0f);
	glTexCoord2f(openCL->width - 1.0f, openCL->height - 1.0f); glVertex2f(screen_w - 1.0f, screen_h - 1.0f);
	glTexCoord2f(0.0f, openCL->height - 1.0f); glVertex2f(0.0f,  screen_h - 1.0f);
	glEnd();
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.0f, 0.0f, 0.8f, 0.5f);
	glRecti(10.f, 10.f, screen_w - 10.f, 40.f);

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2f(15.f, 20.f);
	for (int i = 0; i < strlen (label); i++)
 		glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, label[i]);
	glDisable(GL_BLEND);

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
	switch(key) {
		case 27: exit(0);
		default: break;
	}
}

void idle() {	
	double tick = wallclock();

	openCL->executeKernel();

	float seconds = 1000.f * (wallclock() - tick);
	sprintf(label, "size: (%d, %d), samples: %d, frame: %0.3fms", openCL->width, openCL->height, openCL->samples, seconds);

#ifndef INTEROP
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textid);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, openCL->width, openCL->height, 0, GL_RGB, GL_UNSIGNED_BYTE, openCL->rgb);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
#endif	

	glutPostRedisplay();
}



void glInit(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
	glutInitWindowSize(1024, 768);
	glutInitWindowPosition(50, 50);
	glutCreateWindow(argv[0]);

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	
	glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glGenTextures(1, &textid);
}

int main(int argc, char **argv) {
	Scene *scene = new Scene();
	scene->loadJson("scene.json");
	
	glInit(argc, argv);
	openCL = new OpenCL();
	openCL->scene = scene;
	
	openCL->createTexture();
	openCL->createBuffers();
	openCL->createKernel("raytracer.cl", "raytracer");

	glutMainLoop();
	
	delete openCL;

	return 0;
}