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
#include "vaporinternal/common.h"
class FlowTab;
#define IGNORE_FLAG 2.e30f
namespace VAPoR {
class MapperFunction;
class VaporFlow;
class ExpatParseMgr;
class FlowMapEditor;
class VECTOR4;
class RegionParams;
class PARAMS_API FlowParams: public RenderParams {
	
public: 
	FlowParams(int winnum);
	
	~FlowParams();

	
	void setTab(FlowTab* tab); 
	virtual Params* deepCopy();
	

	// Reinitialize due to new Session:
	bool reinit(bool doOverride);
	//Override default set-enabled to create/destroy FlowLib
	virtual void setEnabled(bool value);
	
	

	//Save, restore stuff:
	XmlNode* buildNode(); 
	virtual bool elementStartHandler(ExpatParseMgr*, int  depth , std::string& tag, const char ** attribs);
	virtual bool elementEndHandler(ExpatParseMgr*, int depth , std::string& tag);
	
	
	
	//Virtual methods to set map bounds.  Get() is in parent class
	//Needed here because the map bounds are saved in params class for each mapped variable
	virtual void setMinColorMapBound(float val);
	virtual void setMaxColorMapBound(float val);
	virtual void setMinOpacMapBound(float val);
	virtual void setMaxOpacMapBound(float val);

	virtual void restart();
	
	int getNumGenerators(int dimNum) { return (int)generatorCount[dimNum];}
	void setNumGenerators(int dimNum, int val){generatorCount[dimNum]=val;}
	int getTotalNumGenerators() { return (int)allGeneratorCount;}
	void setTotalNumGenerators(int val){allGeneratorCount = val;}
	VaporFlow* getFlowLib(){return myFlowLib;}
	bool regenerateFlowData(int frameNum, int minTime, bool fromRake, RegionParams* rParams, float* data, float* rgbas);


	void calcSeedExtents(float *extents);
	float getSeedRegionMin(int coord){ return seedBoxMin[coord];}
	float getSeedRegionMax(int coord){ return seedBoxMax[coord];}
	
	int calcMaxPoints();
	int calcNumSeedPoints(bool rake, int timeStep);
	
	int getNumInjections() { return numInjections;}
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
	float getArrowDiameter() {return arrowDiameter;}
	void setShapeDiameter(float val) {shapeDiameter=val;}
	void setArrowDiameter(float val) {arrowDiameter=val;}
	int getTimeSamplingInterval() {return timeSamplingInterval;}
	void setTimeSamplingInterval(int val){timeSamplingInterval = val;}
	int getTimeSamplingStart() {return timeSamplingStart;}
	void setTimeSamplingStart(int val){timeSamplingStart = val;}
	int getTimeSamplingEnd() {return timeSamplingEnd;}
	void setTimeSamplingEnd(int val){timeSamplingEnd = val;}
	//Distinguish between the number of seed points specified in the
	//current settings and the number actually used the last time
	//the flow was calculated:
	//
	
	int getNumRakeSeedPoints(){
		return (int)(randomGen ? allGeneratorCount : (int)(generatorCount[0]*generatorCount[1]*generatorCount[2]));
	}
	int getNumListSeedPoints() {return (int)seedPointList.size();}
		std::vector<Point4>& getSeedPointList(){return seedPointList;}
	void pushSeed(Point4& newSeed){seedPointList.push_back(newSeed);}
	void moveLastSeed(const float* newCoords){
		if (seedPointList.size() > 0)
			seedPointList[seedPointList.size()-1].set3Val(newCoords);
	}
	void emptySeedList(){seedPointList.empty();}
	int getColorMapEntityIndex() ;
	int getOpacMapEntityIndex() ;
	std::string& getColorMapEntity(int indx) {return colorMapEntity[indx];}
	std::string& getOpacMapEntity(int indx) {return opacMapEntity[indx];}
	bool refreshIsAuto() {return autoRefresh;}
	void setAutoRefresh(bool onOff) {autoRefresh = onOff;}
	bool flowIsSteady() {return (flowType == 0);} // 0= steady, 1 = unsteady
	int getFlowType() {return flowType;}
	
	
	
