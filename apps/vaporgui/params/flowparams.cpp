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
#include "flowtab.h"
#include "flowrenderer.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "vapor/VaporFlow.h"
#include "glutil.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include "mainform.h"
#include "session.h"
#include "params.h"
#include "panelcommand.h"
#include "tabmanager.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
#include "mapperfunction.h"
#include "mapeditor.h"
#include "flowmapeditor.h"
#include "flowmapframe.h"
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

	const string FlowParams::_mappedVariablesAttr = "MappedVariables";
	const string FlowParams::_steadyFlowAttr = "SteadyFlow";
	const string FlowParams::_instanceAttr = "FlowRendererInstance";
	const string FlowParams::_numTransformsAttr = "NumTransforms";
	const string FlowParams::_integrationAccuracyAttr = "IntegrationAccuracy";
	const string FlowParams::_velocityScaleAttr = "velocityScale";
	const string FlowParams::_timeSamplingIntervalAttr = "TimeSamplingInterval";
	
	//Geometry variables:
	const string FlowParams::_geometryTag = "FlowGeometry";
	const string FlowParams::_geometryTypeAttr = "GeometryType";
	const string FlowParams::_objectsPerTimestepAttr = "ObjectsPerTimestep";
	const string FlowParams::_displayIntervalAttr = "DisplayInterval";
	const string FlowParams::_shapeDiameterAttr = "ShapeDiameter";
	const string FlowParams::_colorMappedEntityAttr = "ColorMappedEntity";
	const string FlowParams::_colorMappingBoundsAttr = "ColorMappingBounds";
	const string FlowParams::_hsvAttr = "HSV";
	const string FlowParams::_positionAttr = "Position";
	const string FlowParams::_colorControlPointTag = "ColorControlPoint";
	const string FlowParams::_numControlPointsAttr = "NumColorControlPoints";

FlowParams::FlowParams(int winnum) : Params(winnum) {
	thisParamType = FlowParamsType;
	myFlowTab = MainForm::getInstance()->getFlowTab();
	enabled = false;
	selectedFaceNum = -1;
	faceDisplacement = 0.f;
	
	//Set default values
	flowType = 0; //steady
	instance = 1;
	numTransforms = 4; 
	maxNumTrans = 4; 
	minNumTrans = 0;
	numVariables = 0;
	firstDisplayFrame = 0;
	lastDisplayFrame = 20;
	variableNames.empty();
	//Initially just make one blank:
	variableNames.push_back(" ");
	varNum[0]= 0;
	varNum[1] = 1;
	varNum[2] = 2;
	integrationAccuracy = 0.5f;
	velocityScale = 1.0f;
	constantColor = qRgb(255,0,0);
	constantOpacity = 1.f;
	timeSamplingInterval = 1;
	minFrame = maxFrame = 1;
	editMode = true;
	savedCommand = 0;

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
	objectsPerTimestep = 1.f;

	shapeDiameter = 1.f;
	

	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Age");
	colorMapEntity.push_back("Speed");
	opacMapEntity.clear();
	
	opacMapEntity.push_back("Constant");
	opacMapEntity.push_back("Age");
	opacMapEntity.push_back("Speed");
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
	myFlowLib = 0;
	mapperFunction = 0;
	flowMapEditor = 0;
	//Set up flow data cache:
	flowData = 0;
	flowRGBAs = 0;
	
	
	numSeedPoints = 1;
	numInjections = 1;
	maxPoints = 0;
	numControlPoints = 0;  //obsolete

	minOpacBounds = new float[3];
	maxOpacBounds= new float[3];
	minColorBounds= new float[3];
	maxColorBounds= new float[3];
	for (int k = 0; k<3; k++){
		minColorBounds[k] = minOpacBounds[k] = 0.f;
		maxColorBounds[k] = maxOpacBounds[k] = 1.f;
	}
	
	
}
FlowParams::~FlowParams(){
	if (savedCommand) delete savedCommand;
	if (mapperFunction){
		delete mapperFunction;//this will delete the editor
	}
	if (flowData) {
		for (int i = 0; i<= maxFrame; i++){
			if (flowRGBAs && flowRGBAs[i]) delete flowRGBAs[i];
			if (flowData[i]) delete flowData[i];
		}
		delete flowData;
		if(flowRGBAs)delete flowRGBAs;
	}
}

//Make a copy of  parameters:
Params* FlowParams::
deepCopy(){
	
	FlowParams* newFlowParams = new FlowParams(*this);
	//Clone the map bounds arrays:
	int numVars = numVariables+3;
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
	
	//Clone the Transfer Function and the TFEditor
	if (mapperFunction) {
		newFlowParams->mapperFunction = new MapperFunction(*mapperFunction);
		FlowMapEditor* newFlowMapEditor = new FlowMapEditor((const FlowMapEditor &)(*flowMapEditor));
		newFlowParams->connectMapperFunction(newFlowParams->mapperFunction,newFlowMapEditor); 
	} else {
		newFlowParams->mapperFunction = 0;
	}
	//Don't copy flow data pointers (not that deep!)
	newFlowParams->flowRGBAs = 0;
	newFlowParams->flowData = 0;

	//never keep the SavedCommand:
	newFlowParams->savedCommand = 0;
	return (Params*)newFlowParams;
}


void FlowParams::
makeCurrent(Params* prev, bool newWin) {
	
	VizWinMgr::getInstance()->setFlowParams(vizNum, this);
	updateDialog();
	//Need to create/destroy renderer if there's a change in local/global or enable/disable
	//or if the window is new
	//
	FlowParams* formerParams = (FlowParams*)prev;
	if (formerParams->enabled != enabled || formerParams->local != local || newWin){
		updateRenderer(formerParams->enabled, formerParams->local, newWin);
	}
}

