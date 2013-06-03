#define __CL_ENABLE_EXCEPTIONS

#include "cl.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <sys/time.h>
#include <GLUT/glut.h>


using namespace cl;

#include "geometry.h"

Camera camera = (Camera) {(Vector) {50.f, 45.f, 205.6f},  (Vector) {50.f, 45.f - 0.042612f, 204.6f}};
Sphere spheres[] = {
	(Sphere) {(Vector) {27.f, 16.5f, 47.f}, 16.5f,			(Material) {Specular, (Vector){.9f, .9f, .9f}, 0.f}},	// mirror
	(Sphere) {(Vector) {73.f, 16.5f, 78.f}, 16.5f,			(Material) {Refractive, (Vector){.9f, .9f, .9f}, 0.f}},		// glass
	(Sphere) {(Vector) {50.f, 66.6f, 81.6f}, 7.f,			(Material) {Diffuse, (Vector){.9f, .9f, .9f}, 12.f}},		// light
	(Sphere) {(Vector) {50.f, -1e4f + 81.6f, 81.6f}, 1e4f,	(Material) {Diffuse, (Vector){.75f, .75f, .75f}, 0.f}},		// top
	(Sphere) {(Vector) {50.f, 1e4f, 81.6f}, 1e4f,			(Material) {Diffuse, (Vector){.75f, .75f, .75f}, 0.f}},		// bottom
	(Sphere) {(Vector) {1e4f + 1.f, 40.8f, 81.6f}, 1e4f,	(Material) {Diffuse, (Vector){.75f, .25f, .25f}, 0.f}},		// left
	(Sphere) {(Vector) {-1e4f + 99.f, 40.8f, 81.6f}, 1e4f,	(Material) {Diffuse, (Vector){.25f, .25f, .75f}, 0.f}},		// right
	(Sphere) {(Vector) {50.f, 40.8f, 1e4f}, 1e4f,			(Material) {Diffuse, (Vector){.75f, .75f, .75f}, 0.f}},		// back
	(Sphere) {(Vector) {50.f, 40.8f, -1e4f + 270.f}, 1e4f,	(Material) {Diffuse, (Vector){.0f, .0f, .0f}, 0.f}},		// front
};
int numspheres = sizeof(spheres) / sizeof(Sphere);
GLuint textid;

double wallclock() {
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
}

struct OpenCL {

	std::vector<Platform> platforms;
	std::vector<Device> devices;
	Context context;
	Kernel kernel;
	CommandQueue queue;

	int width;
	int height;

	
	OpenCL() {
		cl_int err = CL_SUCCESS;
		try {
			Platform::get(&platforms);
			std::cout << "[OpenCL] Number of platforms: " << platforms.size() << std::endl;
			for(int i = 0; i < platforms.size(); i++) {
				Platform p = platforms.at(i);
				std::cout << "[OpenCL] Platform: " << i << std::endl;
				std::cout << "[OpenCL] * Name: " << p.getInfo<CL_PLATFORM_NAME>() << std::endl;
				std::cout << "[OpenCL] * Vendor: " << p.getInfo<CL_PLATFORM_VENDOR>() << std::endl;
				std::cout << "[OpenCL] * Version: " << p.getInfo<CL_PLATFORM_VERSION>() << std::endl;
				std::cout << "[OpenCL] * Profile: " << p.getInfo<CL_PLATFORM_PROFILE>() << std::endl;
				std::cout << "[OpenCL] * Extensions: " << p.getInfo<CL_PLATFORM_EXTENSIONS>() << std::endl;
			}
			
			CGLContextObj glContext = CGLGetCurrentContext();
			CGLShareGroupObj shareGroup = CGLGetShareGroup(glContext);

			// mac os only!
			cl_context_properties properties[] = {
				CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)shareGroup,
				CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 
				0 };
			context = Context(CL_DEVICE_TYPE_GPU, properties); 

			devices = context.getInfo<CL_CONTEXT_DEVICES>();
			std::cout << "[OpenCL] Number of devices: " << devices.size() << std::endl;
			for(int i = 0; i < devices.size(); i++) {
				Device d = devices.at(i);
				std::cout << "[OpenCL] Device: " << i << std::endl;
				std::cout << "[OpenCL] * Name: " << d.getInfo<CL_DEVICE_NAME>() << std::endl;
				std::cout << "[OpenCL] * Type: " << d.getInfo<CL_DEVICE_TYPE>() << std::endl;
				std::cout << "[OpenCL] * Version: " << d.getInfo<CL_DEVICE_VERSION>() << std::endl;
				std::cout << "[OpenCL] * Extensions: " << d.getInfo<CL_DEVICE_EXTENSIONS>() << std::endl;
				std::cout << "[OpenCL] * Compute units: " << d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
				std::cout << "[OpenCL] * Global mem size: " << d.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl;
				std::cout << "[OpenCL] * Local mem size: " << d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
				std::cout << "[OpenCL] * Local mem size: " << d.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
				std::cout << "[OpenCL] * Work groups: " << d.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
				std::cout << "[OpenCL] * Image support: " << d.getInfo<CL_DEVICE_IMAGE_SUPPORT>() << std::endl;
			}
			
			queue = CommandQueue(context, devices[0], 0, &err);

		} catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
		
