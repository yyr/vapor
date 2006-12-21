//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:  Implementation of flowparams class 
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "flowparams.h"
#include "regionparams.h"
#include "datastatus.h"
#include "vapor/VaporFlow.h"
#include "vapor/flowlinedata.h"
#include "glutil.h"
#include "errorcodes.h"

#include <qapplication.h>
#include <qcursor.h>

#include "mapperfunction.h"

#include "vaporinternal/common.h"
//Step sizes for integration accuracy:
#define SMALLEST_MIN_STEP 0.05f
#define LARGEST_MIN_STEP 4.f
#define SMALLEST_MAX_STEP 0.25f
#define LARGEST_MAX_STEP 10.f
using namespace VAPoR;
	const string FlowParams::_seedingTag = "FlowSeeding";
	const string FlowParams::_seedRegionMinAttr = "SeedRegionMins";
	const string FlowParams::_seedRegionMaxAttr = "SeedRegionMaxes";
	const string FlowParams::_randomGenAttr = "RandomSeeding";
	const string FlowParams::_randomSeedAttr = "RandomSeed";
	const string FlowParams::_generatorCountsAttr = "GeneratorCountsByDimension";
	const string FlowParams::_totalGeneratorCountAttr = "TotalGeneratorCount";
	const string FlowParams::_seedTimesAttr = "SeedInjectionTimes";
	const string FlowParams::_useRakeAttr = "UseRake";
	const string FlowParams::_useSeedListAttr = "UseSeedList";
	//One tag for each seed point in the list:
	const string FlowParams::_seedPointTag = "SeedPoint";
	const string FlowParams::_positionAttr = "Position";
	const string FlowParams::_timestepAttr = "TimeStep";
	const string FlowParams::_steadyFlowDirectionAttr = "SteadyFlowDirection";
	const string FlowParams::_unsteadyFlowDirectionAttr = "UnsteadyFlowDirection";

	const string FlowParams::_mappedVariableNamesAttr = "MappedVariableNames";//obsolete
	const string FlowParams::_steadyVariableNamesAttr = "SteadyVariableNames";
	const string FlowParams::_unsteadyVariableNamesAttr = "UnsteadyVariableNames";
	
	const string FlowParams::_periodicDimsAttr = "PeriodicDimensions";
	const string FlowParams::_steadyFlowAttr = "SteadyFlow";
	const string FlowParams::_integrationAccuracyAttr = "IntegrationAccuracy";
	const string FlowParams::_velocityScaleAttr = "velocityScale";
	const string FlowParams::_steadyScaleAttr = "steadyScale";
	const string FlowParams::_unsteadyScaleAttr = "unsteadyScale";
	const string FlowParams::_timeSamplingAttr = "SamplingTimes";
	const string FlowParams::_autoRefreshAttr = "AutoRefresh";
	const string FlowParams::_autoScaleAttr = "AutoScale";
	const string FlowParams::_flowLineLengthAttr = "FlowLineLength";
	const string FlowParams::_smoothnessAttr = "Smoothness";
	
	
	
	//Geometry variables:
	const string FlowParams::_steadyDirectionAttr = "steadyDirection";
	const string FlowParams::_unsteadyDirectionAttr = "unsteadyDirection";
	const string FlowParams::_geometryTag = "FlowGeometry";
	const string FlowParams::_geometryTypeAttr = "GeometryType";
	const string FlowParams::_objectsPerFlowlineAttr = "ObjectsPerFlowline";
	const string FlowParams::_objectsPerTimestepAttr = "ObjectsPerTimestep";
	const string FlowParams::_displayIntervalAttr = "DisplayInterval";
	const string FlowParams::_steadyFlowLengthAttr = "SteadyFlowLength";
	const string FlowParams::_shapeDiameterAttr = "ShapeDiameter";
	const string FlowParams::_arrowDiameterAttr = "ArrowheadDiameter";
	const string FlowParams::_colorMappedEntityAttr = "ColorMappedEntityIndex";
	const string FlowParams::_opacityMappedEntityAttr = "OpacityMappedEntityIndex";
	const string FlowParams::_constantColorAttr = "ConstantColorRGBValue";
	const string FlowParams::_constantOpacityAttr = "ConstantOpacityValue";
	//Mapping bounds (for all variables, mapped or not) are inside geometry node
	const string FlowParams::_leftColorBoundAttr = "LeftColorBound";
	const string FlowParams::_rightColorBoundAttr = "RightColorBound";
	const string FlowParams::_leftOpacityBoundAttr = "LeftOpacityBound";
	const string FlowParams::_rightOpacityBoundAttr = "RightOpacityBound";


FlowParams::FlowParams(int winnum) : RenderParams(winnum) {
	thisParamType = FlowParamsType;
	//myFlowLib = 0;
	mapperFunction = 0;
	
	//Set all parameters to default values
	restart();
	
}
FlowParams::~FlowParams(){
	
	if (mapperFunction){
		delete mapperFunction;
	}
	
		
	if (minColorBounds) {
		delete minColorBounds;
		delete minOpacBounds;
		delete maxColorBounds;
		delete maxOpacBounds;
	}
	
}
//Set everything in sight to default state:
void FlowParams::
restart() {
	magChanged = true;
	autoScale = true;
	
	steadyFlowDirection = 0;
	unsteadyFlowDirection = 1; //default is forward
	steadyFlowLength = 1.f;
	steadySmoothness = 20.f;
	periodicDim[0]=periodicDim[1]=periodicDim[2]=false;
	autoRefresh = true;
	enabled = false;
	useTimestepSampleList = false;

	flowType = 0; //steady
	
	numRefinements = 0; 
	maxNumRefinements = 4; 
	numComboVariables = 0;
	firstDisplayFrame = 0;
	lastDisplayFrame = 20;
	
	steadyVarNum[0]= 0;
	steadyVarNum[1] = 1;
	steadyVarNum[2] = 2;
	unsteadyVarNum[0]= 0;
	unsteadyVarNum[1] = 1;
	unsteadyVarNum[2] = 2;

	comboSteadyVarNum[0]= 0;
	comboSteadyVarNum[1] = 1;
	comboSteadyVarNum[2] = 2;
	comboUnsteadyVarNum[0]= 0;
	comboUnsteadyVarNum[1] = 1;
	comboUnsteadyVarNum[2] = 2;
	integrationAccuracy = 0.5f;
	steadyScale = 1.0f;
	unsteadyScale = 1.0f;
	constantColor = qRgb(255,0,0);
	constantOpacity = 1.f;
	timeSamplingInterval = 1;
	timeSamplingStart = 1;
	timeSamplingEnd = 100;
	
	editMode = true;
	

	randomGen = true;
	
	
	randomSeed = 1;
	seedBoxMin[0] = seedBoxMin[1] = seedBoxMin[2] = 0.f;
	seedBoxMax[0] = seedBoxMax[1] = seedBoxMax[2] = 1.f;
	
	generatorCount[0]=generatorCount[1]=generatorCount[2] = 1;

	allGeneratorCount = 10;
	seedTimeStart = 1; 
	seedTimeEnd = 100; 
	seedTimeIncrement = 1;
	currentDimension = 0;

	geometryType = 0;  //0= tube, 1=point, 2 = arrow
	objectsPerFlowline = 20;
	objectsPerTimestep = 1;

	shapeDiameter = 1.f;
	arrowDiameter = 2.f;

	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Timestep");
	colorMapEntity.push_back("Field Magnitude");
	colorMapEntity.push_back("Seed Index");

	opacMapEntity.clear();
	
	opacMapEntity.push_back("Constant");
	opacMapEntity.push_back("Timestep");
	opacMapEntity.push_back("Field Magnitude");
	opacMapEntity.push_back("Seed Index");
	minColorEditBounds = new float[3];
	maxColorEditBounds = new float[3];
	minOpacEditBounds = new float[3];
	maxOpacEditBounds = new float[3];
	for (int i = 0; i< 3; i++){
		minColorEditBounds[i]= 0.f;
		maxColorEditBounds[i]= 1.f;
		minOpacEditBounds[i]=0.f;
		maxOpacEditBounds[i]=1.f;
	}
	
	seedPointList.clear();
	unsteadyTimestepList.clear();
	numInjections = 1;
	maxPoints = 0;
	maxFrame = 0;

	minOpacBounds = new float[3];
	maxOpacBounds= new float[3];
	minColorBounds= new float[3];
	maxColorBounds= new float[3];
	for (int k = 0; k<3; k++){
		minColorBounds[k] = minOpacBounds[k] = 0.f;
		maxColorBounds[k] = maxOpacBounds[k] = 1.f;
	}
	
	
	doRake = true;
	
}

//Make a copy of  parameters:
RenderParams* FlowParams::
deepRCopy(){
	
	FlowParams* newFlowParams = new FlowParams(*this);
	//Keep the flowlib??
	//newFlowParams->myFlowLib = 0;
	//Clone the map bounds arrays:
	int numVars = numComboVariables+4;
	newFlowParams->minColorEditBounds = new float[numVars];
	newFlowParams->maxColorEditBounds = new float[numVars];
	newFlowParams->minOpacEditBounds = new float[numVars];
	newFlowParams->maxOpacEditBounds = new float[numVars];
	//Clone the variable bounds:
	newFlowParams->minColorBounds = new float[numVars];
	newFlowParams->maxColorBounds = new float[numVars];
	newFlowParams->minOpacBounds = new float[numVars];
	newFlowParams->maxOpacBounds = new float[numVars];
	for (int i = 0; i<numVars; i++){
		newFlowParams->minColorEditBounds[i] = minColorEditBounds[i];
		newFlowParams->maxColorEditBounds[i] = maxColorEditBounds[i];
		newFlowParams->minOpacEditBounds[i] = minOpacEditBounds[i];
		newFlowParams->maxOpacEditBounds[i] = maxOpacEditBounds[i];
		newFlowParams->minColorBounds[i] = minColorBounds[i];
		newFlowParams->maxColorBounds[i] = maxColorBounds[i];
		newFlowParams->minOpacBounds[i] = minOpacBounds[i];
		newFlowParams->maxOpacBounds[i] = maxOpacBounds[i];
	}
	
	//Clone the Transfer Functions
	if (mapperFunction) {
		newFlowParams->mapperFunction = new MapperFunction(*mapperFunction);
	} else {
		newFlowParams->mapperFunction = 0;
	}
	

	
	return newFlowParams;
}

