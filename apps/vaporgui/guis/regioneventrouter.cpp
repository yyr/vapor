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
#pragma warning( disable : 4100 )
#endif
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qworkspace.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlistbox.h>
#include "regionparams.h"
#include "regiontab.h"
#include "floweventrouter.h"
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
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"

#include "regioneventrouter.h"
#include "probeeventrouter.h"
#include "eventrouter.h"

using namespace VAPoR;


RegionEventRouter::RegionEventRouter(QWidget* parent, const char* name): RegionTab(parent,name), EventRouter() {
	myParamsType = Params::RegionParamsType;
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
	
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionZCenter()));
	
	connect (xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionXSize()));
	connect (ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionYSize()));
	connect (zSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionZSize()));
	
	connect (setFullRegionButton, SIGNAL(clicked()), this, SLOT (guiSetMaxSize()));
	connect (regionToRakeButton, SIGNAL(clicked()), this, SLOT(copyRegionToRake()));
	connect (regionToProbeButton, SIGNAL(clicked()), this, SLOT(copyRegionToProbe()));
	connect (rakeToRegionButton, SIGNAL(clicked()), this, SLOT(copyRakeToRegion()));
	connect (probeToRegionButton, SIGNAL(clicked()), this, SLOT(copyProbeToRegion()));

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
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
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


/*
 * Respond to a release of slider 
 */
void RegionEventRouter::
setRegionXCenter(){
	guiSetXCenter(xCenterSlider->value());
}
void RegionEventRouter::
setRegionYCenter(){
	guiSetYCenter(yCenterSlider->value());
}
void RegionEventRouter::
setRegionZCenter(){
	guiSetZCenter(zCenterSlider->value());
}

/*
 * Respond to a slider release
 */
