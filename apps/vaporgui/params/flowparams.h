//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:  Definition of FlowParams class 
//		supports all the parameters needed for an Flow renderer
//

#ifndef FLOWPARAMS_H
#define FLOWPARAMS_H

#include <qwidget.h>
#include "params.h"

class FlowTab;

namespace VAPoR {
class MapperFunction;

class VaporFlow;
class ExpatParseMgr;
class MainForm;
class FlowMapEditor;
class FlowParams: public Params {
	
public: 
	FlowParams(int winnum);
	
	~FlowParams();

	
	void setTab(FlowTab* tab); 
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	//implement change of enablement, or change of local/global
	//
	virtual void updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow);
	virtual void updateMapBounds();
	virtual void guiStartChangeMapFcn(char* );
	virtual void guiEndChangeMapFcn();
	
	
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();

	// Reinitialize due to new Session:
	void reinit(bool doOverride);
	//Override default set-enabled to create/destroy FlowLib
	virtual void setEnabled(bool value);
	
	//Methods that record changes in the history:
	//
	virtual void guiSetEnabled(bool);

	//Save, restore stuff:
	XmlNode* buildNode(); 
	virtual bool elementStartHandler(ExpatParseMgr*, int  depth , std::string& tag, const char ** attribs);
	virtual bool elementEndHandler(ExpatParseMgr*, int depth , std::string& tag);
	
	//set geometry/flow data dirty-flags, and force rebuilding all geometry
	void setFlowMappingDirty();
	void setFlowDataDirty();
	//The mapper function calls this when the mapping changes
	virtual void setClutDirty() { setFlowMappingDirty();}
	
	
	int getNumGenerators(int dimNum) { return generatorCount[dimNum];}
	int getTotalNumGenerators() { return allGeneratorCount;}
	VaporFlow* getFlowLib(){return myFlowLib;}
	float* regenerateFlowData(int frameNum);
	//Get an array of rgba's (valid immediately after calling regenerateFlowData)
	//Must test for a valid flowData array first
	float* getRGBAs(int frameNum); 

	//support for seed region manipulation:
	//If a face is selected, this value is >= 0:
	//
	int getSelectedFaceNum() {return selectedFaceNum;}
	//While sliding face, the faceDisplacement indicates how far selected face is 
	//moved.
	float getSeedFaceDisplacement() {return faceDisplacement;}
	//
	//Start to slide a region face.  Need to save direction vector
	//
	void captureMouseDown(int faceNum, float camPos[3], float dirVec[]);
	//When the mouse goes up, save the face displacement into the region.
	void captureMouseUp();
	//Intersect the ray with the specified face, determining the intersection
	//in world coordinates.  Note that meaning of faceNum is specified in 
	//renderer.h
	// Faces of the cube are numbered 0..5 based on view from pos z axis:
	// back, front, bottom, top, left, right
	//
	bool rayCubeIntersect(float ray[3], float cameraPos[3], int faceNum, float intersect[3]);
	//Slide the face based on mouse move from previous capture.  
	//Requires new direction vector associated with current mouse position
	void slideCubeFace(float movedRay[3]);
	//Indicate we are currently dragging a cube face:
	bool draggingFace() {return (selectedFaceNum >= 0);}

	void calcSeedExtents(float *extents);
	float getSeedRegionMin(int coord){ return seedBoxMin[coord];}
	float getSeedRegionMax(int coord){ return seedBoxMax[coord];}
	int getMinFrame() {return minFrame;}
	int getMaxFrame() {return maxFrame;}
	int getMaxPoints() {return maxPoints;}
	int getNumSeedPoints() { return numSeedPoints;}
	int getNumInjections() { return numInjections;}
	int getFirstDisplayFrame() {return firstDisplayFrame;}
	int getLastDisplayFrame() {return lastDisplayFrame;}
	int getStartFrame() {return seedTimeStart;}
	int getLastSeeding() {return seedTimeEnd;}
	int getSeedingIncrement() {return seedTimeIncrement;}
	float getShapeDiameter() {return shapeDiameter;}
	int getColorMapEntityIndex() ;
	int getOpacMapEntityIndex() ;
	bool flowIsSteady() {return (flowType == 0);} // 0= steady, 1 = unsteady
	bool flowDataIsDirty(int timeStep){
		if(flowData && flowData[timeStep]) return false;
		return true;
	}
	bool flowMappingIsDirty(int timeStep) {
		if (flowRGBAs && flowRGBAs[timeStep]) return false;
		return true;
	}
	float* getFlowData(int timeStep){
		return flowData[timeStep];
	}
	int getShapeType() {return geometryType;} //0 = tube, 1 = point, 2 = arrow
	float getObjectsPerTimestep() {return objectsPerTimestep;}
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetAligned();

	
	void setEditMode(bool mode) {editMode = mode;}
	bool getEditMode() {return editMode;}
	virtual MapperFunction* getMapperFunc() {return mapperFunction;}
	//Methods called from vizwinmgr due to settings in gui:
	void guiSetFlowType(int typenum);
	void guiSetNumTrans(int numtrans);
	void guiSetXVarNum(int varnum);
	void guiSetYVarNum(int varnum);
	void guiSetZVarNum(int varnum);
	void guiRecalc();
	void guiSetRandom(bool rand);
	void guiSetGeneratorDimension(int dimNum);
	void guiSetXCenter(int sliderval);
	void guiSetYCenter(int sliderval);
	void guiSetZCenter(int sliderval);
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiSetFlowGeometry(int geomNum);
	void guiSetColorMapEntity( int entityNum);
	void guiSetOpacMapEntity( int entityNum);


protected:
	//Tags for attributes in session save
	//Top level labels
	static const string _mappedVariablesAttr;
	static const string _steadyFlowAttr;
	static const string _instanceAttr;
	static const string _numTransformsAttr;
	static const string _integrationAccuracyAttr;
	static const string _userTimeStepMultAttr;
	static const string _timeSamplingIntervalAttr;

