//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		regionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the RegionParams class.
//		This class supports parameters associted with the
//		region panel
//
#include "regionparams.h"
#include "regiontab.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include "params.h"
#include "vapor/Metadata.h"
#include "tabmanager.h"

using namespace VAPoR;

RegionParams::RegionParams(MainForm* mf, int winnum): Params(mf, winnum){
	thisParamType = RegionParamsType;
	myRegionTab = mf->getRegionTab();
	numTrans = 0;
	maxNumTrans = 9;
	maxSize = 256;
	for (int i = 0; i< 3; i++){
		centerPosition[i] = 128;
		regionSize[i] = 256;
		fullSize[i] = 256;
	}
	
	
}
Params* RegionParams::
deepCopy(){
	//Just make a shallow copy, since there's nothing (yet) extra
	//
	return (Params*)(new RegionParams(*this));
}

RegionParams::~RegionParams(){
}
void RegionParams::
makeCurrent(Params* ,bool) {
	VizWinMgr* vwm = mainWin->getVizWinMgr();
	vwm->setRegionParams(vizNum, this);
	//Also update current Tab.  It's probably visible.
	//
	updateDialog();
	setDirty();
}

//Must turn off undo/redo so events here don't get recorded
//
void RegionParams::updateDialog(){
	
	
	QString strng;
	mainWin->getSession()->blockRecording();
	myRegionTab->numTransSpin->setMaxValue(maxNumTrans);
	myRegionTab->numTransSpin->setValue(numTrans);
	int dataMaxSize = Max(fullSize[0],Max(fullSize[1],fullSize[2]));
	myRegionTab->maxSizeSlider->setMaxValue(dataMaxSize);
	myRegionTab->maxSizeSlider->setValue(maxSize);
	myRegionTab->maxSizeEdit->setText(strng.setNum(maxSize));
	
	myRegionTab->xCenterSlider->setMaxValue(fullSize[0]);
	myRegionTab->yCenterSlider->setMaxValue(fullSize[1]);
	myRegionTab->zCenterSlider->setMaxValue(fullSize[2]);
	myRegionTab->xSizeSlider->setMaxValue(fullSize[0]);
	myRegionTab->ySizeSlider->setMaxValue(fullSize[1]);
	myRegionTab->zSizeSlider->setMaxValue(fullSize[2]);

	//Enforce the requirement that center +- size/2 fits in range
	//
	for (int i = 0; i<3; i++){
		if(centerPosition[i] + (1+regionSize[i])/2 > fullSize[i])
			centerPosition[i] = fullSize[i] - (1+regionSize[i])/2;
		if(centerPosition[i] < (1+regionSize[i])/2)
			centerPosition[i] = (1+regionSize[i])/2;
	}

	myRegionTab->xCntrEdit->setText(strng.setNum(centerPosition[0]));
	myRegionTab->xCenterSlider->setValue(centerPosition[0]);
	myRegionTab->yCntrEdit->setText(strng.setNum(centerPosition[1]));
	myRegionTab->yCenterSlider->setValue(centerPosition[1]);
	myRegionTab->zCntrEdit->setText(strng.setNum(centerPosition[2]));
	myRegionTab->zCenterSlider->setValue(centerPosition[2]);

	myRegionTab->xSizeEdit->setText(strng.setNum(regionSize[0]));
	myRegionTab->xSizeSlider->setValue(regionSize[0]);
	myRegionTab->ySizeEdit->setText(strng.setNum(regionSize[1]));
	myRegionTab->ySizeSlider->setValue(regionSize[1]);
	myRegionTab->zSizeEdit->setText(strng.setNum(regionSize[2]));
	myRegionTab->zSizeSlider->setValue(regionSize[2]);

	if (isLocal())
		myRegionTab->LocalGlobal->setCurrentItem(1);
	else 
		myRegionTab->LocalGlobal->setCurrentItem(0);
	guiSetTextChanged(false);
	mainWin->getSession()->unblockRecording();
}

