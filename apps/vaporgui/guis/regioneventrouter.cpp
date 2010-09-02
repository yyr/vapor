//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regioneventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the RegionEventRouter class.
//		This class supports routing messages from the gui to the params
//		This is derived from the region tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 4996 )
#endif
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qlabel.h>
#include <QFileDialog>
#include "GL/glew.h"
#include "regionparams.h"
#include "regiontab.h"
#include "messagereporter.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "vapor/XmlNode.h"
#include "tabmanager.h"
#include "glutil.h"

#include "regioneventrouter.h"
#include "probeeventrouter.h"
#include "eventrouter.h"
#include "floweventrouter.h"

using namespace VAPoR;


RegionEventRouter::RegionEventRouter(QWidget* parent, const char* ): QWidget(parent), Ui_RegionTab(), EventRouter() {
	setupUi(this);
	myParamsBaseType = VizWinMgr::RegisterEventRouter(Params::_regionParamsTag, this);
	MessageReporter::infoMsg("RegionEventRouter::RegionEventRouter()");
}


RegionEventRouter::~RegionEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new regiontab is created it must be hooked up here
 ************************************************************/
void
RegionEventRouter::hookUpTab()
{
	
	connect (xCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (yCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (zCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (xSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (ySizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (zSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (fullHeightEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	
	connect (fullHeightEdit, SIGNAL( returnPressed()), this, SLOT(regionReturnPressed()));
	connect (xCntrEdit, SIGNAL( returnPressed()), this, SLOT(regionReturnPressed()));
	connect (yCntrEdit, SIGNAL( returnPressed()), this, SLOT(regionReturnPressed()));
	connect (zCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (xSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (ySizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (zSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	
	connect (xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetXCenter(int)));
	connect (yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetYCenter(int)));
	connect (zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetZCenter(int)));
	
	connect (xSizeSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetXSize(int)));
	connect (ySizeSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetYSize(int)));
	connect (zSizeSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetZSize(int)));
	
	connect (setFullRegionButton, SIGNAL(clicked()), this, SLOT (guiSetMaxSize()));
	connect (regionToRakeButton, SIGNAL(clicked()), this, SLOT(copyRegionToRake()));
	connect (regionToProbeButton, SIGNAL(clicked()), this, SLOT(copyRegionToProbe()));
	connect (rakeToRegionButton, SIGNAL(clicked()), this, SLOT(copyRakeToRegion()));
	connect (probeToRegionButton, SIGNAL(clicked()), this, SLOT(copyProbeToRegion()));

	connect (loadRegionsButton, SIGNAL(clicked()), this, SLOT(guiLoadRegionExtents()));
	connect (saveRegionsButton, SIGNAL(clicked()), this, SLOT(saveRegionExtents()));
	connect (adjustExtentsButton, SIGNAL(clicked()), this, SLOT(guiAdjustExtents()));
	connect (refinementCombo, SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (variableCombo, SIGNAL(activated(int)), this, SLOT(guiSetVarNum(int)));
	connect (timestepSpin, SIGNAL(valueChanged(int)), this, SLOT(guiSetTimeStep(int)));

}
/*********************************************************************************
 * Slots associated with RegionTab:
 *********************************************************************************/

void RegionEventRouter::
setRegionTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void RegionEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "edit region text");
	float centerPos[3], regSize[3];
	centerPos[0] = xCntrEdit->text().toFloat();
	centerPos[1] = yCntrEdit->text().toFloat();
	centerPos[2] = zCntrEdit->text().toFloat();
	regSize[0] = xSizeEdit->text().toFloat();
	regSize[1] = ySizeEdit->text().toFloat();
	regSize[2] = zSizeEdit->text().toFloat();

	//Decide whether we are interested in the value of the fullGridHeight:
	
	DataStatus* ds = DataStatus::getInstance();
	bool layered = ds->dataIsLayered();
	
	if (layered) rParams->setFullGridHeight((size_t)(fullHeightEdit->text().toInt()));
	
	for (int i = 0; i<3; i++)
		textToSlider(rParams,i,centerPos[i],regSize[i]);

	refreshRegionInfo(rParams);
	
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	
	PanelCommand::captureEnd(cmd, rParams);
}
void RegionEventRouter::
regionReturnPressed(void){
	confirmText(true);
}


void RegionEventRouter::copyRakeToRegion(){
	guiCopyRakeToRegion();
}
void RegionEventRouter::copyProbeToRegion(){
	guiCopyProbeToRegion();
}
void RegionEventRouter::copyRegionToRake(){
	//Need to find relevant Flowparams, make it update with this region.
	
	FlowEventRouter* fer = VizWinMgr::getInstance()->getFlowRouter();
	fer->guiSetRakeToRegion();
}
void RegionEventRouter::copyRegionToProbe(){
	//Need to find relevant Probe, make it update with this region.
	//Currently there is no probe router
	ProbeEventRouter* per = VizWinMgr::getInstance()->getProbeRouter();
	per->guiCopyRegionToProbe();
}

//Insert values from params into tab panel
//
void RegionEventRouter::updateTab(){
	if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	float* regionMin = rParams->getRegionMin(timestep);
	float* regionMax = rParams->getRegionMax(timestep);
	Session::getInstance()->blockRecording();
	for (int i = 0; i< 3; i++){
		textToSlider(rParams, i, (regionMin[i]+regionMax[i])*0.5f,
			regionMax[i]-regionMin[i]);
	}
	xSizeEdit->setText(QString::number(regionMax[0]-regionMin[0],'g', 4));
	xCntrEdit->setText(QString::number(0.5f*(regionMax[0]+regionMin[0]),'g',5));
	ySizeEdit->setText(QString::number(regionMax[1]-regionMin[1],'g', 4));
	yCntrEdit->setText(QString::number(0.5f*(regionMax[1]+regionMin[1]),'g',5));
	zSizeEdit->setText(QString::number(regionMax[2]-regionMin[2],'g', 4));
	zCntrEdit->setText(QString::number(0.5f*(regionMax[2]+regionMin[2]),'g',5));
	
	bool layered = false;
	DataStatus* ds = DataStatus::getInstance();
    DataMgr	*dataMgr = ds->getDataMgr();

	if (dataMgr) {
		layered = ds->dataIsLayered();
		if (layered)
			fullHeightEdit->setText(QString::number(rParams->getFullGridHeight()));
		else {
			size_t dims[3];
			dataMgr->GetDim(dims, -1);
			fullHeightEdit->setText(QString::number(dims[2]));
		}
	}
	else fullHeightEdit->setText("0");
	fullHeightEdit->setEnabled(layered && (dataMgr != 0));
	
	
	if (rParams->isLocal())
		LocalGlobal->setCurrentIndex(1);
	else 
		LocalGlobal->setCurrentIndex(0);
	refreshRegionInfo(rParams);

	//Provide latlon box extents if available:
	if (DataStatus::getProjectionString().size() == 0){
		minMaxLonLatFrame->hide();
	} else {
		double boxLatLon[4];
		boxLatLon[0] = regionMin[0];
		boxLatLon[1] = regionMin[1];
		boxLatLon[2] = regionMax[0];
		boxLatLon[3] = regionMax[1];
		int timeStep = timestepSpin->value();
		if (DataStatus::convertToLatLon(timeStep,boxLatLon,2)){
			minLonLabel->setText(QString::number(boxLatLon[0]));
			minLatLabel->setText(QString::number(boxLatLon[1]));
			maxLonLabel->setText(QString::number(boxLatLon[2]));
			maxLatLabel->setText(QString::number(boxLatLon[3]));
			minMaxLonLatFrame->show();
		} else {
			minMaxLonLatFrame->hide();
		}
	}
	guiSetTextChanged(false);

    relabel();
	
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}


// Update the axes labels
//
void RegionEventRouter::relabel()
{
  if (Session::getInstance()->sphericalTransform())
  {
    QString labels[3] = {"Lon", "Lat", "Rad"};
    const DataMgr *dataMgr = Session::getInstance()->getDataMgr();
    const vector<long> &permutation = dataMgr->GetGridPermutation();

    xCenterLabel->setText(labels[permutation[0]]);
    yCenterLabel->setText(labels[permutation[1]]);
    zCenterLabel->setText(labels[permutation[2]]);
    
    xSizeLabel->setText(labels[permutation[0]] + " Size");
    ySizeLabel->setText(labels[permutation[1]] + " Size");
    zSizeLabel->setText(labels[permutation[2]] + " Size");
    
    xUserLabel->setText("User " + labels[permutation[0]]);
    yUserLabel->setText("User " + labels[permutation[1]]);
    zUserLabel->setText("User " + labels[permutation[2]]);
    
                          
    xVoxelLabel->setText("Voxel " + labels[permutation[0]]);
    yVoxelLabel->setText("Voxel " + labels[permutation[1]]);
    zVoxelLabel->setText("Voxel " + labels[permutation[2]]);
  }
  else
  {
    xCenterLabel->setText("X");
    yCenterLabel->setText("Y");
    zCenterLabel->setText("Z");

    xSizeLabel->setText("X Size");
    ySizeLabel->setText("Y Size");
    zSizeLabel->setText("Z Size");

    xUserLabel->setText("User X");
    yUserLabel->setText("User Y");
    zUserLabel->setText("User Z");

    xVoxelLabel->setText("Voxel X");
    yVoxelLabel->setText("Voxel Y");
    zVoxelLabel->setText("Voxel Z");
  }
}

//Set slider position, based on text change. 
//
void RegionEventRouter::
textToSlider(RegionParams* rp, int coord, float newCenter, float newSize){
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	bool centerChanged = false;
	bool sizeChanged = false;
	Session* ses = Session::getInstance();
	const float* extents; 
	float regMin = 0.f;
	float regMax = 1.f;
	if (ses){
		extents = ses->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	}
	
	if (newSize > regMax-regMin){
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
	
	rp->setRegionMin(coord, newCenter - newSize*0.5f,timestep); 
	rp->setRegionMax(coord,newCenter + newSize*0.5f,timestep); 
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = xSizeSlider->value();
			oldSliderCenter = xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				xSizeSlider->setValue(sliderSize);
			if(sizeChanged) xSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				xCenterSlider->setValue(sliderCenter);
			if(centerChanged) xCntrEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = ySizeSlider->value();
			oldSliderCenter = yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				ySizeSlider->setValue(sliderSize);
			if(sizeChanged) ySizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				yCenterSlider->setValue(sliderCenter);
			if(centerChanged) yCntrEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = zSizeSlider->value();
			oldSliderCenter = zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				zSizeSlider->setValue(sliderSize);
			if(sizeChanged) zSizeEdit->setText(QString::number(newSize));
			
			if (oldSliderCenter != sliderCenter)
				zCenterSlider->setValue(sliderCenter);
			if(centerChanged) zCntrEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	
	guiSetTextChanged(false);
	update();
	return;
}
//Set text when a slider changes.
//Move the center if the size is too big
void RegionEventRouter::
sliderToText(RegionParams* rp, int coord, int slideCenter, int slideSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	Session* ses = Session::getInstance();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	const float* extents; 
	float regMin = 0.f;
	float regMax = 1.f;
	if (ses){
		extents = ses->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	}
	
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
	rp->setRegionMin(coord,newCenter - newSize*0.5f,timestep); 
	rp->setRegionMax(coord,newCenter + newSize*0.5f,timestep); 
	
	int newSliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	//Always need to change text.  Possibly also change slider if it was moved
	switch(coord) {
		case 0:
			if (sliderChanged) 
				xCenterSlider->setValue(newSliderCenter);
			xSizeEdit->setText(QString::number(newSize));
			xCntrEdit->setText(QString::number(newCenter));
			break;
		case 1:
			if (sliderChanged) 
				yCenterSlider->setValue(newSliderCenter);
			ySizeEdit->setText(QString::number(newSize));
			yCntrEdit->setText(QString::number(newCenter));
			break;
		case 2:
			if (sliderChanged) 
				zCenterSlider->setValue(newSliderCenter);
			zSizeEdit->setText(QString::number(newSize));
			zCntrEdit->setText(QString::number(newCenter));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	update();
	//force a new render with new flow data
	VizWinMgr::getInstance()->setVizDirty(rp, RegionBit, true);
	return;
}	
void RegionEventRouter::
refreshRegionInfo(RegionParams* rParams){
	//First setup the timestep & refinement components
	//Make sure that the refinement levels are valid for the specified timestep.
	//If not, correct them.  If there is no data at specified timestep,
	//Then don't show anything in refinementCombo
	int mdVarNum = variableCombo->currentIndex();
	//This index is only relevant to metadata numbering
	Session* ses = Session::getInstance();
	DataStatus* ds= DataStatus::getInstance();
	if (!ds->getDataMgr()) ds = 0;
	int timeStep = timestepSpin->value();
	//Distinguish between the actual data available and the numtransforms
	//in the metadata.  If the data isn't there, we will display blanks
	//in the "selected" area.
	int maxRefLevel = 10;
	
	int varNum = -1;
	std::string varName;
	bool is3D = false;
	int orientation = -1;
	if (ds ) {
		const DataMgr* dataMgr = ds->getDataMgr();
		varName = dataMgr->GetVariableNames()[mdVarNum];
		vector<string> varnames = dataMgr->GetVariables3D();
		for (int i = 0; i< varnames.size(); i++){
			if (varName == varnames[i]) {
				is3D = true;
				break;
			}
		}
		if (is3D){
			varNum = ds->getSessionVariableNum(varName);
			maxRefLevel = ds->maxXFormPresent(varNum, timeStep);
		}
		else {
			int mdVarnum = ds->getActiveVarNum2D(varName);
			orientation = ds->get2DOrientation(mdVarnum);
			varNum = ds->getSessionVariableNum2D(varName);
			maxRefLevel = ds->maxXFormPresent2D(varNum, timeStep);
		}
		
	} 

	int refLevel = refinementCombo->currentIndex();
	
	const float* fullDataExtents = ses->getExtents();

	fullMinXLabel->setText(QString::number(fullDataExtents[0], 'g',4));
	fullMinYLabel->setText(QString::number(fullDataExtents[1], 'g',4));
	fullMinZLabel->setText(QString::number(fullDataExtents[2], 'g',4));
	fullMaxXLabel->setText(QString::number(fullDataExtents[3], 'g',4));
	fullMaxYLabel->setText(QString::number(fullDataExtents[4], 'g',4));
	fullMaxZLabel->setText(QString::number(fullDataExtents[5], 'g',4));
	fullSizeXLabel->setText(QString::number(fullDataExtents[3]-fullDataExtents[0], 'g',4));
	fullSizeYLabel->setText(QString::number(fullDataExtents[4]-fullDataExtents[1], 'g',4));
	fullSizeZLabel->setText(QString::number(fullDataExtents[5]-fullDataExtents[2], 'g',4));

	minXFullLabel->setText(QString::number(fullDataExtents[0],'g',4));
	minYFullLabel->setText(QString::number(fullDataExtents[1],'g',4));
	minZFullLabel->setText(QString::number(fullDataExtents[2],'g',4));
	maxXFullLabel->setText(QString::number(fullDataExtents[3],'g',4));
	maxYFullLabel->setText(QString::number(fullDataExtents[4],'g',4));
	maxZFullLabel->setText(QString::number(fullDataExtents[5],'g',4));
	
	//For now, the min and max var extents are the whole thing:

	float* regionMin = rParams->getRegionMin(timeStep);
	float* regionMax = rParams->getRegionMax(timeStep);

	minXSelectedLabel->setText(QString::number(regionMin[0],'g',4));
	minYSelectedLabel->setText(QString::number(regionMin[1],'g',4));
	minZSelectedLabel->setText(QString::number(regionMin[2],'g',4));
	maxXSelectedLabel->setText(QString::number(regionMax[0],'g',4));
	maxYSelectedLabel->setText(QString::number(regionMax[1],'g',4));
	maxZSelectedLabel->setText(QString::number(regionMax[2],'g',4));


	//Now produce the corresponding voxel coords:
	size_t min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	
	// if region isn't valid just don't show the bounds:
	if (ds && refLevel <= maxRefLevel){
		rParams->getRegionVoxelCoords(refLevel,min_dim,max_dim,min_bdim,max_bdim,timeStep);
		minXVoxSelectedLabel->setText(QString::number(min_dim[0]));
		minYVoxSelectedLabel->setText(QString::number(min_dim[1]));
		minZVoxSelectedLabel->setText(QString::number(min_dim[2]));
		maxXVoxSelectedLabel->setText(QString::number(max_dim[0]));
		maxYVoxSelectedLabel->setText(QString::number(max_dim[1]));
		maxZVoxSelectedLabel->setText(QString::number(max_dim[2]));
	} else { 
		minXVoxSelectedLabel->setText("");
		minYVoxSelectedLabel->setText("");
		minZVoxSelectedLabel->setText("");
		maxXVoxSelectedLabel->setText("");
		maxYVoxSelectedLabel->setText("");
		maxZVoxSelectedLabel->setText("");
	}
	if (refLevel > maxRefLevel) refLevel = 0;
	if (ds){
		//Entire region size in voxels
		for (int i = 0; i<3; i++){
			max_dim[i] = ds->getFullSizeAtLevel(refLevel,i) - 1;
	
		}
	}
	minXVoxFullLabel->setText("0");
	minYVoxFullLabel->setText("0");
	minZVoxFullLabel->setText("0");
	maxXVoxFullLabel->setText(QString::number(max_dim[0]));
	maxYVoxFullLabel->setText(QString::number(max_dim[1]));
	maxZVoxFullLabel->setText(QString::number(max_dim[2]));

	//Find the variable voxel limits:
	size_t min_vdim[3], max_vdim[3];
	
	double var_ext[6];
	int rc = -1;
	if (ds && refLevel <= maxRefLevel )
		rc = RegionParams::getValidRegion(timeStep, varName.c_str(),
			refLevel, min_vdim, max_vdim);
	if (rc>= 0)	{
		minXVoxVarLabel->setText(QString::number(min_vdim[0]));
		minYVoxVarLabel->setText(QString::number(min_vdim[1]));
		minZVoxVarLabel->setText(QString::number(min_vdim[2]));
		maxXVoxVarLabel->setText(QString::number(max_vdim[0]));
		maxYVoxVarLabel->setText(QString::number(max_vdim[1]));
		maxZVoxVarLabel->setText(QString::number(max_vdim[2]));
		  
		ds->mapVoxelToUserCoords(refLevel, min_vdim, var_ext);
		ds->mapVoxelToUserCoords(refLevel, max_vdim, var_ext+3);
		//Use full extents if variable is at extremes, to avoid confusion...
		for (int k = 0; k<3; k++){
			if (min_vdim[k] == 0) var_ext[k] = fullDataExtents[k];
			if (max_vdim[k] == max_dim[k]) var_ext[k+3] = fullDataExtents[k+3];
		}
		//Calculate fraction of extents:
		minVarXLabel->setText(QString::number(var_ext[0],'g',4));
		minVarYLabel->setText(QString::number(var_ext[1],'g',4));
		minVarZLabel->setText(QString::number(var_ext[2],'g',4));
		maxVarXLabel->setText(QString::number(var_ext[3],'g',4));
		maxVarYLabel->setText(QString::number(var_ext[4],'g',4));
		maxVarZLabel->setText(QString::number(var_ext[5],'g',4));
	} else {
		minXVoxVarLabel->setText("");
		minYVoxVarLabel->setText("");
		minZVoxVarLabel->setText("");
		maxXVoxVarLabel->setText("");
		maxYVoxVarLabel->setText("");
		maxZVoxVarLabel->setText("");
		minVarXLabel->setText("");
		minVarYLabel->setText("");
		minVarZLabel->setText("");
		maxVarXLabel->setText("");
		maxVarYLabel->setText("");
		maxVarZLabel->setText("");
	}

	switch (orientation){
		case 0 : 
			minVarXLabel->setText("-");
			maxVarXLabel->setText("-");
			minXVoxVarLabel->setText("-");
			maxXVoxVarLabel->setText("-");
			minXVoxSelectedLabel->setText("-");
			maxXVoxSelectedLabel->setText("-");
			minXSelectedLabel->setText("-");
			maxXSelectedLabel->setText("-");
			break;
		case 1 : 
			minVarYLabel->setText("-");
			maxVarYLabel->setText("-");
			minYVoxVarLabel->setText("-");
			maxYVoxVarLabel->setText("-");
			minYVoxSelectedLabel->setText("-");
			maxYVoxSelectedLabel->setText("-");
			minYSelectedLabel->setText("-");
			maxYSelectedLabel->setText("-");
			break;
		case 2 : 
			minVarZLabel->setText("-");
			maxVarZLabel->setText("-");
			minZVoxVarLabel->setText("-");
			maxZVoxVarLabel->setText("-");
			minZVoxSelectedLabel->setText("-");
			maxZVoxSelectedLabel->setText("-");
			minZSelectedLabel->setText("-");
			maxZSelectedLabel->setText("-");
			break;
		default:
			break;
	}
	size_t bs[3] = {32,32,32};
	if (ds && ds->getDataMgr())
		ds->getDataMgr()->GetBlockSize(bs, refLevel);
	//Size needed for data assumes blocksize = 2**5, 6 bytes per voxel, times 2.
	float newFullMB;
	if (is3D)
		newFullMB = (float)(bs[0]*bs[1]*bs[2]*(max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1));
	else {
		//get other coords:
		int crd0 = 0, crd1 = 1;
		if (orientation < 2) crd1++;
		if (orientation < 1) crd0++;
		newFullMB = (float)(bs[0]*bs[1]*(max_bdim[crd0]-min_bdim[crd0]+1)*(max_bdim[crd1]-min_bdim[crd1]+1));
	}
	
	//divide by 1 million for megabytes, mult by 4 for 4 bytes per voxel:
	newFullMB /= 262144.f;

	
	selectedDataSizeLabel->setText(QString::number(newFullMB,'g',8));
}

//Methods in support of undo/redo:
//
//Make region match probe.  Responds to button in region panel
void RegionEventRouter::
guiCopyProbeToRegion(){
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "copy probe to region");
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_probeParamsTag);
	if (pParams->getPhi() != 0.f ||
		pParams->getTheta() != 0.f ||
		pParams->getPsi() != 0.f) {
			MessageReporter::warningMsg("Note: current probe is rotated.\n%s",
				"Copied region is the smallest \naxis-aligned box that contains the probe");
	}
	float regMin[3], regMax[3];
	pParams->getContainingRegion(regMin, regMax);
	rParams->setBox(regMin, regMax,timestep);
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	PanelCommand::captureEnd(cmd,rParams);
	
}
//Make region match rake.  Responds to button in region panel
void RegionEventRouter::
guiCopyRakeToRegion(){
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "copy rake to region");
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_flowParamsTag);
	float boxmin[3],boxmax[3];
	fParams->getBox(boxmin, boxmax, timestep);
	rParams->setBox(boxmin, boxmax, timestep);
	
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	PanelCommand::captureEnd(cmd,rParams);
	
}
void RegionEventRouter::
guiSetVarNum(int n){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "set variable");
	rParams->setInfoVarNum(n);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);

}
void RegionEventRouter::guiSetTimeStep(int n){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "set time step");
	rParams->setInfoTimeStep(n);
	PanelCommand::captureEnd(cmd, rParams);
	updateTab();
}
void RegionEventRouter::
guiSetNumRefinements(int n){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "set number of Refinements");
	rParams->setInfoNumRefinements(n);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
}


