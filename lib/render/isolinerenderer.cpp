//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolinerenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Implementation of the isolinerenderer class
//

#include "glutil.h"	// Must be included first!!!
#include "params.h"
#include "isolinerenderer.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"
#include <vapor/errorcodes.h>

#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include "renderer.h"
#include <sstream>
using namespace VAPoR;
GLint IsolineRenderer::_storedBuffer = 0;

IsolineRenderer::IsolineRenderer(GLWindow* glw, IsolineParams* pParams )
:Renderer(glw, pParams, "IsolineRenderer")
{
	_isolineTexid = 0;
	_framebufferid = 0;
}


/*
  Release allocated resources
*/

IsolineRenderer::~IsolineRenderer()
{
	if (_isolineTexid) glDeleteTextures(1, &_isolineTexid);
	
}


// Perform the rendering
//
  

void IsolineRenderer::paintGL()
{
	
	
}


std::string IsolineRenderer::instanceName(){
        ostringstream oss;
        oss << "IsolineRenderer_" << this;
        return(oss.str());
}
/*
  Set up the OpenGL rendering state, 
*/

void IsolineRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	
	initialized = true;
}

