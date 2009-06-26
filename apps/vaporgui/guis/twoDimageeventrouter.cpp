//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDimageeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Implements the TwoDImageEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the TwoD image tab
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
#include <qfileinfo.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qtooltip.h>
#include <qlayout.h>
#include "twoDimagerenderer.h"
#include "mappingframe.h"
#include "transferfunction.h"
#include "regionparams.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "twodframe.h"
#include "floweventrouter.h"
#include "instancetable.h"
#include "qthumbwheel.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "twoDimagetab.h"
#include "vaporinternal/jpegapi.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "GetAppPath.h"
#include "tabmanager.h"
#include "glutil.h"
#include "twoDimageparams.h"
#include "twoDimageeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "savetfdialog.h"
#include "loadtfdialog.h"
#include "VolumeRenderer.h"

using namespace VAPoR;


TwoDImageEventRouter::TwoDImageEventRouter(QWidget* parent,const char* name): TwoDImageTab(parent, name), TwoDEventRouter(){
	myParamsType = Params::TwoDImageParamsType;
}


TwoDImageEventRouter::~TwoDImageEventRouter(){
	
}
/**********************************************************
 * Whenever a new TwoDImagetab is created it must be hooked up here
 ************************************************************/
void
TwoDImageEventRouter::hookUpTab()
{
	//Nudge sliders by clicking on slider bar:
	connect (widthSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXSize(int)));
	connect (xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXCenter(int)));
	connect (lengthSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYSize(int)));
	connect (yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYCenter(int)));
	connect (zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeZCenter(int)));
	
	connect (xCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (yCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (zCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (widthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (lengthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	
	connect (xCenterEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (yCenterEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (zCenterEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (widthEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (lengthEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	
	connect (opacityEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (opacityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	
	
	connect (regionCenterButton, SIGNAL(clicked()), this, SLOT(twoDCenterRegion()));
	connect (viewCenterButton, SIGNAL(clicked()), this, SLOT(twoDCenterView()));
	connect (rakeCenterButton, SIGNAL(clicked()), this, SLOT(twoDCenterRake()));
	connect (probeCenterButton, SIGNAL(clicked()), this, SLOT(guiCenterProbe()));
	connect (addSeedButton, SIGNAL(clicked()), this, SLOT(twoDAddSeed()));
	connect (applyTerrainCheckbox, SIGNAL(toggled(bool)),this, SLOT(guiApplyTerrain(bool)));
	connect (attachSeedCheckbox,SIGNAL(toggled(bool)),this, SLOT(twoDAttachSeed(bool)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDZCenter()));
	connect (widthSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDXSize()));
	connect (lengthSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDYSize()));
	connect (opacitySlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetOpacitySlider(int)));

	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setTwoDEnabled(bool,int)));

	connect (imageFileButton, SIGNAL(clicked()), this, SLOT(guiSelectImageFile()));
	connect (orientationCombo, SIGNAL(activated(int)), this, SLOT(guiSetOrientation(int)));
	connect (geoRefCheckbox, SIGNAL(toggled(bool)),this, SLOT(guiSetGeoreferencing(bool)));
	connect (cropCheckbox, SIGNAL(toggled(bool)),this, SLOT(guiSetCrop(bool)));
	connect (fitToImageButton, SIGNAL(clicked()), this, SLOT(guiFitToImage()));
	connect (placementCombo, SIGNAL(activated(int)), this, SLOT(guiSetPlacement(int)));
	connect (fitToRegionButton, SIGNAL(clicked()), this, SLOT(guiFitToRegion()));

	
}
//Insert values from params into tab panel
//
void TwoDImageEventRouter::updateTab(){
	if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
	if (!isEnabled()) return;
	if (GLWindow::isRendering()) return;
	guiSetTextChanged(false);
	notNudgingSliders = true;  //don't generate nudge events

	DataStatus* ds = DataStatus::getInstance();
	if (ds->getDataMgr()) 
			instanceTable->setEnabled(true);
	else {
		instanceTable->setEnabled(false);
	}
	instanceTable->rebuild(this);
	
	TwoDImageParams* twoDParams = VizWinMgr::getActiveTwoDImageParams();
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int currentTimeStep = vizMgr->getActiveAnimationParams()->getCurrentFrameNumber();
	int winnum = vizMgr->getActiveViz();
	Session* ses = Session::getInstance();
	ses->blockRecording();
    
	
	int orientation = twoDParams->getOrientation();
	
	orientationCombo->setCurrentItem(orientation);
	
	placementCombo->setCurrentItem(twoDParams->getImagePlacement());
	//Force consistent settings, if there is a dataset
	if (ds->getDataMgr()){
		//See if we can do terrain mapping
		if(orientation == 2){
			int varnum = DataStatus::getSessionVariableNum2D("HGT");
			if (varnum < 0 || !ds->dataIsPresent2D(varnum, currentTimeStep)){
				applyTerrainCheckbox->setEnabled(false);
				applyTerrainCheckbox->setChecked(false);
				twoDParams->setMappedToTerrain(false);
			} else {
				applyTerrainCheckbox->setEnabled(true);
				applyTerrainCheckbox->setChecked(twoDParams->isMappedToTerrain());
			}
		} else {
			applyTerrainCheckbox->setChecked(false);
			applyTerrainCheckbox->setEnabled(false);
		}
		if ((ds->getProjectionString().size() > 0) && orientation == 2){
			geoRefCheckbox->setEnabled(true);
			geoRefCheckbox->setChecked(twoDParams->isGeoreferenced());
		} else {
			geoRefCheckbox->setEnabled(false);
			geoRefCheckbox->setChecked(false);
			twoDParams->setGeoreferenced(false);
		}
		bool georef = twoDParams->isGeoreferenced();
		if (georef){
			cropCheckbox->setEnabled(true);
			fitToImageButton->setEnabled(true);
			cropCheckbox->setChecked(twoDParams->imageCrop());
		} else {
			cropCheckbox->setEnabled(false);
			fitToImageButton->setEnabled(false);
			cropCheckbox->setChecked(true);
		}
		//placement combo is disabled if either mapped to terrain or georef:
		if (twoDParams->isMappedToTerrain() || georef){
			placementCombo->setEnabled(false);
			placementCombo->setCurrentItem(0);//upright
			twoDParams->setImagePlacement(0);
		} else {
			placementCombo->setEnabled(true);
		}

	}


	//set up the cursor position
	mapCursor();
	const float* selectedPoint = twoDParams->getSelectedPoint();
	selectedXLabel->setText(QString::number(selectedPoint[0]));
	selectedYLabel->setText(QString::number(selectedPoint[1]));
	selectedZLabel->setText(QString::number(selectedPoint[2]));
	//Provide latlon coords if available:
	if (DataStatus::getProjectionString().size() == 0){
		latLonFrame->hide();
	} else {
		double selectedLatLon[2];
		selectedLatLon[0] = selectedPoint[0];
		selectedLatLon[1] = selectedPoint[1];
		if (DataStatus::convertToLatLon(currentTimeStep,selectedLatLon)){
			selectedLonLabel->setText(QString::number(selectedLatLon[0]));
			selectedLatLabel->setText(QString::number(selectedLatLon[1]));
			latLonFrame->show();
		} else {
			latLonFrame->hide();
		}
	}
	guiSetTextChanged(false);
	attachSeedCheckbox->setChecked(seedAttached);
	
	filenameEdit->setText(twoDParams->getImageFileName().c_str());
	
	float opacityMult = twoDParams->getOpacMult();
	opacityEdit->setText(QString::number(opacityMult));
	opacitySlider->setValue((int)(opacityMult*256.f));
	guiSetTextChanged(false);
	
	
	deleteInstanceButton->setEnabled(vizMgr->getNumTwoDImageInstances(winnum) > 1);
	
	int numViz = vizMgr->getNumVisualizers();

	copyCombo->clear();
	copyCombo->insertItem("Duplicate In:");
	copyCombo->insertItem("This visualizer");
	if (numViz > 1) {
		int copyNum = 2;
		for (int i = 0; i<MAXVIZWINS; i++){
			if (vizMgr->getVizWin(i) && winnum != i){
				copyCombo->insertItem(vizMgr->getVizWinName(i));
				//Remember the viznum corresponding to a combo item:
				copyCount[copyNum++] = i;
			}
		}
	}

	//setup the texture:
	
	resetTextureSize(twoDParams);
	
	QString strn;
	
	//And the center sliders/textboxes:
	float boxmin[3],boxmax[3],boxCenter[3];
	const float* extents = ds->getExtents();
	twoDParams->getBox(boxmin, boxmax);
	for (int i = 0; i<3; i++) boxCenter[i] = (boxmax[i]+boxmin[i])*0.5f;
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-extents[0])/(extents[3]-extents[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-extents[1])/(extents[4]-extents[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-extents[2])/(extents[5]-extents[2])));
	xCenterEdit->setText(QString::number(boxCenter[0]));
	yCenterEdit->setText(QString::number(boxCenter[1]));
	zCenterEdit->setText(QString::number(boxCenter[2]));
	
	//setup the size sliders 
	adjustBoxSize(twoDParams);

	//Specify the box extents in both user and grid coords:
	minUserXLabel->setText(QString::number(boxmin[0]));
	minUserYLabel->setText(QString::number(boxmin[1]));
	minUserZLabel->setText(QString::number(boxmin[2]));
	maxUserXLabel->setText(QString::number(boxmax[0]));
	maxUserYLabel->setText(QString::number(boxmax[1]));
	maxUserZLabel->setText(QString::number(boxmax[2]));

	const VDFIOBase* myReader = ds->getRegionReader();
	if (myReader){
		int fullRefLevel = ds->getNumTransforms();

		double dBoxMin[3], dBoxMax[3];
		size_t gridMin[3],gridMax[3];
		for (int i = 0; i<3; i++) {
			dBoxMin[i] = boxmin[i];
			dBoxMax[i] = boxmax[i];
		}
		myReader->MapUserToVox((size_t)-1, dBoxMin, gridMin, fullRefLevel);
		myReader->MapUserToVox((size_t)-1, dBoxMax, gridMax, fullRefLevel);
		minGridXLabel->setText(QString::number(gridMin[0]));
		minGridYLabel->setText(QString::number(gridMin[1]));
		minGridZLabel->setText(QString::number(gridMin[2]));
		maxGridXLabel->setText(QString::number(gridMax[0]));
		maxGridYLabel->setText(QString::number(gridMax[1]));
		maxGridZLabel->setText(QString::number(gridMax[2]));
	}
    //Provide latlon box extents if available:
	if (DataStatus::getProjectionString().size() == 0){
		minMaxLonLatFrame->hide();
	} else {
		double boxLatLon[4];
		boxLatLon[0] = boxmin[0];
		boxLatLon[1] = boxmin[1];
		boxLatLon[2] = boxmax[0];
		boxLatLon[3] = boxmax[1];
		if (DataStatus::convertToLatLon(currentTimeStep,boxLatLon,2)){
			minLonLabel->setText(QString::number(boxLatLon[0]));
			minLatLabel->setText(QString::number(boxLatLon[1]));
			maxLonLabel->setText(QString::number(boxLatLon[2]));
			maxLatLabel->setText(QString::number(boxLatLon[3]));
			minMaxLonLatFrame->show();
		} else {
			minMaxLonLatFrame->hide();
		}
	}
	
	//Only allow terrain map with horizontal orientation
	
	twoDTextureFrame->setParams(twoDParams, false);
	
	vizMgr->getTabManager()->update();
	twoDTextureFrame->update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	
	notNudgingSliders = false;
}
//Fix for clean Windows scrolling:
void TwoDImageEventRouter::refreshTab(){
	 
	twoDFrameHolder->hide();
	twoDFrameHolder->show();

}

void TwoDImageEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	TwoDImageParams* twoDParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(twoDParams, "edit TwoD text");
	QString strn;
	
	float op = opacityEdit->text().toFloat();
	if (op < 0.f) {op = 0.f; opacityEdit->setText(QString::number(op));}
	if (op > 1.f) {op = 1.f; opacityEdit->setText(QString::number(op));}
	twoDParams->setOpacMult(op);
	opacitySlider->setValue((int)(op*256.f));

	int orientation = twoDParams->getOrientation();
	int xcrd =0, ycrd = 1, zcrd = 2;
	if (orientation < 2) {ycrd = 2; zcrd = 1;}
	if (orientation < 1) {xcrd = 1; zcrd = 0;}

	const float *extents = DataStatus::getInstance()->getExtents();
	//Set the twoD size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[xcrd] = widthEdit->text().toFloat();
	boxSize[ycrd] = lengthEdit->text().toFloat();
	boxSize[zcrd] = 0;
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
	}
	boxCenter[0] = xCenterEdit->text().toFloat();
	boxCenter[1] = yCenterEdit->text().toFloat();
	boxCenter[2] = zCenterEdit->text().toFloat();
	twoDParams->getBox(boxmin, boxmax);
	//the box z-size is not adjustable:
	boxSize[zcrd] = boxmax[zcrd]-boxmin[zcrd];
	for (int i = 0; i<3;i++){
		boxmin[i] = boxCenter[i] - 0.5f*boxSize[i];
		boxmax[i] = boxCenter[i] + 0.5f*boxSize[i];
	}
	twoDParams->setBox(boxmin,boxmax);
	adjustBoxSize(twoDParams);
	//set the center sliders:
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-extents[0])/(extents[3]-extents[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-extents[1])/(extents[4]-extents[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-extents[2])/(extents[5]-extents[2])));
	resetTextureSize(twoDParams);
	//twoDTextureFrame->setTextureSize(voxDims[0],voxDims[1]);
	setTwoDDirty(twoDParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(twoDParams,TwoDTextureBit,true);
	//If we are in twoD mode, force a rerender of all windows using the twoD:
	if (GLWindow::getCurrentMouseMode() == GLWindow::twoDImageMode){
		VizWinMgr::getInstance()->refreshTwoDImage(twoDParams);
	}
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, twoDParams);
}


/*********************************************************************************
 * Slots associated with TwoDImageTab:
 *********************************************************************************/


// Launch a file selection dialog to select an image file
void TwoDImageEventRouter::guiSelectImageFile(){
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(tParams,  "select image file");
	QString filename = QFileDialog::getOpenFileName(
		Session::getInstance()->getJpegDirectory().c_str(),
        "TIFF files (*.tiff *.tif *.gtif)",
        this,
        "Specify image file Dialog",
        "Specify file name for loading image into 2D panel" );
	//Check that user did specify a file:
	if (filename.isNull()) {
		delete cmd;
		return;
	}
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future image I/O
	Session::getInstance()->setJpegDirectory(fileInfo->dirPath(true).ascii());
	
	tParams->setImageFileName(filename.ascii());
	
	filenameEdit->setText(filename);
	PanelCommand::captureEnd(cmd, tParams);
	tParams->setImagesDirty();
	VizWinMgr::getInstance()->refreshTwoDImage(tParams);
}
void TwoDImageEventRouter::guiSetOrientation(int val){
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	int prevOrientation = tParams->getOrientation();
	if (prevOrientation == val) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(tParams, "set 2D orientation");
	//Note that the twodparams will readjust the box size when it is reoriented.
	
	tParams->setOrientation(val);
	if (val!=2) {
		tParams->setGeoreferenced(false);
		tParams->setMappedToTerrain(false);
	} 
	PanelCommand::captureEnd(cmd, tParams); 
	setTwoDDirty(tParams);
	VizWinMgr::getInstance()->setVizDirty(tParams,TwoDTextureBit,true);
	updateTab();
}
void TwoDImageEventRouter::guiFitToImage(){
	//Get the 4 corners of the image (in local coordinates)
	double corners[8];
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	if(!tParams->getImageCorners(timestep, corners)) return;

	PanelCommand* cmd = PanelCommand::captureStart(tParams, "fit 2D extents to image");
	
	float newExts[6];
	
	tParams->getImageCorners(timestep, corners);
	tParams->getBox(newExts, newExts+3);
	newExts[0] = newExts[1] = 1.e30f;
	newExts[3] = newExts[4] = -1.e30f;
	//Adjust the new 2d extents to contain these corners
	
	for (int i = 0; i<4; i++){
		if (newExts[0] > corners[2*i]) newExts[0] = corners[2*i];
		if (newExts[1] > corners[2*i+1]) newExts[1] = corners[2*i+1];
		if (newExts[3] < corners[2*i]) newExts[3] = corners[2*i];
		if (newExts[4] < corners[2*i+1]) newExts[4] = corners[2*i+1];
	}
	tParams->setBox(newExts, newExts+3);
	PanelCommand::captureEnd(cmd, tParams); 
	setTwoDDirty(tParams);
	VizWinMgr::getInstance()->setVizDirty(tParams,TwoDTextureBit,true);
	updateTab();
}
void TwoDImageEventRouter::guiSetGeoreferencing(bool val){
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(tParams, "toggle georeferencing on/off");
	tParams->setGeoreferenced(val);
	if (val){
		tParams->setImagePlacement(0);
		placementCombo->setEnabled(false);
	} else {
		tParams->setImageCrop(false);
		if (!tParams->isGeoreferenced())
			placementCombo->setEnabled(true);
	}
	PanelCommand::captureEnd(cmd, tParams); 
	tParams->setImagesDirty();
	VizWinMgr::getInstance()->setVizDirty(tParams,TwoDTextureBit,true);
	updateTab();
}
void TwoDImageEventRouter::guiSetCrop(bool val){
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(tParams, "toggle 2D cropping on/off");
	tParams->setImageCrop(val);
	PanelCommand::captureEnd(cmd, tParams); 
	VizWinMgr::getInstance()->setVizDirty(tParams,TwoDTextureBit,true);
}
void TwoDImageEventRouter::guiSetPlacement(int val){
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(tParams, "specify image placement");
	tParams->setImagePlacement(val);
	PanelCommand::captureEnd(cmd, tParams); 
	//When placement is changed, need to rebuild geometry for same image
	setTwoDDirty(tParams);
	VizWinMgr::getInstance()->refreshTwoDImage(tParams);
}


void TwoDImageEventRouter::guiApplyTerrain(bool mode){
	confirmText(false);
	TwoDImageParams* dParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle mapping to terrain");
	if(mode) {
		dParams->setOrientation(2);
		dParams->setImagePlacement(0);
		placementCombo->setEnabled(false);
	} else if (!dParams->isGeoreferenced()){
		placementCombo->setEnabled(true);
	}
	dParams->setMappedToTerrain(mode);
	
	//Set box bottom and top to bottom of domain, if we are applying to terrain
	if (mode){
		float extents[6];
		DataStatus::getInstance()->getExtentsAtLevel(dParams->getNumRefinements(), extents );

		dParams->setTwoDMin(2,extents[2]);
		dParams->setTwoDMax(2,extents[2]);
	}
	
	//Reposition cursor:
	mapCursor();
	PanelCommand::captureEnd(cmd, dParams); 
	setTwoDDirty(dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,TwoDTextureBit,true);
	updateTab();
}

void TwoDImageEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1){performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentItem(0);
	performGuiCopyInstanceToViz(viznum);
}

void TwoDImageEventRouter::
setTwoDEnabled(bool val, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	TwoDImageParams* pParams = vizMgr->getTwoDImageParams(activeViz,instance);
	//Make sure this is a change:
	if (pParams->isEnabled() == val ) return;

	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val, instance);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(pParams,!val, false);
	
}


