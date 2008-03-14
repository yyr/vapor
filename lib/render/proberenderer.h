//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		proberenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Definition of the ProbeRenderer class
//
#ifndef PROBERENDERER_H
#define PROBERENDERER_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "assert.h"
#include "renderer.h"
#include "probeparams.h"
namespace VAPoR {

class RENDER_API ProbeRenderer : public Renderer
{

public:

    ProbeRenderer( GLWindow* , ProbeParams* );
    ~ProbeRenderer();
	
	virtual void	initializeGL();
    virtual void		paintGL();

	//Method that uses OpenGL to construct an IBFV texture of specified size.
	//Should be called within an OpenGL rendering context.
	static unsigned char* buildIBFVTexture(int fullHeight, ProbeParams*, int tstep);
	
	static unsigned char* getNextIBFVTexture(int fullHeight, ProbeParams*, int tstep, int frameNum, bool starting);

	static unsigned char* getProbeTexture(ProbeParams*, int frameNum, int fullHeight, bool doCache);
protected:
	GLuint _probeid;
	static void makeIBFVPatterns(ProbeParams*);
	static void getDP(float x, float y, float *px, float *py, float dmaxx, float dmaxy);
	static void pushState(int width, int height);
	static void popState();
	static void stepIBFVTexture(ProbeParams*, int timeStep, int frameNum);
	

};
};

#endif // PROBERENDERER_H
