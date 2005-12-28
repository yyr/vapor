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
#include <qcolor.h>
#include "params.h"
#include "glutil.h"

class FlowTab;
#define IGNORE_FLAG 2.e30f
namespace VAPoR {
class MapperFunction;

class VaporFlow;
class ExpatParseMgr;
class MainForm;
class FlowMapEditor;
class VECTOR4;
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
	void setFlowDataDirty(bool rakeOnly = false, bool listOnly = false);
	//Precise setting of flag value and/or invalidate data
	void setFlowDataDirty(int timeStep, bool isRake, bool newValue);
		
	//The mapper function calls this when the mapping changes
	virtual void setClutDirty() { setFlowMappingDirty();}

	//Virtual methods to set map bounds.  Get() is in parent class
	//Needed here because the map bounds are saved in params class for each mapped variable
	virtual void setMinColorMapBound(float val);
	virtual void setMaxColorMapBound(float val);
	virtual void setMinOpacMapBound(float val);
	virtual void setMaxOpacMapBound(float val);

	virtual void restart();
	
	int getNumGenerators(int dimNum) { return generatorCount[dimNum];}
	int getTotalNumGenerators() { return allGeneratorCount;}
	VaporFlow* getFlowLib(){return myFlowLib;}
	float* regenerateFlowData(int frameNum, bool fromRake);
	//Get an array of rgba's (valid immediately after calling regenerateFlowData)
	//Must test for a valid flowData array first
	float* getRGBAs(int frameNum, bool isRake); 

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
	virtual void captureMouseUp();
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
	
	int getNumInjections() { return numInjections;}
	int getFirstDisplayAge() {return firstDisplayFrame;}
	int getLastDisplayAge() {return lastDisplayFrame;}
	int getSeedStartFrame() {return seedTimeStart;}
	int getLastSeeding() {return seedTimeEnd;}
	int getSeedingIncrement() {return seedTimeIncrement;}
	float getShapeDiameter() {return shapeDiameter;}
	//Distinguish between the number of seed points specified in the
	//current settings and the number actually used the last time
	//the flow was calculated:
	//
	int getNumRakeSeedPointsUsed() {return numRakeSeedPointsUsed;}
	int getNumRakeSeedPoints(){
		return (randomGen ? allGeneratorCount : generatorCount[0]*generatorCount[1]*generatorCount[2]);
	}
	int getNumListSeedPoints() {return seedPointList.size();}
	int getNumListSeedPointsUsed() {return numListSeedPointsUsed;}
	std::vector<Point4>& getSeedPointList(){return seedPointList;}
	int getColorMapEntityIndex() ;
	int getOpacMapEntityIndex() ;
	bool flowIsSteady() {return (flowType == 0);} // 0= steady, 1 = unsteady
	bool flowDataIsDirty(int timeStep, bool rakeOnly, bool listOnly){
		bool rakeDirty = 
			(rakeFlowData && (flowDataDirty[timeStep]&seedRake));
		bool listDirty = 
			(listFlowData && (flowDataDirty[timeStep]&seedList)&&getNumListSeedPoints()>0);
		if (rakeOnly) return rakeDirty;
		if (listOnly) return listDirty;
		return (rakeDirty || listDirty);
	}
	bool flowDataIsDirty(int timeStep){
		if (doRake && flowDataIsDirty(timeStep, true, false)) return true; 
		if (doSeedList && flowDataIsDirty(timeStep, false, true)) return true; 
		return false;
	}
	bool activeFlowDataIsDirty();
	//Stronger than setting the dirty flag, used user clicks "refresh",
	//resulting in deletion of flow data array.
	//
	void invalidateFlowData(int timeStep){
		if (doRake){
			if(rakeFlowData && rakeFlowData[timeStep]){ 
				delete rakeFlowData[timeStep];
				rakeFlowData[timeStep] = 0;
			}
		}
		if (doSeedList){
			if(listFlowData && listFlowData[timeStep]){ 
				delete listFlowData[timeStep];
				listFlowData[timeStep] = 0;
			}
		}
	}
	float* getFlowData(int timeStep, bool isRake){
		if (isRake){
			if (!rakeFlowData) return 0;
			return rakeFlowData[timeStep];
		} else {
			if (!listFlowData) return 0;
			return listFlowData[timeStep];
		}
	}
	float getConstantOpacity() {return constantOpacity;}
	QRgb getConstantColor() {return constantColor;}
	int getShapeType() {return geometryType;} //0 = tube, 1 = point, 2 = arrow
	int getObjectsPerFlowline() {return objectsPerFlowline;}
	bool rakeEnabled() {return doRake;}
	bool listEnabled() {return (doSeedList && getNumListSeedPoints() > 0);}
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
	void guiSetConstantColor(QColor& newColor);
	void guiSetGeomSamples(int sliderVal);
	void guiSetAutoRefresh(bool isOn);
	void guiSetRakeToRegion();
	void guiRefreshFlow();
	void guiCenterRake(float* coords);
	void guiAddSeed(Point4 pt);
	void guiMoveLastSeed(float coords[3]);
	void guiDoSeedList(bool isOn);
	void guiDoRake(bool isOn);
	void guiEditSeedList();


