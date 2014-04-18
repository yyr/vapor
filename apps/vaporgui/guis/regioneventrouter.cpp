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

#include "mainform.h"

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
#include "vapor/ControlExecutive.h"
#include "eventrouter.h"


using namespace VAPoR;


RegionEventRouter::RegionEventRouter(QWidget* parent ): QWidget(parent), Ui_RegionTab(), EventRouter() {
	setupUi(this);
	myParamsBaseType = ControlExec::GetTypeFromTag(Params::_regionParamsTag);
	
	setIgnoreBoxSliderEvents(false);
}


RegionEventRouter::~RegionEventRouter(){
	
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
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	
	Command* cmd = Command::CaptureStart(rParams,"region text edit");
	
	float centerPos[3], regSize[3];
	centerPos[0] = xCntrEdit->text().toFloat();
	centerPos[1] = yCntrEdit->text().toFloat();
	centerPos[2] = zCntrEdit->text().toFloat();
	regSize[0] = xSizeEdit->text().toFloat();
	regSize[1] = ySizeEdit->text().toFloat();
	regSize[2] = zSizeEdit->text().toFloat();

	
	for (int i = 0; i<3; i++)
		textToSlider(rParams,i,centerPos[i],regSize[i],true);

	guiSetTextChanged(false);
	rParams->Validate(false);
	
	Command::CaptureEnd(cmd,rParams);
	guiSetTextChanged(false);
	
	VizWinMgr::getInstance()->forceRender(rParams);
	
	
}
void RegionEventRouter::
regionReturnPressed(void){
	confirmText(true);
}


void RegionEventRouter::copyRakeToRegion(){
	
}
void RegionEventRouter::copyProbeToRegion(){
	
}
void RegionEventRouter::copyRegionToRake(){
	//Need to find relevant Flowparams, make it update with this region.
	
	
}


//Insert values from params into tab panel
//
void RegionEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	double regLocalExts[6], regUsrExts[6];
	rParams->GetBox()->GetLocalExtents(regLocalExts, timestep);
	//Get the full domain extents in user coordinates
	const double* fullExtents = DataStatus::getInstance()->getLocalExtents();
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

	
	for (int i = 0; i< 3; i++){
		textToSlider(rParams, i, (regUsrExts[i]+regUsrExts[i+3])*0.5f,
			regUsrExts[i+3]-regUsrExts[i],false);
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

	if (rParams->IsLocal())
		LocalGlobal->setCurrentIndex(1);
	else 
		LocalGlobal->setCurrentIndex(0);

	
	minMaxLonLatFrame->hide();
	
	guiSetTextChanged(false);

    relabel();
	
	
	VizWinMgr::getInstance()->getTabManager()->update();
	setIgnoreBoxSliderEvents(false);
	
}


