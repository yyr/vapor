//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glbox.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Definition of the glbox class
//  This is a simple Renderer displaying an openGL wireframe box
//  Derived for QT example code
//  The OpenGL code is mostly borrowed from Brian Pauls "spin" example
//  in the Mesa distribution
//
#ifndef GLBOX_H
#define GLBOX_H

#include <qgl.h>

#include "renderer.h"
namespace VAPoR {
class VizWin;
class GLBox : public Renderer
{
    Q_OBJECT

public:

    GLBox( VizWin*  );
    ~GLBox();
	
	virtual void		initializeGL();
    virtual void		paintGL();

protected:

    
    GLuint 	makeObject();

private:

    GLuint object;
   

};
};

#endif // GLBOX_H