void FlowParams::updateDialog(){
	
	Session::getInstance()->blockRecording();
	myFlowTab->flowMapFrame->setEditor(flowMapEditor);
	//Get the region corners from the current applicable region panel,
	//or the global one:
	
	myFlowTab->EnableDisable->setCurrentItem((enabled) ? 1 : 0);
	
	myFlowTab->flowTypeCombo->setCurrentItem(flowType);
	myFlowTab->numTransSpin->setMinValue(minNumTrans);
	myFlowTab->numTransSpin->setMaxValue(maxNumTrans);
	myFlowTab->numTransSpin->setValue(numTransforms);
	//Always allow at least 3 variables in combo:
	int numVars = numVariables;
	if (numVars < 3) numVars = 3;
	myFlowTab->xCoordVarCombo->clear();
	myFlowTab->xCoordVarCombo->setMaxCount(numVars);
	myFlowTab->yCoordVarCombo->clear();
	myFlowTab->yCoordVarCombo->setMaxCount(numVars);
	myFlowTab->zCoordVarCombo->clear();
	myFlowTab->zCoordVarCombo->setMaxCount(numVars);
	for (int i = 0; i< numVars; i++){
		if (variableNames.size() > (unsigned int)i){
			const std::string& s = variableNames.at(i);
			//Direct conversion of std::string& to QString doesn't seem to work
			//Maybe std was not enabled when QT was built?
			const QString& text = QString(s.c_str());
			myFlowTab->xCoordVarCombo->insertItem(text);
			myFlowTab->yCoordVarCombo->insertItem(text);
			myFlowTab->zCoordVarCombo->insertItem(text);
		} else {
			myFlowTab->xCoordVarCombo->insertItem("");
			myFlowTab->yCoordVarCombo->insertItem("");
			myFlowTab->zCoordVarCombo->insertItem("");
		}
	}
	myFlowTab->xCoordVarCombo->setCurrentItem(varNum[0]);
	myFlowTab->yCoordVarCombo->setCurrentItem(varNum[1]);
	myFlowTab->zCoordVarCombo->setCurrentItem(varNum[2]);
	
	
	myFlowTab->timeSampleEdit->setEnabled(flowType == 1);
	myFlowTab->recalcButton->setEnabled(flowType == 1);
	myFlowTab->seedtimeIncrementEdit->setEnabled(flowType == 1);
	myFlowTab->seedtimeEndEdit->setEnabled(flowType == 1);
	myFlowTab->firstDisplayFrameEdit->setEnabled(flowType == 1);
	myFlowTab->seedtimeStartEdit->setEnabled(flowType == 1);

	myFlowTab->randomCheckbox->setChecked(randomGen);
	
	myFlowTab->randomSeedEdit->setEnabled(randomGen);
	myFlowTab->generatorDimensionCombo->setCurrentItem(currentDimension);

	for (int i = 0; i< 3; i++){
		enforceConsistency(i);
		textToSlider(i, (seedBoxMin[i]+seedBoxMax[i])*0.5f,
			seedBoxMax[i]-seedBoxMin[i]);
	}


	//Geometric parameters:
	myFlowTab->geometryCombo->setCurrentItem(geometryType);
	
	myFlowTab->colormapEntityCombo->setCurrentItem(getColorMapEntityIndex());
	myFlowTab->opacmapEntityCombo->setCurrentItem(getOpacMapEntityIndex());
	if (isLocal())
		myFlowTab->LocalGlobal->setCurrentItem(1);
	else 
		myFlowTab->LocalGlobal->setCurrentItem(0);

	if (randomGen){
		myFlowTab->dimensionLabel->setEnabled(false);
		myFlowTab->generatorDimensionCombo->setEnabled(false);
	} else {
		myFlowTab->dimensionLabel->setEnabled(true);
		myFlowTab->generatorDimensionCombo->setEnabled(true);
		myFlowTab->generatorDimensionCombo->setCurrentItem(currentDimension);
	}
	
	//Put all the setText messages here, so they won't trigger a textChanged message
	if (randomGen){
		myFlowTab->generatorCountEdit->setText(QString::number(allGeneratorCount));
	} else {
		myFlowTab->generatorCountEdit->setText(QString::number(generatorCount[currentDimension]));
	}
	myFlowTab->integrationAccuracyEdit->setText(QString::number(integrationAccuracy));
	myFlowTab->scaleFieldEdit->setText(QString::number(velocityScale));
	myFlowTab->timeSampleEdit->setText(QString::number(timeSamplingInterval));
	myFlowTab->randomSeedEdit->setText(QString::number(randomSeed));
	myFlowTab->geometrySamplesEdit->setText(QString::number(objectsPerTimestep,'g',4));
	
	myFlowTab->geometrySamplesSlider->setValue((int)(64.0*log10(100.0*objectsPerTimestep)));
	myFlowTab->firstDisplayFrameEdit->setText(QString::number(firstDisplayFrame));
	myFlowTab->lastDisplayFrameEdit->setText(QString::number(lastDisplayFrame));
	myFlowTab->diameterEdit->setText(QString::number(shapeDiameter));
	myFlowTab->constantOpacityEdit->setText(QString::number(constantOpacity));
	myFlowTab->constantColorButton->setPaletteBackgroundColor(QColor(constantColor));
	if (mapperFunction){
		myFlowTab->minColormapEdit->setText(QString::number(mapperFunction->getMinColorMapValue()));
		myFlowTab->maxColormapEdit->setText(QString::number(mapperFunction->getMaxColorMapValue()));
		myFlowTab->minOpacmapEdit->setText(QString::number(mapperFunction->getMinOpacMapValue()));
		myFlowTab->maxOpacmapEdit->setText(QString::number(mapperFunction->getMaxOpacMapValue()));
	}
	myFlowTab->xSizeEdit->setText(QString::number(seedBoxMax[0]-seedBoxMin[0],'g', 4));
	myFlowTab->xCenterEdit->setText(QString::number(0.5f*(seedBoxMax[0]+seedBoxMin[0]),'g',5));
	myFlowTab->ySizeEdit->setText(QString::number(seedBoxMax[1]-seedBoxMin[1],'g', 4));
	myFlowTab->yCenterEdit->setText(QString::number(0.5f*(seedBoxMax[1]+seedBoxMin[1]),'g',5));
	myFlowTab->zSizeEdit->setText(QString::number(seedBoxMax[2]-seedBoxMin[2],'g', 4));
	myFlowTab->zCenterEdit->setText(QString::number(0.5f*(seedBoxMax[2]+seedBoxMin[2]),'g',5));
	myFlowTab->seedtimeIncrementEdit->setText(QString::number(seedTimeIncrement));
	myFlowTab->seedtimeStartEdit->setText(QString::number(seedTimeStart));
	myFlowTab->seedtimeEndEdit->setText(QString::number(seedTimeEnd));

	//Put the opacity and color bounds for the currently chosen mappings
	int var = getColorMapEntityIndex();
	float maxSpeed;
	switch (var){
		case(0):
			myFlowTab->minColorBound->setText("0.0");
			myFlowTab->maxColorBound->setText("1.0");
			break;
		case (1):
			myFlowTab->minColorBound->setText(QString::number(firstDisplayFrame));
			myFlowTab->maxColorBound->setText(QString::number(lastDisplayFrame));
			break;
		case(2)://speed
			myFlowTab->minColorBound->setText("0.0");
			maxSpeed = 0.f;
			for (int k = 0; k<3; k++){
				int v = varNum[k];
				if (maxSpeed < fabs(Session::getInstance()->getDataStatus()->getDataMaxOverTime(v)))
					maxSpeed = fabs(Session::getInstance()->getDataStatus()->getDataMaxOverTime(v));
				if (maxSpeed < fabs(Session::getInstance()->getDataStatus()->getDataMinOverTime(v)))
					maxSpeed = fabs(Session::getInstance()->getDataStatus()->getDataMinOverTime(v));
			}
			myFlowTab->maxColorBound->setText(QString::number(maxSpeed));
			break;
		default:
			myFlowTab->minColorBound->setText(QString::number(Session::getInstance()->getDataStatus()->getDataMinOverTime(var-3)));
			myFlowTab->maxColorBound->setText(QString::number(Session::getInstance()->getDataStatus()->getDataMaxOverTime(var-3)));
			break;
	}
	var = getOpacMapEntityIndex();
	switch (var){
		case(0):
			myFlowTab->minOpacityBound->setText(QString("0.0"));
			myFlowTab->maxOpacityBound->setText(QString("1.0"));
			break;
		case (1)://age
			myFlowTab->minOpacityBound->setText(QString::number(firstDisplayFrame));
			myFlowTab->maxOpacityBound->setText(QString::number(lastDisplayFrame));
			break;
		case(2)://speed
			myFlowTab->minOpacityBound->setText(QString("0.0"));
			maxSpeed = 0.f;
			for (int k = 0; k<3; k++){
				int v = varNum[k];
				if (maxSpeed < fabs(Session::getInstance()->getDataStatus()->getDataMaxOverTime(v)))
					maxSpeed = fabs(Session::getInstance()->getDataStatus()->getDataMaxOverTime(v));
				if (maxSpeed < fabs(Session::getInstance()->getDataStatus()->getDataMinOverTime(v)))
					maxSpeed = fabs(Session::getInstance()->getDataStatus()->getDataMinOverTime(v));
			}
			myFlowTab->maxOpacityBound->setText(QString::number(maxSpeed));
			break;
		default:
			myFlowTab->minOpacityBound->setText(QString::number(Session::getInstance()->getDataStatus()->getDataMinOverTime(var-3)));
			myFlowTab->maxOpacityBound->setText(QString::number(Session::getInstance()->getDataStatus()->getDataMaxOverTime(var-3)));
			break;
	}
	
	guiSetTextChanged(false);
	if(getFlowMapEditor())getFlowMapEditor()->setDirty();
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}

