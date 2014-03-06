//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDimagerenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Definition of the TwoDImageRenderer class
//
#ifndef TWODIMAGERENDERER_H
#define TWODIMAGERENDERER_H

#include <GL/glew.h>

#ifdef Darwin
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "assert.h"
#include "renderer.h"
#include "twoDimageparams.h"
#include "twoDrenderer.h"
namespace VAPoR {

class RENDER_API TwoDImageRenderer : public TwoDRenderer
{

public:

    TwoDImageRenderer( GLWindow* , TwoDImageParams* );
    ~TwoDImageRenderer();
	
    virtual void		paintGL();

	
protected:
	
	bool rebuildElevationGrid(size_t timestep);
	

};
};

#endif // TWODRENDERER_H
