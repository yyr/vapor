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
#include <vapor/XmlNode.h>
#include "tabmanager.h"

#include "regioneventrouter.h"
#include "probeeventrouter.h"
#include "eventrouter.h"
#include "floweventrouter.h"

using namespace VAPoR;
std::vector<ParamsBase::ParamsBaseType> RegionEventRouter::boxMapping;
const char* RegionEventRouter::webHelpText[] = 
{

"Overview of the Region tab",
"Controlling the region extents",
"Time-varying region extents",
"Displaying information about current data in the region",
"Scene extents and boxes",
"Copying box extents",
"Mouse control of region extents",
"Tips for controlling region extents in the tab",
"<>"
};
const char* RegionEventRouter::webHelpURL[] =
{

	"http://www.vapor.ucar.edu/docs/vapor-gui-help/region-tab#RegionOverview",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/region-tab#RegionControl",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/region-tab#TimeVaryingRegionExtents",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/region-tab#RegionInfoLoadedData",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/scene-extents-and-boxes",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/copying-box-extents",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/mouse-modes",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/control-box-extents-tab",
};

RegionEventRouter::RegionEventRouter(QWidget* parent ): QWidget(parent), Ui_RegionTab(), EventRouter() {
	setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(Params::_regionParamsTag);
	MessageReporter::infoMsg("RegionEventRouter::RegionEventRouter()");
	setIgnoreBoxSliderEvents(false);
	myWebHelpActions = makeWebHelpActions(webHelpText, webHelpURL);
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
	connect (copyBoxButton, SIGNAL(clicked()), this, SLOT(guiCopyBox()));

	connect (loadRegionsButton, SIGNAL(clicked()), this, SLOT(guiLoadRegionExtents()));
	connect (saveRegionsButton, SIGNAL(clicked()), this, SLOT(saveRegionExtents()));
	connect (adjustExtentsButton, SIGNAL(clicked()), this, SLOT(guiAdjustExtents()));
	connect (refinementCombo, SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (variableCombo, SIGNAL(activated(int)), this, SLOT(guiSetVarNum(int)));
	connect (timestepSpin, SIGNAL(valueChanged(int)), this, SLOT(guiSetTimeStep(int)));

	connect (LocalGlobal, SIGNAL (activated (int)), VizWinMgr::getInstance(), SLOT (setRgLocalGlobal(int)));
	connect (VizWinMgr::getInstance(), SIGNAL(enableMultiViz(bool)), LocalGlobal, SLOT(setEnabled(bool)));

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



//Insert values from params into tab panel
//
void RegionEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	MainForm::getInstance()->buildWebTabHelpMenu(myWebHelpActions);
	if (!DataStatus::getInstance()->getDataMgr()) return;
	
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	double regLocalExts[6], regUsrExts[6];
	rParams->GetBox()->GetLocalExtents(regLocalExts, timestep);
	//Get the full domain extents in user coordinates
	const float* fullExtents = DataStatus::getInstance()->getLocalExtents();
	double fullUsrExts[6];
	for (int i = 0; i<3; i++) {
		fullUsrExts[i] = 0.;
		fullUsrExts[i+3] = fullExtents[i+3]-fullExtents[i];
	}
	//To get the region extents in user coordinates, need to add the user coord domain displacement
	vector<double> usrExts(6,0.);
	if (DataStatus::getInstance()->getDataMgr()){
		usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	}
	for (int i = 0; i<6; i++) {
		fullUsrExts[i]+= usrExts[i%3];
		regUsrExts[i] = regLocalExts[i]+usrExts[i%3];
	}

	Session::getInstance()->blockRecording();
	for (int i = 0; i< 3; i++){
		textToSlider(rParams, i, (regUsrExts[i]+regUsrExts[i+3])*0.5f,
			regUsrExts[i+3]-regUsrExts[i]);
	}
	setIgnoreBoxSliderEvents(true);
	xSizeEdit->setText(QString::number(regUsrExts[3]-regUsrExts[0],'g', 4));
	xCntrEdit->setText(QString::number(0.5f*(regUsrExts[3]+regUsrExts[0]),'g',5));
	ySizeEdit->setText(QString::number(regUsrExts[4]-regUsrExts[1],'g', 4));
	yCntrEdit->setText(QString::number(0.5f*(regUsrExts[4]+regUsrExts[1]),'g',5));
	zSizeEdit->setText(QString::number(regUsrExts[5]-regUsrExts[2],'g', 4));
	zCntrEdit->setText(QString::number(0.5f*(regUsrExts[5]+regUsrExts[2]),'g',5));

	fullMinXLabel->setText(QString::number(fullUsrExts[0], 'g',4));
	fullMinYLabel->setText(QString::number(fullUsrExts[1], 'g',4));
	fullMinZLabel->setText(QString::number(fullUsrExts[2], 'g',4));
	fullMaxXLabel->setText(QString::number(fullUsrExts[3], 'g',4));
	fullMaxYLabel->setText(QString::number(fullUsrExts[4], 'g',4));
	fullMaxZLabel->setText(QString::number(fullUsrExts[5], 'g',4));
	fullSizeXLabel->setText(QString::number(fullUsrExts[3]-fullUsrExts[0], 'g',4));
	fullSizeYLabel->setText(QString::number(fullUsrExts[4]-fullUsrExts[1], 'g',4));
	fullSizeZLabel->setText(QString::number(fullUsrExts[5]-fullUsrExts[2], 'g',4));

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
		boxLatLon[0] = regUsrExts[0];
		boxLatLon[1] = regUsrExts[1];
		boxLatLon[2] = regUsrExts[3];
		boxLatLon[3] = regUsrExts[4];
		
		if (DataStatus::convertToLonLat(boxLatLon,2)){
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
	setIgnoreBoxSliderEvents(false);
}


// Update the axes labels
//
void RegionEventRouter::relabel()
{
  if (0 && Session::getInstance()->sphericalTransform())
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
    
    xUserLabel->setText(labels[permutation[0]]);
    yUserLabel->setText(labels[permutation[1]]);
    zUserLabel->setText(labels[permutation[2]]);
    
                          
    xVoxelLabel->setText("Vox" + labels[permutation[0]]);
    yVoxelLabel->setText("Vox" + labels[permutation[1]]);
    zVoxelLabel->setText("Vox" + labels[permutation[2]]);
  }
  else
  {
    xCenterLabel->setText("X");
    yCenterLabel->setText("Y");
    zCenterLabel->setText("Z");

    xSizeLabel->setText("X Size");
    ySizeLabel->setText("Y Size");
    zSizeLabel->setText("Z Size");

    xUserLabel->setText("X");
    yUserLabel->setText("Y");
    zUserLabel->setText("Z");

    xVoxelLabel->setText("VoxX");
    yVoxelLabel->setText("VoxY");
    zVoxelLabel->setText("VoxZ");
  }
}

//Set slider position, based on text change. 
//
void RegionEventRouter::
textToSlider(RegionParams* rp, int coord, float newCenter, float newSize){
	setIgnoreBoxSliderEvents(true);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	bool centerChanged = false;
	bool sizeChanged = false;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	//Get the full extents in user coordinates
	const vector<double>& userExtents = dataMgr->GetExtents((size_t)timestep);
	
	float regMin = userExtents[coord];
	float regMax = userExtents[coord+3];
	
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
	//Now convert back to local extents, put them into the params:
	float localCenter = newCenter-userExtents[coord];
	rp->setLocalRegionMin(coord, localCenter - newSize*0.5f,timestep,false); 
	rp->setLocalRegionMax(coord,localCenter + newSize*0.5f,timestep,false); 
	//Put the user coords into the sliders:
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
	setIgnoreBoxSliderEvents(false);
	return;
}
//Set text when a slider changes.
//Move the center if the size is too big
void RegionEventRouter::
sliderToText(RegionParams* rp, int coord, int slideCenter, int slideSize){
	//force the size to be no greater than the max possible.
	//And force the center to fit in the region.  
	//Then push the center to the middle if the region doesn't fit
	DataStatus* ds = DataStatus::getInstance();
	if(!ds->getDataMgr()) return;
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>userExtents = ds->getDataMgr()->GetExtents((size_t)timestep);
	
	float regMin = userExtents[coord];
	float regMax = userExtents[coord+3];
	
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
	//Convert back to local to put into region params
	float localCenter = newCenter - userExtents[coord];
	rp->setLocalRegionMin(coord,localCenter - newSize*0.5f,timestep,false); 
	rp->setLocalRegionMax(coord,localCenter + newSize*0.5f,timestep,false); 
	
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
	
	DataStatus* ds= DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) ds = 0;
	int timeStep = timestepSpin->value();
	if (!dataMgr) timeStep = 0;
	else if ((timeStep < (int)ds->getMinTimestep()) || (timeStep > (int) ds->getMaxTimestep()) )timeStep = ds->getMinTimestep();

	//Get the user coordinate min/max
	vector<double> userExtents(6,0.);
	if(dataMgr) userExtents = dataMgr->GetExtents((size_t)timeStep);
	//Distinguish between the actual data available and the numtransforms
	//in the metadata.  If the data isn't there, we will display blanks
	//in the "selected" area.
	int maxRefLevel = 10;
	double regionMin[3],regionMax[3];
	for (int i = 0; i<3; i++){
		regionMin[i] = (double)rParams->getLocalRegionMin(i,timeStep)+userExtents[i];
		regionMax[i] = (double)rParams->getLocalRegionMax(i,timeStep)+userExtents[i];
	}
	
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
			varNum = ds->getSessionVariableNum3D(varName);
			maxRefLevel = ds->maxXFormPresent3D(varNum, timeStep);
		}
		else {
			int mdVarnum = ds->getActiveVarNum2D(varName);
			orientation = ds->get2DOrientation(mdVarnum);
			varNum = ds->getSessionVariableNum2D(varName);
			maxRefLevel = ds->maxXFormPresent2D(varNum, timeStep);
		}
		
	} 

	int refLevel = refinementCombo->currentIndex();
	
	minXFullLabel->setText(QString::number(userExtents[0],'g',3));
	minYFullLabel->setText(QString::number(userExtents[1],'g',3));
	minZFullLabel->setText(QString::number(userExtents[2],'g',3));
	maxXFullLabel->setText(QString::number(userExtents[3],'g',3));
	maxYFullLabel->setText(QString::number(userExtents[4],'g',3));
	maxZFullLabel->setText(QString::number(userExtents[5],'g',3));
	
	//For now, the min and max var extents are the whole thing:


	minXSelectedLabel->setText(QString::number(regionMin[0],'g',3));
	minYSelectedLabel->setText(QString::number(regionMin[1],'g',3));
	minZSelectedLabel->setText(QString::number(regionMin[2],'g',3));
	maxXSelectedLabel->setText(QString::number(regionMax[0],'g',3));
	maxYSelectedLabel->setText(QString::number(regionMax[1],'g',3));
	maxZSelectedLabel->setText(QString::number(regionMax[2],'g',3));


	//Now produce the corresponding voxel coords:
	size_t min_dim[3] = {0,0,0}, max_dim[3] = {0,0,0};
		
	// if region isn't valid just don't show the bounds:
	if (ds && refLevel <= maxRefLevel){
		dataMgr->GetEnclosingRegion(timeStep,regionMin,regionMax,min_dim,max_dim,refLevel);
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
		
		dataMgr->MapVoxToUser((size_t)timeStep, min_vdim, var_ext,refLevel,0);
		dataMgr->MapVoxToUser((size_t)timeStep, max_vdim, var_ext+3,refLevel,0);
		
		//Use full extents if variable is at extremes, to avoid confusion...
		for (int k = 0; k<3; k++){
			if (min_vdim[k] == 0) var_ext[k] = userExtents[k];
			if (max_vdim[k] == max_dim[k]) var_ext[k+3] = userExtents[k+3];
		}
		//Calculate fraction of extents:
		minVarXLabel->setText(QString::number(var_ext[0],'g',3));
		minVarYLabel->setText(QString::number(var_ext[1],'g',3));
		minVarZLabel->setText(QString::number(var_ext[2],'g',3));
		maxVarXLabel->setText(QString::number(var_ext[3],'g',3));
		maxVarYLabel->setText(QString::number(var_ext[4],'g',3));
		maxVarZLabel->setText(QString::number(var_ext[5],'g',3));
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
	if (rc >= 0){
		rParams->getRegionVoxelCoords(refLevel,0,min_dim,max_dim,
									  timeStep);
		
		//Size needed for data assumes blocksize = 2**5, 6 bytes per voxel, times 2.
		float newFullMB;
		if (is3D)
			newFullMB = (float)((max_dim[0]-min_dim[0]+1)*(max_dim[1]-min_dim[1]+1)*(max_dim[2]-min_dim[2]+1));
		else {
			//get other coords:
			int crd0 = 0, crd1 = 1;
			if (orientation < 2) crd1++;
			if (orientation < 1) crd0++;
			newFullMB = (float)((max_dim[crd0]-min_dim[crd0]+1)*(max_dim[crd1]-min_dim[crd1]+1));
		}
		
		//divide by 1 million for megabytes, mult by 4 for 4 bytes per voxel:
		newFullMB /= 262144.f;

		selectedDataSizeLabel->setText(QString::number(newFullMB,'g',8));
	}
	else
		selectedDataSizeLabel->setText("Data not present in VDC");
}