/*
 * Respond to a slider release
 */

void TwoDImageEventRouter::
twoDCenterRegion(){
	TwoDImageParams* pParams = (TwoDImageParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDImageParamsType);
	VizWinMgr::getInstance()->getRegionRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void TwoDImageEventRouter::
twoDCenterView(){
	TwoDImageParams* pParams = (TwoDImageParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDImageParamsType);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void TwoDImageEventRouter::
twoDCenterRake(){
	TwoDImageParams* pParams = (TwoDImageParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDImageParamsType);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPoint());
}

void TwoDImageEventRouter::
twoDAddSeed(){
	Point4 pt;
	TwoDImageParams* pParams = (TwoDImageParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDImageParamsType);
	mapCursor();
	pt.set3Val(pParams->getSelectedPoint());
	AnimationParams* ap = (AnimationParams*)VizWinMgr::getInstance()->getApplicableParams(Params::AnimationParamsType);
	
	pt.set1Val(3,(float)ap->getCurrentFrameNumber());
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	//Check that it's OK:
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (!fParams->isEnabled()){
		MessageReporter::warningMsg("Seed is being added to a disabled flow");
	}
	if (fParams->rakeEnabled()){
		MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the target flow is using \nrake instead of seed list");
	}
	//Check that the point is in the current Region:
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float boxMin[3], boxMax[3];
	rParams->getBox(boxMin, boxMax,-1);
	if (pt.getVal(0) < boxMin[0] || pt.getVal(1) < boxMin[1] || pt.getVal(2) < boxMin[2] ||
		pt.getVal(0) > boxMax[0] || pt.getVal(1) > boxMax[1] || pt.getVal(2) > boxMax[2]) {
			MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the seed point is outside current region");
	}
	fRouter->guiAddSeed(pt);
}	

void TwoDImageEventRouter::
setTwoDXCenter(){
	guiSetXCenter(
		xCenterSlider->value());
}
void TwoDImageEventRouter::
setTwoDYCenter(){
	guiSetYCenter(
		yCenterSlider->value());
}
void TwoDImageEventRouter::
setTwoDZCenter(){
	guiSetZCenter(
		zCenterSlider->value());
}
void TwoDImageEventRouter::
setTwoDXSize(){
	guiSetXSize(
		widthSlider->value());
}
void TwoDImageEventRouter::
setTwoDYSize(){
	guiSetYSize(
		lengthSlider->value());
}



//Make region match twoDImage.  Responds to button in region panel
void TwoDImageEventRouter::
guiCopyRegionToTwoD(){
	confirmText(false);
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "copy region to twoDImage");
	
	for (int i = 0; i< 3; i++){
		pParams->setTwoDMin(i, rParams->getRegionMin(i,timestep));
		pParams->setTwoDMax(i, rParams->getRegionMax(i,timestep));
	}
	
	updateTab();
	setTwoDDirty(pParams);
	
	PanelCommand::captureEnd(cmd,pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}



//Reinitialize TwoD tab settings, session has changed.
//Note that this is called after the globalTwoDParams are set up, but before
//any of the localTwoDParams are setup.
void TwoDImageEventRouter::
reinitTab(bool doOverride){
	
    Session *ses = Session::getInstance();
	setEnabled(true);

	seedAttached = false;

	//Set up the refinement combo:
	const Metadata* md = ses->getCurrentMetadata();
	
	int numRefinements = md->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= numRefinements; i++){
		refinementCombo->insertItem(QString::number(i));
	}
	updateTab();
}