//Reinitialize settings, session has changed
//If dooverride is true, then go back to default state.  If it's false, try to 
//make use of previous settings
//
bool FlowParams::
reinit(bool doOverride){
	int i;
	if(doOverride) restart();
	int nlevels = DataStatus::getInstance()->getNumTransforms();
	
	setMaxNumTrans(nlevels);
	
	//Make min and max conform to new data.
	//Start with values in session:
	int minFrame = (int)(DataStatus::getInstance()->getMinTimestep());
	maxFrame = (int)(DataStatus::getInstance()->getMaxTimestep());
	editMode = true;
	// set the params state based on whether we are overriding or not:
	if (doOverride) {
		
		numRefinements = 0;
		seedTimeStart = minFrame;
		seedTimeEnd = maxFrame;
		seedTimeIncrement = 1;
		autoRefresh = true;
		periodicDim[0]=periodicDim[1]=periodicDim[2];
	} else {
		if (numRefinements> maxNumRefinements) numRefinements = maxNumRefinements;
		
		//Make sure we really can use the specified numTrans,
		//with the current region settings
		
		//numRefinements = rParams->validateNumTrans(numRefinements);
		if (seedTimeStart >= maxFrame) seedTimeStart = maxFrame-1;
		if (seedTimeStart < minFrame) seedTimeStart = minFrame;
		if (seedTimeEnd >= maxFrame) seedTimeEnd = maxFrame-1;
		if (seedTimeEnd < seedTimeStart) seedTimeEnd = seedTimeStart;
	}
	

	//Set up the seed region:
	const float* fullExtents = DataStatus::getInstance()->getExtents();
	if (doOverride){
		for (i = 0; i<3; i++){
			seedBoxMin[i] = fullExtents[i];
			seedBoxMax[i] = fullExtents[i+3];
		}
	} else {
		for (i = 0; i<3; i++){
			if(seedBoxMin[i] < fullExtents[i])
				seedBoxMin[i] = fullExtents[i];
			if(seedBoxMax[i] > fullExtents[i+3])
				seedBoxMax[i] = fullExtents[i+3];
			if(seedBoxMax[i] < seedBoxMin[i]) 
				seedBoxMax[i] = seedBoxMin[i];
		}
	}

	//Set up variables:
	//Get the variable names:
	
	int newNumVariables = DataStatus::getInstance()->getNumVariables();
	int newNumComboVariables = DataStatus::getInstance()->getNumMetadataVariables();
	
	//Rebuild map bounds arrays:
	if(minOpacBounds) delete minOpacBounds;
	minOpacBounds = new float[newNumComboVariables+4];
	if(maxOpacBounds) delete maxOpacBounds;
	maxOpacBounds = new float[newNumComboVariables+4];
	if(minColorBounds) delete minColorBounds;
	minColorBounds = new float[newNumComboVariables+4];
	if(maxColorBounds) delete maxColorBounds;
	maxColorBounds = new float[newNumComboVariables+4];
	
	
	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Timestep");
	colorMapEntity.push_back("Field Magnitude");
	colorMapEntity.push_back("Seed Index");
	opacMapEntity.clear();
	opacMapEntity.push_back("Constant");
	opacMapEntity.push_back("Timestep");
	opacMapEntity.push_back("Field Magnitude");
	opacMapEntity.push_back("Seed Index");
	for (i = 0; i< newNumComboVariables; i++){
		colorMapEntity.push_back(DataStatus::getInstance()->getMetadataVarName(i));
		opacMapEntity.push_back(DataStatus::getInstance()->getMetadataVarName(i));
	}
	
	if(doOverride || getOpacMapEntityIndex() >= newNumComboVariables+4){
		setOpacMapEntity(0);
	}
	if(doOverride || getColorMapEntityIndex() >= newNumComboVariables+4){
		setColorMapEntity(0);
	}
	if (doOverride){
		for (int j = 0; j<3; j++){
			comboSteadyVarNum[j] = Min(j,newNumComboVariables-1);
			steadyVarNum[j] = DataStatus::getInstance()->mapMetadataToRealVarNum(comboSteadyVarNum[j]);
		}
		for (int j = 0; j<3; j++){
			comboUnsteadyVarNum[j] = Min(j,newNumComboVariables-1);
			unsteadyVarNum[j] = DataStatus::getInstance()->mapMetadataToRealVarNum(comboUnsteadyVarNum[j]);
		}
	} 
	for (int dim = 0; dim < 3; dim++){
		//See if current steadyvarNum is valid.  If not, 
		//reset to first variable that is present:
		if (!DataStatus::getInstance()->variableIsPresent(steadyVarNum[dim])){
			steadyVarNum[dim] = -1;
			for (i = 0; i<newNumVariables; i++) {
				if (DataStatus::getInstance()->variableIsPresent(i)){
					steadyVarNum[dim] = i;
					break;
				}
			}
		}
		//See if current unsteadyvarNum is valid.  If not, 
		//reset to first variable that is present:
		if (!DataStatus::getInstance()->variableIsPresent(unsteadyVarNum[dim])){
			unsteadyVarNum[dim] = -1;
			for (i = 0; i<newNumVariables; i++) {
				if (DataStatus::getInstance()->variableIsPresent(i)){
					unsteadyVarNum[dim] = i;
					break;
				}
			}
		}
	}
	if (steadyVarNum[0] == -1){
		
		numComboVariables = 0;
		return false;
	}
	//Determine the combo var nums from the varNum's
	for (int dim = 0; dim < 3; dim++){
		comboSteadyVarNum[dim] = DataStatus::getInstance()->mapRealToMetadataVarNum(steadyVarNum[dim]);
		comboUnsteadyVarNum[dim] = DataStatus::getInstance()->mapRealToMetadataVarNum(unsteadyVarNum[dim]);
	}
	//Set up sampling.  
	if (doOverride){
		timeSamplingStart = minFrame;
		timeSamplingEnd = maxFrame;
		timeSamplingInterval = 1;
	}
	
	validateSampling(minFrame);

	
	//Now set up bounds arrays based on current mapped variable settings:
	for (i = 0; i< newNumComboVariables+4; i++){
		minOpacBounds[i] = minColorBounds[i] = minRange(i, minFrame);
		maxOpacBounds[i] = maxColorBounds[i] = maxRange(i, minFrame);
	}
	
	
	//Create new edit bounds whether we override or not
	float* newMinOpacEditBounds = new float[newNumComboVariables+4];
	float* newMaxOpacEditBounds = new float[newNumComboVariables+4];
	float* newMinColorEditBounds = new float[newNumComboVariables+4];
	float* newMaxColorEditBounds = new float[newNumComboVariables+4];
	//Either try to reuse existing MapperFunction, or create a new one.
	if (doOverride){ //create new ones:
		//For now, assume 8-bits mapping
		MapperFunction* newMapperFunction = new MapperFunction(this, 8);
		//Initialize to be fully opaque:
		newMapperFunction->setOpaque();

        //Set to default map bounds
		newMapperFunction->setMinColorMapValue(0.f);
		newMapperFunction->setMaxColorMapValue(1.f);
		newMapperFunction->setMinOpacMapValue(0.f);
		newMapperFunction->setMaxOpacMapValue(1.f);
		//const
		newMinOpacEditBounds[0] = 0.f;
		newMaxOpacEditBounds[0] = 1.f;
		newMinColorEditBounds[0] = 0.f;
		newMaxColorEditBounds[0] = 1.f;
		//speed
		newMinOpacEditBounds[1] = 0.f;
		newMaxOpacEditBounds[1] = maxOpacBounds[1];
		newMinColorEditBounds[1] = 0.f;
		newMaxColorEditBounds[1] = maxColorBounds[1];
		//age
		newMinOpacEditBounds[2] = minOpacBounds[2];
		newMaxOpacEditBounds[2] = maxOpacBounds[2];
		newMinColorEditBounds[2] = minColorBounds[2];
		newMaxColorEditBounds[2] = maxColorBounds[2];
		//Other variables:
		for (i = 0; i< newNumComboVariables; i++){
			if (DataStatus::getInstance()->variableIsPresent(i)){
				newMinOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMax(i);
				newMinColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin(i);
			} else {
				newMinOpacEditBounds[i+4] = 0.f;
				newMaxOpacEditBounds[i+4] = 1.f;
				newMinColorEditBounds[i+4] = 0.f;
				newMaxColorEditBounds[i+4] = 1.f;
			}
		} 

		delete mapperFunction;
        mapperFunction = newMapperFunction;

		setColorMapEntity(0);
		setOpacMapEntity(0);
	}
	else { 
		
		//Try to reuse existing bounds:
		
		for (i = 0; i< newNumComboVariables; i++){
			if (i >= numComboVariables){
				newMinOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMax(i);
				newMinColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin(i);
				newMaxColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMax(i);
			} else {
				newMinOpacEditBounds[i+4] = minOpacEditBounds[i+4];
				newMaxOpacEditBounds[i+4] = maxOpacEditBounds[i+4];
				newMinColorEditBounds[i+4] = minColorEditBounds[i+4];
				newMaxColorEditBounds[i+4] = maxColorEditBounds[i+4];
			}
		} 
		if (!mapperFunction) mapperFunction = new MapperFunction(this, 8);

        mapperFunction->setColorVarNum(getColorMapEntityIndex());
        mapperFunction->setOpacVarNum(getOpacMapEntityIndex());
	}
	
		
	delete minOpacEditBounds;
	delete minColorEditBounds;
	delete maxOpacEditBounds;
	delete maxColorEditBounds;
	
	minOpacEditBounds = newMinOpacEditBounds;
	maxOpacEditBounds = newMaxOpacEditBounds;
	minColorEditBounds = newMinColorEditBounds;
	maxColorEditBounds = newMaxColorEditBounds;

	
	
	numComboVariables = newNumComboVariables;
	
	//Don't disable, keep previous state
	//setEnabled(false);

	setMinColorEditBound(getMinColorMapBound(),getColorMapEntityIndex());
	setMaxColorEditBound(getMaxColorMapBound(),getColorMapEntityIndex());
	setMinOpacEditBound(getMinOpacMapBound(),getOpacMapEntityIndex());
	setMaxOpacEditBound(getMaxOpacMapBound(),getOpacMapEntityIndex());
	
	return true;
}


float FlowParams::getOpacityScale() {

  if (mapperFunction)
  {
    return mapperFunction->getOpacityScaleFactor();
  }

  return 1.0;
}

void FlowParams::setOpacityScale(float val) {
 
  if (mapperFunction)
  {
    mapperFunction->setOpacityScaleFactor(val);
  }
}



bool FlowParams::
writeStreamline(FILE* saveFile, int streamNum, int currentFrameNum, float* flowDataArray){

	//For each seed go from the start to the end.
	//IGNORE_FLAG's get replaced with END_FLOW_FLAG
	//If there's a stationary_stream_flag at the start, then find the 
	//first nonstationary point and copy that.
	//If there's a stationary_stream_flag at end, just copy the last
	//nonstationary point to the end.
	//First check for stationary
	float firstx = flowDataArray[3*maxPoints*streamNum];
	float stationaryPoint[3];
	bool doingStationary = false;
	bool outOfBounds = false;
	bool foundValidPoint = false;
	int j;
	if (firstx == STATIONARY_STREAM_FLAG){
		doingStationary = true;
		//Find the first nonstationary point:
		
		for (j =1; j<maxPoints; j++){
			if (flowDataArray[3*(maxPoints*streamNum + j)] != STATIONARY_STREAM_FLAG){
				stationaryPoint[0] = flowDataArray[3*(maxPoints*streamNum + j)];
				stationaryPoint[1] = flowDataArray[1+3*(maxPoints*streamNum + j)];
				stationaryPoint[2] = flowDataArray[2+3*(maxPoints*streamNum + j)];
				break;
			}
		}
		//We should have found a nonstationary point!
		assert(j < maxPoints);
	}
	int rc;
	//Now write the points with the current time step
	for (j = 0; j<maxPoints; j++){
		float* point = flowDataArray + 3*(maxPoints*streamNum +j);
		//Check if the stationary flag is valid
		if (doingStationary && (*point != STATIONARY_STREAM_FLAG)){
			doingStationary = false;
		}
		//If we have had valid points, an endflow flag puts us out of bounds:
		if (foundValidPoint && point[0] == END_FLOW_FLAG)
			outOfBounds = true;
		//Now write the appropriate point:
		if(doingStationary){ //Output current stationary point, or
			rc=fprintf(saveFile,"%8g %8g %8g %8g\n",
				stationaryPoint[0],stationaryPoint[1],stationaryPoint[2],
				(float)currentFrameNum);
			if (rc <= 0) return false;
		} 
		else if (point[0] == STATIONARY_STREAM_FLAG){
			//finding stationary point now..
			//If we find it after the first position, it will be 
			//stationary to the end.
			//Go back to previous point:
			doingStationary = true;
			assert (j>0);
			stationaryPoint[0]= *(point-3);
			stationaryPoint[1]= *(point-2);
			stationaryPoint[2]= *(point-1);
			rc=fprintf(saveFile,"%8g %8g %8g %8g\n",
				stationaryPoint[0],stationaryPoint[1],stationaryPoint[2],
				(float)currentFrameNum);
			if (rc <= 0) return false;
		} 
		else if (point[0] == END_FLOW_FLAG || point[0] == IGNORE_FLAG || outOfBounds){
			//If past or before end, just fill with END_FLOW_FLAG...
			rc=fprintf(saveFile,"%8g %8g %8g %8g\n",
				END_FLOW_FLAG,END_FLOW_FLAG,END_FLOW_FLAG,
				(float)currentFrameNum);
			if (rc <= 0) return false;
		} else { //otherwise just output the actual point
			
			rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
				*point, *(point+1),*(point+2),
				(float)currentFrameNum);
			if (rc <= 0) return false;
			foundValidPoint = true;
		}
	}
	return true;
}
bool FlowParams::
writePathline(FILE* saveFile, int pathNum, int minFrame, int injNum, float* flowDataArray){

	//For each seed go from the start to the end.
	//output END_FLOW_FLAG if found
	//Calculate and output timesteps for each value
	//Don't worry about stationary flow flag
	
	int j;
	bool outOfBounds = false;
	int rc;
	
		
	//Go through all the points.
	//Find the timestep for the seed point to be emitted:
	float seedTime = (float)(seedTimeStart+injNum*seedTimeIncrement);
	float timeIncrement = (float)(maxFrame-minFrame)/(float)objectsPerFlowline;
	for (j = 0; j<maxPoints; j++){
		float* point = flowDataArray +3*(maxPoints*pathNum +j);
		//Determine the appropriate timestep for this point.
		float frameTime = seedTime + timeIncrement*j;
		
		if (point[0] == END_FLOW_FLAG || point[0] == IGNORE_FLAG || outOfBounds){
			//If past or before end, just fill with END_FLOW_FLAG...
			rc=fprintf(saveFile,"%8g %8g %8g %8g\n",
				END_FLOW_FLAG,END_FLOW_FLAG,END_FLOW_FLAG,
				frameTime);
			if (rc <= 0) return false;
			outOfBounds = true;
		} else { //otherwise just output the actual point
			rc = fprintf(saveFile,"%8g %8g %8g %8g\n",
				*point, *(point+1),*(point+2),
				frameTime);
			if (rc <= 0) return false;
		}
	}
	return true;
}