//Update all the panel state associated with textboxes, after user presses return
//
void FlowParams::
updatePanelState(){
	
	integrationAccuracy = myFlowTab->integrationAccuracyEdit->text().toFloat();
	if (integrationAccuracy < 0.f || integrationAccuracy > 1.f) {
		if (integrationAccuracy > 1.f) integrationAccuracy = 1.f;
		if (integrationAccuracy < 0.f) integrationAccuracy = 0.f;
		myFlowTab->integrationAccuracyEdit->setText(QString::number(integrationAccuracy));
	}

	velocityScale = myFlowTab->scaleFieldEdit->text().toFloat();
	if (velocityScale < 1.e-20f){
		velocityScale = 1.e-20f;
		myFlowTab->scaleFieldEdit->setText(QString::number(velocityScale));
	}
	timeSamplingInterval = myFlowTab->timeSampleEdit->text().toInt();
	if (timeSamplingInterval < 1){
		timeSamplingInterval = 1;
		myFlowTab->timeSampleEdit->setText(QString::number(timeSamplingInterval));
	}
	randomSeed = myFlowTab->randomSeedEdit->text().toUInt();

	//Get slider positions from text boxes:
	
	float boxCtr = myFlowTab->xCenterEdit->text().toFloat();
	float boxSize = myFlowTab->xSizeEdit->text().toFloat();
	seedBoxMin[0] = boxCtr - 0.5*boxSize;
	seedBoxMax[0] = boxCtr + 0.5*boxSize;
	textToSlider(0, boxCtr, boxSize);
	boxCtr = myFlowTab->yCenterEdit->text().toFloat();
	boxSize = myFlowTab->ySizeEdit->text().toFloat();
	seedBoxMin[1] = boxCtr - 0.5*boxSize;
	seedBoxMax[1] = boxCtr + 0.5*boxSize;
	textToSlider(1, boxCtr, boxSize);
	boxCtr = myFlowTab->zCenterEdit->text().toFloat();
	boxSize = myFlowTab->zSizeEdit->text().toFloat();
	seedBoxMin[2] = boxCtr - 0.5*boxSize;
	seedBoxMax[2] = boxCtr + 0.5*boxSize;
	textToSlider(2, boxCtr, boxSize);

	int genCount = myFlowTab->generatorCountEdit->text().toInt();
	if (genCount < 1) {
		genCount = 1;
		myFlowTab->generatorCountEdit->setText(QString::number(genCount));
	}
	if (randomGen) {
		allGeneratorCount = genCount;
	} else {
		generatorCount[currentDimension] = genCount;
	}
	
	seedTimeStart = myFlowTab->seedtimeStartEdit->text().toUInt();
	seedTimeEnd = myFlowTab->seedtimeEndEdit->text().toUInt(); 
	bool changed = false;
	if (seedTimeStart < minFrame) {seedTimeStart = minFrame; changed = true;}
	if (seedTimeEnd > maxFrame) {seedTimeEnd = maxFrame; changed = true;}
	if (seedTimeEnd < seedTimeStart) {seedTimeEnd = seedTimeStart; changed = true;}
	if (changed){
		myFlowTab->seedtimeStartEdit->setText(QString::number(seedTimeStart));
		myFlowTab->seedtimeEndEdit->setText(QString::number(seedTimeEnd));
	}

	seedTimeIncrement = myFlowTab->seedtimeIncrementEdit->text().toUInt();

	objectsPerTimestep = myFlowTab->geometrySamplesEdit->text().toFloat();
	if (objectsPerTimestep <= 0.01f || objectsPerTimestep > 100.f) {
		objectsPerTimestep = 1.f;
		myFlowTab->geometrySamplesEdit->setText(QString::number(objectsPerTimestep));
	}
	myFlowTab->geometrySamplesSlider->setValue((int)(64.0*log10(100.0*objectsPerTimestep)));
	firstDisplayFrame = myFlowTab->firstDisplayFrameEdit->text().toInt();
	lastDisplayFrame = myFlowTab->lastDisplayFrameEdit->text().toInt();
	if (lastDisplayFrame < -firstDisplayFrame) {
		lastDisplayFrame = -firstDisplayFrame;
		myFlowTab->lastDisplayFrameEdit->setText(QString::number(lastDisplayFrame));
	}
	shapeDiameter = myFlowTab->diameterEdit->text().toFloat();
	if (shapeDiameter < 0.f) {
		shapeDiameter = 0.f;
		myFlowTab->diameterEdit->setText(QString::number(shapeDiameter));
	}
	constantOpacity = myFlowTab->constantOpacityEdit->text().toFloat();
	if (constantOpacity < 0.f) constantOpacity = 0.f;
	if (constantOpacity > 1.f) constantOpacity = 1.f;
	float colorMapMin = myFlowTab->minColormapEdit->text().toFloat();
	float colorMapMax = myFlowTab->maxColormapEdit->text().toFloat();
	if (colorMapMin >= colorMapMax){
		colorMapMax = colorMapMin+1.e-6;
		myFlowTab->maxColormapEdit->setText(QString::number(colorMapMax));
	}
	float opacMapMin = myFlowTab->minOpacmapEdit->text().toFloat();
	float opacMapMax = myFlowTab->maxOpacmapEdit->text().toFloat();
	if (opacMapMin >= opacMapMax){
		opacMapMax = opacMapMin+1.e-6;
		myFlowTab->maxOpacmapEdit->setText(QString::number(opacMapMax));
	}
	if (mapperFunction){
		mapperFunction->setMaxColorMapValue(colorMapMax);
		mapperFunction->setMinColorMapValue(colorMapMin);
		mapperFunction->setMaxOpacMapValue(opacMapMax);
		mapperFunction->setMinOpacMapValue(opacMapMin);
	}
	if(flowMapEditor)flowMapEditor->setDirty();
	myFlowTab->flowMapFrame->update();
	guiSetTextChanged(false);
	setFlowDataDirty();
	
}
//Reinitialize settings, session has changed:
void FlowParams::
reinit(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	Session* session = Session::getInstance();
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->
			getRegionParams(vizNum);
	int nlevels = md->GetNumTransforms();
	int minTrans = session->getDataStatus()->minXFormPresent();
	if(minTrans < 0) minTrans = nlevels; 
	setMinNumTrans(minTrans);
	setMaxNumTrans(nlevels);
	//Clean out any existing caches:
	if (flowData){
		for (i = 0; i<= maxFrame; i++){
			if (flowData[i]) delete flowData[i];
			if (flowRGBAs && flowRGBAs[i]) delete flowRGBAs[i];
		}
		delete flowData;
		if (flowRGBAs) delete flowRGBAs;
		flowData = 0;
		flowRGBAs = 0;
	}
	//Make min and max conform to new data:
	minFrame = (int)(session->getMinTimestep());
	maxFrame = (int)(session->getMaxTimestep());
	editMode = true;
	if (doOverride) {
		numTransforms = maxNumTrans;
		seedTimeStart = minFrame;
		seedTimeEnd = maxFrame;
	} else {
		if (numTransforms> nlevels) numTransforms = maxNumTrans;
		if (numTransforms < minNumTrans) numTransforms = minNumTrans;
		//Make sure we really can use the specified numTrans.
		
		numTransforms = rParams->validateNumTrans(numTransforms);
		if (seedTimeStart > maxFrame) seedTimeStart = maxFrame;
		if (seedTimeStart < minFrame) seedTimeStart = minFrame;
		if (seedTimeEnd > maxFrame) seedTimeEnd = maxFrame;
		if (seedTimeEnd < minFrame) seedTimeEnd = minFrame;
	}
	//Set up the seed region:
	
	if (doOverride){
		for (i = 0; i<3; i++){
			seedBoxMin[i] = rParams->getFullDataExtent(i);
			seedBoxMax[i] = rParams->getFullDataExtent(i+3);
		}
	} else {
		for (i = 0; i<3; i++){
			if(seedBoxMin[i] < rParams->getFullDataExtent(i))
				seedBoxMin[i] = rParams->getFullDataExtent(i);
			if(seedBoxMax[i] > rParams->getFullDataExtent(i+3))
				seedBoxMax[i] = rParams->getFullDataExtent(i+3);
			if(seedBoxMax[i] < seedBoxMin[i]) 
				seedBoxMax[i] = seedBoxMin[i];
		}
	}

	//Set up variables:
	//Get the variable names:
	variableNames = md->GetVariableNames();
	int newNumVariables = md->GetVariableNames().size();

	//Rebuild map bounds arrays:
	if(minOpacBounds) delete minOpacBounds;
	minOpacBounds = new float[newNumVariables+3];
	if(maxOpacBounds) delete maxOpacBounds;
	maxOpacBounds = new float[newNumVariables+3];
	if(minColorBounds) delete minColorBounds;
	minColorBounds = new float[newNumVariables+3];
	if(maxColorBounds) delete maxColorBounds;
	maxColorBounds = new float[newNumVariables+3];
	
	
	colorMapEntity.clear();
	colorMapEntity.push_back("Constant");
	colorMapEntity.push_back("Age");
	colorMapEntity.push_back("Speed");
	opacMapEntity.clear();
	opacMapEntity.push_back("Constant");
	opacMapEntity.push_back("Age");
	opacMapEntity.push_back("Speed");
	for (i = 0; i< newNumVariables; i++){
		colorMapEntity.push_back(variableNames[i]);
		opacMapEntity.push_back(variableNames[i]);
	}
	//Set up the color, opac map entity combos:
	//This is done when we load a new dataset!
	assert(myFlowTab);
	myFlowTab->colormapEntityCombo->clear();
	for (i = 0; i< (int)colorMapEntity.size(); i++){
		myFlowTab->colormapEntityCombo->insertItem(QString(colorMapEntity[i].c_str()));
	}
	myFlowTab->opacmapEntityCombo->clear();
	for (i = 0; i< (int)colorMapEntity.size(); i++){
		myFlowTab->opacmapEntityCombo->insertItem(QString(opacMapEntity[i].c_str()));
	}
	if(doOverride || getOpacMapEntityIndex() >= newNumVariables+3){
		setOpacMapEntity(0);
	}
	if(doOverride || getColorMapEntityIndex() >= newNumVariables+3){
		setColorMapEntity(0);
	}
	if (doOverride){
		varNum[0] = 0; varNum[1] = 1; varNum[2] = 2;
	} 
	for (int dim = 0; dim < 3; dim++){
		//See if current varNum is valid.  If not, 
		//reset to first variable that is present:
		if (!Session::getInstance()->getDataStatus()->variableIsPresent(varNum[dim])){
			varNum[dim] = -1;
			for (i = 0; i<newNumVariables; i++) {
				if (Session::getInstance()->getDataStatus()->variableIsPresent(i)){
					varNum[dim] = i;
					break;
				}
			}
		}
	}
	if (varNum[0] == -1){
		MessageReporter::errorMsg("Flow Params: No data in specified dataset");
		numVariables = 0;
		return;
	}
	//Now set up bounds arrays based on current mapped variable settings:
	minOpacBounds[0] = 0.f;
	maxOpacBounds[0] = 1.f;
	minColorBounds[0] = 0.f;
	maxColorBounds[0] = 1.f;
	minOpacBounds[1] = (float)getFirstDisplayFrame(); //min age
	maxOpacBounds[1] = (float)getFirstDisplayFrame(); //min age
	minColorBounds[1] = (float)getLastDisplayFrame(); //min age
	maxColorBounds[1] = (float)getLastDisplayFrame(); //min age
	minOpacBounds[2] = 0.f;
	minColorBounds[2] = 0.f;
	float maxSpeed = 0.f;
	for (int k = 0; k<3; k++){
		int var = varNum[k];
		if (maxSpeed < fabs(Session::getInstance()->getDataStatus()->getDataMaxOverTime(var)))
			maxSpeed = fabs(Session::getInstance()->getDataStatus()->getDataMaxOverTime(var));
		if (maxSpeed < fabs(Session::getInstance()->getDataStatus()->getDataMinOverTime(var)))
			maxSpeed = fabs(Session::getInstance()->getDataStatus()->getDataMinOverTime(var));
	}
	maxOpacBounds[2] = maxColorBounds[2] = maxSpeed;
	for (int k = 0; k<newNumVariables; k++){
		if (Session::getInstance()->getDataStatus()->variableIsPresent(k)){
			minOpacBounds[k+3] = minColorBounds[k+3] = Session::getInstance()->getDataStatus()->getDataMinOverTime(k);
			maxOpacBounds[k+3] = maxColorBounds[k+3] = Session::getInstance()->getDataStatus()->getDataMaxOverTime(k);
		}
		else {
			minOpacBounds[k+3] = minColorBounds[k+3] = 0.f;
			maxOpacBounds[k+3] = maxColorBounds[k+3] = 1.f;
		}
	}
	
	//For now, assume 8-bits mapping
	MapperFunction* newMapperFunction = new MapperFunction(this, 8);
	//Initialize to be fully opaque:
	newMapperFunction->setOpaque();
	//The edit and tf bounds need to be set up with const, speed, etc in mind.
	FlowMapEditor* newFlowMapEditor = new FlowMapEditor(newMapperFunction, myFlowTab->flowMapFrame);
	connectMapperFunction(newMapperFunction, newFlowMapEditor);
	//If we are overriding, setup for constant color map.  Otherwise,
	//use previous values.
	float* newMinOpacEditBounds = new float[newNumVariables+3];
	float* newMaxOpacEditBounds = new float[newNumVariables+3];
	float* newMinColorEditBounds = new float[newNumVariables+3];
	float* newMaxColorEditBounds = new float[newNumVariables+3];
	if (doOverride){
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
		for (i = 0; i< newNumVariables; i++){
			if (Session::getInstance()->getDataStatus()->variableIsPresent(i)){
				newMinOpacEditBounds[i+3] = Session::getInstance()->getDataRange(i)[0];
				newMaxOpacEditBounds[i+3] = Session::getInstance()->getDataRange(i)[1];
				newMinColorEditBounds[i+3] = Session::getInstance()->getDataRange(i)[0];
				newMaxColorEditBounds[i+3] = Session::getInstance()->getDataRange(i)[1];
			} else {
				newMinOpacEditBounds[i+3] = 0.f;
				newMaxOpacEditBounds[i+3] = 1.f;
				newMinColorEditBounds[i+3] = 0.f;
				newMaxColorEditBounds[i+3] = 1.f;
			}
		} 
	} else { //Try to reuse existing bounds:
		//Set to default map bounds
		newMapperFunction->setMinColorMapValue(mapperFunction->getMinColorMapValue());
		newMapperFunction->setMaxColorMapValue(mapperFunction->getMaxColorMapValue());
		newMapperFunction->setMinOpacMapValue(mapperFunction->getMinOpacMapValue());
		newMapperFunction->setMaxOpacMapValue(mapperFunction->getMaxOpacMapValue());
		for (i = 0; i< newNumVariables; i++){
			if (i >= numVariables){
				newMinOpacEditBounds[i+3] = Session::getInstance()->getDataRange(i)[0];
				newMaxOpacEditBounds[i+3] = Session::getInstance()->getDataRange(i)[1];
				newMinColorEditBounds[i+3] = Session::getInstance()->getDataRange(i)[0];
				newMaxColorEditBounds[i+3] = Session::getInstance()->getDataRange(i)[1];
			} else {
				newMinOpacEditBounds[i+3] = minOpacEditBounds[i+3];
				newMaxOpacEditBounds[i+3] = maxOpacEditBounds[i+3];
				newMinColorEditBounds[i+3] = minColorEditBounds[i+3];
				newMaxColorEditBounds[i+3] = maxColorEditBounds[i+3];
			}
		} 
	}
	if (mapperFunction) {
		delete mapperFunction;
		delete minOpacEditBounds;
		delete minColorEditBounds;
		delete maxOpacEditBounds;
		delete maxColorEditBounds;
	}
	minOpacEditBounds = newMinOpacEditBounds;
	maxOpacEditBounds = newMaxOpacEditBounds;
	minColorEditBounds = newMinColorEditBounds;
	maxColorEditBounds = newMaxColorEditBounds;
	mapperFunction = newMapperFunction;
	flowMapEditor = newFlowMapEditor;
	numVariables = newNumVariables;
	
	//Always disable
	bool wasEnabled = enabled;
	setEnabled(false);

	//setup the caches
	flowData = new float*[maxFrame+1];
	for (int j = 0; j<= maxFrame; j++) flowData[j] = 0;

	//don't change local/global 
	updateRenderer(wasEnabled, isLocal(), false);
	
	if(numVariables>0) getFlowMapEditor()->setDirty();
	//If flow is the current front tab, and if it applies to the active visualizer,
	//update its values
	if(MainForm::getInstance()->getTabManager()->isFrontTab(myFlowTab)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum == vizNum)
			updateDialog();
	}
	//force a new render with new flow data
	setFlowDataDirty();
}
//Set slider position, based on text change. 
//
void FlowParams::
textToSlider(int coord, float newCenter, float newSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	bool centerChanged = false;
	bool sizeChanged = false;
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	float regMin = rParams->getFullDataExtent(coord);
	float regMax = rParams->getFullDataExtent(coord+3);
	if (newSize > (regMax-regMin)){
		newSize = regMax-regMin;
		sizeChanged = true;
	}
	if (newSize < 0.f) {
		newSize = 0.f;
		sizeChanged = true;
	}
	if (newCenter < regMin) {
		newCenter = regMin;
		centerChanged = true;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
		centerChanged = true;
	}
	if ((newCenter - newSize*0.5f) < regMin){
		newCenter = regMin+ newSize*0.5f;
		centerChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regMax){
		newCenter = regMax- newSize*0.5f;
		centerChanged = true;
	}
	if (newSize <= 0.f && !randomGen){
		if (generatorCount[coord] != 1) {
			generatorCount[coord] = 1;
			if (currentDimension == coord)
				myFlowTab->generatorCountEdit->setText("1");
		}
	}
	seedBoxMin[coord] = newCenter - newSize*0.5f; 
	seedBoxMax[coord] = newCenter + newSize*0.5f; 
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = myFlowTab->xSizeSlider->value();
			oldSliderCenter = myFlowTab->xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myFlowTab->xSizeSlider->setValue(sliderSize);
			if(sizeChanged) myFlowTab->xSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myFlowTab->xCenterSlider->setValue(sliderCenter);
			if(centerChanged) myFlowTab->xCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = myFlowTab->ySizeSlider->value();
			oldSliderCenter = myFlowTab->yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myFlowTab->ySizeSlider->setValue(sliderSize);
			if(sizeChanged) myFlowTab->ySizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myFlowTab->yCenterSlider->setValue(sliderCenter);
			if(centerChanged) myFlowTab->yCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = myFlowTab->zSizeSlider->value();
			oldSliderCenter = myFlowTab->zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				myFlowTab->zSizeSlider->setValue(sliderSize);
			if(sizeChanged) myFlowTab->zSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				myFlowTab->zCenterSlider->setValue(sliderCenter);
			if(centerChanged) myFlowTab->zCenterEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	myFlowTab->update();
	return;
}
//Set text when a slider changes.
//Move the center if the size is too big
//
void FlowParams::
sliderToText(int coord, int slideCenter, int slideSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	float regMin = rParams->getFullDataExtent(coord);
	float regMax = rParams->getFullDataExtent(coord+3);
	bool sliderChanged = false;
	
	float newSize = slideSize*(regMax-regMin)/256.f;
	float newCenter = regMin+ slideCenter*(regMax-regMin)/256.f;
	
	if (newCenter < regMin) {
		newCenter = regMin;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
	}
	if ((newCenter - newSize*0.5f) < regMin){
		newCenter = regMin+ newSize*0.5f;
		sliderChanged = true;
	}
	if ((newCenter + newSize*0.5f) > regMax){
		newCenter = regMax- newSize*0.5f;
		sliderChanged = true;
	}
	seedBoxMin[coord] = newCenter - newSize*0.5f; 
	seedBoxMax[coord] = newCenter + newSize*0.5f; 
	if (newSize <= 0.f && !randomGen){
		if (generatorCount[coord] != 1) {
			generatorCount[coord] = 1;
			if (currentDimension == coord)
				myFlowTab->generatorCountEdit->setText("1");
		}
	}
	int newSliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	//Always need to change text.  Possibly also change slider if it was moved
	switch(coord) {
		case 0:
			if (sliderChanged) 
				myFlowTab->xCenterSlider->setValue(newSliderCenter);
			myFlowTab->xSizeEdit->setText(QString::number(newSize));
			myFlowTab->xCenterEdit->setText(QString::number(newCenter));
			break;
		case 1:
			if (sliderChanged) 
				myFlowTab->yCenterSlider->setValue(newSliderCenter);
			myFlowTab->ySizeEdit->setText(QString::number(newSize));
			myFlowTab->yCenterEdit->setText(QString::number(newCenter));
			break;
		case 2:
			if (sliderChanged) 
				myFlowTab->zCenterSlider->setValue(newSliderCenter);
			myFlowTab->zSizeEdit->setText(QString::number(newSize));
			myFlowTab->zCenterEdit->setText(QString::number(newCenter));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	myFlowTab->update();
	//force a new render with new flow data
	setFlowDataDirty();
	return;
}	




//Methods that record changes in the history:
//
void FlowParams::
guiSetEnabled(bool on){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "enable/disable flow render");
	setEnabled(on);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetFlowType(int typenum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow type");
	setFlowType(typenum);
	myFlowTab->firstDisplayFrameEdit->setEnabled(flowType == 1);
	if (typenum == 0) 
		firstDisplayFrame = 0;
	PanelCommand::captureEnd(cmd, this);
	
}
void FlowParams::
guiSetNumTrans(int n){
	confirmText(false);
	
	int newNumTrans = ((RegionParams*)(VizWinMgr::getInstance()->getRegionParams(vizNum)))->validateNumTrans(n);
	if (newNumTrans != n) {
		MessageReporter::warningMsg("%s","Invalid number of Transforms for current region, data cache size");
		myFlowTab->numTransSpin->setValue(newNumTrans);
	}
	PanelCommand* cmd = PanelCommand::captureStart(this, "set number of Transformations in Flow data");
	setNumTrans(newNumTrans);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetXVarNum(int varnum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set X field variable");
	setXVarNum(varnum);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetYVarNum(int varnum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set Y field variable");
	setYVarNum(varnum);
	PanelCommand::captureEnd(cmd, this);

}
void FlowParams::
guiSetZVarNum(int varnum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set Z field variable");
	setZVarNum(varnum);
	PanelCommand::captureEnd(cmd, this);

}
void FlowParams::
guiRecalc(){ //Not covered by redo/undo
	confirmText(false);
	//Need to implement!
}
void FlowParams::
guiSetConstantColor(QColor& newColor){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set constant mapping color");
	constantColor = newColor.rgb();
	PanelCommand::captureEnd(cmd, this);
	setFlowMappingDirty();
}
//Slider sets the geometry sampling rate, between 0.01 and 100.0
// value is 0.01*10**(4s)  where s is between 0 and 1
void FlowParams::
guiSetGeomSamples(int sliderPos){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set geometry sampling rate");
	float s = ((float)(sliderPos))/256.f;
	objectsPerTimestep = 0.01f*pow(10.f,4.f*s);
	//Make it easy to get 1.0:
	if (objectsPerTimestep < 1.05f && objectsPerTimestep > 0.95) objectsPerTimestep = 1.0f;
	myFlowTab->geometrySamplesEdit->setText(QString::number(objectsPerTimestep,'g',4));
	guiSetTextChanged(false);
	
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}

void FlowParams::
guiSetRandom(bool rand){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "toggle random setting");
	setRandom(rand);
	//Also display the appropriate numGenerators
	int genCount = allGeneratorCount;
	if (!rand) genCount = generatorCount[currentDimension];
	myFlowTab->generatorCountEdit->setText(QString::number(genCount));
	guiSetTextChanged(false);
	myFlowTab->update();
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetXCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator X center");
	setXCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}
void FlowParams::
guiSetYCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Y center");
	setYCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}
void FlowParams::
guiSetZCenter(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Z center");
	setZCenter(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}
void FlowParams::
guiSetXSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator X size");
	setXSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}
void FlowParams::
guiSetYSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Y size");
	setYSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}
