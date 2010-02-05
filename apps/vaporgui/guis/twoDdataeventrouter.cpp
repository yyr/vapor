//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDdataeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Implements the TwoDDataEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the TwoDData tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
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
#include <qfileinfo.h>
#include <QFileDialog>
#include <qlabel.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qtooltip.h>
#include <qlayout.h>
#include "twoDdatarenderer.h"
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
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "twoDdatatab.h"
#include "vaporinternal/jpegapi.h"
#include "vapor/XmlNode.h"
#include "GetAppPath.h"
#include "tabmanager.h"
#include "glutil.h"
#include "twoDdataparams.h"
#include "twoDdataeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "VolumeRenderer.h"

using namespace VAPoR;


TwoDDataEventRouter::TwoDDataEventRouter(QWidget* parent,const char* ): QWidget(parent), Ui_TwoDDataTab(), TwoDEventRouter(){
	setupUi(this);
	myParamsType = Params::TwoDDataParamsType;
	savedCommand = 0;
	ignoreListboxChanges = false;
	numVariables = 0;
	seedAttached = false;
	
	MessageReporter::infoMsg("TwoDDataEventRouter::TwoDDataEventRouter()");
}


TwoDDataEventRouter::~TwoDDataEventRouter(){
}
/**********************************************************
 * Whenever a new TwoDDatatab is created it must be hooked up here
 ************************************************************/
