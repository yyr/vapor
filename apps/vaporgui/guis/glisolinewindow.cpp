//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glisolinewindow.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Nov 2005
//
//	Description:  Implementation of GLIsolineWindow class: 
//		This performs the opengl drawing of a texture in the isoline frame.
//

#include "glutil.h"	// Must be included first!!!
#include "glisolinewindow.h"
#include "isolinerenderer.h"
#include <vapor/MyBase.h>
#include <vapor/errorcodes.h>
#include <vapor/jpegapi.h>
#include "isolineframe.h"
#include "isolineparams.h"
#include "isolineeventrouter.h"
#include "messagereporter.h"
#include "session.h"
#include <math.h>
#include <qgl.h>
#include <qapplication.h>
#include <qcursor.h>
#include "assert.h"
#ifdef WIN32
#pragma warning(disable : 4996)
#endif

using namespace VAPoR;
void GLIsolineWindow::mousePressEvent( QMouseEvent * e){
	IsolineEventRouter* per = (IsolineEventRouter*)VizWinMgr::getInstance()->getEventRouter(IsolineParams::_isolineParamsTag);
	if (!isolineFrame->getParams()) return;
	float x,y;
	mapPixelToIsolineCoords(e->x(),e->y(), &x, &y);

	per->guiStartCursorMove();
	vector<double> coords;
	coords.push_back((double)x);
	coords.push_back((double)y);
	isolineFrame->getParams()->SetCursorCoords(coords);
	
	update();
	
}
void GLIsolineWindow::mouseMoveEvent( QMouseEvent * e){
	
	if (!isolineFrame->getParams()) return;
	float x,y;
	mapPixelToIsolineCoords(e->x(),e->y(), &x, &y);
	vector<double> coords;
	coords.push_back((double)x);
	coords.push_back((double)y);
	isolineFrame->getParams()->SetCursorCoords(coords);
	
	
	update();
	
}
void GLIsolineWindow::mouseReleaseEvent( QMouseEvent *e ){
	
	if (!isolineFrame->getParams()) return;
	IsolineEventRouter* per = (IsolineEventRouter*)VizWinMgr::getInstance()->getEventRouter(IsolineParams::_isolineParamsTag);
	float x,y;
	mapPixelToIsolineCoords(e->x(),e->y(), &x, &y);
	vector<double> coords;
	coords.push_back((double)x);
	coords.push_back((double)y);
	isolineFrame->getParams()->SetCursorCoords(coords);
	per->guiEndCursorMove();
	update();
}

GLIsolineWindow::GLIsolineWindow( QGLFormat& fmt, QWidget* parent, const char* , IsolineFrame* pf ) : 
	QGLWidget(fmt, parent) {

	if(!doubleBuffer()){
		QString strng(" Inadequate rendering capability.\n");
		strng += "Ensure your graphics card is properly configured, and/or \n";
		strng += "Be sure to use 'vglrun' if you are in a VirtualGL session.";
		Params::BailOut((const char*)strng.toAscii(),__FILE__,__LINE__);
	}
	rendering = false;
	horizTexSize = 1.f;
	vertTexSize = 1.f;
	rectLeft = -1.f;
	rectTop = 1.f;
	isolineFrame = pf;
	animatingTexture = false;
	animatingFrameNum = 0;
	animationStarting = true;
	
	setAutoBufferSwap(true);
	patternListNum = -1;
	currentAnimationTimestep = 0;
	capturing = false;
	captureNum = 0;
}


/*!
  Release allocated resources.
*/