//Function to iterate over specified time steps, whether in list or 
//specified by interval
int FlowParams::
getUnsteadyTimestepSample(int index, int minStep, int maxStep){
	if (unsteadyFlowDirection > 0){
		if (useTimestepSampleList){
			int position = -1;
			for (int i = 0; i< unsteadyTimestepList.size(); i++){
				int ts = unsteadyTimestepList[i];
				if (ts >= minStep && ts <= maxStep) {
					position++; //count the number of acceptable steps 
					if (position < index) continue;
					return ts;
				}
				return -1;
			}
		} else { //samples specified by start, end, increment:
			assert(timeSamplingInterval > 0);
			int ts = timeSamplingStart;
			while (ts < minStep) ts += timeSamplingInterval;
			ts += index*timeSamplingInterval;
			if (ts > maxStep || ts > timeSamplingEnd) return -1;
			return ts;
		}
	} else { //direction is negative:
		if (useTimestepSampleList){
			int position = -1;
			for (int i = unsteadyTimestepList.size()-1; i >= 0; i--){
				int ts = unsteadyTimestepList[i];
				if (ts >= minStep && ts <= maxStep) {
					position++; //count the number of acceptable steps 
					if (position < index) continue;
					return ts;
				}
				return -1;
			}
		} else { //samples specified by start, end, increment:
			assert(timeSamplingInterval > 0);
			int ts = timeSamplingEnd;
			while (ts > timeSamplingStart && ((ts - timeSamplingStart)%timeSamplingInterval != 0)) ts--;
			while (ts > maxStep) ts -= timeSamplingInterval;
			ts -= index*timeSamplingInterval;
			if (ts < minStep || ts < timeSamplingStart) return -1;
			return ts;
		}
	}
	return -1; //can never happen
}
//Find the index that maps to the specified timestep.
//Indices are counting the time steps between minStep and maxStep
//Return -1 if none works.
//
int FlowParams::
getTimeSampleIndex(int tstep, int minStep, int maxStep){
	if (tstep < minStep || tstep > maxStep) return -1;
	if (useTimestepSampleList){
		int index = -1;
		for (int i = 0; i < unsteadyTimestepList.size();  i++){
			int ts = unsteadyTimestepList[i];
			if (ts < minStep) continue;
			if (ts > maxStep) return -1;
			if (ts > tstep) return -1;
			index++;
			if (ts < tstep) continue;
			//OK this is where it is in the 
			return index;
		}
	} else { // not using timestep list...
		assert(seedTimeIncrement > 0);
		//Make sure the tstep is in fact a valid index
		if (((tstep - seedTimeStart) % seedTimeIncrement) != 0) return -1;
		//value of index from seedTimeStart
		int index = (tstep - seedTimeStart)/seedTimeIncrement;
		//decrement to be above minStep
		if (minStep > seedTimeStart){
			int stepsSkipped = (minStep - seedTimeStart + seedTimeIncrement -1)/seedTimeIncrement;
			assert(stepsSkipped * seedTimeIncrement + seedTimeStart >= minStep);
			assert((stepsSkipped-1) * seedTimeIncrement + seedTimeStart < minStep);
			index -= stepsSkipped;
		}
		return index;
	}
	return -1;
}
//Generate steady flow data, either from a rake or from a seed list.
//Results go into container.
//If speeds are used for rgba mapping, they are (temporarily) calculated
//and then mapped.