void FlowParams::
guiSetZSize(int sliderval){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "slide flow generator Z size");
	setZSize(sliderval);
	PanelCommand::captureEnd(cmd, this);
	setFlowDataDirty();
}
void FlowParams::
guiSetFlowGeometry(int geomNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow geometry type");
	setFlowGeometry(geomNum);
	PanelCommand::captureEnd(cmd, this);
	updateMapBounds();
	updateDialog();
	myFlowTab->update();
	//If you change the geometry, you do not need to recalculate the flow,
	//But you need to rerender
	VizWinMgr::getInstance()->refreshFlow(this);
}
void FlowParams::
guiSetColorMapEntity( int entityNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow colormap entity");
	setColorMapEntity(entityNum);
	PanelCommand::captureEnd(cmd, this);
	updateMapBounds();
	
	updateDialog();
	myFlowTab->update();
	//We only need to redo the flowData if the entity is changing to "speed"
	if(entityNum == 2) setFlowDataDirty();
	else setFlowMappingDirty();
}
void FlowParams::
guiSetOpacMapEntity( int entityNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "set flow opacity map entity");
	setOpacMapEntity(entityNum);
	
	PanelCommand::captureEnd(cmd, this);
	updateMapBounds();
	updateDialog();
	myFlowTab->update();
	//We only need to redo the flowData if the entity is changing to "speed"
	if(entityNum == 2) setFlowDataDirty();
	else setFlowMappingDirty();
}
void FlowParams::
guiSetGeneratorDimension( int dimNum){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this,  "specify generator dimension");
	setCurrentDimension(dimNum);
	myFlowTab->generatorCountEdit->setText(QString::number(generatorCount[dimNum]));
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, this);
	myFlowTab->update();
	setFlowDataDirty();
}
//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void FlowParams::
guiSetEditMode(bool mode){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "set edit/navigate mode");
	setEditMode(mode);
	PanelCommand::captureEnd(cmd, this);
}
void FlowParams::
guiSetAligned(){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, "align map function in edit frame");
	setMinColorEditBound(getMinColorMapBound(),getColorMapEntityIndex());
	setMaxColorEditBound(getMaxColorMapBound(),getColorMapEntityIndex());
	setMinOpacEditBound(getMinOpacMapBound(),getOpacMapEntityIndex());
	setMaxOpacEditBound(getMaxOpacMapBound(),getOpacMapEntityIndex());
	getFlowMapEditor()->setDirty();
	myFlowTab->flowMapFrame->update();
	PanelCommand::captureEnd(cmd, this);
}
//When the center slider moves, set the seedBoxMin and seedBoxMax
void FlowParams::
setXCenter(int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(0, sliderval, myFlowTab->xSizeSlider->value());
	setFlowDataDirty();
}
void FlowParams::
setYCenter(int sliderval){
	sliderToText(1, sliderval, myFlowTab->ySizeSlider->value());
	setFlowDataDirty();
}
void FlowParams::
setZCenter(int sliderval){
	sliderToText(2, sliderval, myFlowTab->zSizeSlider->value());
	setFlowDataDirty();
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void FlowParams::
setXSize(int sliderval){
	sliderToText(0, myFlowTab->xCenterSlider->value(),sliderval);
	setFlowDataDirty();
}
void FlowParams::
setYSize(int sliderval){
	sliderToText(1, myFlowTab->yCenterSlider->value(),sliderval);
	setFlowDataDirty();
}
void FlowParams::
setZSize(int sliderval){
	sliderToText(2, myFlowTab->zCenterSlider->value(),sliderval);
	setFlowDataDirty();
}
	
/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * Flow settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void FlowParams::
updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow){
	bool isLocal = local;
	if (newWindow) {
		prevEnabled = false;
		wasLocal = true;
		isLocal = true;
	}
	if (prevEnabled == enabled && wasLocal == local) return;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* viz = vizWinMgr->getActiveVisualizer();
	if (!viz) return;
	int activeViz = vizWinMgr->getActiveViz();
	assert(activeViz >= 0);
	
	//Four cases to consider:
	//1.  change of local/global with unchanged disabled renderer; do nothing.
	// Same for change of local/global with unchanged, enabled renderer
	
	if (prevEnabled == enabled) return;
	
	//2.  Change of disable->enable with unchanged local renderer.  Create a new renderer in active window.
	// Also applies to double change: disable->enable and local->global 
	// Also applies to disable->enable with global->local
	//3.  change of disable->enable with unchanged global renderer.  Create new renderers in all global windows, 
	//    including active window.
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global Flow settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	//For a new renderer

	
	if (enabled && !prevEnabled){//For cases 2. or 3. :  create a renderer in the active window:
		FlowRenderer* myRenderer = new FlowRenderer (viz);
		viz->prependRenderer(myRenderer, Params::FlowParamsType);
		//Quit if not case 3:
		if (wasLocal || isLocal) return;
	}
	
	if (!isLocal && enabled){ //case 3: create renderers in all *other* global windows, then return
		for (int i = 0; i<MAXVIZWINS; i++){
			if (i == activeViz) continue;
			viz = VizWinMgr::getInstance()->getVizWin(i);
			if (viz && !vizWinMgr->getFlowParams(i)->isLocal()){
				FlowRenderer* myRenderer = new FlowRenderer (viz);
				viz->prependRenderer(myRenderer, Params::FlowParamsType);
			}
		}
		return;
	}
	if (!enabled && prevEnabled && !isLocal && !wasLocal) { //case 5., disable all global renderers
		for (int i = 0; i<MAXVIZWINS; i++){
			viz = VizWinMgr::getInstance()->getVizWin(i);
			if (viz && !vizWinMgr->getFlowParams(i)->isLocal()){
				viz->removeRenderer(Params::FlowParamsType);
			}
		}
		return;
	}
	assert(prevEnabled && !enabled && (isLocal ||(isLocal != wasLocal))); //case 6, disable local only
	viz->removeRenderer(Params::FlowParamsType);
	
	return;

}

