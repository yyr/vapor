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
#include <vapor/common.h>
class FlowTab;
#define IGNORE_FLAG 2.e30f
namespace VAPoR {
class MapperFunction;
class VaporFlow;
class ExpatParseMgr;
class VECTOR4;
class RegionParams;
class FlowLineData;
class PathLineData;
class PARAMS_API FlowParams: public RenderParams {
	
public: 
	FlowParams(int winnum);
	
	~FlowParams();
	static ParamsBase* CreateDefaultInstance() {return new FlowParams(-1);}
	const std::string& getShortName() {return _shortName;}
	
	void setTab(FlowTab* tab); 
	
	virtual Params* deepCopy(ParamNode* =0);

	virtual bool usingVariable(const string& varname);
	
	virtual int GetCompressionLevel() {return compressionLevel;}
	virtual void SetCompressionLevel(int val) {compressionLevel = val; }

	// Reinitialize due to new Session:
	bool reinit(bool doOverride);
	
	//Check that the current settings are OK for rebuilding the flow at timestep
	//Issue error/warning messages as appropriate.  Return false on error.
	bool validateSettings(int timestep);

	//Save, restore stuff:
	ParamNode* buildNode(); 
	virtual bool elementStartHandler(ExpatParseMgr*, int  depth , std::string& tag, const char ** attribs);
	virtual bool elementEndHandler(ExpatParseMgr*, int depth , std::string& tag);
	
	
	
	//Virtual methods to set map bounds.  Get() is in parent class
	//Needed here because the map bounds are saved in params class for each mapped variable
	virtual void setMinColorMapBound(float val);
	virtual void setMaxColorMapBound(float val);
	virtual void setMinOpacMapBound(float val);
	virtual void setMaxOpacMapBound(float val);

	virtual void restart();
	static void setDefaultPrefs();
	int getNumFLASamples() {return numFLASamples;}
	void setNumFLASamples(int numSamp){numFLASamples = numSamp;}
	bool getFLAAdvectBeforePrioritize(){return flaAdvectBeforePrioritize;}
	void setFLAAdvectBeforePrioritize(bool val){flaAdvectBeforePrioritize = val;}
	int getNumGenerators(int dimNum) { return (int)generatorCount[dimNum];}
	void setNumGenerators(int dimNum, int val){generatorCount[dimNum]=val;}
	int getTotalNumGenerators() { return (int)allGeneratorCount;}
	void setTotalNumGenerators(int val){allGeneratorCount = val;}
	
	//Following will be replaced by regenerateSteadyFieldLines
	//FlowLineData* regenerateSteadyFlowData(VaporFlow* , int timeStep, int minFrame, RegionParams*);
	
	//To rebuild the PathLineData associated with field line advection, need to do the following:
	//1.  Create a new PathLineData with the starting seeds in it:
	FlowLineData* setupUnsteadyStartData(VaporFlow*, int minFrame, int maxFrame, RegionParams* rParams);
	bool setupFlowRegion(RegionParams* rParams, VaporFlow* flowLib, int timeStep);

	//2.  Call GenStreamLines for the seed time, prioritizing the seeds, and putting the
	//		resulting FlowLineData into the cache.  Done by calling regenerateSteadyFlowData.
	//3.  For each sample time between the seed time and the current time, and one more, if
	//		the current time is not a seed time: 
	//	3a. Extend the PathLineData to the next sample time
	//	3b. Call GenStreamLines, generating a new FlowLineData, and prioritizing seeds at
	//		the next sample time.  (These FlowLineData's should be put in the cache)
	//4.  If the currentTime is not a sample time, call GenStreamLines to create a 
	//		flowLineData for the current time, but not prioritizing seeds.
	//  The following methods support this process:
	
	//  For steps 2, 3b, and 4
	//If the flowLineData is non-null, the seeds are taken from it.
	//else if the pathlineData is non-null, it provides the seeds.
	// else If the pathlinedata is null, we get the seeds from the rake or seedlist:
	FlowLineData* regenerateSteadyFieldLines(VaporFlow*, FlowLineData*, PathLineData*, int timeStep, int minFrame, RegionParams* rParams, bool prioritize);
	//  For step 3a use VaporFlow::ExtendPathLine()

