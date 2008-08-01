//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2008
//
//	Description:	Implements the TwoDEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the TwoD tab
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
#include "twoDrenderer.h"
#include "MappingFrame.h"
#include "transferfunction.h"
#include "regionparams.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "twoDframe.h"
#include "floweventrouter.h"
#include "instancetable.h"
#include "qthumbwheel.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "twoDtab.h"
#include "vaporinternal/jpegapi.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"
#include "twoDparams.h"
#include "twoDeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "savetfdialog.h"
#include "loadtfdialog.h"
#include "VolumeRenderer.h"
#include "vapor/errorcodes.h"

using namespace VAPoR;


TwoDEventRouter::TwoDEventRouter(QWidget* parent,const char* name): TwoDtab(parent, name), EventRouter(){
	myParamsType = Params::TwoDParamsType;
	savedCommand = 0;
	ignoreListboxChanges = false;
	numVariables = 0;
	seedAttached = false;
	notNudgingSliders = false;
	
}


TwoDEventRouter::~TwoDEventRouter(){
	if (savedCommand) delete savedCommand;
	
	
}
/**********************************************************
 * Whenever a new TwoDtab is created it must be hooked up here
 ************************************************************/
void
TwoDEventRouter::hookUpTab()
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
	connect (displacementEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
	
	connect (displacementEdit, SIGNAL(returnPressed()), this, SLOT(twoDReturnPressed()));
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
	connect (variableListBox,SIGNAL(selectionChanged(void)), this, SLOT(guiChangeVariables(void)));
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

	connect (opacityScaleSlider, SIGNAL(sliderReleased()), this, SLOT (twoDOpacityScale()));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setTwoDNavigateMode(bool)));
	
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setTwoDEditMode(bool)));
	
	connect(alignButton, SIGNAL(clicked()), this, SLOT(guiSetAligned()));
	
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
	
}
//Insert values from params into tab panel
//
void TwoDEventRouter::updateTab(){
	
	guiSetTextChanged(false);
	notNudgingSliders = true;  //don't generate nudge events

    setEnabled(!Session::getInstance()->sphericalTransform());
	DataStatus* ds = DataStatus::getInstance();
	if (ds->getDataMgr()
		&& ds->getNumMetadataVariables2D()>0) 
			instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);
	
	TwoDParams* twoDParams = VizWinMgr::getActiveTwoDParams();
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	
	guiSetTextChanged(false);


	
	deleteInstanceButton->setEnabled(vizMgr->getNumTwoDInstances(winnum) > 1);
	
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

	int orientation = DataStatus::getInstance()->get2DOrientation(twoDParams->getFirstVarNum());
	if (orientation == 0)
		orientationLabel->setText("X-Y");
	else if (orientation == 1)
		orientationLabel->setText("Y-Z");
	else {
		assert(orientation == 2);
		orientationLabel->setText("X-Z");
	}
	displacementEdit->setText(QString::number(twoDParams->getVerticalDisplacement()));
	bool terrainMap = twoDParams->isMappedToTerrain();
	if (terrainMap) displacementEdit->setEnabled(true);
	else displacementEdit->setEnabled(false);
	applyTerrainCheckbox->setChecked(terrainMap);
	//setup the texture:
	
	resetTextureSize(twoDParams);
	
	QString strn;
	Session* ses = Session::getInstance();
	ses->blockRecording();

    transferFunctionFrame->setMapperFunction(twoDParams->getMapperFunc());
    transferFunctionFrame->updateParams();
	int numvars = 0;
	QString varnames = getMappedVariableNames(&numvars);
    if (numvars > 1)
    {
      transferFunctionFrame->setVariableName(varnames.ascii());
	  QString labelString = "Magnitude of mapped variables("+varnames+") at selected point:";
	  variableLabel->setText(labelString);
    }
	else if (numvars > 0)
	{
      transferFunctionFrame->setVariableName(varnames.ascii());
	  QString labelString = "Value of mapped variable("+varnames+") at selected point:";
	  variableLabel->setText(labelString);
    }
    else
    {
      transferFunctionFrame->setVariableName("");
	  variableLabel->setText("");
    }
	int numRefs = twoDParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentItem(numRefs);
	
	histoScaleEdit->setText(QString::number(twoDParams->GetHistoStretch()));

	
	//setup the size sliders 
	adjustBoxSize(twoDParams);

	//And the center sliders/textboxes:
	float boxmin[3],boxmax[3],boxCenter[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	twoDParams->getBox(boxmin, boxmax);
	for (int i = 0; i<3; i++) boxCenter[i] = (boxmax[i]+boxmin[i])*0.5f;
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-extents[0])/(extents[3]-extents[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-extents[1])/(extents[4]-extents[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-extents[2])/(extents[5]-extents[2])));
	xCenterEdit->setText(QString::number(boxCenter[0]));
	yCenterEdit->setText(QString::number(boxCenter[1]));
	zCenterEdit->setText(QString::number(boxCenter[2]));
	
	
	const float* selectedPoint = twoDParams->getSelectedPoint();
	selectedXLabel->setText(QString::number(selectedPoint[0]));
	selectedYLabel->setText(QString::number(selectedPoint[1]));
	selectedZLabel->setText(QString::number(selectedPoint[2]));
	attachSeedCheckbox->setChecked(seedAttached);
	float val = calcCurrentValue(twoDParams,selectedPoint);

	if (val == OUT_OF_BOUNDS)
		valueMagLabel->setText(QString(" "));
	else valueMagLabel->setText(QString::number(val));
	guiSetTextChanged(false);
	//Set the selection in the variable listbox.
	//Turn off listBox message-listening
	ignoreListboxChanges = true;
	for (int i = 0; i< ds->getNumMetadataVariables2D(); i++){
		if (variableListBox->isSelected(i) != twoDParams->variableIsSelected(ds->mapMetadataToSessionVarNum2D(i)))
			variableListBox->setSelected(i, twoDParams->variableIsSelected(ds->mapMetadataToSessionVarNum2D(i)));
	}
	ignoreListboxChanges = false;

	updateMapBounds(twoDParams);
	
	float sliderVal = twoDParams->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	
	//Set the mode buttons:
	
	if (twoDParams->getEditMode()){
		
		editButton->setOn(true);
		navigateButton->setOn(false);
	} else {
		editButton->setOn(false);
		navigateButton->setOn(true);
	}
		
	
	twoDTextureFrame->setParams(twoDParams);
	
	vizMgr->getTabManager()->update();
	
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	
	notNudgingSliders = false;
}
//Fix for clean Windows scrolling:
void TwoDEventRouter::refreshTab(){
	twoDFrameHolder->hide();
	twoDFrameHolder->show();
	appearanceFrame->hide();
	appearanceFrame->show();
}

void TwoDEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	TwoDParams* twoDParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(twoDParams, "edit TwoD text");
	QString strn;
	
	twoDParams->setHistoStretch(histoScaleEdit->text().toFloat());

	if(twoDParams->isMappedToTerrain())
		twoDParams->setVerticalDisplacement(displacementEdit->text().toFloat());
	
	//Set the twoD size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[0] = xSizeEdit->text().toFloat();
	boxSize[1] = ySizeEdit->text().toFloat();
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
		if (boxSize[i] > maxBoxSize[i]) boxSize[i] = maxBoxSize[i];
	}
	boxCenter[0] = xCenterEdit->text().toFloat();
	boxCenter[1] = yCenterEdit->text().toFloat();
	boxCenter[2] = zCenterEdit->text().toFloat();
	twoDParams->getBox(boxmin, boxmax);
	const float* extents = DataStatus::getInstance()->getExtents();
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
	if (GLWindow::getCurrentMouseMode() == GLWindow::twoDMode){
		VizWinMgr::getInstance()->refreshTwoD(twoDParams);
	}
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, twoDParams);
}


/*********************************************************************************
 * Slots associated with TwoDTab:
 *********************************************************************************/

void TwoDEventRouter::guiChangeInstance(int inst){
	performGuiChangeInstance(inst);
}
void TwoDEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void TwoDEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}
void TwoDEventRouter::guiApplyTerrain(bool mode){
	confirmText(false);
	TwoDParams* dParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle mapping to terrain");
	dParams->setMappedToTerrain(mode);
	displacementEdit->setEnabled(mode);
	
	PanelCommand::captureEnd(cmd, dParams); 
}

void TwoDEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1){performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentItem(0);
	performGuiCopyInstanceToViz(viznum);
}
void TwoDEventRouter::
setTwoDTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void TwoDEventRouter::
twoDReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	confirmText(true);
}

void TwoDEventRouter::
setTwoDEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	TwoDParams* pParams = vizMgr->getTwoDParams(activeViz,instance);
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
	setDatarangeDirty(pParams);
}

void TwoDEventRouter::
setTwoDEditMode(bool mode){
	navigateButton->setOn(!mode);
	guiSetEditMode(mode);
}
void TwoDEventRouter::
setTwoDNavigateMode(bool mode){
	editButton->setOn(!mode);
	guiSetEditMode(!mode);
}

void TwoDEventRouter::
refreshTwoDHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(pParams);
	}
	setEditorDirty();
}
/*
 * Respond to a slider release
 */