//Update all the panel state associated with textboxes.
//Then enforce consistency requirements
//Whenever this affects a slider, move it.
//
void RegionParams::
updatePanelState(){
	
	
	centerPosition[0] = myRegionTab->xCntrEdit->text().toFloat();
	centerPosition[1] = myRegionTab->yCntrEdit->text().toFloat();
	centerPosition[2] = myRegionTab->zCntrEdit->text().toFloat();
	regionSize[0] = myRegionTab->xSizeEdit->text().toFloat();
	regionSize[1] = myRegionTab->ySizeEdit->text().toFloat();
	regionSize[2] = myRegionTab->zSizeEdit->text().toFloat();
	int oldSize = maxSize;
	maxSize = myRegionTab->maxSizeEdit->text().toInt();
	//If maxSize changed, need to attempt to adjust everything:
	//If this is an increase, try to increase other sliders:
	//
	bool rgChanged[3];
	rgChanged[0]=rgChanged[1]=rgChanged[2]=false;
	if (oldSize < maxSize){
		for (int i = 0; i< 3; i++) {
			if (regionSize[i] < maxSize) {
				regionSize[i] = maxSize;
				rgChanged[i]=true;
			}
		}
	} else if (maxSize < oldSize) { //likewise for decrease:
		for (int i = 0; i< 3; i++) {
			if (regionSize[i] > maxSize){ 
				regionSize[i] = maxSize;
				rgChanged[i]=true;
			}
		}
	}
	for(int i = 0; i< 3; i++)
		if(enforceConsistency(i))rgChanged[i]=true;
	
	if(myRegionTab->xCenterSlider->value() != centerPosition[0])
		myRegionTab->xCenterSlider->setValue(centerPosition[0]);
	if(myRegionTab->yCenterSlider->value() != centerPosition[1])
		myRegionTab->yCenterSlider->setValue(centerPosition[1]);
	if(myRegionTab->zCenterSlider->value() != centerPosition[2])
		myRegionTab->zCenterSlider->setValue(centerPosition[2]);
	if (myRegionTab->xSizeSlider->value() != regionSize[0])
		myRegionTab->xSizeSlider->setValue(regionSize[0]);
	if (myRegionTab->ySizeSlider->value() != regionSize[1])
		myRegionTab->ySizeSlider->setValue(regionSize[1]);
	if (myRegionTab->zSizeSlider->value() != regionSize[2])
		myRegionTab->zSizeSlider->setValue(regionSize[2]);
	if (myRegionTab->maxSizeSlider->value() != maxSize)
		myRegionTab->maxSizeSlider->setValue(maxSize);
	
	if (rgChanged[0]){
			myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
			myRegionTab->xSizeEdit->setText(QString::number(regionSize[0]));
	}
	if (rgChanged[1]){
			myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
			myRegionTab->ySizeEdit->setText(QString::number(regionSize[1]));
	}
	if (rgChanged[2]){
			myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
			myRegionTab->zSizeEdit->setText(QString::number(regionSize[2]));
	}
	setDirty();
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
}
//Must Set this region as well as any others that are sharing
//RegionDirty refers to the latest region settings not yet visualized.
//
void RegionParams::
setDirty(){
	mainWin->getVizWinMgr()->setRegionDirty(this);
}
//Method to make center and size values legitimate for specified dimension
//Returns true if anything changed.
//Ignores MaxSize, since it's just a guideline.
//
bool RegionParams::
enforceConsistency(int dim){
	bool rc = false;
	if (regionSize[dim]>fullSize[dim]) {
		setRegionSize(dim,fullSize[dim]); 
		rc = true;
	}
	if (centerPosition[dim]<(1+regionSize[dim])/2) {
		setCenterPosition(dim, (1+regionSize[dim])/2);
		rc = true;
	}
	if (centerPosition[dim]+(1+regionSize[dim])/2 > fullSize[dim]){
		setCenterPosition(dim, fullSize[dim]-(1+regionSize[dim])/2);
		rc = true;
	}
	return rc;
}
//Methods in support of undo/redo:
//

void RegionParams::
guiSetNumTrans(int n){
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"set number of Transformations");
	setNumTrans(n);
	PanelCommand::captureEnd(cmd, this);
}
//Following are set when slider is released:
//
void RegionParams::
guiSetXCenter(int n){
	confirmText(false);
	
	//Was there a change?
	//
	if (n!= centerPosition[0]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region x-center");
		centerPosition[0] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(0)){
			myRegionTab->xCenterSlider->setValue(centerPosition[0]);
		}
		myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd,this);
	}
	setDirty();
}

void RegionParams::
guiSetXSize(int n){
	confirmText(false);
	
	//Was there a change?
	if (n!= regionSize[0]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region x-size");
		regionSize[0] = n;
		//If new position is invalid, move the slider back where it belongs, and
		//potentially move other slider as well
		//
		if (enforceConsistency(0)){
			myRegionTab->xSizeSlider->setValue(regionSize[0]);
			myRegionTab->xCenterSlider->setValue(centerPosition[0]);
			myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
		}
		myRegionTab->xSizeEdit->setText(QString::number(regionSize[0]));
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd,this);
	}
	setDirty();
}
void RegionParams::
guiSetYCenter(int n){
	confirmText(false);
	
	//Was there a change?
	//
	if (n!= centerPosition[1]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region y-center");
		centerPosition[1] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(1)){
			myRegionTab->yCenterSlider->setValue(centerPosition[1]);
		}
		myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd,this);
	}
	setDirty();
}

void RegionParams::
guiSetYSize(int n){
	confirmText(false);
	
	//Was there a change?
	if (n!= regionSize[1]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region y-size");
		regionSize[1] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(1)){
			myRegionTab->ySizeSlider->setValue(regionSize[1]);
			myRegionTab->yCenterSlider->setValue(centerPosition[1]);
			myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
		}
		myRegionTab->ySizeEdit->setText(QString::number(regionSize[1]));
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}
void RegionParams::
guiSetZCenter(int n){
	confirmText(false);
	
	//Was there a change?
	//
	if (n!= centerPosition[2]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region z-center");
		centerPosition[2] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(2)){
			myRegionTab->zCenterSlider->setValue(centerPosition[2]);
		}
		myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}