	//Method that advects multiple points, then maximizes them at next timeStep.
	//Results go in steadyFlowCache at appropriate time steps.
	bool multiAdvectFieldLines(VaporFlow*, FlowLineData**, int startTime, int endTime, int minFrame, RegionParams*);

	bool singleAdvectFieldLines(VaporFlow*, FlowLineData**, PathLineData*, int startTime, int endTime, int minFrame, RegionParams*);
	//Methods to allow iteration over unsteady timestep samples in either direction,
	//within a prescribed min/max interval.  Returns -1 at end of interval 
	int getUnsteadyTimestepSample(int index, int minStep, int maxStep, int flowDir);
	int getTimeSampleIndex(int ts, int minStep, int maxStep);

	//void calcSeedExtents(float *extents);
	float getSeedRegionMin(int coord){ return seedBoxMin[coord];}
	float getSeedRegionMax(int coord){ return seedBoxMax[coord];}
	
	int calcMaxPoints();
	int calcNumSeedPoints(int timeStep);
	int getFirstDisplayFrame() {return firstDisplayFrame;}
	int getLastDisplayFrame() {return lastDisplayFrame;}
	void setFirstDisplayFrame(int val) {firstDisplayFrame = val;}
	void setLastDisplayFrame(int val) {lastDisplayFrame = val;}
	int getSeedTimeStart() {return seedTimeStart;}
	void setSeedTimeStart(int val){seedTimeStart = val;}
	int getSeedTimeEnd() {return seedTimeEnd;}
	void setSeedTimeEnd(int val) {seedTimeEnd = val;}
	int getSeedTimeIncrement() {return seedTimeIncrement;}
	void setSeedTimeIncrement(int val){seedTimeIncrement = val;}
	float getShapeDiameter() {return shapeDiameter;}
	float getDiamondDiameter() {return diamondDiameter;}
	float getArrowDiameter() {return arrowDiameter;}
	void setShapeDiameter(float val) {shapeDiameter=val;}
	void setDiamondDiameter(float val) {diamondDiameter=val;}
	void setArrowDiameter(float val) {arrowDiameter=val;}
	int getTimeSamplingInterval() {return timeSamplingInterval;}
	void setTimeSamplingInterval(int val){timeSamplingInterval = val;}
	int getTimeSamplingStart() {return timeSamplingStart;}
	void setTimeSamplingStart(int val){timeSamplingStart = val;}
	int getTimeSamplingEnd() {return timeSamplingEnd;}
	void setTimeSamplingEnd(int val){timeSamplingEnd = val;}
	void enableDisplayLists(bool val){useDisplayLists = val;}
	bool usingDisplayLists(){return useDisplayLists;}
	int getNumTimestepSamples(){
		if (usingTimestepSampleList())
			return (int)unsteadyTimestepList.size();
		else return (1+(timeSamplingEnd - timeSamplingStart)/timeSamplingInterval);
	}
	int getTimestepSample(int n) {
		if (usingTimestepSampleList())
			return unsteadyTimestepList[n];
		else return (timeSamplingStart+n*timeSamplingInterval);
	}
	int getFirstSampleTimestep() {
		if (usingTimestepSampleList() && unsteadyTimestepList.size()>0) return unsteadyTimestepList[0];
		else return timeSamplingStart;
	}
	int getLastSampleTimestep() {
		if (usingTimestepSampleList() && unsteadyTimestepList.size()>0) return unsteadyTimestepList[unsteadyTimestepList.size()-1];
		else return timeSamplingEnd;
	}
	bool isAutoScale(){return autoScale;}
	void setAutoScale(bool val){autoScale = val;}
	
	//Distinguish between the number of seed points specified in the
	//current settings and the number actually used the last time
	//the flow was calculated:
	//
	bool usingTimestepSampleList() {return useTimestepSampleList;}
	void setTimestepSampleList(bool on) {useTimestepSampleList = on;}
	