void TwoDEventRouter::
twoDOpacityScale() {
	guiSetOpacityScale(opacityScaleSlider->value());
}
void TwoDEventRouter::
twoDLoadInstalledTF(){
	TwoDParams* pParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	//Get the path from the environment:
	char *home = getenv("VAPOR_HOME");
	QString installPath = QString(home)+ "/share/palettes";
	fileLoadTF(pParams, pParams->getSessionVarNum(), installPath.ascii(),false);
	updateClut(pParams);
}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the TwoD params
void TwoDEventRouter::
twoDSaveTF(void){
	TwoDParams* dParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	saveTF(dParams);
}
void TwoDEventRouter::
twoDLoadTF(void){
	TwoDParams* pParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	loadTF(pParams, pParams->getSessionVarNum());
	updateClut(pParams);
}
void TwoDEventRouter::
twoDCenterRegion(){
	TwoDParams* pParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	VizWinMgr::getInstance()->getRegionRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void TwoDEventRouter::
twoDCenterView(){
	TwoDParams* pParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void TwoDEventRouter::
twoDCenterRake(){
	TwoDParams* pParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPoint());
}

void TwoDEventRouter::
twoDAddSeed(){
	Point4 pt;
	TwoDParams* pParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
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
			"the target flow is using a rake instead of seed list");
	}
	//Check that the point is in the current Region:
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float boxMin[3], boxMax[3];
	rParams->getBox(boxMin, boxMax);
	if (pt.getVal(0) < boxMin[0] || pt.getVal(1) < boxMin[1] || pt.getVal(2) < boxMin[2] ||
		pt.getVal(0) > boxMax[0] || pt.getVal(1) > boxMax[1] || pt.getVal(2) > boxMax[2]) {
			MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the seed point is outside the current region");
	}
	fRouter->guiAddSeed(pt);
}	

void TwoDEventRouter::
twoDAttachSeed(bool attach){
	if (attach) twoDAddSeed();
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::FlowParamsType);
	
	guiAttachSeed(attach, fParams);
}


void TwoDEventRouter::
setTwoDXCenter(){
	guiSetXCenter(
		xCenterSlider->value());
}
void TwoDEventRouter::
setTwoDYCenter(){
	guiSetYCenter(
		yCenterSlider->value());
}
void TwoDEventRouter::
setTwoDZCenter(){
	guiSetZCenter(
		zCenterSlider->value());
}
void TwoDEventRouter::
setTwoDXSize(){
	guiSetXSize(
		xSizeSlider->value());
}
void TwoDEventRouter::
setTwoDYSize(){
	guiSetYSize(
		ySizeSlider->value());
}


//Respond to user request to load/save TF
//Assumes name is valid
//
void TwoDEventRouter::
sessionLoadTF(QString* name){
	
	confirmText(false);
	TwoDParams* dParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->ascii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getSessionVarNum();
	dParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	setDatarangeDirty(dParams);
	setEditorDirty();
}

//Make region match twoD.  Responds to button in region panel
void TwoDEventRouter::
guiCopyRegionToTwoD(){
	confirmText(false);
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "copy region to twoD");
	
	for (int i = 0; i< 3; i++){
		pParams->setTwoDMin(i, rParams->getRegionMin(i));
		pParams->setTwoDMax(i, rParams->getRegionMax(i));
	}
	//Note:  the twoD may not fit in the region.  
	updateTab();
	setTwoDDirty(pParams);
	
	PanelCommand::captureEnd(cmd,pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}



//Reinitialize TwoD tab settings, session has changed.
//Note that this is called after the globalTwoDParams are set up, but before
//any of the localTwoDParams are setup.
void TwoDEventRouter::
reinitTab(bool doOverride){
	Session* ses = Session::getInstance();
	
    setEnabled(!ses->sphericalTransform());

	numVariables = DataStatus::getInstance()->getNumSessionVariables2D();
	//Set the names in the variable listbox
	ignoreListboxChanges = true;
	variableListBox->clear();
	for (int i = 0; i< DataStatus::getInstance()->getNumMetadataVariables2D(); i++){
		const std::string& s = DataStatus::getInstance()->getMetadataVarName2D(i);
		const QString& text = QString(s.c_str());
		variableListBox->insertItem(text, i);
	}
	ignoreListboxChanges = false;

	seedAttached = false;

	//Set up the refinement combo:
	const Metadata* md = ses->getCurrentMetadata();
	
	int numRefinements = md->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= numRefinements; i++){
		refinementCombo->insertItem(QString::number(i));
	}
	if (histogramList) {
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete histogramList;
		histogramList = 0;
		numHistograms = 0;
	}
	
	updateTab();
}
//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void TwoDEventRouter::
guiSetEditMode(bool mode){
	confirmText(false);
	TwoDParams* dParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set edit/navigate mode");
	dParams->setEditMode(mode);
	PanelCommand::captureEnd(cmd, dParams); 
}
void TwoDEventRouter::
guiSetAligned(){
	TwoDParams* dParams = (TwoDParams*)VizWinMgr::getInstance()->getApplicableParams(Params::TwoDParamsType);
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "align tf in edit frame");
		
	setEditorDirty();

	update();
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
 * TwoD settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void TwoDEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,   bool newWindow){

	TwoDParams* pParams = (TwoDParams*)rParams;
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


		TwoDRenderer* myRenderer = new TwoDRenderer (viz->getGLWindow(), pParams);
		viz->getGLWindow()->prependRenderer(pParams,myRenderer);

		setTwoDDirty(pParams);
		return;
	}
	
	
	
	//case 6, disable 
	assert(prevEnabled && !nowEnabled); 
	viz->getGLWindow()->removeRenderer(pParams);
	return;
}
void TwoDEventRouter::
setEditorDirty(RenderParams* p){
	TwoDParams* dp = (TwoDParams*)p;
	if(!dp) dp = VizWinMgr::getInstance()->getActiveTwoDParams();
	if(dp->getMapperFunc())dp->getMapperFunc()->setParams(dp);
    transferFunctionFrame->setMapperFunction(dp->getMapperFunc());
    transferFunctionFrame->updateParams();

    Session *session = Session::getInstance();

	if (DataStatus::getInstance()->getNumSessionVariables2D())
    {
	  int tmp;
      transferFunctionFrame->setVariableName(getMappedVariableNames(&tmp).ascii());
    }
    else
    {
      transferFunctionFrame->setVariableName("");
    }
}


void TwoDEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	TwoDParams* pParams = VizWinMgr::getInstance()->getTwoDParams(winnum, instance);    
	confirmText(false);
	assert(value != pParams->isEnabled());
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "toggle twoD enabled",instance);
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
}


//Respond to a change in opacity scale factor
void TwoDEventRouter::
guiSetOpacityScale(int val){
	TwoDParams* pp = VizWinMgr::getActiveTwoDParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(pp, "modify opacity scale slider");
	pp->setOpacityScale( ((float)(256-val))/256.f);
	float sliderVal = pp->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal));
	

	setTwoDDirty(pp);
	twoDTextureFrame->update();
	
	PanelCommand::captureEnd(cmd,pp);
	
	VizWinMgr::getInstance()->setVizDirty(pp,TwoDTextureBit,true);
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void TwoDEventRouter::
guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	TwoDParams* pp = VizWinMgr::getInstance()->getActiveTwoDParams();
    savedCommand = PanelCommand::captureStart(pp, qstr.latin1());
}
void TwoDEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand::captureEnd(savedCommand,pParams);
	savedCommand = 0;
	setTwoDDirty(pParams);
	setDatarangeDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}

void TwoDEventRouter::
guiBindColorToOpac(){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Color to Opacity");
    transferFunctionFrame->bindColorToOpacity();
	PanelCommand::captureEnd(cmd, pParams);
}
void TwoDEventRouter::
guiBindOpacToColor(){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Opacity to Color");
    transferFunctionFrame->bindOpacityToColor();
	PanelCommand::captureEnd(cmd, pParams);
}
//Make the twoD center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void TwoDEventRouter::guiCenterTwoD(){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center TwoD to Selected Point");
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
//Following method sets up (or releases) a connection to the Flow 
void TwoDEventRouter::
guiAttachSeed(bool attach, FlowParams* fParams){
	confirmText(false);
	//Don't capture the attach/detach event.
	//This cannot be easily undone/redone because it requires maintaining the
	//state of both the flowparams and the twoDparams.
	//But we will capture the successive seed moves that occur while
	//the seed is attached.
	if (attach){ 
		attachedFlow = fParams;
		seedAttached = true;
		
	} else {
		seedAttached = false;
		attachedFlow = 0;
	} 
}
//Respond to an update of the variable listbox.  set the appropriate bits
void TwoDEventRouter::
guiChangeVariables(){
	//Don't react if the listbox is being reset programmatically:
	if (ignoreListboxChanges) return;
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "change twoD-selected variable(s)");
	int firstVar = -1;
	int numSelected = 0;
	//Session* ses = Session::getInstance();
	int orientation = 0;
	for (int i = 0; i< DataStatus::getInstance()->getNumMetadataVariables2D(); i++){
		//Index by session variable num:
		int varnum = DataStatus::getInstance()->mapMetadataToSessionVarNum2D(i);
		if (variableListBox->isSelected(i)){
			pParams->setVariableSelected(varnum,true);
			
			if(firstVar == -1) {
				firstVar = varnum;
				orientation = DataStatus::getInstance()->get2DOrientation(i);
			} else if (orientation != DataStatus::getInstance()->get2DOrientation(i)){
				//De-select any variables that don't match the first variable's orientation
				variableListBox->setSelected(i,false);
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
	if (orientation == 0)
		orientationLabel->setText("X-Y");
	else if (orientation == 1)
		orientationLabel->setText("Y-Z");
	else {
		assert(orientation == 2);
		orientationLabel->setText("X-Z");
	}

	//reset the editing display range shown on the tab, 
	//this also sets dirty flag
	updateMapBounds(pParams);
	
	//Force a redraw of the transfer function frame
    setEditorDirty();
	   
	
	PanelCommand::captureEnd(cmd, pParams);
	//Need to update the selected point for the new variables
	updateTab();
	
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
void TwoDEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD X center");
	setXCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}
void TwoDEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD Y center");
	setYCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	
}
void TwoDEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD Z center");
	setZCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD X size");
	setXSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	//setup the texture:
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}
void TwoDEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide twoD Y size");
	setYSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);

}


void TwoDEventRouter::
guiSetNumRefinements(int n){
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
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
	setTwoDDirty(pParams);
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
}
	
