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
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
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
	//for each timestep,there is a pair consisting of the isovalue index and a vector of 
	//4 floats (x1,y1,x2,y2) specifying
	//the two endpoints of an isoline segment that crosses a cell 
	std::map<pair<int,int>,vector<float*> > lineCache;
	std::map<int,bool> cacheValidFlags;
	vector<float*>& getLineSegments(int timestep, int isoindex){
		pair<int,int> indexpair = make_pair(timestep,isoindex);
		return lineCache[indexpair];
	}
	bool cacheIsValid(int timestep) {return cacheValidFlags[timestep];}
	bool buildLineCache(int timestep);
	void invalidateLineCache(int timestep);
	void setupCache();
	void performRendering(int timestep);
	//Find code for edges based on isoline crossings
	int edgeCode(int i, int j, int gridx, float isoval, float* dataVals); 
	void addLineSegment(int timestep, int isoIndex, float x1, float y1, float x2, float y2);
	int numIsovalsInCache() {return numIsovalsCached;}
	int numIsovalsCached;
};
};

#endif // ISOLINERENDERER_H
