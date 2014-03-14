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
	horizImgSize = 1.f;
	vertImgSize = 1.f;
	rectLeft = -1.f;
	rectTop = 1.f;
	isolineFrame = pf;
	
	setAutoBufferSwap(true);
	
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
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	qglClearColor(palette().color(QPalette::Window));
	

	//Calculate the lower-left and upper right positions for the 
	//texture to be mapped
	float winAspect = (float)_winHeight/(float)_winWidth;
	float imgAspect = vertImgSize/horizImgSize;
	if (imgAspect > 1.f) imgAspect = 1.f;//Never taller than square
	if (imgAspect < winAspect) imgAspect = winAspect;  //Never wider than window.
	if (winAspect > imgAspect){
		//Window is taller than image, so fill from left to right:
		rectLeft = -1.f;
		rectTop = imgAspect/winAspect;
	} else {
		rectLeft = winAspect/imgAspect;
		rectTop = 1.f;
	}

}
	
void GLIsolineWindow::setImageSize(float horiz, float vert){
	vertImgSize = vert;
	horizImgSize = horiz;
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
			
	if(rendering) return;
	rendering = true;

	printOpenGLErrorMsg("GLIsolineWindow");
	_resizeGL();
	 glClearColor(.5,.5,.5,0.); 
	 glClear(GL_COLOR_BUFFER_BIT);
	//If there is a valid cache in the renderer, use it to draw in the tab.
	IsolineParams* iParams = isolineFrame->getParams();
	if (!iParams) {rendering = false; return;}
	IsolineRenderer* iRender = (IsolineRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(iParams);
	if (!iRender) {rendering = false; return;}
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentTimestep();
	if (!iRender->cacheIsValid(timestep)) {rendering = false; return;}

	const std::map<pair<int,int>,vector<float*> >&  lineCache = iRender->GetLineCache();
	performRendering(timestep, lineCache);
	printOpenGLErrorMsg("GLIsolineWindow");
	rendering = false;
}

void GLIsolineWindow::performRendering(int timestep, const std::map<pair<int,int>,vector<float*> >&  lineCache){

	IsolineParams* iParams = isolineFrame->getParams();
	//Set up lighting and color
	const vector<double>& dcolors = iParams->GetConstantColor();
	float fcolors[3];
	for (int i = 0; i<3; i++) fcolors[i] = (float)dcolors[i];

	
	glColor3fv(fcolors);
	glLineWidth(iParams->GetLineThickness());
	
	float pointa[3],pointb[3]; //points in cache
	pointa[2]=pointb[2] = 0.;

	glBegin(GL_LINES);
	
	for(int iso = 0; iso< iParams->getNumIsovalues(); iso++){
		pair<int,int> mapPair = make_pair(timestep, iso);
		const vector<float*>& lineVec = lineCache.at(mapPair);
		int numlines = lineVec.size();
		for (int linenum = 0; linenum < numlines; linenum++){
			const float* points = lineVec[linenum];
			pointa[0] = points[0];
			pointa[1] = points[1];
			pointb[0] = points[2];
			pointb[1] = points[3];
			glVertex3fv(pointa);
			glVertex3fv(pointb);
		}
	}
	glEnd();

}
//
//  Set up the OpenGL rendering state, and define display list
//

void GLIsolineWindow::initializeGL()
{
	printOpenGLErrorMsg("GLIsolineWindow");
	if (GLWindow::isRendering()) return;
   	makeCurrent();
	
	qglClearColor(palette().color(QPalette::Window));
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	
	
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
