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
#pragma warning( disable : 4100 4996 )
#endif
#include <QScrollArea>
#include <QScrollBar>
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
#include <vapor/jpegapi.h>
#include <vapor/XmlNode.h>
#include "vapor/GetAppPath.h"
#include "tabmanager.h"
#include "twoDdataparams.h"
#include "twoDdataeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "VolumeRenderer.h"

using namespace VAPoR;


TwoDDataEventRouter::TwoDDataEventRouter(QWidget* parent ): QWidget(parent), Ui_TwoDDataTab(), TwoDEventRouter(){
	setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(Params::_twoDDataParamsTag);
	savedCommand = 0;
	ignoreComboChanges = false;
	numVariables = 0;
	seedAttached = false;
	
	MessageReporter::infoMsg("TwoDDataEventRouter::TwoDDataEventRouter()");

#if defined(Darwin) && (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	texShown = false;
	opacityMapShown = false;
	transferFunctionFrame->hide();
	twoDTextureFrame->hide();
#endif
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
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (variableCombo,SIGNAL(activated(int)), this, SLOT(guiChangeVariable(int)));
	connect (heightCombo, SIGNAL(activated(int)),this,SLOT(guiSetHeightVarNum(int)));
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
	connect(smoothCheckbox, SIGNAL(toggled(bool)),this, SLOT(guiToggleSmooth(bool)));
	
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
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (dataMgr && ds->dataIsPresent2D()){
		setEnabled(true);
		instanceTable->setEnabled(true);
	}
	else {
		setEnabled(false);
		instanceTable->setEnabled(false);
	}
	instanceTable->rebuild(this);

	TwoDDataParams* twoDParams = VizWinMgr::getActiveTwoDDataParams();
	const string hname = twoDParams->GetHeightVariableName();
	int hNum = ds->getActiveVarNum2D(hname);
	if (hNum <0) hNum = 0;
	heightCombo->setCurrentIndex(hNum);
	
	if (!isEnabled()) {
		transferFunctionFrame->setMapperFunction(0); 
		return;
	}
	if (GLWindow::isRendering()) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	guiSetTextChanged(false);
	setIgnoreBoxSliderEvents(true);  //don't generate nudge events

	
	instanceTable->rebuild(this);
	
	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int currentTimeStep = vizMgr->getActiveAnimationParams()->getCurrentTimestep();
	int winnum = vizMgr->getActiveViz();
	const vector<double>& tvExts = dataMgr->GetExtents((size_t)currentTimeStep);
	lodCombo->setCurrentIndex(twoDParams->GetCompressionLevel());
	
	int orientation = 2; //x-y aligned
	//set up the cursor position
	mapCursor();
	const float* localSelectedPoint = twoDParams->getSelectedPointLocal();
	
	selectedXLabel->setText(QString::number(localSelectedPoint[0]+tvExts[0]));
	selectedYLabel->setText(QString::number(localSelectedPoint[1]+tvExts[1]));
	selectedZLabel->setText(QString::number(localSelectedPoint[2]+tvExts[2]));
	
	//Provide latlon coords if available:
	if (DataStatus::getProjectionString().size() == 0){
		latLonFrame->hide();
	} else {
		double selectedLatLon[2];
		selectedLatLon[0] = localSelectedPoint[0];
		selectedLatLon[1] = localSelectedPoint[1];
		if (DataStatus::convertLocalToLonLat(currentTimeStep,selectedLatLon)){
			selectedLonLabel->setText(QString::number(selectedLatLon[0]));
			selectedLatLabel->setText(QString::number(selectedLatLon[1]));
			latLonFrame->show();
		} else {
			latLonFrame->hide();
		}
	}
	guiSetTextChanged(false);
	attachSeedCheckbox->setChecked(seedAttached);
	smoothCheckbox->setChecked(twoDParams->linearInterpTex());
	captureButton->setEnabled(twoDParams->isEnabled());

	//orientation = ds->get2DOrientation(twoDParams->getFirstVarNum());
	orientationCombo->setCurrentIndex(orientation);
	orientationCombo->setEnabled(false);
	if (twoDParams->GetMapperFunc()){
		twoDParams->GetMapperFunc()->setParams(twoDParams);
		transferFunctionFrame->setMapperFunction(twoDParams->GetMapperFunc());
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
	int numRefs = twoDParams->GetRefinementLevel();
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
	string varname = ds->getVariableName2D(twoDParams->getFirstVarNum());	
	if (twoDParams->isEnabled()){
		double selectPoint[3];
		for (int i = 0; i<3; i++) selectPoint[i] = localSelectedPoint[i]+tvExts[i];
		val = RegionParams::calcCurrentValue(varname,selectPoint,twoDParams->GetRefinementLevel(), twoDParams->GetCompressionLevel(), (size_t)currentTimeStep);
	}

	if (val == OUT_OF_BOUNDS)
		valueMagLabel->setText(QString(" "));
	else valueMagLabel->setText(QString::number(val));
	guiSetTextChanged(false);
	//Set the selection in the variable combo
	//Turn off combo message-listening
	ignoreComboChanges = true;
	for (int i = 0; i< ds->getNumActiveVariables2D(); i++){
		
		bool selection = twoDParams->variableIsSelected(ds->mapActiveToSessionVarNum2D(i));
		if (selection)
			variableCombo->setCurrentIndex(i);
	}
	ignoreComboChanges = false;



	updateBoundsText(twoDParams);
	guiSetTextChanged(false);
	float sliderVal = twoDParams->getOpacityScale();
	
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	
	//Set the mode buttons:
	
	if (twoDParams->getEditMode()){
		
		editButton->setChecked(true);
		navigateButton->setChecked(false);
	} else {
		editButton->setChecked(false);
		navigateButton->setChecked(true);
	}
	
	
	deleteInstanceButton->setEnabled(Params::GetNumParamsInstances(Params::_twoDDataParamsTag,winnum) > 1);
	
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
	
	twoDParams->getLocalBox(boxmin, boxmax);
	for (int i = 0; i<3; i++) boxCenter[i] = (boxmax[i]+boxmin[i])*0.5f + tvExts[i];
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-tvExts[0])/(tvExts[3]-tvExts[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-tvExts[1])/(tvExts[4]-tvExts[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-tvExts[2])/(tvExts[5]-tvExts[2])));
	xCenterEdit->setText(QString::number(boxCenter[0]));
	yCenterEdit->setText(QString::number(boxCenter[1]));
	zCenterEdit->setText(QString::number(boxCenter[2]));
	
	for(int i = 0; i<3; i++){
		boxmin[i] += tvExts[i];
		boxmax[i] += tvExts[i];
	}
	//setup the size sliders 
	adjustBoxSize(twoDParams);

	//Specify the box extents in both user and grid coords:
	minUserXLabel->setText(QString::number(boxmin[0]));
	minUserYLabel->setText(QString::number(boxmin[1]));
	minUserZLabel->setText(QString::number(boxmin[2]));
	maxUserXLabel->setText(QString::number(boxmax[0]));
	maxUserYLabel->setText(QString::number(boxmax[1]));
	maxUserZLabel->setText(QString::number(boxmax[2]));
	
	size_t gridExts[6];
	if (dataMgr ){
		int fullRefLevel = ds->getNumTransforms();
		twoDParams->mapBoxToVox(dataMgr,fullRefLevel,twoDParams->GetCompressionLevel(),currentTimeStep,gridExts);
		
		minGridXLabel->setText(QString::number(gridExts[0]));
		minGridYLabel->setText(QString::number(gridExts[1]));
		minGridZLabel->setText(QString::number(gridExts[2]));
		maxGridXLabel->setText(QString::number(gridExts[3]));
		maxGridYLabel->setText(QString::number(gridExts[4]));
		maxGridZLabel->setText(QString::number(gridExts[5]));
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
	//Only allow terrain map with horizontal orientation, and if
	//HGT variable is there
	if (orientation != 2) {
		applyTerrainCheckbox->setEnabled(false);
		applyTerrainCheckbox->setChecked(false);
		heightCombo->setEnabled(false);
	} else {
		int varnum = ds->getSessionVariableNum2D(twoDParams->GetHeightVariableName());
		if (varnum < 0 || !ds->dataIsPresent2D(varnum,currentTimeStep)){
			applyTerrainCheckbox->setEnabled(false);
			applyTerrainCheckbox->setChecked(false);
		} else {
			bool terrainMap = twoDParams->isMappedToTerrain();
			if (terrainMap != applyTerrainCheckbox->isChecked())
				applyTerrainCheckbox->setChecked(terrainMap);
			applyTerrainCheckbox->setEnabled(true);
		}
		heightCombo->setEnabled(ds->getNumActiveVariables2D() > 0);
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
	
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	TwoDDataParams* twoDParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(twoDParams, "edit TwoDData text");
	QString strn;
	
	twoDParams->setHistoStretch(histoScaleEdit->text().toFloat());
	size_t timestep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& tvExts = dataMgr->GetExtents(timestep);
	
	int orientation = DataStatus::getInstance()->get2DOrientation(twoDParams->getFirstVarNum());
	int xcrd =0, ycrd = 1, zcrd = 2;
	if (orientation < 2) {ycrd = 2; zcrd = 1;}
	if (orientation < 1) {xcrd = 1; zcrd = 0;}

	
	//Set the twoD size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[xcrd] = xSizeEdit->text().toFloat();
	boxSize[ycrd] = ySizeEdit->text().toFloat();
	boxSize[zcrd] = 0;
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
		if (boxSize[i] > (tvExts[i+3]-tvExts[i])) boxSize[i] = (tvExts[i+3]-tvExts[i]);
	}
	boxCenter[0] = xCenterEdit->text().toFloat();
	boxCenter[1] = yCenterEdit->text().toFloat();
	boxCenter[2] = zCenterEdit->text().toFloat();
	twoDParams->getLocalBox(boxmin, boxmax);
	//the box z-size is not adjustable:
	boxSize[zcrd] = boxmax[zcrd]-boxmin[zcrd];
	for (int i = 0; i<3;i++){
		if (i!=2){
			if (boxCenter[i] < tvExts[i])boxCenter[i] = tvExts[i];
			if (boxCenter[i] > tvExts[i+3])boxCenter[i] = tvExts[i+3];
		}
		//boxmin, boxmax are in local coordinates
		boxmin[i] = boxCenter[i] - 0.5f*boxSize[i]-tvExts[i];
		boxmax[i] = boxCenter[i] + 0.5f*boxSize[i]-tvExts[i];
	}
	twoDParams->setLocalBox(boxmin,boxmax);
	adjustBoxSize(twoDParams);
	//set the center sliders:
	setIgnoreBoxSliderEvents(true);
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-tvExts[0])/(tvExts[3]-tvExts[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-tvExts[1])/(tvExts[4]-tvExts[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-tvExts[2])/(tvExts[5]-tvExts[2])));
	setIgnoreBoxSliderEvents(false);
	resetTextureSize(twoDParams);
	//twoDTextureFrame->setTextureSize(voxDims[0],voxDims[1]);
	setTwoDDirty(twoDParams);
	if (twoDParams->GetMapperFunc()) {
		((TransferFunction*)twoDParams->GetMapperFunc())->setMinMapValue(leftMappingBound->text().toFloat());
		((TransferFunction*)twoDParams->GetMapperFunc())->setMaxMapValue(rightMappingBound->text().toFloat());
	
		setDatarangeDirty(twoDParams);
		setEditorDirty();
		update();
		twoDTextureFrame->update();
	}
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(twoDParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
	
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
	
	
	dParams->setMappedToTerrain(mode);
	//If setting to terrain, set box bottom and top to bottom of domain
	if (mode){
		dParams->setLocalTwoDMin(2,0.);
		dParams->setLocalTwoDMax(2,0.);
	}
	if (dParams->isEnabled()) {
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		int viznum = vizMgr->getActiveViz();
		float disp = dParams->getLocalTwoDMin(2);
		//Check that we aren't putting this on another planar surface:
		if (vizMgr->findCoincident2DSurface(viznum, disp, dParams))
		{
			MessageReporter::warningMsg("This 2D data surface is close to another enabled 2D surface.\n%s\n",
					"Change the 2D data position in order to avoid rendering defects");
		}
	}
	
	//Reposition cursor:
	mapCursor();
	PanelCommand::captureEnd(cmd, dParams); 
	setTwoDDirty(dParams);
	VizWinMgr::getInstance()->forceRender(dParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
		if (vizMgr->findCoincident2DSurface(activeViz, pParams->getLocalTwoDMin(orientation),pParams)){
			
				MessageReporter::warningMsg("This 2D data surface is close to another enabled 2D surface.\n%s\n",
					"Change the 2D data position in order to avoid rendering defects");
			}
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val, instance);
	
}

void TwoDDataEventRouter::
setTwoDEditMode(bool mode){
	navigateButton->setChecked(!mode);
	guiSetEditMode(mode);
}
void TwoDDataEventRouter::
setTwoDNavigateMode(bool mode){
	editButton->setChecked(!mode);
	guiSetEditMode(!mode);
}

void TwoDDataEventRouter::
refreshTwoDHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	if (pParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(pParams,pParams->getSessionVarNum(), pParams->getCurrentDatarange());
	}
	setEditorDirty();
}