void
TwoDDataEventRouter::hookUpTab()
{
	//Nudge sliders by clicking on slider bar:
	connect (xSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXSize(int)));
	connect (xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXCenter(int)));
	connect (ySizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYSize(int)));
	connect (yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYCenter(int)));
	
	connect (zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeZCenter(int)));
	
	connect (xCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (yCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (zCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (xSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (ySizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (leftMappingBound, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (rightMappingBound, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	
	connect (xCenterEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (yCenterEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (zCenterEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (xSizeEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (ySizeEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	
	connect (histoScaleEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	connect (regionCenterButton, SIGNAL(clicked()), this, SLOT(twoDCenterRegion()));
	connect (viewCenterButton, SIGNAL(clicked()), this, SLOT(twoDCenterView()));
	connect (rakeCenterButton, SIGNAL(clicked()), this, SLOT(twoDCenterRake()));
	connect (probeCenterButton, SIGNAL(clicked()), this, SLOT(guiCenterProbe()));
	connect (addSeedButton, SIGNAL(clicked()), this, SLOT(twoDAddSeed()));
	connect (applyTerrainCheckbox, SIGNAL(toggled(bool)),this, SLOT(guiApplyTerrain(bool)));
	connect (attachSeedCheckbox,SIGNAL(toggled(bool)),this, SLOT(twoDAttachSeed(bool)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (variableListBox,SIGNAL(itemSelectionChanged(void)), this, SLOT(guiChangeVariables(void)));
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDZCenter()));
	connect (xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDXSize()));
	connect (ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setTwoDYSize()));
	connect (loadButton, SIGNAL(clicked()), this, SLOT(twoDLoadTF()));
	connect (loadInstalledButton, SIGNAL(clicked()), this, SLOT(twoDLoadInstalledTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(twoDSaveTF()));
	connect (captureButton, SIGNAL(clicked()), this, SLOT(captureImage()));
	connect (leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setTwoDTabTextChanged(const QString&)));
	connect (opacityScaleSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetOpacityScale(int)));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setTwoDNavigateMode(bool)));
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setTwoDEditMode(bool)));
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshTwoDHisto()));
	
	// Transfer function controls:
	connect(editButton, SIGNAL(toggled(bool)), 
            transferFunctionFrame, SLOT(setEditMode(bool)));

	connect(alignButton, SIGNAL(clicked()),
            transferFunctionFrame, SLOT(fitToView()));

    connect(transferFunctionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeMapFcn(QString)));

    connect(transferFunctionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeMapFcn()));

    connect(transferFunctionFrame, SIGNAL(canBindControlPoints(bool)),
            this, SLOT(setBindButtons(bool)));

	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setTwoDEnabled(bool,int)));
	connect (orientationCombo, SIGNAL(activated(int)), this, SLOT(guiSetOrientation(int)));
	connect (fitToRegionButton, SIGNAL(clicked()), this, SLOT(guiFitToRegion()));
	connect (fitDataButton, SIGNAL(clicked()), this, SLOT(guiFitTFToData()));
}
//Insert values from params into tab panel
//
void TwoDDataEventRouter::updateTab(){
	if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
	DataStatus* ds = DataStatus::getInstance();
	if (isEnabled() && ds->getDataMgr()) 
			instanceTable->setEnabled(true);
	else {
		instanceTable->setEnabled(false);
	}
	instanceTable->rebuild(this);
	TwoDDataParams* twoDParams = VizWinMgr::getActiveTwoDDataParams();
	
	if (!isEnabled()) {
		transferFunctionFrame->setMapperFunction(0); 
		return;
	}
	if (GLWindow::isRendering()) return;
	guiSetTextChanged(false);
	setIgnoreBoxSliderEvents(true);  //don't generate nudge events

	
	instanceTable->rebuild(this);
	
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int currentTimeStep = vizMgr->getActiveAnimationParams()->getCurrentFrameNumber();
	int winnum = vizMgr->getActiveViz();
	
	
	int orientation = 2; //x-y aligned
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
	
	captureButton->setEnabled(twoDParams->isEnabled());

	//orientation = ds->get2DOrientation(twoDParams->getFirstVarNum());
	orientationCombo->setCurrentIndex(orientation);
	orientationCombo->setEnabled(false);
	if (twoDParams->getMapperFunc()){
		twoDParams->getMapperFunc()->setParams(twoDParams);
		transferFunctionFrame->setMapperFunction(twoDParams->getMapperFunc());
	}
	transferFunctionFrame->updateParams();
	int numvars = 0;
	QString varnames = getMappedVariableNames(&numvars);
	QString shortVarNames = varnames;
	if(shortVarNames.length() > 12){
		shortVarNames.resize(12);
		shortVarNames.append("...");
	}
	if (numvars > 1)
	{
	transferFunctionFrame->setVariableName(varnames.toStdString());
	QString labelString = "Length of vector("+shortVarNames+") at selected point:";
	variableLabel->setText(labelString);
	}
	else if (numvars > 0)
	{
	transferFunctionFrame->setVariableName(varnames.toStdString());
	QString labelString = "Value of variable("+shortVarNames+") at selected point:";
	variableLabel->setText(labelString);
	}
	else
	{
	transferFunctionFrame->setVariableName("");
	variableLabel->setText("");
	}
	int numRefs = twoDParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentIndex(numRefs);
	
	histoScaleEdit->setText(QString::number(twoDParams->GetHistoStretch()));
	//List the variables in the combo
	int* sessionVarNums = new int[numVariables];
	int totVars = 0;
	for (int varnum = 0; varnum < ds->getNumSessionVariables2D(); varnum++){
		if (!twoDParams->variableIsSelected(varnum)) continue;
		sessionVarNums[totVars++] = varnum;
	}
	float val = OUT_OF_BOUNDS;
	if (numVariables > 0) {val = calcCurrentValue(twoDParams,selectedPoint,sessionVarNums, totVars);
		delete sessionVarNums;
	}
	if (val == OUT_OF_BOUNDS)
		valueMagLabel->setText(QString(" "));
	else valueMagLabel->setText(QString::number(val));
	guiSetTextChanged(false);
	//Set the selection in the variable listbox.
	//Turn off listBox message-listening
	ignoreListboxChanges = true;
	for (int i = 0; i< ds->getNumMetadataVariables2D(); i++){
		if (variableListBox->item(i)->isSelected() != twoDParams->variableIsSelected(ds->mapMetadataToSessionVarNum2D(i)))
			variableListBox->item(i)->setSelected(twoDParams->variableIsSelected(ds->mapMetadataToSessionVarNum2D(i)));
	}
	ignoreListboxChanges = false;

	updateBoundsText(twoDParams);
	guiSetTextChanged(false);
	float sliderVal = twoDParams->getOpacityScale();
	
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	
	//Set the mode buttons:
	
	if (twoDParams->getEditMode()){
		
		editButton->setDown(true);
		navigateButton->setDown(false);
	} else {
		editButton->setDown(false);
		navigateButton->setDown(true);
	}
	
	
	deleteInstanceButton->setEnabled(vizMgr->getNumTwoDDataInstances(winnum) > 1);
	
	int numViz = vizMgr->getNumVisualizers();

	copyCombo->clear();
	copyCombo->addItem("Duplicate In:");
	copyCombo->addItem("This visualizer");
	if (numViz > 1) {
		int copyNum = 2;
		for (int i = 0; i<MAXVIZWINS; i++){
			if (vizMgr->getVizWin(i) && winnum != i){
				copyCombo->addItem(vizMgr->getVizWinName(i));
				//Remember the viznum corresponding to a combo item:
				copyCount[copyNum++] = i;
			}
		}
	}

	//setup the texture:
	
	resetTextureSize(twoDParams);
	
	QString strn;
	Session* ses = Session::getInstance();
	ses->blockRecording();

    
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

	const DataMgr *dataMgr = ds->getDataMgr();
	if (dataMgr){
		int fullRefLevel = ds->getNumTransforms();

		double dBoxMin[3], dBoxMax[3];
		size_t gridMin[3],gridMax[3];
		for (int i = 0; i<3; i++) {
			dBoxMin[i] = boxmin[i];
			dBoxMax[i] = boxmax[i];
		}
		dataMgr->MapUserToVox((size_t)-1, dBoxMin, gridMin, fullRefLevel);
		dataMgr->MapUserToVox((size_t)-1, dBoxMax, gridMax, fullRefLevel);
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
	guiSetTextChanged(false);
	//Only allow terrain map with horizontal orientation, and if
	//HGT variable is there
	if (orientation != 2) {
		applyTerrainCheckbox->setEnabled(false);
		applyTerrainCheckbox->setChecked(false);
	} else {
		int varnum = ds->getSessionVariableNum2D("HGT");
		if (varnum < 0 || !ds->dataIsPresent2D(varnum,currentTimeStep)){
			applyTerrainCheckbox->setEnabled(false);
			applyTerrainCheckbox->setChecked(false);
		} else {
			bool terrainMap = twoDParams->isMappedToTerrain();
			if (terrainMap != applyTerrainCheckbox->isChecked())
				applyTerrainCheckbox->setChecked(terrainMap);
			applyTerrainCheckbox->setEnabled(true);
		}
	}
	twoDTextureFrame->setParams(twoDParams, true);
	
	vizMgr->getTabManager()->update();
	twoDTextureFrame->update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	
	setIgnoreBoxSliderEvents(false);
}
//Fix for clean Windows scrolling:
void TwoDDataEventRouter::refreshTab(){
	 
	twoDFrameHolder->hide();
	twoDFrameHolder->show();
	
}

void TwoDDataEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	TwoDDataParams* twoDParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(twoDParams, "edit TwoDData text");
	QString strn;
	
	twoDParams->setHistoStretch(histoScaleEdit->text().toFloat());

	
	int orientation = DataStatus::getInstance()->get2DOrientation(twoDParams->getFirstVarNum());
	int xcrd =0, ycrd = 1, zcrd = 2;
	if (orientation < 2) {ycrd = 2; zcrd = 1;}
	if (orientation < 1) {xcrd = 1; zcrd = 0;}

	const float *extents = DataStatus::getInstance()->getExtents();
	//Set the twoD size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[xcrd] = xSizeEdit->text().toFloat();
	boxSize[ycrd] = ySizeEdit->text().toFloat();
	boxSize[zcrd] = 0;
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
		if (boxSize[i] > (extents[i+3]-extents[i])) boxSize[i] = (extents[i+3]-extents[i]);
	}
	boxCenter[0] = xCenterEdit->text().toFloat();
	boxCenter[1] = yCenterEdit->text().toFloat();
	boxCenter[2] = zCenterEdit->text().toFloat();
	twoDParams->getBox(boxmin, boxmax);
	//the box z-size is not adjustable:
	boxSize[zcrd] = boxmax[zcrd]-boxmin[zcrd];
	for (int i = 0; i<3;i++){
		
		if (boxCenter[i] < extents[i])boxCenter[i] = extents[i];
		if (boxCenter[i] > extents[i+3])boxCenter[i] = extents[i+3];
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
	if (twoDParams->getMapperFunc()) {
		((TransferFunction*)twoDParams->getMapperFunc())->setMinMapValue(leftMappingBound->text().toFloat());
		((TransferFunction*)twoDParams->getMapperFunc())->setMaxMapValue(rightMappingBound->text().toFloat());
	
		setDatarangeDirty(twoDParams);
		setEditorDirty();
		update();
		twoDTextureFrame->update();
	}
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(twoDParams,TwoDTextureBit,true);
	//If we are in twoD mode, force a rerender of all windows using the twoD:
	if (GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode){
		VizWinMgr::getInstance()->refreshTwoDData(twoDParams);
	}
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, twoDParams);
}


