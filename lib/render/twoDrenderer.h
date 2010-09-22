//************************************************************************
//																		*
//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDrenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2008
//
//	Description:	Definition of the TwoDRenderer class, parent class
//					for TwoDDataRenderer and TwoDImageRenderer
//
#ifndef TWODRENDERER_H
#define TWODRENDERER_H

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
#include "twoDparams.h"
namespace VAPoR {

class RENDER_API TwoDRenderer : public Renderer
{

public:

    TwoDRenderer( GLWindow* , TwoDParams* );
    ~TwoDRenderer();
	
	virtual void	initializeGL();
    virtual void		paintGL()=0;

	static unsigned char* getTwoDTexture(TwoDParams*, int frameNum, bool doCache);
	void setAllDataDirty(){ ((TwoDParams*)getRenderParams())->setTwoDDirty();}
	
	
protected:
	GLuint _twoDid;
	
	//cache elev grid is just for a single time step
	//If cachedTimeStep is < 0 or not the current time step, then it's invalid.
	int maxXElev, maxYElev;
	int cachedTimeStep;
	float *elevVert, *elevNorm;
	float maxXTex, minXTex, maxYTex, minYTex;

	
	void invalidateElevGrid();
	void drawElevationGrid(size_t timestep);
	virtual bool rebuildElevationGrid(size_t timestep)=0;
	void calcElevGridNormals(size_t timestep);

};
};

#endif // TWODRENDERER_H
