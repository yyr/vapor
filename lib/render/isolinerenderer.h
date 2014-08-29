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
	void invalidateLineCache(int timestep);
	void invalidateLineCache();
	bool cacheIsValid(int timestep) {return cacheValidFlags[timestep];}
	bool buildLineCache(int timestep);
	const std::map<pair<int,int>,vector<float*> >&  GetLineCache() {return lineCache;}
protected:
	//for each timestep,there is a pair consisting of the isovalue index and a vector of 
	//4 floats (x1,y1,x2,y2) specifying
	//the two endpoints of an isoline segment that crosses a cell 
	std::map<pair<int,int>,vector<float*> > lineCache;
	
	// cache valid flags indicate validity of cache at each time step.
	std::map<int,bool> cacheValidFlags;
	//Each edge is represented as a pair (int, int).  Horizontal edges have both ints positive; vertical have both negative.
	//Vertical edges connecting grid vertices (i,j) and (i,j+1) are represented by the pair (-i, -j-1) (-i <=0; -j-1 < 0)
	//Horizontal edges connecting grid vertices (i,j) and (i+1,j) are represented by the pair (i+1,j) (i+1 > 0, j>=0)
	//
	
	// lineCache maps pair<int,int>->vector<float[4]> maps a pair (timestep,isovalueIndex) to a vector of segment endpoints, with one segment for each line in the isoline
	//For each segment in the lineCache, there are 5 temporary mappings:
	// prevEdge maps segments to edges.  Each segment is identified by (timestep, isoval, segmentIndex), corresponding edge is pair(int,int)
	// nextEdge is the same; map(int,int,int)->pair(int,int)
	// plus prevSegment maps prevEdge to segment; i.e. map (timestep, isoval, (int,int))-> segmentIndex
	// similarly nextSegment maps nextEdge to segment; i.e. map (timestep, isoval, (int,int))-> segmentIndex
	// plus forwardEdge maps (timestep,isoval,edge)-> edge
	// and backwardEdge maps (timestep,isoval,edge)-> edge
	
	std::map< pair<int,int>, int> edgeSeg; //map an edge to a segment, to find 3d coords where edge is located
	
	std::map<pair<int,int>,pair<int,int> > edgeEdge1; //map an edge to one adjacent edge;
	std::map<pair<int,int>,pair<int,int> > edgeEdge2; //map an edge to other adjacent edge;
	std::map<pair<int,int>, bool> markerBit;  //indicate whether or not an edge has been visited during current traversal
	vector<int> componentLength;	//indicates the number of segments in a component.
	vector<pair<int,int> > endEdge;  //indicates and ending edge for each component


	//Whenever a segment is added, construct associated edge and segment mappings
	void addEdges(int segIndex, pair<int,int> edge1, pair<int,int> edge2);
	//traverse the curves defined by the latest edge->edge mappings
	void traverseCurves(int iso, int timestep);
	
	vector<float*>& getLineSegments(int timestep, int isoindex){
		pair<int,int> indexpair = make_pair(timestep,isoindex);
		return lineCache[indexpair];
	}
	//Interpolate the isovalue on the grid line from (i,j) to (i,j+1)
	float interp_j(int i, int j, float isoval, float* dataVals){
		return ( -1. + 2.*(double)(j)/((double)gridSize-1.) //y coordinate at (i,j)
		+ 2./(double)(gridSize -1.)  //y grid spacing
		*(isoval - dataVals[i+gridSize*j])/(dataVals[i+gridSize*(j+1)]-dataVals[i+gridSize*j])); //ratio: 1 is iso at top, 0 if iso at bottom
	}
	//Interpolate the isovalue on the grid line from (i,j) to (i+1,j)
	float interp_i(int i, int j, float isoval, float* dataVals){
		return (-1. + 2.*(double)(i)/((double)gridSize-1.) //x coordinate at (i,j)
		+ 2./(double)(gridSize -1.)  //x grid spacing
		*(isoval - dataVals[i+gridSize*j])/(dataVals[i+1+gridSize*(j)]-dataVals[i+gridSize*j])); //ratio: 1 is iso at right, 0 if iso at left
	}
	
	
	void setupCache();
	void performRendering(int timestep);
	//Find code for edges based on isoline crossings
	int edgeCode(int i, int j, float isoval, float* dataVals); 
	//insert a line segment, return index in the associated vector that was added.
	int addLineSegment(int timestep, int isoIndex, float x1, float y1, float x2, float y2);
	int numIsovalsInCache() {return numIsovalsCached;}
	int numIsovalsCached;
	int gridSize;
	std::map<int,int> objectNums;
};
};

#endif // ISOLINERENDERER_H