FlowLineData* FlowParams::
regenerateSteadyFlowData(VaporFlow* myFlowLib, int timeStep, int minFrame, RegionParams* rParams){
	
	
	float* seedList = 0;
	if (!myFlowLib) return 0;
	
	//If we are doing autoscale, calculate the average field 
	//magnitude at lowest resolution, over the full domain:
	if (autoScale && magChanged){
		float avgMag = getAvgVectorMag(rParams, 0, timeStep);
		if (avgMag == 0.f) avgMag = 1.f;
		magChanged = false;
		//Scale the steady field so that it will go steadyFlowLength diameters 
		//in unit time
		
		const float* fullExtents = DataStatus::getInstance()->getExtents();
		float diff[3];
		vsub(fullExtents, fullExtents+3, diff);
		float diam = sqrt(vdot(diff, diff));
		steadyScale = diam*steadyFlowLength/avgMag;
		//And set the numsamples
		objectsPerFlowline = steadyFlowLength*steadySmoothness;
	}

	DataStatus* ds;
	//specify field components:
	ds = DataStatus::getInstance();
	const char* xVar, *yVar, *zVar;
	
	xVar = ds->getVariableName(steadyVarNum[0]).c_str();
	yVar = ds->getVariableName(steadyVarNum[1]).c_str();
	zVar = ds->getVariableName(steadyVarNum[2]).c_str();
	myFlowLib->SetSteadyFieldComponents(xVar, yVar, zVar);
	
	myFlowLib->SetPeriodicDimensions(periodicDim[0],periodicDim[1],periodicDim[2]);

	if (!setupFlowRegion(rParams, myFlowLib, timeStep, minFrame)) return 0;
	

	int numSeedPoints;
	if (doRake){
		numSeedPoints = getNumRakeSeedPoints();
		if (randomGen) {
			myFlowLib->SetRandomSeedPoints(seedBoxMin, seedBoxMax, (int)allGeneratorCount);
			
		} else {
			float boxmin[3], boxmax[3];
			for (int i = 0; i<3; i++){
				if (generatorCount[i] <= 1) {
					boxmin[i] = (seedBoxMin[i]+seedBoxMax[i])*0.5f;
					boxmax[i] = boxmin[i];
				} else {
					boxmin[i] = seedBoxMin[i];
					boxmax[i] = seedBoxMax[i];
				}
			}
			myFlowLib->SetRegularSeedPoints(boxmin, boxmax, generatorCount);
			
		}
	} else { //determine how many seed points to send to flowlib
		
		//For steady flow, count how many seeds will be in the array
		int seedCounter = 0;
		for (int i = 0; i<getNumListSeedPoints(); i++){
			float time = seedPointList[i].getVal(3);
			if ((time < 0.f) || ((int)(time+0.5f) == timeStep)){
				seedCounter++;
			}
		}
		numSeedPoints = seedCounter;
		if (numSeedPoints == 0) {
			MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "No seeds at current time step");
			return false;
		}
		seedCounter = 0;
		seedList = new float[numSeedPoints*3];
		for (int i = 0; i<getNumListSeedPoints(); i++){
			float time = seedPointList[i].getVal(3);
			if ((time < 0.f) || ((int)(time+0.5f) == timeStep)){
				for (int j = 0; j< 3; j++){
					seedList[3*seedCounter+j] = seedPointList[i].getVal(j);
				}
				seedCounter++;
			}
		}
		
	}
	// setup integration parameters:
	
	float minIntegStep = SMALLEST_MIN_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MIN_STEP; 
	float maxIntegStep = SMALLEST_MAX_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MAX_STEP; 
	
	
	myFlowLib->SetIntegrationParams(minIntegStep, maxIntegStep);

	//myFlowLib->SetTimeStepInterval(timeStep, maxFrame, timeSamplingInterval);
	myFlowLib->SetSteadyTimeSteps(timeStep, steadyFlowDirection); 
	
	
	myFlowLib->ScaleSteadyTimeStepSizes(steadyScale, 1.f/(float)objectsPerFlowline);
	
	//Note:  Following duplicates code in calcMaxPoints()
	maxPoints = objectsPerFlowline+1;
	if (maxPoints < 2) maxPoints = 2;
	if (steadyFlowDirection == 0 && maxPoints < 4) maxPoints = 4;

	bool useSpeeds =  (getColorMapEntityIndex() == 2 || getOpacMapEntityIndex() == 2);
	bool doRGBAs = (getColorMapEntityIndex() + getOpacMapEntityIndex() > 0);

	FlowLineData* flowData = new FlowLineData(numSeedPoints, maxPoints, useSpeeds, steadyFlowDirection, doRGBAs); 
		
	numInjections = 1;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
	bool rc = true;
	
	if (doRake){
		rc = myFlowLib->GenStreamLines(flowData, randomSeed);
	} else {
		rc = myFlowLib->GenStreamLinesNoRake(flowData,seedList);
	}

	if (!rc){
		MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "Error integrating steady flow lines");	
		delete flowData;
		return 0;
	}


	//Restore original cursor:
	QApplication::restoreOverrideCursor();
	//Now map colors (if needed)
		
	if (doRGBAs){
		mapColors(flowData, timeStep, minFrame);
		//Now we can release the speeds:
		if (useSpeeds) flowData->releaseSpeeds(); 
	}

	return flowData;
}
//Generate steady flow data, like above method, but uses seeds in the pathData container.
//Produces a FlowLineData containing the flow lines.
//If speeds are used for rgba mapping, they are (temporarily) calculated
//and then mapped.
//If the pathData is null, we get the seeds from the rake or seedlist
FlowLineData* FlowParams::
regenerateSteadyFieldLines(VaporFlow* myFlowLib, PathLineData* pathData, int timeStep, int minFrame, RegionParams* rParams, bool prioritize){
	
	
	float* seedList = 0;
	if (!myFlowLib) return 0;
	
	//If we are doing autoscale, calculate the average field 
	//magnitude at lowest resolution, over the full domain:
	if (autoScale && magChanged){
		float avgMag = getAvgVectorMag(rParams, 0, timeStep);
		if (avgMag == 0.f) avgMag = 1.f;
		magChanged = false;
		//Scale the steady field so that it will go steadyFlowLength diameters 
		//in unit time
		
		const float* fullExtents = DataStatus::getInstance()->getExtents();
		float diff[3];
		vsub(fullExtents, fullExtents+3, diff);
		float diam = sqrt(vdot(diff, diff));
		steadyScale = diam*steadyFlowLength/avgMag;
		//And set the numsamples
		objectsPerFlowline = steadyFlowLength*steadySmoothness;
	}

	DataStatus* ds;
	//specify field components:
	ds = DataStatus::getInstance();
	const char* xVar, *yVar, *zVar;
	
	xVar = ds->getVariableName(steadyVarNum[0]).c_str();
	yVar = ds->getVariableName(steadyVarNum[1]).c_str();
	zVar = ds->getVariableName(steadyVarNum[2]).c_str();
	myFlowLib->SetSteadyFieldComponents(xVar, yVar, zVar);
	
	myFlowLib->SetPeriodicDimensions(periodicDim[0],periodicDim[1],periodicDim[2]);
	
	if (!setupFlowRegion(rParams, myFlowLib, timeStep, minFrame)) return 0;
	
	int numSeedPoints;
	if (pathData)numSeedPoints = pathData->getNumLines(timeStep);
	else {// get the seed points from the rake or seedlist...
		if (doRake){
			numSeedPoints = getNumRakeSeedPoints();
			if (randomGen) {
				myFlowLib->SetRandomSeedPoints(seedBoxMin, seedBoxMax, (int)allGeneratorCount);
				
			} else {
				float boxmin[3], boxmax[3];
				for (int i = 0; i<3; i++){
					if (generatorCount[i] <= 1) {
						boxmin[i] = (seedBoxMin[i]+seedBoxMax[i])*0.5f;
						boxmax[i] = boxmin[i];
					} else {
						boxmin[i] = seedBoxMin[i];
						boxmax[i] = seedBoxMax[i];
					}
				}
				myFlowLib->SetRegularSeedPoints(boxmin, boxmax, generatorCount);
				
			}
		} else { //determine how many seed points to send to flowlib
			
			//For steady flow, count how many seeds will be in the array
			int seedCounter = 0;
			for (int i = 0; i<getNumListSeedPoints(); i++){
				float time = seedPointList[i].getVal(3);
				if ((time < 0.f) || ((int)(time+0.5f) == timeStep)){
					seedCounter++;
				}
			}
			numSeedPoints = seedCounter;
			if (numSeedPoints == 0) {
				MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "No seeds at current time step");
				return false;
			}
			seedCounter = 0;
			seedList = new float[numSeedPoints*3];
			for (int i = 0; i<getNumListSeedPoints(); i++){
				float time = seedPointList[i].getVal(3);
				if ((time < 0.f) || ((int)(time+0.5f) == timeStep)){
					for (int j = 0; j< 3; j++){
						seedList[3*seedCounter+j] = seedPointList[i].getVal(j);
					}
					seedCounter++;
				}
			}
			
		}
	}

	// setup integration parameters:
	
	float minIntegStep = SMALLEST_MIN_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MIN_STEP; 
	float maxIntegStep = SMALLEST_MAX_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MAX_STEP; 
	
	
	myFlowLib->SetIntegrationParams(minIntegStep, maxIntegStep);

	//myFlowLib->SetTimeStepInterval(timeStep, maxFrame, timeSamplingInterval);
	myFlowLib->SetSteadyTimeSteps(timeStep, steadyFlowDirection); 
	
	
	myFlowLib->ScaleSteadyTimeStepSizes(steadyScale, 1.f/(float)objectsPerFlowline);
	
	//Note:  Following duplicates code in calcMaxPoints()
	maxPoints = objectsPerFlowline+1;
	if (maxPoints < 2) maxPoints = 2;
	if (steadyFlowDirection == 0 && maxPoints < 4) maxPoints = 4;

	bool useSpeeds =  (getColorMapEntityIndex() == 2 || getOpacMapEntityIndex() == 2);
	bool doRGBAs = (getColorMapEntityIndex() + getOpacMapEntityIndex() > 0);


	FlowLineData* steadyFlowData = new FlowLineData(numSeedPoints, maxPoints, useSpeeds, steadyFlowDirection, doRGBAs); 
		
	numInjections = 1;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
	bool rc;
	if (pathData){
	//generate the steady flow lines 
		rc =  myFlowLib->GenStreamLines(steadyFlowData, pathData, timeStep, prioritize);
	} else { // use rake or seed list..
		if (doRake){
			rc = myFlowLib->GenStreamLines(steadyFlowData, randomSeed);
		} else {
			rc = myFlowLib->GenStreamLinesNoRake(steadyFlowData,seedList);
		}
	}
	if (!rc){
		MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "Error integrating steady flow lines");	
		delete steadyFlowData;
		return 0;
	}


	//Restore original cursor:
	QApplication::restoreOverrideCursor();
	//Now map colors (if needed)
		
	if (doRGBAs){
		mapColors(steadyFlowData, timeStep, minFrame);
		//Now we can release the speeds:
		if (useSpeeds) steadyFlowData->releaseSpeeds(); 
	}

	return steadyFlowData;
}
////////////////////////////////////////////////////////////
//  Method to create a new PathLineData for unsteady flow, and
//  to insert the appropriate seeds into it.
//  Does setup of flow lib for unsteady advection
//  Returns null if there are no seeds, or if unable to allocate memory
///////////////////////////////////////////////////////////
PathLineData* FlowParams::
setupPathLineData(VaporFlow* flowLib, int minFrame, int maxFrame, RegionParams* rParams){
	//The number of lines is the total number of seeds to be inserted.
	int numLines = 0;
	//The max number of points in the unsteady flow
	int mPoints = objectsPerTimestep*(maxFrame - minFrame + 1);

	//build the unsteady timestep sample list.  This lists the timesteps to be sampled
	//in the order of integration.
	int numTimesteps;
	int* timeStepList;
	if (useTimestepSampleList) {
		numTimesteps = unsteadyTimestepList.size();
		timeStepList = new int[numTimesteps];
		for (int i = 0; i<numTimesteps; i++){
			timeStepList[i] = unsteadyTimestepList[i];
		}
	}
	else {
		numTimesteps = 1+ (timeSamplingEnd - timeSamplingStart)/timeSamplingInterval;
		timeStepList = new int[numTimesteps];
		for (int i = 0; i< numTimesteps; i++){
			timeStepList[i] = timeSamplingStart + i*timeSamplingInterval;
		}
	}
	if (numTimesteps < 2){
		MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "Insufficient time steps for unsteady advection");
		delete timeStepList;
		return false;
	}
	if (!setupFlowRegion(rParams, flowLib, -1, minFrame)) return 0;
	// setup integration parameters:
	
	float minIntegStep = SMALLEST_MIN_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MIN_STEP; 
	float maxIntegStep = SMALLEST_MAX_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MAX_STEP; 
	
	
	flowLib->SetIntegrationParams(minIntegStep, maxIntegStep);
	if (unsteadyFlowDirection < 0){// Invert the list, so it's ordered in integration order
		for (int i = 0; i< numTimesteps/2; i++){
			//swap each pair:
			int saveIndex = timeStepList[i];
			timeStepList[i] = timeStepList[numTimesteps-i-1];
			timeStepList[numTimesteps-i-1] = saveIndex;
		}
	} 
	
	const char* xVar, *yVar, *zVar;
	DataStatus* ds = DataStatus::getInstance();
	xVar = ds->getVariableName(unsteadyVarNum[0]).c_str();
	yVar = ds->getVariableName(unsteadyVarNum[1]).c_str();
	zVar = ds->getVariableName(unsteadyVarNum[2]).c_str();
	flowLib->SetUnsteadyFieldComponents(xVar, yVar, zVar);
	
	flowLib->SetUnsteadyTimeSteps(timeStepList,numTimesteps, (unsteadyFlowDirection > 0));
	
	flowLib->ScaleUnsteadyTimeStepSizes(unsteadyScale, (float)objectsPerTimestep);

	float sampleRate = (float)objectsPerTimestep;
	
	//Need to set up flowlib for rake seed generation
	if (doRake){
		
		if (randomGen) {
			flowLib->SetRandomSeedPoints(seedBoxMin, seedBoxMax, (int)allGeneratorCount);
		} else {
			float boxmin[3], boxmax[3];
			for (int i = 0; i<3; i++){
				if (generatorCount[i] <= 1) {
					boxmin[i] = (seedBoxMin[i]+seedBoxMax[i])*0.5f;
					boxmax[i] = boxmin[i];
				} else {
					boxmin[i] = seedBoxMin[i];
					boxmax[i] = seedBoxMax[i];
				}
			}
			flowLib->SetRegularSeedPoints(boxmin, boxmax, generatorCount);
			
		}
	}
	
	if (flowType == 2){ //field line advection only inserts at first seed frame:
		int numLines = calcNumSeedPoints(seedTimeStart);
		//In field line advection, we don't store speeds or colors in the path line data,
		//just in the steady flowLineData
		PathLineData* pathLines = new PathLineData(numLines, mPoints, false, false, minFrame, maxFrame, sampleRate);
		// Now insert the seeds:
		int seedsInserted = insertSeeds(flowLib, pathLines, seedTimeStart);
		if (seedsInserted <= 0) { 
			delete pathLines;
			return 0;
		}
		return pathLines;
	} else { //Unsteady flow; Insert at all seed frames (independent of sampling)
		//Count the number of lines (i.e. seeds) for all seed times.
		for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
			if (i < minFrame || i > maxFrame) continue;
			numLines += calcNumSeedPoints(i);
		}
		if (numLines <= 0) return 0;
		bool useSpeeds = (getColorMapEntityIndex() == 2 || getOpacMapEntityIndex() == 2);
		bool doRGBAs = (getColorMapEntityIndex() > 0 || getOpacMapEntityIndex() > 0);
		PathLineData* pathLines = new PathLineData(numLines, mPoints, useSpeeds, doRGBAs, minFrame, maxFrame, sampleRate);
		// Now insert the seeds:
		int seedsInserted = 0;
		for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
			//Don't insert if invalid timestep 
			if (i < minFrame || i > maxFrame) continue;
			seedsInserted += insertSeeds(flowLib, pathLines, i);
		}
		if (seedsInserted <= 0) { 
			delete pathLines;
			return 0;
		}
		return pathLines;
	}
}
//Insert seeds for the specified timeStep.
//Return the actual number of seeds that were inserted and were inside the current region 
//If the seed list is being used, only use seeds that are valid at the specified
//timestep
int FlowParams::insertSeeds(VaporFlow* fLib, PathLineData* pathLines, int timeStep){
	int seedCount = 0;
	if(doRake){
		//Allocate an array to hold the seeds:
		float* seeds = new float[3*calcNumSeedPoints(timeStep)];
		seedCount = fLib->GenRakeSeeds(seeds, timeStep, randomSeed);
		if (seedCount < calcNumSeedPoints(timeStep)) {
			delete seeds;
			return 0;
		}
		for (int i = 0; i<seedCount; i++){
			pathLines->insertSeedAtTime(i, timeStep, seeds[3*i], seeds[3*i+1], seeds[3*i+2]);
		}
		delete seeds;
		return seedCount;

	} else { //insert from seedList
		
		for (int i = 0; i< getNumListSeedPoints(); i++){
			Point4 point = seedPointList[i];
			float tstep = point.getVal(3);
			if (tstep >= 0.f && ((int)(tstep +0.5f) != timeStep)) continue;
			pathLines->insertSeedAtTime(i, timeStep, point.getVal(0), point.getVal(1), point.getVal(2));
			seedCount++;
		}
	}
	return seedCount;

}