void FlowParams::
setEnabled(bool on){
	if (enabled == on) return;
	enabled = on;
	if (!enabled){//delete existing flow lib
		if (myFlowLib) delete myFlowLib;
		myFlowLib = 0;
		return;
	}
	//create a new flow lib:
	assert(myFlowLib == 0);
	DataMgr* dataMgr = Session::getInstance()->getDataMgr();
	assert(dataMgr);
	myFlowLib = new VaporFlow(dataMgr);
	return;
	
}

float* FlowParams::
regenerateFlowData(int timeStep){
	int i;
	int min_dim[3], max_dim[3]; 
	size_t min_bdim[3], max_bdim[3];
	
	float minFull[3], maxFull[3], extents[6];
	if (!myFlowLib) return 0;
	
	float* speeds = 0;
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//specify field components:
	const char* xVar = variableNames[varNum[0]].c_str();
	const char* yVar = variableNames[varNum[1]].c_str();
	const char* zVar = variableNames[varNum[2]].c_str();
	myFlowLib->SetFieldComponents(xVar, yVar, zVar);
	//If these flowparams are shared, we have a problem here; 
	//Different windows could have different regions
	RegionParams* rParams;
	if (vizNum < 0) {
		MessageReporter::warningMsg("FlowParams: Multiple region params may apply to flow");
		rParams = vizMgr->getRegionParams(vizMgr->getActiveViz());
	}
	else rParams = VizWinMgr::getInstance()->getRegionParams(vizNum);
	rParams->calcRegionExtents(min_dim, max_dim, min_bdim, max_bdim, 
		numTransforms, minFull, maxFull, extents);
	myFlowLib->SetRegion(numTransforms, min_bdim, max_bdim);
	if (flowType == 0) { //steady
		myFlowLib->SetTimeStepInterval(timeStep, maxFrame, timeSamplingInterval);
	} else {
		myFlowLib->SetTimeStepInterval(seedTimeStart, maxFrame, timeSamplingInterval);
	}
	myFlowLib->ScaleTimeStepSizes(velocityScale, 1./objectsPerTimestep);
	if (randomGen) {
		myFlowLib->SetRandomSeedPoints(seedBoxMin, seedBoxMax, allGeneratorCount);
		numSeedPoints = allGeneratorCount;
	} else {
		float boxmin[3], boxmax[3];
		for (i = 0; i<3; i++){
			if (generatorCount[i] <= 1) {
				boxmin[i] = (seedBoxMin[i]+seedBoxMax[i])*0.5f;
				boxmax[i] = boxmin[i];
			} else {
				boxmin[i] = seedBoxMin[i];
				boxmax[i] = seedBoxMax[i];
			}
		}
		myFlowLib->SetRegularSeedPoints(boxmin, boxmax, generatorCount);
		numSeedPoints = generatorCount[0]*generatorCount[1]*generatorCount[2];
	}
	// setup integration parameters:
	
	float minIntegStep = SMALLEST_MIN_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MIN_STEP; 
	float maxIntegStep = SMALLEST_MAX_STEP*(integrationAccuracy) + (1.f - integrationAccuracy)*LARGEST_MAX_STEP; 
	
	
	myFlowLib->SetIntegrationParams(minIntegStep, maxIntegStep);
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	
	
	if (flowType == 0) { //steady
		if (flowData[timeStep]) delete flowData[timeStep];
		numInjections = 1;
		maxPoints = (lastDisplayFrame+1)*objectsPerTimestep;
		if (maxPoints < 2) maxPoints = 2;
		flowData[timeStep] = new float[3*maxPoints*numSeedPoints*numInjections];
	} else {// determine largest possible number of injections
		numInjections = 1+ (seedTimeEnd - seedTimeStart)/seedTimeIncrement;
		//For unsteady flow, the firstDisplayFrame and lastDisplayFrame give a window
		//on a longer timespan that potentially goes from 
		//the first seed start time to the last frame in the animation.
		//lastDisplayFrame does not limit the length of the flow
		maxPoints = (maxFrame - seedTimeStart+1)*objectsPerTimestep;
		if (maxPoints < 2) maxPoints = 2;
		if (flowData[0]) delete flowData[0];
		flowData[0] = new float[3*maxPoints*numSeedPoints*numInjections];
	}
	
	if (getColorMapEntityIndex() == 2 || getOpacMapEntityIndex() == 2){
		//to map speed, need to calculate it:
		speeds = new float[maxPoints*numSeedPoints*numInjections];
	}

	///call the flowlib
	if (flowType == 0){ //steady
		
		myFlowLib->GenStreamLines(flowData[timeStep], maxPoints, randomSeed, speeds);
		
	} else {
		
		myFlowLib->GenStreakLines(flowData[0], maxPoints, randomSeed, seedTimeStart, seedTimeEnd, seedTimeIncrement, speeds);
		// With streaklines, need to fill flags to end, and fill speeds as well
		for (int q = 0; q< numSeedPoints*numInjections; q++){
			//Scan streakline for flags
			for (int posn = 0; posn<maxPoints; posn++){
				if(*(flowData[0]+3*(posn +q*maxPoints)) == END_FLOW_FLAG){
					//fill rest of streakline with END_FLOW:
					for (int fillIndex = posn+1; fillIndex<maxPoints; fillIndex++){
						*(flowData[0]+3*(fillIndex +q*maxPoints)) = END_FLOW_FLAG;
					}
					//Done with this streakline:
					break;
				}
				if(*(flowData[0]+3*(posn +q*maxPoints)) == STATIONARY_STREAM_FLAG){
					//fill rest of streakline with STATIONARY_FLOW_FLAG
					for (int fillIndex = posn+1; fillIndex<maxPoints; fillIndex++){
						*(flowData[0]+3*(fillIndex +q*maxPoints)) = STATIONARY_STREAM_FLAG;
					}
					//Done with this streakline:
					break;
				}
			}
		}


	}
	//Invalidate colors and opacities:
	if (flowRGBAs && flowRGBAs[timeStep]) {
		delete flowRGBAs[timeStep];
		flowRGBAs[timeStep] = 0;
	}
		
	if ((getColorMapEntityIndex() + getOpacMapEntityIndex()) > 0){
		if (!flowRGBAs){
			flowRGBAs = new float*[maxFrame+1];
			for (int j = 0; j<= maxFrame; j++) flowRGBAs[j] = 0;
		}
		flowRGBAs[timeStep] = new float[maxPoints*numSeedPoints*numInjections*4];
		mapColors(speeds, timeStep);
		//Now we can release the speeds:
		if (speeds) delete speeds;
	}

	
	// the dirty flag is reset during flow rendering
	return flowData[timeStep];
}
float* FlowParams::getRGBAs(int timeStep){
	if (flowRGBAs && flowRGBAs[timeStep]) return flowRGBAs[timeStep];
	assert((getOpacMapEntityIndex() != 2)&&(getColorMapEntityIndex() != 2)); //Can't map speeds here!
	assert(flowData && flowData[timeStep]);
	if (!flowRGBAs){
		flowRGBAs = new float*[maxFrame+1];
		for (int j = 0; j<= maxFrame; j++) flowRGBAs[j] = 0;
	}
	flowRGBAs[timeStep] = new float[maxPoints*numSeedPoints*numInjections*4];
	mapColors(0, timeStep);
	return flowRGBAs[timeStep];
}