/*********************************************************************************
 * Slots associated with TwoDDataTab:
 *********************************************************************************/



void TwoDDataEventRouter::guiSetOrientation(int val){
	return;  //can't set this yet
	
}

void TwoDDataEventRouter::guiApplyTerrain(bool mode){
	confirmText(false);
	TwoDDataParams* dParams = VizWinMgr::getActiveTwoDDataParams();
	if (mode == dParams->isMappedToTerrain()) return;
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle mapping to terrain");
	float extents[6];
	DataStatus::getInstance()->getExtentsAtLevel(dParams->getNumRefinements(), extents);
	if (dParams->isEnabled()) {
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		int viznum = vizMgr->getActiveViz();
		float disp; 
		if (mode) disp = extents[2];
		else disp = dParams->getTwoDMin(2);
		//Check that we aren't putting this on another planar surface:
		if (vizMgr->findCoincident2DSurface(viznum, 2, 
			disp, mode))
		{
			MessageReporter::warningMsg("This 2D data surface is close to another enabled 2D surface.\n%s\n",
					"Change the 2D data position in order to avoid rendering defects");
		}
	}
	dParams->setMappedToTerrain(mode);
	
	//If setting to terrain, set box bottom and top to bottom of domain
	if (mode){
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

void TwoDDataEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1){performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentIndex(0);
	performGuiCopyInstanceToViz(viznum);
}

void TwoDDataEventRouter::
setTwoDEnabled(bool val, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	TwoDDataParams* pParams = vizMgr->getTwoDDataParams(activeViz,instance);
	//Make sure this is a change:
	if (pParams->isEnabled() == val ) return;
	//Make sure it's oriented horizontally:
	
	int orientation = DataStatus::getInstance()->get2DOrientation(pParams->getFirstVarNum());
	if (orientation != 2) {
		MessageReporter::errorMsg("Display of non-horizontal \n2D variables not supported");
		return;
	}

	//If we are enabling, also make this the current instance:
	if (val) {
		if (vizMgr->findCoincident2DSurface(activeViz, orientation, pParams->getTwoDMin(orientation),
			pParams->isMappedToTerrain())){
				MessageReporter::warningMsg("This 2D data surface is close to another enabled 2D surface.\n%s\n",
					"Change the 2D data position in order to avoid rendering defects");
			}
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val, instance);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(pParams,!val, false);
	setDatarangeDirty(pParams);
}

void TwoDDataEventRouter::
setTwoDEditMode(bool mode){
	navigateButton->setDown(!mode);
	guiSetEditMode(mode);
}
void TwoDDataEventRouter::
setTwoDNavigateMode(bool mode){
	editButton->setDown(!mode);
	guiSetEditMode(!mode);
}

void TwoDDataEventRouter::
refreshTwoDHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	if (pParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(pParams);
	}
	setEditorDirty();
}

void TwoDDataEventRouter::
twoDLoadInstalledTF(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	TransferFunction* tf = pParams->getTransFunc();
	if (!tf) return;
	float minb = tf->getMinMapValue();
	float maxb = tf->getMaxMapValue();
	if (minb >= maxb){ minb = 0.0; maxb = 1.0;}

	//Get the path from the environment:
    vector <string> paths;
    paths.push_back("palettes");
    string palettes = GetAppPath("VAPOR", "share", paths);

	QString installPath = palettes.c_str();
	fileLoadTF(pParams, pParams->getSessionVarNum(), (const char *) installPath.toAscii(),false);

	tf = pParams->getTransFunc();
	tf->setMinMapValue(minb);
	tf->setMaxMapValue(maxb);
	setEditorDirty();
	updateClut(pParams);
}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the TwoDData params
void TwoDDataEventRouter::
twoDSaveTF(void){
	TwoDDataParams* dParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	saveTF(dParams);
}
void TwoDDataEventRouter::
twoDLoadTF(void){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	loadTF(pParams, pParams->getSessionVarNum());
	updateClut(pParams);
}
void TwoDDataEventRouter::
twoDCenterRegion(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	VizWinMgr::getInstance()->getRegionRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void TwoDDataEventRouter::
twoDCenterView(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void TwoDDataEventRouter::
twoDCenterRake(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPoint());
}

void TwoDDataEventRouter::
twoDAddSeed(){
	Point4 pt;
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDDataParamsType);
	mapCursor();
	pt.set3Val(pParams->getSelectedPoint());
	AnimationParams* ap = (AnimationParams*)VizWinMgr::getInstance()->getApplicableParams(Params::AnimationParamsType);
	int timestep = ap->getCurrentFrameNumber();
	pt.set1Val(3,(float)timestep);
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
	rParams->getBox(boxMin, boxMax,timestep);
	if (pt.getVal(0) < boxMin[0] || pt.getVal(1) < boxMin[1] || pt.getVal(2) < boxMin[2] ||
		pt.getVal(0) > boxMax[0] || pt.getVal(1) > boxMax[1] || pt.getVal(2) > boxMax[2]) {
			MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the seed point is outside current region");
	}
	fRouter->guiAddSeed(pt);
}	

void TwoDDataEventRouter::
setTwoDXCenter(){
	guiSetXCenter(
		xCenterSlider->value());
}
void TwoDDataEventRouter::
setTwoDYCenter(){
	guiSetYCenter(
		yCenterSlider->value());
}
void TwoDDataEventRouter::
setTwoDZCenter(){
	guiSetZCenter(
		zCenterSlider->value());
}
void TwoDDataEventRouter::
setTwoDXSize(){
	guiSetXSize(
		xSizeSlider->value());
}
void TwoDDataEventRouter::
setTwoDYSize(){
	guiSetYSize(
		ySizeSlider->value());
}


//Respond to user request to load/save TF
//Assumes name is valid
//
void TwoDDataEventRouter::
sessionLoadTF(QString* name){
	
	confirmText(false);
	TwoDDataParams* dParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->toStdString());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getSessionVarNum();
	dParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	setDatarangeDirty(dParams);
	setEditorDirty();
}