//Method to construct Xml for state saving
XmlNode* FlowParams::
buildNode() {
	DataStatus* ds;
	ds = DataStatus::getInstance();
	//Construct the flow node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	oss << (long)numComboVariables;
	attrs[_numVariablesAttr] = oss.str();

	oss.str(empty);
	oss << (long)numRefinements;
	attrs[_numTransformsAttr] = oss.str();

	oss.str(empty);
	oss << (double)integrationAccuracy;
	attrs[_integrationAccuracyAttr] = oss.str();

	oss.str(empty);
	oss << (double)steadyScale;
	attrs[_steadyScaleAttr] = oss.str();

	oss.str(empty);
	oss << (double)unsteadyScale;
	attrs[_unsteadyScaleAttr] = oss.str();

	oss.str(empty);
	oss << (double)integrationAccuracy;
	attrs[_integrationAccuracyAttr] = oss.str();

	oss.str(empty);
	oss << (double)steadySmoothness;
	attrs[_smoothnessAttr] = oss.str();

	oss.str(empty);
	oss << (double)steadyFlowLength;
	attrs[_steadyFlowLengthAttr] = oss.str();

	oss.str(empty);
	oss << (double)steadyFlowDirection;
	attrs[_steadyFlowDirectionAttr] = oss.str();

	oss.str(empty);
	oss << (double)unsteadyFlowDirection;
	attrs[_unsteadyFlowDirectionAttr] = oss.str();

	oss.str(empty);
	oss << (long)timeSamplingStart<<" "<<timeSamplingEnd<<" "<<timeSamplingInterval;
	attrs[_timeSamplingAttr] = oss.str();

	oss.str(empty);
	for (int i = 0; i< 3; i++){
		if (periodicDim[i]) oss << "true ";
		else oss << "false ";
	}
	attrs[_periodicDimsAttr] = oss.str();

	oss.str(empty);
	if (autoRefresh)
		oss << "true";
	else 
		oss << "false";
	attrs[_autoRefreshAttr] = oss.str();
	
	oss.str(empty);
	if (autoScale)
		oss << "true";
	else 
		oss << "false";
	attrs[_autoScaleAttr] = oss.str();

	oss.str(empty);
	if (flowType == 0) oss << "true";
	else oss << "false";
	attrs[_steadyFlowAttr] = oss.str();

	oss.str(empty);
	oss << ds->getVariableName(steadyVarNum[0])<<" "<<ds->getVariableName(steadyVarNum[1])<<" "<<ds->getVariableName(steadyVarNum[2]);
	attrs[_steadyVariableNamesAttr] = oss.str();
	
	oss.str(empty);
	oss << ds->getVariableName(unsteadyVarNum[0])<<" "<<ds->getVariableName(unsteadyVarNum[1])<<" "<<ds->getVariableName(unsteadyVarNum[2]);
	attrs[_unsteadyVariableNamesAttr] = oss.str();

	XmlNode* flowNode = new XmlNode(_flowParamsTag, attrs, 2+numComboVariables);

	//Now add children:  
	//There's a child for geometry, a child for
	//Seeding, and a child for each variable.
	//The geometry child contains the mapperFunction
	
	
	attrs.clear();

	oss.str(empty);
	oss << (double)seedBoxMin[0]<<" "<<(double)seedBoxMin[1]<<" "<<(double)seedBoxMin[2];
	attrs[_seedRegionMinAttr] = oss.str();

	oss.str(empty);
	oss << (double)seedBoxMax[0]<<" "<<(double)seedBoxMax[1]<<" "<<(double)seedBoxMax[2];
	attrs[_seedRegionMaxAttr] = oss.str();

	oss.str(empty);
	oss << (long)generatorCount[0]<<" "<<(long)generatorCount[1]<<" "<<(long)generatorCount[2];
	attrs[_generatorCountsAttr] = oss.str();
	
	oss.str(empty);
	oss << (long)seedTimeStart<<" "<<(long)seedTimeEnd<<" "<<(long)seedTimeIncrement;
	attrs[_seedTimesAttr] = oss.str();

	oss.str(empty);
	oss << (long)allGeneratorCount;
	attrs[_totalGeneratorCountAttr] = oss.str();

	oss.str(empty);
	if (randomGen)
		oss << "true";
	else 
		oss << "false";
	attrs[_randomGenAttr] = oss.str();

	oss.str(empty);
	oss << (unsigned long)randomSeed;
	attrs[_randomSeedAttr] = oss.str();

	oss.str(empty);
	if (doRake)
		oss << "true";
	else
		oss << "false";
	attrs[_useRakeAttr] = oss.str();
	oss.str(empty);

	XmlNode* seedingNode = new XmlNode(_seedingTag,attrs,getNumListSeedPoints());

	//Add a node for each seed point:
	for (int i = 0; i<getNumListSeedPoints(); i++){
		attrs.clear();
		oss.str(empty);
		oss << (double)seedPointList[i].getVal(0) << " " <<
			(double)seedPointList[i].getVal(1) << " " <<
			(double)seedPointList[i].getVal(2);
		attrs[_positionAttr] = oss.str();
		oss.str(empty);
		oss << (double)seedPointList[i].getVal(3);
		attrs[_timestepAttr] = oss.str();
		XmlNode* seedNode = new XmlNode(_seedPointTag, attrs, 0);
		seedingNode->AddChild(seedNode);
	}

	flowNode->AddChild(seedingNode);

	//Now do graphic parameters.  
	
	
	attrs.clear();
	oss.str(empty);
	oss << (long)geometryType;
	attrs[_geometryTypeAttr] = oss.str();
	oss.str(empty);
	oss << (int)objectsPerFlowline;
	attrs[_objectsPerFlowlineAttr] = oss.str();
	oss.str(empty);
	oss << (int)objectsPerTimestep;
	attrs[_objectsPerTimestepAttr] = oss.str();
	oss.str(empty);
	oss << (long)firstDisplayFrame <<" "<<(long)lastDisplayFrame;
	attrs[_displayIntervalAttr] = oss.str();
	oss.str(empty);
	oss << (float)shapeDiameter;
	attrs[_shapeDiameterAttr] = oss.str();
	oss.str(empty);
	oss << (float)arrowDiameter;
	attrs[_arrowDiameterAttr] = oss.str();
	oss.str(empty);
	oss << getColorMapEntityIndex();
	attrs[_colorMappedEntityAttr] = oss.str();
	oss.str(empty);
	oss << getOpacMapEntityIndex();
	attrs[_opacityMappedEntityAttr] = oss.str();
	oss.str(empty);
	int r = qRed(constantColor);
	int g = qGreen(constantColor);
	int b = qBlue(constantColor);
	oss << r << " " << g << " " << b;
	attrs[_constantColorAttr] = oss.str();
	oss.str(empty);
	oss << (double)constantOpacity;
	attrs[_constantOpacityAttr] = oss.str();

	//Specify the opacity scale from the transfer function
	if(mapperFunction){
		oss.str(empty);
		oss << mapperFunction->getOpacityScaleFactor();
		attrs[_opacityScaleAttr] = oss.str();
	}

	XmlNode* graphicNode = new XmlNode(_geometryTag,attrs,2*numComboVariables+1);

	//Create a mapper function node, add it as child
	if(mapperFunction) {
		XmlNode* mfNode = mapperFunction->buildNode(empty);
		graphicNode->AddChild(mfNode);
	}
	


	flowNode->AddChild(graphicNode);
	//Create a node for each of the variables
	for (int i = 0; i< numComboVariables+4; i++){
		map <string, string> varAttrs;
		oss.str(empty);
		if (i > 3){
			oss << ds->getVariableName(i-4);
		} else {
			if (i == 0) oss << "Constant";
			else if (i == 1) oss << "Timestep";
			else if (i == 2) oss << "FieldMagnitude";
			else if (i == 3) oss << "SeedIndex";
		}
		varAttrs[_variableNameAttr] = oss.str();

		
		oss.str(empty);
		oss << (double)minColorBounds[i];
		varAttrs[_leftColorBoundAttr] = oss.str();
		oss.str(empty);
		oss << (double)maxColorBounds[i];
		varAttrs[_rightColorBoundAttr] = oss.str();
		oss.str(empty);
		oss << (double)minOpacBounds[i];
		varAttrs[_leftOpacityBoundAttr] = oss.str();
		oss.str(empty);
		oss << (double)maxOpacBounds[i];
		varAttrs[_rightOpacityBoundAttr] = oss.str();
		flowNode->NewChild(_variableTag, varAttrs, 0);
	}
	
	
	
	return flowNode;
}
//Parse flow params from file:
bool FlowParams::
elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tagString, const char ** attrs){
	
	//Take care of attributes of flowParamsNode
	if (StrCmpNoCase(tagString, _flowParamsTag) == 0) {
		//Start with a new, default mapperFunction
		if (mapperFunction) delete mapperFunction;
		mapperFunction = new MapperFunction(this, 8);

		int newNumVariables = 0;
		//Default autoscale to false, consistent with pre-1.2
		autoScale = false;
		
		steadyFlowLength = 1.f;
		steadySmoothness = 20.f;
		steadyFlowDirection = 1;  //Forward was default for previous versions
		unsteadyFlowDirection = 1;
		//If it's a Flow tag, save 11 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _numVariablesAttr) == 0) {
				ist >> newNumVariables;
			}
			
			else if (StrCmpNoCase(attribName, _mappedVariableNamesAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the obsolete name
					int varnum = DataStatus::getInstance()->mergeVariableName(vName);
					steadyVarNum[i] = varnum;
					unsteadyVarNum[i] = varnum;
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboUnsteadyVarNum[i] = -1;
					comboSteadyVarNum[i] = -1;
				}
			}
			else if (StrCmpNoCase(attribName, _steadyVariableNamesAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the steady name
					int varnum = DataStatus::getInstance()->mergeVariableName(vName);
					steadyVarNum[i] = varnum;
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboSteadyVarNum[i] = -1;
				}
			}
			else if (StrCmpNoCase(attribName, _unsteadyVariableNamesAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the unsteady name
					int varnum = DataStatus::getInstance()->mergeVariableName(vName);
					unsteadyVarNum[i] = varnum;
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboUnsteadyVarNum[i] = -1;	
				}
			}
			else if (StrCmpNoCase(attribName, _periodicDimsAttr) == 0){
				string boolVal;
				for (int i = 0; i<3; i++){
					ist >> boolVal;
					if (boolVal == "true") setPeriodicDim(i,true);
					else setPeriodicDim(i,false);
				}
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				//ignore
			}
			else if (StrCmpNoCase(attribName, _autoRefreshAttr) == 0) {
				if (value == "true") autoRefresh = true; else autoRefresh = false;
			}
			else if (StrCmpNoCase(attribName, _autoScaleAttr) == 0) {
				if (value == "true") autoScale = true; else autoScale = false;
			}
			else if (StrCmpNoCase(attribName, _steadyFlowAttr) == 0) {
				if (value == "true") setFlowType(0); else setFlowType(1);
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			else if (StrCmpNoCase(attribName, _integrationAccuracyAttr) == 0){
				ist >> integrationAccuracy;
			}
			else if (StrCmpNoCase(attribName, _steadyFlowDirectionAttr) == 0){
				ist >> steadyFlowDirection;
			}
			else if (StrCmpNoCase(attribName, _unsteadyFlowDirectionAttr) == 0){
				ist >> unsteadyFlowDirection;
			}
			else if (StrCmpNoCase(attribName, _velocityScaleAttr) == 0){
				ist >> steadyScale;
				ist >> unsteadyScale;
			}
			else if (StrCmpNoCase(attribName, _steadyScaleAttr) == 0){
				ist >> steadyScale;
			}
			else if (StrCmpNoCase(attribName, _unsteadyScaleAttr) == 0){
				ist >> unsteadyScale;
			}
			else if (StrCmpNoCase(attribName, _timeSamplingAttr) == 0){
				ist >> timeSamplingStart;ist >>timeSamplingEnd; ist>>timeSamplingInterval;
			}
			else if (StrCmpNoCase(attribName, _smoothnessAttr) == 0) {
				ist >> steadySmoothness;
			}
			else if (StrCmpNoCase(attribName, _steadyFlowLengthAttr) == 0) {
				ist >> steadyFlowLength;
			}
			else return false;
		}
		//Reset the variable Names, Nums, bounds.  These will be initialized
		//by the geometry nodes
		numComboVariables = newNumVariables;
		
		delete minOpacBounds;
		delete maxOpacBounds;
		delete minColorBounds;
		delete maxColorBounds;
		minOpacBounds = new float[numComboVariables+4];
		maxOpacBounds = new float[numComboVariables+4];
		minColorBounds = new float[numComboVariables+4];
		maxColorBounds = new float[numComboVariables+4];
		return true;
	}
	//Parse the seeding node:
	else if (StrCmpNoCase(tagString, _seedingTag) == 0) {
		//Clear out the seeds, will be added in later if they are found.
		seedPointList.clear();
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
		
			if (StrCmpNoCase(attribName, _seedRegionMinAttr) == 0) {
				ist >> seedBoxMin[0];ist >> seedBoxMin[1];ist >> seedBoxMin[2];
			}
			else if (StrCmpNoCase(attribName, _seedRegionMaxAttr) == 0) {
				
				ist >> seedBoxMax[0];ist >> seedBoxMax[1];ist >> seedBoxMax[2];
			}
			else if (StrCmpNoCase(attribName, _randomGenAttr) == 0) {
				if (value == "true") setRandom(true); else setRandom(false);
			}
			else if (StrCmpNoCase(attribName, _randomSeedAttr) == 0) {
				ist >> randomSeed;
			}
			else if (StrCmpNoCase(attribName, _generatorCountsAttr) == 0) {
				ist >> generatorCount[0];ist >> generatorCount[1];ist >> generatorCount[2];
			}
			else if (StrCmpNoCase(attribName, _totalGeneratorCountAttr) == 0) {
				ist >> allGeneratorCount;
			}
			else if (StrCmpNoCase(attribName, _seedTimesAttr) == 0) {
				ist >> seedTimeStart;
				ist >> seedTimeEnd;
				ist >> seedTimeIncrement;
			}
			else if (StrCmpNoCase(attribName, _useRakeAttr) == 0) {
				if (value == "true") doRake = true; else doRake = false;
			}
			else if (StrCmpNoCase(attribName, _useSeedListAttr) == 0) {
				if (value == "true") doRake = false; else doRake = true;
			}
			else return false;
		}
		return true;
	}
	//Parse the seed point nodes:
	else if (StrCmpNoCase(tagString, _seedPointTag) == 0){
		//Create a new point
		float psn[4] = {0.f,0.f,0.f,0.f};
		Point4 pt;

		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);

			if (StrCmpNoCase(attribName, _positionAttr) == 0) {
				ist >> psn[0]; ist >>psn[1]; ist >> psn[2];
			}
			else if (StrCmpNoCase(attribName, _timestepAttr) == 0) {
				ist >> psn[3];
			}
			else return false;
		}
		pt.setVal(psn);
		seedPointList.push_back(pt);
	}
	// Parse the geometry node:
	else if (StrCmpNoCase(tagString, _geometryTag) == 0) {
		arrowDiameter = 2.f; //Default value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _geometryTypeAttr) == 0) {
				ist >> geometryType;
			}
			else if (StrCmpNoCase(attribName, _objectsPerFlowlineAttr) == 0) {
				ist >> objectsPerFlowline;
			}
			else if (StrCmpNoCase(attribName, _objectsPerTimestepAttr) == 0) {
				ist >> objectsPerTimestep;
			}
			else if (StrCmpNoCase(attribName, _displayIntervalAttr) == 0) {
				ist >> firstDisplayFrame;ist >> lastDisplayFrame;
			}
			else if (StrCmpNoCase(attribName, _shapeDiameterAttr) == 0) {
				ist >> shapeDiameter;
			}
			else if (StrCmpNoCase(attribName, _arrowDiameterAttr) == 0) {
				ist >> arrowDiameter;
			}
			else if (StrCmpNoCase(attribName, _colorMappedEntityAttr) == 0) {
				int indx;
				ist >> indx;
				setColorMapEntity(indx);
			}
			else if (StrCmpNoCase(attribName, _opacityMappedEntityAttr) == 0) {
				int indx;
				ist >> indx;
				setOpacMapEntity(indx);
			}
			else if (StrCmpNoCase(attribName, _constantColorAttr) == 0){
				int r,g,b;
				ist >> r; ist >>g; ist >> b;
				constantColor = qRgb(r,g,b);
			} 
			else if (StrCmpNoCase(attribName, _opacityScaleAttr) == 0){
				float opacScale;
				ist >> opacScale;
				mapperFunction->setOpacityScaleFactor(opacScale);
			} 
			else if (StrCmpNoCase(attribName, _constantOpacityAttr) == 0){
				ist >> constantOpacity;
			}
			
			else return false;
		}
	//Parse a mapperFunction node (inside the geometry node):
	} else if (StrCmpNoCase(tagString, MapperFunction::_mapperFunctionTag) == 0) {
		//Need to "push" to mapper function parser.
		//That parser will "pop" back to flowparams when done.
		
		pm->pushClassStack(mapperFunction);
		mapperFunction->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	//Parse variableMapping tags:
	} else if (StrCmpNoCase(tagString,_variableTag) == 0){
		//Save the attributes:
		string varName = "";
		
		float leftColorBound = 0.f;
		float rightColorBound = 1.f;
		float leftOpacBound = 0.f;
		float rightOpacBound = 1.f;
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _variableNameAttr) == 0) {
				ist >> varName;
			}
			else if (StrCmpNoCase(attribName, _leftColorBoundAttr) == 0) {
				ist >> leftColorBound;
			}
			else if (StrCmpNoCase(attribName, _rightColorBoundAttr) == 0) {
				ist >> rightColorBound;
			}
			else if (StrCmpNoCase(attribName, _leftOpacityBoundAttr) == 0) {
				ist >> leftOpacBound;
			}
			else if (StrCmpNoCase(attribName, _rightOpacityBoundAttr) == 0) {
				ist >> rightOpacBound;
			}
			else return false;
		}
		//Plug in these values:
		//Check first for predefined names.  Same ones are used for opacity and
		//for color
		int varNum = -1;
		if(StrCmpNoCase(varName, "Constant") == 0) varNum = 0;
		else 	if (StrCmpNoCase(varName, "Timestep") == 0) varNum = 1;
		else 	if (StrCmpNoCase(varName, "FieldMagnitude") == 0) varNum = 2;
		else 	if (StrCmpNoCase(varName, "SeedIndex") == 0) varNum = 3;
		else {
			varNum = 4+ DataStatus::getInstance()->mergeVariableName(varName);
			if (varNum > numComboVariables+4) {
				pm->parseError("Invalid variable name in FlowParams: %s", varName.c_str());
				return false;
			}
		}
		minColorBounds[varNum]= leftColorBound;
		maxColorBounds[varNum] = rightColorBound;
		minOpacBounds[varNum]= leftOpacBound;
		maxOpacBounds[varNum] = rightOpacBound;
	}
	return true;
}
	
	
bool FlowParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	if (StrCmpNoCase(tag, _flowParamsTag) == 0) {
		
		setMinColorEditBound(getMinColorMapBound(),getColorMapEntityIndex());
		setMaxColorEditBound(getMaxColorMapBound(),getColorMapEntityIndex());
		setMinOpacEditBound(getMinOpacMapBound(),getOpacMapEntityIndex());
		setMaxOpacEditBound(getMaxOpacMapBound(),getOpacMapEntityIndex());
		
		//If this is a flowparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} 
	else if (StrCmpNoCase(tag, _seedingTag) == 0)
		return true;
	else if (StrCmpNoCase(tag, _seedPointTag) == 0)
		return true;
	else if (StrCmpNoCase(tag, _geometryTag) == 0)
		return true;
	else if (StrCmpNoCase(tag, _variableTag) == 0)
		return true;
	else if (StrCmpNoCase(tag, MapperFunction::_mapperFunctionTag) == 0) {
		return true;
	} else {
		pm->parseError("Unrecognized end tag in FlowParams %s",tag.c_str());
		return false; 
	}
}
	