//Methods in support of undo/redo:
//
//Make region match probe.  Responds to button in region panel
void RegionEventRouter::
guiCopyBox(){
	confirmText(false);
	if (!DataStatus::getInstance()->getDataMgr()) return;
	int viznum = VizWinMgr::getInstance()->getActiveViz();
	if (viznum <0) return;
	ParamsBase::ParamsBaseType fromParams = boxMapping[copyBoxFromCombo->currentIndex()];
	ParamsBase::ParamsBaseType toParams = boxMapping[copyBoxToCombo->currentIndex()];
	if (toParams == fromParams) {
		MessageReporter::errorMsg("Source and Target of extents copy cannot be the same");
		return;
	}
	
	Params* pFrom = Params::GetCurrentParamsInstance(fromParams, viznum);
	Params* pTo = Params::GetCurrentParamsInstance(toParams, viznum);
	PanelCommand* cmd = PanelCommand::captureStart(pTo, "copy box extents");
	double toExtents[6], fromExtents[6];
	pFrom->GetBox()->GetLocalExtents(fromExtents);
	pTo->GetBox()->GetLocalExtents(toExtents);
	//Check if the source is 2D;  If the target is not 2D then don't alter its vertical extents
	if (fromExtents[2] == fromExtents[5] && toExtents[2] != toExtents[5]){
		fromExtents[2] = toExtents[2];
		fromExtents[5] = toExtents[5];
	}
	//Check if target is 2D, and source is not 2D don't change its extents
	else if (toExtents[2] == toExtents[5] && fromExtents[2] != fromExtents[5]){
		fromExtents[2] = toExtents[2];
		fromExtents[5] = toExtents[5];
	}
	pTo->GetBox()->SetLocalExtents(fromExtents);
	PanelCommand::captureEnd(cmd,pTo);
	VizWin* vizwin = VizWinMgr::getInstance()->getVizWin(viznum);
	if (vizwin) vizwin->updateGL();
	
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


//Move the region center to specified local coords, shrink it if necessary
void RegionEventRouter::
guiSetCenter(const float* coords){
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);

	if (!DataStatus::getInstance()->getDataMgr()) return;
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "move region center");
	
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const float* fullExtent = DataStatus::getInstance()->getLocalExtents();
	
	float boxmin[3], boxmax[3];
	rParams->getLocalBox(boxmin,boxmax,timestep);
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float regMin = 0.;
		float regMax = fullExtent[i+3]-fullExtent[i];
		if (coord < regMin) coord = regMin;
		if (coord > regMax) coord = regMax;
		float regSize = boxmax[i]-boxmin[i];
		if (coord + 0.5f*regSize > regMax) regSize = 2.f*(regMax - coord);
		if (coord - 0.5f*regSize < regMin) regSize = 2.f*(coord - regMin);
		boxmax[i] = coord + 0.5f*regSize;
		boxmin[i] = coord - 0.5f*regSize;
	}
	rParams->setLocalBox(boxmin, boxmax, timestep);
	PanelCommand::captureEnd(cmd, rParams);
	VizWinMgr::getInstance()->setRegionDirty(rParams);
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetXCenter(int sliderval){
	if(ignoreBoxSliderEvents)return;
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
	if(ignoreBoxSliderEvents)return;
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
	if(ignoreBoxSliderEvents)return;
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
	if(ignoreBoxSliderEvents)return;
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
	if(ignoreBoxSliderEvents)return;
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
	if(ignoreBoxSliderEvents)return;
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
	const float* fullDataExtents = DataStatus::getInstance()->getLocalExtents();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	float boxmin[3],boxmax[3];
	for (int i = 0; i<3; i++){
		boxmin[i] = 0.;
		boxmax[i]= fullDataExtents[i+3]-fullDataExtents[i];
	}
	rParams->setLocalBox(boxmin, boxmax, timestep);
	
	updateTab();
	PanelCommand::captureEnd(cmd, rParams);

	VizWinMgr::getInstance()->setVizDirty(rParams, RegionBit, true);
}

