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
#include "flowparams.h"
namespace VAPoR {
//class VizWin;
//enum DirtyBitType;
class RENDER_API FlowRenderer : public Renderer
{
    Q_OBJECT

public:

    FlowRenderer( GLWindow* , FlowParams* );
    ~FlowRenderer();
	
	virtual void		initializeGL();
    virtual void		paintGL();

	//Methods to access and to rebuild the flow data cache.
	//get...used() methods specify the number of seed points associated
	//with the current data in the cache.
	int getNumRakeSeedPointsUsed() {return numRakeSeedPointsUsed;}
	int getNumListSeedPointsUsed(int timeStep) {return numListSeedPointsUsed[timeStep];}
	

	//Obtain the current flow data.  
	float* getFlowData(int timeStep, bool isRake){
		if (isRake){
			return rakeFlowData[timeStep];
		} else {
			return listFlowData[timeStep];
		}
	}
	//Obtain the current flow data.  
	float* getRGBAs(int timeStep, bool isRake){
		if (isRake){
			return rakeFlowRGBAs[timeStep];
		} else {
			return listFlowRGBAs[timeStep];
		}
	}

	//The data dirty and needsRefresh flags are used to control updates 
	//The data dirty indicates whether or not the current flow data is valid.
	//The needsRefresh flag indicates whether or not invalid data should be
	//recalculated (based on the autorefresh setting)
	//When users click "refresh", all dirty data turns on the needsRefresh flag.
	//   clean data will keep the needsrefresh off, so that when the next
	//   dirty is set, they won't be refreshed.
	//When users click "auto off", all needsRefresh flags are turned off.
	//When users click "auto on", all needsRefresh flags are turned on.
	//When data is reconstructed, the appropriate dirty flag is turned off.  The
	//	needsRefresh flag is left on.
	//When something changes, all dirty flags are turned on.  
	//	If auto is on, needsrefresh is also turned on.
	//  If auto is off, needsRefresh remains as is
	//At render time, data is reconstructed if both needsRefresh and dirty are true.

	void setNeedOfRefresh(int timeStep, bool value){ if(needRefreshFlag)needRefreshFlag[timeStep]=value;}
	bool needsRefresh(FlowParams* fParams, int timeStep) {return (fParams->refreshIsAuto() || !needRefreshFlag || needRefreshFlag[timeStep]);}
	void setAllNeedRefresh(bool value);
	bool flowDataIsDirty(int timeStep){return (flowDataDirty && flowDataDirty[timeStep]);}
	
	
	//clean flag (after rebuild data), also turn off needRefresh
	void setFlowDataClean(int timeStep, bool isRake);

	//When a bit is set dirty, must set appropriate bits for 
	//all timesteps.
	void setDataDirty();
	void setGraphicsDirty();
	void setRegionDirty(bool dirty = true) {regionDirty = dirty;}
	bool regionIsDirty() {return regionDirty;}

	bool rebuildFlowData(int timeStep, bool doRake);
	void setRegionValid(bool trueFalse) {regionIsValid = trueFalse;}
	bool regionValid() {return regionIsValid;}


protected:

	void calcPeriodicExtents();
	int numFrames;
			
	//Render geometry using the current values of flowDataArray, flowRGBAs
	void renderFlowData(bool constColors,int currentFrameNum);

	//OR do it with FlowLineData:
	void renderFlowData(FlowLineData*,bool constColors, int currentFrameNum);
	void renderTubes(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderCurves(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderPoints(float radius, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderArrows(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderTubes(FlowLineData*, float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderCurves(FlowLineData*, float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderPoints(FlowLineData*, float radius, int firstAge, int lastAge, int startIndex, bool constMap);
	void renderArrows(FlowLineData*, float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap);
	
	/*
	float* getFlowPoint(int timeStep, int seedNum, int injectionNum){
		assert ((3*(timeStep+ maxPoints*(seedNum+ numSeedPoints*injectionNum)))<
			3*maxPoints*numSeedPoints*numInjections );
		return (flowDataArray+ 3*(timeStep+ maxPoints*(seedNum+ numSeedPoints*injectionNum)));
	}
	*/
	//convert original coordinates to lie inside central cycle.  Identify the cycle it's in
	bool mapPeriodicCycle(float origCoord[3], float mappedCoord[3], int oldcycle[3], int newcycle[3]);
	// Render a "stationary symbol" at the specified point
	void renderStationary(float* point);
	//draw one arrow
	void drawArrow(bool isLit, float* firstColor, float* firstPoint, float* endPoint, float* dirVec, float* bVec, 
		float* UVec, float radius, bool constMap);
	// draw one cylinder of tube
	void drawTube(bool isLit, float* secondColor, float* startPoint, float* endPoint, float* currentB, float* currentU, 
		float radius, bool constMap, float* prevNormal, float* prevVertex, float* currentNormal, float* currentVertex);

	//Constants that are used, recalculated in each rendering:
	float constFlowColor[4];
	float arrowHeadRadius;
	float voxelSize;
	float stationaryRadius;

	//Flow and geometry caches:
	//Arrays to hold data associated with a flow.
	//The flowData comes from the flowLib, potentially with speeds 
	//These are mapped to flowRGBs and speeds are released
	//after the data is obtained.
	//There is potentially one array for each timestep (with streamlines)
	//With Pathlines, there is one array, rakeFlowData[0].
	float** rakeFlowData;
	float** listFlowData;
	//Array of pointers to FlowLineData containers
	FlowLineData** flowLineRakeData;
	FlowLineData** flowLineListData;
	//remember the number of seeds in list that are used
	int* numListSeedPointsUsed;
	//One dirty flag for each time-step.  All are set true when
	//setDirty is called.
	bool* flowDataDirty;
	//need refresh flag, when false means that we won't refresh (for !autorefresh)
	//One flag for each timestep, in order to handle situations where auto
	//refresh is clicked and we need to remember that some but not all timesteps
	//are in need of refresh.
	bool* needRefreshFlag;
	//graphicsDirty flag, forces reconstruction of RGBAs.  
	//Only important if flowDataDirty is false.
	bool* flowMapDirty;
	float** rakeFlowRGBAs;
	float** listFlowRGBAs;
	//These are established each time
	//The flow data cache is regenerated:
	int numRakeSeedPointsUsed;
	//Following parameters are obtained from the flowparams at the
	//time the data cache is created.  They are needed to render
	//the contents of the cache.
	int numInjections;
	int maxFrame, minFrame;
	bool steadyFlow;
	int seedIncrement, startSeed, endSeed, firstDisplayAge, lastDisplayAge;
	float objectsPerTimestep;
	bool doList, doRake;
	float* flowDataArray;
	float* flowRGBAs;
	int maxPoints;
	int numSeedPoints;
	bool regionIsValid;
	bool regionDirty;

	//FlowParams* myFlowParams;
	//Remember the last time step that was rendered, for capturing.
	int lastTimeStep;
	//save the periodic extents (slightly different than extents)
	float periodicExtents[6];
	VaporFlow* myFlowLib;



};
};

#endif // FLOWRENDERER_H