//Generate a list of colors and opacities, one per (valid) vertex.
//The number of points is maxPoints*numSeedings*numInjections
//The flowData parameter is either the rakeFlowData or the listFlowData
//For the specific time step.
//Note that, if a variable is mapped, only the first time step is used for
//the mapping
//
void FlowParams::
mapColors(float* speeds, int currentTimeStep, int minFrame, int numSeeds, float* flowData, float* flowRGBAs, bool isRake){
	//Create lut based on current mapping data
	float* lut = new float[256*4];
	mapperFunction->makeLut(lut);
	//Setup mapping
	
	float opacMin = mapperFunction->getMinOpacMapValue();
	float colorMin = mapperFunction->getMinColorMapValue();
	float opacMax = mapperFunction->getMaxOpacMapValue();
	float colorMax = mapperFunction->getMaxColorMapValue();

	float opacVar, colorVar;
	float* opacRegion, *colorRegion;
	float opacVarMin[3], opacVarMax[3], colorVarMin[3], colorVarMax[3];
	int opacSize[3],colorSize[3];
	DataStatus* ds = DataStatus::getInstance();
		
	//Get the variable (entire region) if needed
	if (getOpacMapEntityIndex() > 3){
		//set up args for GetRegion
		//If flow is unsteady, just get the first available timestep
		int timeStep = currentTimeStep;
		if(flowType != 0){//unsteady flow
			timeStep = ds->getFirstTimestep(getOpacMapEntityIndex()-4);
			if (timeStep < 0) MyBase::SetErrMsg("No data for mapped variable");
		}
		size_t minSize[3];
		size_t maxSize[3];
		const size_t *bs = DataStatus::getInstance()->getDataMgr()->GetMetadata()->GetBlockSize();
		for (int i = 0; i< 3; i++){
			minSize[i] = 0;
			opacSize[i] = (int)((ds->getFullDataSize(i) >> (ds->getNumTransforms() - numRefinements)));
			maxSize[i] = opacSize[i]/bs[i] -1;
			opacVarMin[i] = DataStatus::getInstance()->getExtents()[i];
			opacVarMax[i] = DataStatus::getInstance()->getExtents()[i+3];
		}
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		opacRegion = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
			opacMapEntity[getOpacMapEntityIndex()].c_str(),
			numRefinements, (size_t*) minSize, (size_t*) maxSize, 0);
		QApplication::restoreOverrideCursor();
	}
	if (getColorMapEntityIndex() > 3){
		//set up args for GetRegion
		int timeStep = ds->getFirstTimestep(getColorMapEntityIndex()-4);
		if (timeStep < 0) MyBase::SetErrMsg("No data for mapped variable");
		size_t minSize[3];
		size_t maxSize[3];
		const size_t *bs = DataStatus::getInstance()->getDataMgr()->GetMetadata()->GetBlockSize();
		for (int i = 0; i< 3; i++){
			minSize[i] = 0;
			colorSize[i] = (int)((ds->getFullDataSize(i) >> (ds->getNumTransforms() - numRefinements)));
			maxSize[i] = (colorSize[i]/bs[i] - 1);
			colorVarMin[i] = DataStatus::getInstance()->getExtents()[i];
			colorVarMax[i] = DataStatus::getInstance()->getExtents()[i+3];
		}
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		colorRegion = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
			colorMapEntity[getColorMapEntityIndex()].c_str(),
			numRefinements, (size_t*) minSize, (size_t*) maxSize, 0);
		QApplication::restoreOverrideCursor();

	}
	
	//Cycle through all the points.  Map to rgba as we go.
	//
	int firstSteadyStep=0, lastSteadyStep=0;
	if (steadyFlowDirection < 0){
		firstSteadyStep = -steadyFlowLength;
	} else if (steadyFlowDirection > 0){
		lastSteadyStep = steadyFlowLength;
	} else {
		firstSteadyStep = -steadyFlowLength/2;
		lastSteadyStep = steadyFlowLength/2;
	}

	

	for (int i = 0; i<numInjections; i++){
		for (int j = 0; j<numSeeds; j++){
			for (int k = 0; k<maxPoints; k++) {
				//check for end of flow:
				if (flowData[3*(k+ maxPoints*(j+ (numSeeds*i)))] == END_FLOW_FLAG)
					break;
				switch (getOpacMapEntityIndex()){
					case (0): //constant
						opacVar = 0.f;
						break;
					case (1): //age
						if (flowIsSteady())
							//Map k in [0..objectsPerFlowline] to the interval (firstDisplayFrame, lastDisplayFrame)
							opacVar =  (float)firstSteadyStep +
								(float)k*((float)(lastSteadyStep-firstSteadyStep))/((float)objectsPerFlowline);
						else
							opacVar = (float)k*((float)(maxFrame-minFrame))/((float)objectsPerFlowline);
						break;
					case (2): //speed
						opacVar = speeds[(k+ maxPoints*(j+ (numSeeds*i)))];
						break;
					case (3): //opacity mapped from seed index
						
						if (isRake) opacVar = -1.f - (float)j;
						else opacVar = (float)j;

						break;
					default : //variable
						int x,y,z;
						float* dataPoint = flowData+3*(k+ maxPoints*(j+ (numSeeds*i)));
						//Check for ignore flag... make transparent
						if (dataPoint[0] == IGNORE_FLAG){
							opacVar = opacMin;
							break;
						}
						float remappedPoint[3];
						periodicMap(dataPoint,remappedPoint);
						x = (int)(0.5f+((remappedPoint[0] - opacVarMin[0])*opacSize[0])/(opacVarMax[0]-opacVarMin[0]));
						y = (int)(0.5f+((remappedPoint[1] - opacVarMin[1])*opacSize[1])/(opacVarMax[1]-opacVarMin[1]));
						z = (int)(0.5f+((remappedPoint[2] - opacVarMin[2])*opacSize[2])/(opacVarMax[2]-opacVarMin[2]));
						if (x>=opacSize[0]) x = opacSize[0]-1;
						if (y>=opacSize[1]) y = opacSize[1]-1;
						if (z>=opacSize[2]) z = opacSize[2]-1;
						
						opacVar = opacRegion[x+opacSize[0]*(y+opacSize[1]*z)];
						break;
				}
				int opacIndex = (int)((opacVar - opacMin)*255.99/(opacMax-opacMin));
				if (opacIndex<0) opacIndex = 0;
				if (opacIndex> 255) opacIndex =255;
				//opacity = lut[3+4*opacIndex];
				switch (getColorMapEntityIndex()){
					case (0): //constant
						colorVar = 0.f;
						break;
					case (1): //age
						if (flowIsSteady())
							colorVar = (float)firstSteadyStep +
								(float)k*((float)(lastSteadyStep-firstSteadyStep))/((float)objectsPerFlowline);
						else
							colorVar = (float)k*((float)(maxFrame-minFrame))/((float)objectsPerFlowline);
						break;
					case (2): //speed
						colorVar = speeds[(k+ maxPoints*(j+ (numSeeds*i)))];
						break;
					case (3) : //seed index.  Will use constant color
						if (isRake) colorVar = -1.f - (float)j;
						else colorVar = (float)j;
						break;
					default : //variable
						int x,y,z;
						float* dataPoint = flowData+3*(k+ maxPoints*(j+ (numSeeds*i)));
						//Check for ignore flag... map to min color
						if (dataPoint[0] == IGNORE_FLAG){
							colorVar = colorMin;
							break;
						}
						float remappedPoint[3];
						periodicMap(dataPoint,remappedPoint);
						x = (int)(0.5f+((remappedPoint[0] - colorVarMin[0])*colorSize[0])/(colorVarMax[0]-colorVarMin[0]));
						y = (int)(0.5f+((remappedPoint[1] - colorVarMin[1])*colorSize[1])/(colorVarMax[1]-colorVarMin[1]));
						z = (int)(0.5f+((remappedPoint[2] - colorVarMin[2])*colorSize[2])/(colorVarMax[2]-colorVarMin[2]));
						if (x>=colorSize[0]) x = colorSize[0]-1;
						if (y>=colorSize[1]) y = colorSize[1]-1;
						if (z>=colorSize[2]) z = colorSize[2]-1;
						colorVar = colorRegion[x+colorSize[0]*(y+colorSize[1]*z)];
						break;
				}
				int colorIndex = (int)((colorVar - colorMin)*255.99/(colorMax-colorMin));
				if (colorIndex<0) colorIndex = 0;
				if (colorIndex> 255) colorIndex =255;
				//Special case for constant colors and/or opacities
				if (getOpacMapEntityIndex() == 0){
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))+3]= constantOpacity;
				} else {
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))+3]= lut[4*opacIndex+3];
				}
				if (getColorMapEntityIndex() == 0){
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))]= ((float)qRed(constantColor))/255.f;
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))+1]= ((float)qGreen(constantColor))/255.f;
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))+2]= ((float)qBlue(constantColor))/255.f;
				} else {
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))]= lut[4*colorIndex];
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))+1]= lut[4*colorIndex+1];
					flowRGBAs[4*(k+ maxPoints*(j+ (numSeeds*i)))+2]= lut[4*colorIndex+2];
				}
			}
		}
	}
}

