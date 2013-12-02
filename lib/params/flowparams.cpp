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

#include "glutil.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "flowparams.h"
#include "regionparams.h"
#include "datastatus.h"
#include <vapor/VaporFlow.h>
#include <vapor/flowlinedata.h>
#include <vapor/errorcodes.h>
#include "vapor/RegularGrid.h"

#include <qapplication.h>
#include <qcursor.h>

#include "transferfunction.h"

#include <vapor/common.h>

using namespace VAPoR;
	const string FlowParams::_shortName = "Flow";
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
	const string FlowParams::_seedDistVariableNamesAttr = "SeedDistVariableNames";
	const string FlowParams::_priorityVariableNamesAttr = "PriorityVariableNames";
	const string FlowParams::_priorityBoundsAttr = "PriorityBounds";
	const string FlowParams::_seedDistBiasAttr = "SeedDistribBias";
	const string FlowParams::_numFLASamplesAttr = "NumFLASamples";
	const string FlowParams::_advectBeforePrioritizeAttr = "AdvectBeforePrioritize";

	const string FlowParams::_periodicDimsAttr = "PeriodicDimensions";
	const string FlowParams::_steadyFlowAttr = "SteadyFlow";//obsolete
	const string FlowParams::_flowTypeAttr = "FlowType";
	const string FlowParams::_integrationAccuracyAttr = "IntegrationAccuracy";
	const string FlowParams::_velocityScaleAttr = "velocityScale";
	const string FlowParams::_steadyScaleAttr = "steadyScale";
	const string FlowParams::_unsteadyScaleAttr = "unsteadyScale";
	const string FlowParams::_timeSamplingAttr = "SamplingTimes";
	const string FlowParams::_autoRefreshAttr = "AutoRefresh";
	const string FlowParams::_autoScaleAttr = "AutoScale";
	const string FlowParams::_flowLineLengthAttr = "FlowLineLength";
	const string FlowParams::_smoothnessAttr = "Smoothness";
	const string FlowParams::_useDisplayListsAttr = "UseDisplayLists";
	
	
	
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
	const string FlowParams::_diamondDiameterAttr = "DiamondDiameter";
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
	float FlowParams::defaultFlowLength = 1.f;
	float FlowParams::defaultSmoothness = 20.f;
	int FlowParams::defaultGeometryType = 0;  //0= tube, 1=point, 2 = arrow
	float FlowParams::defaultIntegrationAccuracy = 0.5f;
	float FlowParams::defaultFlowDiameter = 1.f;
	float FlowParams::defaultArrowSize = 2.f;
	float FlowParams::defaultDiamondSize = 2.f;

	FlowParams::FlowParams(int winnum) : RenderParams(winnum, Params::_flowParamsTag) {
	
	myBox= 0;
	transferFunction = 0;
	
	//Set all parameters to default values
	restart();
	
}
FlowParams::~FlowParams(){
	if(myBox) delete myBox;
	if (transferFunction){
		delete transferFunction;
	}
	
		
	if (minColorBounds) {
		delete [] minColorBounds;
		delete [] maxColorBounds;
	}
	
}
//Set everything in sight to default state:
void FlowParams::
restart() {
	histoStretchFactor = 1.;
	useDisplayLists = false;
	stopFlag = false;
	autoScale = true;
	numFLASamples = 5;
	flaAdvectBeforePrioritize = false;
	steadyFlowDirection = 0;
	unsteadyFlowDirection = 1; //default is forward
	compressionLevel = 0;

	if (!myBox){
		myBox = new Box();
	}
	steadyFlowLength = defaultFlowLength;
	steadySmoothness = defaultSmoothness;
	geometryType = defaultGeometryType;  //0= tube, 1=point, 2 = arrow
	integrationAccuracy = defaultIntegrationAccuracy;
	shapeDiameter = defaultFlowDiameter;
	arrowDiameter = defaultArrowSize;
	diamondDiameter = defaultDiamondSize;

	SetIgnoreFidelity(false);
	flowType = 0; //steady
	periodicDim[0]=periodicDim[1]=periodicDim[2]=false;
	autoRefresh = true;
	enabled = false;
	useTimestepSampleList = false;
	unsteadyTimestepList.clear();
	numRefinements = 0; 
	maxNumRefinements = 4; 
	numComboVariables = 0;
	firstDisplayFrame = -100;
	lastDisplayFrame = 1;
	
	
	steadyVarNum[0]= 1;
	steadyVarNum[1] = 2;
	steadyVarNum[2] = 3;
	unsteadyVarNum[0]= 1;
	unsteadyVarNum[1] = 2;
	unsteadyVarNum[2] = 3;
	seedDistVarNum[0]= 0;
	seedDistVarNum[1] = 1;
	seedDistVarNum[2] = 2;
	priorityVarNum[0]= 0;
	priorityVarNum[1] = 1;
	priorityVarNum[2] = 2;
	
	seedDistBias = 0.f;
	priorityMin = 0.f;
	priorityMax = 1.e30f;
	comboSteadyVarNum[0]= 1;
	comboSteadyVarNum[1] = 2;
	comboSteadyVarNum[2] = 3;
	comboUnsteadyVarNum[0]= 1;
	comboUnsteadyVarNum[1] = 2;
	comboUnsteadyVarNum[2] = 3;
	comboSeedDistVarNum[0]= 1;
	comboSeedDistVarNum[1] = 2;
	comboSeedDistVarNum[2] = 3;
	comboPriorityVarNum[0] = 1;
	comboPriorityVarNum[1] = 2;
	comboPriorityVarNum[2] = 3;
	
	steadyScale = 1.0f;
	unsteadyScale = 1.0f;
	constantColor = qRgb(255,0,0);
	constantOpacity = 1.f;
	timeSamplingInterval = 1;
	timeSamplingStart = 0;
	timeSamplingEnd = 100;
	
	editMode = true;
	

	randomGen = true;
	
	
	randomSeed = 1;
	double seedBoxExtents[6];
	seedBoxExtents[0] = seedBoxExtents[1] = seedBoxExtents[2] = 0.f;
	seedBoxExtents[3] = seedBoxExtents[4] = seedBoxExtents[5] = 1.f;
	myBox->SetLocalExtents(seedBoxExtents);
	
	generatorCount[0]=generatorCount[1]=generatorCount[2] = 1;

	allGeneratorCount = 10;
	seedTimeStart = 0; 
	seedTimeEnd = 0; 
	seedTimeIncrement = 1;
	currentDimension = 0;

	
	objectsPerFlowline = 20;
	objectsPerTimestep = 1;

	
	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Timestep");
	colorMapEntity.push_back("Field Magnitude");
	colorMapEntity.push_back("Seed Index");

	
	minColorEditBounds = new float[4];
	maxColorEditBounds = new float[4];
	minOpacEditBounds = new float[4];
	maxOpacEditBounds = new float[4];
	for (int i = 0; i< 4; i++){
		minColorEditBounds[i]= 0.f;
		maxColorEditBounds[i]= 1.f;
		minOpacEditBounds[i]=0.f;
		maxOpacEditBounds[i]=1.f;
	}
	
	seedPointList.clear();
	
	
	maxPoints = 0;
	maxFrame = 0;

	minColorBounds= new float[4];
	maxColorBounds= new float[4];
	for (int k = 0; k<4; k++){
		minColorBounds[k] = 0.f;
		maxColorBounds[k] = 1.f;
	}
	
	doRake = true;

	seedDistIsSteady = false;
	priorityIsSteady = false;

}



void FlowParams::setDefaultPrefs(){
	defaultFlowLength = 1.f;
	defaultSmoothness = 20.f;
	defaultGeometryType = 0;  //0= tube, 1=point, 2 = arrow
	defaultIntegrationAccuracy = 0.5f;
	defaultFlowDiameter = 1.f;
	defaultArrowSize = 2.f;
	defaultDiamondSize = 2.f;
}

//Make a copy of  parameters:
Params* FlowParams::
deepCopy(ParamNode*){
	
	FlowParams* newFlowParams = new FlowParams(*this);
	ParamNode* pNode = new ParamNode(*(myBox->GetRootNode()));
	newFlowParams->myBox = (Box*) myBox->deepCopy(pNode);
	
	//Clone the map bounds arrays:
	int numVars = numComboVariables+4;
	newFlowParams->minColorEditBounds = new float[numVars];
	newFlowParams->maxColorEditBounds = new float[numVars];
	newFlowParams->minOpacEditBounds = new float[numVars];
	newFlowParams->maxOpacEditBounds = new float[numVars];
	//Clone the variable bounds:
	newFlowParams->minColorBounds = new float[numVars];
	newFlowParams->maxColorBounds = new float[numVars];
	for (int i = 0; i<numVars; i++){
		newFlowParams->minColorEditBounds[i] = minColorEditBounds[i];
		newFlowParams->maxColorEditBounds[i] = maxColorEditBounds[i];
		newFlowParams->minOpacEditBounds[i] = minOpacEditBounds[i];
		newFlowParams->maxOpacEditBounds[i] = maxOpacEditBounds[i];
		newFlowParams->minColorBounds[i] = minColorBounds[i];
		newFlowParams->maxColorBounds[i] = maxColorBounds[i];
	}
	
	//Clone the Transfer Functions
	if (transferFunction) {
		newFlowParams->transferFunction = new TransferFunction(*transferFunction);
	} else {
		newFlowParams->transferFunction = 0;
	}
	

	
	return newFlowParams;
}

