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

    ProbeRenderer( GLWindow* , ProbeParams*);
    ~ProbeRenderer();
	
	virtual void	initializeGL();
    virtual void		paintGL();

	//Methods that use OpenGL to construct IBFV texture of specified size.
	//Should be called within an OpenGL rendering context.
	//Static so they can be called either from the proberenderer or from
	//the probe gl window.
	static unsigned char* buildIBFVTexture(ProbeParams*, int tstep, GLuint fbid, GLuint texid);
	static unsigned char* getNextIBFVTexture(ProbeParams*, int tstep, int frameNum, bool starting, int* listNum, GLuint fbid, GLuint texid);
	static unsigned char* getProbeTexture(ProbeParams*, int frameNum, bool doCache, GLuint fbid, GLuint texid);
	
protected:
	GLuint _probeid;
	GLuint _framebufferid;
	static int makeIBFVPatterns(ProbeParams*, int prevListNum);
	
	static void pushState(int width, int height, GLuint fbid, GLuint texid);
	static void popState();
	static void stepIBFVTexture(ProbeParams*, int timeStep, int frameNum, int listNum);
	
	

};
};

#endif // PROBERENDERER_H
