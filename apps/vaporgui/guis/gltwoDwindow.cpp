//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		gltwoDwindow.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:  Implementation of GLTwoDWindow class: 
//		This performs the opengl drawing of a texture in the twoD (data or image) frame.
//

#include "gltwoDwindow.h"
#include "twoDrenderer.h"

#include "glutil.h"
#include "twodframe.h"
#include "twoDparams.h"
#include "twoDeventrouter.h"
#include "messagereporter.h"
#include "session.h"
#include <math.h>
#include <qgl.h>
#include <qapplication.h>
#include <qcursor.h>
#include "assert.h"

using namespace VAPoR;


GLTwoDWindow::GLTwoDWindow( const QGLFormat& fmt, QWidget* parent, const char* name, TwoDFrame* pf )
: QGLWidget(fmt, parent, name)

{
	if(!doubleBuffer()){
		QString strng(" Inadequate rendering capability.\n");
		strng += "Ensure your graphics card is properly configured, and/or \n";
		strng += "Be sure to use 'vlgrun' if you are in a VirtualGL session.";
		Params::BailOut(strng.ascii(),__FILE__,__LINE__);
	}
	rendering = false;
	horizTexSize = 1.f;
	vertTexSize = 1.f;
	rectLeft = -1.f;
	rectTop = 1.f;
	twoDFrame = pf;
	
	setAutoBufferSwap(true);
	
}


/*!
  Release allocated resources.
*/

GLTwoDWindow::~GLTwoDWindow()
{
    makeCurrent();
	//delete the texture?
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLTwoDWindow::resizeGL( int width, int height )
{
	if (GLWindow::isRendering()) return;
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
	
void GLTwoDWindow::setTextureSize(float horiz, float vert){
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


void GLTwoDWindow::paintGL()
{
	if (GLWindow::isRendering()) return;
	if (rendering) return;
	
	rendering = true;
	printOpenGLErrorMsg("GLTwoDWindow");
	
	TwoDParams* myParams;
	if (isDataWindow) myParams = VizWinMgr::getActiveTwoDDataParams();
	else myParams = VizWinMgr::getActiveTwoDImageParams();
	
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentFrameNumber();

    qglClearColor( QColor(233,236,216) ); 		// same as frame
	
	glClearDepth(1);
	glPolygonMode(GL_FRONT,GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if (myParams->doBypass(timestep)) {rendering = false; return;}
	if (!myParams->isEnabled()) {rendering = false; return;}
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	//get the twoD texture:
	unsigned char* twoDTexture = 0;
	int imgSize[2];
	if(myParams ){
		if (myParams->twoDIsDirty(timestep)){
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
			twoDTexture = TwoDRenderer::getTwoDTexture(myParams,timestep,false);
			QApplication::restoreOverrideCursor();
		} else {
			twoDTexture = TwoDRenderer::getTwoDTexture(myParams,timestep,false);
		}
	}
	
	
	if(twoDTexture) {
		myParams->getTextureSize(imgSize, timestep);
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgSize[0],imgSize[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, twoDTexture);
	} else {
		rendering = false;
		return;
	}

	glBegin(GL_QUADS);
	//Just specify the rectangle corners
	glTexCoord2f(0.f,0.f); glVertex2f(rectLeft,-rectTop);
	glTexCoord2f(0.f, 1.f); glVertex2f(rectLeft, rectTop);
	glTexCoord2f(1.f,1.f); glVertex2f(-rectLeft,rectTop);
	glTexCoord2f(1.f, 0.f); glVertex2f(-rectLeft, -rectTop);
	glEnd();
	//glFlush();
	if (twoDTexture) glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	

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
	printOpenGLErrorMsg("GLTwoDWindow");
	rendering = false;
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLTwoDWindow::initializeGL()
{
	printOpenGLErrorMsg("GLTwoDWindow");
	if (GLWindow::isRendering()) return;
   	makeCurrent();
	
	qglClearColor( QColor(233,236,216) ); 		// same as frame
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	//glGenTextures(1, &texName);
	//glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
    
	printOpenGLErrorMsg("GLTwoDWindow");
}
//
//To map the window coords, first map the device coords (0..width-1)
// and (0..height-1) to
// float (-1,1).  Then stretch according to rectLeft, rectRight
//
void GLTwoDWindow::mapPixelToTwoDCoords( int ix, int iy, float* x, float*y){
	float xcoord = 2.f*((float)ix)/((float)(width()-1))-1.f;
	float ycoord = 1.f - 2.f*((float)iy)/((float)(height()-1));
	*x = xcoord/rectLeft;
	*y = ycoord/rectTop;
}
//Produce an array based on current contents of the (front) buffer.
//This is like the similar method in GLWindow.  Must be invoked while rendering.
bool GLTwoDWindow::
getPixelData(int minx, int miny, int sizex, int sizey, unsigned char* data){
	// set the current window 
	makeCurrent();
	 // Must clear previous errors first.
	while(glGetError() != GL_NO_ERROR);

	//if (front)
	//
	//glReadBuffer(GL_FRONT);
	//
	//else
	//  {
	glReadBuffer(GL_BACK_LEFT);
	//  }
	glDisable( GL_SCISSOR_TEST );


 
	// Turn off texturing in case it is on - some drivers have a problem
	// getting / setting pixels with texturing enabled.
	glDisable( GL_TEXTURE_2D );

	assert( 0<=minx && minx < sizex);
	assert(sizex <= width());
	assert (0 <= miny && sizey > miny);
	assert(sizey <= height());
	// Calling pack alignment ensures that we can grab the any size window
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadPixels(minx, miny, sizex,sizey, GL_RGB,
				GL_UNSIGNED_BYTE, data);
	if (glGetError() != GL_NO_ERROR)
		return false;
	//Unfortunately gl reads these in the reverse order that jpeg expects, so
	//Now we need to swap top and bottom!
	unsigned char val;
	for (int j = 0; j< sizey/2; j++){
		for (int i = 0; i<sizex*3; i++){
			val = data[i+sizex*3*j];
			data[i+sizex*3*j] = data[i+sizex*3*(sizey-j-1)];
			data[i+sizex*3*(sizey-j-1)] = val;
		}
	}
	
	return true;
}