// Update the axes labels
//
void RegionEventRouter::relabel()
{
  
 
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
textToSlider(RegionParams* rp, int coord, float newCenter, float newSize, bool doSet){
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
	if (doSet){
		rp->SetLocalRegionMin(coord, localCenter - newSize*0.5f,timestep); 
		rp->SetLocalRegionMax(coord,localCenter + newSize*0.5f,timestep);
	}
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
	Command* cmd = Command::CaptureStart(rp, "Region slider move");
	rp->SetLocalRegionMin(coord,localCenter - newSize*0.5f,timestep); 
	rp->SetLocalRegionMax(coord,localCenter + newSize*0.5f,timestep); 
	Command::CaptureEnd(cmd,rp);
	
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
	
	return;
}	





//Move the region center to specified user coords, shrink it if necessary
void RegionEventRouter::
guiSetCenter(const double* coords){
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);

	if (!DataStatus::getInstance()->getDataMgr()) return;
	
	
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>&userExtents = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	float boxexts[6];
	rParams->GetBox()->GetLocalExtents(boxexts, timestep);
	
	for (int i = 0; i< 3; i++){
		float coord = coords[i];
		float fullMin = userExtents[i];
		float fullMax = userExtents[i+3];
		if (coord < fullMin) coord = fullMin;
		if (coord > fullMax) coord = fullMax;
		float regSize = boxexts[i+3]-boxexts[i];
		if (coord + 0.5f*regSize > fullMax) regSize = 2.f*(fullMax - coord);
		if (coord - 0.5f*regSize < fullMin) regSize = 2.f*(coord - fullMin);
		boxexts[i+3] = coord + 0.5f*regSize - userExtents[i];
		boxexts[i] = coord - 0.5f*regSize - userExtents[i];
	}
	rParams->GetBox()->SetLocalExtents(boxexts, rParams, timestep);
	
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetXCenter(int sliderval){
	if(ignoreBoxSliderEvents)return;
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	
	sliderToText(rParams, 0, sliderval, xSizeSlider->value());
	
	
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetYCenter(int sliderval){
	if(ignoreBoxSliderEvents)return;
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	sliderToText(rParams, 1, sliderval, ySizeSlider->value());
	
}
//Following are set when slider is released:
void RegionEventRouter::
guiSetZCenter(int sliderval){
	if(ignoreBoxSliderEvents)return;
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	sliderToText(rParams, 2, sliderval, zSizeSlider->value());
	
}

void RegionEventRouter::
guiSetXSize(int sliderval){
	if(ignoreBoxSliderEvents)return;
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	sliderToText(rParams, 0, xCenterSlider->value(),sliderval);
	
}
void RegionEventRouter::
guiSetYSize(int sliderval){
	if(ignoreBoxSliderEvents)return;
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	sliderToText(rParams, 1, yCenterSlider->value(),sliderval);
	
}
void RegionEventRouter::
guiSetZSize(int sliderval){
	if(ignoreBoxSliderEvents)return;
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	sliderToText(rParams, 2, zCenterSlider->value(),sliderval);
	
}

void RegionEventRouter::
guiSetMaxSize(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	const double* fullDataExtents = DataStatus::getInstance()->getLocalExtents();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	double boxexts[6];
	for (int i = 0; i<3; i++){
		boxexts[i] = 0.;
		boxexts[i+3]= fullDataExtents[i+3]-fullDataExtents[i];
	}
	rParams->GetBox()->SetLocalExtents(boxexts, rParams,timestep);
	
	updateTab();
	
}

//Reinitialize region tab settings, session has changed
//Need to force the regionMin, regionMax to be OK.
void RegionEventRouter::
reinitTab(bool doOverride){
	int i;
	setIgnoreBoxSliderEvents(false);
	const DataMgr *dataMgr = DataStatus::getInstance()->getDataMgr();
	
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
}



//Save undo/redo state when user grabs a rake handle
//
void RegionEventRouter::
captureMouseDown(int){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	guiSetTextChanged(false);
	
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshRegion(rParams);
}
void RegionEventRouter::
captureMouseUp(){
	//Update the tab:
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	updateTab();
	
	
	//Force a rerender:
	VizWinMgr::getInstance()->refreshRegion(rParams);
	
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
		".",
        	"Text files (*.txt)");
	//Check that user did specify a file:
	if (filename.isNull()) {
		return;
	}
	
	//Open the file:
	FILE* regionFile = fopen((const char*)filename.toAscii(),"r");
	if (!regionFile){
		
		return;
	}
	//File is OK, save the state in command queue
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);

	const double* fullExtents = DataStatus::getInstance()->getLocalExtents();
	//Read the file

	int numregions = 0;
	
	while (1){
		int ts;
		float exts[6];
		int numVals = fscanf(regionFile, "%d %g %g %g %g %g %g", &ts,
			exts, exts+1, exts+2, exts+3, exts+4, exts+5);
			
		if (numVals < 7) {
			if (numVals > 0 && numregions > 0){
				
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
			
		}
		rParams->insertTime(ts);
		double dbexts[6];
		for (int i = 0; i<6; i++)dbexts[i] = exts[i];
		rParams->GetBox()->SetLocalExtents(dbexts, rParams,ts);

	}
	if (numregions == 0) {
		
		
		fclose(regionFile);
		return;
	}

	
	
	
	fclose(regionFile);
	updateTab();
	
}
void RegionEventRouter::
saveRegionExtents(){
	//save the list of
	//region extents.   
	//Timesteps that have not been modified do not get saved.
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	
	//Are there any extents to write?
	if (!rParams->extentsAreVarying()) {
		
		return;
	}
	//Launch a file-open dialog
	QString filename = QFileDialog::getSaveFileName(this,
        	"Specify file name for saving list of current time-varying Local Region extents",
		".",
        	"Text files (*.txt)");
	//Check that user did specify a file:
	if (filename.isNull()) {
		return;
	}
	
	//Open the file:
	FILE* regionFile = fopen((const char*)filename.toAscii(),"w");
	if (!regionFile){
		
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
			
			fclose(regionFile);
			return;
		}
	}
	
	fclose(regionFile);
}
void RegionEventRouter::
guiAdjustExtents(){
	confirmText(false);
	RegionParams* rParams = (RegionParams*)ControlExec::GetActiveParams(Params::_regionParamsTag);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	
	if(!rParams->insertTime(timestep)){  //no change
		
		return;
	}
	
}