//Generate a list of colors and opacities, one per (valid) vertex.
//Puts them into the FlowLineData
//Note that, if a variable is mapped, only the first time step is used for
//the mapping
//
void FlowParams::
mapColors(FlowLineData* container, int currentTimeStep, int minFrame){
	//Create lut based on current mapping data
	float* lut = new float[256*4];
	mapperFunction->makeLut(lut);
	//Setup mapping
	
	float opacMin = mapperFunction->getMinOpacMapValue();
	float colorMin = mapperFunction->getMinColorMapValue();
	float opacMax = mapperFunction->getMaxOpacMapValue();
	float colorMax = mapperFunction->getMaxColorMapValue();

	float opacVar, colorVar;
	float* opacRegion, *colorRegion;
	float opacVarMin[3], opacVarMax[3], colorVarMin[3], colorVarMax[3];
	int opacSize[3],colorSize[3];
	DataStatus* ds = DataStatus::getInstance();
	//Make sure RGBAs are available if needed:
	if (getOpacMapEntityIndex() + getColorMapEntityIndex() > 0)
		container->enableRGBAs();
		
	//Get the variable (entire region) if needed
	if (getOpacMapEntityIndex() > 3){
		//set up args for GetRegion
		//If flow is unsteady, just get the first available timestep
		int timeStep = currentTimeStep;
		if(flowType != 0){//unsteady flow
			timeStep = ds->getFirstTimestep(getOpacMapEntityIndex()-4);
			if (timeStep < 0) MyBase::SetErrMsg("No data for mapped variable");
		}
		size_t minSize[3];
		size_t maxSize[3];
		const size_t *bs = DataStatus::getInstance()->getDataMgr()->GetMetadata()->GetBlockSize();
		for (int i = 0; i< 3; i++){
			minSize[i] = 0;
			opacSize[i] = (int)((ds->getFullDataSize(i) >> (ds->getNumTransforms() - numRefinements)));
			maxSize[i] = opacSize[i]/bs[i] -1;
			opacVarMin[i] = DataStatus::getInstance()->getExtents()[i];
			opacVarMax[i] = DataStatus::getInstance()->getExtents()[i+3];
		}
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		opacRegion = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
			opacMapEntity[getOpacMapEntityIndex()].c_str(),
			numRefinements, (size_t*) minSize, (size_t*) maxSize, 0);
		QApplication::restoreOverrideCursor();
	}
	if (getColorMapEntityIndex() > 3){
		//set up args for GetRegion
		int timeStep = ds->getFirstTimestep(getColorMapEntityIndex()-4);
		if (timeStep < 0) MyBase::SetErrMsg("No data for mapped variable");
		size_t minSize[3];
		size_t maxSize[3];
		const size_t *bs = DataStatus::getInstance()->getDataMgr()->GetMetadata()->GetBlockSize();
		for (int i = 0; i< 3; i++){
			minSize[i] = 0;
			colorSize[i] = (int)((ds->getFullDataSize(i) >> (ds->getNumTransforms() - numRefinements)));
			maxSize[i] = (colorSize[i]/bs[i] - 1);
			colorVarMin[i] = DataStatus::getInstance()->getExtents()[i];
			colorVarMax[i] = DataStatus::getInstance()->getExtents()[i+3];
		}
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		colorRegion = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
			colorMapEntity[getColorMapEntityIndex()].c_str(),
			numRefinements, (size_t*) minSize, (size_t*) maxSize, 0);
		QApplication::restoreOverrideCursor();

	}
	
	//Cycle through all the points.  Map to rgba as we go.
	//
	for (int i = 0; i<numInjections; i++){
		for (int lineNum = 0; lineNum<container->getNumLines(); lineNum++){
			for (int pointNum = container->getStartIndex(lineNum); pointNum<= container->getEndIndex(lineNum); pointNum++){
			//for (int k = 0; k<maxPoints; k++) {
				
				switch (getOpacMapEntityIndex()){
					case (0): //constant
						opacVar = 0.f;
						break;
					case (1): //age
						if (flowIsSteady())
							//Map k in [0..objectsPerFlowline] to the interval (0,1)
							opacVar =  
								(float)pointNum/((float)objectsPerFlowline);
						else
							opacVar = (float)pointNum*((float)(maxFrame-minFrame))/((float)objectsPerFlowline);
						break;
					case (2): //speed
						opacVar = container->getSpeed(lineNum,pointNum);
						break;
					case (3): //opacity mapped from seed index
						
						
						opacVar = (float)lineNum;

						break;
					default : //variable
						int x,y,z;
						float* dataPoint = container->getFlowPoint(lineNum,pointNum);
							
						//Check for ignore flag... make transparent
						//Should never happen!
						assert (dataPoint[0] != IGNORE_FLAG);
						//	opacVar = opacMin;
						//	break;
						//}
						float remappedPoint[3];
						periodicMap(dataPoint,remappedPoint);
						x = (int)(0.5f+((remappedPoint[0] - opacVarMin[0])*opacSize[0])/(opacVarMax[0]-opacVarMin[0]));
						y = (int)(0.5f+((remappedPoint[1] - opacVarMin[1])*opacSize[1])/(opacVarMax[1]-opacVarMin[1]));
						z = (int)(0.5f+((remappedPoint[2] - opacVarMin[2])*opacSize[2])/(opacVarMax[2]-opacVarMin[2]));
						if (x>=opacSize[0]) x = opacSize[0]-1;
						if (y>=opacSize[1]) y = opacSize[1]-1;
						if (z>=opacSize[2]) z = opacSize[2]-1;
						
						opacVar = opacRegion[x+opacSize[0]*(y+opacSize[1]*z)];
						break;
				}
				int opacIndex = (int)((opacVar - opacMin)*255.99/(opacMax-opacMin));
				if (opacIndex<0) opacIndex = 0;
				if (opacIndex> 255) opacIndex =255;
				//opacity = lut[3+4*opacIndex];
				switch (getColorMapEntityIndex()){
					case (0): //constant
						colorVar = 0.f;
						break;
					case (1): //age
						if (flowIsSteady())
							colorVar = 
								(float)pointNum/((float)objectsPerFlowline);
						else
							colorVar = (float)pointNum*((float)(maxFrame-minFrame))/((float)objectsPerFlowline);
						break;
					case (2): //speed
						colorVar = container->getSpeed(lineNum,pointNum);
						break;
					case (3) : //seed index.  Will use constant color
						
						colorVar = (float)lineNum;
						break;
					default : //variable
						int x,y,z;
						float* dataPoint = container->getFlowPoint(lineNum,pointNum);
						assert(dataPoint[0] != IGNORE_FLAG);
						//Check for ignore flag... map to min color
						//if (dataPoint[0] == IGNORE_FLAG){
						//	colorVar = colorMin;
						//	break;
						//}
						float remappedPoint[3];
						periodicMap(dataPoint,remappedPoint);
						x = (int)(0.5f+((remappedPoint[0] - colorVarMin[0])*colorSize[0])/(colorVarMax[0]-colorVarMin[0]));
						y = (int)(0.5f+((remappedPoint[1] - colorVarMin[1])*colorSize[1])/(colorVarMax[1]-colorVarMin[1]));
						z = (int)(0.5f+((remappedPoint[2] - colorVarMin[2])*colorSize[2])/(colorVarMax[2]-colorVarMin[2]));
						if (x>=colorSize[0]) x = colorSize[0]-1;
						if (y>=colorSize[1]) y = colorSize[1]-1;
						if (z>=colorSize[2]) z = colorSize[2]-1;
						colorVar = colorRegion[x+colorSize[0]*(y+colorSize[1]*z)];
						break;
				}
				int colorIndex = (int)((colorVar - colorMin)*255.99/(colorMax-colorMin));
				if (colorIndex<0) colorIndex = 0;
				if (colorIndex> 255) colorIndex =255;
				//Now assign color etc.
				//Special case for constant colors and/or opacities
				if (getOpacMapEntityIndex() == 0){
					container->setAlpha(lineNum,pointNum,constantOpacity);
				} else {
					container->setAlpha(lineNum,pointNum,lut[4*opacIndex+3]);
				}
				if (getColorMapEntityIndex() == 0){
					container->setRGB(lineNum,pointNum,((float)qRed(constantColor))/255.f,
						((float)qGreen(constantColor))/255.f,
						((float)qBlue(constantColor))/255.f);
				} else {
					container->setRGB(lineNum,pointNum,lut[4*colorIndex],
						lut[4*colorIndex+1],
						lut[4*colorIndex+2]);
				}
			}
		}
	}
}
//Map periodic coords into data extents.
//Note this is different than the mapping used by flowlib (at least when numrefinements < max numrefinements)
void FlowParams::periodicMap(float origCoords[3], float mappedCoords[3]){
	const float* extents = DataStatus::getInstance()->getExtents();
	for (int i = 0; i<3; i++){
		mappedCoords[i] = origCoords[i];
		if (periodicDim[i]){
			//If it's off, most likely it's only off by one cycle:
			while (mappedCoords[i] < extents[i]) {mappedCoords[i] += (extents[i+3]-extents[i]);}
			while (mappedCoords[i] > extents[i+3]) {mappedCoords[i] -= (extents[i+3]-extents[i]);}
		}
	}
}

int FlowParams::
getColorMapEntityIndex() {
	if (!mapperFunction) return 0;
	return mapperFunction->getColorVarNum();
}

int FlowParams::
getOpacMapEntityIndex() {
	if (!mapperFunction) return 0;
	return mapperFunction->getOpacVarNum();
}
void FlowParams::
setColorMapEntity( int entityNum){
	if (!mapperFunction) return;
	mapperFunction->setMinColorMapValue(minColorBounds[entityNum]);
	mapperFunction->setMaxColorMapValue(maxColorBounds[entityNum]);
	mapperFunction->setColorVarNum(entityNum);
}
void FlowParams::
setOpacMapEntity( int entityNum){
	if (!mapperFunction) return;
	mapperFunction->setMinOpacMapValue(minOpacBounds[entityNum]);
	mapperFunction->setMaxOpacMapValue(maxOpacBounds[entityNum]);
    mapperFunction->setOpacVarNum(entityNum);
}



//Calculate the extents of the seedBox region when transformed into the unit cube
void FlowParams::
calcSeedExtents(float* extents){
	const float* fullExtent = DataStatus::getInstance()->getExtents();
	
	int i;
	float maxCrd = -1.f;
	
	float regSize[3];
	for (i=0; i<3; i++){
		regSize[i] =  fullExtent[i+3]-fullExtent[i];
		if(regSize[i] > maxCrd ) {
			maxCrd = regSize[i];
		}
	}

	for (i = 0; i<3; i++){
		extents[i] = (seedBoxMin[i] - fullExtent[i])/maxCrd;
		extents[i+3] = (seedBoxMax[i] - fullExtent[i])/maxCrd;
	}
}

//When we set the min/map bounds, must save them locally and in the mapper function
void FlowParams::setMinColorMapBound(float val){
	minColorBounds[getColorMapEntityIndex()] = val;	
	getMapperFunc()->setMinColorMapValue(val);
}
void FlowParams::setMaxColorMapBound(float val){
	maxColorBounds[getColorMapEntityIndex()] = val;
	getMapperFunc()->setMaxColorMapValue(val);
}
void FlowParams::setMinOpacMapBound(float val){
	minOpacBounds[getOpacMapEntityIndex()] = val;
	getMapperFunc()->setMinOpacMapValue(val);
}
void FlowParams::setMaxOpacMapBound(float val){
	maxOpacBounds[getOpacMapEntityIndex()] = val;
	getMapperFunc()->setMaxOpacMapValue(val);
}

