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
	static unsigned char* buildIBFVTexture(ProbeParams*, int tstep);
	
	static unsigned char* getNextIBFVTexture(ProbeParams*, int frameNum, bool starting);

	static unsigned char* getProbeTexture(ProbeParams*, int frameNum, int fullHeight);
protected:
	GLuint _probeid;
	static void makeIBFVPatterns(ProbeParams*);
	static void getDP(float x, float y, float *px, float *py, float dmaxx, float dmaxy);
	static void pushState();
	static void popState();
	static void stepIBFVTexture(ProbeParams*, int frameNum);
	

};
};

#endif // PROBERENDERER_H