void TwoDDataEventRouter::
twoDLoadInstalledTF(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	TransferFunction* tf = pParams->GetTransFunc();
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

	tf = pParams->GetTransFunc();
	tf->setMinMapValue(minb);
	tf->setMaxMapValue(maxb);
	setEditorDirty();
	updateClut(pParams);
}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the TwoDData params
void TwoDDataEventRouter::
twoDSaveTF(void){
	TwoDDataParams* dParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	saveTF(dParams);
}
void TwoDDataEventRouter::
twoDLoadTF(void){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	loadTF(pParams, pParams->getSessionVarNum());
	updateClut(pParams);
}
void TwoDDataEventRouter::
twoDCenterRegion(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	VizWinMgr::getInstance()->getRegionRouter()->guiSetCenter(pParams->getSelectedPointLocal());
}
void TwoDDataEventRouter::
twoDCenterView(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPointLocal());
}
void TwoDDataEventRouter::
twoDCenterRake(){
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPointLocal());
}

void TwoDDataEventRouter::
twoDAddSeed(){
	Point4 pt;
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	mapCursor();
	pt.set3Val(pParams->getSelectedPointLocal());
	AnimationParams* ap = (AnimationParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_animationParamsTag);
	int timestep = ap->getCurrentTimestep();
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
	rParams->getLocalBox(boxMin, boxMax,timestep);
	if (pt.getVal(0) < boxMin[0] || pt.getVal(1) < boxMin[1] || pt.getVal(2) < boxMin[2] ||
		pt.getVal(0) > boxMax[0] || pt.getVal(1) > boxMax[1] || pt.getVal(2) > boxMax[2]) {
			MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the seed point is outside current region");
	}
	//convert to user coordinates:
	if (!DataStatus::getInstance()->getDataMgr()) return;
	const vector<double>& usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	for (int i = 0; i<3; i++) pt.set1Val(i, (float)( pt.getVal(i)+usrExts[i]));
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
void TwoDDataEventRouter::
guiSetHeightVarNum(int vnum){
	int sesvarnum=0;
	confirmText(true);
	TwoDDataParams* tParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_twoDDataParamsTag);
	//Make sure its a valid variable..
	sesvarnum = DataStatus::getInstance()->mapActiveToSessionVarNum2D(vnum);
	if (sesvarnum < 0) return; 
	PanelCommand* cmd = PanelCommand::captureStart(tParams, "set terrain height variable");
	tParams->SetHeightVariableName(DataStatus::getInstance()->getVariableName2D(sesvarnum));
	updateTab();
	PanelCommand::captureEnd(cmd, tParams);
	if (!tParams->isMappedToTerrain()) return;
	setTwoDDirty(tParams);
	VizWinMgr::getInstance()->forceRender(tParams);	
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
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "copy region to TwoDData");
	
	for (int i = 0; i< 3; i++){
		pParams->setLocalTwoDMin(i, rParams->getLocalRegionMin(i,timestep));
		pParams->setLocalTwoDMax(i, rParams->getLocalRegionMax(i,timestep));
	}
	
	updateTab();
	setTwoDDirty(pParams);
	
	PanelCommand::captureEnd(cmd,pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
	
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
	//Set the names in the variable combo and the height combo
	
	ignoreComboChanges = true;
	variableCombo->clear();
	heightCombo->clear();
	for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables2D(); i++){
		const std::string& s = DataStatus::getInstance()->getActiveVarName2D(i);
		const QString& text = QString(s.c_str());
		variableCombo->insertItem(i,text);
		heightCombo->insertItem(i,text);
	}
	ignoreComboChanges = false;

	seedAttached = false;

	//Set up the refinement combo:
	const DataMgr *dataMgr = ses->getDataMgr();
	
	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= numRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	if (dataMgr){
		vector<size_t> cRatios = dataMgr->GetCRatios();
		lodCombo->clear();
		lodCombo->setMaxCount(cRatios.size());
		for (int i = 0; i<cRatios.size(); i++){
			QString s = QString::number(cRatios[i]);
			s += ":1";
			lodCombo->addItem(s);
		}
	}
	
	if (histogramList) {
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete [] histogramList;
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
		VizWinMgr::getInstance()->setClutDirty(pParams);
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
	if(dp->GetMapperFunc())dp->GetMapperFunc()->setParams(dp);
    transferFunctionFrame->setMapperFunction(dp->GetMapperFunc());
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
		MapperFunction* mf = dp->GetMapperFunc();
		if (mf) {
			leftMappingBound->setText(QString::number(mf->getMinOpacMapValue()));
			rightMappingBound->setText(QString::number(mf->getMaxOpacMapValue()));
		}
	}
}


void TwoDDataEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	TwoDDataParams* pParams = VizWinMgr::getInstance()->getTwoDDataParams(winnum, instance);    
	confirmText(false);
	if(value == pParams->isEnabled()) return;
	
	PanelCommand* cmd;
	if (undoredo) cmd = PanelCommand::captureStart(pParams, "toggle TwoDData enabled",instance);
	pParams->setEnabled(value);
	if(undoredo) PanelCommand::captureEnd(cmd, pParams);
	
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(pParams,!value, false);
	setDatarangeDirty(pParams);
	//Need to rerender the texture:
	pParams->setTwoDDirty();
	//and refresh the gui
	updateTab();
	setDatarangeDirty(pParams);
	setEditorDirty();
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,true);
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
	
	VizWinMgr::getInstance()->forceRender(pp);
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
	VizWinMgr::getInstance()->setClutDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
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
	const float* selectedPoint = pParams->getSelectedPointLocal();
	float twoDMin[3],twoDMax[3];
	pParams->getLocalBox(twoDMin,twoDMax);
	for (int i = 0; i<3; i++)
		textToSlider(pParams,i,selectedPoint[i], twoDMax[i]-twoDMin[i]);
	PanelCommand::captureEnd(cmd, pParams);
	updateTab();
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);

}
//Make the probe center at selectedPoint.

