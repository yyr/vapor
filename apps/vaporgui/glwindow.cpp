//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glwindow.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Implementation of GLWindow class: 
//		it's a pure abstract class for doing openGL in VAPoR
//

#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "vizwin.h"
#include "renderer.h"
#include <math.h>
#include <qgl.h>
#include "assert.h"

using namespace VAPoR;


GLWindow::GLWindow( const QGLFormat& fmt, QWidget* parent, const char* name, VizWin* vw )
: QGLWidget(fmt, parent, name)

{
	myVizWin = vw;
	setAutoBufferSwap(false);
	wCenter[0] = 0.f;
	wCenter[1] = 0.f;
	wCenter[2] = 0.f;
	maxDim = 1.f;
	perspective = false;
	oldPerspective = false;
}


/*!
  Release allocated resources.
*/

GLWindow::~GLWindow()
{
    makeCurrent();
	for (int i = 0; i< myVizWin->getNumRenderers(); i++){
		delete myVizWin->renderer[i];
		myVizWin->setNumRenderers(0);
	}
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLWindow::resizeGL( int width, int height )
{
	/*
    glViewport( 0, 0, (GLint)width, (GLint)height );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
	GLfloat w = (float) width / (float) height;
    GLfloat h = 1.0;
	//Set for orthogonal view:
    glOrtho(-2*w, 2*w, -2*h, 2*h, 5.0, 15.0);
	//glFrustum( -w, w, -h, h, 5.0, 15.0 );
    glMatrixMode( GL_MODELVIEW );
	*/

	glViewport( 0, 0, (GLint)width, (GLint)height );
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (perspective) {
		GLfloat w = (float) width / (float) height;
		
		float mindist = Max(0.2f, wCenter[2]-maxDim-1.0f);
		//glFrustum( -w, w, -h, h, mindist,(wCenter[2]+ maxDim + 1.0) );
		gluPerspective(45., w, mindist, (wCenter[2]+ maxDim + 10.0) );
		
	} else {
		if (width > height) {
			float l, r, s;
			s = (float) width / (float) height;
			l = wCenter[0] - (maxDim*s);
			r = wCenter[0] + (maxDim*s);
			
			glOrtho(l, r, wCenter[1]-maxDim, wCenter[1]+maxDim, wCenter[2]-10.*maxDim-1.0, wCenter[2]+10.*maxDim+1.0); 
			//fprintf(stderr, "glOrtho(%f, %f, %f, %f, %f, %f\n", l,r,yc-hmaxDim, yc+hmaxDim, zc-hmaxDim-1.0, zc+hmaxDim+1.0);
		}
		else {
			float b, t, s;
			s = (float) height / (float) width;
			b = wCenter[1] - (maxDim*s);
			t = wCenter[1] + (maxDim*s);
			
			glOrtho(wCenter[0]-maxDim, wCenter[1]+maxDim, b, t, wCenter[2]-10.*maxDim-1.0, wCenter[2]+10.*maxDim+1.0); 
				//fprintf(stderr, "glOrtho(%f, %f, %f, %f, %f, %f\n", xc-hmaxDim, xc+hmaxDim, b, t, zc-hmaxDim-1.0, zc+hmaxDim+1.0);
			
				//glFrustum((wCenter[0]-maxDim)*.5, (wCenter[1]+maxDim)*.5, b*.5, t*.5, wCenter[2]-maxDim-1.0, wCenter[2]+maxDim+1.0); 
		}
	}
	glMatrixMode(GL_MODELVIEW);

}
	

/*
 * Obtain current view frame from gl model matrix
 */
void GLWindow::
changeViewerFrame(){
	GLfloat m[16], minv[16];
	//Get the frame from GL:
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	//Invert it:
	minvert(m, minv);
	vscale(minv+8, -1.f);
	if (!perspective){
		//Note:  This is a bad hack.  Putting off the time when I correctly implement
		//Ortho coords to actually send perspective viewer to infinity.
		//get the scale out of the (1st 3 elements of ) matrix:
		//
		float scale = vlength(m);
		float trans;
		if (scale < 5.f) trans = 1.-5./scale;
		else trans = scale - 5.f;
		minv[14] = -trans;
	}
	myVizWin->changeCoords(minv+12, minv+8, minv+4);
}

void GLWindow::paintGL()
{
	//Force a resize if perspective has changed:
	if (perspective != oldPerspective){
		resizeGL(width(), height());
		oldPerspective = perspective;
	}
	glClear( GL_COLOR_BUFFER_BIT );
	//assert(myVizWin->getNumRenderers() <= 1);
	for (int i = 0; i< myVizWin->getNumRenderers(); i++){
		myVizWin->renderer[i]->paintGL();
	}
	swapBuffers();
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLWindow::initializeGL()
{
    qglClearColor( black ); 		// Let OpenGL clear to black
	//Initialize existing renderers:
	//
	for (int i = 0; i< myVizWin->getNumRenderers(); i++){
		myVizWin->renderer[i]->initializeGL();
	}
    
}