void FlowParams::setFlowMappingDirty(){
	// delete colors and opacities
	// If we are mapping speed, must regenerate flowData
	if ((getOpacMapEntityIndex() == 2)||(getColorMapEntityIndex() == 2)) {
		setFlowDataDirty();
		return;
	}
	if(flowRGBAs){
		for (int i = 0; i<=maxFrame; i++){
			if (flowRGBAs[i]){
				delete flowRGBAs[i];
				flowRGBAs[i] = 0;
			}
		}
	}
	VizWinMgr::getInstance()->refreshFlow(this);
}
void FlowParams::setFlowDataDirty(){
	//The data pointers themselves are the valid flags
	if(flowData){
		for (int i = 0; i<=maxFrame; i++){
			if (flowData[i]) {
				delete flowData[i];
				flowData[i] = 0;
			}
			if (flowRGBAs && flowRGBAs[i]){
				delete flowRGBAs[i];
				flowRGBAs[i] = 0;
			}
		}
	}
	VizWinMgr::getInstance()->refreshFlow(this);
}
//Method to construct Xml for state saving
XmlNode* FlowParams::
buildNode() {
	//Construct the flow node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();

	oss.str(empty);
	oss << (long)numVariables;
	attrs[_numVariablesAttr] = oss.str();

	oss.str(empty);
	oss << (long)numTransforms;
	attrs[_numTransformsAttr] = oss.str();

	oss.str(empty);
	oss << (double)integrationAccuracy;
	attrs[_integrationAccuracyAttr] = oss.str();

	oss.str(empty);
	oss << (double)velocityScale;
	attrs[_velocityScaleAttr] = oss.str();

	oss.str(empty);
	oss << (double)integrationAccuracy;
	attrs[_integrationAccuracyAttr] = oss.str();

	oss.str(empty);
	oss << (long)timeSamplingInterval;
	attrs[_timeSamplingIntervalAttr] = oss.str();

	oss.str(empty);
	oss << (long)instance;
	attrs[_instanceAttr] = oss.str();

	oss.str(empty);
	if (flowType == 0) oss << "true";
	else oss << "false";
	attrs[_steadyFlowAttr] = oss.str();

	oss.str(empty);
	oss << variableNames[varNum[0]]<<" "<<variableNames[varNum[1]]<<" "<<variableNames[varNum[2]];
	attrs[_mappedVariablesAttr] = oss.str();

	XmlNode* flowNode = new XmlNode(_flowParamsTag, attrs, 2);

	//Now add children:  
	//There's a child for geometry and a child for
	//Seeding.
	
	
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

	XmlNode* seedNode = new XmlNode(_seedingTag,attrs,0);
	flowNode->AddChild(seedNode);
	//Now do graphic parameters.  It has one child node for the
	//control points
	
	
	attrs.clear();
	oss.str(empty);
	oss << (long)geometryType;
	attrs[_geometryTypeAttr] = oss.str();
	oss.str(empty);
	oss << (double)objectsPerTimestep;
	attrs[_objectsPerTimestepAttr] = oss.str();
	oss.str(empty);
	oss << (long)firstDisplayFrame <<" "<<(long)lastDisplayFrame;
	attrs[_displayIntervalAttr] = oss.str();
	oss.str(empty);
	oss << (float)shapeDiameter;
	attrs[_shapeDiameterAttr] = oss.str();

	oss.str(empty);
	oss << colorMapEntity[getColorMapEntityIndex()];
	attrs[_colorMappedEntityAttr] = oss.str();
	oss.str(empty);
	oss << (long)numControlPoints;
	attrs[_numControlPointsAttr] = oss.str();

	XmlNode* graphicNode = new XmlNode(_geometryTag,attrs,numControlPoints);
/*
	for (int i = 0; i< numControlPoints; i++){
		attrs.clear();
		oss.str(empty);
		oss << (double)colorControlPoints[i].position;
		attrs[_positionAttr] = oss.str();

		oss.str(empty);
		oss << (double)colorControlPoints[i].hsv[0]<<" "<<(double)colorControlPoints[i].hsv[1]<<" "<<(double)colorControlPoints[i].hsv[2];
		attrs[_hsvAttr] = oss.str();
		XmlNode* colorControlPointNode = new XmlNode(_colorControlPointTag, attrs,0);
		graphicNode->AddChild(colorControlPointNode);
	}
	*/
	flowNode->AddChild(graphicNode);
	return flowNode;
}
//Parse flow params from file:
bool FlowParams::
elementStartHandler(ExpatParseMgr* pm, int  depth, std::string& tagString, const char ** attrs){
	
	//Take care of attributes of flowParamsNode
	if (StrCmpNoCase(tagString, _flowParamsTag) == 0) {
		int newNumVariables = 0;
		//If it's a Flow tag, save 10 attributes (2 are from Params class)
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
			else if (StrCmpNoCase(attribName, _mappedVariablesAttr) == 0) {
				ist >> varNum[0]; ist>>varNum[1]; ist>>varNum[2];
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _steadyFlowAttr) == 0) {
				if (value == "true") setFlowType(0); else setFlowType(1);
			}
			else if (StrCmpNoCase(attribName, _instanceAttr) == 0){
				ist >> instance;
			}
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numTransforms;
			}
			else if (StrCmpNoCase(attribName, _integrationAccuracyAttr) == 0){
				ist >> integrationAccuracy;
			}
			else if (StrCmpNoCase(attribName, _velocityScaleAttr) == 0){
				ist >> velocityScale;
			}
			else if (StrCmpNoCase(attribName, _timeSamplingIntervalAttr) == 0){
				ist >> timeSamplingInterval;
			}
			else return false;
		}
		//Empty out the colorControlPoints:
		//colorControlPoints.empty();
		return true;
	}
	//Parse the seeding node:
	else if (StrCmpNoCase(tagString, _seedingTag) == 0) {
		
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
			else return false;
		}
		return true;
	}
	// Parse the values from geometry:
	else if (StrCmpNoCase(tagString, _geometryTag) == 0) {
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _geometryTypeAttr) == 0) {
				ist >> geometryType;
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
			else if (StrCmpNoCase(attribName, _colorMappedEntityAttr) == 0) {
				int indx;
				ist >> indx;
				setColorMapEntity(indx);
			}
			else if (StrCmpNoCase(attribName, _numControlPointsAttr) == 0) {
				ist >> numControlPoints;
			}
			else return false;
		}
	}
	//Parse a color control point node:
	else if (StrCmpNoCase(tagString, _colorControlPointTag) == 0) {
		//Create a color control point with default values,
		//and with specified position
		//Currently only support HSV colors
		//Repeatedly pull off attribute name and values
		string attribName;
		float hue = 0.f, sat = 1.f, val=1.f, posn=0.f;
		while (*attrs){
			attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _positionAttr) == 0) {
				ist >> posn;
			} else if (StrCmpNoCase(attribName, _hsvAttr) == 0) { 
				ist >> hue;
				ist >> sat;
				ist >> val;
			} else return false; //Unknown attribute
		}
		//Then insert color control point
		//int indx = insertColorControlPoint(posn,hue,sat,val);
		//if (indx >= 0) return true;
		return false;
	}//end of color control point tag
	return true;
}
	
	//static const string _numControlPointsAttr;?? notneeded?
	
	