		width = height = 512;
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
//				program.build("-Werror -cl-strict-aliasing -cl-unsafe-math-optimizations -cl-finite-math-only");
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
			
			kernel.setArg(0, spheres_b);
			kernel.setArg(1, numspheres);
			kernel.setArg(2, camera_b);
			kernel.setArg(3, fb_b);
			kernel.setArg(4, sizeof(Sphere) * numspheres, NULL);
			
			std::cout << "[OpenCL] Kernel: " << k << std::endl;
			std::cout << "[OpenCL] * Private mem size: " << kernel.getWorkGroupInfo<CL_KERNEL_PRIVATE_MEM_SIZE>(devices[0]) << std::endl;
			std::cout << "[OpenCL] * Local mem size: " << kernel.getWorkGroupInfo<CL_KERNEL_LOCAL_MEM_SIZE>(devices[0]) << std::endl;
			
		} catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
	}
	
	Buffer spheres_b, camera_b;
	ImageGL fb_b;
	std::vector<Memory> glObjects;
	
	void createBuffers() {
		try {
			spheres_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Sphere) * numspheres, spheres);
			camera_b = Buffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(Camera), &camera);
			fb_b = ImageGL(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, textid);
			glObjects.push_back(fb_b);
		} catch (Error err) {
			std::cerr << "[OpenCL] Error: " << err.what() << "(" << err.err() << ")" << std::endl;
			exit(1);
		}
	}
	
	void executeKernel() {
		cl_int err = CL_SUCCESS;
		double tick;

		try {
			tick = wallclock();
			queue.enqueueAcquireGLObjects(&glObjects);
			//printf("[OpenCL] Write buffers: %.2f ms\n", 1000.f * (wallclock() - tick));

			tick = wallclock();
			Event event;
			NDRange global(width, height);
			queue.enqueueNDRangeKernel(kernel, NullRange, global, NullRange, NULL, &event); 

			event.wait();
			//printf("[OpenCL] Execution: %.2f ms\n", 1000.f * (wallclock() - tick));

			tick = wallclock();
			queue.enqueueReleaseGLObjects(&glObjects);
			//printf("[OpenCL] Read buffers: %.2fms\n", 1000.f * (wallclock() - tick));
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

	glBindTexture(GL_TEXTURE_2D, textid);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f(screen_w - 1.0f, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f(screen_w - 1.0f, screen_h - 1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f,  screen_h - 1.0f);
	glEnd();

	glEnable(GL_BLEND);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.f, 0.f, 0.8f, 0.7f);
	glRecti(10.f, 10.f, screen_w - 10.f, 40.f);

	glColor3f(1.f, 1.f, 1.f);
	glRasterPos2f(15.f, 20.f);
	for (int i = 0; i < strlen (label); i++)
 		glutBitmapCharacter (GLUT_BITMAP_HELVETICA_18, label[i]);

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
	sprintf(label, "size: (%d, %d), time: %0.3fms", openCL->width, openCL->height, seconds);	

	glutPostRedisplay();
}

void createTexture() {
	glBindTexture(GL_TEXTURE_2D, textid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, openCL->width, openCL->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
}

void glInit(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA);
	glutInitWindowSize(1024, 1024);
	glutInitWindowPosition(1300, 50);
	glutCreateWindow(argv[0]);

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &textid);
}

int main(int argc, char **argv) {	

	glInit(argc, argv);
	openCL = new OpenCL();
	
	createTexture();
	openCL->createBuffers();
	openCL->createKernel("raytracer.cl", "raytracer");

	glutMainLoop();
	
	delete openCL;

	return EXIT_SUCCESS;
}