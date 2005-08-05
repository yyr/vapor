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
	int maxPoints, minAge, numSeedPoints, numInjections;
	
	void renderTubes(float radius, bool isLit, int firstAge, int lastAge, float* data);
	void renderCurves(float radius, bool isLit, int firstAge, int lastAge, float* data);
	void renderPoints(float radius, int firstAge, int lastAge, float* data);
	float* getFlowPoint(int timeStep, int seedNum, int injectionNum){
		assert ((3*(timeStep+ maxPoints*(seedNum+ numSeedPoints*injectionNum)))<
			3*maxPoints*numSeedPoints*numInjections );
		return (flowDataArray+ 3*(timeStep+ maxPoints*(seedNum+ numSeedPoints*injectionNum)));
	}


};
};

#endif // FLOWRENDERER_H