	void setMapBoundsChanged(bool on){mapBoundsChanged = on; flowGraphicsChanged = on;}
	void setFlowDataChanged(bool on){flowDataChanged = on;}
	void setFlowGraphicsChanged(bool on){flowGraphicsChanged = on;}


	virtual void getBox(float boxmin[], float boxmax[]){
		for (int i = 0; i< 3; i++){
			boxmin[i] = seedBoxMin[i];
			boxmax[i] = seedBoxMax[i];
		}
	}
	virtual void setBox(const float boxMin[], const float boxMax[]){
		for (int i = 0; i< 3; i++){
			seedBoxMin[i] = boxMin[i];
			seedBoxMax[i] = boxMax[i];
		}
	}
	float getListSeedPoint(int i, int coord){
		return seedPointList[i].getVal(coord);
	}
protected:
	//Tags for attributes in session save
	//Top level labels
	static const string _mappedVariablesAttr;
	static const string _steadyFlowAttr;
	static const string _instanceAttr;
	static const string _numTransformsAttr;
	static const string _integrationAccuracyAttr;
	static const string _velocityScaleAttr;
	static const string _timeSamplingAttr;
	static const string _autoRefreshAttr;

	//flow seeding tags and attributes
	static const string _seedingTag;
	static const string _seedRegionMinAttr;
	static const string _seedRegionMaxAttr;
	static const string _randomGenAttr;
	static const string _randomSeedAttr;
	static const string _generatorCountsAttr;
	static const string _totalGeneratorCountAttr;
	static const string _seedTimesAttr;
	static const string _useRakeAttr;
	static const string _useSeedListAttr;
	static const string _seedPointTag;
	static const string _positionAttr;
	static const string _timestepAttr;

	//Geometry attributes
	static const string _geometryTag;
	static const string _geometryTypeAttr;
	static const string _objectsPerFlowlineAttr;
	static const string _displayIntervalAttr;
	static const string _shapeDiameterAttr;
	static const string _colorMappedEntityAttr;
	static const string _opacityMappedEntityAttr;
	static const string _constantColorAttr;
	static const string _constantOpacityAttr;

	//Mapping bounds, variable names (for all variables, mapped or not) are in variableMapping node
	
	static const string _leftColorBoundAttr;
	static const string _rightColorBoundAttr;
	static const string _leftOpacityBoundAttr;
	static const string _rightOpacityBoundAttr;
	
	void setFlowType(int typenum){flowType = typenum; setFlowDataDirty();}
	void setNumTrans(int numtrans){numTransforms = numtrans; setFlowDataDirty();}
	void setMaxNumTrans(int maxNT) {maxNumTrans = maxNT;}
	void setMinNumTrans(int minNT) {minNumTrans = minNT;}
	void setXVarNum(int varnum){varNum[0] = varnum; setFlowDataDirty();}
	void setYVarNum(int varnum){varNum[1] = varnum; setFlowDataDirty();}
	void setZVarNum(int varnum){varNum[2] = varnum; setFlowDataDirty();}
	void setRandom(bool rand){randomGen = rand; setFlowDataDirty(true);}
	void setXCenter(int sliderval);
	void setYCenter(int sliderval);
	void setZCenter(int sliderval);
	void setXSize(int sliderval);
	void setYSize(int sliderval);
	void setZSize(int sliderval);
	void setFlowGeometry(int geomNum){geometryType = geomNum;}
	void setColorMapEntity( int entityNum);
	void setOpacMapEntity( int entityNum);

	//Calculate max and min range for variables based on current data/flow settings
	//Only used for guidance in setting info in flowtab
	float minRange(int varNum);
	float maxRange(int varNum);
	
	void setCurrentDimension(int dimNum) {currentDimension = dimNum;}
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	//Methods to make sliders and text consistent for seed region:
	void textToSlider(int coord, float center, float size);
	void sliderToText(int coord, int center, int size);
	
	void mapColors(float* speeds, int timeStep, int numSeeds, float** flowData, float **rgbas, bool isRake);

	void setSeedRegionMax(int coord, float val){
		seedBoxMax[coord] = val;
	}
	void setSeedRegionMin(int coord, float val){
		seedBoxMin[coord] = val;
	}
	
	
	//Check the variables in the flow data for missing timesteps 
	//Independent of animation params
	//Provide an info message if data does not match input request
	//Return false if anything changed
	bool validateSampling();
	//check if vector field is present for a timestep
	bool validateVectorField(int timestep);

	void cleanFlowDataCaches();

	enum seedType { //dirty flags identify which kind of seeds are used:
		nullSeedType = 0,
		seedList = 1,
		seedRake = 2
	}; 