	float getConstantOpacity() {return constantOpacity;}
	void setConstantOpacity(float val){constantOpacity = val;}
	QRgb getConstantColor() {return constantColor;}
	void setConstantColor(QRgb clr){constantColor = clr;}
	int getShapeType() {return geometryType;} //0 = tube, 1 = point, 2 = arrow
	int getObjectsPerFlowline() {return objectsPerFlowline;}
	void setObjectsPerFlowline(int objs){objectsPerFlowline = objs;}
	bool rakeEnabled() {return doRake;}
	bool listEnabled() {return (doSeedList && getNumListSeedPoints() > 0);}
	void enableRake(bool onOff){ doRake = onOff;}
	void enableList(bool onOff) {doSeedList = onOff;}
	

	
	void setEditMode(bool mode) {editMode = mode;}
	bool getEditMode() {return editMode;}
	virtual MapperFunction* getMapperFunc() {return mapperFunction;}


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
	virtual int getNumRefinements() {return numRefinements;}
	
	void mapColors(float* speeds, int timeStep, int minFrame, int numSeeds, float* flowData, float *rgbas, bool isRake);
	//Check the variables in the flow data for missing timesteps 
	//Independent of animation params
	//Provide an info message if data does not match input request
	//Return false if anything changed
	bool validateSampling(int minFrame);
	FlowMapEditor* getFlowMapEditor() {return flowMapEditor;}
	void setFlowType(int typenum){flowType = typenum;}
	void setNumRefinements(int numtrans){numRefinements = numtrans;}
	void setMaxNumTrans(int maxNT) {maxNumRefinements = maxNT;}
	float getIntegrationAccuracy() {return integrationAccuracy;}
	void setIntegrationAccuracy(float val){integrationAccuracy = val;}
	float getVelocityScale() {return velocityScale;}
	void setVelocityScale(float val){velocityScale = val;}
	
	void setXVarNum(int varnum){varNum[0] = varnum;}
	void setYVarNum(int varnum){varNum[1] = varnum;}
	void setZVarNum(int varnum){varNum[2] = varnum;}
	void setRandom(bool rand){randomGen = rand;}
	void setRandomSeed(unsigned int seed){randomSeed = seed;}
	unsigned int getRandomSeed(){return randomSeed;}
	bool isRandom() {return randomGen;}
	
	void setFlowGeometry(int geomNum){geometryType = geomNum;}
	void setColorMapEntity( int entityNum);
	void setOpacMapEntity( int entityNum);
	void setComboVarnum(int indx, int varnum){comboVarNum[indx] = varnum;}
	int getComboVarnum(int indx) {return comboVarNum[indx];}
	int getNumComboVariables() {return numComboVariables;}
	// helper functions for writing stream and path lines
	bool writeStreamline(FILE* saveFile, int streamNum, int frameNum, float* flowDataArray);
	bool writePathline(FILE* saveFile, int pathNum, int minFrame, int injectionNum, float* flowDataArray);
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
	void periodicMap(float origCoords[3],float newCoords[3]);
	

protected:
	//Tags for attributes in session save
	//Top level labels
	static const string _mappedVariableNamesAttr;
	static const string _steadyFlowAttr;
	static const string _integrationAccuracyAttr;
	static const string _velocityScaleAttr;
	static const string _timeSamplingAttr;
	static const string _autoRefreshAttr;
	static const string _periodicDimsAttr;

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
	
	void setCurrentDimension(int dimNum) {currentDimension = dimNum;}
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	
	//check if vector field is present for a timestep
	bool validateVectorField(int timestep);
	
	int flowType; //steady = 0, unsteady = 1;
	
	int numRefinements, maxNumRefinements;
	int numComboVariables;// = number of variables in metadata with data associated
	
	int varNum[3]; //field variable num's in x, y, and z.
	int comboVarNum[3];  //indices in combos
	float integrationAccuracy;
	float velocityScale;
	int timeSamplingInterval;
	int timeSamplingStart;
	int timeSamplingEnd;

	
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
	float arrowDiameter;
	QRgb constantColor;
	float constantOpacity;
	
	
	
	MapperFunction* mapperFunction;
	FlowMapEditor* flowMapEditor;
	
	//Save the min and max bounds for each of the flow mappings, plus 
	//min and max for each variable
	float* minOpacBounds;
	float* maxOpacBounds;
	float* minColorBounds;
	float* maxColorBounds;
	
	int maxPoints;  //largest length of any flow
	
	int maxFrame;  //Maximum frame timeStep.  Used to determine array sizes for
	//flow data caches.
	
	std::vector<string> colorMapEntity;
	std::vector<string> opacMapEntity;

	std::vector<Point4> seedPointList;
	
	
	VaporFlow* myFlowLib;

	
	bool autoRefresh;

	bool doRake;
	bool doSeedList;

	bool periodicDim[3];
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	
	int numInjections;

};
};
#endif //FLOWPARAMS_H 