	int getNumRakeSeedPoints(){
		return (int)(randomGen ? allGeneratorCount : (int)(generatorCount[0]*generatorCount[1]*generatorCount[2]));
	}
	int getNumListSeedPoints() {return (int)seedPointList.size();}
	float* getRakeSeeds(RegionParams* rParams, int* numseeds, int steadyTimestep);
	std::vector<Point4>& getSeedPointList(){return seedPointList;}
	void pushSeed(Point4& newSeed){seedPointList.push_back(newSeed);}
	void moveLastSeed(const float* newCoords){
		if (seedPointList.size() > 0)
			seedPointList[seedPointList.size()-1].set3Val(newCoords);
	}
	void emptySeedList(){seedPointList.clear();}
	int getColorMapEntityIndex() ;
	int getOpacMapEntityIndex() ;
	std::string& getColorMapEntity(int indx) {return colorMapEntity[indx];}
	std::string& getOpacMapEntity(int indx) {return opacMapEntity[indx];}
	bool refreshIsAuto() {return autoRefresh;}
	void setAutoRefresh(bool onOff) {autoRefresh = onOff;}
	bool flowIsSteady() {return (flowType == 0);} // 0= steady, 1 = unsteady, 2 = line advection
	int getFlowType() {return flowType;}
	int getSteadyDirection() { return steadyFlowDirection;}
	int getUnsteadyDirection() { return unsteadyFlowDirection;}
	void setSteadyDirection(int dir){steadyFlowDirection = dir;}
	void setUnsteadyDirection(int dir){unsteadyFlowDirection = dir;}
	float getSteadyFlowLength(){return steadyFlowLength;}
	void setSteadyFlowLength(float val) {steadyFlowLength = val;}
	float getSteadySmoothness(){return steadySmoothness;}
	void setSteadySmoothness(float val) {steadySmoothness = val;}
	
	
	
	float getConstantOpacity() {return constantOpacity;}
	void setConstantOpacity(float val){constantOpacity = val;}
	QRgb getConstantColor() {return constantColor;}
	void setConstantColor(QRgb clr){constantColor = clr;}
	int getShapeType() {return geometryType;} //0 = tube, 1 = point, 2 = arrow
	int getObjectsPerFlowline() {return objectsPerFlowline;}
	int getObjectsPerTimestep() {return objectsPerTimestep;}
	void setObjectsPerFlowline(int objs){objectsPerFlowline = objs;}
	void setObjectsPerTimestep(int objs){objectsPerTimestep = objs;}
	bool rakeEnabled() {return doRake;}
	bool listEnabled() {return (!doRake && getNumListSeedPoints() > 0);}
	void enableRake(bool onOff){ doRake = onOff;}
	void enableList(bool onOff) {doRake = !onOff;}
	

	
	void setEditMode(bool mode) {editMode = mode;}
	bool getEditMode() {return editMode;}
	virtual MapperFunction* getMapperFunc() {return mapperFunction;}


	virtual void getBox(float boxmin[], float boxmax[], int){
		for (int i = 0; i< 3; i++){
			boxmin[i] = seedBoxMin[i];
			boxmax[i] = seedBoxMax[i];
		}
	}
	virtual void setBox(const float boxMin[], const float boxMax[], int){
		for (int i = 0; i< 3; i++){
			seedBoxMin[i] = boxMin[i];
			seedBoxMax[i] = boxMax[i];
		}
	}
	float getListSeedPoint(int i, int coord){
		return seedPointList[i].getVal(coord);
	}
	virtual int getNumRefinements() {return numRefinements;}
	void mapColors(FlowLineData*, int timeStep, int minFrame, RegionParams* rParams);
	
	//Check the variables in the flow data for missing timesteps 
	//Independent of animation params
	//Provide a warning message if data does not match input request
	//Return false if anything changed
	bool validateSampling(int minFrame, int numRefs, const int varnums[3]);
	void setFlowType(int typenum){flowType = typenum;}
	void setNumRefinements(int numtrans){numRefinements = numtrans;}
	void setMaxNumTrans(int maxNT) {maxNumRefinements = maxNT;}
	float getIntegrationAccuracy() {return integrationAccuracy;}
	void setIntegrationAccuracy(float val){integrationAccuracy = val;}
	float getSteadyScale() {return steadyScale;}
	float getUnsteadyScale() {return unsteadyScale;}
	void setSteadyScale(float val){steadyScale = val;}
	void setUnsteadyScale(float val){unsteadyScale = val;}
	