void RegionEventRouter::
setRegionXSize(){
	guiSetXSize(xSizeSlider->value());
}
void RegionEventRouter::
setRegionYSize(){
	guiSetYSize(ySizeSlider->value());
}
void RegionEventRouter::
setRegionZSize(){
	guiSetZSize(zSizeSlider->value());
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

	float* regionMin = rParams->getRegionMin();
	float* regionMax = rParams->getRegionMax();
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
	const VDFIOBase* reader = ds->getRegionReader();
	
	if (reader) {
		layered = ds->dataIsLayered();
		if (layered)
			fullHeightEdit->setText(QString::number(rParams->getFullGridHeight()));
		else {
			size_t dims[3];
			reader->GetDim(dims, -1);
			fullHeightEdit->setText(QString::number(dims[2]));
		}
	}
	else fullHeightEdit->setText("0");
	fullHeightEdit->setEnabled(layered && (reader != 0));
	
	
	if (rParams->isLocal())
		LocalGlobal->setCurrentItem(1);
	else 
		LocalGlobal->setCurrentItem(0);
	refreshRegionInfo(rParams);
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
    const Metadata *metadata = Session::getInstance()->getCurrentMetadata();
    const vector<long> &permutation = metadata->GetGridPermutation();

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
	
	rp->setRegionMin(coord, newCenter - newSize*0.5f); 
	rp->setRegionMax(coord,newCenter + newSize*0.5f); 
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
	rp->setRegionMin(coord,newCenter - newSize*0.5f); 
	rp->setRegionMax(coord,newCenter + newSize*0.5f); 
	
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
	size_t bs = 32;
	int mdVarNum = variableCombo->currentItem();
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
		const Metadata* md = ds->getCurrentMetadata();
		varName = md->GetVariableNames()[mdVarNum];
		vector<string> varnames = md->GetVariables3D();
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
			int mdVarnum = ds->getMetadataVarNum2D(varName);
			orientation = ds->get2DOrientation(mdVarnum);
			varNum = ds->getSessionVariableNum2D(varName);
			maxRefLevel = ds->maxXFormPresent2D(varNum, timeStep);
		}
		
	} 

	int refLevel = refinementCombo->currentItem();
	
	const float* fullDataExtents = ses->getExtents();

	fullMinXLabel->setText(QString::number(fullDataExtents[0], 'g', 5));
	fullMinYLabel->setText(QString::number(fullDataExtents[1], 'g', 5));
	fullMinZLabel->setText(QString::number(fullDataExtents[2], 'g', 5));
	fullMaxXLabel->setText(QString::number(fullDataExtents[3], 'g', 5));
	fullMaxYLabel->setText(QString::number(fullDataExtents[4], 'g', 5));
	fullMaxZLabel->setText(QString::number(fullDataExtents[5], 'g', 5));
	fullSizeXLabel->setText(QString::number(fullDataExtents[3]-fullDataExtents[0], 'g', 5));
	fullSizeYLabel->setText(QString::number(fullDataExtents[4]-fullDataExtents[1], 'g', 5));
	fullSizeZLabel->setText(QString::number(fullDataExtents[5]-fullDataExtents[2], 'g', 5));

	minXFullLabel->setText(QString::number(fullDataExtents[0],'g',5));
	minYFullLabel->setText(QString::number(fullDataExtents[1],'g',5));
	minZFullLabel->setText(QString::number(fullDataExtents[2],'g',5));
	maxXFullLabel->setText(QString::number(fullDataExtents[3],'g',5));
	maxYFullLabel->setText(QString::number(fullDataExtents[4],'g',5));
	maxZFullLabel->setText(QString::number(fullDataExtents[5],'g',5));
	
	//For now, the min and max var extents are the whole thing:

	float* regionMin = rParams->getRegionMin();
	float* regionMax = rParams->getRegionMax();

	minXSelectedLabel->setText(QString::number(regionMin[0],'g',5));
	minYSelectedLabel->setText(QString::number(regionMin[1],'g',5));
	minZSelectedLabel->setText(QString::number(regionMin[2],'g',5));
	maxXSelectedLabel->setText(QString::number(regionMax[0],'g',5));
	maxYSelectedLabel->setText(QString::number(regionMax[1],'g',5));
	maxZSelectedLabel->setText(QString::number(regionMax[2],'g',5));


	//Now produce the corresponding voxel coords:
	size_t min_dim[3], max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	
	// if region isn't valid just don't show the bounds:
	if (ds && refLevel <= maxRefLevel){
		rParams->getRegionVoxelCoords(refLevel,min_dim,max_dim,min_bdim,max_bdim);
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
		minVarXLabel->setText(QString::number(var_ext[0],'g',5));
		minVarYLabel->setText(QString::number(var_ext[1],'g',5));
		minVarZLabel->setText(QString::number(var_ext[2],'g',5));
		maxVarXLabel->setText(QString::number(var_ext[3],'g',5));
		maxVarYLabel->setText(QString::number(var_ext[4],'g',5));
		maxVarZLabel->setText(QString::number(var_ext[5],'g',5));
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
	if (ds && ds->getCurrentMetadata())
		bs = *(ds->getCurrentMetadata()->GetBlockSize());
	//Size needed for data assumes blocksize = 2**5, 6 bytes per voxel, times 2.
	float newFullMB;
	if (is3D)
		newFullMB = (float)(bs*bs*bs*(max_bdim[0]-min_bdim[0]+1)*(max_bdim[1]-min_bdim[1]+1)*(max_bdim[2]-min_bdim[2]+1));
	else {
		//get other coords:
		int crd0 = 0, crd1 = 1;
		if (orientation < 2) crd1++;
		if (orientation < 1) crd0++;
		newFullMB = (float)(bs*bs*(max_bdim[crd0]-min_bdim[crd0]+1)*(max_bdim[crd1]-min_bdim[crd1]+1));
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
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "copy probe to region");
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	if (pParams->getPhi() != 0.f ||
		pParams->getTheta() != 0.f ||
		pParams->getPsi() != 0.f) {
			MessageReporter::warningMsg("Note: current probe is rotated.\n%s",
				"Copied region is the smallest \naxis-aligned box that contains the probe");
	}
	float regMin[3], regMax[3];
	pParams->getContainingRegion(regMin, regMax);
	rParams->setBox(regMin, regMax);
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	PanelCommand::captureEnd(cmd,rParams);
	
}
//Make region match rake.  Responds to button in region panel
void RegionEventRouter::
guiCopyRakeToRegion(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "copy rake to region");
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::FlowParamsType);
	for (int i = 0; i< 3; i++){
		rParams->setRegionMin(i,fParams->getSeedRegionMin(i));
		rParams->setRegionMax(i,fParams->getSeedRegionMax(i));
	}
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	PanelCommand::captureEnd(cmd,rParams);
	
}
void RegionEventRouter::
guiSetVarNum(int n){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "set variable");
	rParams->setInfoVarNum(n);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);

}
void RegionEventRouter::guiSetTimeStep(int n){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "set time step");
	rParams->setInfoTimeStep(n);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
}
void RegionEventRouter::
guiSetNumRefinements(int n){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "set number of Refinements");
	rParams->setInfoNumRefinements(n);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
}


