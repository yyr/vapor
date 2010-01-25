//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDdatarenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Definition of the TwoDDataRenderer class
//
#ifndef TWODDATARENDERER_H
#define TWODDATARENDERER_H

#include <GL/glew.h>

#ifdef Darwin
#include <gl.h>
#include <glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "assert.h"
#include "twoDrenderer.h"
#include "twoDdataparams.h"
namespace VAPoR {

class RENDER_API TwoDDataRenderer : public TwoDRenderer
{

public:

    TwoDDataRenderer( GLWindow* , TwoDDataParams* );
    ~TwoDDataRenderer();
	
    virtual void		paintGL();

	
protected:
	bool rebuildElevationGrid(size_t timestep);
	
};
};

#endif // TWODDATARENDERER_H