//Reinitialize region tab settings, session has changed
//Need to force the regionMin, regionMax to be OK.
void RegionEventRouter::
reinitTab(bool doOverride){
	int i;
	setIgnoreBoxSliderEvents(false);
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
	timestepSpin->setValue(mints);

	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	if (VizWinMgr::getInstance()->getNumVisualizers() > 1) LocalGlobal->setEnabled(true);
	else LocalGlobal->setEnabled(false);

	//Set up the copy combos
	copyBoxFromCombo->clear();
	copyBoxToCombo->clear();
	boxMapping.clear();
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		ParamsBase::ParamsBaseType type = i;
		Params* p = Params::GetDefaultParams(type);
		if (! p->GetBox()) continue;
		QString pname = QString(p->getShortName().c_str());
		copyBoxFromCombo->addItem(pname);
		copyBoxToCombo->addItem(pname);
		boxMapping.push_back(i);
	}
}



//Save undo/redo state when user grabs a rake handle
//
void RegionEventRouter::
captureMouseDown(int){
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

	const float* fullExtents = DataStatus::getInstance()->getLocalExtents();
	//Read the file

	int numregions = 0;
	
	while (1){
		int ts;
		float exts[6];
		int numVals = fscanf(regionFile, "%d %g %g %g %g %g %g", &ts,
			exts, exts+1, exts+2, exts+3, exts+4, exts+5);
			
		if (numVals < 7) {
			if (numVals > 0 && numregions > 0){
				MessageReporter::warningMsg("Invalid region extents at timestep %d",ts);
			}
			break;
		}
		numregions++;
		bool ok = true;
		//Force it to be valid:
		for (int i = 0; i< 3; i++){
			if (exts[i] < 0.) { ok = false; exts[i] = 0.;}
			if (exts[i+3] > fullExtents[i+3]-fullExtents[i]){ok = false; exts[i+3] = fullExtents[i+3]-fullExtents[i]; }
			if (exts[i] > exts[i+3]) { ok = false; exts[i+3] = exts[i];}
		}
		if (!ok){
			MessageReporter::warningMsg("Repaired region extents at timestep %d",ts);
		}
		rParams->insertTime(ts);
		double dbexts[6];
		for (int i = 0; i<6; i++)dbexts[i] = exts[i];
		rParams->GetBox()->SetLocalExtents(dbexts,ts);

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

	
	//Are there any extents to write?
	if (!rParams->extentsAreVarying()) {
		MessageReporter::errorMsg("No time-varying region extents to write to file");
		return;
	}
	//Launch a file-open dialog
	QString filename = QFileDialog::getSaveFileName(this,
        	"Specify file name for saving list of current time-varying Local Region extents",
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
	const vector<double> extents = rParams->GetAllExtents();
	const vector<long> times = rParams->GetTimes();
	for (int i = 1; i<times.size(); i++){
		int timestep = times[i];
		float exts[6]; 
		for (int j = 0; j< 6; j++)  exts[j] = extents[6*i+j];
		int rc = fprintf(regionFile, "%d %g %g %g %g %g %g\n",
			timestep, exts[0], exts[1],exts[2], exts[3],exts[4], exts[5]);
		if (rc < 7) {
			MessageReporter::errorMsg("Error writing region extents at timestep %d",timestep);
			fclose(regionFile);
			return;
		}
	}
	
	fclose(regionFile);
}
void RegionEventRouter::
guiAdjustExtents(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_regionParamsTag);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	PanelCommand* cmd = PanelCommand::captureStart(rParams,  "adjust current extents");
	
	if(!rParams->insertTime(timestep)){  //no change
		delete cmd;
		return;
	}
	PanelCommand::captureEnd(cmd, rParams);
}