	//flow seeding tags and attributss
	static const string _seedingTag;
	static const string _seedRegionMinAttr;
	static const string _seedRegionMaxAttr;
	static const string _randomGenAttr;
	static const string _randomSeedAttr;
	static const string _generatorCountsAttr;
	static const string _totalGeneratorCountAttr;
	static const string _seedTimesAttr;

	//Geometry attributes
	static const string _geometryTag;
	static const string _geometryTypeAttr;
	static const string _objectsPerTimestepAttr;
	static const string _displayIntervalAttr;
	static const string _shapeDiameterAttr;
	static const string _colorMappedEntityAttr;
	static const string _colorMappingBoundsAttr;
	static const string _hsvAttr;
	static const string _positionAttr;
	static const string _colorControlPointTag;
	static const string _numControlPointsAttr;

	void setFlowType(int typenum){flowType = typenum; setFlowDataDirty();}
	void setNumTrans(int numtrans){numTransforms = numtrans; setFlowDataDirty();}
	void setMaxNumTrans(int maxNT) {maxNumTrans = maxNT;}
	void setMinNumTrans(int minNT) {minNumTrans = minNT;}
	void setXVarNum(int varnum){varNum[0] = varnum; setFlowDataDirty();}
	void setYVarNum(int varnum){varNum[1] = varnum; setFlowDataDirty();}
	void setZVarNum(int varnum){varNum[2] = varnum; setFlowDataDirty();}
	void setRandom(bool rand){randomGen = rand; setFlowDataDirty();}
	void setXCenter(int sliderval);
	void setYCenter(int sliderval);
	void setZCenter(int sliderval);
	void setXSize(int sliderval);
	void setYSize(int sliderval);
	void setZSize(int sliderval);
	void setFlowGeometry(int geomNum){geometryType = geomNum;}
	void setColorMapEntity( int entityNum);
	void setOpacMapEntity( int entityNum);
	
	void setCurrentDimension(int dimNum) {currentDimension = dimNum;}
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	//Methods to make sliders and text consistent for seed region:
	void textToSlider(int coord, float center, float size);
	void sliderToText(int coord, int center, int size);
	
	void mapColors(float* speeds, int timeStep);

	void setSeedRegionMax(int coord, float val){
		seedBoxMax[coord] = val;
	}
	void setSeedRegionMin(int coord, float val){
		seedBoxMin[coord] = val;
	}
	//Force seed box to be inside region
	//Return true if anything changed.
	bool enforceConsistency(int coord);
	FlowTab* myFlowTab;
	int flowType; //steady = 0, unsteady = 1;
	int instance;
	int numTransforms, maxNumTrans, minNumTrans;
	int numVariables;
	std::vector<std::string> variableNames;
	int varNum[3]; //field variable num's in x, y, and z.
	float integrationAccuracy;
	float userTimeStepMultiplier;
	int timeSamplingInterval;
	bool editMode;
	
	bool randomGen;
	
	
	unsigned int randomSeed;
	float seedBoxMin[3], seedBoxMax[3];
	size_t generatorCount[3];
	size_t allGeneratorCount;
	int seedTimeStart, seedTimeEnd, seedTimeIncrement;
	int currentDimension;//Which dimension is showing in generator count

	int geometryType;  //0= tube, 1=point, 2 = arrow
	float objectsPerTimestep;
	int firstDisplayFrame, lastDisplayFrame;
	
	float shapeDiameter;
	
	
	PanelCommand* savedCommand;
	MapperFunction* mapperFunction;
	FlowMapEditor* flowMapEditor;
	FlowMapEditor* getFlowMapEditor() {return flowMapEditor;}

	int maxPoints;  //largest length of any flow
	
	
	std::vector<string> colorMapEntity;
	std::vector<string> opacMapEntity;
	int numControlPoints;
	
	VaporFlow* myFlowLib;

	//Arrays to hold data associated with a flow.
	//The flowData comes from the flowLib, potentially with speeds 
	//These are mapped to flowRGBs and speeds are released
	//after the data is obtained.
	//There is potentially one array for each timestep (with streamlines)
	//With streaklines, there is one array, flowData[0].
	float** flowData;
	float** flowRGBAs;

	
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	
	int numSeedPoints;
	int numInjections;
	
	//Keep track of min, max frames in available data:
	int maxFrame, minFrame;
	//for seed region manipulation:
	int selectedFaceNum;
	float faceDisplacement;
	float initialSelectionRay[3];

};
};
#endif //FLOWPARAMS_H 
