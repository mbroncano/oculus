//
//  main.cpp
//  Oculus
//
//  Created by Manuel Broncano Rodriguez on 6/12/13.
//  Copyright (c) 2013 Manuel Broncano Rodriguez. All rights reserved.
//

#include "defs.h"
#include "scene.h"
#include "opencl.h"
#include "util.h"
#include <GLUT/GLUT.h>
#include <sys/time.h>

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
    
	openCL->executeKernel();
    
	float seconds = 1000.f * (wallclock() - tick);
	sprintf(label, "size: (%d, %d), prim: %ld, samples: %d, frame: %0.2fms",
            openCL->width,
            openCL->height,
            openCL->scene->primitive_vector.size(),
            openCL->samples,
            seconds);
	printf("%s\n", label);
    
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

int main(int argc, char **argv)
{
	Scene *scene = new Scene();
    //	scene->loadJson("cornell.json");
	scene->testScene();
	scene->buildBVH();
	
	glInit(argc, argv);
	openCL = new OpenCL();
	openCL->scene = scene;
	
	openCL->createTexture();
	openCL->createBuffers();
	openCL->createKernel();
    //	openCL->executeKernel();
    
	glutMainLoop();
	
	delete openCL;
    
	return 0;
}