void TwoDDataEventRouter::guiCenterProbe(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	TwoDDataParams* tParams = VizWinMgr::getActiveTwoDDataParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center Probe at Selected Point");
	const float* selectedPoint = tParams->getSelectedPointLocal();
	float pMin[3],pMax[3];
	pParams->getLocalBox(pMin,pMax,-1);
	//Move center so it coincides with the selected point
	for (int i = 0; i<3; i++){
		float diff = (pMax[i]-pMin[i])*0.5;
		pMin[i] = selectedPoint[i] - diff;
		pMax[i] = selectedPoint[i] + diff; 
	}
	pParams->setLocalBox(pMin,pMax,-1);
		
	PanelCommand::captureEnd(cmd, pParams);
	
	pParams->setProbeDirty();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);

}
//Respond to an change of variable.  set the appropriate bits
void TwoDDataEventRouter::
guiChangeVariable(int varnum){
	//Don't react if the combo is being reset programmatically:
	if (ignoreComboChanges) return;
	if (!DataStatus::getInstance()->dataIsPresent2D()) return;
	confirmText(false);
	TwoDDataParams* tParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(tParams, "change 2d-selected variable");
	int firstVar = 0;
	
	for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables2D(); i++){
		//Index by session variable num:
		int svnum = DataStatus::getInstance()->mapActiveToSessionVarNum2D(i);
		
		if (i == varnum){
			tParams->setVariableSelected(svnum,true);
			firstVar = varnum;
		}
		else 
			tParams->setVariableSelected(svnum,false);
	}
	tParams->setNumVariablesSelected(1);
	tParams->setFirstVarNum(firstVar);
	//reset the editing display range shown on the tab, 
	//this also sets dirty flag
	updateMapBounds(tParams);
	
	//Force a redraw of the transfer function frame
    setEditorDirty();
	   
	
	PanelCommand::captureEnd(cmd, tParams);
	//Need to update the selected point for the new variables
	updateTab();
	
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(tParams);
}

void TwoDDataEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide TwoDData X center");
	setXCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
	
}
void TwoDDataEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide TwoDData Y center");
	setYCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
	
}
void TwoDDataEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide TwoDData Z center");
	setZCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);

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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);

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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);

}
void TwoDDataEventRouter::guiFitToRegion(){
	confirmText(false);
	TwoDDataParams* tParams = VizWinMgr::getActiveTwoDDataParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	PanelCommand* cmd = PanelCommand::captureStart(tParams,  "Fit 2D data to region");
	//Just match the first two dimensions:
	for (int i = 0; i<2; i++){
		tParams->setLocalTwoDMin(i, rParams->getLocalRegionMin(i,ts));
		tParams->setLocalTwoDMax(i, rParams->getLocalRegionMax(i,ts));
	}
	if (tParams->getLocalTwoDMin(2) < rParams->getLocalRegionMin(2,ts)) 
		tParams->setLocalTwoDMin(2,rParams->getLocalRegionMin(2,ts));
	if (tParams->getLocalTwoDMin(2) > rParams->getLocalRegionMax(2,ts)) 
		tParams->setLocalTwoDMin(2, rParams->getLocalRegionMax(2,ts));

	tParams->setLocalTwoDMax(2,tParams->getLocalTwoDMin(2));

	PanelCommand::captureEnd(cmd, tParams);
	setTwoDDirty(tParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(tParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);

}
void TwoDDataEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	TwoDDataParams* dParams = VizWinMgr::getActiveTwoDDataParams();
	if (num == dParams->GetCompressionLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set compression level");
	
	dParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(dParams);

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
	pParams->SetRefinementLevel(n);
	PanelCommand::captureEnd(cmd, pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
}
	
//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//Should not change values in params unless the text is invalid.
//
void TwoDDataEventRouter::
textToSlider(TwoDDataParams* pParams, int coord, float newCenter, float newSize){
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	size_t timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>&tvExts = dataMgr->GetExtents(timestep);
	newCenter -= tvExts[coord];
	pParams->setLocalTwoDMin(coord, newCenter-0.5f*newSize);
	pParams->setLocalTwoDMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	return;
	
}
//Set text when a slider changes.
//
void TwoDDataEventRouter::
sliderToText(TwoDDataParams* pParams, int coord, int slideCenter, int slideSize){
	//Determine which coordinate of box is being slid:
	int orientation = DataStatus::getInstance()->get2DOrientation(pParams->getFirstVarNum());
	if (orientation < coord) coord++;
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	size_t timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& tvExts = dataMgr->GetExtents(timestep);
	//calculate center in user coords:
	float newCenter = tvExts[coord] + ((float)slideCenter)*(tvExts[coord+3]-tvExts[coord])/256.f;
	float newSize = (tvExts[coord+3]-tvExts[coord])*(float)slideSize/256.f;
	//If it's not inside the x,y domain, move the center:
	if (coord != 2){
		if ((newCenter - 0.5*newSize) < tvExts[coord]) newCenter = tvExts[coord]+0.5*newSize;
		if ((newCenter + 0.5*newSize) > tvExts[coord+3]) newCenter = tvExts[coord+3]-0.5*newSize;
	}

	pParams->setLocalTwoDMin(coord, newCenter-0.5f*newSize - tvExts[coord]);
	pParams->setLocalTwoDMax(coord, newCenter+0.5f*newSize- tvExts[coord]);
	adjustBoxSize(pParams);
	//Set the text in the edit boxes
	mapCursor();
	const float* localSelectedPoint = pParams->getSelectedPointLocal();
	
	switch(coord) {
		case 0:
			xSizeEdit->setText(QString::number(newSize,'g',7));
			xCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedXLabel->setText(QString::number(localSelectedPoint[coord]+tvExts[coord]));
			break;
		case 1:
			ySizeEdit->setText(QString::number(newSize,'g',7));
			yCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedYLabel->setText(QString::number(localSelectedPoint[coord]+tvExts[coord]));
			break;
		case 2:
			zCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedZLabel->setText(QString::number(localSelectedPoint[coord]+tvExts[coord]));
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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
	VizWinMgr::getInstance()->forceRender(twoDDataParams);
	
}

void TwoDDataEventRouter::setBindButtons(bool canbind)
{
  OpacityBindButton->setEnabled(canbind);
  ColorBindButton->setEnabled(canbind);
}

//Save undo/redo state when user grabs a twoD handle, or maybe a twoD face (later)
//
void TwoDDataEventRouter::
captureMouseDown(int){
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
	//pParams->getLocalBox(boxMin,boxMax);
	//twoDTextureFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	//Update the tab if it's in front:
	if(MainForm::getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getTwoDDataParams(viznum)))
			updateTab();
	}
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,true);
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
		VizWinMgr::getInstance()->getFlowRouter()->guiMoveLastSeed(pParams->getSelectedPointLocal());
	}
	
	//Update the tab, it's in front:
	updateTab();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
	
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
	if (!prevParams) assert(Params::GetNumParamsInstances(Params::_twoDDataParamsTag,vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::GetTypeFromTag(Params::_twoDDataParamsTag), instance);
	setEditorDirty();
	updateTab();
	TwoDDataParams* formerParams = (TwoDDataParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled  changed:
	if (newWin || (formerParams->isEnabled() != pParams->isEnabled())){
		updateRenderer(pParams, wasEnabled,  newWin);
	}
	VizWin* viz = VizWinMgr::getInstance()->getVizWin(vizNum);
	viz->setColorbarDirty(true);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,true);
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void TwoDDataEventRouter::
setDatarangeDirty(RenderParams* params)
{
	TwoDDataParams* pParams = (TwoDDataParams*)params;
	if (!pParams->GetMapperFunc()) return;
	const float* currentDatarange = pParams->getCurrentDatarange();
	float minval = pParams->GetMapperFunc()->getMinColorMapValue();
	float maxval = pParams->GetMapperFunc()->getMaxColorMapValue();
	if (currentDatarange[0] != minval || currentDatarange[1] != maxval){
			pParams->setCurrentDatarange(minval, maxval);
			VizWin* viz = VizWinMgr::getInstance()->getVizWin(pParams->getVizNum());
			viz->setColorbarDirty(true);
			VizWinMgr::getInstance()->forceRender(pParams);
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
	MainForm::getInstance()->showCitationReminder();
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	if (!pParams->isEnabled()) return;
	QString filename = QFileDialog::getSaveFileName(this,
													"Specify image capture Jpeg file name",
													Session::getInstance()->getJpegDirectory().c_str(),
													"Jpeg Images (*.jpg)");
	if(filename == QString("")) return;
	
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
	
	
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
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
	delete [] twoDTex;
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; \nError writing jpeg file: \n%s",
			(const char*)filename.toAscii());
		delete [] buf;
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
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 0);
	float pmin = pParams->getLocalTwoDMin(0);
	float pmax = pParams->getLocalTwoDMax(0);
	float maxExtent = ds->getLocalExtents()[3];
	float minExtent = ds->getLocalExtents()[0];
	float newSize = pmax - pmin;
	if (val > lastXSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastXSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setLocalTwoDMin(0, pmin-voxelSize);
			pParams->setLocalTwoDMax(0, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastXSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setLocalTwoDMin(0, pmin+voxelSize);
			pParams->setLocalTwoDMax(0, pmax-voxelSize);
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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 0);
	float pmin = pParams->getLocalTwoDMin(0);
	float pmax = pParams->getLocalTwoDMax(0);
	float maxExtent = ds->getLocalExtents()[3];
	float minExtent = ds->getLocalExtents()[0];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastXCenterSlider){//move by 1 voxel, but don't move past end
		lastXCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setLocalTwoDMin(0, pmin+voxelSize);
			pParams->setLocalTwoDMax(0, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastXCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setLocalTwoDMin(0, pmin-voxelSize);
			pParams->setLocalTwoDMax(0, pmax-voxelSize);
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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 1);
	float pmin = pParams->getLocalTwoDMin(1);
	float pmax = pParams->getLocalTwoDMax(1);
	float maxExtent = ds->getLocalExtents()[4];
	float minExtent = ds->getLocalExtents()[1];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastYCenterSlider){//move by 1 voxel, but don't move past end
		lastYCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setLocalTwoDMin(1, pmin+voxelSize);
			pParams->setLocalTwoDMax(1, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastYCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setLocalTwoDMin(1, pmin-voxelSize);
			pParams->setLocalTwoDMax(1, pmax-voxelSize);
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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 2);
	float pmin = pParams->getLocalTwoDMin(2);
	float pmax = pParams->getLocalTwoDMax(2);
	float maxExtent = ds->getLocalExtents()[5];
	float minExtent = ds->getLocalExtents()[2];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastZCenterSlider){//move by 1 voxel, but don't move past end
		lastZCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setLocalTwoDMin(2, pmin+voxelSize);
			pParams->setLocalTwoDMax(2, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastZCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setLocalTwoDMin(2, pmin-voxelSize);
			pParams->setLocalTwoDMax(2, pmax-voxelSize);
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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 1);
	float pmin = pParams->getLocalTwoDMin(1);
	float pmax = pParams->getLocalTwoDMax(1);
	float maxExtent = ds->getLocalExtents()[4];
	float minExtent = ds->getLocalExtents()[1];
	float newSize = pmax - pmin;
	if (val > lastYSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastYSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setLocalTwoDMin(1, pmin-voxelSize);
			pParams->setLocalTwoDMax(1, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastYSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setLocalTwoDMin(1, pmin+voxelSize);
			pParams->setLocalTwoDMax(1, pmax-voxelSize);
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
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::twoDDataMode);
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
	pParams->getLocalBox(boxmin, boxmax);

	const float* extents = DataStatus::getInstance()->getLocalExtents();
	
	//force 2d data to be no larger than extents:
	
	if (boxmin[xcrd] < 0.) boxmin[xcrd] = 0.;
	if (boxmax[xcrd] > extents[xcrd+3]) boxmax[xcrd] = extents[xcrd+3];
	if (boxmin[ycrd] < 0.) boxmin[ycrd] = 0.;
	if (boxmax[ycrd] > extents[ycrd+3]) boxmax[ycrd] = extents[ycrd+3];
	
	
	pParams->setLocalBox(boxmin, boxmax);
	//Set the size sliders appropriately:
	xSizeEdit->setText(QString::number(boxmax[xcrd]-boxmin[xcrd]));
	ySizeEdit->setText(QString::number(boxmax[ycrd]-boxmin[ycrd]));
	
	xSizeSlider->setValue((int)(256.f*(boxmax[xcrd]-boxmin[xcrd])/(extents[xcrd+3])));
	ySizeSlider->setValue((int)(256.f*(boxmax[ycrd]-boxmin[ycrd])/(extents[ycrd+3])));
	
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
Histo* TwoDDataEventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool){
	
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
	
	refreshHistogram(renParams,renParams->getSessionVarNum(), currentDatarange);
	return histogramList[varNum];
	
}

// Map the cursor coords into local coordinates space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void TwoDDataEventRouter::mapCursor(){
	//Get the transform 
	TwoDDataParams* tParams = VizWinMgr::getInstance()->getActiveTwoDDataParams();
	if(!DataStatus::getInstance()->getDataMgr()) return;
	size_t timeStep = (size_t) VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentFrameNumber();
	const vector<double>& userExtents = DataStatus::getInstance()->getDataMgr()->GetExtents(timeStep);
	float twoDCoord[3];
	float a[2],b[2],constVal[2];
	int mapDims[3];
	tParams->buildLocal2DTransform(a,b,constVal,mapDims);
	const float* cursorCoords = tParams->getCursorCoords();
	//If using flat plane, the cursor sits in the z=0 plane of the twoD box coord system.
	//x is reversed because we are looking from the opposite direction 
	twoDCoord[0] = -cursorCoords[0];
	twoDCoord[1] = cursorCoords[1];
	twoDCoord[2] = 0.f;
	double selectPoint[3];
	selectPoint[mapDims[0]] = twoDCoord[0]*a[0]+b[0];
	selectPoint[mapDims[1]] = twoDCoord[1]*a[1]+b[1];
	selectPoint[mapDims[2]] = constVal[0];
	
	

	if (tParams->isMappedToTerrain()) {
		//Find terrain height at selected point:
		//mapDims are just 0,1,2
		assert (mapDims[0] == 0 && mapDims[1] == 1 && mapDims[2] == 2);
		
		double sPoint[3];
		for (int i = 0; i<3; i++) sPoint[i] = selectPoint[i]+userExtents[i];
		float val = RegionParams::calcCurrentValue(tParams->GetHeightVariableName(),selectPoint,tParams->GetRefinementLevel(), tParams->GetCompressionLevel(), timeStep);
		if (val != OUT_OF_BOUNDS)
				selectPoint[mapDims[2]] = val+tParams->getLocalTwoDMin(2);
		
	} 
	float spt[3];
	for (int i = 0; i<3; i++) spt[i] = selectPoint[i];
	tParams->setSelectedPointLocal(spt);
}
void TwoDDataEventRouter::updateBoundsText(RenderParams* params){
	QString strn;
	TwoDDataParams* twoDParams = (TwoDDataParams*) params;
	DataStatus* ds = DataStatus::getInstance();
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	float mnval = 1.e30f, mxval = -1.30f;
	float minval, maxval;
	bool multvars = (twoDParams->getNumVariablesSelected()>1);
	for (int i = 0; i<ds->getNumSessionVariables2D(); i++){
		if (twoDParams->variableIsSelected(i) && ds->dataIsPresent2D(i,ts)){
			if (twoDParams->isEnabled()){
				minval = ds->getDataMin2D(i, ts);
				maxval = ds->getDataMax2D(i, ts);
			} else {
				minval = ds->getDefaultDataMin2D(i);
				maxval = ds->getDefaultDataMax2D(i);
			}
			if (multvars){
				maxval = Max(abs(minval),abs(maxval));
				minval = 0.f;
			}
			if (minval < mnval) mnval = minval;
			if (maxval > mxval) mxval = maxval;
		}
	}
	
	if (mnval > mxval){ //no data
		mxval = 1.f;
		mnval = 0.f;
	}
	minDataBound->setText(strn.setNum(mnval));
	maxDataBound->setText(strn.setNum(mxval));
	
	if (twoDParams->GetMapperFunc()){
		leftMappingBound->setText(strn.setNum(twoDParams->GetMapperFunc()->getMinColorMapValue(),'g',4));
		rightMappingBound->setText(strn.setNum(twoDParams->GetMapperFunc()->getMaxColorMapValue(),'g',4));
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
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_flowParamsTag);
	
	guiAttachSeed(attach, fParams);
}


void TwoDDataEventRouter::
guiToggleSmooth(bool val){
	confirmText(false);
	TwoDDataParams* pParams = VizWinMgr::getActiveTwoDDataParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "toggle smoothing");
	pParams->setLinearInterp(val);
	PanelCommand::captureEnd(cmd,pParams);
	setTwoDDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams);
	updateTab();
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
	
	
	for (int i = 0; i<ds->getNumSessionVariables2D(); i++){
		if (pParams->variableIsSelected(i) && ds->dataIsPresent2D(i,ts)){
			float minval = ds->getDataMin2D(i, ts);
			float maxval = ds->getDataMax2D(i, ts);
			if (multVars){
				maxval = Max(abs(minval),abs(maxval));
				minval = 0.f;
			} 
			if (minval < minBound) minBound = minval;
			if (maxval > maxBound) maxBound = maxval;
		}
	}
	if (minBound > maxBound){ //no data
		maxBound = 1.f;
		minBound = 0.f;
	}
	
	((TransferFunction*)pParams->GetMapperFunc())->setMinMapValue(minBound);
	((TransferFunction*)pParams->GetMapperFunc())->setMaxMapValue(maxBound);
	PanelCommand::captureEnd(cmd, pParams);
	setDatarangeDirty(pParams);
	updateTab();
	
}
//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widgets 