/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * TwoDImage settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void TwoDImageEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,   bool newWindow){

	TwoDImageParams* pParams = (TwoDImageParams*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	//The actual enabled state of "this" :
	bool nowEnabled = pParams->isEnabled();
	
	if (prevEnabled == nowEnabled) return;
	
	VizWin* viz = 0;
	if(pParams->getVizNum() >= 0){//Find the viz that this applies to:
		//Note that this is only for the cases below where one particular
		//visualizer is needed
		viz = vizWinMgr->getVizWin(pParams->getVizNum());
	} 
	
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global renderer settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	pParams->setEnabled(nowEnabled);

	
	if (nowEnabled && !prevEnabled ){//For case 2:  create a renderer in the active window:


		TwoDImageRenderer* myRenderer = new TwoDImageRenderer (viz->getGLWindow(), pParams);
		viz->getGLWindow()->prependRenderer(pParams,myRenderer);

		pParams->setImagesDirty();
		return;
	}
	
	
	
	//case 6, disable 
	assert(prevEnabled && !nowEnabled); 
	viz->getGLWindow()->removeRenderer(pParams);
	return;
}


void TwoDImageEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	TwoDImageParams* pParams = VizWinMgr::getInstance()->getTwoDImageParams(winnum, instance);    
	confirmText(false);
	assert(value != pParams->isEnabled());
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "toggle twoDImage enabled",instance);
	pParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, pParams);
	//Force a rerender
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	//and refresh the gui
	updateTab();
	
	guiSetTextChanged(false);
}



