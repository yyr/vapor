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
	//Save the current value...
	glGetIntegerv(GL_VIEWPORT, winViewport);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (perspective) {
		GLfloat w = (float) width / (float) height;
		
		float mindist = Max(0.2f, wCenter[2]-maxDim-1.0f);
		//glFrustum( -w, w, -h, h, mindist,(wCenter[2]+ maxDim + 1.0) );
		gluPerspective(45., w, mindist, (wCenter[2]+ maxDim + 10.0) );
		//save the current value...
		glGetDoublev(GL_PROJECTION_MATRIX, (GLdouble *) projectionMatrix);
		
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
	//Also, save the modelview matrix for picking purposes:
	glGetDoublev(GL_MODELVIEW_MATRIX, (GLdouble*) modelviewMatrix);
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
	myVizWin->setViewerCoordsChanged(false);
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
//projectPoint returns true if point is in front of camera
//resulting screen coords returned in 2nd argument.  Note that
//OpenGL coords are 0 at bottom of window!
//
bool GLWindow::projectPointToWin(float cubeCoords[3], float winCoords[2]){
	double depth;
	GLdouble wCoords[2];
	GLdouble cbCoords[3];
	for (int i = 0; i< 3; i++)
		cbCoords[i] = (double) cubeCoords[i];

	bool success = gluProject(cbCoords[0],cbCoords[1],cbCoords[2],(GLdouble*)modelviewMatrix,
		(GLdouble*)projectionMatrix, (GLint*)winViewport, wCoords, (wCoords+1),(GLdouble*)(&depth));
	if (!success) return false;
	winCoords[0] = (float)wCoords[0];
	winCoords[1] = (float)wCoords[1];
	return (depth > 0.0);
}
//Convert a screen coord to a direction vector, representing the direction
//from the camera associated with the screen coords.  Note screen coords
//are OpenGL style
//
bool GLWindow::pixelToVector(int x, int y, const float camPos[3], float dirVec[3]){
	GLdouble pt[3];
	float v[3];
	//Obtain the coords of a point in view:
	bool success = gluUnProject((GLdouble)x,(GLdouble)y,(GLdouble)1.0, (GLdouble*)modelviewMatrix,
		(GLdouble*)projectionMatrix, (GLint*)winViewport,pt, pt+1, pt+2);
	if (success){
		//Convert point to float
		v[0] = (float)pt[0];
		v[1] = (float)pt[1];
		v[2] = (float)pt[2];
		//transform position to world coords
		ViewpointParams::worldFromCube(v,dirVec);
		//Subtract viewer coords to get a direction vector:
		vsub(dirVec, camPos, dirVec);
	}
	return success;
}
//Test if the screen projection of a 3D quad encloses a point on the screen.
//The 4 corners of the quad must be specified in counter-clockwise order
//as viewed from the outside (pickable side) of the quad.  
//Window coords are as in OpenGL (0 at bottom of window)
//
bool GLWindow::
pointIsOnQuad(float cor1[3], float cor2[3], float cor3[3], float cor4[3], float pickPt[2])
{
	float winCoord1[2];
	float winCoord2[2];
	float winCoord3[2];
	float winCoord4[2];
	if(!projectPointToWin(cor1, winCoord1)) return false;
	if (!projectPointToWin(cor2, winCoord2)) return false;
	if (pointOnRight(winCoord1, winCoord2, pickPt)) return false;
	if (!projectPointToWin(cor3, winCoord3)) return false;
	if (pointOnRight(winCoord2, winCoord3, pickPt)) return false;
	if (!projectPointToWin(cor4, winCoord4)) return false;
	if (pointOnRight(winCoord3, winCoord4, pickPt)) return false;
	if (pointOnRight(winCoord4, winCoord1, pickPt)) return false;
	return true;
}



