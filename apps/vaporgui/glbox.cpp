//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glbox.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Implementation of the glbox class
//  This is a simple Renderer displaying an openGL wireframe box
//  Derived for QT example code
//  The OpenGL code is mostly borrowed from Brian Pauls "spin" example
//  in the Mesa distribution
//


#include "glbox.h"
#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "vizwin.h"
#include <math.h>
#include <qgl.h>
#include <qcolor.h>
#include "renderer.h"

/*!
  Create a GLBox widget
*/
using namespace VAPoR;
GLBox::GLBox(VizWin* vw )
:Renderer(vw)
{
    object = 0;
}


/*!
  Release allocated resources
*/

GLBox::~GLBox()
{
    
    glDeleteLists( object, 1 );
}


/*!
  Paint the box. The actual openGL commands for drawing the box are
  performed here.
*/

void GLBox::paintGL()
{
	//Check first if the gui has changed coords
	

    //glClear( GL_COLOR_BUFFER_BIT );
	
    glPushMatrix();

	glLoadIdentity();
	glTranslatef(.5,.5,.5);
    //Note:  this doesn't work right with parallel view.
	myGLWindow->getTBall()->TrackballSetMatrix();
	glTranslatef(-.5,-.5,-.5);
	//If there are new coords, get them from GL, send them to the gui
	if (myVizWin->newViewerCoords){ 
		myGLWindow->changeViewerFrame();
	}
    //glCallList( object );
	myGLWindow->qglColor( Qt::white );		      // Shorthand for glColor3f or glIndex

    glLineWidth( 2.0 );
/* center at .5,.5,.5
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
	*/
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
    
	
	glPopMatrix();
	
}


/*!
  Set up the OpenGL rendering state, and define display list
*/

void GLBox::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
    if(object == 0)object = makeObject();		// Generate an OpenGL display list
    glShadeModel( GL_FLAT );
}

/*!
  Generate an OpenGL display list for the object to be shown, i.e. the box
*/

GLuint GLBox::makeObject()
{	
    GLuint list;

    list = glGenLists( 10 );

    glNewList( list, GL_COMPILE );

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

    return list;
}