bool FlowParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	if (StrCmpNoCase(tag, _flowParamsTag) == 0) {
		//If this is a flowparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} 
	else if (StrCmpNoCase(tag, _seedingTag) == 0)
		return true;
	else if (StrCmpNoCase(tag, _geometryTag) == 0)
		return true;
	else if (StrCmpNoCase(tag, _colorControlPointTag) == 0)
		return true;
	else {
		pm->parseError("Unrecognized end tag in FlowParams %s",tag.c_str());
		return false; 
	}
}
	

// Setup pointers between mapper function, editor, and this:
//
void FlowParams::
connectMapperFunction(MapperFunction* tf, MapEditor* tfe){
	tf->setEditor(tfe);
	tfe->setFrame((QFrame*)(myFlowTab->flowMapFrame));
	tfe->setMapperFunction(tf);
	tf->setParams(this);
	tfe->setColorVarNum(getColorMapEntityIndex());
	tfe->setOpacVarNum(getOpacMapEntityIndex());
}
void FlowParams::
setTab(FlowTab* tab) {
	myFlowTab = tab;
	if (mapperFunction)
		mapperFunction->getEditor()->setFrame(myFlowTab->flowMapFrame);
}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the MapEditor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void FlowParams::
updateMapBounds(){
	QString strn;
	if (getMapperFunc()){
		myFlowTab->minOpacmapEdit->setText(strn.setNum(getMapperFunc()->getMinOpacMapValue(),'g',4));
		myFlowTab->maxOpacmapEdit->setText(strn.setNum(getMapperFunc()->getMaxOpacMapValue(),'g',4));
		myFlowTab->minColormapEdit->setText(strn.setNum(getMapperFunc()->getMinColorMapValue(),'g',4));
		myFlowTab->maxColormapEdit->setText(strn.setNum(getMapperFunc()->getMaxColorMapValue(),'g',4));
	} else {
		myFlowTab->minOpacmapEdit->setText("0.0");
		myFlowTab->maxOpacmapEdit->setText("1.0");
		myFlowTab->minColormapEdit->setText("0.0");
		myFlowTab->maxColormapEdit->setText("1.0");
	}

}
//Respond to a change in mapper function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void FlowParams::
guiStartChangeMapFcn(char* str){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	savedCommand = PanelCommand::captureStart(this, str);
}
void FlowParams::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand,this);
	setFlowMappingDirty();
	savedCommand = 0;
}
//Generate a list of colors and opacities, one per (valid) vertex.
//The number of points is maxPoints*numSeedings*numInjections
//Note that, if a variable is mapped, only the first time step is used for
//the mapping
//
void FlowParams::
mapColors(float* speeds, int currentTimeStep){
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
	DataStatus* ds = Session::getInstance()->getDataStatus();
	//Get the variable (entire region) if needed
	if (getOpacMapEntityIndex() > 3){
		//set up args for GetRegion
		//If flow is unsteady, just get the first available timestep
		int timeStep = currentTimeStep;
		if(flowType != 0){//unsteady flow
			timeStep = ds->getFirstTimestep(getOpacMapEntityIndex()-3);
			if (timeStep < 0) MessageReporter::errorMsg("No data for mapped variable");
		}
		size_t minSize[3];
		size_t maxSize[3];
		int bs = Session::getInstance()->getDataMgr()->GetMetadata()->GetBlockSize();
		for (int i = 0; i< 3; i++){
			minSize[i] = 0;
			opacSize[i] = (ds->getFullDataSize(i) >> numTransforms);
			maxSize[i] = opacSize[i]/bs;
			opacVarMin[i] = Session::getInstance()->getDataMgr()->GetMetadata()->GetExtents()[i];
			opacVarMax[i] = Session::getInstance()->getDataMgr()->GetMetadata()->GetExtents()[i+3];
		}
		
		opacRegion = Session::getInstance()->getDataMgr()->GetRegion((size_t)timeStep,
			opacMapEntity[getOpacMapEntityIndex()].c_str(),
			numTransforms, (size_t*) minSize, (size_t*) maxSize, 0);
	}
	if (getColorMapEntityIndex() > 3){
		//set up args for GetRegion
		int timeStep = ds->getFirstTimestep(getColorMapEntityIndex()-3);
		if (timeStep < 0) MessageReporter::errorMsg("No data for mapped variable");
		size_t minSize[3];
		size_t maxSize[3];
		int bs = Session::getInstance()->getDataMgr()->GetMetadata()->GetBlockSize();
		for (int i = 0; i< 3; i++){
			minSize[i] = 0;
			colorSize[i] = (ds->getFullDataSize(i) >> numTransforms);
			maxSize[i] = colorSize[i]/bs;
			colorVarMin[i] = Session::getInstance()->getDataMgr()->GetMetadata()->GetExtents()[i];
			colorVarMax[i] = Session::getInstance()->getDataMgr()->GetMetadata()->GetExtents()[i+3];
		}
		
		colorRegion = Session::getInstance()->getDataMgr()->GetRegion((size_t)timeStep,
			colorMapEntity[getColorMapEntityIndex()].c_str(),
			numTransforms, (size_t*) minSize, (size_t*) maxSize, 0);

	}
	
	//Cycle through all the points.  Map to rgba as we go.
	//
	for (int i = 0; i<numInjections; i++){
		for (int j = 0; j<numSeedPoints; j++){
			for (int k = 0; k<maxPoints; k++) {
				//check for end of flow:
				if (flowData[currentTimeStep][3*(k+ maxPoints*(j+ (numSeedPoints*i)))] == END_FLOW_FLAG)
					break;
				switch (getOpacMapEntityIndex()){
					case (0): //constant
						opacVar = 0.f;
						break;
					case (1): //age
						opacVar = (float)k/(objectsPerTimestep);
						break;
					case (2): //speed
						opacVar = speeds[(k+ maxPoints*(j+ (numSeedPoints*i)))];
						break;
					default : //variable
						int x,y,z;
						float* dataPoint = flowData[currentTimeStep]+3*(k+ maxPoints*(j+ (numSeedPoints*i)));
						x = (int)(dataPoint[0] - opacVarMin[0])*opacSize[0]/(opacVarMax[0]-opacVarMin[0]);
						y = (int)(dataPoint[1] - opacVarMin[1])*opacSize[1]/(opacVarMax[1]-opacVarMin[1]);
						z = (int)(dataPoint[2] - opacVarMin[2])*opacSize[2]/(opacVarMax[2]-opacVarMin[2]);
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
						colorVar = (float)k/(objectsPerTimestep);
						break;
					case (2): //speed
						colorVar = speeds[(k+ maxPoints*(j+ (numSeedPoints*i)))];
						break;
					default : //variable
						int x,y,z;
						float* dataPoint = flowData[currentTimeStep]+3*(k+ maxPoints*(j+ (numSeedPoints*i)));
						x = (int)(dataPoint[0] - colorVarMin[0])*colorSize[0]/(colorVarMax[0]-colorVarMin[0]);
						y = (int)(dataPoint[1] - colorVarMin[1])*colorSize[1]/(colorVarMax[1]-colorVarMin[1]);
						z = (int)(dataPoint[2] - colorVarMin[2])*colorSize[2]/(colorVarMax[2]-colorVarMin[2]);
						colorVar = colorRegion[x+colorSize[0]*(y+colorSize[1]*z)];
						break;
				}
				int colorIndex = (int)((colorVar - colorMin)*255.99/(colorMax-colorMin));
				if (colorIndex<0) colorIndex = 0;
				if (colorIndex> 255) colorIndex =255;
				//Special case for constant colors and/or opacities
				if (getOpacMapEntityIndex() == 0){
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))+3]= constantOpacity;
				} else {
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))+3]= lut[4*colorIndex+3];
				}
				if (getColorMapEntityIndex() == 0){
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))]= ((float)qRed(constantColor))/255.f;
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))+1]= ((float)qGreen(constantColor))/255.f;
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))+2]= ((float)qBlue(constantColor))/255.f;
				} else {
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))]= lut[4*colorIndex];
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))+1]= lut[4*colorIndex+1];
					flowRGBAs[currentTimeStep][4*(k+ maxPoints*(j+ (numSeedPoints*i)))+2]= lut[4*colorIndex+2];
				}
			}
		}
	}
}
int FlowParams::
getColorMapEntityIndex() {
	if (!flowMapEditor) return 0;
	return flowMapEditor->getColorVarNum();
}

