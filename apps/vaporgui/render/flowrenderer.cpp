//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowrenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:	Implementation of the flowrenderer class
//


#include "flowrenderer.h"
#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "vizwin.h"
#include <math.h>
#include <qgl.h>
#include <qcolor.h>
#include "renderer.h"

/*!
  Create a FlowRenderer widget
*/
using namespace VAPoR;
FlowRenderer::FlowRenderer(VizWin* vw )
:Renderer(vw)
{
    
}


/*!
  Release allocated resources
*/

FlowRenderer::~FlowRenderer()
{
    
    
}


// Perform the rendering
//
  

void FlowRenderer::paintGL()
{
	FlowParams* myFlowParams = VizWinMgr::getInstance()->getFlowParams(myVizWin->getWindowNum());
	//Do we need to regenerate the flow data?
	if (myFlowParams->isDirty()){
		myFlowParams->regenerateFlowData();
	}

	//Do we need to regenerate the rendering geometry?

	//just convert the flow data to a set of lines...
    
	myGLWindow->qglColor( Qt::white );		      // Shorthand for glColor3f or glIndex

    glLineWidth( 2.0 );

// center at .5,.5,.5
	glBegin( GL_LINE_LOOP );
    glVertex3f(  1.0,  1.0, 0.0 );
    glVertex3f(  1.0, 0.0, 0.0 );
    glVertex3f( 0.0, 0.0, 0.0 );
    glVertex3f( 0.0,  1.0, 0.0 );
    glEnd();

    glBegin( GL_LINE_LOOP );
    glVertex3f(  1.0,  1.0, 1.0 );
    glVertex3f(  1.0, 0.0, 1.0 );
    glVertex3f( 0.0, 0.0, 1.0 );
    glVertex3f( 0.0,  1.0, 1.0 );
    glEnd();

    glBegin( GL_LINES );
    glVertex3f(  1.0,  1.0, 0.0 );   glVertex3f(  1.0,  1.0, 1.0 );
    glVertex3f(  1.0, 0.0, 0.0 );   glVertex3f(  1.0, 0.0, 1.0 );
    glVertex3f( 0.0, 0.0, 0.0 );   glVertex3f( 0.0, 0.0, 1.0 );
    glVertex3f( 0.0,  1.0, 0.0 );   glVertex3f( 0.0,  1.0, 1.0 );
    glEnd();
    
	
}


/*!
  Set up the OpenGL rendering state, and define display list
*/

void FlowRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
   
    glShadeModel( GL_FLAT );
}

/*!
  construct the geometry to be rendered
*/

void FlowRenderer::buildFlowGeometry()
{	
    

	myGLWindow->qglColor( Qt::white );		      // Shorthand for glColor3f or glIndex

    glLineWidth( 2.0 );

    glBegin( GL_LINE_LOOP );
    glVertex3f(  0.5,  0.5, -0.5 );
    glVertex3f(  0.5, -0.5, -0.5 );
    glVertex3f( -0.5, -0.5, -0.5 );
    glVertex3f( -0.5,  0.5, -0.5 );
    glEnd();

    glBegin( GL_LINE_LOOP );
    glVertex3f(  0.5,  0.5, 0.5 );
    glVertex3f(  0.5, -0.5, 0.5 );
    glVertex3f( -0.5, -0.5, 0.5 );
    glVertex3f( -0.5,  0.5, 0.5 );
    glEnd();

    glBegin( GL_LINES );
    glVertex3f(  0.5,  0.5, -0.5 );   glVertex3f(  0.5,  0.5, 0.5 );
    glVertex3f(  0.5, -0.5, -0.5 );   glVertex3f(  0.5, -0.5, 0.5 );
    glVertex3f( -0.5, -0.5, -0.5 );   glVertex3f( -0.5, -0.5, 0.5 );
    glVertex3f( -0.5,  0.5, -0.5 );   glVertex3f( -0.5,  0.5, 0.5 );
    glEnd();

    glEndList();

    
}





