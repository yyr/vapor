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
#include <GL/gl.h>
#include <GL/glu.h>
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
	
protected:
	GLuint _twoDid;
	int numElevTimesteps;
	//Cached elevation grid (one for each time step)
	int *maxXElev, *maxYElev; // size for each time step
	float** elevVert, **elevNorm;
	float *maxXTex, *minXTex, *maxYTex, *minYTex;
	
	void invalidateElevGrid();
	void drawElevationGrid(size_t timestep);
	virtual bool rebuildElevationGrid(size_t timestep)=0;
	void calcElevGridNormals(size_t timestep);
	

};
};

#endif // TWODRENDERER_H