int FlowParams::
getOpacMapEntityIndex() {
	if (!flowMapEditor) return 0;
	return flowMapEditor->getOpacVarNum();
}
void FlowParams::
setColorMapEntity( int entityNum){
	if (!flowMapEditor) return;
	flowMapEditor->setColorVarNum(entityNum);
}
void FlowParams::
setOpacMapEntity( int entityNum){
	if (!flowMapEditor) return;
	flowMapEditor->setOpacVarNum(entityNum);
}
//Methods for seed box manipulation... Copied from regionparams.cpp
//Grab a region face:
//
void FlowParams::
captureMouseDown(int faceNum, float camPos[3], float dirVec[3]){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(this,  "slide seed region boundary");
	selectedFaceNum = faceNum;
	faceDisplacement = 0.f;
	//Calculate intersection of ray with specified plane
	if (!rayCubeIntersect(dirVec, camPos, faceNum, initialSelectionRay))
		selectedFaceNum = -1;
	//The selection ray is the vector from the camera to the intersection point,
	//So subtract the camera position
	vsub(initialSelectionRay, camPos, initialSelectionRay);
	setFlowDataDirty();
}

void FlowParams::
captureMouseUp(){
	//If no change, nothing to do, except
	//nullify unDo state
	if (faceDisplacement ==0.f || (selectedFaceNum < 0)) {
		if (savedCommand) {
			delete savedCommand;
			savedCommand = 0;
		}
		
	} else { //terminate dragging
		int coord = (5-selectedFaceNum)/2;
		if (selectedFaceNum %2) {
			setSeedRegionMax(coord, getSeedRegionMax(coord)+faceDisplacement);
		} else {
			setSeedRegionMin(coord, getSeedRegionMin(coord)+faceDisplacement);
		}
		
		enforceConsistency(coord);
		updateDialog();
	}
	faceDisplacement = 0.f;
	selectedFaceNum = -1;
	setFlowDataDirty();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, this);
	savedCommand = 0;
	
}

//Intersect the ray with the specified face, determining the intersection
//in world coordinates.  Note that meaning of faceNum is specified in 
//renderer.cpp
// Faces of the cube are numbered 0..5 based on view from pos z axis:
// back, front, bottom, top, left, right
// return false if no intersection
//
bool FlowParams::
rayCubeIntersect(float ray[3], float cameraPos[3], int faceNum, float intersect[3]){
	double val;
	int coord;// = (5-faceNum)/2;
	// if (faceNum%2) val = getRegionMin(coord); else val = getRegionMax(coord);
	switch (faceNum){
		case(0): //back; z = zmin
			val = getSeedRegionMin(2);
			coord = 2;
			break;
		case(1): //front; z = zmax
			val = getSeedRegionMax(2);
			coord = 2;
			break;
		case(2): //bot; y = min
			val = getSeedRegionMin(1);
			coord = 1;
			break;
		case(3): //top; y = max
			val = getSeedRegionMax(1);
			coord = 1;
			break;
		case(4): //left; x = min
			val = getSeedRegionMin(0);
			coord = 0;
			break;
		case(5): //right; x = max
			val = getSeedRegionMax(0);
			coord = 0;
			break;
		default:
			return false;
	}
	if (ray[coord] == 0.0) return false;
	float param = (val - cameraPos[coord])/ray[coord];
	for (int i = 0; i<3; i++){
		intersect[i] = cameraPos[i]+param*ray[i];
	}
	return true;
}


//Slide the face based on mouse move from previous capture.  
//Requires new direction vector associated with current mouse position
//The new face position requires finding the planar displacement such that 
//the ray (in the scene) associated with the new mouse position is as near
//as possible to the line projected from the original mouse position in the
//direction of planar motion
//displacement from the original intersection is as close as possible to the 
void FlowParams::
slideCubeFace(float movedRay[3]){
	float normalVector[3] = {0.f,0.f,0.f};
	float q[3], r[3], w[3];
	int coord = (5 - selectedFaceNum)/2;
	assert(selectedFaceNum >= 0);
	if (selectedFaceNum < 0) return;
	normalVector[coord] = 1.f;
	
	//Calculate W:
	vcopy(movedRay, w);
	vnormal(w);
	float scaleFactor = 1.f/vdot(w,normalVector);
	//Calculate q:
	vmult(w, scaleFactor, q);
	vsub(q, normalVector, q);
	
	//Calculate R:
	scaleFactor *= vdot(initialSelectionRay, normalVector);
	vmult(w, scaleFactor, r);
	vsub(r, initialSelectionRay, r);

	float denom = vdot(q,q);
	faceDisplacement = 0.f;
	if (denom != 0.f)
		faceDisplacement = -vdot(q,r)/denom;
	
	//Make sure the faceDisplacement is OK.  Not allowed to
	//extent face beyond end of current region, nor beyond opposite face.
	//First, convert to a displacement in cube coordinates.  
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	double regMin = rParams->getRegionMin(coord);
	double regMax = rParams->getRegionMax(coord);
	
	if (selectedFaceNum%2) { //Are we moving min or max?
		//Moving max, since selectedFace is odd:
		if (seedBoxMax[coord] + faceDisplacement > regMax)
			faceDisplacement = regMax - seedBoxMax[coord];
		if (seedBoxMax[coord] + faceDisplacement < regMin)
			faceDisplacement = regMin - seedBoxMax[coord];
	} else { //Moving region min:
		if (seedBoxMin[coord] + faceDisplacement > regMax)
			faceDisplacement = regMax - seedBoxMin[coord];
		if (seedBoxMin[coord] + faceDisplacement < regMin)
			faceDisplacement = regMin - seedBoxMin[coord];
	}
	
}
//Calculate the extents of the seed region when transformed into the unit cube
void FlowParams::
calcSeedExtents(float* extents){
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	
	int i;
	float maxCrd = -1.f;
	
	float regSize[3];
	for (i=0; i<3; i++){
		regSize[i] =  rParams->getFullDataExtent(i+3) - rParams->getFullDataExtent(i);
		if(regSize[i] > maxCrd ) {
			maxCrd = regSize[i];
		}
	}

	for (i = 0; i<3; i++){
		extents[i] = (seedBoxMin[i] - rParams->getFullDataExtent(i))/maxCrd;
		extents[i+3] = (seedBoxMax[i] - rParams->getFullDataExtent(i))/maxCrd;
	}
}
//Force the seed region to fit inside the current Region
bool FlowParams::
enforceConsistency(int i){
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	float regMin = rParams->getRegionMin(i);
	float regMax = rParams->getRegionMax(i);
	bool unchanged = true;
	if (seedBoxMin[i]< regMin) {
		seedBoxMin[i] = regMin;
		unchanged = false;
	}
	if (seedBoxMax[i]> regMax) {
		seedBoxMax[i] = regMax;
		unchanged = false;
	}
	if (seedBoxMin[i]> seedBoxMax[i]) {
		seedBoxMin[i] = seedBoxMax[i];
		unchanged = false;
	}
	return unchanged;
}