bool FlowParams::usingVariable(const string& varname){
	int varnum = DataStatus::getInstance()->getSessionVariableNum3D(varname);
	if (flowType == 0 || flowType == 2){
		if ((steadyVarNum[0] == varnum+1 )|| (steadyVarNum[1] == varnum+1) || (steadyVarNum[1] == varnum+1)) return true;
	}
	if (flowType == 1){
		if ((unsteadyVarNum[0] == varnum+1 )|| (unsteadyVarNum[1] == varnum+1) || (unsteadyVarNum[1] == varnum+1)) return true;
	}
	if (randomGen && seedDistBias != 0.f){
		if ((seedDistVarNum[0] == varnum )|| (seedDistVarNum[1] == varnum) || (seedDistVarNum[1] == varnum)) return true;
	}
	if (flowType == 2){
		if ((priorityVarNum[0] == varnum )|| (priorityVarNum[1] == varnum) || (priorityVarNum[1] == varnum)) return true;
	}
	return false;
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
	int minFrame = 0;
	maxFrame = (int)(DataStatus::getInstance()->getNumTimesteps()-1);
	editMode = true;
	// set the params state based on whether we are overriding or not:
	if (doOverride) {
		
		numRefinements = 0;
		seedTimeStart = minFrame;
		seedTimeEnd = minFrame;
		seedTimeIncrement = 1;
		autoRefresh = true;
		periodicDim[0]=periodicDim[1]=periodicDim[2] = false;
		steadyFlowLength = defaultFlowLength;
		steadySmoothness = defaultSmoothness;
		geometryType = defaultGeometryType;  //0= tube, 1=point, 2 = arrow
		integrationAccuracy = defaultIntegrationAccuracy;
		shapeDiameter = defaultFlowDiameter;
		arrowDiameter = defaultArrowSize;
		diamondDiameter = defaultDiamondSize;
		SetFidelityLevel(0);
		SetIgnoreFidelity(false);
	} else {
		if (numRefinements> maxNumRefinements) numRefinements = maxNumRefinements;
		
		//Make sure we really can use the specified numTrans,
		//with the current region settings
		
		if (seedTimeStart > maxFrame) seedTimeStart = maxFrame;
		if (seedTimeStart < minFrame) seedTimeStart = minFrame;
		if (seedTimeEnd > maxFrame) seedTimeEnd = maxFrame;
		if (seedTimeEnd < seedTimeStart) seedTimeEnd = seedTimeStart;
	}
	//Make sure fidelity is valid:
	int fidelity = GetFidelityLevel();
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (dataMgr && fidelity > maxNumRefinements+dataMgr->GetCRatios().size()-1)
		SetFidelityLevel(maxNumRefinements+dataMgr->GetCRatios().size()-1);

	//Set up the seed region:
	const float* fullExtents = DataStatus::getInstance()->getLocalExtents();
	double seedBoxExtents[6];
	GetBox()->GetLocalExtents(seedBoxExtents);
	if (doOverride){
		for (i = 0; i<3; i++){
			seedBoxExtents[i] = 0.;
			seedBoxExtents[i+3] = (fullExtents[i+3]-fullExtents[i]);
		}
	} else {
		if (DataStatus::pre22Session()){
			float * offset = DataStatus::getPre22Offset();
			//In old session files, the coordinate of box extents were not 0-based
			
			for (int i = 0; i<3; i++){
				seedBoxExtents[i] -= offset[i];
				seedBoxExtents[i+3] -= offset[i];
			}
		}

		for (i = 0; i<3; i++){
			if(seedBoxExtents[i] < 0.)
				seedBoxExtents[i] = 0.;
			if(seedBoxExtents[i+3] > (fullExtents[i+3]-fullExtents[i]))
				seedBoxExtents[i+3] = (fullExtents[i+3]-fullExtents[i]);
			if(seedBoxExtents[i+3] < seedBoxExtents[i]) 
				seedBoxExtents[i+3] = seedBoxExtents[i];
		}
	}
	myBox->SetLocalExtents(seedBoxExtents);
	//Likewise, in pre-22 sessions, need to offset the seeds in the seed list
	if (DataStatus::pre22Session()){
		for (int i = 0; i<seedPointList.size(); i++){
			Point4 psn = seedPointList[i];
			int ts = (int) psn.getVal(3);
			if (ts < 0) ts = 0; //Use zero for extents (which won't move) if seed applies to all timesteps
			float pt[3];
			const vector<double>& usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)ts);
			for (int j = 0; j<3; j++) pt[j] = psn.getVal(j) + usrExts[j];
			seedPointList[i].set3Val(pt);
		}
	}
	//Set up variables:
	//Get the variable names:
	
	int newNumVariables = DataStatus::getInstance()->getNumSessionVariables();
	int newNumComboVariables = DataStatus::getInstance()->getNumActiveVariables3D();
	
	if (newNumVariables == 0 || newNumComboVariables == 0) return false;
	//Rebuild map bounds arrays:
	
	if(minColorBounds) delete [] minColorBounds;
	minColorBounds = new float[newNumComboVariables+4];
	if(maxColorBounds) delete [] maxColorBounds;
	maxColorBounds = new float[newNumComboVariables+4];
	
	
	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Timestep");
	colorMapEntity.push_back("Field Magnitude");
	colorMapEntity.push_back("Seed Index");
	
	for (i = 0; i< newNumComboVariables; i++){
		colorMapEntity.push_back(DataStatus::getInstance()->getActiveVarName3D(i));
	}
	
	if(doOverride || getColorMapEntityIndex() >= newNumComboVariables+4){
		setColorMapEntity(0);
	}
	if (doOverride){
		for (int j = 0; j<3; j++){
			comboSteadyVarNum[j] = Min(j+1,newNumComboVariables);
			steadyVarNum[j] = DataStatus::getInstance()->mapActiveToSessionVarNum3D(comboSteadyVarNum[j]-1)+1;
		}
		for (int j = 0; j<3; j++){
			comboUnsteadyVarNum[j] = Min(j+1,newNumComboVariables);
			unsteadyVarNum[j] = DataStatus::getInstance()->mapActiveToSessionVarNum3D(comboUnsteadyVarNum[j]-1)+1;
		}
	} 
	for (int dim = 0; dim < 3; dim++){
		//See if current steadyvarNum is valid.  If not, 
		//reset to first variable that is present:
		if (steadyVarNum[dim] > 0) {
			if (!DataStatus::getInstance()->variableIsPresent3D(steadyVarNum[dim]-1)){
				steadyVarNum[dim] = -1;
				for (i = 0; i<newNumVariables; i++) {
					if (DataStatus::getInstance()->variableIsPresent3D(i)){
						steadyVarNum[dim] = i+1;
						break;
					}
				}
			}
		}
		//See if current unsteadyvarNum is valid.  If not, 
		//reset to first variable that is present:
		if(unsteadyVarNum[dim] > 0) {
			if (!DataStatus::getInstance()->variableIsPresent3D(unsteadyVarNum[dim]-1)){
				unsteadyVarNum[dim] = -1;
				for (i = 0; i<newNumVariables; i++) {
					if (DataStatus::getInstance()->variableIsPresent3D(i)){
						unsteadyVarNum[dim] = i+1;
						break;
					}
				}
			}
		}
		if (!DataStatus::getInstance()->variableIsPresent3D(seedDistVarNum[dim])){
			seedDistVarNum[dim] = -1;
			for (i = 0; i<newNumVariables; i++) {
				if (DataStatus::getInstance()->variableIsPresent3D(i)){
					seedDistVarNum[dim] = i;
					break;
				}
			}
		}
		if (!DataStatus::getInstance()->variableIsPresent3D(priorityVarNum[dim])){
			priorityVarNum[dim] = -1;
			for (i = 0; i<newNumVariables; i++) {
				if (DataStatus::getInstance()->variableIsPresent3D(i)){
					priorityVarNum[dim] = i;
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
		if(steadyVarNum[dim] == 0) comboSteadyVarNum[dim] = 0;
		else 
			comboSteadyVarNum[dim] = DataStatus::getInstance()->mapSessionToActiveVarNum3D(steadyVarNum[dim]-1)+1;
		if (unsteadyVarNum[dim] == 0) comboUnsteadyVarNum[dim] = 0;
		else
			comboUnsteadyVarNum[dim] = DataStatus::getInstance()->mapSessionToActiveVarNum3D(unsteadyVarNum[dim]-1)+1;
		comboSeedDistVarNum[dim] = DataStatus::getInstance()->mapSessionToActiveVarNum3D(seedDistVarNum[dim]);
		comboPriorityVarNum[dim] = DataStatus::getInstance()->mapSessionToActiveVarNum3D(priorityVarNum[dim]);
	}
	//Set up sampling.  
	if (doOverride){
		timeSamplingStart = minFrame;
		timeSamplingEnd = maxFrame;
		timeSamplingInterval = 1;
	}
	//Don't validate sampling if this is just the default params:
	int varnums[3];
	if (vizNum >= 0){
		if (flowType != 1){
			for (int i = 0; i< 3; i++) varnums[i] = steadyVarNum[i] -1;
			validateSampling(minFrame, numRefinements, varnums);
		}
		if (flowType != 0){
			for (int i = 0; i< 3; i++) varnums[i] = unsteadyVarNum[i] -1;
			validateSampling(minFrame, numRefinements, varnums);
		}
	}

	
	//Now set up bounds arrays based on current mapped variable settings:
	for (i = 0; i< newNumComboVariables+4; i++){
		minColorBounds[i] = minRange(i, minFrame);
		maxColorBounds[i] = maxRange(i, minFrame);
	}
	
	
	//Create new edit bounds whether we override or not
	float* newMinOpacEditBounds = new float[newNumComboVariables+4];
	float* newMaxOpacEditBounds = new float[newNumComboVariables+4];
	float* newMinColorEditBounds = new float[newNumComboVariables+4];
	float* newMaxColorEditBounds = new float[newNumComboVariables+4];
	//Either try to reuse existing transferFunction, or create a new one.
	if (doOverride){ //create new ones:
		//For now, assume 8-bits mapping
		TransferFunction* newTransferFunction = new TransferFunction(this, 8);
		//Initialize to be fully opaque:
		newTransferFunction->setOpaque();

        //Set to default map bounds
		
		newTransferFunction->setMinMapValue(0.f);
		newTransferFunction->setMaxMapValue(1.f);
		//const
		newMinOpacEditBounds[0] = 0.f;
		newMaxOpacEditBounds[0] = 1.f;
		newMinColorEditBounds[0] = 0.f;
		newMaxColorEditBounds[0] = 1.f;
		//speed
		newMinOpacEditBounds[1] = 0.f;
		newMaxOpacEditBounds[1] = maxColorBounds[1];
		newMinColorEditBounds[1] = 0.f;
		newMaxColorEditBounds[1] = maxColorBounds[1];
		//age
		
		newMinColorEditBounds[2] = minColorBounds[2];
		newMaxColorEditBounds[2] = maxColorBounds[2];
		//Other variables:
		for (i = 0; i< newNumComboVariables; i++){
			if (DataStatus::getInstance()->variableIsPresent3D(i)){
				newMinOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin3D(i);
				newMaxOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMax3D(i);
				newMinColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin3D(i);
				newMaxColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin3D(i);
			} else {
				newMinOpacEditBounds[i+4] = 0.f;
				newMaxOpacEditBounds[i+4] = 1.f;
				newMinColorEditBounds[i+4] = 0.f;
				newMaxColorEditBounds[i+4] = 1.f;
			}
		} 

		delete transferFunction;
        transferFunction = newTransferFunction;

		setColorMapEntity(0);
	}
	else { 
		
		//Try to reuse existing bounds:
		
		for (i = 0; i< newNumComboVariables; i++){
			if (i >= numComboVariables){
				newMinOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin3D(i);
				newMaxOpacEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMax3D(i);
				newMinColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMin3D(i);
				newMaxColorEditBounds[i+4] = DataStatus::getInstance()->getDefaultDataMax3D(i);
			} else {
				newMinOpacEditBounds[i+4] = minOpacEditBounds[i+4];
				newMaxOpacEditBounds[i+4] = maxOpacEditBounds[i+4];
				newMinColorEditBounds[i+4] = minColorEditBounds[i+4];
				newMaxColorEditBounds[i+4] = maxColorEditBounds[i+4];
			}
		} 
		if (!transferFunction) transferFunction = new TransferFunction(this, 8);

        transferFunction->setColorVarNum(getColorMapEntityIndex());
	}
	
		
	delete [] minOpacEditBounds;
	delete [] minColorEditBounds;
	delete [] maxOpacEditBounds;
	delete [] maxColorEditBounds;
	
	minOpacEditBounds = newMinOpacEditBounds;
	maxOpacEditBounds = newMaxOpacEditBounds;
	minColorEditBounds = newMinColorEditBounds;
	maxColorEditBounds = newMaxColorEditBounds;

	
	
	numComboVariables = newNumComboVariables;
	
	//Don't disable, keep previous state
	//setEnabled(false);

	setMinColorEditBound(getMinColorMapBound(),getColorMapEntityIndex());
	setMaxColorEditBound(getMaxColorMapBound(),getColorMapEntityIndex());
	setMinOpacEditBound(getMinOpacMapBound(),getColorMapEntityIndex());
	setMaxOpacEditBound(getMaxOpacMapBound(),getColorMapEntityIndex());
	initializeBypassFlags();
	return true;
}


float FlowParams::getOpacityScale() {

  if (transferFunction)
  {
    return transferFunction->getOpacityScaleFactor();
  }

  return 1.0;
}

void FlowParams::setOpacityScale(float val) {
 
  if (transferFunction)
  {
    transferFunction->setOpacityScaleFactor(val);
  }
}

// Method to generate a list of rake seeds, including a 4th float for timestep
// the timestep is the current timestep
float* FlowParams::getRakeSeeds(RegionParams* rParams, int* numseeds, int timeStep){
	assert(doRake);  //Should only be doing this with a rake
	//Set up a flowLib to generate the rake seeds.
	DataStatus* ds = DataStatus::getInstance();
	VaporFlow* flowLib = new VaporFlow(ds->getDataMgr());
	if (!flowLib) return 0;
	
	setupFlowRegion(rParams, flowLib, timeStep);
	//Prepare the flowLib:
	double seedBox[6];
	GetBox()->GetLocalExtents(seedBox);
	if (randomGen){
		const char* xVar = ds->getVariableName3D(seedDistVarNum[0]).c_str();
		const char* yVar = ds->getVariableName3D(seedDistVarNum[1]).c_str();
		const char* zVar = ds->getVariableName3D(seedDistVarNum[2]).c_str();
		assert(seedDistBias >= -15.f && seedDistBias <= 15.f);
		flowLib->SetDistributedSeedPoints(seedBox, seedBox+3, (int)allGeneratorCount, 
			xVar, yVar, zVar, seedDistBias, flowType);
	} else {//Set up the uniform rake bounds
		flowLib->SetRegularSeedPoints(seedBox, seedBox+3, generatorCount);
	}
	
	//If the seeds are not random, or not biased, then all seeds
	//are valid for all time steps.
	//If the seeds are random and biased, then we need to generate them
	//for each time step:  This has the following cases:
	//  For steady flow or unsteady flow, generate the seeds separately for each of the 
	//	seed times
	//If we are doing field line advection, there is just one seed frame.
	//
	
	int nSeeds= 0;
	int actualNumSeeds;
	if (isRandom()){
		//Make sure the timesteps are valid for biased sampling:
		int minFrame = ds->getMinTimestep();
		validateSampling(minFrame, numRefinements, seedDistVarNum);
		if (flowType == 2){
			nSeeds = calcNumSeedPoints(seedTimeStart);
		} else if (flowType == 1) {
			for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
				nSeeds += calcNumSeedPoints(i);
			}
		} else { //flowType = 0
			nSeeds = calcNumSeedPoints(timeStep);
		}
		//Allocate an array to hold the seeds:
		float* seeds = new float[4*nSeeds];
		if (flowType == 2){
			actualNumSeeds = flowLib->GenRakeSeeds(seeds,seedTimeStart, randomSeed, 4);
			if (actualNumSeeds != nSeeds) {
				delete [] seeds; 
				delete flowLib;
				return 0;
			}
			for (int i = 0; i<nSeeds; i++){
				seeds[4*i+3] = (float)seedTimeStart;
			}
			*numseeds = actualNumSeeds;
			delete flowLib;
			return seeds;
		}
		if (flowType == 1) {
			//Do biased seeds for all time steps
			//Keep a pointer to the 
			actualNumSeeds = 0;
			for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
				int seedsFound = flowLib->GenRakeSeeds(seeds+4*actualNumSeeds,i, randomSeed, 4);
				if (seedsFound != calcNumSeedPoints(i)){
					delete [] seeds;
					delete flowLib;
					return 0;
				}
				//Plug in the time step:
				for (int j = 0; j< seedsFound; j++)
					*(seeds+4*(actualNumSeeds+j)+3) = (float)i;
				actualNumSeeds += seedsFound;
			}
			*numseeds = actualNumSeeds;
			delete flowLib;
			return seeds;
		}
		//Flowtype = 0:
		assert(flowType == 0);
		actualNumSeeds = flowLib->GenRakeSeeds(seeds,timeStep, randomSeed, 4);
		if (actualNumSeeds != nSeeds) {
			delete [] seeds; 
			delete flowLib;
			return 0;
		}
		for (int i = 0; i<nSeeds; i++){
			seeds[4*i+3] = (float)timeStep;
		}
		*numseeds = actualNumSeeds;
		delete flowLib;
		return seeds;
	} else { //seeds are same for all timesteps:
		nSeeds = calcNumSeedPoints(seedTimeStart);
		float* seeds = new float[nSeeds*4];
		int seedsFound = flowLib->GenRakeSeeds(seeds,seedTimeStart, randomSeed, 4);
		if (seedsFound != nSeeds){
			delete [] seeds;
			delete flowLib;
			return 0;
		}
		//Plug in the code for "all" time steps
		for (int j = 0; j< seedsFound; j++){
			seeds[4*j+3] = -1.f;
		}
		*numseeds = seedsFound;
		delete flowLib;
		return seeds;
	} 
}


//Function to iterate over sample time steps, whether in list or 
//specified by interval.  The first sample step is moved up to avoid
//unnecessary samples before the first seed time.
//This skips any timesteps for which the unsteady field is not present
//at the required refinement level
//Note that this is always called after the list of valid unsteady timesteps has been
//sent to flowlib, so it would be smarter to use that list in this function.

