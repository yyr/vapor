#ifndef GLINC_H
#define GLINC_H

//#include <GL/glew.h>
#ifdef	Darwin
//#include <OpenGL/gl.h>
//#include <OpenGL/glu.h>
//#include <glut.h>
#else
//#include <GL/gl.h>
//#include <GL/glu.h>
//#include <GL/glut.h>
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#endif

#endif

