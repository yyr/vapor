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
	printOpenGLErrorMsg("GLIsoWindowResizeEvent");
	
	_winWidth = width;
	_winHeight = height;
	
	_resizeGL();
}

void GLIsolineWindow::_resizeGL() {

	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	//update the size of the drawing rectangle
	glViewport( 0, 0, (GLint)_winWidth, (GLint)_winHeight);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	
	const QColor& wincolor = palette().color(QPalette::Window);
	float wcolor[3];
	wcolor[0] = (float)(wincolor.red())/255.;
	wcolor[1] = (float)(wincolor.green())/255.;
	wcolor[2] = (float)(wincolor.blue())/255.;
	glClearColor(wcolor[0],wcolor[1],wcolor[2],1.f);

	//Calculate the lower-left and upper right positions for the 
	//image to be mapped
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
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
}
	
void GLIsolineWindow::setImageSize(float horiz, float vert){
	vertImgSize = vert;
	horizImgSize = horiz;
	_resizeGL();
}

void GLIsolineWindow::paintGL()
{
	printOpenGLErrorMsg("GLIsoWindowPaintGL");
	
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

	printOpenGLErrorMsg("GLIsolineWindowPaintGL");

	_resizeGL();
	
	//If there is a valid cache in the renderer, use it to draw in the tab.
	IsolineParams* iParams = isolineFrame->getParams();
	if (!iParams) {rendering = false; return;}
	IsolineRenderer* iRender = (IsolineRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getVisualizer()->getRenderer(iParams);
	if (!iRender) {rendering = false; return;}
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentTimestep();
	if (!iRender->cacheIsValid(timestep)) {rendering = false; return;}
	
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT);
	const vector<double>& dcolors = iParams->GetPanelBackgroundColor();
	glColor3f((float)dcolors[0],(float)dcolors[1],(float)dcolors[2]); 
	//Paint a rectangle behind the lines
	glBegin(GL_QUADS);
	glVertex2f(rectLeft,-rectTop);
	glVertex2f(rectLeft, rectTop);
	glVertex2f(-rectLeft,rectTop);
	glVertex2f(-rectLeft, -rectTop);
	glEnd();
	const std::map<pair<int,int>,vector<float*> >&  lineCache = iRender->GetLineCache();
	performRendering(timestep, lineCache);

	//Now draw the crosshairs
	if (iParams){
		
		const vector<double>& crosshair = iParams->GetCursorCoords();
	
		float crossX = -crosshair[0]*rectLeft;
		float crossY = crosshair[1]*rectTop;
		glColor3f(ISOL_CURSOR_COLOR);
		glLineWidth(2.f);
		glBegin(GL_LINES);
		glVertex2f(crossX-ISOL_CURSOR_SIZE, crossY);
		glVertex2f(crossX+ISOL_CURSOR_SIZE, crossY);
		glVertex2f(crossX, crossY-ISOL_CURSOR_SIZE);
		glVertex2f(crossX, crossY+ISOL_CURSOR_SIZE);
		glEnd();
	}
	printOpenGLErrorMsg("GLIsolineWindow");
	rendering = false;
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
}

void GLIsolineWindow::performRendering(int timestep, const std::map<pair<int,int>,vector<float*> >&  lineCache){

	IsolineParams* iParams = isolineFrame->getParams();
	
	glLineWidth(iParams->GetPanelLineThickness());
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
	float pointa[3],pointb[3]; //points in cache
	pointa[2]=pointb[2] = 0.;

	//points in cache go from -1 to 1 in both dimensions.  They need to be rescaled to go between rectLeft, -rectTop to -rectLeft,rectTop
	glBegin(GL_LINES);
	
	for(int iso = 0; iso< iParams->getNumIsovalues(); iso++){
		float lineColor[3];
		iParams->getLineColor(iso,lineColor);
		glColor3fv(lineColor);
		pair<int,int> mapPair = make_pair(timestep, iso);
		const vector<float*>& lineVec = lineCache.at(mapPair);
		int numlines = lineVec.size();
		for (int linenum = 0; linenum < numlines; linenum++){
			const float* points = lineVec[linenum];
			pointa[0] = points[0]*rectLeft;
			pointa[1] = points[1]*rectTop;
			pointb[0] = points[2]*rectLeft;
			pointb[1] = points[3]*rectTop;
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
	printOpenGLErrorMsg("GLIsolineWindowInitialize");
	
   	makeCurrent();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	qglClearColor(palette().color(QPalette::Window));
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	
	glPopAttrib();
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