void RegionParams::
guiSetZSize(int n){
	confirmText(false);
	
	//Was there a change?
	if (n!= regionSize[2]) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region z-size");
		regionSize[2] = n;
		//If new position is invalid, move the slider back where it belongs:
		//
		if (enforceConsistency(2)){
			myRegionTab->zSizeSlider->setValue(regionSize[2]);
			myRegionTab->zCenterSlider->setValue(centerPosition[2]);
			myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
		}
		myRegionTab->zSizeEdit->setText(QString::number(regionSize[2]));
		//Ignore the resulting textChanged event:
		//
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}
void RegionParams::
guiSetMaxSize(int n){
	confirmText(false);
	int oldSize = maxSize;
	bool didChange[3];
	//Was there a change?
	//
	if (n!= maxSize) {
		PanelCommand* cmd = PanelCommand::captureStart(this, mainWin->getSession(),"change region Max Size");
		maxSize = n;
		didChange[0] = didChange[1] = didChange[2] = false;
		//If this is an increase, try to increase other sliders:
		//
		if (oldSize < maxSize){
			for (int i = 0; i< 3; i++) {
				if (regionSize[i] < maxSize) {
					regionSize[i] = maxSize;
					didChange[i] = true;
				}
			}
		} else { //likewise for decrease:
			for (int i = 0; i< 3; i++) {
				if (regionSize[i] > maxSize){ 
					regionSize[i] = maxSize;
					didChange[i] = true;
				}
			}
		}
		//Then enforce consistency:
		//
		if (enforceConsistency(0)||didChange[0]){
			myRegionTab->xSizeSlider->setValue(regionSize[0]);
			myRegionTab->xCenterSlider->setValue(centerPosition[0]);
			myRegionTab->xSizeEdit->setText(QString::number(regionSize[0]));
			myRegionTab->xCntrEdit->setText(QString::number(centerPosition[0]));
		}
		if (enforceConsistency(1)||didChange[1]){
			myRegionTab->ySizeSlider->setValue(regionSize[1]);
			myRegionTab->yCenterSlider->setValue(centerPosition[1]);
			myRegionTab->ySizeEdit->setText(QString::number(regionSize[1]));
			myRegionTab->yCntrEdit->setText(QString::number(centerPosition[1]));
		}
		if (enforceConsistency(2)||didChange[2]){
			myRegionTab->zSizeSlider->setValue(regionSize[2]);
			myRegionTab->zSizeEdit->setText(QString::number(regionSize[2]));
			myRegionTab->zCenterSlider->setValue(centerPosition[2]);
			myRegionTab->zCntrEdit->setText(QString::number(centerPosition[2]));
		}
		
		myRegionTab->maxSizeEdit->setText(QString::number(maxSize));
		//Ignore the resulting textChanged events:
		textChangedFlag = false;
		PanelCommand::captureEnd(cmd, this);
	}
	setDirty();
}

//Just a silly hack to demonstrate use of mouse to control center position
//Not supported (yet) by undo/redo
//
void RegionParams::
slide (QPoint& ){
	//Make a nonsensical change in region size:
	//centerPosition[0] += delta.x()/10;
	//centerPosition[1] -= delta.y()/10;
	//centerPosition[2] += (delta.x()-delta.y())/10;
	//Hack (not the right place to check input values)
	//for (int i = 0; i< 3; i++){
	//	if (centerPosition[i] < 0) centerPosition[i] = 0;
	//	if (centerPosition[i] > regionSize[i]) centerPosition[i] = regionSize[i];
	//}
	//then force a UI update:
	updateDialog();
}
//Reinitialize region settings, session has changed:
void RegionParams::
reinit(){
	const Metadata* md = mainWin->getSession()->getCurrentMetadata();
	//Setup the global region parameters based on bounds in Metadata
	const size_t* dataDim = md->GetDimension();
	
	
	int nx = dataDim[0];
	int ny = dataDim[1];
	int nz = dataDim[2];
	setMaxSize(Max(Max(nx, ny), nz));
	setFullSize(0, nx);
	setFullSize(1, ny);
	setFullSize(2, nz);

	setRegionSize(0, nx);
	setRegionSize(1, ny);
	setRegionSize(2, nz);
	
	setCenterPosition(0, nx/2);
	setCenterPosition(1, ny/2);
	setCenterPosition(2, nz/2);
	int nlevels = md->GetNumTransforms();
	setMaxNumTrans(nlevels);
	setNumTrans(nlevels);
	//Data extents (user coords) will need to be presented in gui
	//
	setDataExtents(md->GetExtents());
	//This will force the current tab to be reset to values
	//consistent with the data.
	//
	if(mainWin->getTabManager()->isFrontTab(myRegionTab)) updateDialog();
}