//Make the twoDImage center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void TwoDImageEventRouter::guiCenterTwoD(){
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center TwoDImage to Selected Point");
	const float* selectedPoint = pParams->getSelectedPoint();
	float twoDMin[3],twoDMax[3];
	pParams->getBox(twoDMin,twoDMax);
	for (int i = 0; i<3; i++)
		textToSlider(pParams,i,selectedPoint[i], twoDMax[i]-twoDMin[i]);
	PanelCommand::captureEnd(cmd, pParams);
	updateTab();
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
//Make the probe center at selectedPoint.

void TwoDImageEventRouter::guiCenterProbe(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center Probe at Selected Point");
	const float* selectedPoint = tParams->getSelectedPoint();
	float pMin[3],pMax[3];
	pParams->getBox(pMin,pMax,-1);
	//Move center so it coincides with the selected point
	for (int i = 0; i<3; i++){
		float diff = (pMax[i]-pMin[i])*0.5;
		pMin[i] = selectedPoint[i] - diff;
		pMax[i] = selectedPoint[i] + diff; 
	}
	pParams->setBox(pMin,pMax,-1);
		
	PanelCommand::captureEnd(cmd, pParams);
	
	pParams->setProbeDirty();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}

void TwoDImageEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD X center");
	setXCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}
void TwoDImageEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD Y center");
	setYCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}
void TwoDImageEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD Z center");
	setZCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDImageEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide Planar width");
	setXSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	//setup the texture:
	resetTextureSize(pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDImageEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide Planar Height");
	setYSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	resetTextureSize(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}


void TwoDImageEventRouter::
guiSetNumRefinements(int n){
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	confirmText(false);
	int maxNumRefinements = 0;
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "set number Refinements for twoD");
	if (DataStatus::getInstance()) {
		maxNumRefinements = DataStatus::getInstance()->getNumTransforms();
		if (n > maxNumRefinements) {
			MessageReporter::warningMsg("%s","Invalid number of Refinements for current data");
			n = maxNumRefinements;
			refinementCombo->setCurrentItem(n);
		}
	} else if (n > maxNumRefinements) maxNumRefinements = n;
	pParams->setNumRefinements(n);
	PanelCommand::captureEnd(cmd, pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
	
void TwoDImageEventRouter::
guiSetOpacitySlider(int val){
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	confirmText(false);
	float sliderpos = val/256.f;
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "move opacity slider");
	pParams->setOpacMult(sliderpos);
	opacityEdit->setText(QString::number(sliderpos));
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, pParams);
	//Must rebuild all textures:
	pParams->setImagesDirty();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//Should not change values in params unless the text is invalid.
//
void TwoDImageEventRouter::
textToSlider(TwoDImageParams* pParams, int coord, float newCenter, float newSize){
	pParams->setTwoDMin(coord, newCenter-0.5f*newSize);
	pParams->setTwoDMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	return;
	//force the new center to fit in the full domain,
	
	bool centerChanged = false;
	DataStatus* ds = DataStatus::getInstance();
	const float* extents; 
	float regMin = 0.f;
	float regMax = 1.f;
	float boxMin,boxMax;
	if (ds && ds->getDataMgr()){
		extents = DataStatus::getInstance()->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	
		if (newCenter < regMin) {
			newCenter = regMin;
			centerChanged = true;
		}
		if (newCenter > regMax) {
			newCenter = regMax;
			centerChanged = true;
		}
	} else {
		regMin = newCenter - newSize*0.5f; 
		regMax = newCenter + newSize*0.5f;
	}
		
	boxMin = newCenter - newSize*0.5f; 
	boxMax= newCenter + newSize*0.5f; 
	if (centerChanged){
		pParams->setTwoDMin(coord, boxMin);
		pParams->setTwoDMax(coord, boxMax);
	}
	
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			
			oldSliderSize = widthSlider->value();
			
			oldSliderCenter = xCenterSlider->value();
			if (oldSliderSize != sliderSize){
				widthSlider->setValue(sliderSize);
			}
			
			if (oldSliderCenter != sliderCenter)
				xCenterSlider->setValue(sliderCenter);
			if(centerChanged) xCenterEdit->setText(QString::number(newCenter,'g',7));
			lastXSizeSlider = sliderSize;
			lastXCenterSlider = sliderCenter;
			break;
		case 1:
			oldSliderSize = lengthSlider->value();
			oldSliderCenter = yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				lengthSlider->setValue(sliderSize);
			
			if (oldSliderCenter != sliderCenter)
				yCenterSlider->setValue(sliderCenter);
			if(centerChanged) yCenterEdit->setText(QString::number(newCenter,'g',7));
			lastYSizeSlider = sliderSize;
			lastYCenterSlider = sliderCenter;
			break;
		case 2:
			
			oldSliderCenter = zCenterSlider->value();
			if (oldSliderCenter != sliderCenter)
				zCenterSlider->setValue(sliderCenter);
			if(centerChanged) zCenterEdit->setText(QString::number(newCenter,'g',7));
			lastZCenterSlider = sliderCenter;
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	if(centerChanged) {
		setTwoDDirty(pParams);
		twoDTextureFrame->update();
		VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	}
	update();
	return;
}
//Set text when a slider changes.
//
void TwoDImageEventRouter::
sliderToText(TwoDImageParams* pParams, int coord, int sliderVal, bool isSize){
	//Determine which coordinate of box is being slid:

	int orientation = pParams->getOrientation();
	//There are only two size sliders...
	if (isSize && orientation <= coord) coord++;
	const float* extents = DataStatus::getInstance()->getExtents();
	float center = 0.5f*(pParams->getTwoDMin(coord)+pParams->getTwoDMax(coord));
	float size = pParams->getTwoDMax(coord)- pParams->getTwoDMin(coord);
	if (isSize) 
		size = (extents[coord+3]-extents[coord])*(float)sliderVal/256.f;
	else 
		center = extents[coord] + ((float)sliderVal)*(extents[coord+3]-extents[coord])/256.f;
	
	pParams->setTwoDMin(coord, center-0.5f*size);
	pParams->setTwoDMax(coord, center+0.5f*size);
	if (isSize) adjustBoxSize(pParams);
	//Set the text in the edit boxes
	mapCursor();
	const float* selectedPoint = pParams->getSelectedPoint();
	
	switch(coord) {
		case 0:
			if (!isSize)xCenterEdit->setText(QString::number(center,'g',7));
			selectedXLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 1:
			if (!isSize)yCenterEdit->setText(QString::number(center,'g',7));
			selectedYLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 2:
			if (!isSize)zCenterEdit->setText(QString::number(center,'g',7));
			selectedZLabel->setText(QString::number(selectedPoint[coord]));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	resetTextureSize(pParams);
	update();
	//force a new render with new TwoD data
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	return;
	
}	



//Save undo/redo state when user grabs a twoD handle, or maybe a twoD face (later)
//
void TwoDImageEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	savedCommand = PanelCommand::captureStart(pParams,  "slide twoD handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshTwoDImage(pParams);
}
//The Manip class will have already changed the box?..
void TwoDImageEventRouter::
captureMouseUp(){
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	//float boxMin[3],boxMax[3];
	//pParams->getBox(boxMin,boxMax);
	//twoDTextureFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getTwoDImageParams(viznum)))
			updateTab();
	}
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	
}
//When the center slider moves, set the TwoDMin and TwoDMax
void TwoDImageEventRouter::
setXCenter(TwoDImageParams* pParams,int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(pParams,0, sliderval, false);
	
}
void TwoDImageEventRouter::
setYCenter(TwoDImageParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval, false);
	
}
void TwoDImageEventRouter::
setZCenter(TwoDImageParams* pParams,int sliderval){
	sliderToText(pParams,2, sliderval, false);
	
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void TwoDImageEventRouter::
setXSize(TwoDImageParams* pParams,int sliderval){
	sliderToText(pParams,0, sliderval,true);
	
}
void TwoDImageEventRouter::
setYSize(TwoDImageParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval,true);
}