	int* getSteadyVarNums() {return steadyVarNum;}
	int* getUnsteadyVarNums() {return unsteadyVarNum;}
	int* getPriorityVarNums() {return priorityVarNum;}
	int* getSeedDistVarNums() {return seedDistVarNum;}
	void setXSteadyVarNum(int varnum){steadyVarNum[0] = varnum;
		if (priorityIsSteady && varnum > 0) priorityVarNum[0] = varnum-1;
		if (seedDistIsSteady && varnum > 0) seedDistVarNum[0] = varnum-1;
	}
	void setYSteadyVarNum(int varnum){steadyVarNum[1] = varnum;
		if (priorityIsSteady&& varnum > 0) priorityVarNum[1] = varnum-1;
		if (seedDistIsSteady&& varnum > 0) seedDistVarNum[1] = varnum-1;
	}
	void setZSteadyVarNum(int varnum){steadyVarNum[2] = varnum;
		if (priorityIsSteady&& varnum > 0) priorityVarNum[2] = varnum-1;
		if (seedDistIsSteady&& varnum > 0) seedDistVarNum[2] = varnum-1;
	}
	void setXUnsteadyVarNum(int varnum){unsteadyVarNum[0] = varnum;}
	void setYUnsteadyVarNum(int varnum){unsteadyVarNum[1] = varnum;}
	void setZUnsteadyVarNum(int varnum){unsteadyVarNum[2] = varnum;}
	//When seed dist or priority vars are changed (!= steady), set the flags too.
	void setXSeedDistVarNum(int varnum){
		seedDistVarNum[0] = varnum;
		if (varnum != steadyVarNum[0]-1) seedDistIsSteady = false;
	}
	void setYSeedDistVarNum(int varnum){
		seedDistVarNum[1] = varnum;
		if (varnum != steadyVarNum[1]-1) seedDistIsSteady = false;
	}
	void setZSeedDistVarNum(int varnum){
		seedDistVarNum[2] = varnum;
		if (varnum != steadyVarNum[2]-1) seedDistIsSteady = false;
	}
	void setXPriorityVarNum(int varnum){priorityVarNum[0] = varnum;
		if (varnum != steadyVarNum[0]-1) priorityIsSteady = false;
	}
	void setYPriorityVarNum(int varnum){priorityVarNum[1] = varnum;
		if (varnum != steadyVarNum[1]-1) priorityIsSteady = false;
	}
	void setZPriorityVarNum(int varnum){priorityVarNum[2] = varnum;
		if (varnum != steadyVarNum[2]-1) priorityIsSteady = false;
	}
	
	void setPriorityMin(float val){priorityMin = val;}
	void setPriorityMax(float val){priorityMax = val;}
	void setSeedDistBias(float val){
		if (val>= -15.f && val <= 15.f)
			seedDistBias = val;
		else seedDistBias = 0.f;
	}
	float getPriorityMin(){return priorityMin;}
	float getPriorityMax(){return priorityMax;}
	float getSeedDistBias(){return seedDistBias;}


	void setRandom(bool rand){randomGen = rand;}
	void setRandomSeed(unsigned int seed){randomSeed = seed;}
	unsigned int getRandomSeed(){return randomSeed;}
	bool isRandom() {return randomGen;}
	
	void setFlowGeometry(int geomNum){geometryType = geomNum;}
	void setColorMapEntity( int entityNum);
	void setOpacMapEntity( int entityNum);
	void setComboSteadyVarnum(int indx, int varnum){
		comboSteadyVarNum[indx] = varnum;
		if (priorityIsSteady && varnum >0) comboPriorityVarNum[indx] = varnum-1;
		if (seedDistIsSteady && varnum >0) comboSeedDistVarNum[indx] = varnum-1;
	}
	int getComboSteadyVarnum(int indx) {return comboSteadyVarNum[indx];}
	void setComboUnsteadyVarnum(int indx, int varnum){comboUnsteadyVarNum[indx] = varnum;}
	int getComboUnsteadyVarnum(int indx) {return comboUnsteadyVarNum[indx];}
	void setComboSeedDistVarnum(int indx, int varnum){comboSeedDistVarNum[indx] = varnum;}
	int getComboSeedDistVarnum(int indx) {return comboSeedDistVarNum[indx];}
	void setComboPriorityVarnum(int indx, int varnum){comboPriorityVarNum[indx] = varnum;}
	int getComboPriorityVarnum(int indx) {return comboPriorityVarNum[indx];}
	