//Make region match twoD.  Responds to button in region panel
void TwoDDataEventRouter::
guiCopyRegionToTwoD(){
	confirmText(false);
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "copy region to TwoDData");
	
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



//Reinitialize TwoDData tab settings, session has changed.
//Note that this is called after the globalTwoDDataParams are set up, but before
//any of the localTwoDDataParams are setup.
void TwoDDataEventRouter::
reinitTab(bool doOverride){
	setIgnoreBoxSliderEvents(false);
    Session *ses = Session::getInstance();
	if (DataStatus::getInstance()->dataIsPresent2D()&&!ses->sphericalTransform()) setEnabled(true);
	else setEnabled(false);


	numVariables = DataStatus::getInstance()->getNumSessionVariables2D();
	//Set the names in the variable listbox
	ignoreListboxChanges = true;
	variableListBox->clear();
	for (int i = 0; i< DataStatus::getInstance()->getNumMetadataVariables2D(); i++){
		const std::string& s = DataStatus::getInstance()->getMetadataVarName2D(i);
		const QString& text = QString(s.c_str());
		QListWidgetItem* newItem = new QListWidgetItem(text);
		variableListBox->insertItem(i,newItem);
	}
	ignoreListboxChanges = false;

	seedAttached = false;

	//Set up the refinement combo:
	const DataMgr *dataMgr = ses->getDataMgr();
	
	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= numRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	if (histogramList) {
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete histogramList;
		histogramList = 0;
		numHistograms = 0;
	}
	setBindButtons(false);
	updateTab();
}
//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void TwoDDataEventRouter::
guiSetEditMode(bool mode){
	confirmText(false);
	TwoDDataParams* dParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set edit/navigate mode");
	dParams->setEditMode(mode);
	PanelCommand::captureEnd(cmd, dParams); 
}


/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * TwoDData settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void TwoDDataEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,   bool newWindow){

	TwoDDataParams* pParams = (TwoDDataParams*)rParams;
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


		TwoDDataRenderer* myRenderer = new TwoDDataRenderer (viz->getGLWindow(), pParams);
		viz->getGLWindow()->insertSortedRenderer(pParams,myRenderer);

		setTwoDDirty(pParams);
		return;
	}
	
	
	
	//case 6, disable 
	assert(prevEnabled && !nowEnabled); 
	viz->getGLWindow()->removeRenderer(pParams);
	return;
}
void TwoDDataEventRouter::
setEditorDirty(RenderParams* p){
	TwoDDataParams* dp = (TwoDDataParams*)p;
	if(!dp) dp = VizWinMgr::getInstance()->getActiveTwoDDataParams();
	if(dp->getMapperFunc())dp->getMapperFunc()->setParams(dp);
    transferFunctionFrame->setMapperFunction(dp->getMapperFunc());
    transferFunctionFrame->updateParams();

	if (DataStatus::getInstance()->getNumSessionVariables2D())
    {
	  int tmp;
      transferFunctionFrame->setVariableName(getMappedVariableNames(&tmp).toStdString());
    }
    else
    {
      transferFunctionFrame->setVariableName("");
    }
	if(dp) {
		MapperFunction* mf = dp->getMapperFunc();
		if (mf) {
			leftMappingBound->setText(QString::number(mf->getMinOpacMapValue()));
			rightMappingBound->setText(QString::number(mf->getMaxOpacMapValue()));
		}
	}
}


void TwoDDataEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	TwoDDataParams* pParams = VizWinMgr::getInstance()->getTwoDDataParams(winnum, instance);    
	confirmText(false);
	assert(value != pParams->isEnabled());
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "toggle TwoDData enabled",instance);
	pParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, pParams);
	
	//Need to rerender the texture:
	pParams->setTwoDDirty();
	//and refresh the gui
	updateTab();
	setDatarangeDirty(pParams);
	setEditorDirty();
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	update();
	guiSetTextChanged(false);
}


//Respond to a change in opacity scale factor
void TwoDDataEventRouter::
guiSetOpacityScale(int val){
	TwoDDataParams* pp = VizWinMgr::getActiveTwoDDataParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(pp, "modify opacity scale slider");
	pp->setOpacityScale( ((float)(256-val))/256.f);
	float sliderVal = pp->getOpacityScale();
	
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal));

	setTwoDDirty(pp);
	twoDTextureFrame->update();
	
	PanelCommand::captureEnd(cmd,pp);
	
	VizWinMgr::getInstance()->setVizDirty(pp,TwoDTextureBit,true);
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void TwoDDataEventRouter::
guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	TwoDDataParams* pp = VizWinMgr::getInstance()->getActiveTwoDDataParams();
    savedCommand = PanelCommand::captureStart(pp, qstr.toLatin1());
}
void TwoDDataEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand::captureEnd(savedCommand,pParams);
	savedCommand = 0;
	setTwoDDirty(pParams);
	setDatarangeDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}

void TwoDDataEventRouter::
guiBindColorToOpac(){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Color to Opacity");
    transferFunctionFrame->bindColorToOpacity();
	PanelCommand::captureEnd(cmd, pParams);
}
void TwoDDataEventRouter::
guiBindOpacToColor(){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Opacity to Color");
    transferFunctionFrame->bindOpacityToColor();
	PanelCommand::captureEnd(cmd, pParams);
}
//Make the TwoDData center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void TwoDDataEventRouter::guiCenterTwoD(){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center TwoDData to Selected Point");
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