//Move the region center to specified coords, shrink it if necessary
void RegionEventRouter::
guiSetCenter(const float* coords){
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "move region center");
	const float* extents = Session::getInstance()->getExtents();
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float fullMin = extents[i];
		float fullMax = extents[i+3];
		if (coord < fullMin) coord = fullMin;
		if (coord > fullMax) coord = fullMax;
		float regSize = rParams->getRegionMax(i) - rParams->getRegionMin(i);
		if (coord + 0.5f*regSize > fullMax) regSize = 2.f*(fullMax - coord);
		if (coord - 0.5f*regSize < fullMin) regSize = 2.f*(coord - fullMin);
		rParams->setRegionMax(i, coord + 0.5f*regSize);
		rParams->setRegionMin(i, coord - 0.5f*regSize);
	}
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setRegionDirty(rParams);
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
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
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
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
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Z center");
	sliderToText(rParams, 2, sliderval, zSizeSlider->value());
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
	
}

void RegionEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region X size");
	sliderToText(rParams, 0, xCenterSlider->value(),sliderval);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}
void RegionEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Y size");
	sliderToText(rParams, 1, yCenterSlider->value(),sliderval);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}
void RegionEventRouter::
guiSetZSize(int sliderval){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "slide region Z size");
	sliderToText(rParams, 2, zCenterSlider->value(),sliderval);
	refreshRegionInfo(rParams);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}

void RegionEventRouter::
guiSetMaxSize(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	PanelCommand* cmd = PanelCommand::captureStart(rParams, "change region size to max");
	const float* fullDataExtents = Session::getInstance()->getExtents();
	for (int i = 0; i<3; i++){
		rParams->setRegionMin(i, fullDataExtents[i]);
		rParams->setRegionMax(i, fullDataExtents[i+3]);
	}
	updateTab();
	PanelCommand::captureEnd(cmd, rParams);

	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}

//Reinitialize region tab settings, session has changed
//Need to force the regionMin, regionMax to be OK.
void RegionEventRouter::
reinitTab(bool doOverride){
	int i;
	const Metadata* md = Session::getInstance()->getCurrentMetadata();
	
	//Set up the combo boxes in the gui based on info in the session:
	const vector<string>& varNames = md->GetVariableNames();
	variableCombo->clear();
	for (i = 0; i<(int)varNames.size(); i++)
		variableCombo->insertItem(varNames[i].c_str());
	
	int mints = DataStatus::getInstance()->getMinTimestep();
	int maxts = DataStatus::getInstance()->getMaxTimestep();

	timestepSpin->setMinValue(mints);
	timestepSpin->setMaxValue(maxts);

	int numRefinements = md->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
		refinementCombo->insertItem(QString::number(i));
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
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(rParams,  "slide region handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshRegion(rParams);
}
void RegionEventRouter::
captureMouseUp(){
	//Update the tab:
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::RegionParamsType);
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