	int getNumComboVariables() {return numComboVariables;}

	float getOpacityScale(); 
	void setOpacityScale(float val); 
	int getMaxFrame() {return maxFrame;}
	//Calculate max and min range for variables based on current data/flow settings
	//Only used for guidance in setting info in flowtab
	float minRange(int varNum, int tstep);
	float maxRange(int varNum, int tstep);
	void setSeedRegionMax(int coord, float val){
		seedBoxMax[coord] = val;
	}
	void setSeedRegionMin(int coord, float val){
		seedBoxMin[coord] = val;
	}
	void setPeriodicDim(int coord, bool val){periodicDim[coord] = val;}
	bool getPeriodicDim(int coord){return periodicDim[coord];}
	void periodicMap(float origCoords[3],float newCoords[3], bool unscale);
	std::vector<int>& getUnsteadyTimesteps() { return unsteadyTimestepList;}
	
	//This virtual RenderParams method is not useful for flow params
	virtual int getSessionVarNum() { assert(0); return -1;}

	static float getDefaultIntegrationAccuracy(){return defaultIntegrationAccuracy;}
	static float getDefaultSmoothness(){ return defaultSmoothness;}
	static float getDefaultFlowLength(){ return defaultFlowLength;}
	static float getDefaultFlowDiameter(){ return defaultFlowDiameter;}
	static float getDefaultArrowSize(){ return defaultArrowSize;}
	static float getDefaultDiamondSize(){return defaultDiamondSize;}
	static int getDefaultGeometryType() {return defaultGeometryType;}
	static void setDefaultIntegrationAccuracy(float val) {defaultIntegrationAccuracy = val;}
	static void setDefaultSmoothness(float val) {defaultSmoothness = val;}
	static void setDefaultFlowLength(float val){defaultFlowLength = val;}
	static void setDefaultFlowDiameter(float val){defaultFlowDiameter = val;}
	static void setDefaultArrowSize(float val){defaultArrowSize = val;}
	static void setDefaultDiamondSize(float val){defaultDiamondSize = val;}
	static void setDefaultGeometryType(int val){defaultGeometryType = val;}

	
protected:
	static const string _shortName;
	//Tags for attributes in session save
	//Top level labels
	static const string _mappedVariableNamesAttr;//obsolete
	static const string _steadyVariableNamesAttr;
	static const string _unsteadyVariableNamesAttr;
	static const string _seedDistVariableNamesAttr;
	static const string _priorityVariableNamesAttr;
	static const string _seedDistBiasAttr;
	static const string _priorityBoundsAttr;
	static const string _steadyFlowAttr;//obsolete
	static const string _flowTypeAttr;
	static const string _integrationAccuracyAttr;
	static const string _velocityScaleAttr;
	static const string _steadyScaleAttr;
	static const string _unsteadyScaleAttr;
	static const string _timeSamplingAttr;
	static const string _autoRefreshAttr;
	static const string _periodicDimsAttr;
	static const string _flowLineLengthAttr;
	static const string _smoothnessAttr;
	static const string _autoScaleAttr;
	static const string _steadyFlowDirectionAttr;
	static const string _unsteadyFlowDirectionAttr;
	


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
	static const string _useSeedListAttr;//obsolete
	static const string _seedPointTag;
	static const string _positionAttr;
	static const string _timestepAttr;

	//Geometry attributes
	static const string _geometryTag;
	static const string _geometryTypeAttr;
	static const string _objectsPerFlowlineAttr;
	static const string _objectsPerTimestepAttr;
	static const string _displayIntervalAttr;
	static const string _shapeDiameterAttr;
	static const string _diamondDiameterAttr;
	static const string _arrowDiameterAttr;
	static const string _colorMappedEntityAttr;
	static const string _opacityMappedEntityAttr;
	static const string _constantColorAttr;
	static const string _constantOpacityAttr;

	//Mapping bounds, variable names (for all variables, mapped or not) are in variableMapping node
	