void TwoDDataEventRouter::guiCenterProbe(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	TwoDDataParams* tParams = VizWinMgr::getActiveTwoDDataParams();
	
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

//Respond to an update of the variable listbox.  set the appropriate bits
void TwoDDataEventRouter::
guiChangeVariables(){
	//Don't react if the listbox is being reset programmatically:
	if (ignoreListboxChanges) return;
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "change TwoDData-selected variable(s)");
	int firstVar = -1;
	int numSelected = 0;
	//Session* ses = Session::getInstance();
	int orientation = 0;
	for (int i = 0; i< DataStatus::getInstance()->getNumMetadataVariables2D(); i++){
		//Index by session variable num:
		int varnum = DataStatus::getInstance()->mapMetadataToSessionVarNum2D(i);
		if (variableListBox->item(i)->isSelected()){
			pParams->setVariableSelected(varnum,true);
			
			if(firstVar == -1) {
				firstVar = varnum;
				orientation = DataStatus::getInstance()->get2DOrientation(i);
			} else if (orientation != DataStatus::getInstance()->get2DOrientation(i)){
				//De-select any variables that don't match the first variable's orientation
				variableListBox->item(i)->setSelected(false);
				numSelected--;
			}
			numSelected++;
		}
		else 
			pParams->setVariableSelected(varnum,false);
	}
	//If nothing is selected, select the first one:
	if (firstVar == -1) {
		firstVar = 0;
		numSelected = 1;
		orientation = DataStatus::getInstance()->get2DOrientation(0);
	}
	pParams->setNumVariablesSelected(numSelected);
	pParams->setFirstVarNum(firstVar);
	orientationCombo->setCurrentIndex(orientation);
	
	//Only allow terrain map with horizontal orientation
	if (orientation != 2) {
		pParams->setMappedToTerrain(false);
		applyTerrainCheckbox->setEnabled(false);
		applyTerrainCheckbox->setChecked(false);
		
	} else {
		bool terrainMap = pParams->isMappedToTerrain();
		applyTerrainCheckbox->setChecked(terrainMap);
		applyTerrainCheckbox->setEnabled(true);
	}
	//reset the editing display range shown on the tab, 
	//this also sets dirty flag
	updateMapBounds(pParams);
	
	//Force a redraw of the transfer function frame
    setEditorDirty();
	   
	
	PanelCommand::captureEnd(cmd, pParams);

	//Need to update the selected point for the new variables
	updateTab();
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDDataEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide TwoDData X center");
	setXCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}
void TwoDDataEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide TwoDData Y center");
	setYCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}
void TwoDDataEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide TwoDData Z center");
	setZCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDDataEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide Planar width");
	setXSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	//setup the texture:
	resetTextureSize(pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDDataEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide Planar Height");
	setYSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDDataEventRouter::guiFitToRegion(){
	confirmText(false);
	TwoDDataParams* tParams = VizWinMgr::getActiveTwoDDataParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	PanelCommand* cmd = PanelCommand::captureStart(tParams,  "Fit 2D data to region");
	//Just match the first two dimensions:
	for (int i = 0; i<2; i++){
		tParams->setTwoDMin(i, rParams->getRegionMin(i,ts));
		tParams->setTwoDMax(i, rParams->getRegionMax(i,ts));
	}
	if (tParams->getTwoDMin(2) < rParams->getRegionMin(2,ts)) 
		tParams->setTwoDMin(2,rParams->getRegionMin(2,ts));
	if (tParams->getTwoDMin(2) > rParams->getRegionMax(2,ts)) 
		tParams->setTwoDMin(2, rParams->getRegionMax(2,ts));

	tParams->setTwoDMax(2,tParams->getTwoDMin(2));

	PanelCommand::captureEnd(cmd, tParams);
	setTwoDDirty(tParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(tParams,TwoDTextureBit,true);

}

void TwoDDataEventRouter::
guiSetNumRefinements(int n){
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	confirmText(false);
	int maxNumRefinements = 0;
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "set number Refinements for twoD");
	if (DataStatus::getInstance()) {
		maxNumRefinements = DataStatus::getInstance()->getNumTransforms();
		if (n > maxNumRefinements) {
			MessageReporter::warningMsg("%s","Invalid number of Refinements for current data");
			n = maxNumRefinements;
			refinementCombo->setCurrentIndex(n);
		}
	} else if (n > maxNumRefinements) maxNumRefinements = n;
	pParams->setNumRefinements(n);
	PanelCommand::captureEnd(cmd, pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
	
//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//Should not change values in params unless the text is invalid.
//
void TwoDDataEventRouter::
textToSlider(TwoDDataParams* pParams, int coord, float newCenter, float newSize){
	setIgnoreBoxSliderEvents(true);
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
			
			oldSliderSize = xSizeSlider->value();
			
			oldSliderCenter = xCenterSlider->value();
			if (oldSliderSize != sliderSize){
				xSizeSlider->setValue(sliderSize);
			}
			
			if (oldSliderCenter != sliderCenter)
				xCenterSlider->setValue(sliderCenter);
			if(centerChanged) xCenterEdit->setText(QString::number(newCenter,'g',7));
			lastXSizeSlider = sliderSize;
			lastXCenterSlider = sliderCenter;
			break;
		case 1:
			oldSliderSize = ySizeSlider->value();
			oldSliderCenter = yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				ySizeSlider->setValue(sliderSize);
			
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
	setIgnoreBoxSliderEvents(false);
	return;
}
//Set text when a slider changes.
//
void TwoDDataEventRouter::
sliderToText(TwoDDataParams* pParams, int coord, int slideCenter, int slideSize){
	//Determine which coordinate of box is being slid:
	int orientation = DataStatus::getInstance()->get2DOrientation(pParams->getFirstVarNum());
	if (orientation < coord) coord++;
	const float* extents = DataStatus::getInstance()->getExtents();
	
	float newCenter = extents[coord] + ((float)slideCenter)*(extents[coord+3]-extents[coord])/256.f;
	float newSize = (extents[coord+3]-extents[coord])*(float)slideSize/256.f;
	//If it's not inside the domain, move the center:
	if ((newCenter - 0.5*newSize) < extents[coord]) newCenter = extents[coord]+0.5*newSize;
	if ((newCenter + 0.5*newSize) > extents[coord+3]) newCenter = extents[coord+3]-0.5*newSize;

	pParams->setTwoDMin(coord, newCenter-0.5f*newSize);
	pParams->setTwoDMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	//Set the text in the edit boxes
	mapCursor();
	const float* selectedPoint = pParams->getSelectedPoint();
	
	switch(coord) {
		case 0:
			xSizeEdit->setText(QString::number(newSize,'g',7));
			xCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedXLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 1:
			ySizeEdit->setText(QString::number(newSize,'g',7));
			yCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedYLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 2:
			zCenterEdit->setText(QString::number(newCenter,'g',7));
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
/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the TFE editor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void TwoDDataEventRouter::
updateMapBounds(RenderParams* params){
	TwoDDataParams* twoDDataParams = (TwoDDataParams*)params;
	
	updateBoundsText(params);
	setTwoDDirty(twoDDataParams);
	setDatarangeDirty(twoDDataParams);
	setEditorDirty();
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(twoDDataParams,TwoDTextureBit,true);
	
}

void TwoDDataEventRouter::setBindButtons(bool canbind)
{
  OpacityBindButton->setEnabled(canbind);
  ColorBindButton->setEnabled(canbind);
}

//Save undo/redo state when user grabs a twoD handle, or maybe a twoD face (later)
//
void TwoDDataEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	savedCommand = PanelCommand::captureStart(pParams,  "slide twoD handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshTwoDData(pParams);
}
//The Manip class will have already changed the box?..
void TwoDDataEventRouter::
captureMouseUp(){
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	//float boxMin[3],boxMax[3];
	//pParams->getBox(boxMin,boxMax);
	//twoDTextureFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getTwoDDataParams(viznum)))
			updateTab();
	}
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	
}
//When the center slider moves, set the TwoDMin and TwoDMax
void TwoDDataEventRouter::
setXCenter(TwoDDataParams* pParams,int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(pParams,0, sliderval, xSizeSlider->value());
	
}
void TwoDDataEventRouter::
setYCenter(TwoDDataParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval, ySizeSlider->value());
	
}
void TwoDDataEventRouter::
setZCenter(TwoDDataParams* pParams,int sliderval){
	sliderToText(pParams,2, sliderval, 0);
	
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void TwoDDataEventRouter::
setXSize(TwoDDataParams* pParams,int sliderval){
	sliderToText(pParams,0, xCenterSlider->value(),sliderval);
	
}
void TwoDDataEventRouter::
setYSize(TwoDDataParams* pParams,int sliderval){
	sliderToText(pParams,1, yCenterSlider->value(),sliderval);
	
}

//Save undo/redo state when user clicks cursor
//
void TwoDDataEventRouter::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(VizWinMgr::getActiveTwoDDataParams(),  "move planar cursor");
}
void TwoDDataEventRouter::
guiEndCursorMove(){
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
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

//Obtain a new histogram for the current selected variables.
//Save histogram at the position associated with firstVarNum
void TwoDDataEventRouter::
refreshHistogram(RenderParams* p){
	TwoDDataParams* pParams = (TwoDDataParams*)p;
	int firstVarNum = pParams->getFirstVarNum();
	const float* currentDatarange = pParams->getCurrentDatarange();
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	if(pParams->doBypass(timeStep)) return;
	if (!histogramList){
		histogramList = new Histo*[numVariables];
		numHistograms = numVariables;
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	if (!histogramList[firstVarNum]){
		histogramList[firstVarNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	}
	Histo* histo = histogramList[firstVarNum];
	histo->reset(256,currentDatarange[0],currentDatarange[1]);
	//Determine what resolution is available:
	int refLevel = pParams->getNumRefinements();
	
	if (ds->useLowerRefinementLevel()){
		for (int varnum = 0; varnum < (int)ds->getNumSessionVariables2D(); varnum++){
			if (pParams->variableIsSelected(varnum)) {
				refLevel = Min(ds->maxXFormPresent2D(varnum, timeStep), refLevel);
			}
		}
		if (refLevel < 0) return;
	}

	//create the smallest containing box
	size_t blkMin[3],blkMax[3];
	size_t boxMin[3],boxMax[3];
	
	pParams->getAvailableBoundingBox(timeStep, blkMin, blkMax, boxMin, boxMax, refLevel);
	
	
	float boxExts[6];
	RegionParams::convertToBoxExtents(refLevel,boxMin, boxMax,boxExts); 
	int numMBs = RegionParams::getMBStorageNeeded(boxExts, boxExts+3, refLevel);
	//Check which variables are needed:
	int varCount = 0;
	std::vector<int> varNums;
	for (int varnum = 0; varnum < (int)ds->getNumSessionVariables2D(); varnum++){
		if (pParams->variableIsSelected(varnum)) {
			varCount++;
			varNums.push_back(varnum);
		}
	}
	int cacheSize = DataStatus::getInstance()->getCacheMB();
	if (numMBs*varCount > (int)(cacheSize*0.75)){
		pParams->setAllBypass(true);
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small\nfor current twoD and resolution.\n%s \n%s",
			"Lower the refinement level,\nreduce the plane size,\nor increase the cache size.",
			"Rendering has been disabled.");
		pParams->setEnabled(false);
		updateTab();
		return;
	}
	const size_t* bSize =  (DataStatus::getInstance()->getDataMgr()->GetBlockSize());
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then histogram rms on the variables (if > 1 specified)
	float** planarData = new float*[numVariables];
	//Now obtain all of the volumes needed for this twoD:
	
	for (int var = 0; var < varCount; var++){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		planarData[var] = pParams->getContainingVolume(blkMin, blkMax, refLevel, varNums[var], timeStep, true);
		QApplication::restoreOverrideCursor();
		if (!planarData[var]) {
			pParams->setBypass(timeStep);
			return;
		}
	}
	
	//Loop over the pixels in the volume(s)
	int xcrd = 0, ycrd = 1;
	int orientation = DataStatus::getInstance()->get2DOrientation(pParams->getFirstVarNum());
	if (orientation < 2) ycrd++;
	if (orientation < 1) xcrd++;

	for (int j = boxMin[ycrd]; j<= boxMax[ycrd]; j++){
		for (int i = boxMin[xcrd]; i<= boxMax[xcrd]; i++){
			int xyzCoord = (i - blkMin[xcrd]*bSize[xcrd]) +
					(j - blkMin[ycrd]*bSize[ycrd])*(bSize[xcrd]*(blkMax[xcrd]-blkMin[xcrd]+1));	
			float varVal;
					
			if (varCount == 1) varVal = planarData[0][xyzCoord];
			else { //Add up the squares of the variables
				varVal = 0.f;
				for (int d = 0; d<varCount; d++){
					varVal += planarData[d][xyzCoord]*planarData[d][xyzCoord];
				}
				varVal = sqrt(varVal);
			}
			histo->addToBin(varVal);
		}
	}

}

	
//Method called when undo/redo changes params.  It does the following:
//  puts the new params into the vizwinmgr, deletes the old one
//  Updates the tab if it's the current instance
//  Calls updateRenderer to rebuild renderer 
//	Makes the vizwin update.
void TwoDDataEventRouter::
makeCurrent(Params* prevParams, Params* nextParams, bool newWin, int instance,bool) {

	assert(instance >= 0);
	TwoDDataParams* pParams = (TwoDDataParams*)(nextParams->deepCopy());
	int vizNum = pParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumTwoDDataInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::TwoDDataParamsType, instance);
	setEditorDirty();
	updateTab();
	TwoDDataParams* formerParams = (TwoDDataParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled  changed:
	if (newWin || (formerParams->isEnabled() != pParams->isEnabled())){
		updateRenderer(pParams, wasEnabled,  newWin);
	}
	setDatarangeDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void TwoDDataEventRouter::
setDatarangeDirty(RenderParams* params)
{
	TwoDDataParams* pParams = (TwoDDataParams*)params;
	if (!pParams->getMapperFunc()) return;
	const float* currentDatarange = pParams->getCurrentDatarange();
	float minval = pParams->getMapperFunc()->getMinColorMapValue();
	float maxval = pParams->getMapperFunc()->getMaxColorMapValue();
	if (currentDatarange[0] != minval || currentDatarange[1] != maxval){
			pParams->setCurrentDatarange(minval, maxval);
			VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	}
	
}

void TwoDDataEventRouter::cleanParams(Params* p) 
{
  transferFunctionFrame->setMapperFunction(NULL);
  transferFunctionFrame->setVariableName("");
  transferFunctionFrame->update();
}
	
//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void TwoDDataEventRouter::captureImage() {
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	if (!pParams->isEnabled()) return;
	QFileDialog fileDialog(this,
		"Specify Jpeg image capture file name",
		Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg Images (*.jpg)");
	fileDialog.setWindowTitle("Specify image capture file name");
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QString filename = fileDialog.selectedFiles()[0];
    
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	
	if (fileInfo->exists()){
		int rc = QMessageBox::warning(0, "File Exists", QString("OK to replace existing jpeg file?\n %1 ").arg(filename), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory((const char*)fileInfo->absolutePath().toAscii());
	if (!filename.endsWith(".jpg")) filename += ".jpg";
	//
	
	//reconstruct with appropriate aspect ratio.
	
	
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int imgSize[2];
	pParams->getTextureSize(imgSize, timestep);
	int wid = imgSize[0];
	int ht = imgSize[1];
	unsigned char* twoDTex;
	unsigned char* buf;
	
		
		//Determine the image size.  Start with the texture dimensions in 
		// the scene, then increase x or y to make the aspect ratio match the
		// aspect ratio in the scene
		float aspRatio = pParams->getRealImageHeight()/pParams->getRealImageWidth();
	
		//Make sure the image is at least 256x256
		if (wid < 256) wid = 256;
		if (ht < 256) ht = 256;
		float imAspect = (float)ht/(float)wid;
		if (imAspect > aspRatio){
			//want ht/wid = asp, but ht/wid > asp; make wid larger:
			wid = (int)(0.5f+ (imAspect/aspRatio)*(float)wid);
		} else { //Make ht larger:
			ht = (int) (0.5f + (aspRatio/imAspect)*(float)ht);
		}
		//Construct the twoD texture of the desired dimensions:
		buf = pParams->calcTwoDDataTexture(timestep,wid,ht);
	
	//Construct an RGB image from this.  Ignore alpha.
	//invert top and bottom while removing alpha component
	twoDTex = new unsigned char[3*wid*ht];
	for (int j = 0; j<ht; j++){
		for (int i = 0; i< wid; i++){
			for (int k = 0; k<3; k++)
				twoDTex[k+3*(i+wid*j)] = buf[k+4*(i+wid*(ht-j-1))];
		}
	}
		
	
	
	
	
	//Now open the jpeg file:
	FILE* jpegFile = fopen((const char*)filename.toAscii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: \nError opening output Jpeg file: \n%s",(const char*)filename.toAscii());
		return;
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = GLWindow::getJpegQuality();
	int rc = write_JPEG_file(jpegFile, wid, ht, twoDTex, quality);
	delete twoDTex;
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; \nError writing jpeg file: \n%s",
			(const char*)filename.toAscii());
		delete buf;
		return;
	}
	//Provide a message stating the capture in effect.
	MessageReporter::infoMsg("Image is captured to %s",
			(const char*)filename.toAscii());
}

void TwoDDataEventRouter::guiNudgeXSize(int val) {
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	
	//ignore if change is not 1 
	if(abs(val - lastXSizeSlider) != 1) {
		lastXSizeSlider = val;
		return;
	}
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	
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
		xSizeSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDDataEventRouter::guiNudgeXCenter(int val) {
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastXCenterSlider) != 1) {
		lastXCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	
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
void TwoDDataEventRouter::guiNudgeYCenter(int val) {
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYCenterSlider) != 1) {
		lastYCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	
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
void TwoDDataEventRouter::guiNudgeZCenter(int val) {
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastZCenterSlider) != 1) {
		lastZCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	
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

void TwoDDataEventRouter::guiNudgeYSize(int val) {
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYSizeSlider) != 1) {
		lastYSizeSlider = val;
		return;
	}
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	
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
		ySizeSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}

void TwoDDataEventRouter::
adjustBoxSize(TwoDDataParams* pParams){
	//Determine the max x, y, z sizes of twoD slice, and make sure it fits.
	int orientation = DataStatus::getInstance()->get2DOrientation(pParams->getFirstVarNum());
	int xcrd = 0, ycrd = 1;
	if (orientation < 2) ycrd++;
	if (orientation < 1) xcrd++;
	
	float boxmin[3], boxmax[3];
	//Don't do anything if we haven't read the data yet:
	if (!Session::getInstance()->getDataMgr()) return;
	pParams->getBox(boxmin, boxmax);

	const float* extents = DataStatus::getInstance()->getExtents();
	
	//force 2d data to be no larger than extents:
	
	if (boxmin[xcrd] < extents[xcrd]) boxmin[xcrd] = extents[xcrd];
	if (boxmax[xcrd] > extents[xcrd+3]) boxmax[xcrd] = extents[xcrd+3];
	if (boxmin[ycrd] < extents[ycrd]) boxmin[ycrd] = extents[ycrd];
	if (boxmax[ycrd] > extents[ycrd+3]) boxmax[ycrd] = extents[ycrd+3];
	
	
	pParams->setBox(boxmin, boxmax);
	//Set the size sliders appropriately:
	xSizeEdit->setText(QString::number(boxmax[xcrd]-boxmin[xcrd]));
	ySizeEdit->setText(QString::number(boxmax[ycrd]-boxmin[ycrd]));
	
	xSizeSlider->setValue((int)(256.f*(boxmax[xcrd]-boxmin[xcrd])/(extents[xcrd+3]-extents[xcrd])));
	ySizeSlider->setValue((int)(256.f*(boxmax[ycrd]-boxmin[ycrd])/(extents[ycrd+3]-extents[ycrd])));
	
	//Cancel any response to text events generated in this method:
	//
	guiSetTextChanged(false);
}
void TwoDDataEventRouter::resetTextureSize(TwoDDataParams* twoDParams){
	//setup the texture:
	float voxDims[2];
	twoDParams->getTwoDVoxelExtents(voxDims);
	twoDTextureFrame->setTextureSize(voxDims[0],voxDims[1]);
}

QString TwoDDataEventRouter::getMappedVariableNames(int* numvars){
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	QString names("");
	*numvars = 0;
	for (int i = 0; i< DataStatus::getInstance()->getNumSessionVariables2D(); i++){
		//Index by session variable num:
		if (pParams->variableIsSelected(i)){
			if (*numvars > 0) names = names + ",";
			names = names + DataStatus::getInstance()->getVariableName2D(i).c_str();
			(*numvars)++;
		}
	}
	return names;
}
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
//Boolean flag is only used by isoeventrouter version
Histo* TwoDDataEventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool ){
	
	int numVariables = DataStatus::getInstance()->getNumSessionVariables2D();
	int varNum = renParams->getSessionVarNum();
	if (varNum >= numVariables || varNum < 0) return 0;
	if (varNum >= numHistograms || !histogramList){
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
		numHistograms = numVariables;
	}
	
	const float* currentDatarange = renParams->getCurrentDatarange();
	if (histogramList[varNum]) return histogramList[varNum];
	
	if (!mustGet) return 0;
	histogramList[varNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	refreshHistogram(renParams);
	return histogramList[varNum];
	
}

// Map the cursor coords into world space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void TwoDDataEventRouter::mapCursor(){
	//Get the transform 
	TwoDDataParams* tParams = VizWinMgr::getInstance()->getActiveTwoDDataParams();
	if(!DataStatus::getInstance()->getDataMgr()) return;
	float twoDCoord[3];
	float a[2],b[2],constVal[2];
	int mapDims[3];
	tParams->build2DTransform(a,b,constVal,mapDims);
	const float* cursorCoords = tParams->getCursorCoords();
	//If using flat plane, the cursor sits in the z=0 plane of the twoD box coord system.
	//x is reversed because we are looking from the opposite direction 
	twoDCoord[0] = -cursorCoords[0];
	twoDCoord[1] = cursorCoords[1];
	twoDCoord[2] = 0.f;
	float selectPoint[3];
	selectPoint[mapDims[0]] = twoDCoord[0]*a[0]+b[0];
	selectPoint[mapDims[1]] = twoDCoord[1]*a[1]+b[1];
	selectPoint[mapDims[2]] = constVal[0];
	if (tParams->isMappedToTerrain()) {
		//Find terrain height at selected point:
		//mapDims are just 0,1,2
		assert (mapDims[0] == 0 && mapDims[1] == 1 && mapDims[2] == 2);
		int varnum = DataStatus::getInstance()->getSessionVariableNum2D("HGT");
		if (varnum >= 0){
			float val = calcCurrentValue(tParams,selectPoint,&varnum, 1);
			if (val != OUT_OF_BOUNDS)
				selectPoint[mapDims[2]] = val+tParams->getTwoDMin(2);
		}
	} 
	tParams->setSelectedPoint(selectPoint);
}
void TwoDDataEventRouter::updateBoundsText(RenderParams* params){
	QString strn;
	TwoDDataParams* twoDParams = (TwoDDataParams*) params;
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	minDataBound->setText(strn.setNum(twoDParams->getDataMinBound(currentTimeStep)));
	maxDataBound->setText(strn.setNum(twoDParams->getDataMaxBound(currentTimeStep)));
	if (twoDParams->getMapperFunc()){
		leftMappingBound->setText(strn.setNum(twoDParams->getMapperFunc()->getMinColorMapValue(),'g',4));
		rightMappingBound->setText(strn.setNum(twoDParams->getMapperFunc()->getMaxColorMapValue(),'g',4));
	} else {
		leftMappingBound->setText("0.0");
		rightMappingBound->setText("1.0");
	}
}


void TwoDDataEventRouter::guiChangeInstance(int inst){
	performGuiChangeInstance(inst);
}
void TwoDDataEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void TwoDDataEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void TwoDDataEventRouter::
setTwoDTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void TwoDDataEventRouter::
twoDReturnPressed(void){
	confirmText(true);
}

void TwoDDataEventRouter::
twoDAttachSeed(bool attach){
	if (attach) twoDAddSeed();
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::FlowParamsType);
	
	guiAttachSeed(attach, fParams);
}

void TwoDDataEventRouter::refreshGLWindow()
{
	if (twoDTextureFrame->getGLWindow()->isRendering()) return;
	twoDTextureFrame->getGLWindow()->updateGL();
}
void TwoDDataEventRouter::guiFitTFToData(){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "fit TF to data");
	//Get bounds from DataStatus:
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	
	float minBound = 1.e30f;
	float maxBound = -1.e30f;
	//loop over selected variables to calc min/max bound
	bool multVars = (pParams->getNumVariablesSelected() > 1);
	float sumsqvals = 0.f;
	
	for (int i = 0; i<ds->getNumSessionVariables2D(); i++){
		if (pParams->variableIsSelected(i) && ds->dataIsPresent2D(i,ts)){
			float minval = ds->getDataMin2D(i, ts);
			float maxval = ds->getDataMax2D(i, ts);
			if (multVars){
				float mxval = Max(abs(maxval),abs(minval));
				float mnval = Min(abs(maxval),abs(minval));
				sumsqvals += mxval*mxval;
				minval = mnval*mnval;
			} 
			if (minval < minBound) minBound = minval;
			if (maxval > maxBound) maxBound = maxval;
		}
	}
	if (multVars){
		minBound = sqrt(minBound);  //sqrt of smallest min*min
		maxBound = sqrt(sumsqvals); //sqrt of rms of max's
	}
	if (minBound > maxBound){ //no data
		maxBound = 1.f;
		minBound = 0.f;
	}
	
	((TransferFunction*)pParams->getMapperFunc())->setMinMapValue(minBound);
	((TransferFunction*)pParams->getMapperFunc())->setMaxMapValue(maxBound);
	PanelCommand::captureEnd(cmd, pParams);
	setDatarangeDirty(pParams);
	updateTab();
	
}
