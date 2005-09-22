//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowrenderer.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:	Definition of the FlowRenderer class
//
#ifndef FLOWRENDERER_H
#define FLOWRENDERER_H

#include <qgl.h>
#include "assert.h"
#include "renderer.h"
namespace VAPoR {
class VizWin;
class FlowRenderer : public Renderer
{
    Q_OBJECT

public:

    FlowRenderer( VizWin*  );
    ~FlowRenderer();
	
	virtual void		initializeGL();
    virtual void		paintGL();

protected:
	float* flowDataArray;
	float* flowRGBAs;
	int maxPoints, firstDisplayFrame, lastDisplayFrame, numSeedPoints, numInjections;
	
	void renderTubes(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderCurves(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderPoints(float radius, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderArrows(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	float* getFlowPoint(int timeStep, int seedNum, int injectionNum){
		assert ((3*(timeStep+ maxPoints*(seedNum+ numSeedPoints*injectionNum)))<
			3*maxPoints*numSeedPoints*numInjections );
		return (flowDataArray+ 3*(timeStep+ maxPoints*(seedNum+ numSeedPoints*injectionNum)));
	}
	// Render a "stationary symbol" at the specified point
	void renderStationary(float* point, float rad);
	//draw one arrow
	void drawArrow(bool isLit, int firstIndex, float* dirVec, float* bVec, float* UVec, float radius, bool constMap);

	float constFlowColor[4];
	float arrowHeadRadius;

};
};

#endif // FLOWRENDERER_H