//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//Should not change values in params unless the text is invalid.
//
void TwoDEventRouter::
textToSlider(TwoDParams* pParams, int coord, float newCenter, float newSize){
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
	return;
}
//Set text when a slider changes.
//
void TwoDEventRouter::
sliderToText(TwoDParams* pParams, int coord, int slideCenter, int slideSize){
	
	const float* extents = DataStatus::getInstance()->getExtents();
	float newCenter = extents[coord] + ((float)slideCenter)*(extents[coord+3]-extents[coord])/256.f;
	float newSize = maxBoxSize[coord]*(float)slideSize/256.f;
	pParams->setTwoDMin(coord, newCenter-0.5f*newSize);
	pParams->setTwoDMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	//Set the text in the edit boxes

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
void TwoDEventRouter::
updateMapBounds(RenderParams* params){
	TwoDParams* twoDParams = (TwoDParams*)params;
	QString strn;
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
	
	setTwoDDirty(twoDParams);
	setDatarangeDirty(twoDParams);
	setEditorDirty();
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(twoDParams,TwoDTextureBit,true);
	
}

void TwoDEventRouter::setBindButtons(bool canbind)
{
  OpacityBindButton->setEnabled(canbind);
  ColorBindButton->setEnabled(canbind);
}

//Save undo/redo state when user grabs a twoD handle, or maybe a twoD face (later)
//
void TwoDEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	savedCommand = PanelCommand::captureStart(pParams,  "slide twoD handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshTwoD(pParams);
}
//The Manip class will have already changed the box?..
void TwoDEventRouter::
captureMouseUp(){
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	//float boxMin[3],boxMax[3];
	//pParams->getBox(boxMin,boxMax);
	//twoDTextureFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	resetTextureSize(pParams);
	setTwoDDirty(pParams);
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getTwoDParams(viznum)))
			updateTab();
	}
	twoDTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	
}
//When the center slider moves, set the TwoDMin and TwoDMax
void TwoDEventRouter::
setXCenter(TwoDParams* pParams,int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(pParams,0, sliderval, xSizeSlider->value());
	setTwoDDirty(pParams);
}
void TwoDEventRouter::
setYCenter(TwoDParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval, ySizeSlider->value());
	setTwoDDirty(pParams);
}
void TwoDEventRouter::
setZCenter(TwoDParams* pParams,int sliderval){
	sliderToText(pParams,2, sliderval, 0);
	setTwoDDirty(pParams);
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void TwoDEventRouter::
setXSize(TwoDParams* pParams,int sliderval){
	sliderToText(pParams,0, xCenterSlider->value(),sliderval);
	setTwoDDirty(pParams);
}
void TwoDEventRouter::
setYSize(TwoDParams* pParams,int sliderval){
	sliderToText(pParams,1, yCenterSlider->value(),sliderval);
	setTwoDDirty(pParams);
}

//Save undo/redo state when user clicks cursor
//
void TwoDEventRouter::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(VizWinMgr::getActiveTwoDParams(),  "move twoD cursor");
}
void TwoDEventRouter::
guiEndCursorMove(){
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	//Update the selected point:
	//If we are connected to a seed, move it:
	if (seedIsAttached() && attachedFlow){
		VizWinMgr::getInstance()->getFlowRouter()->guiMoveLastSeed(pParams->getSelectedPoint());
	}
	
	//Update the tab, it's in front:
	updateTab();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	setDatarangeDirty(pParams);
	
}
//calculate the variable, or rms of the variables, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not (in region and in twoD).
//


float TwoDEventRouter::
calcCurrentValue(TwoDParams* pParams, const float point[3]){
	double regMin[3],regMax[3];
	if (numVariables <= 0) return OUT_OF_BOUNDS;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) return 0.f;
	if (!pParams->isEnabled()) return 0.f;
	int arrayCoord[3];
	
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	
	//Get the data dimensions (at current resolution):
	
	int numRefinements = pParams->getNumRefinements();
	
	//Find the region that contains the twoD.

	//List the variables we are interested in
	int* sessionVarNums = new int[numVariables];
	int totVars = 0;
	for (int varnum = 0; varnum < ds->getNumSessionVariables2D(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		sessionVarNums[totVars++] = varnum;
	}
	
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	if (0 > rParams->getAvailableVoxelCoords(numRefinements, coordMin, coordMax, blkMin, blkMax, timeStep,
		sessionVarNums, totVars, regMin, regMax)) return OUT_OF_BOUNDS;

	for (int i = 0; i< 3; i++){
		if ((point[i] < regMin[i]) || (point[i] > regMax[i])) return OUT_OF_BOUNDS;
		arrayCoord[i] = coordMin[i] + (int) (0.5f+((float)(coordMax[i]- coordMin[i]))*(point[i] - regMin[i])/(regMax[i]-regMin[i]));
		//Make sure the transformed coords are in the region
		if (arrayCoord[i] < (int)coordMin[i] || arrayCoord[i] > (int)coordMax[i] ) {
			return OUT_OF_BOUNDS;
		} 
	}
	
	const size_t* bSize =  ds->getCurrentMetadata()->GetBlockSize();

	//Get the block coords (in the full volume) of the desired array coordinate:
	for (int i = 0; i< 3; i++){
		blkMin[i] = arrayCoord[i]/bSize[i];
		blkMax[i] = blkMin[i];
	}
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this twoD:
	totVars = 0;
	
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for (int varnum = 0; varnum < ds->getNumSessionVariables2D(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		volData[totVars] = pParams->getContainingVolume(blkMin, blkMax, numRefinements, varnum, timeStep);
		if (!volData[totVars]) {
			//failure to get data.  
			QApplication::restoreOverrideCursor();
			return OUT_OF_BOUNDS;
		}
		totVars++;
	}
	QApplication::restoreOverrideCursor();
			
	int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize[0]) +
		(arrayCoord[1] - blkMin[1]*bSize[1])*(bSize[1]*(blkMax[0]-blkMin[0]+1)) +
		(arrayCoord[2] - blkMin[2]*bSize[2])*(bSize[1]*(blkMax[1]-blkMin[1]+1))*(bSize[0]*(blkMax[0]-blkMin[0]+1));
	
	float varVal;
	//use the int xyzCoord to index into the loaded data
	if (totVars == 1) varVal = volData[0][xyzCoord];
	else { //Add up the squares of the variables
		varVal = 0.f;
		for (int k = 0; k<totVars; k++){
			varVal += volData[k][xyzCoord]*volData[k][xyzCoord];
		}
		varVal = sqrt(varVal);
	}
	delete volData;
	return varVal;
}