//Move the region center to specified coords, shrink it if necessary
void RegionEventRouter::
guiSetCenter(const float* coords){
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "move region center");
	const float* extents = Session::getInstance()->getExtents();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	float boxmin[3], boxmax[3];
	rParams->getBox(boxmin,boxmax,timestep);
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float fullMin = extents[i];
		float fullMax = extents[i+3];
		if (coord < fullMin) coord = fullMin;
		if (coord > fullMax) coord = fullMax;
		float regSize = boxmax[i]-boxmin[i];
		if (coord + 0.5f*regSize > fullMax) regSize = 2.f*(fullMax - coord);
		if (coord - 0.5f*regSize < fullMin) regSize = 2.f*(coord - fullMin);
		boxmax[i] = coord + 0.5f*regSize;
		boxmin[i] = coord - 0.5f*regSize;
	}
	rParams->setBox(boxmin, boxmax, timestep);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setRegionDirty(rParams);
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region X center");
	
	sliderToText(rParams, 0, sliderval, xSizeSlider->value());
	
	PanelCommand::captureEnd(cmd, rParams);
	refreshRegionInfo(rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	VizWinMgr::getInstance()->setRegionDirty(rParams);
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Y center");
	sliderToText(rParams, 1, sliderval, ySizeSlider->value());
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Z center");
	sliderToText(rParams, 2, sliderval, zSizeSlider->value());
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	
}

void RegionEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region X size");
	sliderToText(rParams, 0, xCenterSlider->value(),sliderval);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}
void RegionEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Y size");
	sliderToText(rParams, 1, yCenterSlider->value(),sliderval);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}
void RegionEventRouter::
guiSetZSize(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Z size");
	sliderToText(rParams, 2, zCenterSlider->value(),sliderval);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}

void RegionEventRouter::
guiSetMaxSize(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "change region size to max");
	const float* fullDataExtents = Session::getInstance()->getExtents();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	rParams->setBox(fullDataExtents, fullDataExtents+3, timestep);
	
	updateTab();
	PanelCommand::captureEnd(cmd, rParams);

	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}

//Reinitialize region tab settings, session has changed
//Need to force the regionMin, regionMax to be OK.
void RegionEventRouter::
reinitTab(bool doOverride){
	int i;
	const DataMgr *dataMgr = Session::getInstance()->getDataMgr();
	
	//Set up the combo boxes in the gui based on info in the session:
	const vector<string>& varNames = dataMgr->GetVariableNames();
	variableCombo->clear();
	for (i = 0; i<(int)varNames.size(); i++)
		variableCombo->addItem(varNames[i].c_str());
	
	int mints = DataStatus::getInstance()->getMinTimestep();
	int maxts = DataStatus::getInstance()->getMaxTimestep();

	timestepSpin->setMinimum(mints);
	timestepSpin->setMaximum(maxts);

	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	if (VizWinMgr::getInstance()->getNumVisualizers() > 1) LocalGlobal->setEnabled(true);
	else LocalGlobal->setEnabled(false);
}