GLIsolineWindow::~GLIsolineWindow()
{
    makeCurrent();
	//delete the texture?
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLIsolineWindow::resizeGL( int width, int height )
{
	_winWidth = width;
	_winHeight = height;
	if (GLWindow::isRendering()) return;
	_resizeGL();
}

void GLIsolineWindow::_resizeGL() {

	//update the size of the drawing rectangle
	glViewport( 0, 0, (GLint)_winWidth, (GLint)_winHeight);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(1.0, -1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	qglClearColor(palette().color(QPalette::Window));
	

	//Calculate the lower-left and upper right positions for the 
	//texture to be mapped
	float winAspect = (float)_winHeight/(float)_winWidth;
	float texAspect = vertTexSize/horizTexSize;
	if (texAspect > 1.f) texAspect = 1.f;//Never taller than square
	if (texAspect < winAspect) texAspect = winAspect;  //Never wider than window.
	if (winAspect > texAspect){
		//Window is taller than texture, so fill from left to right:
		rectLeft = -1.f;
		rectTop = texAspect/winAspect;
	} else {
		rectLeft = winAspect/texAspect;
		rectTop = 1.f;
	}

}
	
void GLIsolineWindow::setTextureSize(float horiz, float vert){
	vertTexSize = vert;
	horizTexSize = horiz;
	_resizeGL();
}

void GLIsolineWindow::paintGL()
{
#ifdef  Darwin
	//
	// Under Mac OS 10.8.2 paintGL() is called before the frame buffer
	// is ready for drawing
	//
	GLenum status;
	if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
		MyBase::SetDiagMsg(
			"GLIsolineWindow::paintGL() - glCheckFramebufferStatus() = %d", status
		);
		return;
	}
#endif
			
	if (GLWindow::isRendering()) return;
	if(rendering) return;
	rendering = true;

	printOpenGLErrorMsg("GLIsolineWindow");

	_resizeGL();


	printOpenGLErrorMsg("GLIsolineWindow");
	rendering = false;
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLIsolineWindow::initializeGL()
{
	printOpenGLErrorMsg("GLIsolineWindow");
	if (GLWindow::isRendering()) return;
   	makeCurrent();
	glGenTextures(1, &_fbTexid);
	glGenTextures(1, &_isolineTexid);
	glBindTexture(GL_TEXTURE_2D, _isolineTexid);
	if(GLEW_EXT_framebuffer_object) glGenFramebuffersEXT(1, &_fbid);
	qglClearColor(palette().color(QPalette::Window));
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	

	capturing = false;
	
	printOpenGLErrorMsg("GLIsolineWindow");
    
}
//
//To map the window coords, first map the device coords (0..width-1)
// and (0..height-1) to
// float (-1,1).  Then stretch according to rectLeft, rectRight
//
void GLIsolineWindow::mapPixelToIsolineCoords( int ix, int iy, float* x, float*y){
	float xcoord = 1. - 2.f*((float)ix)/((float)(width()-1));
	float ycoord = 1.f - 2.f*((float)iy)/((float)(height()-1));
	*x = xcoord/rectLeft;
	*y = ycoord/rectTop;
}
//Produce an array based on current contents of the (front) buffer.
//This is like the similar method in GLWindow.  Must be invoked while rendering.
bool GLIsolineWindow::
getPixelData(int minx, int miny, int sizex, int sizey, unsigned char* data){
	// set the current window 
	makeCurrent();
	 // Must clear previous errors first.
	while(glGetError() != GL_NO_ERROR);

	glReadBuffer(GL_BACK);
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

//Capture the IBFV image.  Only used for capturing sequence of IBFV images
void GLIsolineWindow::
doFrameCapture(){
	
	//Create a string consisting of captureName, followed by nnnn (framenum), 
	//followed by .jpg
	QString filename = captureName;
	filename += (QString("%1").arg(captureNum)).rightJustified(4,'0');
	filename +=  ".jpg";
	
	//Now open the jpeg file:
	FILE* jpegFile = fopen((const char*)filename.toAscii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: \nError opening output Jpeg file: \n%s",(const char*)filename.toAscii());
		capturing = 0;
		return;
	}
	//The window to extract is x from (1-rectLeft)*w/2 to (1+rectLeft)*w/2 -1
	// y goes from ((1-rectTop)*h/2 to (1+rectTop)*h/2 -1
	int minx = (int) (((1-rectLeft)*width()*.5)+.5);
	int sizex = (int)(rectLeft*width()-1);
	int miny = (int)(((1.-rectTop)*height()*.5)+.5);
	int sizey = (int)(rectTop*height()-1);

	unsigned char* pixData = new unsigned char[sizex*sizey*3];
	
	
	if(!getPixelData(minx, miny, sizex,sizey,pixData)){
		MessageReporter::errorMsg("Image Capture Error; \nerror obtaining GL data");
		capturing = 0;
		delete [] pixData;
		return;
	}
	
	//Now call the Jpeg library to compress and write the file
	//
	int quality = GLWindow::getJpegQuality();
	
	int rc = write_JPEG_file(jpegFile, sizex,sizey, pixData, quality);
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; \nError writing jpeg file ");
		delete [] pixData;
		return;
	}
	delete [] pixData;
	captureNum++;
}

