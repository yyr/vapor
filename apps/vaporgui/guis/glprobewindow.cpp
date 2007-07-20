//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glprobewindow.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Nov 2005
//
//	Description:  Implementation of GLProbeWindow class: 
//		This performs the opengl drawing of a texture in the probe frame.
//

#include "glprobewindow.h"

#include "glutil.h"
#include "probeframe.h"
#include "probeparams.h"
#include "probeeventrouter.h"
#include <math.h>
#include <qgl.h>
#include <qapplication.h>
#include <qcursor.h>
#include "assert.h"

using namespace VAPoR;


GLProbeWindow::GLProbeWindow( const QGLFormat& fmt, QWidget* parent, const char* name, ProbeFrame* pf )
: QGLWidget(fmt, parent, name)

{
	horizTexSize = 1.f;
	vertTexSize = 1.f;
	rectLeft = -1.f;
	rectTop = 1.f;
	probeFrame = pf;
	
}


/*!
  Release allocated resources.
*/

GLProbeWindow::~GLProbeWindow()
{
    makeCurrent();
	//delete the texture?
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLProbeWindow::resizeGL( int width, int height )
{
	//update the size of the drawing rectangle
	glViewport( 0, 0, (GLint)width, (GLint)height );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	qglClearColor( QColor(233,236,216) ); 		// same as frame

	//Calculate the lower-left and upper right positions for the 
	//texture to be mapped
	float winAspect = (float)height/(float)width;
	float texAspect = vertTexSize/horizTexSize;
	if (winAspect > texAspect){
		//Window is taller than texture, so fill from left to right:
		rectLeft = -1.f;
		rectTop = texAspect/winAspect;
	} else {
		rectLeft = winAspect/texAspect;
		rectTop = 1.f;
	}

}
	
void GLProbeWindow::setTextureSize(float horiz, float vert){
	vertTexSize = vert;
	horizTexSize = horiz;
	float winAspect = (float)height()/(float)width();
	float texAspect = vertTexSize/horizTexSize;
	if (winAspect > texAspect){
		//Window is taller than texture, so fill from left to right:
		rectLeft = -1.f;
		
		rectTop = texAspect/winAspect;
		
	} else {
		rectLeft = -winAspect/texAspect;
		rectTop = 1.f;
	}
}


void GLProbeWindow::paintGL()
{
	ProbeParams* myParams = VizWinMgr::getActiveProbeParams();
	size_t fullHeight = VizWinMgr::getActiveRegionParams()->getFullGridHeight();
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentFrameNumber();

	//VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();

    qglClearColor( QColor(233,236,216) ); 		// same as frame
    //qglClearColor(vizWin->getBackgroundColor()); // same as vizualizer win

	glClearDepth(1);
	glPolygonMode(GL_FRONT,GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT);
	
	//glDrawBuffer(buffer);
	//glColor3f(1.0f,0.f,0.f);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);
	glDisable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	//get the probe texture:
	
	unsigned char* probeTexture = 0;
	
	if(myParams){
		if (myParams->probeIsDirty(timestep)){
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
			probeTexture = myParams->getProbeTexture(timestep,fullHeight);
			QApplication::restoreOverrideCursor();
		} else {
			probeTexture = myParams->getProbeTexture(timestep,fullHeight);
		}
	}
	int imgWidth = myParams->getImageWidth();
	int imgHeight = myParams->getImageHeight();
	if(probeTexture) {
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgWidth,imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, probeTexture);
	} else {
		return;
		//glColor4f(0.,0.,0.,0.);
	}
	glBegin(GL_QUADS);
	//Just specify the rectangle corners
	glTexCoord2f(0.f,0.f); glVertex2f(rectLeft,-rectTop);
	glTexCoord2f(0.f, 1.f); glVertex2f(rectLeft, rectTop);
	glTexCoord2f(1.f,1.f); glVertex2f(-rectLeft,rectTop);
	glTexCoord2f(1.f, 0.f); glVertex2f(-rectLeft, -rectTop);
	glEnd();
	glFlush();
	if (probeTexture) glDisable(GL_TEXTURE_2D);
	//Now draw the crosshairs
	if (myParams){
		const float* crossHairCoords = myParams->getCursorCoords();
		float crossX = crossHairCoords[0]*rectLeft;
		float crossY = crossHairCoords[1]*rectTop;
		glColor3f(CURSOR_COLOR);
		glLineWidth(2.f);
		glBegin(GL_LINES);
		glVertex2f(crossX-CURSOR_SIZE, crossY);
		glVertex2f(crossX+CURSOR_SIZE, crossY);
		glVertex2f(crossX, crossY-CURSOR_SIZE);
		glVertex2f(crossX, crossY+CURSOR_SIZE);
		glEnd();
	}
	
      glDisable(GL_BLEND);
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLProbeWindow::initializeGL()
{
   	makeCurrent();
	qglClearColor( QColor(233,236,216) ); 		// same as frame
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glGenTextures(1, &texName);
	//glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//Build probe texture:
	
    
}
//
//To map the window coords, first map the device coords (0..width-1)
// and (0..height-1) to
// float (-1,1).  Then stretch according to rectLeft, rectRight
//
void GLProbeWindow::mapPixelToProbeCoords( int ix, int iy, float* x, float*y){
	float xcoord = 2.f*((float)ix)/((float)(width()-1))-1.f;
	float ycoord = 1.f - 2.f*((float)iy)/((float)(height()-1));
	*x = xcoord/rectLeft;
	*y = ycoord/rectTop;
}