//Save undo/redo state when user grabs a rake handle
//
void RegionEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(rParams,  "slide region handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshRegion(rParams);
}
void RegionEventRouter::
captureMouseUp(){
	//Update the tab:
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	if (!savedCommand) return;
	updateTab();
	PanelCommand::captureEnd(savedCommand, rParams);
	//Set region  dirty
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	savedCommand = 0;
	//Force a rerender:
	VizWinMgr::getInstance()->refreshRegion(rParams);
	
}
void RegionEventRouter::
makeCurrent(Params* prev, Params* nextParams ,bool, int,bool) {
	RegionParams* rParams = (RegionParams*) nextParams;
	int vizNum = rParams->getVizNum();
	VizWinMgr::getInstance()->setRegionParams(vizNum, rParams);
	//Also update current Tab.  It's probably visible.
	//
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams,RegionBit,true);
}
void RegionEventRouter::
guiLoadRegionExtents(){
	//load a list of
	//region extents.   The previous default box is used if the
	//list does not specify the extents at a timestep.
	confirmText(false);
	//Launch a file-open dialog
	QString filename = QFileDialog::getOpenFileName(this,
        	"Specify file name for loading list of time-varying Region extents", 
		Session::getInstance()->getFlowDirectory().c_str(),
        	"Text files (*.txt)");
	//Check that user did specify a file:
	if (filename.isNull()) {
		return;
	}
	
	//Open the file:
	FILE* regionFile = fopen((const char*)filename.toAscii(),"r");
	if (!regionFile){
		MessageReporter::errorMsg("Region Load Error;\nUnable to open file %s",(const char*)filename.toAscii());
		return;
	}
	//File is OK, save the state in command queue
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "read regions file");

	const float* fullExtents = DataStatus::getInstance()->getExtents();
	//Read the file
	std::map<int,float*>& extentsMapping = rParams->getExtentsMapping();
	int numregions = 0;
	
	while (1){
		int ts;
		float* exts = new float[6];
		int numVals = fscanf(regionFile, "%d %g %g %g %g %g %g", &ts,
			exts, exts+1, exts+2, exts+3, exts+4, exts+5);
			
		if (numVals < 7) {
			if (numVals > 0 && numregions > 0){
				MessageReporter::warningMsg("Invalid region extents at timestep %d",ts);
			}
			delete exts;
			break;
		}
		numregions++;
		bool ok = true;
		//Force it to be valid:
		for (int i = 0; i< 3; i++){
			if (exts[i] < fullExtents[i]) { ok = false; exts[i] = fullExtents[i];}
			if (exts[i+3] > fullExtents[i+3]){ok = false; exts[i+3] = fullExtents[i+3]; }
			if (exts[i] > exts[i+3]) { ok = false; exts[i+3] = exts[i];}
		}
		if (!ok){
			MessageReporter::warningMsg("Repaired region extents at timestep %d",ts);
		}
		extentsMapping[ts] = exts;

	}
	if (numregions == 0) {
		MessageReporter::errorMsg("Region Load Error;\nNo valid region extents in file %s",(const char*)filename.toAscii());
		delete cmd;
		fclose(regionFile);
		return;
	}

	
	PanelCommand::captureEnd(cmd, rParams);
	
	fclose(regionFile);
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams,RegionBit,true);
}
void RegionEventRouter::
saveRegionExtents(){
	//save the list of
	//region extents.   
	//Timesteps that have not been modified do not get saved.
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);

	std::map<int,float*>& extentsMapping = rParams->getExtentsMapping();
	//Are there any extents to write?
	if (extentsMapping.size() == 0) {
		MessageReporter::errorMsg("No time-varying region extents to write to file");
		return;
	}
	//Launch a file-open dialog
	QString filename = QFileDialog::getSaveFileName(this,
        	"Specify file name for saving list of current time-varying Region extents",
		Session::getInstance()->getFlowDirectory().c_str(),
        	"Text files (*.txt)");
	//Check that user did specify a file:
	if (filename.isNull()) {
		return;
	}
	
	//Open the file:
	FILE* regionFile = fopen((const char*)filename.toAscii(),"w");
	if (!regionFile){
		MessageReporter::errorMsg("Region Save Error;\nUnable to open file %s",(const char*)filename.toAscii());
		return;
	}
	
	map <int, float*>::iterator s =  extentsMapping.begin();
	//write the extents:
	while (s != extentsMapping.end()){
		int timestep = s->first;
		float* exts = s->second;
		int rc = fprintf(regionFile, "%d %g %g %g %g %g %g\n",
			timestep, exts[0], exts[1],exts[2], exts[3],exts[4], exts[5]);
		if (rc < 7) {
			MessageReporter::errorMsg("Error writing region extents at timestep %d",timestep);
			fclose(regionFile);
			return;
		}
		s++;
	}
	
	fclose(regionFile);
}
void RegionEventRouter::
guiAdjustExtents(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "adjust current extents");
	//Set the current extents in the extents list
	map<int,float*>& extentsMap = rParams->getExtentsMapping();
	map <int, float*> :: const_iterator extsIter;
	extsIter = extentsMap.find( timestep);
	if ( extsIter != extentsMap.end()) {
		//no change
		delete cmd;
		return;
	}
	//set the extents at the current timestep:
	float* newExtents = new float[6];
	float* extents = rParams->getRegionExtents(timestep);
	for (int i = 0; i< 6; i++) newExtents[i] = extents[i];
	extentsMap[timestep] = newExtents;
	PanelCommand::captureEnd(cmd, rParams);
}
