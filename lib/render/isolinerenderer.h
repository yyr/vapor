//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolinerenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2014
//
//	Description:	Definition of the IsolineRenderer class
//
#ifndef ISOLINERENDERER_H
#define ISOLINERENDERER_H

#include <GL/glew.h>
#ifdef Darwin
#include <gl.h>
#include <glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "assert.h"
#include "renderer.h"
#include "isolineparams.h"
namespace VAPoR {

class RENDER_API IsolineRenderer : public Renderer
{

public:

    IsolineRenderer( GLWindow* , IsolineParams*);
    ~IsolineRenderer();
	
	virtual void	initializeGL();
    virtual void		paintGL();

	virtual void setAllDataDirty() {}
	
protected:
	GLuint _isolineTexid;
	GLuint _framebufferid;
	static GLint _storedBuffer;
	std::string instanceName();		
	
};
};

#endif // ISOLINERENDERER_H
