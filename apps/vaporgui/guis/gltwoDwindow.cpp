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

#include "glutil.h"	// Must be included first!!!
#include "gltwoDwindow.h"
#include "twoDrenderer.h"

#include "twodframe.h"
#include "twoDparams.h"
#include "twoDdataeventrouter.h"
#include "twoDimageeventrouter.h"
#include "messagereporter.h"
#include "session.h"
#include <math.h>
#include <qgl.h>
#include <qapplication.h>
#include <qcursor.h>
#include "assert.h"

using namespace VAPoR;


GLTwoDWindow::GLTwoDWindow( const QGLFormat& fmt, QWidget* parent, const char* , TwoDFrame* pf )
: QGLWidget(fmt, parent)

{
	if(!doubleBuffer()){
		QString strng(" Inadequate rendering capability.\n");
		strng += "Ensure your graphics card is properly configured, and/or \n";
		strng += "Be sure to use 'vlgrun' if you are in a VirtualGL session.";
		Params::BailOut((const char*)strng.toAscii(),__FILE__,__LINE__);
	}
	rendering = false;
	horizTexSize = 1.f;
	vertTexSize = 1.f;
	rectLeft = -1.f;
	rectTop = 1.f;
	twoDFrame = pf;
	_winWidth = 1;
	_winHeight = 1;
	
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
	_winWidth = width;
	_winHeight = height;
	if (GLWindow::isRendering()) return;
	_resizeGL();
}

void GLTwoDWindow::_resizeGL() {

	//update the size of the drawing rectangle
	glViewport( 0, 0, (GLint) _winWidth, (GLint) _winHeight);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	qglClearColor(palette().color(QPalette::Window));		// same as frame

	//Calculate the lower-left and upper right positions for the 
	//texture to be mapped
	float winAspect = (float) _winHeight/(float) _winWidth;
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
	
void GLTwoDWindow::setTextureSize(float horiz, float vert) {
	vertTexSize = vert;
	horizTexSize = horiz;

	_resizeGL();
}


void GLTwoDWindow::paintGL()
{
#ifdef	Darwin
	//
	// Under Mac OS 10.8.2 paintGL() is called before the frame buffer
	// is ready for drawing
	//
	GLenum status;
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        MyBase::SetDiagMsg(
            "GLProbeWindow::paintGL() - glCheckFramebufferStatus() = %d", status
        );
        return;
    }
#endif

	if (GLWindow::isRendering()) return;
	if (rendering) return;

	_resizeGL();

	rendering = true;
	printOpenGLErrorMsg("GLTwoDWindow");
	
	TwoDParams* myParams;
	if (isDataWindow) myParams = VizWinMgr::getActiveTwoDDataParams();
	else myParams = VizWinMgr::getActiveTwoDImageParams();
	
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentTimestep();

    qglClearColor(palette().color(QPalette::Window));		// same as frame
	
	glClearDepth(1);
	glPolygonMode(GL_FRONT,GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	if (myParams->doBypass(timestep)) {rendering = false; return;}
	if (!myParams->isEnabled()) {rendering = false; return;}
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	//get the twoD texture:

	int texWidth, texHeight;
	const unsigned char *twoDTexture = NULL;
	if(myParams ) {
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

		twoDTexture = TwoDRenderer::getTwoDTexture(
				myParams,timestep,texWidth,texHeight
		);

		QApplication::restoreOverrideCursor();
	}
	
	
	if(twoDTexture) {
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth,texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, twoDTexture);
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
	
	qglClearColor(palette().color(QPalette::Window));	// same as frame
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	//glGenTextures(1, &texName);
	//glBindTexture(GL_TEXTURE_2D, texName);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	

	
    
	printOpenGLErrorMsg("GLTwoDWindow");
}
//
//To map the window coords, first map the device coords (0..width-1)
// and (0..height-1) to
// float (-1,1).  Then stretch according to rectLeft, rectRight
//
void GLTwoDWindow::mapPixelToTwoDCoords( int ix, int iy, float* x, float*y){
	float xcoord = 1.f - 2.f*((float)ix)/((float)(width()-1));
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
	glReadBuffer(GL_BACK);
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
void GLTwoDWindow::paintEvent(QPaintEvent* event){
	QRect geom = twoDFrame->geometry();
        if (!GLWindow::isRendering()) QGLWidget::paintEvent(event);
}

void GLTwoDWindow::mousePressEvent( QMouseEvent * e){
	float x,y;
	if (!twoDFrame->getParams()) return;
	if (isDataWindow){
		TwoDDataEventRouter* per = VizWinMgr::getInstance()->getTwoDDataRouter();
		mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		per->guiStartCursorMove();
	}
	else {
		TwoDImageEventRouter* per = VizWinMgr::getInstance()->getTwoDImageRouter();
		mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		per->guiStartCursorMove();
	}
	twoDFrame->getParams()->setCursorCoords(x,y);
	update();
	
}


void GLTwoDWindow::mouseReleaseEvent( QMouseEvent *e ){
	
	if (!twoDFrame->getParams()) return;
	float x,y;
	if (isDataWindow){
		TwoDDataEventRouter* per = VizWinMgr::getInstance()->getTwoDDataRouter();
		mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		twoDFrame->getParams()->setCursorCoords(x,y);
		per->guiEndCursorMove();
	}
	else {
		TwoDImageEventRouter* per = VizWinMgr::getInstance()->getTwoDImageRouter();
		mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
		twoDFrame->getParams()->setCursorCoords(x,y);
		per->guiEndCursorMove();
	}
	update();
}
	
//When the mouse moves, display its new coordinates.  Move the "grabbed" 
//control point, or zoom/pan the display
//
void GLTwoDWindow::mouseMoveEvent( QMouseEvent * e){
	
	if (!twoDFrame->getParams()) return;
	float x,y;
	mapPixelToTwoDCoords(e->x(),e->y(), &x, &y);
	twoDFrame->getParams()->setCursorCoords(x,y);
	update();
	
}

