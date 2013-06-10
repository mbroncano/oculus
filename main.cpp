#define __CL_ENABLE_EXCEPTIONS

#include "cl.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
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

// functor to sort by axis
struct sort_axis {
	int i;
	
	sort_axis(int i) : i(i) {}
	bool operator()(const BVH& a, const BVH& b) { return (a.min.s[i] + a.max.s[i])/2.f < (b.max.s[i] + b.min.s[i])/2.f; }
};


struct BVHTree {
	std::vector<BVH> bvh_vec;

	BVHTree(std::vector<Primitive>& pvec) {
		for (int i = 0; i < pvec.size(); i ++) {
			BVH bvh = nodeFromPrimitive(pvec, i);
			bvh_vec.push_back(bvh);
		};

		std::vector<BVH *> flat;
		int root = flatBVHTree(bvh_vec, bvh_vec.begin(), bvh_vec.end() - 1, 0, flat);

		bvh_vec.clear();
		for (int i = 0; i < flat.size(); i++) {
			bvh_vec.push_back(*flat[i]);
		}
	}

	BVH nodeFromPrimitive(std::vector<Primitive>& pvec, int i) {
		Primitive p = pvec[i];
		BVH node;
		if (p.t == sphere) {
			Vector r = (Vector){p.sphere.r, p.sphere.r, p.sphere.r};
			node.min = p.sphere.c - r;
			node.max = p.sphere.c + r;
		} else if (p.t == triangle) {
			node.min = min(p.triangle.p[0], p.triangle.p[1], p.triangle.p[2]);
			node.max = max(p.triangle.p[0], p.triangle.p[1], p.triangle.p[2]);
		}
		node.left = node.right = 0;
		node.pid = i;
		
		return node;
	}

	int flatBVHTree(
		std::vector<BVH> list, 
		std::vector<BVH>::iterator start, 
		std::vector<BVH>::iterator end, 
		int axis,
		std::vector<BVH *>& flat
		)
	{
		int d = end - start;
		int index = flat.size();

		flat.push_back(new BVH());
		BVH *p = flat.back();

		p->min = (*start).min;
		p->max = (*start).max;

		if (d == 0) {
			p->pid = (*start).pid;
			p->left = p->right = -1;
			return index;
		}

		for (std::vector<BVH>::iterator it = start; it != end; ++it) {
			p->min = min(p->min, (*it).min);
			p->max = max(p->max, (*it).max);
		}

		std::sort(start, end, sort_axis(axis % 3));
		axis++;

		p->pid = -1;
		p->left = flatBVHTree(list, start, start + d/2, axis, flat);
		p->right = flatBVHTree(list, start + d/2 + 1, end, axis, flat);

		return index;
	}
	
};

struct Scene {
	Camera camera;
	std::map<std::string, Material> material_map;
	std::vector<Primitive> primitive_vector;
	BVHTree *bvhTree;
	
	void buildBVH() {
		bvhTree = new BVHTree(primitive_vector);
		for (int i = 0; i < bvhTree->bvh_vec.size(); i++) {
			std::cout << "["<< i << "] " << bvhTree->bvh_vec[i] << std::endl;
		}
		
	}

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
	Program *program;
	Kernel *initKernel;
	Kernel *runKernel;
	CommandQueue queue;

	int width;
	int height;
	int samples;
	Scene *scene;
	
	GLuint textid;
	Buffer prim_b, camera_b, random_b, frame_b, ray_b;

#ifdef INTEROP
	ImageGL image_b;
	std::vector<Memory> glObjects;
#else
	Pixel *rgb;
	Buffer rgb_b;
#endif


	void platformDump(Platform p) {
		std::cout 
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " Platform: " << p.getInfo<CL_PLATFORM_NAME>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Vendor: " << p.getInfo<CL_PLATFORM_VENDOR>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Version: " << p.getInfo<CL_PLATFORM_VERSION>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Profile: " << p.getInfo<CL_PLATFORM_PROFILE>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Extensions: " << p.getInfo<CL_PLATFORM_EXTENSIONS>() << std::endl;
	}
	