//Save undo/redo state when user clicks cursor
//
void TwoDImageEventRouter::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(VizWinMgr::getActiveTwoDImageParams(),  "move planar cursor");
}
void TwoDImageEventRouter::
guiEndCursorMove(){
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	//Update the selected point:
	mapCursor();
	//If we are connected to a seed, move it:
	if (seedIsAttached() && attachedFlow){
		VizWinMgr::getInstance()->getFlowRouter()->guiMoveLastSeed(pParams->getSelectedPoint());
	}
	
	//Update the tab, it's in front:
	updateTab();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}


	
//Method called when undo/redo changes params.  It does the following:
//  puts the new params into the vizwinmgr, deletes the old one
//  Updates the tab if it's the current instance
//  Calls updateRenderer to rebuild renderer 
//	Makes the vizwin update.
void TwoDImageEventRouter::
makeCurrent(Params* prevParams, Params* nextParams, bool newWin, int instance,bool) {

	assert(instance >= 0);
	TwoDImageParams* pParams = (TwoDImageParams*)(nextParams->deepCopy());
	int vizNum = pParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumTwoDImageInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::TwoDImageParamsType, instance);
	
	updateTab();
	TwoDImageParams* formerParams = (TwoDImageParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled  changed:
	if (newWin || (formerParams->isEnabled() != pParams->isEnabled())){
		updateRenderer(pParams, wasEnabled,  newWin);
	}
	pParams->setImagesDirty();
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}

	