	FlowTab* myFlowTab;
	int flowType; //steady = 0, unsteady = 1;
	int instance;
	int numTransforms, maxNumTrans, minNumTrans;
	int numVariables;
	std::vector<std::string> variableNames;
	int varNum[3]; //field variable num's in x, y, and z.
	float integrationAccuracy;
	float velocityScale;
	int timeSamplingInterval;
	int timeSamplingStart;
	int timeSamplingEnd;

	//Flags to know what has changed when text changes:
	bool flowDataChanged;
	bool flowGraphicsChanged;
	bool mapBoundsChanged;

	bool editMode;
	
	bool randomGen;
	
	
	unsigned int randomSeed;
	float seedBoxMin[3], seedBoxMax[3];
	
	size_t generatorCount[3];
	size_t allGeneratorCount;
	int seedTimeStart, seedTimeEnd, seedTimeIncrement;
	int currentDimension;//Which dimension is showing in generator count

	int geometryType;  //0= tube, 1=point, 2 = arrow
	int objectsPerFlowline;
	int firstDisplayFrame, lastDisplayFrame;
	
	float shapeDiameter;
	QRgb constantColor;
	float constantOpacity;
	
	
	PanelCommand* savedCommand;
	MapperFunction* mapperFunction;
	FlowMapEditor* flowMapEditor;
	FlowMapEditor* getFlowMapEditor() {return flowMapEditor;}
	//Save the min and max bounds for each of the flow mappings, plus 
	//min and max for each variable
	float* minOpacBounds;
	float* maxOpacBounds;
	float* minColorBounds;
	float* maxColorBounds;
	
	int maxPoints;  //largest length of any flow
	
	
	std::vector<string> colorMapEntity;
	std::vector<string> opacMapEntity;

	std::vector<Point4> seedPointList;
	
	
	VaporFlow* myFlowLib;

	//Arrays to hold data associated with a flow.
	//The flowData comes from the flowLib, potentially with speeds 
	//These are mapped to flowRGBs and speeds are released
	//after the data is obtained.
	//There is potentially one array for each timestep (with streamlines)
	//With Pathlines, there is one array, rakeFlowData[0].
	float** rakeFlowData;
	float** listFlowData;
	seedType* flowDataDirty;
	float** rakeFlowRGBAs;
	float** listFlowRGBAs;
	bool autoRefresh;

	bool doRake;
	bool doSeedList;

	
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	
	int numRakeSeedPointsUsed;
	int numListSeedPointsUsed;
	int numInjections;
	
	//Keep track of min, max frames in available data:
	int maxFrame, minFrame;
	//for seed region manipulation:
	int selectedFaceNum;
	float faceDisplacement;
	float initialSelectionRay[3];

	
	//The following is to support asynchronous Pathline calculation.
	//There is a semaphore controlling access to the state variables, which include:
	///////////////////////////
	//  int lastCompletedTimestep //indicates how many valid timesteps are in the data
	//								//note that this is -1 is equivalent to dataDirty = true
	//  enum flowCalcState  // can be idle, busy, or ending
	//////////////////////////////
	// When flowDataDirty is set, if Pathlines are being done, then instead of deleting
	// the flowdata, 
	//		if flowCalcState is busy it is set to "ending"
	//		if flowCalcState is idle, lastCompletedTimestep is set to -1
	///////////////////////////
	// When the flowlib completes a timestep, 
	//	if flowCalcState is ending, flowcalcstate is set to idle and lastCompletedTimestep to -1, and
	//	a rerender is requested, and the calculation is stopped.
	//  if flowCalcState is not ending, the lastCompletedStep is incremented. and rerender is requested
	//  flowCalcState should not be idle
	/////////////////////////////////////////
	// When a new render begins:
	//	 if lastCompletedTimestep is -1, regen is called, plus:
	//      the flowdata is reallocated
	//		flowCalcState is set to busy (it should already be idle)
	//   otherwise the valid frames are rendered
	///////////////////////////////////////////////////
	//  question: do we always need to lock the semaphore???
	////////////////////////////////////////////////////////////////////////
	//  FOllowing is probably wrong::::::>>>>>>>
	//		When a new Pathline calculation is started, set
	// isEnding = false, isWorking = true, needData = false, lastCompletedTimestep = -1
	//		When flowlib completes a timestep:
	// set lastCompletedTimestep to the correct value
	// request GLUpdate of visualizer
	// check the isEnding flag.  If isEnding==true, set isWorking = false and 
	// return false (cancelling further Path calculations)
	//		When users set data dirty (necessitating a new Pathline calc):
	// set isEnding = true
	// set needData = true
	//		When there is a new rendering:
	// If the dataDirty flag is true, then perform the above steps for starting a Pathline calc
	//	  perform the above steps f
	// check needData (don't need to get a lock for this)
	//////////////////////////////////// 

};
};
#endif //FLOWPARAMS_H 