//Obtain a new histogram for the current selected variables.
//Save it at the position associated with firstVarNum
void TwoDEventRouter::
refreshHistogram(RenderParams* p){
	TwoDParams* pParams = (TwoDParams*)p;
	int firstVarNum = pParams->getFirstVarNum();
	const float* currentDatarange = pParams->getCurrentDatarange();
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
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
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
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
	//Make sure this box will fit in current 
	//Check if the region/resolution is too big:
	
	float boxExts[6];
	RegionParams::convertToBoxExtents(refLevel,boxMin, boxMax,boxExts); 
	int numMBs = RegionParams::getMBStorageNeeded(boxExts, boxExts+3, refLevel);
	//Check how many variables are needed:
	int varCount = 0;
	for (int varnum = 0; varnum < (int)ds->getNumSessionVariables2D(); varnum++){
		if (pParams->variableIsSelected(varnum)) varCount++;
	}
	int cacheSize = DataStatus::getInstance()->getCacheMB();
	if (numMBs*varCount > (int)(cacheSize*0.75)){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current twoD and resolution.\n%s \n%s",
			"Lower the refinement level, reduce the twoD size, or increase the cache size.",
			"Rendering has been disabled.");
		pParams->setEnabled(false);
		updateTab();
		return;
	}
	int bSize =  *(DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize());
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then histogram rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this twoD:
	int totVars = 0;
	
	for (int varnum = 0; varnum < (int)DataStatus::getInstance()->getNumSessionVariables2D(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		assert(varnum >= firstVarNum);
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		volData[totVars] = pParams->getContainingVolume(blkMin, blkMax, refLevel, varnum, timeStep);
		QApplication::restoreOverrideCursor();
		if (!volData[totVars]) return;
		totVars++;
	}
	
	//Get the data dimensions (at current resolution):
	size_t dataSize[3];
	float gridSpacing[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	
	for (int i = 0; i< 3; i++){
		dataSize[i] = DataStatus::getInstance()->getFullSizeAtLevel(refLevel,i);
		gridSpacing[i] = (extents[i+3]-extents[i])/(float)(dataSize[i]-1);
		//if (boxMin[i]< 0) boxMin[i] = 0;  //unsigned, can't be < 0
		if (boxMax[i] >= dataSize[i]) boxMax[i] = dataSize[i] - 1;
	}
	float voxSize = vlength(gridSpacing);

	//Prepare for test by finding corners and normals to box:
	float corner[8][3];
	float normals[6][3];
	float vec1[3], vec2[3];

	//Get box that is slightly fattened, to ensure nondegenerate normals
	pParams->calcBoxCorners(corner, 0.5*voxSize);
	//The first 6 corners are reference points for testing
	//the 6 normal vectors are outward pointing from these points
	//Normals are calculated as if cube were axis aligned but this is of 
	//course not really true, just gives the right orientation
	//
	// +Z normal: (c2-c0)X(c1-c0)
	vsub(corner[2],corner[0],vec1);
	vsub(corner[1],corner[0],vec2);
	vcross(vec1,vec2,normals[0]);
	vnormal(normals[0]);
	// -Y normal: (c5-c1)X(c0-c1)
	vsub(corner[5],corner[1],vec1);
	vsub(corner[0],corner[1],vec2);
	vcross(vec1,vec2,normals[1]);
	vnormal(normals[1]);
	// +Y normal: (c6-c2)X(c3-c2)
	vsub(corner[6],corner[2],vec1);
	vsub(corner[3],corner[2],vec2);
	vcross(vec1,vec2,normals[2]);
	vnormal(normals[2]);
	// -X normal: (c7-c3)X(c1-c3)
	vsub(corner[7],corner[3],vec1);
	vsub(corner[1],corner[3],vec2);
	vcross(vec1,vec2,normals[3]);
	vnormal(normals[3]);
	// +X normal: (c6-c4)X(c0-c4)
	vsub(corner[6],corner[4],vec1);
	vsub(corner[0],corner[4],vec2);
	vcross(vec1,vec2,normals[4]);
	vnormal(normals[4]);
	// -Z normal: (c7-c5)X(c4-c5)
	vsub(corner[7],corner[5],vec1);
	vsub(corner[4],corner[5],vec2);
	vcross(vec1,vec2,normals[5]);
	vnormal(normals[5]);

	
	float xyz[3];
	//int lastxyz = -1;
	//int incount = 0;
	//int outcount = 0;
	//Now loop over the grid points in the bounding box
	for (size_t k = boxMin[2]; k <= boxMax[2]; k++){
		xyz[2] = extents[2] + (((float)k)/(float)(dataSize[2]-1))*(extents[5]-extents[2]);
		if (xyz[2] > (boxExts[5]+voxSize) || (xyz[2] < boxExts[2]-voxSize)) continue;
		for (size_t j = boxMin[1]; j <= boxMax[1]; j++){
			xyz[1] = extents[1] + (((float)j)/(float)(dataSize[1]-1))*(extents[4]-extents[1]);
			if (xyz[1] > (boxExts[4]+voxSize) || xyz[1] < (boxExts[1]-voxSize)) continue;
			for (size_t i = boxMin[0]; i <= boxMax[0]; i++){
				xyz[0] = extents[0] + (((float)i)/(float)(dataSize[0]-1))*(extents[3]-extents[0]);
				if (xyz[0] > (boxExts[3]+voxSize) || xyz[0] < (boxExts[0]-voxSize)) continue;
				//test if x,y,z is in twoD:
				if (pParams->distanceToCube(xyz, normals, corner) < voxSize){
					//incount++;
					//Point is (almost) inside.
					//Evaluate the variable(s):
					int xyzCoord = (i - blkMin[0]*bSize) +
						(j - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
						(k - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
					//qWarning(" sampled coord %d",xyzCoord);
	
					assert(xyzCoord >= 0);
					assert(xyzCoord < (int)((blkMax[0]-blkMin[0]+1)*(blkMax[1]-blkMin[1]+1)*(blkMax[2]-blkMin[2]+1)*bSize*bSize*bSize));
					float varVal;
					//use the int xyzCoord to index into the loaded data
					if (totVars == 1) varVal = volData[0][xyzCoord];
					else { //Add up the squares of the variables
						varVal = 0.f;
						for (int d = 0; d<totVars; d++){
							varVal += volData[d][xyzCoord]*volData[d][xyzCoord];
						}
						varVal = sqrt(varVal);
					}
					histo->addToBin(varVal);
				
				} 
				//else {
				//	outcount++;
				//}
			}
		}
	}

}

	
//Method called when undo/redo changes params.  It does the following:
//  puts the new params into the vizwinmgr, deletes the old one
//  Updates the tab if it's the current instance
//  Calls updateRenderer to rebuild renderer 
//	Makes the vizwin update.
void TwoDEventRouter::
makeCurrent(Params* prevParams, Params* nextParams, bool newWin, int instance,bool) {

	assert(instance >= 0);
	TwoDParams* pParams = (TwoDParams*)(nextParams->deepCopy());
	int vizNum = pParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumTwoDInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::TwoDParamsType, instance);
	setEditorDirty();
	updateTab();
	TwoDParams* formerParams = (TwoDParams*)prevParams;
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
void TwoDEventRouter::
setDatarangeDirty(RenderParams* params)
{
	TwoDParams* pParams = (TwoDParams*)params;
	if (!pParams->getMapperFunc()) return;
	const float* currentDatarange = pParams->getCurrentDatarange();
	float minval = pParams->getMapperFunc()->getMinColorMapValue();
	float maxval = pParams->getMapperFunc()->getMaxColorMapValue();
	if (currentDatarange[0] != minval || currentDatarange[1] != maxval){
			pParams->setCurrentDatarange(minval, maxval);
			VizWinMgr::getInstance()->setVizDirty(pParams,TwoDTextureBit,true);
	}
	
}

void TwoDEventRouter::cleanParams(Params* p) 
{
  transferFunctionFrame->setMapperFunction(NULL);
  transferFunctionFrame->setVariableName("");
  transferFunctionFrame->update();
}
	
//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void TwoDEventRouter::captureImage() {
	QFileDialog fileDialog(Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg Images (*.jpg)",
		this,
		"Image capture dialog",
		true);  //modal
	//fileDialog.move(pos());
	fileDialog.setMode(QFileDialog::AnyFile);
	fileDialog.setCaption("Specify image capture file name");
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QString filename = fileDialog.selectedFile();
    
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	
	if (fileInfo->exists()){
		int rc = QMessageBox::warning(0, "File Exists", QString("OK to replace existing jpeg file?\n %1 ").arg(filename), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory(fileInfo->dirPath(true).ascii());
	if (!filename.endsWith(".jpg")) filename += ".jpg";
	//
	//If this is IBFV, then we save texture as is.
	//If this is data, then reconstruct with appropriate aspect ratio.
	
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int imgSize[2];
	pParams->getTextureSize(imgSize);
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
				twoDTex[k+3*(i+wid*j)] = buf[k+4*(i+wid*(ht-j+1))];
		}
	}
		
	
	
	
	
	//Now open the jpeg file:
	FILE* jpegFile = fopen(filename.ascii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: Error opening output Jpeg file: %s",filename.ascii());
		return;
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = GLWindow::getJpegQuality();
	int rc = write_JPEG_file(jpegFile, wid, ht, twoDTex, quality);
	delete twoDTex;
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; Error writing jpeg file %s",
			filename.ascii());
		delete buf;
		return;
	}
	//Provide a message stating the capture in effect.
	MessageReporter::infoMsg("Image is captured to %s",
			filename.ascii());
}

void TwoDEventRouter::guiNudgeXSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	
	//ignore if change is not 1 
	if(abs(val - lastXSizeSlider) != 1) {
		lastXSizeSlider = val;
		return;
	}
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	
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
}
void TwoDEventRouter::guiNudgeXCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastXCenterSlider) != 1) {
		lastXCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	
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
}
void TwoDEventRouter::guiNudgeYCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYCenterSlider) != 1) {
		lastYCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	
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
}
void TwoDEventRouter::guiNudgeZCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastZCenterSlider) != 1) {
		lastZCenterSlider = val;
		return;
	}
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	
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
}