void TwoDImageEventRouter::guiNudgeXSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	
	//ignore if change is not 1 
	if(abs(val - lastXSizeSlider) != 1) {
		lastXSizeSlider = val;
		return;
	}
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge twoD X size");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 0);
	float pmin = pParams->getTwoDMin(0);
	float pmax = pParams->getTwoDMax(0);
	float maxExtent = ds->getExtents()[3];
	float minExtent = ds->getExtents()[0];
	float newSize = pmax - pmin;
	if (val > lastXSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastXSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setTwoDMin(0, pmin-voxelSize);
			pParams->setTwoDMax(0, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastXSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setTwoDMin(0, pmin+voxelSize);
			pParams->setTwoDMax(0, pmax-voxelSize);
			newSize = newSize - 2.*voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*newSize/(maxExtent-minExtent) +0.5f);
	if(lastXSizeSlider != newSliderPos){
		lastXSizeSlider = newSliderPos;
		widthSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDImageEventRouter::guiNudgeXCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastXCenterSlider) != 1) {
		lastXCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge twoD X center");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 0);
	float pmin = pParams->getTwoDMin(0);
	float pmax = pParams->getTwoDMax(0);
	float maxExtent = ds->getExtents()[3];
	float minExtent = ds->getExtents()[0];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastXCenterSlider){//move by 1 voxel, but don't move past end
		lastXCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setTwoDMin(0, pmin+voxelSize);
			pParams->setTwoDMax(0, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastXCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setTwoDMin(0, pmin-voxelSize);
			pParams->setTwoDMax(0, pmax-voxelSize);
			newCenter = (pmin+pmax)*0.5f - voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*(newCenter - minExtent)/(maxExtent-minExtent) +0.5f);
	if(lastXCenterSlider != newSliderPos){
		lastXCenterSlider = newSliderPos;
		xCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDImageEventRouter::guiNudgeYCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYCenterSlider) != 1) {
		lastYCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge twoD Y center");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 1);
	float pmin = pParams->getTwoDMin(1);
	float pmax = pParams->getTwoDMax(1);
	float maxExtent = ds->getExtents()[4];
	float minExtent = ds->getExtents()[1];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastYCenterSlider){//move by 1 voxel, but don't move past end
		lastYCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setTwoDMin(1, pmin+voxelSize);
			pParams->setTwoDMax(1, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastYCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setTwoDMin(1, pmin-voxelSize);
			pParams->setTwoDMax(1, pmax-voxelSize);
			newCenter = (pmin+pmax)*0.5f - voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*(newCenter - minExtent)/(maxExtent-minExtent) +0.5f);
	if(lastYCenterSlider != newSliderPos){
		lastYCenterSlider = newSliderPos;
		yCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDImageEventRouter::guiNudgeZCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastZCenterSlider) != 1) {
		lastZCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge twoD Z center");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 2);
	float pmin = pParams->getTwoDMin(2);
	float pmax = pParams->getTwoDMax(2);
	float maxExtent = ds->getExtents()[5];
	float minExtent = ds->getExtents()[2];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastZCenterSlider){//move by 1 voxel, but don't move past end
		lastZCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setTwoDMin(2, pmin+voxelSize);
			pParams->setTwoDMax(2, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastZCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setTwoDMin(2, pmin-voxelSize);
			pParams->setTwoDMax(2, pmax-voxelSize);
			newCenter = (pmin+pmax)*0.5f - voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*(newCenter - minExtent)/(maxExtent-minExtent) +0.5f);
	if(lastZCenterSlider != newSliderPos){
		lastZCenterSlider = newSliderPos;
		zCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}

void TwoDImageEventRouter::guiNudgeYSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYSizeSlider) != 1) {
		lastYSizeSlider = val;
		return;
	}
	confirmText(false);
	TwoDImageParams* pParams = VizWinMgr::getActiveTwoDImageParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge twoD Y size");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 1);
	float pmin = pParams->getTwoDMin(1);
	float pmax = pParams->getTwoDMax(1);
	float maxExtent = ds->getExtents()[4];
	float minExtent = ds->getExtents()[1];
	float newSize = pmax - pmin;
	if (val > lastYSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastYSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setTwoDMin(1, pmin-voxelSize);
			pParams->setTwoDMax(1, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastYSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setTwoDMin(1, pmin+voxelSize);
			pParams->setTwoDMax(1, pmax-voxelSize);
			newSize = newSize - 2.*voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*newSize/(maxExtent-minExtent) +0.5f);
	if(lastYSizeSlider != newSliderPos){
		lastYSizeSlider = newSliderPos;
		lengthSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDImageEventRouter::guiFitToRegion(){
	confirmText(false);
	TwoDImageParams* tParams = VizWinMgr::getActiveTwoDImageParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int orientation = tParams->getOrientation();
	PanelCommand* cmd = PanelCommand::captureStart(tParams,  "Fit image box to region");
	//match the non-orientation dimensions
	for (int i = 0; i<3; i++){
		if (i == orientation) continue;
		tParams->setTwoDMin(i, rParams->getRegionMin(i,ts));
		tParams->setTwoDMax(i, rParams->getRegionMax(i,ts));
	}
	if (tParams->getTwoDMin(orientation) < rParams->getRegionMin(orientation,ts)) 
		tParams->setTwoDMin(orientation,rParams->getRegionMin(orientation,ts));
	if (tParams->getTwoDMin(orientation) > rParams->getRegionMax(orientation,ts)) 
		tParams->setTwoDMin(orientation, rParams->getRegionMax(orientation,ts));


	tParams->setTwoDMax(orientation,tParams->getTwoDMin(orientation));

	PanelCommand::captureEnd(cmd, tParams);
	setTwoDDirty(tParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(tParams,TwoDTextureBit,true);

}
void TwoDImageEventRouter::
adjustBoxSize(TwoDImageParams* pParams){
	//Determine the max x, y, z sizes of twoD slice, and make sure it fits.
	int orientation = pParams->getOrientation();
	//Determine the axes associated with width and length
	int xcrd = 0, ycrd = 1;
	if (orientation < 2) ycrd++;
	if (orientation < 1) xcrd++;
	
	float boxmin[3], boxmax[3];
	//Don't do anything if we haven't read the data yet:
	if (!Session::getInstance()->getDataMgr()) return;
	pParams->getBox(boxmin, boxmax);

	const float* extents = DataStatus::getInstance()->getExtents();
	//In image mode, just make box have nonnegative extent

	
	if (boxmin[xcrd]> boxmax[xcrd]) boxmax[xcrd] = boxmin[xcrd];
	if (boxmin[ycrd]> boxmax[ycrd]) boxmax[ycrd] = boxmin[ycrd];

	//Set the box to have size 0 in the orientation coordinate:

	if (boxmin[orientation] != boxmax[orientation]){
		float mid = 0.5f*(boxmin[orientation]+boxmax[orientation]);
		boxmin[orientation] = boxmax[orientation] = mid;
	}
	
	pParams->setBox(boxmin, boxmax);
	widthEdit->setText(QString::number(boxmax[xcrd]-boxmin[xcrd]));
	lengthEdit->setText(QString::number(boxmax[ycrd]-boxmin[ycrd]));
	//If the box is bigger than the extents, just put the sliders at the
	//max value
	float xsize = (boxmax[xcrd]-boxmin[xcrd])/(extents[xcrd+3]-extents[xcrd]);
	float ysize = (boxmax[ycrd]-boxmin[ycrd])/(extents[ycrd+3]-extents[ycrd]);
	if (xsize > 1.f) xsize = 1.f;
	if (ysize > 1.f) ysize = 1.f;
	widthSlider->setValue((int)(256.f*xsize));
	lengthSlider->setValue((int)(256.f*ysize));
	guiSetTextChanged(false);
	return;
	
}
void TwoDImageEventRouter::resetTextureSize(TwoDImageParams* twoDParams){
	//setup the texture size.  If the image has been read, get the dimensions 
	//from the image:
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	if (twoDParams->getCurrentTwoDTexture(timestep)){
		int txsize[2];
		twoDParams->getTextureSize(txsize, timestep);
		twoDTextureFrame->setTextureSize((float)txsize[0],(float)txsize[1]);
		return;
	}
	//Otherwise just do a square
	twoDTextureFrame->setTextureSize(1.f,1.f);
}



// Map the cursor coords into world space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void TwoDImageEventRouter::mapCursor(){
	//If the scene and image are georeferenced we do this differently than
	//if not.
	
	//Get the transform 
	TwoDImageParams* tParams = VizWinMgr::getInstance()->getActiveTwoDImageParams();
	if(!DataStatus::getInstance()->getDataMgr()) return;
	float selectPoint[3];
	const float* cursorCoords = tParams->getCursorCoords();
	
	if (tParams->isGeoreferenced()){
		
		double mappt[2];
		mappt[0] = -cursorCoords[0];
		mappt[1] = cursorCoords[1];
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
		if (tParams->mapGeorefPoint(timestep, mappt)) {
			selectPoint[0] = mappt[0];
			selectPoint[1] = mappt[1];
			selectPoint[2] = tParams->getTwoDMin(2);
		}
		else
			for (int i = 0; i< 3; i++) selectPoint[i] = 0.f;
	}
		
	else { //map linearly into volume
		float twoDCoord[3];
		float a[2],b[2],constVal[2];
		int mapDims[3];
		tParams->build2DTransform(a,b,constVal,mapDims);
		
		//If using flat plane, the cursor sits in the z=0 plane of the twoD box coord system.
		//x is reversed because we are looking from the opposite direction 
		twoDCoord[0] = -cursorCoords[0];
		twoDCoord[1] = cursorCoords[1];
		twoDCoord[2] = 0.f;
	
		selectPoint[mapDims[0]] = twoDCoord[0]*a[0]+b[0];
		selectPoint[mapDims[1]] = twoDCoord[1]*a[1]+b[1];
		selectPoint[mapDims[2]] = constVal[0];

	}



	if (tParams->isMappedToTerrain()) {
		//Find terrain height at selected point:
		const float* extents = DataStatus::getInstance()->getExtents();
		int varnum = DataStatus::getInstance()->getSessionVariableNum2D("HGT");
		if (varnum >= 0){
			float val = calcCurrentValue(tParams,selectPoint,&varnum, 1);
			if (val != OUT_OF_BOUNDS){
				selectPoint[2] = val+(tParams->getTwoDMin(2)-extents[2]);
			}
		}
	} 
	tParams->setSelectedPoint(selectPoint);
}


void TwoDImageEventRouter::guiChangeInstance(int inst){
	performGuiChangeInstance(inst);
}
void TwoDImageEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void TwoDImageEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void TwoDImageEventRouter::
setTwoDTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void TwoDImageEventRouter::
twoDReturnPressed(void){
	confirmText(true);
}

void TwoDImageEventRouter::
twoDAttachSeed(bool attach){
	if (attach) twoDAddSeed();
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::FlowParamsType);
	
	guiAttachSeed(attach, fParams);
}

void TwoDImageEventRouter::refreshGLWindow()
{
	if (twoDTextureFrame->getGLWindow()->isRendering()) return;
	twoDTextureFrame->getGLWindow()->updateGL();
}