#ifdef Darwin
void TwoDDataEventRouter::paintEvent(QPaintEvent* ev){

	//First show the texture frame, next time through, show the tf frame
	//Other order doesn't work.
	if(!texShown ){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
		QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
		sArea->ensureWidgetVisible(twoDFrameHolder);
		texShown = true;
#endif
		twoDTextureFrame->updateGeometry();
		twoDTextureFrame->show();
		QWidget::paintEvent(ev);
		return;
	} 
	if (!opacityMapShown){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
		QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
		 sArea->ensureWidgetVisible(tfFrame);
		 opacityMapShown = true;
#endif
		 transferFunctionFrame->show();
	
	 } 
	QWidget::paintEvent(ev);
	adjustSize();
	return;
	
}
#endif

//Obtain a new histogram for the current selected variables.
//Save it at the position associated with firstVarNum
void TwoDDataEventRouter::
refreshHistogram(RenderParams* p, int, const float[2]){
	TwoDDataParams* tParams = (TwoDDataParams*)p;
	int firstVarNum = tParams->getFirstVarNum();
	const float* currentDatarange = tParams->getCurrentDatarange();
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	size_t timeStep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	if(tParams->doBypass(timeStep)) return;
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
	

	RegularGrid* histoGrid;
	int actualRefLevel = tParams->GetRefinementLevel();
	int lod = tParams->GetCompressionLevel();
	vector<string>varnames;
	varnames.push_back(ds->getVariableName2D(firstVarNum));
	double exts[6];
	tParams->GetBox()->GetLocalExtents(exts);
	const vector<double>& userExts = dataMgr->GetExtents(timeStep);
	for (int i = 0; i<3; i++){
		exts[i]+=userExts[i];
		exts[i+3]+=userExts[i];
	}
	int rc = Params::getGrids( timeStep, varnames, exts, &actualRefLevel, &lod, &histoGrid);
	
	if(!rc) return;

	histoGrid->SetInterpolationOrder(0);	
	
	float v;
	RegularGrid *rg_const = (RegularGrid *) histoGrid;   
	RegularGrid::Iterator itr;
	
	for (itr = rg_const->begin(); itr!=rg_const->end(); ++itr) {
		v = *itr;
		if (v == histoGrid->GetMissingValue()) continue;
		histo->addToBin(v);
	}
	
	delete histoGrid;

}