void TwoDEventRouter::guiNudgeYSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYSizeSlider) != 1) {
		lastYSizeSlider = val;
		return;
	}
	confirmText(false);
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
	
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
}

void TwoDEventRouter::
adjustBoxSize(TwoDParams* pParams){
	//Determine the max x, y, z sizes of twoD:
	
	float boxmin[3], boxmax[3];
	//Don't do anything if we haven't read the data yet:
	if (!Session::getInstance()->getDataMgr()) return;
	pParams->getBox(boxmin, boxmax);
	float rotMatrix[9];
	getRotationMatrix(pParams->getTheta()*M_PI/180.,pParams->getPhi()*M_PI/180., pParams->getPsi()*M_PI/180., rotMatrix);
	//Apply rotation matrix inverted to full domain size
	const float* extents = DataStatus::getInstance()->getExtents();
	float extentSize[3];
	for (int i= 0; i<3; i++) extentSize[i] = (extents[i+3]-extents[i]);
	
	//Transform each side of box by rotMatrix.   Effectively that amounts
	//to multiplying each column of rotMatrix by the box side
	//If it projects longer (in any dimension) than extents,
	//then that side must be shrunk appropriately
	//In other words, the max box is obtained by taking a column and multiplying it
	//by the max extents in that dimension.
	//The largest box size is obtained by finding the minimum of
	//extentSize[i]/norm(col(i))

	//For each j (axis) find max over i of (col(j))sub i /extent[i].
	//This is the reciprocal of the max value of j box side.
	for (int axis = 0; axis<3; axis++){
		float maxval = 0.;
		for (int i = 0; i<3; i++){
			if ((abs(rotMatrix[3*i+axis])/extentSize[i])> maxval)
				maxval = (abs(rotMatrix[3*i+axis])/extentSize[i]);
		}
		assert(maxval > 0.f);
		maxBoxSize[axis] = 1.f/maxval;
	}
	
	
	//Now make sure the twoD box fits
	bool boxOK = true;
	float boxmid[3];
	for (int i = 0; i<3; i++){
		if ((boxmax[i]-boxmin[i]) > maxBoxSize[i]){
			boxOK = false;
			boxmid[i] = 0.5f*(boxmax[i]+boxmin[i]);
			boxmin[i] = boxmid[i] - 0.5f* maxBoxSize[i];
			boxmax[i] = boxmid[i] + 0.5f*maxBoxSize[i];
		}
	}
	
	pParams->setBox(boxmin, boxmax);
	//Set the size sliders appropriately:
	xSizeEdit->setText(QString::number(boxmax[0]-boxmin[0]));
	ySizeEdit->setText(QString::number(boxmax[1]-boxmin[1]));
	
	xSizeSlider->setValue((int)(256.f*(boxmax[0]-boxmin[0])/(maxBoxSize[0])));
	ySizeSlider->setValue((int)(256.f*(boxmax[1]-boxmin[1])/(maxBoxSize[1])));
	
	//Cancel any response to text events generated in this method:
	//
	guiSetTextChanged(false);
}
void TwoDEventRouter::resetTextureSize(TwoDParams* twoDParams){
	//setup the texture:
	float voxDims[2];
	twoDParams->getTwoDVoxelExtents(voxDims);
	twoDTextureFrame->setTextureSize(voxDims[0],voxDims[1]);
}



QString TwoDEventRouter::getMappedVariableNames(int* numvars){
	TwoDParams* pParams = VizWinMgr::getActiveTwoDParams();
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