	void deviceDump(Device d) {
		std::cout
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " Device: " << d.getInfo<CL_DEVICE_NAME>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Type: " << d.getInfo<CL_DEVICE_TYPE>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Version: " << d.getInfo<CL_DEVICE_VERSION>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Extensions: " << d.getInfo<CL_DEVICE_EXTENSIONS>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Compute units: " << d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Global mem size: " << d.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Local mem size: " << d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Work groups: " << d.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Image support: " << d.getInfo<CL_DEVICE_IMAGE_SUPPORT>() << std::endl;
	}
	
	#define HAS_MASK(a, mask) if ((a & mask) == mask)
	
	const char *kernelArgAddress(unsigned int a) {
		std::string ret;
		
		HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_GLOBAL) ret += "global ";
		HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_LOCAL) ret += "local ";
		HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_CONSTANT) ret += "constant ";
		HAS_MASK(a, CL_KERNEL_ARG_ADDRESS_PRIVATE) ret += "private ";
		
		return ret.c_str();
	}

	const char *kernelArgAccess(unsigned int a) {
		std::string ret;
		
		HAS_MASK(a, CL_KERNEL_ARG_ACCESS_READ_ONLY) ret += "read ";
		HAS_MASK(a, CL_KERNEL_ARG_ACCESS_WRITE_ONLY) ret += "write ";
		HAS_MASK(a, CL_KERNEL_ARG_ACCESS_READ_WRITE) ret += "read/write ";
		HAS_MASK(a, CL_KERNEL_ARG_ACCESS_NONE) ret += "none ";
		
		return ret.c_str();
	}

	const char *kernelArgType(unsigned int a) {
		std::string ret;
		
		HAS_MASK(a, CL_KERNEL_ARG_TYPE_NONE) ret += "none ";
		HAS_MASK(a, CL_KERNEL_ARG_TYPE_CONST) ret += "const ";
		HAS_MASK(a, CL_KERNEL_ARG_TYPE_RESTRICT) ret += "restrict ";
		HAS_MASK(a, CL_KERNEL_ARG_TYPE_VOLATILE) ret += "volatile ";
		
		return ret.c_str();
	}
	
	void kernelDump(Kernel *k) {
		std::cout 
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " Kernel: " << k->getInfo<CL_KERNEL_FUNCTION_NAME>() << std::endl
			<< "[OpenCL:"<< __FUNCTION__ << "]" << " * Args: " << k->getInfo<CL_KERNEL_NUM_ARGS>() << std::endl;
			/*
		for (int i = 0; i < initKernel->getInfo<CL_KERNEL_NUM_ARGS>(); i ++) {
			std::cout
				<< "[OpenCL:"<< __FUNCTION__ << "]" << " = Name: " << k->getArgInfo<CL_KERNEL_ARG_NAME>(i) << std::endl
				<< "[OpenCL:"<< __FUNCTION__ << "]" << "   = Type: " << k->getArgInfo<CL_KERNEL_ARG_TYPE_NAME>(i) << std::endl
				<< "[OpenCL:"<< __FUNCTION__ << "]" << "   = Type qualifier: " << kernelArgType(initKernel->getArgInfo<CL_KERNEL_ARG_TYPE_QUALIFIER>(i)) << std::endl
				<< "[OpenCL:"<< __FUNCTION__ << "]" << "   = Access qualifier: " << kernelArgAccess(k->getArgInfo<CL_KERNEL_ARG_ACCESS_QUALIFIER>(i)) << std::endl
				<< "[OpenCL:"<< __FUNCTION__ << "]" << "   = Address qualifier: " << kernelArgAddress(k->getArgInfo<CL_KERNEL_ARG_ADDRESS_QUALIFIER>(i)) << std::endl;
		}*/
	}
	
	const char *programBuildStatus(int s) {
		if (s == 0) return "success";
		if (s == -1) return "none";
		if (s == -2) return "error";
		if (s == -3) return "in progress";
		return "unknown";
	}
	
	void programBuildDump(Program *p) {
		std::cerr << "[OpenCL:"<< __FUNCTION__ << "]" << " Build Status: " << programBuildStatus(program->getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0])) << std::endl;
		std::cerr << "[OpenCL:"<< __FUNCTION__ << "]" << " Build Options: " << program->getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
		std::string log = program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]);
		if (log.size() > 0)
			std::cerr << "[OpenCL:"<< __FUNCTION__ << "]" << " Build Log\n\n" << program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
	}

	void errorDump(Error err) {
		std::cerr << "[OpenCL:"<< __FUNCTION__ << "]" << " Error: " << err.what() << "(" << err.err() << ")" << std::endl;
	}
	
	OpenCL() {
		cl_int err = CL_SUCCESS;
		try {
			Platform::get(&platforms);
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

			std::cout << "[OpenCL:"<< __FUNCTION__ << "]" << " Number of devices: " << context.getInfo<CL_CONTEXT_NUM_DEVICES>() << std::endl;
			for(int i = 0; i < devices.size(); i++) {
				Device d = devices.at(i);
				deviceDump(d);
			}
			
			queue = CommandQueue(context, devices[0], 0, &err);

		} catch (Error err) {
			errorDump(err);
			exit(1);
		}
		
		width = 1024;
		height = 768;
	}
	
	void createTexture() {
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glGenTextures(1, &textid);

		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textid);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	}
	
	
	Program *compileProgram(const char *f, const char *params = NULL) {
		try {
			std::string filename(f);
			std::ifstream sourceFile(filename.c_str());
			std::string sourceCode(std::istreambuf_iterator<char>(sourceFile), (std::istreambuf_iterator<char>()));

			Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
			program = new Program(context, source);

			double tick = wallclock();
			std::cout << "[OpenCL:"<< __FUNCTION__ << "]" << " Compiling: " << filename << std::endl;
			try {
				std::string kparams("-cl-strict-aliasing -cl-unsafe-math-optimizations -cl-finite-math-only ");
				if (params)
					kparams += std::string(params);
				
				program->build(kparams.c_str());
			} catch (Error err) {
				programBuildDump(program);
				throw err;
			}
			programBuildDump(program);
			std::cout << "[OpenCL:"<< __FUNCTION__ << "]" << " Compile time: " << f << " ("<< 1000.f * (wallclock() - tick) << " ms)" << std::endl;
		} catch (Error err) {
			errorDump(err);
			exit(1);
		}
		
		return program;
	}
	
	void createKernel() {
		program = compileProgram("raytracer.cl");
		
		try {
			runKernel = new Kernel(*program, "raytracer");
			kernelDump(runKernel);
		} catch (Error err) {
			errorDump(err);
			exit(1);
		}
	}
	
	void createBuffers() {
		try {
			prim_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Primitive) * scene->primitive_vector.size(), &scene->primitive_vector[0]);
			camera_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), &scene->camera);
			frame_b = Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Vector));
			ray_b = Buffer(context, CL_MEM_READ_WRITE, width * height * sizeof(Ray));

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
	
	void executeKernel() {
		cl_int err = CL_SUCCESS;

		try {
			int argc = 0;
			
			runKernel->setArg(argc++, prim_b);
			runKernel->setArg(argc++, scene->primitive_vector.size());
			runKernel->setArg(argc++, camera_b);
			runKernel->setArg(argc++, sizeof(Primitive) * scene->primitive_vector.size(), NULL);
			runKernel->setArg(argc++, (random_state_t){random(), random()});
			runKernel->setArg(argc++, frame_b);
#ifdef INTEROP
			runKernel->setArg(argc++, image_b);
#else
			runKernel->setArg(argc++, rgb_b);
#endif
			runKernel->setArg(argc++, samples++);

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
};

// main object
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

	//openCL->executeInitKernel();
	openCL->executeKernel();

	float seconds = 1000.f * (wallclock() - tick);
	sprintf(label, "size: (%d, %d), samples: %d, frame: %0.3fms", openCL->width, openCL->height, openCL->samples, seconds);

#ifndef INTEROP
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, openCL->textid);
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
}

int main(int argc, char **argv) {
	Scene *scene = new Scene();
	scene->loadJson("cornell.json");
	scene->buildBVH();
	return 0;
	
	glInit(argc, argv);
	openCL = new OpenCL();
	openCL->scene = scene;
	
	openCL->createTexture();
	openCL->createBuffers();
	openCL->createKernel();

	glutMainLoop();
	
	delete openCL;

	return 0;
}