int FlowParams::
getUnsteadyTimestepSample(int index, int minStep, int maxStep, int unsteadyFlowDir){
	DataStatus* ds = DataStatus::getInstance();
	int minRefLevel = numRefinements;
	int minLOD = GetCompressionLevel();
	if (ds->useLowerAccuracy()) {minRefLevel = 0; minLOD = 0;}
	if (unsteadyFlowDir > 0){
		
		if (useTimestepSampleList){
			int position = -1;
			for (int i = 0; i< unsteadyTimestepList.size(); i++){
				int ts = unsteadyTimestepList[i];
				if (ts >= minStep && ts <= maxStep) {
					bool dataOK = true;
					for (int j = 0; j<3; j++){
						if (unsteadyVarNum[j] == 0) continue;
						if(ds->maxXFormPresent3D(unsteadyVarNum[j]-1, ts)< minRefLevel) dataOK = false;
						if (ds->maxLODPresent3D(unsteadyVarNum[j]-1, ts)< minLOD) dataOK = false;
					}
					if(!dataOK) continue; //skip this timestep
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
			for (;(ts <= maxStep && ts <= timeSamplingEnd); ts += timeSamplingInterval){
				bool dataOK = true;
				for (int j = 0; j<3; j++){
					if (unsteadyVarNum[j] == 0) continue;
					if(ds->maxXFormPresent3D(unsteadyVarNum[j]-1, ts)< minRefLevel) dataOK = false;
					if (ds->maxLODPresent3D(unsteadyVarNum[j]-1, ts)< minLOD) dataOK = false;
				}
				if(dataOK) break;
			}
			if (ts > maxStep || ts > timeSamplingEnd) return -1;
			return ts;
		}
	} else { //direction is negative:
		if (useTimestepSampleList){
			int position = -1;
			for (int i = unsteadyTimestepList.size()-1; i >= 0; i--){
				int ts = unsteadyTimestepList[i];
				if (ts >= minStep && ts <= maxStep) {
					bool dataOK = true;
					for (int j = 0; j<3; j++){
						if (unsteadyVarNum[j] == 0) continue;
						if(ds->maxXFormPresent3D(unsteadyVarNum[j]-1, ts)< minRefLevel) dataOK = false;
						if (ds->maxLODPresent3D(unsteadyVarNum[j]-1, ts)< minLOD) dataOK = false;
					}
					if(!dataOK) continue; //skip this timestep
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
			for (;(ts >= minStep && ts >= timeSamplingStart); ts -= timeSamplingInterval){
				bool dataOK = true;
				for (int j = 0; j<3; j++){
					if (unsteadyVarNum[j] == 0) continue;
					if(ds->maxXFormPresent3D(unsteadyVarNum[j]-1, ts)< minRefLevel) dataOK = false;
					if (ds->maxLODPresent3D(unsteadyVarNum[j]-1, ts)< minLOD) dataOK = false;
				}
				if(dataOK) break;
			}
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

//Generate steady flow data.
//Seeds are from one of three sources:
// If the input FlowLineData* is non-null, it contains the seeds
// Else, if the pathLineData is non-null, it contains the seeds,
// otherwise they come from the rake/seedlist
// If the FlowLineData* is 0, a new one is constructed, and returned.
// If the flowLineData is nonzero, it is reused and returned.
//Produces a FlowLineData containing the flow lines.
//If speeds are used for rgba mapping, they are (temporarily) calculated
//and then mapped.
//If prioritize is true, the 

FlowLineData* FlowParams::
regenerateSteadyFieldLines(VaporFlow* myFlowLib, FlowLineData* flowLines, PathLineData* pathData, 
			int timeStep, int minFrame, RegionParams* rParams, bool prioritize){
	float* seedList = 0;
	if (!myFlowLib) return 0;
	double seedBox[6];
	GetBox()->GetLocalExtents(seedBox);
	//If we are doing autoscale, calculate the average field 
	//magnitude at lowest resolution, over the region:
	if (autoScale){
		float avgMag = getAvgVectorMag(rParams, 0, timeStep);
		//If error, just use 1.
		if (avgMag <= 0.f) avgMag = 1.f;
		
		//Scale the steady field so that it will go steadyFlowLength diameters 
		//in unit time
		
		const float* fullExtents = DataStatus::getInstance()->getLocalExtents();
		float diff[3];
		vsub(fullExtents, fullExtents+3, diff);
		float diam = sqrt(vdot(diff, diff));
		steadyScale = diam*steadyFlowLength/avgMag;
		//And set the numsamples
		objectsPerFlowline = (int) (steadyFlowLength*steadySmoothness);
	}

	DataStatus* ds;
	//specify field components:
	ds = DataStatus::getInstance();
	const char* xVar = "0", *yVar = "0", *zVar = "0";
	
	if(steadyVarNum[0]>0) xVar = ds->getVariableName3D(steadyVarNum[0]-1).c_str();
	if(steadyVarNum[1]>0) yVar = ds->getVariableName3D(steadyVarNum[1]-1).c_str();
	if(steadyVarNum[2]>0) zVar = ds->getVariableName3D(steadyVarNum[2]-1).c_str();
	myFlowLib->SetSteadyFieldComponents(xVar, yVar, zVar);

	if (flowType == 2){ //establish prioritization field variables:
		xVar = ds->getVariableName3D(priorityVarNum[0]).c_str();
		yVar = ds->getVariableName3D(priorityVarNum[1]).c_str();
		zVar = ds->getVariableName3D(priorityVarNum[2]).c_str();
		myFlowLib->SetPriorityField(xVar, yVar, zVar, priorityMin, priorityMax);
	}
	
	if (!setupFlowRegion(rParams, myFlowLib, timeStep)) return 0;
	
	int numSeedPoints;
	if (flowLines) {  //Get the seeds from the flow lines:
		numSeedPoints = flowLines->getNumLines();
		seedList = new float[numSeedPoints*3];
		for (int i = 0; i<numSeedPoints; i++){
			for (int k = 0; k<3; k++) {
				seedList[3*i+k] = flowLines->getFlowPoint(i, flowLines->getSeedPosition())[k];
			}
		}
	} else if (pathData) {
		numSeedPoints = pathData->getNumLines();
	} else {// get the seed points from the rake or seedlist...
		if (doRake){
			numSeedPoints = getNumRakeSeedPoints();
			if (randomGen) {
				xVar = ds->getVariableName3D(seedDistVarNum[0]).c_str();
				yVar = ds->getVariableName3D(seedDistVarNum[1]).c_str();
				zVar = ds->getVariableName3D(seedDistVarNum[2]).c_str();
				myFlowLib->SetDistributedSeedPoints(seedBox, seedBox+3, (int)allGeneratorCount, 
					xVar, yVar, zVar, seedDistBias, flowType);
			} else {
				double boxmin[3], boxmax[3];
				for (int i = 0; i<3; i++){
					if (generatorCount[i] <= 1) {
						boxmin[i] = (seedBox[i]+seedBox[i+3])*0.5f;
						boxmax[i] = boxmin[i];
					} else {
						boxmin[i] = seedBox[i];
						boxmax[i] = seedBox[i+3];
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
			//Only give a warning if the seed list is nonempty:
			if (numSeedPoints == 0 && getNumListSeedPoints() > 0) {
				MyBase::SetErrMsg(VAPOR_WARNING_SEEDS, "No seeds at current time step");
				return 0;
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

	
	myFlowLib->SetIntegrationAccuracy(integrationAccuracy);
	myFlowLib->SetSteadyTimeSteps(timeStep, steadyFlowDirection); 
	myFlowLib->ScaleSteadyTimeStepSizes(steadyScale, 1.f/(float)objectsPerFlowline);
	
	maxPoints = calcMaxPoints();

	bool useSpeeds =  (getColorMapEntityIndex() == 2 );
	bool doRGBAs = (getColorMapEntityIndex() > 0);


	FlowLineData* steadyFlowData = flowLines;
	if (!steadyFlowData) steadyFlowData = new FlowLineData(numSeedPoints, maxPoints, useSpeeds, steadyFlowDirection, doRGBAs); 

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	bool rc;
	if (flowLines) { //Use the seedlist constructed above:
		rc = myFlowLib->GenStreamLinesNoRake(steadyFlowData,seedList);
	} else if (pathData){
	//generate the steady flow lines 
		rc =  myFlowLib->GenStreamLines(timeStep, steadyFlowData, pathData, prioritize);
	} else { // use rake or seed list..
		if (doRake){
			rc = myFlowLib->GenStreamLines(timeStep, steadyFlowData, randomSeed);
		} else {
			rc = myFlowLib->GenStreamLinesNoRake(steadyFlowData,seedList);
		}
	}
	//Restore original cursor:
	QApplication::restoreOverrideCursor();

	if (!rc){
		MyBase::SetErrMsg(VAPOR_ERROR_INTEGRATION, "Error integrating steady flow lines");	
		delete steadyFlowData;
		return 0;
	}
	
	//Now map colors (if needed)
		
	if (doRGBAs){
		mapColors(steadyFlowData, timeStep, minFrame, rParams);
	}
	//Rescale now (after color mapping, before rendering)!!!
	steadyFlowData->scaleLines(DataStatus::getInstance()->getStretchFactors());

	return steadyFlowData;
}
////////////////////////////////////////////////////////////
//  Method to create a new PathLineData for unsteady flow, and
//  to insert the appropriate seeds into it.
//  Does setup of flow lib for unsteady advection
//  Returns null if there are no seeds, or if unable to allocate memory
///////////////////////////////////////////////////////////
FlowLineData* FlowParams::
setupUnsteadyStartData(VaporFlow* flowLib, int minFrame, int maxFrame, RegionParams* rParams){
	//The number of lines is the total number of seeds to be inserted.
	int numLines = 0;
	//The max number of points in the unsteady flow
	int mPoints = objectsPerTimestep*(maxFrame - minFrame + 1);
	double seedBox[6];
	GetBox()->GetLocalExtents(seedBox);
	//build the unsteady timestep sample list.  This lists the timesteps to be sampled
	//in the order of integration.  Don't include timesteps for which the unsteady field
	//is not available at the required resolution
	DataStatus* ds = DataStatus::getInstance();
	int minRefLevel = numRefinements;
	int minLOD = GetCompressionLevel();
	if (ds->useLowerAccuracy()) {minRefLevel = 0; minLOD = 0;}
	int numTimesteps;
	int* timeStepList;
	int numValidTimesteps = 0;
	if (useTimestepSampleList) {
		numTimesteps = (int)unsteadyTimestepList.size();
		timeStepList = new int[numTimesteps];
		for (int i = 0; i<numTimesteps; i++){
			bool levelOK = true;
			for (int j = 0; j<3; j++){
				if (unsteadyVarNum[j]) { 
					if(ds->maxXFormPresent3D(unsteadyVarNum[j]-1, unsteadyTimestepList[i]) < minRefLevel){
						levelOK = false; break;
					}
					if(ds->maxLODPresent3D(unsteadyVarNum[j]-1, unsteadyTimestepList[i]) < minLOD){
						levelOK = false; break;
					}
				}
			}
			if (levelOK)
				timeStepList[numValidTimesteps++] = unsteadyTimestepList[i];
		}
	}
	else {
		numTimesteps = 1+ (timeSamplingEnd - timeSamplingStart)/timeSamplingInterval;
		timeStepList = new int[numTimesteps];

		for (int i = 0; i< numTimesteps; i++){
			bool levelOK = true;
			int tstep = timeSamplingStart + i*timeSamplingInterval;
			for (int j = 0; j<3; j++){
				if (unsteadyVarNum[j]){ 
					if(ds->maxXFormPresent3D(unsteadyVarNum[j]-1, tstep) < minRefLevel){
						levelOK = false; break;
					}
					if(ds->maxLODPresent3D(unsteadyVarNum[j]-1, tstep) < minLOD){
						levelOK = false; break;
					}
				}
			}
			if(levelOK) timeStepList[numValidTimesteps++] = tstep;
		}
	}
	if (numValidTimesteps < 2){
		MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "Insufficient time steps for unsteady advection");
		delete [] timeStepList;
		return false;
	}
	if (numValidTimesteps < numTimesteps && ds->warnIfDataMissing()){
		
		SetErrMsg(VAPOR_WARNING_DATA_UNAVAILABLE,"Unsteady flow field data not available\nat required level %d for all requested timesteps\n %s", 
			minRefLevel,
			"This message can be silenced\nusing the User Preference Panel settings." );
	}
	
	if (!setupFlowRegion(rParams, flowLib, -1)) return 0;

	//Get bounds
	// setup integration parameters:
	flowLib->SetIntegrationAccuracy(integrationAccuracy);

	
	const char* xVar="0", *yVar="0", *zVar="0";
	
	if(unsteadyVarNum[0]>0)xVar = ds->getVariableName3D(unsteadyVarNum[0]-1).c_str();
	if(unsteadyVarNum[1]>0)yVar = ds->getVariableName3D(unsteadyVarNum[1]-1).c_str();
	if(unsteadyVarNum[2]>0)zVar = ds->getVariableName3D(unsteadyVarNum[2]-1).c_str();
	flowLib->SetUnsteadyFieldComponents(xVar, yVar, zVar);
	
	flowLib->SetUnsteadyTimeSteps(timeStepList,numValidTimesteps);
	
	flowLib->ScaleUnsteadyTimeStepSizes(unsteadyScale, (float)objectsPerTimestep);

	float sampleRate = (float)objectsPerTimestep;
	
	//Need to set up flowlib for rake seed generation
	if (doRake){
		
		if (randomGen) {
			xVar = ds->getVariableName3D(seedDistVarNum[0]).c_str();
			yVar = ds->getVariableName3D(seedDistVarNum[1]).c_str();
			zVar = ds->getVariableName3D(seedDistVarNum[2]).c_str();
			flowLib->SetDistributedSeedPoints(seedBox, seedBox+3, (int)allGeneratorCount, 
				xVar, yVar, zVar, seedDistBias, flowType);
		} else {
			double boxmin[3], boxmax[3];
			for (int i = 0; i<3; i++){
				if (generatorCount[i] <= 1) {
					boxmin[i] = (seedBox[i]+seedBox[i+3])*0.5f;
					boxmax[i] = boxmin[i];
				} else {
					boxmin[i] = seedBox[i];
					boxmax[i] = seedBox[i+3];
				}
			}
			flowLib->SetRegularSeedPoints(boxmin, boxmax, generatorCount);
			
		}
	}
	
	if (flowType == 2){ //field line advection only inserts at first seed frame:
		int numLines = calcNumSeedPoints(seedTimeStart);
		if (flaAdvectBeforePrioritize) {
			//Here there is no path line data.
			//All points are kept in flow line data (steadyCache)
			//Get some initial settings:
			maxPoints = calcMaxPoints();
			bool useSpeeds =  (getColorMapEntityIndex() == 2 );
			bool doRGBAs = (getColorMapEntityIndex() );
			FlowLineData* flData1 = new FlowLineData(numLines, maxPoints, useSpeeds, steadyFlowDirection, doRGBAs); 
			//Now set this up with seed points:
			int nlines = insertSteadySeeds(rParams, flowLib, flData1, seedTimeStart);
			if (nlines <= 0) {
				MyBase::SetErrMsg(VAPOR_ERROR_SEEDS, "No seeds to start field line advection");
				delete flData1;
				return 0;
			}
			//Then do a steady advection at the start time
			FlowLineData* flData = regenerateSteadyFieldLines(flowLib, flData1, 0, seedTimeStart, minFrame, rParams, false);
			if (!flData) {
				delete flData1;
			}
			return flData;
		} else {
			//In this version of field line advection, 
			// we don't store speeds or colors in the path line data,
			// just in the steady flowLineData
			PathLineData* pathLines = new PathLineData(numLines, mPoints, false, false, minFrame, maxFrame, sampleRate);
			// Now insert the seeds:
			int seedsInserted = insertUnsteadySeeds(rParams, flowLib, pathLines, seedTimeStart);
			if (seedsInserted <= 0) { 
				//Make sure that (if list is used) that it's nonempty
				if (doRake || listEnabled()){
					MyBase::SetErrMsg(VAPOR_ERROR_SEEDS,"No valid flow seeds to advect");
					delete pathLines;
					return 0;
				}
			}
			return (FlowLineData*)pathLines;
		}
	} else { //Unsteady flow; Insert at all seed frames (independent of sampling)
		//Count the number of lines (i.e. seeds) for all seed times.
		for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
			if (i < minFrame || i > maxFrame) continue;
			numLines += calcNumSeedPoints(i);
		}
		if (numLines <= 0) return 0;
		bool useSpeeds = (getColorMapEntityIndex() == 2 );
		bool doRGBAs = (getColorMapEntityIndex() > 0);
		PathLineData* pathLines = new PathLineData(numLines, mPoints, useSpeeds, doRGBAs, minFrame, maxFrame, sampleRate);
		// Now insert the seeds:
		int seedsInserted = 0;
		for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
			//Don't insert if invalid timestep 
			if (i < minFrame || i > maxFrame) continue;
			seedsInserted += insertUnsteadySeeds(rParams, flowLib, pathLines, i);
		}
		if (seedsInserted <= 0) { 
			if (doRake || listEnabled()){
				MyBase::SetErrMsg(VAPOR_ERROR_SEEDS,"No valid flow seeds to advect");
				delete pathLines;
				return 0;
			}
		}
		return pathLines;
	}
}
//Insert seeds for the specified timeStep into pathLineData.
//Return the actual number of seeds that were inserted 
//If the seed list is being used, only use seeds that are valid at the specified
//timestep
int FlowParams::insertUnsteadySeeds(RegionParams* rParams, VaporFlow* fLib, PathLineData* pathLines, int timeStep){
	int seedCount = 0;
	//Get the region bounds for testing seeds:
	double minExt[3], maxExt[3];
	size_t min_dim[3], max_dim[3]; 

	int sesvarnums[3];
	int varcount = 0;
	int lod = GetCompressionLevel();
	for (int i = 0; i< 3; i++){if (unsteadyVarNum[i]) sesvarnums[varcount++] = unsteadyVarNum[i] -1;}
	int dataValid = rParams->getAvailableVoxelCoords(numRefinements,lod,  min_dim, max_dim, 
			timeStep, sesvarnums, varcount, minExt, maxExt);
	if (dataValid < 0) {
		MyBase::SetErrMsg(VAPOR_ERROR_SEEDS, "Unable to inject seeds at current time step. %s\n %s\n",
			"Vector field may not be available;",
			"Also be sure that seed injection\ntimes are timestep sample times");
		return 0;
	}
	float userExts[6];
	rParams->GetBox()->GetUserExtents(userExts,timeStep);
	
	int seedsInRegion = 0;
	if(doRake){
		//Allocate an array to hold the seeds:
		float* seeds = new float[3*calcNumSeedPoints(timeStep)];
		seedCount = fLib->GenRakeSeeds(seeds, timeStep, randomSeed);
		if (seedCount < calcNumSeedPoints(timeStep)) {
			delete [] seeds;
			return 0;
		}
		for (int i = 0; i<seedCount; i++){
			bool inside = true;
			for (int j = 0; j< 3; j++){
				if (seeds[3*i+j] < userExts[j] || seeds[3*i+j] > userExts[j+3]) inside = false;
			}
			if (inside) seedsInRegion++;
			pathLines->insertSeedAtTime(i, timeStep, seeds[3*i], seeds[3*i+1], seeds[3*i+2]);
		}
		delete [] seeds;
		

	} else { //insert from seedList
		
		for (int i = 0; i< getNumListSeedPoints(); i++){
			Point4 point = seedPointList[i];
			float tstep = point.getVal(3);
			if (tstep >= 0.f && ((int)(tstep +0.5f) != timeStep)) continue;
			bool inside = true;
			for (int j = 0; j< 3; j++){
				if (point.getVal(j) < userExts[j] || point.getVal(j) > userExts[j+3]) inside = false;
			}
			if (inside) seedsInRegion++;
			pathLines->insertSeedAtTime(i, timeStep, point.getVal(0), point.getVal(1), point.getVal(2));
			seedCount++;
		}
	}
	if (seedCount > seedsInRegion){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,
			"Flow seeds are outside region: \n %d seeds outside of total %d seeds.",
			seedCount - seedsInRegion, seedCount);
	}

	return seedCount;

}
//Insert seeds for the specified timeStep into fieldLineData, using rake or 
//list.
//Return the actual number of seeds that were inserted
//If the seed list is being used, only use seeds that are valid at the specified
//timestep
int FlowParams::insertSteadySeeds(RegionParams* rParams, VaporFlow* fLib, FlowLineData* flowLines, int timeStep){
	int seedCount = 0;
	//Get the region bounds for testing seeds:
	double minExt[3], maxExt[3];
	size_t min_dim[3], max_dim[3]; 
	
	int sesvarnums[3];
	int varcount = 0;
	int lod = GetCompressionLevel();
	for (int i = 0; i< 3; i++){if (steadyVarNum[i]) sesvarnums[varcount++] = steadyVarNum[i] -1;}
	int dataValid = rParams->getAvailableVoxelCoords(numRefinements, lod, min_dim, max_dim, 
			timeStep, sesvarnums,varcount, minExt, maxExt);
	if (dataValid < 0) {
		MyBase::SetErrMsg(VAPOR_ERROR_SEEDS, "Unable to inject seeds at current time step. %s\n %s\n",
			"Vector field may not be available;",
			"Also be sure that seed injection\ntimes are timestep sample times");
		return 0;
	}
	
	int seedsInRegion = 0;
	if(doRake){
		//Allocate an array to hold the seeds:
		float* seeds = new float[3*calcNumSeedPoints(timeStep)];
		seedCount = fLib->GenRakeSeeds(seeds, timeStep, randomSeed);
		if (seedCount < calcNumSeedPoints(timeStep)) {
			delete [] seeds;
			return 0;
		}
		for (int i = 0; i<seedCount; i++){
			bool inside = true;
			for (int j = 0; j< 3; j++){
				if (seeds[3*i+j] < minExt[j] || seeds[3*i+j] > maxExt[j]) inside = false;
			}
			if (inside) seedsInRegion++;
			//Set this seed as the start point in the line:
			flowLines->setFlowPoint(i, 0, seeds[3*i], seeds[3*i+1], seeds[3*i+2]);
			flowLines->setFlowStart(i,0);
			flowLines->setFlowEnd(i,0);
		}
		delete [] seeds;
		

	} else { //insert from seedList
		
		for (int i = 0; i< getNumListSeedPoints(); i++){
			Point4 point = seedPointList[i];
			float tstep = point.getVal(3);
			if (tstep >= 0.f && ((int)(tstep +0.5f) != timeStep)) continue;
			bool inside = true;
			for (int j = 0; j< 3; j++){
				if (point.getVal(j) < minExt[j] || point.getVal(j) > maxExt[j]) inside = false;
			}
			if (inside) seedsInRegion++;
			flowLines->setFlowPoint(seedCount, 0, point.getVal(0), point.getVal(1), point.getVal(2));
			flowLines->setFlowStart(seedCount,0);
			flowLines->setFlowEnd(seedCount,0);
			seedCount++;
		}
	}
	if (seedCount > seedsInRegion){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,
			"Flow seeds are outside region: \n %d seeds outside of total %d seeds.",
			seedCount - seedsInRegion, seedCount);
	}
	return seedCount;

}
//Method to construct Xml for state saving
ParamNode* FlowParams::
buildNode() {
	DataStatus* ds;
	ds = DataStatus::getInstance();
	if (numComboVariables <= 0) return 0;
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
	oss << (long)compressionLevel;
	attrs[_CompressionLevelTag] = oss.str();

	oss.str(empty);
	oss << (int)GetFidelityLevel();
	attrs[_FidelityLevelTag] = oss.str();
	oss.str(empty);
	if (GetIgnoreFidelity())
		oss << "true";
	else 
		oss << "false";
	attrs[_IgnoreFidelityTag] = oss.str();

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
	oss << (double)seedDistBias;
	attrs[_seedDistBiasAttr] = oss.str();
	oss.str(empty);

	oss << (double)priorityMin <<" "<<(double)priorityMax;
	attrs[_priorityBoundsAttr] = oss.str();

	oss.str(empty);
	oss << (long)timeSamplingStart<<" "<<timeSamplingEnd<<" "<<timeSamplingInterval;
	attrs[_timeSamplingAttr] = oss.str();

	oss.str(empty);
	if (useTimestepSampleList){
		oss << "true";
	} else {
		oss << "false";
	}
	attrs[_useTimestepSampleListAttr] =  oss.str();

	if (unsteadyTimestepList.size()> 0){
		oss.str(empty);
		//First entry is the size of the list, followed by the values in the list
		oss << (long) unsteadyTimestepList.size();
		for (int i = 0; i<unsteadyTimestepList.size(); i++){
			oss<<" "<<(long)unsteadyTimestepList[i];
		}
		attrs[_timestepSampleListAttr] = oss.str();
	}

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
	if (flaAdvectBeforePrioritize)
		oss << "true";
	else 
		oss << "false";
	attrs[_advectBeforePrioritizeAttr] = oss.str();

	oss.str(empty);
	oss << numFLASamples;
	attrs[_numFLASamplesAttr] = oss.str();
	
	oss.str(empty);
	if (autoScale)
		oss << "true";
	else 
		oss << "false";
	attrs[_autoScaleAttr] = oss.str();

	oss.str(empty);
	oss << flowType;
	attrs[_flowTypeAttr] = oss.str();

	oss.str(empty);
	const char* varnam1[3] = {"0","0","0"};
	for (int i = 0; i<3; i++){
		if(steadyVarNum[i]> 0) varnam1[i] = ds->getVariableName3D(steadyVarNum[i]-1).c_str();
	}
	oss << varnam1[0] <<" " << varnam1[1] <<" "<< varnam1[2];
		//oss << ds->getVariableName3D(steadyVarNum[0])<<" "<<ds->getVariableName3D(steadyVarNum[1])<<" "<<ds->getVariableName3D(steadyVarNum[2]);
	attrs[_steadyVariableNamesAttr] = oss.str();
	
	oss.str(empty);
	
	const char* varnam2[3] = {"0","0","0"};
	for (int i = 0; i<3; i++){
		if(unsteadyVarNum[i]> 0) varnam2[i] = ds->getVariableName3D(unsteadyVarNum[i]-1).c_str();
	}
	oss << varnam2[0] <<" "<< varnam2[1]<<" " << varnam2[2];
	//oss << ds->getVariableName3D(unsteadyVarNum[0])<<" "<<ds->getVariableName3D(unsteadyVarNum[1])<<" "<<ds->getVariableName3D(unsteadyVarNum[2]);
	attrs[_unsteadyVariableNamesAttr] = oss.str();

	oss.str(empty);
	oss << ds->getVariableName3D(seedDistVarNum[0])<<" "<<ds->getVariableName3D(seedDistVarNum[1])<<" "<<ds->getVariableName3D(seedDistVarNum[2]);
	attrs[_seedDistVariableNamesAttr] = oss.str();

	oss.str(empty);
	oss << ds->getVariableName3D(priorityVarNum[0])<<" "<<ds->getVariableName3D(priorityVarNum[1])<<" "<<ds->getVariableName3D(priorityVarNum[2]);
	attrs[_priorityVariableNamesAttr] = oss.str();

	oss.str(empty);
	if (useDisplayLists)
		oss << "true";
	else 
		oss << "false";
	attrs[_useDisplayListsAttr] = oss.str();


	ParamNode* flowNode = new ParamNode(_flowParamsTag, attrs, 2+numComboVariables);

	//Now add children:  
	//There's a child for geometry, a child for
	//Seeding, and a child for each variable.
	//The geometry child contains the transferFunction
	
	
	attrs.clear();

	oss.str(empty);
	double seedBox[6];
	GetBox()->GetLocalExtents(seedBox);
	oss << (double)seedBox[0]<<" "<<(double)seedBox[1]<<" "<<(double)seedBox[2];
	attrs[_seedRegionMinAttr] = oss.str();

	oss.str(empty);
	oss << (double)seedBox[3]<<" "<<(double)seedBox[4]<<" "<<(double)seedBox[5];
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

	ParamNode* seedingNode = new ParamNode(_seedingTag,attrs,getNumListSeedPoints());

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
		ParamNode* seedNode = new ParamNode(_seedPointTag, attrs, 0);
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
	oss << (float)diamondDiameter;
	attrs[_diamondDiameterAttr] = oss.str();
	oss.str(empty);
	oss << (float)arrowDiameter;
	attrs[_arrowDiameterAttr] = oss.str();
	oss.str(empty);
	oss << getColorMapEntityIndex();
	attrs[_colorMappedEntityAttr] = oss.str();
	
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
	if(transferFunction){
		oss.str(empty);
		oss << transferFunction->getOpacityScaleFactor();
		attrs[_opacityScaleAttr] = oss.str();
	}

	ParamNode* graphicNode = new ParamNode(_geometryTag,attrs,2*numComboVariables+1);

	//Create a mapper function node, add it as child
	if(transferFunction) {
		ParamNode* mfNode = transferFunction->buildNode();
		graphicNode->AddChild(mfNode);
	}
	
	flowNode->AddChild(graphicNode);
	//Create a node for each of the variables
	for (int i = 0; i< numComboVariables+4; i++){
		map <string, string> varAttrs;
		oss.str(empty);
		if (i > 3){
			oss << ds->getVariableName3D(i-4);
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
		flowNode->NewChild(_variableTag, varAttrs, 0);
	}
	
	return flowNode;
}
//Parse flow params from file:
bool FlowParams::
elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tagString, const char ** attrs){
	
	//Take care of attributes of flowParamsNode
	if (StrCmpNoCase(tagString, _flowParamsTag) == 0) {
		SetIgnoreFidelity(true);
		SetFidelityLevel(0);
		//Start with a new, default transferFunction
		if (transferFunction) delete transferFunction;
		transferFunction = new TransferFunction(this, 8);

		int newNumVariables = 0;
		//Default autoscale to false, consistent with pre-1.2
		autoScale = false;
		
		steadyFlowLength = 1.f;
		steadySmoothness = 20.f;
		steadyFlowDirection = 1;  //Forward was default for previous versions
		unsteadyFlowDirection = 1;
		useDisplayLists = false;
		//initially set field varnums to -1. 
		//Once they are set, we compare to steady varnums for setting flags
		for (int i = 0; i< 3; i++){
			seedDistVarNum[i] = -1;
			priorityVarNum[i] = -1;
		}
		priorityIsSteady = true;
		seedDistIsSteady = true;
		useTimestepSampleList = false;
		unsteadyTimestepList.clear();

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
					steadyVarNum[i] = varnum+1;
					unsteadyVarNum[i] = varnum+1;
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
					if (strcmp(vName.c_str(),"0") != 0){
						int varnum = DataStatus::getInstance()->mergeVariableName(vName);
						steadyVarNum[i] = varnum+1;
					} else {
						steadyVarNum[i] = 0;
					}
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboSteadyVarNum[i] = -1;
				}
			}
			else if (StrCmpNoCase(attribName, _unsteadyVariableNamesAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the unsteady name
					if (strcmp(vName.c_str(),"0") != 0){
						int varnum = DataStatus::getInstance()->mergeVariableName(vName);
						unsteadyVarNum[i] = varnum+1;
					} else unsteadyVarNum[i] = 0;
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboUnsteadyVarNum[i] = -1;	
				}
			}
			else if (StrCmpNoCase(attribName, _seedDistVariableNamesAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the steady name
					int varnum = DataStatus::getInstance()->mergeVariableName(vName);
					seedDistVarNum[i] = varnum;
					
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboSeedDistVarNum[i] = -1;
				}
			}
			else if (StrCmpNoCase(attribName, _priorityVariableNamesAttr) == 0) {
				string vName;
				for (int i = 0; i< 3; i++){
					ist >> vName;//peel off the steady name
					int varnum = DataStatus::getInstance()->mergeVariableName(vName);
					priorityVarNum[i] = varnum;
					
					//The combo setting will need to change when/if the variable is
					//read in the metadata
					comboPriorityVarNum[i] = -1;
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
			else if (StrCmpNoCase(attribName, _steadyFlowAttr) == 0) {//backwards compatibility
				if (value == "true") setFlowType(0); else setFlowType(1);
			}
			else if (StrCmpNoCase(attribName, _useDisplayListsAttr) == 0) {
				if (value == "true") useDisplayLists = true; else useDisplayLists = false;
			}
			else if (StrCmpNoCase(attribName, _flowTypeAttr) == 0) {
				ist >> flowType;
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			else if (StrCmpNoCase(attribName, _CompressionLevelTag) == 0){
				ist >> compressionLevel;
			}
			else if (StrCmpNoCase(attribName, _FidelityLevelTag) == 0){
				int fid;
				ist >> fid;
				SetFidelityLevel(fid);
			}
			else if (StrCmpNoCase(attribName, _IgnoreFidelityTag) == 0){
				if (value == "true") SetIgnoreFidelity(true); 
				else SetIgnoreFidelity(false);
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
			else if (StrCmpNoCase(attribName, _velocityScaleAttr) == 0){//obsolete, used in 1.0
				ist >> steadyScale;
				unsteadyScale = steadyScale;
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
			else if (StrCmpNoCase(attribName, _useTimestepSampleListAttr) == 0){
				if (value == "true") useTimestepSampleList = true; else useTimestepSampleList = false;
			}

			else if (StrCmpNoCase(attribName, _timestepSampleListAttr) == 0){
				//The first value is the list size, followed by the entries in the list:
				int listLength, tstep;
				ist >> listLength;
				for (int i = 0; i<listLength; i++){
					ist >> tstep;
					unsteadyTimestepList.push_back(tstep);
				}
			}

			else if (StrCmpNoCase(attribName, _smoothnessAttr) == 0) {
				ist >> steadySmoothness;
			}
			else if (StrCmpNoCase(attribName, _steadyFlowLengthAttr) == 0) {
				ist >> steadyFlowLength;
			}
			else if (StrCmpNoCase(attribName, _priorityBoundsAttr) == 0) {
				ist >> priorityMin; ist>>priorityMax;
			}
			else if (StrCmpNoCase(attribName, _seedDistBiasAttr) == 0) {
				ist >> seedDistBias;
			}
			else if (StrCmpNoCase(attribName, _advectBeforePrioritizeAttr) == 0) {
				if (value == "true") flaAdvectBeforePrioritize = true;
				else flaAdvectBeforePrioritize = false;
			}
			else if (StrCmpNoCase(attribName, _numFLASamplesAttr) == 0) {
				ist >> numFLASamples;
			}
		}
		//Reset the variable Names, Nums, bounds.  These will be initialized
		//by the geometry nodes
		numComboVariables = newNumVariables;
		
		delete [] minColorBounds;
		delete [] maxColorBounds;
		minColorBounds = new float[numComboVariables+4];
		maxColorBounds = new float[numComboVariables+4];
		//Do same for edit bounds
		delete [] minOpacEditBounds;
		delete [] maxOpacEditBounds;
		delete [] minColorEditBounds;
		delete [] maxColorEditBounds;
		minOpacEditBounds = new float[numComboVariables+4];
		maxOpacEditBounds = new float[numComboVariables+4];
		minColorEditBounds = new float[numComboVariables+4];
		maxColorEditBounds = new float[numComboVariables+4];
		return true;
	}
	//Parse the seeding node:
	else if (StrCmpNoCase(tagString, _seedingTag) == 0) {
		//Clear out the seeds, will be added in later if they are found.
		seedPointList.clear();
		float seedBox[6];
		GetBox()->GetLocalExtents(seedBox);
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
		
			if (StrCmpNoCase(attribName, _seedRegionMinAttr) == 0) {
				ist >> seedBox[0];ist >> seedBox[1];ist >> seedBox[2];
				GetBox()->SetLocalExtents(seedBox);
			}
			else if (StrCmpNoCase(attribName, _seedRegionMaxAttr) == 0) {
				
				ist >> seedBox[3];ist >> seedBox[4];ist >> seedBox[5];
				GetBox()->SetLocalExtents(seedBox);
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
			else if (StrCmpNoCase(attribName, _diamondDiameterAttr) == 0) {
				ist >> diamondDiameter;
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
				//obsolete
			}
			else if (StrCmpNoCase(attribName, _constantColorAttr) == 0){
				int r,g,b;
				ist >> r; ist >>g; ist >> b;
				constantColor = qRgb(r,g,b);
			} 
			else if (StrCmpNoCase(attribName, _opacityScaleAttr) == 0){
				float opacScale;
				ist >> opacScale;
				transferFunction->setOpacityScaleFactor(opacScale);
			} 
			else if (StrCmpNoCase(attribName, _constantOpacityAttr) == 0){
				ist >> constantOpacity;
			}
			
		}
	//Parse a mapperFunction node (inside the geometry node) (prior to 2.3.0)
	} else if (StrCmpNoCase(tagString, MapperFunction::_mapperFunctionTag) == 0) {
		//Need to "push" to transfer function parser.
		//That parser will "pop" back to flowparams when done.
		
		pm->pushClassStack(transferFunction);
		transferFunction->elementStartHandler(pm, depth, tagString, attrs);
		
		return true;
	} else if (StrCmpNoCase(tagString, TransferFunction::_transferFunctionTag) == 0) {
		//Need to "push" to mapper function parser.
		//That parser will "pop" back to flowparams when done.
		
		pm->pushClassStack(transferFunction);
		transferFunction->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	//Parse variableMapping tags:
	} else if (StrCmpNoCase(tagString,_variableTag) == 0){
		//Save the attributes:
		string varName = "";
		
		float leftColorBound = 0.f;
		float rightColorBound = 1.f;
		
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
				//obsolete
			}
			else if (StrCmpNoCase(attribName, _rightOpacityBoundAttr) == 0) {
				//obsolete
			}
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
	
		minColorEditBounds[varNum]= leftColorBound;
		maxColorEditBounds[varNum] = rightColorBound;
		minOpacEditBounds[varNum]= leftColorBound;
		maxOpacEditBounds[varNum] = rightColorBound;
		
	}
	return true;
}
	
	
bool FlowParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	if (StrCmpNoCase(tag, _flowParamsTag) == 0) {
		
		setMinColorEditBound(getMinColorMapBound(),getColorMapEntityIndex());
		setMaxColorEditBound(getMaxColorMapBound(),getColorMapEntityIndex());
		setMinOpacEditBound(getMinOpacMapBound(),getColorMapEntityIndex());
		setMaxOpacEditBound(getMaxOpacMapBound(),getColorMapEntityIndex());
		
		//Check if the seedDist and priority field names were set.
		// if not, set them to the steady field:
		for (int i = 0; i<3; i++){
			int steadyNum = (steadyVarNum[i] > 0 ? steadyVarNum[i]-1 : 0);
			if (seedDistVarNum[i] == -1) seedDistVarNum[i] = steadyNum;
			if (priorityVarNum[i] == -1) priorityVarNum[i] = steadyNum;
			if (seedDistVarNum[i] != steadyVarNum[i]-1) seedDistIsSteady = false;
			if (priorityVarNum[i] != steadyVarNum[i]-1) priorityIsSteady = false;
		}

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
	else if (StrCmpNoCase(tag, MapperFunction::_mapperFunctionTag) == 0)	
		return true;
	else if (StrCmpNoCase(tag, TransferFunction::_transferFunctionTag) == 0) 
		return true;
	else {
		pm->parseError("Unrecognized end tag in FlowParams %s",tag.c_str());
		return false; 
	}
}
	


//Generate a list of colors and opacities, one per (valid) vertex.
//Puts them into the FlowLineData
//Note that, if a variable is mapped, only the first time step is used for
//the mapping
//
void FlowParams::
mapColors(FlowLineData* container, int currentTimeStep, int minFrame, RegionParams* rParams){
	//Create lut based on current mapping data
	float* lut = new float[256*4];
	transferFunction->makeLut(lut);
	//Setup mapping
	
	float colorMin = transferFunction->getMinMapValue();
	float colorMax = transferFunction->getMaxMapValue();

	float colorVar;
	RegularGrid* colorGrid = 0;
	//min and max of valid mappings into data volume:
	int colorMinMap[3], colorMaxMap[3];
	
	//separate color, opac 
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if(!dataMgr) return;
	//Make sure RGBAs are available if needed:
	if (getColorMapEntityIndex() > 0)
		container->enableRGBAs();
		
	
	if (getColorMapEntityIndex() > 3){
		//set up args for GetGrid
		//If flow is unsteady, just get the first available timestep  (BUG!!!)
		int timeStep = currentTimeStep;
		if(flowType == 1){//unsteady flow
			timeStep = ds->getFirstTimestep(getColorMapEntityIndex()-4);
			if (timeStep < 0) {
				MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"No data for flow mapped variable");
				return;
			}
		}
		
		size_t min_dim[3], max_dim[3];
		double validExts[6];
		rParams->GetBox()->GetUserExtents(validExts, (size_t)timeStep);
		int lod = GetCompressionLevel();
		vector<string> colorVarname;
		colorVarname.push_back(colorMapEntity[getColorMapEntityIndex()]);
		int colorRefLevel = RegionParams::PrepareCoordsForRetrieval(numRefinements, lod, timeStep, colorVarname, 
			validExts, validExts+3,min_dim, max_dim);
		if (colorRefLevel < 0) {
			SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"flow color mapping data unavailable at timestep %d\n", timeStep);
			setBypass(timeStep);
			return;
		}

		//Obtain the required grid from the DataMgr.  The LOD of the data may need to be reduced.
	
		int useLOD = GetCompressionLevel();
		int maxLOD = ds->maxLODPresent(colorVarname[0],timeStep);
		if (maxLOD < useLOD){
			if (!ds->useLowerAccuracy()){
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Color data unavailable at LOD %d\n", GetCompressionLevel());
				setBypass(timeStep);
				return;
			}
			useLOD = maxLOD;
		}

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		colorGrid = dataMgr->GetGrid(timeStep, colorVarname[0],colorRefLevel, useLOD,min_dim,max_dim,0);
		QApplication::restoreOverrideCursor();

		if (!colorGrid){
			DataStatus::getInstance()->getDataMgr()->SetErrCode(0);
			if (DataStatus::getInstance()->warnIfDataMissing())
				MyBase::SetErrMsg(VAPOR_ERROR_FLOW_DATA,"Color mapped variable data unavailable\nfor refinement %d at timestep %d", colorRefLevel, timeStep);
			return;
		}
		for (int i = 0; i<3; i++){
			colorMinMap[i] = min_dim[i];
			colorMaxMap[i] = max_dim[i];
		}
	}
	
	//Cycle through all the points.  Map to rgba as we go.
	//

	for (int lineNum = 0; lineNum<container->getNumLines(); lineNum++){
		for (int pointNum = container->getStartIndex(lineNum); pointNum<= container->getEndIndex(lineNum); pointNum++){
		
			switch (getColorMapEntityIndex()){
				case (0): //constant
					colorVar = 0.f;
					break;
				case (1): //age
					if (flowType != 1)//relative position on line
						colorVar = (float)pointNum/((float)objectsPerFlowline);
					else //type = 1:  time step
						colorVar = (float)(minFrame + (float)pointNum/(float)objectsPerTimestep);
					break;
				case (2): //speed
					//In case the speeds haven't been built yet:
					if (container->doSpeeds())
						colorVar = container->getSpeed(lineNum,pointNum);
					else colorVar = 0.f;
					break;

				case (3) : //seed index.  Will use same color along each line

					colorVar = container->getSeedIndex(lineNum);
					break;
				default : //variable
		
					float* dataPoint = container->getFlowPoint(lineNum,pointNum);
					assert(dataPoint[0] != IGNORE_FLAG);
					colorVar = colorGrid->GetValue(dataPoint[0],dataPoint[1],dataPoint[2]);
					if (colorVar == colorGrid->GetMissingValue()) {
						//Edge points should get colored by the previous or next point
						int nextPointNum = -1;
						colorVar = colorMin;
						if(pointNum == container->getStartIndex(lineNum) && pointNum < container->getEndIndex(lineNum))
							nextPointNum = pointNum+1;
						else if (pointNum == container->getEndIndex(lineNum) && pointNum > container->getStartIndex(lineNum))
							nextPointNum = pointNum-1;
						if (nextPointNum >=0){
							dataPoint = container->getFlowPoint(lineNum,nextPointNum);
							colorVar = colorGrid->GetValue(dataPoint[0],dataPoint[1],dataPoint[2]);
							if(colorVar == colorGrid->GetMissingValue()) colorVar = colorMin;
						}	
					}
					break;
			}
			int colorIndex = (int)((colorVar - colorMin)*255.99/(colorMax-colorMin));
			if (colorIndex<0) colorIndex = 0;
			if (colorIndex> 255) colorIndex =255;
			//Now assign color etc.
			
			if (getColorMapEntityIndex() == 0){
				container->setRGB(lineNum,pointNum,((float)qRed(constantColor))/255.f,
					((float)qGreen(constantColor))/255.f,
					((float)qBlue(constantColor))/255.f);
				container->setAlpha(lineNum,pointNum,constantOpacity);
			} else {
				container->setRGB(lineNum,pointNum,lut[4*colorIndex],
					lut[4*colorIndex+1],
					lut[4*colorIndex+2]);
				container->setAlpha(lineNum,pointNum,lut[4*colorIndex+3]);
			}
		}
		
	}
}
//Same as above, but for unsteady flow.  Assumes color is mapped to a variable.
// color variable is retrieved for
// each time step along the flow.
//
void FlowParams::
mapUnsteadyColors(PathLineData* container, int startTimeStep, int minFrame, RegionParams* rParams){
	//Only do this if there is a color variable to be mapped:
	if(getColorMapEntityIndex() <= 3 ){
		mapColors(container,startTimeStep,minFrame, rParams);
		return;
	}
	//Create lut based on current transferFunction (assumes it doesn't change over time!)
	float* lut = new float[256*4];
	transferFunction->makeLut(lut);
	//Setup color and opacity mappings
	

	float colorMin = transferFunction->getMinMapValue();
	float colorMax = transferFunction->getMaxMapValue();

	float colorVar;
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if(!dataMgr) return;
	//Make sure RGBAs are available:

	container->enableRGBAs();
	double validExts[6];
	
	vector<string> colorVarname;
	colorVarname.push_back(colorMapEntity[getColorMapEntityIndex()]);

	//Determine the first and last time steps in the pathlinedata:
	int firstTS = 10000000, lastTS = -1;
	for (int line = 0; line < container->getNumLines(); line++){
	//Obtain the required grid from the DataMgr.  The LOD of the data may need to be reduced.
		if (container->getFirstTimestep(line) < firstTS) firstTS = container->getFirstTimestep(line);
		if (container->getLastTimestep(line) > lastTS) lastTS = container->getLastTimestep(line);
	}
	if (firstTS > lastTS) return; //nothing to do.
	int firstIndex = container->getIndexFromTimestep(firstTS);
	int lastIndex = container->getIndexFromTimestep(lastTS);

	//Loop over indices,  maintain pointers and timeSteps for current and next data
	int nextDataTime = -1;
	int currDataTime = -1;
	RegularGrid* currData = 0;
	RegularGrid* nextData = 0;
	int numGrids = 0;
		
	for (int indx = firstIndex; indx<=lastIndex; indx++){
		int actualRefLevel = GetRefinementLevel();
		int actualLOD = GetCompressionLevel();
			
		//Determine the current and next sample timesteps for this index:
		float fltTime = container->getTimeInPath(indx);
		//Note:  If fltTime is a valid timestep, these will be equal:
		int currentTimestep = getCurrentTimestepSample(fltTime);
		int nextTimestep = getNextTimestepSample(fltTime);
		// As we move forward in time, either:
		//(1) currData and nextData remain valid, or
		//(2) the nextData becomes currData, and nextData remains nextData, or
		//(3) the nextData becomes currData, and new nextData must be obtained, or
		//(4a) nextData was equal to currData and both advance to equal new values
		//(4b) nextData was equal to currData and both advance to different values
		//(5a) first time thru, currentData == nextData (handled by case 4a)
		//(5b) first time thru, currentData != nextData (handled by case 4b)
		int rc = 0;
		if (nextDataTime == currentTimestep && nextDataTime == nextTimestep) { //case (2)
			//delete currData, set currData = nextData, no allocation needed
			delete currData;
			numGrids--;
			currData = nextData;
			currDataTime = currentTimestep;

		} else if (nextDataTime == currentTimestep && nextDataTime != nextTimestep){//case (3)
			//Delete currData, set currData = nextData
			if (nextData != currData) {
				delete currData;
				numGrids--;
			}
			currData = nextData;
			currDataTime = currentTimestep;
			rParams->GetBox()->GetUserExtents(validExts, (size_t) currentTimestep);
			rc = getGrids((size_t) nextTimestep, colorVarname, validExts, &actualRefLevel, &actualLOD, &nextData);
			if (!rc) {
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"flow color mapping data unavailable at timestep %d\n", currentTimestep);
				setBypass(currentTimestep);
				return;
			}
			numGrids++;
			nextDataTime = nextTimestep;
		} else if (currDataTime == nextDataTime && currentTimestep == nextTimestep && currentTimestep != currDataTime){//case 4a
			//delete currData, allocate one new grid for both currData and nextData
			if (currData) {
				delete currData;
				numGrids--;
			}
			rParams->GetBox()->GetUserExtents(validExts, (size_t) currentTimestep);
			rc = getGrids((size_t) currentTimestep, colorVarname, validExts, &actualRefLevel, &actualLOD, &currData);
			if (!rc) {
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"flow color mapping data unavailable at timestep %d\n", currentTimestep);
				setBypass(currentTimestep);
				return;
			}
			numGrids++;
			currDataTime = currentTimestep;
			nextDataTime = currentTimestep;
			nextData = currData;
		} else if (currDataTime == nextDataTime && currentTimestep != nextTimestep && currentTimestep != currDataTime ) {//case 4b
			//allocate a new currData and a new nextData
			if(currData) {
				delete currData;
				numGrids--;
			}
			rParams->GetBox()->GetUserExtents(validExts, (size_t) currentTimestep);
			rc = getGrids((size_t) currentTimestep, colorVarname, validExts, &actualRefLevel, &actualLOD, &currData);
			if (!rc) {
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"flow color mapping data unavailable at timestep %d\n", currentTimestep);
				setBypass(currentTimestep);
				return;
			}
			numGrids++;
			rParams->GetBox()->GetUserExtents(validExts, (size_t) nextTimestep);
			rc = getGrids((size_t) nextTimestep, colorVarname, validExts, &actualRefLevel, &actualLOD, &nextData);
			if (!rc) {
				SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"flow color mapping data unavailable at timestep %d\n", nextTimestep);
				setBypass(currentTimestep);
				return;
			}
			numGrids++;
			currDataTime = currentTimestep;
			nextDataTime = nextTimestep;
		} else {  //must be case 1
			assert(currentTimestep >= currDataTime && nextTimestep <= nextDataTime);
			//do nothing.
		}
		
		//Now use these grids to color the flow lines.
		//Loop through the lines, interpolate color in HSV between current point and next point.
		float colorRatio = 1.f;
		if (currentTimestep != nextTimestep) 
			colorRatio = (fltTime - (float)currentTimestep)/((float)nextTimestep - (float)currentTimestep);

		for (int lineNum = 0; lineNum<container->getNumLines(); lineNum++){
			float* dataPoint = container->getFlowPoint(lineNum,indx);
			assert(dataPoint[0] != IGNORE_FLAG);
			float colorVar1 = currData->GetValue(dataPoint[0],dataPoint[1],dataPoint[2]);
			float colorVar2 = nextData->GetValue(dataPoint[0],dataPoint[1],dataPoint[2]);
			//If both of these are missing values, we will try to fix it later
			if (colorVar1 == currData->GetMissingValue() && colorVar2 == nextData->GetMissingValue() ) {
				colorVar = colorVar1;
			} else if (colorVar1 == currData->GetMissingValue()) colorVar = colorVar2;
			else if (colorVar2 == nextData->GetMissingValue()) colorVar = colorVar1;
			else //interpolate
				colorVar = colorRatio*colorVar2 + (1.-colorRatio)*colorVar1;

			//Now map the color of the point
			int colorIndex = -1;
			if (colorVar != currData->GetMissingValue()){
				colorIndex = (int)((colorVar - colorMin)*255.99/(colorMax-colorMin));
				if (colorIndex<0) colorIndex = 0;
				if (colorIndex> 255) colorIndex =255;
			}
			
			//Map to color and opacity
			if (colorIndex >= 0){
				container->setRGB(lineNum,indx,lut[4*colorIndex],lut[4*colorIndex+1],lut[4*colorIndex+2]);
				container->setAlpha(lineNum, indx, lut[4*colorIndex+3]);
			}
			else { //use the bottom value, but it will be transparent
				container->setRGB(lineNum,indx,lut[0],lut[1],lut[2]);
				container->setAlpha(lineNum,indx,0.);
			}
		} //End loop over lines 
	} //end loop over indx
	if (currData) {delete currData;numGrids--;}
	if (nextData && nextData != currData) {
		delete nextData;
		numGrids--;
	}
	assert(numGrids == 0);

}

//Map periodic coords into data extents.
//Note this is different than the mapping used by flowlib (at least when numrefinements < max numrefinements)
//If unscale is true then the resulting values are divided by scale factor
//(note that input values are always pre-scaled)
void FlowParams::periodicMap(float origCoords[3], float mappedCoords[3],bool unscale){
	const float* sizes = DataStatus::getInstance()->getFullStretchedSizes();
	const float* scales = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++){
		mappedCoords[i] = origCoords[i];
		if (periodicDim[i]){
			//If it's off, most likely it's only off by one cycle:
			while (mappedCoords[i] < 0.) {mappedCoords[i] += sizes[i];}
			while (mappedCoords[i] > sizes[i]) {mappedCoords[i] -= sizes[i];}
		}
		if (unscale) mappedCoords[i] /= scales[i];
	}
}

int FlowParams::
getColorMapEntityIndex() {
	if (!transferFunction) return 0;
	return transferFunction->getColorVarNum();
}


void FlowParams::
setColorMapEntity( int entityNum){
	if (!transferFunction) return;
	transferFunction->setMinMapValue(minColorBounds[entityNum]);
	transferFunction->setMaxMapValue(maxColorBounds[entityNum]);
	transferFunction->setVarNum(entityNum);
}



//When we set the min/map bounds, must save them locally and in the mapper function
void FlowParams::setMinColorMapBound(float val){
	minColorBounds[getColorMapEntityIndex()] = val;	
	if(GetMapperFunc())GetMapperFunc()->setMinColorMapValue(val);
}
void FlowParams::setMaxColorMapBound(float val){
	maxColorBounds[getColorMapEntityIndex()] = val;
	if(GetMapperFunc())GetMapperFunc()->setMaxColorMapValue(val);
}
void FlowParams::setMinOpacMapBound(float val){
	minColorBounds[getColorMapEntityIndex()] = val;
	if(GetMapperFunc()){
		GetMapperFunc()->setMinColorMapValue(val);
		GetMapperFunc()->setMinOpacMapValue(val);
	}
}
void FlowParams::setMaxOpacMapBound(float val){
	maxColorBounds[getColorMapEntityIndex()] = val;
	if(GetMapperFunc()){
		GetMapperFunc()->setMaxColorMapValue(val);
		GetMapperFunc()->setMaxOpacMapValue(val);
	}
}

//Determine the max and min (likely range) associated with a mapped index:
float FlowParams::minRange(int index, int timestep){
	switch(index){
		case (0): return 0.f;
		case (1): //
			//Need to fix this for unsteady
			if (flowType != 1){
				return 0.f;
			}
			else return ((float)DataStatus::getInstance()->getMinTimestep());
		case (2): return (0.f);// minimum speed
		case (3): //seed index.  Goes from 0 to num seedsp
			return (0.f );
		default:
			int varnum = DataStatus::mapActiveToSessionVarNum3D(index -4);
			if (DataStatus::getInstance()&& DataStatus::getInstance()->variableIsPresent3D(varnum)){
				return( DataStatus::getInstance()->getDataMin3D(varnum,timestep));
			}
			else return 0.f;
	}
}
float FlowParams::maxRange(int index, int timestep){
	float maxSpeed = 0.f;
	
	switch(index){
		case (0): return 1.f;
		case (1): // length along line
			if (flowType != 1){
				return 1.f;
			}
			//unsteady:
			return (float)maxFrame;
		case (2): //speed
			for (int k = 0; k<3; k++){
				int var;
				if (flowIsSteady()) var = steadyVarNum[k];
				else var = unsteadyVarNum[k];
				if (var == 0) continue;
				if (maxSpeed < fabs(DataStatus::getInstance()->getDefaultDataMax3D(var-1)))
					maxSpeed = fabs(DataStatus::getInstance()->getDefaultDataMax3D(var-1));
				if (maxSpeed < fabs(DataStatus::getInstance()->getDefaultDataMin3D(var-1)))
					maxSpeed = fabs(DataStatus::getInstance()->getDefaultDataMin3D(var-1));
			}
			return maxSpeed;
		case (3): // seed Index, from 0 to numseeds-1
			
			return (doRake ? (float)getNumRakeSeedPoints()-1 :(float)(getNumListSeedPoints()-1));
		default:
			int varnum = DataStatus::mapActiveToSessionVarNum3D(index -4);
			if (DataStatus::getInstance()&& DataStatus::getInstance()->variableIsPresent3D(varnum)){
				return( DataStatus::getInstance()->getDataMax3D(varnum,timestep));
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
//Returns false if the sampling is changed.
//OK if varnum < 0.
bool FlowParams::
validateSampling(int minFrame, int numRefs, const int varnums[3])
{
	//Don't do anything if no data has been read:
	if (!DataStatus::getInstance()->getDataMgr()) return true;
	
	if (useTimestepSampleList){
		//Just check all the sample times, see if the field exists for each step:
		for (int i = 0; i< unsteadyTimestepList.size(); i++){
			int ts = unsteadyTimestepList[i];
			if (!validateVectorField(ts,numRefs,varnums)){
				MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"Invalid timestep sample list:\n %s",
					"Unsteady field not available at requested \nrefinement at all time steps in list.");
				return true;
			}
		}
		return true;
	}

	bool changed = false;
	if (timeSamplingStart < minFrame || timeSamplingStart > maxFrame){
		timeSamplingStart = minFrame;
		changed = true;
	}
	//See if start is valid; if not set to first valid start:
	int ts;
	for (ts = timeSamplingStart; ts <= maxFrame; ts++) {
		if (validateVectorField(ts, numRefs, varnums)) break;
	}
	//Now ts is the first valid frame.
	//do there exist valid frames?
	if (ts > maxFrame) {
		timeSamplingStart = minFrame;
		timeSamplingEnd = minFrame;
		timeSamplingInterval = 1;
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"Flow vector field is not available\nat required refinement/lod\nat any sampled time steps");
		return false;
	}
	//was the proposed start invalid?
	if (ts > timeSamplingStart) {
		timeSamplingStart = ts;
		changed = true;
	}
	//If the proposed sampling is just one frame, it's only ok for steady flow
	if (timeSamplingEnd < timeSamplingStart+timeSamplingInterval) {
		if (timeSamplingEnd < timeSamplingStart) {
			timeSamplingEnd = timeSamplingStart;
			changed = true;
		}
		//if(changed) MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"time sampling specification was modified");
		return !changed;
	}
	//Now we have a valid start, see if the increment is invalid.
	if (timeSamplingInterval < 1) {
		timeSamplingInterval = 1;
		changed = true;
	}
	//It is OK if the second one is valid or if there is no second one:
	if (!validateVectorField(timeSamplingStart+timeSamplingInterval, numRefs, varnums)){
		//Need to find the first valid increment:
		for (ts = timeSamplingStart+1; ts <= maxFrame; ts++){
			if (validateVectorField(ts, numRefs, varnums)) break;
		}
		//Is there a valid second timestep?
		if (ts > maxFrame){
			timeSamplingInterval = 1;
			timeSamplingEnd = timeSamplingStart;
			//MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"time sampling specification was modified");
			return false;
		}
		//Use the second valid step to define interval:
		timeSamplingInterval = ts - timeSamplingStart;
		changed = true;
	}
	//Now establish the last frame
	for (ts = timeSamplingStart; ts <= timeSamplingEnd; ts+= timeSamplingInterval){
		if (!validateVectorField(ts, numRefs, varnums)) break;
	}
	if (ts > timeSamplingEnd){
		//if(changed) MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"time sampling specification changed");
		return (!changed);
	} else {
		timeSamplingEnd = ts - timeSamplingInterval;
		//MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"time sampling specification changed");
		return false;
	}
}

//Check to see if there is valid field data for specified timestep.
//Return true if its ok
//Ignores field variables that are < 0
//
 
bool FlowParams::
validateVectorField(int ts, int refLevel, const int varNums[3]) {
	DataStatus* dStatus = DataStatus::getInstance();
	if (!dStatus) return false;
	int minLOD = GetCompressionLevel();
	if (dStatus->useLowerAccuracy()) {refLevel = 0; minLOD = 0;}
	
	for (int i = 0; i< 3; i++){
		if (varNums[i] < 0) continue;
		if(dStatus->maxXFormPresent3D(varNums[i], ts)< refLevel)
			return false;
		if(dStatus->maxLODPresent3D(varNums[i], ts)< minLOD)
			return false;
	}
	return true;
}

int FlowParams::calcMaxPoints(){
	
	if (flowType != 1) { //steady or flow line advection
		if (autoScale) {
			objectsPerFlowline = (int)(steadyFlowLength*steadySmoothness);
		}
		maxPoints = objectsPerFlowline;
		if (maxPoints < 2) maxPoints = 2;
		if (steadyFlowDirection == 0 && maxPoints < 3) maxPoints = 3;
	} else {
		
		//For unsteady flow, use the time steps in the DataStatus.
		maxPoints = (objectsPerTimestep*(1+DataStatus::getInstance()->getMaxTimestep()-DataStatus::getInstance()->getMinTimestep()));
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
//used to automatically set vector scale factor.
float FlowParams::getAvgVectorMag(RegionParams* rParams, int numrefts, int timeStep){
	
	const RegularGrid* varData[3];
	size_t min_dim[3],max_dim[3];
	int varnums[3];
	int varcount = 0;
	int lod = GetCompressionLevel();
	for (int i = 0; i< 3; i++){if (steadyVarNum[i]) varnums[varcount++] = steadyVarNum[i] -1;}
	int availRefLevel =  rParams->getAvailableVoxelCoords(numrefts, lod, min_dim, max_dim, (size_t)timeStep, varnums, varcount);
	if (availRefLevel < 0) {
		return -1.f;
	}
	
	DataStatus* ds = DataStatus::getInstance();
	
	DataMgr* dataMgr = (DataMgr*)(ds->getDataMgr());
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	//Obtain the variables from the dataMgr:
	for (int var = 0; var<3; var++){
		if (steadyVarNum[var]== 0) varData[var] = 0;
		else {
			int lod = GetCompressionLevel();
			if (ds->useLowerAccuracy())
				lod = Min(lod, ds->maxLODPresent3D(steadyVarNum[var]-1, timeStep));
			varData[var] = dataMgr->GetGrid((size_t)timeStep,
				ds->getVariableName3D(steadyVarNum[var]-1).c_str(),
				availRefLevel, lod, min_dim, max_dim,  1);
			if (!varData[var]) {
				dataMgr->SetErrCode(0);
				ds->setDataMissing3D(timeStep, availRefLevel,lod, steadyVarNum[var]-1);
				//release currently locked regions:
				for (int k = 0; k<var; k++){
					if (varData[k])
						dataMgr->UnlockGrid(varData[k]);
				}
				QApplication::restoreOverrideCursor();
				return -2.f;
			}	
		}
	}
	
	int numPts = 0;
	float dataSum = 0.f;
	//push down the zero grids.
	for (int i = 0; i<2; i++){
		if (!varData[0]){
			varData[0] = varData[1];
			varData[1] = varData[2];
			varData[2] = 0;
		}
		if (!varData[1]){
			varData[1] = varData[2];
			varData[2] = 0;
		}
	}
	if (!varData[0]) return -1.f;  //No grids
		
	RegularGrid::ConstIterator itr0, itr1, itr2;
	itr0 = varData[0]->begin();
	
	float mv = varData[0]->GetMissingValue();
	if (varData[2])itr2 = varData[2]->begin(); 
	if (varData[1])itr1 = varData[1]->begin();
	for (itr0 = varData[0]->begin(); itr0 != varData[0]->end(); ++itr0){
		float val[3] = { 0.f, 0.f, 0.f};
		val[0] = *itr0;
		if(varData[1]) val[1] = *itr1;
		if(varData[2]) val[2] = *itr2;
		//Ignore if first variable is mv
		if (val[0] != mv) {
			//Note:  we assume that all variables have missing values in same place
			assert (val[1] != mv && val[2] != mv);
			float mag = sqrt(val[0]*val[0]+val[1]*val[1]+val[2]*val[2]);
			dataSum += mag;
			numPts++;
		}
		if (varData[1]) ++itr1;
		if (varData[2]) ++itr2;
	}
		
	for (int k = 0; k<3; k++){
		if(varData[k]) dataMgr->UnlockGrid(varData[k]);
	}
	QApplication::restoreOverrideCursor();
	if (numPts == 0) return -1.f;
	return (dataSum/(float)numPts);
	
}
//Tell the flowLib about the current region, return false on error
//timeStep argument is only used for steady flow.
bool FlowParams::
setupFlowRegion(RegionParams* rParams, VaporFlow* flowLib, int timeStep){
	size_t min_dim[3], max_dim[3]; 
	DataStatus* ds = DataStatus::getInstance();
	flowLib->SetPeriodicDimensions(periodicDim[0],periodicDim[1],periodicDim[2]);
	
	//For steady flow, determine what is the available region for the current time step.
	//For other flow, determine the available region for all the sampled timesteps.
	int availRefLevel = numRefinements;
	int lod = GetCompressionLevel();
	if (flowType == 0 ){
		int varnums[3];
		int varcount = 0;
		
		for (int i = 0; i< 3; i++){if (steadyVarNum[i]) varnums[varcount++] = steadyVarNum[i] -1;}
		availRefLevel = rParams->getAvailableVoxelCoords(numRefinements, lod, min_dim, max_dim,  
			timeStep, varnums,varcount);
		
		if (ds->useLowerAccuracy()){
			for (int i = 0; i<3; i++){
				if(steadyVarNum[i]>0) lod = Min(lod, ds->maxLODPresent3D(steadyVarNum[i]-1, timeStep));
			}
		}
		if(availRefLevel < 0 || lod < 0){
			return false;
		}
	} else { 
		//Unsteady flow.  Find the intersection of all the regions for the timesteps to be sampled.
		// Also get available refinement/compression level which is min of all ones available
		
		size_t gmin_dim[3],gmax_dim[3];
		//Get min refinement level that's OK for all time steps for which there is data.
		//We will skip timesteps with no data
		if (ds->useLowerAccuracy()) {
			for (int indx = 0; indx < getNumTimestepSamples(); indx++){
				int ts = getTimestepSample(indx);
				int levelAtTime = availRefLevel;
				int lodAtTime = lod;
				for (int i = 0; i< 3; i++){
					if (unsteadyVarNum[i]){
						levelAtTime = Min(levelAtTime, ds->maxXFormPresent3D(unsteadyVarNum[i]-1, ts));
						lodAtTime = Min(lodAtTime, ds->maxLODPresent3D(unsteadyVarNum[i]-1, ts));
					}
				}
				if (levelAtTime >= 0) availRefLevel = levelAtTime;
				if (lodAtTime >= 0) lod = lodAtTime;
			}
		}
		//Start with the extents at the start timestep:
		int ts = getTimestepSample(0);
		rParams->getRegionVoxelCoords(availRefLevel, GetCompressionLevel(),gmin_dim, gmax_dim, ts);
		
		//Intersect the regions for the timesteps with field data:
		for (int indx = 0; indx < getNumTimestepSamples(); indx++){
			int ts = getTimestepSample(indx);
			bool tsIsOK = true;
			for (int i = 0; i < 3; i++){
				if (unsteadyVarNum[i] == 0) continue;
				if(ds->maxXFormPresent3D(unsteadyVarNum[i]-1, ts) < 0) {tsIsOK = false;break;}
				if(!ds->useLowerAccuracy()){
					if ((ds->maxXFormPresent3D(unsteadyVarNum[i]-1, ts)) < availRefLevel){
						tsIsOK = false; break;
					}
					if ((ds->maxLODPresent3D(unsteadyVarNum[i]-1, ts)) < lod){
						tsIsOK = false; break;
					}
				}
			}
			if (!tsIsOK) continue;
			int varnums[3];
			int varcount = 0;
			for (int i = 0; i< 3; i++){varnums[varcount++] = unsteadyVarNum[i] -1;}
			int refLevel = rParams->getAvailableVoxelCoords(availRefLevel, lod, min_dim, max_dim, 
				ts, varnums, varcount);
			if(refLevel < 0)
				return false;
			assert(refLevel == availRefLevel);
			
			//Intersect the available region bounds.
			for (int i = 0; i< 3; i++){
				gmin_dim[i] = Max(gmin_dim[i],min_dim[i]);
				gmax_dim[i] = Min(gmax_dim[i],max_dim[i]);
				
			}
		}
		//Copy back the intersected region:
		for (int i = 0; i< 3; i++){
			min_dim[i] = gmin_dim[i];
			max_dim[i] = gmax_dim[i];
		}
	}
	
	//Check that the cache is big enough for all three (or 4) variables
	float numVoxels = (max_dim[0]-min_dim[0]+1)*(max_dim[1]-min_dim[1]+1)*(max_dim[2]-min_dim[2]+1);
	float nvars =  3.f;
	if (flowType != 0) nvars = 6.f;
	
	//Multiply by  *4 to get total bytes, divide by 1M mbytes, * num variables
	float memreq = nvars*numVoxels/250000.;
	if (memreq >= (float)DataStatus::getInstance()->getCacheMB()){
		SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG," Data cache size %d is\ntoo small for this flow integration",
			DataStatus::getInstance()->getCacheMB());
		return false;
	}
	//At this point min_dim and max_dim are the valid voxel extents of region that contains the valid float extents where
	//the flow lines will be calculated.  These may properly contain the user extents of the region,
	//however the integration will stop when a flow line gets to the user extents.  
	double rexts[6];
	rParams->getLocalRegionExtents(rexts,timeStep);
	
	flowLib->SetLocalRegion(availRefLevel, lod, min_dim, max_dim, rexts);
	// specify the bounds of the rake, in case it is needed.  
	
	double rakeCoords[6];
	GetBox()->GetLocalExtents(rakeCoords);
	
	flowLib->SetLocalRakeRegion(rakeCoords);
	return true;
}
//Perform all the major steps in field line advection where the prioritization is
//done at the "next" time step.  Results are placed in the steadyFlowCache.
//If steadyFlowCache[startTime] does not exists, it is created, and populated with
//seed points, and these are advected to steady flow lines.
//Then, create a new, default, steadyFlowCache for all intermediate times.
//Then the following is repeated for each valid seed (seedNum) at startTime:
//	 Call resampleFieldLines, obtaining indices to a set of points along the
//		flow line associated with seedNum
//   Call AdvectFieldLines(seedNum).  This will advect all those points in the
//		unsteady field, inserting the results in subsequent steadyFlowCache's in the
//		positions associated with the pointIndex and seedNum.
//	 Find the maximal point that survived the advection, using VaporFlow::prioritizeSeeds,
//		inserting it in the last position.
//	 Perform steadyFlowAdvection on the last time step
bool FlowParams::multiAdvectFieldLines(VaporFlow* myFlowLib, FlowLineData** steadyFlowCache, int startTime, int endTime, int minFrame, RegionParams* rParams){
	
	//Get some initial settings:
	//One-line Fix for bug 1680062 in 1.1.0:
	int numSeedPoints = calcNumSeedPoints(seedTimeStart);
	maxPoints = calcMaxPoints();
	bool useSpeeds =  (getColorMapEntityIndex() == 2);
	bool doRGBAs = (getColorMapEntityIndex() > 0);

	//  Previous cache should already exist:
	assert(steadyFlowCache[startTime]);
	
	assert (startTime != endTime);
	//Create new flow line data for all other times:
	int timeDir = (startTime < endTime) ? 1 : -1 ;
	int forwardSamples = getNumFLASamples();
	if (forwardSamples < 2) forwardSamples = 2;
	if (forwardSamples > steadyFlowCache[startTime]->getMaxPoints())
		forwardSamples = steadyFlowCache[startTime]->getMaxPoints();
	for (int i = startTime + timeDir;; i+= timeDir){
		if (steadyFlowCache[i]) delete steadyFlowCache[i];
		//Each intermediate cache will not use speeds, and will go forward.
		if (i != endTime)
			steadyFlowCache[i] = new FlowLineData(numSeedPoints, maxPoints, false, 1, doRGBAs); 
		else {
			//The last one leaves space for speeds if needed.
			//It will later be modified to get the same direction as the starting line.
			steadyFlowCache[i] = new FlowLineData(numSeedPoints, maxPoints, useSpeeds, 1, doRGBAs); 
		}
		//Fill the steadyFlow cache with END_FLOW_FLAG's:
		for (int line = 0; line < numSeedPoints; line++){
			for (int j = 0; j < forwardSamples; j++){
				steadyFlowCache[i]->setFlowPoint(line, j, END_FLOW_FLAG,END_FLOW_FLAG,END_FLOW_FLAG);
			}
		}
		if(i == endTime) break;
	}
	
	if (!myFlowLib->AdvectFieldLines(steadyFlowCache,startTime,endTime,forwardSamples)){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"No flow lines were advected\nto time step %d", endTime);
		return false;
	}
	//For each line, put the highest priority seed at the start position at endTime
	if(!myFlowLib->prioritizeSeeds(steadyFlowCache[endTime],0,endTime)){
		MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"Unable to prioritize seeds");
	}
	//If there are intermediate field lines, realign them to start at valid points.
	for (int i = startTime+1; i< endTime; i++){
		steadyFlowCache[i]->realignFlowLines();
	} 
	//See how many seeds have exited the region.
	int prevSeeds = 0;
	int postSeeds = 0;
	for (int i = 0; i<numSeedPoints; i++){
		if (steadyFlowCache[startTime]->getFlowStart(i)[0] != END_FLOW_FLAG) prevSeeds++;
		if (steadyFlowCache[endTime]->getFlowStart(i)[0] != END_FLOW_FLAG) postSeeds++;
	}
	//if (postSeeds < prevSeeds){
	//	MyBase::SetErrMsg(VAPOR_WARNING_FLOW, "%d field lines remain in region at time %d",
	//		postSeeds, endTime);
	//}
	//Then do steady advection at the end time:
	regenerateSteadyFieldLines(myFlowLib, steadyFlowCache[endTime], 0, endTime, minFrame, rParams, false);
	
	return true;
}
bool FlowParams::
singleAdvectFieldLines(VaporFlow* myFlowLib, FlowLineData** steadyFlowCache, PathLineData* unsteadyFlowCache, int prevStep, int nextStep, int minFrame, RegionParams* rParams){
	
	assert(steadyFlowCache[prevStep]);
	
	//we need to build nextStep:
	assert(!steadyFlowCache[nextStep]);
	//To do the next step, need first to advect from prevStep:
	//extend the pathline from prevstep to nextstep, prioritizing first:
					
	if(!myFlowLib->ExtendPathLines(unsteadyFlowCache, prevStep, nextStep, true))
		return false;

	steadyFlowCache[nextStep] = regenerateSteadyFieldLines(myFlowLib, 0, unsteadyFlowCache, nextStep, minFrame, rParams, true);
	if(!steadyFlowCache[nextStep]){
		MyBase::SetErrMsg(VAPOR_ERROR_INTEGRATION,"Unable to perform steady integration\nat timestep %d", prevStep);
		return false;
	}
	return true;
}


bool FlowParams::validateSettings(int tstep){
	DataStatus* ds = DataStatus::getInstance();
	if (!DataStatus::getInstance()->getDataMgr()) return false;
	const vector<double>& usrExts = ds->getDataMgr()->GetExtents(tstep);
	//If we are using a rake, force it to fit inside the current data extents
	if (doRake){
		double rakeBox[6];
		GetBox()->GetLocalExtents(rakeBox);
		float shrExts[6];
	
		//Shrink exts slightly, then subtract to put into local extents
		for (int i = 0; i< 3; i++){
			float mid = (usrExts[i]+usrExts[i+3])*0.5;
			shrExts[i] = 0.999999*usrExts[i] + 0.000001*mid - usrExts[i];
			shrExts[i+3] = 0.999999*usrExts[i+3] + 0.000001*mid- usrExts[i];
			
		}

		for (int i = 0; i<3; i++){
			if(rakeBox[i] < shrExts[i]) rakeBox[i] = shrExts[i];
			if(rakeBox[i+3] < shrExts[i]) rakeBox[i+3] = shrExts[i];
			if(rakeBox[i+3] > shrExts[i+3]) rakeBox[i+3] = shrExts[i+3];
			if(rakeBox[i] > shrExts[i+3]) rakeBox[i] = shrExts[i+3];
			if(rakeBox[i+3] < rakeBox[i]){
				rakeBox[i] = shrExts[i];
				rakeBox[i+3] = shrExts[i+3];
			}
		}
		GetBox()->SetLocalExtents(rakeBox);
	}
	//See if the steady field is OK (type 0 and 2)
		// for type 0, needed for tstep
		// for type 2, needed for all sample times 
	
	switch (flowType) {
		case (0) : 
			if (steadyVarNum[0] == 0 && steadyVarNum[1] == 0 && steadyVarNum[2] == 0) return false;
			if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),tstep, 
						steadyVarNum[0]-1,steadyVarNum[1]-1, steadyVarNum[2]-1)){
				
				if (ds->warnIfDataMissing()){
					autoRefresh = false;
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
						"Steady field not available at required resolution\nfor timestep %d\n%s",
						tstep, "Auto refresh has been disabled\n to facilitate corrective action");
				}
				return false;
			}
			break;
		case (1) :
			break;
		case (2) :
			if (steadyVarNum[0] == 0 && steadyVarNum[1] == 0 && steadyVarNum[2] == 0) return false;
			for (int i = 0; i<getNumTimestepSamples(); i++){
				int ts = getTimestepSample(i);
				if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),ts, 
						steadyVarNum[0]-1,steadyVarNum[1]-1, steadyVarNum[2]-1)){
					if (ds->warnIfDataMissing()){
						autoRefresh = false;
						MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
							"Steady field not available at required\nresolution for timestep %d\n%s",
							ts, "Auto refresh has been disabled\n to facilitate corrective action");
					}
					
					break;
				}
			}
			break;
	}//end switch

	//See if the unsteady field is OK  (type 1 and 2)
	// Need at least two legitimate time steps
	if (flowType != 0) {
		if (unsteadyVarNum[0] == 0 && unsteadyVarNum[1] == 0 && unsteadyVarNum[2] == 0) return false;
		int numLegitimate = 0;
		for (int i = 0; i<getNumTimestepSamples(); i++){
			int ts = getTimestepSample(i);
			if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),ts, 
					unsteadyVarNum[0]-1,unsteadyVarNum[1]-1, unsteadyVarNum[2]-1)) continue;
			if (flowType == 2 && !ds->fieldDataOK(numRefinements, GetCompressionLevel(),ts, 
					steadyVarNum[0]-1,steadyVarNum[1]-1, steadyVarNum[2]-1)) continue;
			numLegitimate++;
		}

		if (numLegitimate <= 1){
			autoRefresh = false;
			MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
				"At least two time step samples at \nspecified refinement are necessary \nfor unsteady flow integration.\n%s",
				"Auto refresh has been disabled\nto facilitate corrective action");
				
				return false;
		}
		//If warnings are on, make sure the unsteady flow exists at every timestep:
		if (ds->warnIfDataMissing()){
			for (int i = 0; i<getNumTimestepSamples(); i++){
				int ts = getTimestepSample(i);
				if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),ts, 
						unsteadyVarNum[0]-1,unsteadyVarNum[1]-1, unsteadyVarNum[2]-1)){
					autoRefresh = false;
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
							"Unsteady field not available at required\nresolution for timestep %d\n%s",
							ts, "Auto refresh has been disabled\nto facilitate corrective action");
					
					return false;
				}
			}
		}
	}
		
	//See if the seed prioritization field is OK, provided using random
	//rake and bias != 0.    
		//for type 0 needed for tstep
		//for type 1 needed for all seed times between first and last sample time
		//for type 2 needed for seed time start
	
	if (seedDistBias != 0.f && doRake && randomGen && ds->warnIfDataMissing()){
		switch (flowType) {
			case (0) :
				if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),tstep, 
						seedDistVarNum[0],seedDistVarNum[1], seedDistVarNum[2])){
					autoRefresh = false;
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
							"Seed distribution field not available at\nrequired resolution for timestep %d\n%s",
							tstep, "Auto refresh has been disabled\nto facilitate corrective action");
					
					return false;
				}
				break;
			case(1) :
				for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
					if (i >= getFirstSampleTimestep() && i <= getLastSampleTimestep()) {
						if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),i, 
								seedDistVarNum[0],seedDistVarNum[1], seedDistVarNum[2])){
							autoRefresh = false;
							MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
									"Seed distribution field not available at\nrequired resolution for timestep %d\n%s",
									i, "Auto refresh has been disabled\nto enable corrective action");
							
							return false;
						}
					}
				}
				break;
			case(2) :
				{ 
					if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),seedTimeStart, 
							seedDistVarNum[0],seedDistVarNum[1], seedDistVarNum[2])){
						autoRefresh = false;
						MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
								"Seed distribution field not available at\nrequired resolution for timestep %d\n%s",
								seedTimeStart, "Auto refresh has been disabled\nto facilitate corrective action");
						
						return false;
					}
				}
				break;
		}//end switch
	}

	//see if the fla prioritization field is OK (type 2)
	// needed for all sample timesteps
	if (flowType == 2 && ds->warnIfDataMissing()) {
		for (int i = 0; i<getNumTimestepSamples(); i++){
			int ts = getTimestepSample(i);
			if (!ds->fieldDataOK(numRefinements, GetCompressionLevel(),ts, 
					priorityVarNum[0],priorityVarNum[1], priorityVarNum[2])){
				autoRefresh = false;
				MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
						"Prioritization field not available at\nrequired resolution for timestep %d\n%s",
						ts, "Auto refresh has been disabled\nto facilitate corrective action");
				
				return false;
			}
		}
	}
	// See if samples and seeds are consistent:
	// for type 0, need both samples and seeds at tstep
	switch (flowType){
		case (0) : 
			//Is tstep a seed time?  
			if (!doRake) {
				bool found = false;
				for (int i = 0; i< seedPointList.size(); i++){
					if (seedPointList[i].getVal(3) < 0.f || tstep == (int)(seedPointList[i].getVal(3)+0.5f)) 
					{
						found = true;
						break;
					}
				}
				if (!found && seedPointList.size()>0 ) {
					if(refreshIsAuto())setBypass(tstep);
					MyBase::SetErrMsg(VAPOR_WARNING_FLOW,
						"No seed points specified at current timestep\n");
					return false;
				}
			}
			break;
		// for type 1, need a seed at or after first sample timestep
		//	plus need tstep to be after first sample and after first seed
		// If rake is used, seed times are determined by start, end, increment.
		// If seed list is used seed times are from seed
		case (1) :
			{
				int ts;
				bool found = false;
				
				if (unsteadyFlowDirection > 0) {
					ts = getFirstSampleTimestep();
					for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
						//see if there is a seed after time ts:
						if (ts > i) continue;
						if (doRake) {found = true; break;}
						//See if there is a seed in the list at this time step:
						for (int j = 0; j<seedPointList.size(); j++){
							if (seedPointList[j].getVal(3) < 0.f || i == (int)(seedPointList[j].getVal(3)+0.5f)){
								found = true;
								break;
							}
						}
						if (found) break;
					}
				} else { //go backwards in time:
					ts = getLastSampleTimestep();
					for (int i = seedTimeStart; i<= seedTimeEnd; i+= seedTimeIncrement){
						//see if there is a seed before time ts:
						if (ts < i) continue;
						if (doRake) {found = true; break;}
						//See if there is a seed in the list at this time step:
						for (int j = 0; j<seedPointList.size(); j++){
							if (seedPointList[j].getVal(3) < 0.f || i == (int)(seedPointList[j].getVal(3)+0.5f)){
								found = true;
								break;
							}
						}
						if (found) break;
					}
				}
				if (!found && seedPointList.size()> 0) {
					autoRefresh = false;
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
							"No seeds available after the first sample\ntime %d of the unsteady flow\n%s",
							ts,"Auto refresh has been disabled\nto facilitate corrective action");
						
						return false;
				}
			}
			break;
		// for type 2, need seed time start to be a sample time, and for there to
		//		be a seed at that time.
		//  Also: need there to be a sample time at tstep, or on the other side of
		//	tstep from seedTimeStart.
		case (2) : 
			{
				bool OK = false;
				//See if there are any seeds at seed time start:
				if (!doRake) {
					//See if there is a seed in the list at start time step:
					for (int j = 0; j<seedPointList.size(); j++){
						if ((seedPointList[j].getVal(3) < 0.f) || (seedTimeStart == (int)(seedPointList[j].getVal(3)+0.5f))){
							OK = true;
							break;
						}
					}
					if (!OK && seedPointList.size() > 0) {
						autoRefresh = false;
						MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
								"No seeds available at start seed time %d \n%s",
								seedTimeStart,
								"Auto refresh has been disabled\nto facilitate corrective action");
						
						return false;
					}
				}
				//See if the seedTimeStart is a sample time
				OK = false;
				for (int i = 0; i<getNumTimestepSamples(); i++){
					if (getTimestepSample(i) == seedTimeStart){
						OK = true;
						break;
					}
				}
				if (!OK) {
					autoRefresh = false;
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
							"start seed time %d is not a sample time\n%s",
							seedTimeStart,
							"Auto refresh has been disabled\nto enable corrective action");
					
					return false;
				}

				//Verify that there's a sample time on the 'other' side of current time:
				OK = false;
				for (int i = 0; i<getNumTimestepSamples(); i++){
					if (tstep <= seedTimeStart &&
						getTimestepSample(i) <= tstep) OK = true;
					if (tstep >= seedTimeStart &&
						getTimestepSample(i) >= tstep) OK = true;
					if (OK) break;
				}
				if (!OK){
					autoRefresh = false;
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW,
						"Cannot perform field line advection\nfrom the seed time %d to current time %d\n%s\n%s",
						seedTimeStart, tstep,
						"Because the current time is not\nbetween the seed time and a sample time",
						"Auto refresh has been disabled\nto facilitate corrective action");
					return false;
				}

			}
			break;
		}   //End of switch
	return true;
}
int FlowParams::getCurrentTimestepSample(float ts){
	if (usingTimestepSampleList()){
		int prev = unsteadyTimestepList[0];
		for (int i = 1; i< unsteadyTimestepList.size(); i++){
			if (unsteadyTimestepList[i] <= ts){ prev = i; continue;}
		}
		return prev;
	}
	//find largest integer n such that (timeSamplingStart+n*timeSamplingInterval <= ts)
	int n = (int)(0.5+ (ts - timeSamplingStart)/(float)timeSamplingInterval);
	if (timeSamplingStart+n*timeSamplingInterval > ts + .001f) n--; //n is too big
	return (timeSamplingStart+n*timeSamplingInterval);
		
}
int FlowParams::getNextTimestepSample(float ts){
	if (usingTimestepSampleList()){
		int next = unsteadyTimestepList[unsteadyTimestepList.size()-1];
		for (int i = unsteadyTimestepList.size()-1; i>= 0; i--){
			if (unsteadyTimestepList[i] >= ts){ next = i; continue;}
		}
		return next;
	}
	//find smallest integer n such that (timeSamplingStart+n*timeSamplingInterval >= ts)
	int n = (int)(0.5+ (ts - timeSamplingStart)/(float)timeSamplingInterval);
	if (timeSamplingStart+n*timeSamplingInterval < ts - .001f) n++; //n is too small
	return (timeSamplingStart+n*timeSamplingInterval);
}
//Hook up the new transfer function in specified slot,
//Delete the old one.  This is called whenever a new tf is loaded.
//
void FlowParams::
hookupTF(TransferFunction* tf, int index){

	//Create a new TFEditor
	if (transferFunction) delete transferFunction;
	transferFunction = tf;

	minColorEditBounds[index] = tf->getMinMapValue();
	maxColorEditBounds[index] = tf->getMaxMapValue();
	tf->setParams(this);
	tf->setVarNum(index);
	
}
int FlowParams::getSessionVarNum() { 
	int varIndex = (getColorMapEntityIndex()-4);
	if (varIndex < 0) return varIndex;
	return(DataStatus::getInstance()->mapActiveToSessionVarNum3D(varIndex));
}
const float* FlowParams::getCurrentDatarange(){
	if (!transferFunction) {
		dataRange[0]=0.;
		dataRange[1]=1.;
	} else {
		dataRange[0] = transferFunction->getMinMapValue();
		dataRange[1] = transferFunction->getMaxMapValue();
	}
	return dataRange;
}