	static const string _leftColorBoundAttr;
	static const string _rightColorBoundAttr;
	static const string _leftOpacityBoundAttr;
	static const string _rightOpacityBoundAttr;
	static const string _steadyDirectionAttr;
	static const string _unsteadyDirectionAttr;
	static const string _steadyFlowLengthAttr;
	static const string _numFLASamplesAttr;
	static const string _advectBeforePrioritizeAttr;
	
	//Insert seeds into a pathLineData at the specified time step.
	int insertUnsteadySeeds(RegionParams* rParams, VaporFlow* fLib, PathLineData* pathLines, int timeStep);
	//Similarly with fieldLineData:
	int insertSteadySeeds(RegionParams* rParams, VaporFlow* fLib, FlowLineData* flowLines, int timeStep);
	
	void setCurrentDimension(int dimNum) {currentDimension = dimNum;}
	
	//check if vector field is present for a timestep
	bool validateVectorField(int timestep, int numRefs, const int varnums[3]);

	//Find the average vector flow value over current region, at specified resolution.
	//Note:  this can be time-consuming!
	float getAvgVectorMag(RegionParams* reg, int numRefnts, int timestep);
	
	int flowType; //steady = 0, unsteady = 1;
	
	int numRefinements, maxNumRefinements;
	int numComboVariables;// = number of variables in metadata with data associated
	
	int steadyVarNum[3]; //field variable num's in x, y, and z.  These are session var nums
	int unsteadyVarNum[3]; //field variable num's in x, y, and z.
	int priorityVarNum[3]; //varnum for priority field
	int seedDistVarNum[3]; //varnum for seed distrib
	int comboSteadyVarNum[3];  //indices in combos.  These are metadata var nums.
	int comboUnsteadyVarNum[3];  //indices in combos
	int comboPriorityVarNum[3];
	int comboSeedDistVarNum[3];
	bool seedDistIsSteady; //Seed distrib field same as steady field
	bool priorityIsSteady; //Priority field same as steady field
	float integrationAccuracy;
	float steadyScale;
	float unsteadyScale;
	float steadySmoothness;
	
	int timeSamplingInterval;
	int timeSamplingStart;
	int timeSamplingEnd;

	bool editMode;
	bool randomGen;
	unsigned int randomSeed;
	float seedBoxMin[3], seedBoxMax[3];
	float seedDistBias;
	float priorityMin, priorityMax;
	
	size_t generatorCount[3];
	size_t allGeneratorCount;
	int seedTimeStart, seedTimeEnd, seedTimeIncrement;
	int currentDimension;//Which dimension is showing in generator count

	int geometryType;  //0= tube, 1=point, 2 = arrow
	int objectsPerFlowline;
	int objectsPerTimestep;
	int firstDisplayFrame, lastDisplayFrame;
	
	float shapeDiameter;
	float diamondDiameter;
	float arrowDiameter;
	QRgb constantColor;
	float constantOpacity;
	
	MapperFunction* mapperFunction;
	
	//Save the min and max bounds for each of the flow mappings, plus 
	//min and max for each variable
	float* minOpacBounds;
	float* maxOpacBounds;
	float* minColorBounds;
	float* maxColorBounds;
	
	int maxPoints;  //largest length of any flow
	int compressionLevel;
	
	int maxFrame;  //Maximum frame timeStep.  Used to determine array sizes for
	//flow data caches.
	
	std::vector<string> colorMapEntity;
	std::vector<string> opacMapEntity;

	std::vector<Point4> seedPointList;
	std::vector<int> unsteadyTimestepList;
	
	bool autoRefresh;
	bool autoScale;
	bool doRake;
	int numFLASamples;
	bool flaAdvectBeforePrioritize;
	
	bool useTimestepSampleList;
	bool useDisplayLists;
	bool periodicDim[3];
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	int unsteadyFlowDirection; //either -1 or 1, backwards or forwards
	int steadyFlowDirection; //either -1, 1, or 0 backwards, forwards, or both
	float steadyFlowLength;
	static float defaultIntegrationAccuracy;
	static float defaultSmoothness;
	static float defaultFlowLength;
	static float defaultFlowDiameter;
	static float defaultArrowSize;
	static float defaultDiamondSize;
	static int defaultGeometryType;
		

};
};
#endif //FLOWPARAMS_H 