//Determine the max and min (likely range) associated with a mapped index:
float FlowParams::minRange(int index, int timestep){
	switch(index){
		case (0): return 0.f;
		case (1): 
			//Need to fix this for unsteady
			if (flowIsSteady()){
				return 0.f;
			}
			else return (0.f);//??
		case (2): return (0.f);//speed
		case (3): //seed index.  Goes from -1 to -(rakesize)
			return (doRake ? -(float)getNumRakeSeedPoints() : 0.f );
		default:
			int varnum = index -4;
			if (DataStatus::getInstance()&& DataStatus::getInstance()->variableIsPresent(varnum)){
				return( DataStatus::getInstance()->getDataMin(varnum, timestep));
			}
			else return 0.f;
	}
}
float FlowParams::maxRange(int index, int timestep){
	float maxSpeed = 0.f;
	
	switch(index){
		case (0): return 1.f;
		case (1): if (flowIsSteady()){
				return 1.f;
			}
			//unsteady:
			return (float)maxFrame;//??
		case (2): //speed
			for (int k = 0; k<3; k++){
				int var;
				if (flowIsSteady()) var = steadyVarNum[k];
				else var = unsteadyVarNum[k];
				
				if (maxSpeed < fabs(DataStatus::getInstance()->getDefaultDataMax(var)))
					maxSpeed = fabs(DataStatus::getInstance()->getDefaultDataMax(var));
				if (maxSpeed < fabs(DataStatus::getInstance()->getDefaultDataMin(var)))
					maxSpeed = fabs(DataStatus::getInstance()->getDefaultDataMin(var));
			}
			return maxSpeed;
		case (3): // seed Index, from 0 to listsize -1
			return (!doRake ? (float)(getNumListSeedPoints()-1) : 0.f );
		default:
			int varnum = index -4;
			if (DataStatus::getInstance()&& DataStatus::getInstance()->variableIsPresent(varnum)){
				return( DataStatus::getInstance()->getDataMax(varnum, timestep));
			}
			else return 1.f;
	}
}
//Method to validate and fix time sampling values used in Pathlines
//This tries to keep previously set values of timeSamplingInterval, timeSamplingStart,
//and timeSamplingEnd.  If they don't work, info message is provided and
//they are set to new values as follows:
//if not valid, timeSamplingStart is set to the first valid frame after previously set timeSamplingStart
//if not valid, i.e. if second frame does not exist, then
//timeSamplingInterval is set to difference between second and first valid frame.
//if not valid, timeSamplingEnd is set to last valid frame in sequence established by 
//timeSamplingStart and timeSamplingInterval
bool FlowParams::
validateSampling(int minFrame)
{
	//Don't do anything if no data has been read:
	if (!DataStatus::getInstance()->getDataMgr()) return false;
	
	bool changed = false;
	if (timeSamplingStart < minFrame || timeSamplingStart > maxFrame){
		timeSamplingStart = minFrame;
		changed = true;
	}
	//See if start is valid; if not set to first valid start:
	int ts;
	for (ts = timeSamplingStart; ts <= maxFrame; ts++) {
		if (validateVectorField(ts)) break;
	}
	//do there exist valid frames?
	if (ts > maxFrame) {
		timeSamplingStart = minFrame;
		timeSamplingEnd = minFrame;
		timeSamplingInterval = 1;
		return false;
	}
	//was the proposed start invalid?
	if (ts > timeSamplingStart) {
		timeSamplingStart = ts;
		changed = true;
	}
	//If the proposed sampling is just one frame, it's ok
	if (timeSamplingEnd < timeSamplingStart+timeSamplingInterval) {
		if (timeSamplingEnd < timeSamplingStart) {
			timeSamplingEnd = timeSamplingStart;
			changed = true;
		}
		return !changed;
	}
	//Now we have a valid start, see if the increment is invalid.
	if (timeSamplingInterval < 1) {
		timeSamplingInterval = 1;
		changed = true;
	}
	//It is OK if the second one is valid or if there is no second one:
	if (!validateVectorField(timeSamplingStart+timeSamplingInterval)){
		//Need to find the first valid increment:
		for (ts = timeSamplingStart+1; ts <= maxFrame; ts++){
			if (validateVectorField(ts)) break;
		}
		//Is there a valid second timestep?
		if (ts > maxFrame){
			timeSamplingInterval = 1;
			timeSamplingEnd = timeSamplingStart;
			return false;
		}
		//Use the second valid step to define interval:
		timeSamplingInterval = ts - timeSamplingStart;
		changed = true;
	}
	//Now establish the last frame
	for (ts = timeSamplingStart; ts <= timeSamplingEnd; ts+= timeSamplingInterval){
		if (!validateVectorField(ts)) break;
	}
	if (ts > timeSamplingEnd) return (!changed);
	timeSamplingEnd = ts - timeSamplingInterval;
	return false;
}

//Check to see if there is valid field data for specified timestep
//
 
bool FlowParams::
validateVectorField(int ts) {
	DataStatus* dStatus = DataStatus::getInstance();
	if (!dStatus) return false;
	for (int var = 0; var<3; var++){
		if(dStatus->maxXFormPresent(var, ts)< 0)
			return false;
	}
	return true;
}

			


int FlowParams::calcMaxPoints(){
	int numPrePoints = 0, numPostPoints = 0;
	if (flowType == 0) { //steady
		
		maxPoints = objectsPerFlowline+1;
		if (maxPoints < 2) maxPoints = 2;
		if (steadyFlowDirection == 0 && maxPoints < 4) maxPoints = 4;
		//If bidirectional allocate between prePoints and postPoints
		if (steadyFlowDirection == 0){
			numPrePoints = maxPoints/2;
			numPostPoints = maxPoints - numPrePoints;
			//Midpoint is used by both post- and pre-points so maxPoints
			//is one less than the sum:
			maxPoints--;
		} else if (steadyFlowDirection < 0){
			numPrePoints = maxPoints;
			numPostPoints = 0;
		} else {
			numPrePoints = 0;
			numPostPoints = maxPoints;
		}
		
	} else {
		
		//For unsteady flow, the firstDisplayFrame and lastDisplayFrame give a window
		//on a longer timespan that potentially goes from 
		//the first seed start time to the last frame in the animation.
		//lastDisplayFrame does not limit the length of the flow
		maxPoints = objectsPerFlowline+1;
	}
	return maxPoints;
}
int FlowParams::calcNumSeedPoints(int timeStep){
	if (doRake) return getNumRakeSeedPoints();
	//determine how many seed points for current timestep...
		
	//For unsteady flow, use all the seeds ????
	//if (flowType != 0) return getNumListSeedPoints();
	
	//For steady flow, count how many seeds will be in the array
	int seedCounter = 0;
	for (int i = 0; i<getNumListSeedPoints(); i++){
		float time = seedPointList[i].getVal(3);
		if ((time < 0.f) || ((int)(time+0.5f) == timeStep)){
			seedCounter++;
		}
	}
	return seedCounter;
}
//Function to calculate the average magnitude of the steady vector field over specified region
//Plan on using this to help automatically set vector scale factor.
float FlowParams::getAvgVectorMag(RegionParams* rParams, int numrefts, int timeStep){
	
	float* varData[3];
	
	size_t min_dim[3],max_dim[3],min_bdim[3],max_bdim[3];
	bool ok =  rParams->getAvailableVoxelCoords(numrefts, min_dim, max_dim, min_bdim,  max_bdim, (size_t)timeStep, steadyVarNum, 3);
	if (!ok) return -1.f;


	DataMgr* dataMgr = (DataMgr*)(DataStatus::getInstance()->getDataMgr());
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	//Obtain the variables from the dataMgr:
	for (int var = 0; var<3; var++){
		varData[var] = dataMgr->GetRegion((size_t)timeStep,
			DataStatus::getInstance()->getVariableName(steadyVarNum[var]).c_str(),
			numrefts, min_bdim, max_bdim, 1);
		if (!varData[var]) {
			//release currently locked regions:
			for (int k = 0; k<var; k++){
				dataMgr->UnlockRegion(varData[k]);
			}
			QApplication::restoreOverrideCursor();
			return -2.f;
		}	
	}
	int bSize =  (int)(*(DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize()));
	int numPts = 0;
	float dataSum = 0.f;
	//OK, we got the data. find the sum:
	for (size_t i = min_dim[0]; i<=max_dim[0]; i++){
		for (size_t j = min_dim[1]; j<=max_dim[1]; j++){
			for (size_t k = min_dim[2]; k<=max_dim[2]; k++){
				int xyzCoord = (i - min_bdim[0]*bSize) +
					(j - min_bdim[1]*bSize)*(bSize*(max_bdim[0]-min_bdim[0]+1)) +
					(k - min_bdim[2]*bSize)*(bSize*(max_bdim[1]-min_bdim[1]+1))*(bSize*(max_bdim[0]-min_bdim[0]+1));
				float sumsq = varData[0][xyzCoord]*varData[0][xyzCoord]+varData[1][xyzCoord]*varData[1][xyzCoord]+varData[2][xyzCoord]*varData[2][xyzCoord];
				//dataMax = Max(dataMax, sqrt(sumsq));
				dataSum += sqrt(sumsq);
				numPts++;
			}
		}
	}
	for (int k = 0; k<3; k++){
		dataMgr->UnlockRegion(varData[k]);
	}
	QApplication::restoreOverrideCursor();
	if (numPts == 0) return -1.f;
	return (dataSum/(float)numPts);
	//return dataMax;
}
//Tell the flowLib about the current region, return false on error
bool FlowParams::
setupFlowRegion(RegionParams* rParams, VaporFlow* flowLib, int timeStep, int minFrame){
	size_t min_dim[3], max_dim[3]; 
	size_t min_bdim[3], max_bdim[3];
	
	//For steady flow, determine what is the available region for the current time step.
	//For unsteady flow, determine the available region for all the sampled timesteps.
	if (flowIsSteady()){
		bool dataValid = rParams->getAvailableVoxelCoords(numRefinements, min_dim, max_dim, min_bdim, max_bdim, timeStep, steadyVarNum, 3);
	
		if(!dataValid){
			MyBase::SetErrMsg("Vector field data unavailable for refinement %d at timestep %d", numRefinements, timeStep);
			return 0;
		}
	} else { //Find the intersection of all the regions for the timesteps to be sampled.
		size_t gmin_dim[3],gmax_dim[3],gmin_bdim[3], gmax_bdim[3];
		rParams->getRegionVoxelCoords(numRefinements, gmin_dim, gmax_dim, gmin_bdim,gmax_bdim);
		int firstSample = timeSamplingStart;
		int lastSample = Min(timeSamplingEnd, maxFrame);
		if (timeSamplingStart < minFrame) firstSample = ((1+minFrame/timeSamplingInterval)*timeSamplingInterval);
		for (int i = firstSample; i<= lastSample; i+= timeSamplingInterval){
			bool dataValid = rParams->getAvailableVoxelCoords(numRefinements, min_dim, max_dim, min_bdim, max_bdim, i, unsteadyVarNum, 3);
			if(!dataValid){
				SetErrMsg(102,"Vector field data unavailable for refinement %d at timestep %d", numRefinements, i);
				return 0;
			}
			//Intersect the available region bounds.
			for (int i = 0; i< 3; i++){
				gmin_dim[i] = Max(gmin_dim[i],min_dim[i]);
				gmax_dim[i] = Min(gmax_dim[i],max_dim[i]);
				gmin_bdim[i] = Max(gmin_bdim[i],min_bdim[i]);
				gmax_bdim[i] = Min(gmax_bdim[i],max_bdim[i]);
			}
		}
		//Copy back the intersected region:
		for (int i = 0; i< 3; i++){
			min_dim[i] = gmin_dim[i];
			max_dim[i] = gmax_dim[i];
			min_bdim[i] = gmin_bdim[i];
			max_bdim[i] = gmax_bdim[i];
		}
	}
	
	//Check that the cache is big enough for all three (or 4) variables
	float numVoxels = (max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1);
	float nvars =  3.f;
	
	//Multiply by 32^3 *4 to get total bytes, divide by 2^20 for mbytes, * num variables
	if (nvars*numVoxels/8.f >= (float)DataStatus::getInstance()->getCacheMB()){
		SetErrMsg(101," Data cache size %d is too small for this flow integration",
			DataStatus::getInstance()->getCacheMB());
		return false;
	}
	flowLib->SetRegion(numRefinements, min_dim, max_dim, min_bdim, max_bdim);
	return true;
}

	