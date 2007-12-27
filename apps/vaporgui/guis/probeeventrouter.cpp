//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the ProbeEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the region tab
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
#include <qapplication.h>
#include <qcursor.h>
#include <qtooltip.h>
#include "proberenderer.h"
#include "MappingFrame.h"
#include "transferfunction.h"
#include "regionparams.h"
#include "regiontab.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "probeframe.h"
#include "floweventrouter.h"
#include "instancetable.h"
#include "qthumbwheel.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "probetab.h"
#include "vaporinternal/jpegapi.h"
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"
#include "probeparams.h"
#include "probeeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "savetfdialog.h"
#include "loadtfdialog.h"
#include "VolumeRenderer.h"
#include "vapor/errorcodes.h"

using namespace VAPoR;


ProbeEventRouter::ProbeEventRouter(QWidget* parent,const char* name): ProbeTab(parent, name), EventRouter(){
	myParamsType = Params::ProbeParamsType;
	savedCommand = 0;
	ignoreListboxChanges = false;
	numVariables = 0;
	seedAttached = false;
	notNudgingSliders = false;
	
}


ProbeEventRouter::~ProbeEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Probetab is created it must be hooked up here
 ************************************************************/
void
ProbeEventRouter::hookUpTab()
{
	//Nudge sliders by clicking on slider bar:
	connect (xSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXSize(int)));
	connect (xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXCenter(int)));
	connect (ySizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYSize(int)));
	connect (yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYCenter(int)));
	connect (zSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeZSize(int)));
	connect (zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeZCenter(int)));
	connect (xCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (yCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (zCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (thetaEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (phiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (psiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (xSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (ySizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (zSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	
	connect (leftMappingBound, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (rightMappingBound, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	
	connect (xCenterEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (yCenterEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (zCenterEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (xSizeEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (ySizeEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (zSizeEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (thetaEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (phiEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (psiEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (histoScaleEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (regionCenterButton, SIGNAL(clicked()), this, SLOT(probeCenterRegion()));
	connect (viewCenterButton, SIGNAL(clicked()), this, SLOT(probeCenterView()));
	connect (rakeCenterButton, SIGNAL(clicked()), this, SLOT(probeCenterRake()));
	connect (probeCenterButton, SIGNAL(clicked()), this, SLOT(guiCenterProbe()));
	connect (addSeedButton, SIGNAL(clicked()), this, SLOT(probeAddSeed()));
	connect (axisAlignButton, SIGNAL(clicked()), this, SLOT(guiAxisAlign()));
	connect (xThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateXWheel(int)));
	connect (yThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateYWheel(int)));
	connect (zThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateZWheel(int)));
	connect (xThumbWheel, SIGNAL(released(int)), this, SLOT(guiReleaseXWheel(int)));
	connect (yThumbWheel, SIGNAL(released(int)), this, SLOT(guiReleaseYWheel(int)));
	connect (zThumbWheel, SIGNAL(released(int)), this, SLOT(guiReleaseZWheel(int)));
	connect (planarCheckbox, SIGNAL(toggled(bool)), this, SLOT(guiTogglePlanar(bool)));
	connect (attachSeedCheckbox,SIGNAL(toggled(bool)),this, SLOT(probeAttachSeed(bool)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	
	connect (variableListBox,SIGNAL(selectionChanged(void)), this, SLOT(guiChangeVariables(void)));
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setProbeXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setProbeYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setProbeZCenter()));
	connect (xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setProbeXSize()));
	connect (ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setProbeYSize()));
	connect (zSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setProbeZSize()));
	
	connect (loadButton, SIGNAL(clicked()), this, SLOT(probeLoadTF()));
	connect (loadInstalledButton, SIGNAL(clicked()), this, SLOT(probeLoadInstalledTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(probeSaveTF()));
	
	connect (captureButton, SIGNAL(clicked()), this, SLOT(captureImage()));
	connect (leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));

	connect (opacityScaleSlider, SIGNAL(sliderReleased()), this, SLOT (probeOpacityScale()));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setProbeNavigateMode(bool)));
	
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setProbeEditMode(bool)));
	
	connect(alignButton, SIGNAL(clicked()), this, SLOT(guiSetAligned()));
	
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshProbeHisto()));
	
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
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setProbeEnabled(bool,int)));
	
}

/*********************************************************************************
 * Slots associated with ProbeTab:
 *********************************************************************************/
void ProbeEventRouter::
rotateXWheel(int val){
	
	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = viz->getGLWindow()->getProbeManip();
	manip->setTempRotation((float)val/10.f, 0);
	viz->updateGL();
	
}
void ProbeEventRouter::
rotateYWheel(int val){
	
	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = viz->getGLWindow()->getProbeManip();
	manip->setTempRotation(-(float)val/10.f, 1);
	viz->updateGL();
}
void ProbeEventRouter::
rotateZWheel(int val){
	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = viz->getGLWindow()->getProbeManip();
	manip->setTempRotation((float)val/10.f, 2);
	viz->updateGL();
}
void ProbeEventRouter::
guiReleaseXWheel(int val){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "rotate probe");

	//Renormalize and apply rotation:
	pParams->rotateAndRenormalizeBox(0, (float)val/10.f);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = viz->getGLWindow()->getProbeManip();
	manip->setTempRotation(0.f, 0);
	
	updateTab();
	pParams->setProbeDirty();
	PanelCommand::captureEnd(cmd,pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
void ProbeEventRouter::
guiReleaseYWheel(int val){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "rotate probe");
	//Renormalize and apply rotation:
	pParams->rotateAndRenormalizeBox(1, -(float)val/10.f);
	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = viz->getGLWindow()->getProbeManip();
	manip->setTempRotation(0.f, 1);
	
	updateTab();
	pParams->setProbeDirty();
	PanelCommand::captureEnd(cmd,pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
void ProbeEventRouter::
guiReleaseZWheel(int val){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "rotate probe");
	//Renormalize and apply rotation:
	pParams->rotateAndRenormalizeBox(2, (float)val/10.f);
	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = viz->getGLWindow()->getProbeManip();
	manip->setTempRotation(0.f, 2);
	
	updateTab();
	pParams->setProbeDirty();
	PanelCommand::captureEnd(cmd,pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
void ProbeEventRouter::guiChangeInstance(int inst){
	performGuiChangeInstance(inst);
}
void ProbeEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void ProbeEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void ProbeEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1){performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentItem(0);
	performGuiCopyInstanceToViz(viznum);
}
void ProbeEventRouter::
setProbeTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void ProbeEventRouter::
probeReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	confirmText(true);
}

void ProbeEventRouter::
setProbeEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	ProbeParams* pParams = vizMgr->getProbeParams(activeViz,instance);
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

void ProbeEventRouter::
setProbeEditMode(bool mode){
	navigateButton->setOn(!mode);
	guiSetEditMode(mode);
}
void ProbeEventRouter::
setProbeNavigateMode(bool mode){
	editButton->setOn(!mode);
	guiSetEditMode(!mode);
}

void ProbeEventRouter::
refreshProbeHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(pParams);
	}
	setEditorDirty();
}
/*
 * Respond to a slider release
 */
void ProbeEventRouter::
probeOpacityScale() {
	guiSetOpacityScale(opacityScaleSlider->value());
}
void ProbeEventRouter::
probeLoadInstalledTF(){
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	//Get the path from the environment:
	char *home = getenv("VAPOR_HOME");
	QString installPath = QString(home)+ "/share/palettes";
	fileLoadTF(pParams,installPath.ascii(),false);
}
void ProbeEventRouter::
probeLoadTF(void){
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	
	//If there are no TF's currently in Session, just launch file load dialog.
	if (Session::getInstance()->getNumTFs() > 0){
		LoadTFDialog* loadTFDialog = new LoadTFDialog(this, this,
			"Load TF Dialog", true);
		int rc = loadTFDialog->exec();
		if (rc == 0) return;
		if (rc == 1) fileLoadTF(pParams, Session::getInstance()->getTFFilePath().c_str(), true);
		//if rc == 2, we already (probably) loaded a tf from the session
	} else {
		fileLoadTF(pParams, Session::getInstance()->getTFFilePath().c_str(), true);
	}
	
	setEditorDirty();
}
void ProbeEventRouter::
probeCenterRegion(){
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	VizWinMgr::getInstance()->getRegionRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void ProbeEventRouter::
probeCenterView(){
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPoint());
}
void ProbeEventRouter::
probeCenterRake(){
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPoint());
}

void ProbeEventRouter::
probeAddSeed(){
	Point4 pt;
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
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
void ProbeEventRouter::
guiTogglePlanar(bool isOn){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "toggle planar probe");
	if (isOn){
		//when make it planar, force z-thickness to 0:
		pParams->setPlanar(true);
		setZSize(pParams,0);
		zSizeSlider->setEnabled(false);
		zSizeEdit->setEnabled(false);
		probeTextureFrame->update();
		updateTab();
	} else {
		pParams->setPlanar(false);
		zSizeSlider->setEnabled(true);
		zSizeEdit->setEnabled(true);
	}
	//Force a redraw
	
	pParams->setProbeDirty();
	PanelCommand::captureEnd(cmd,pParams);
	
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
void ProbeEventRouter::
guiAxisAlign(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "axis-align probe");
	float theta = pParams->getTheta();
	//convert to closest number of quarter-turns
	int angleInt = (int)(((theta+180.)/90.)+0.5) - 2;
	theta = angleInt*90.;
	float psi = pParams->getPsi();
	angleInt = (int)(((psi+180.)/90.)+0.5) - 2;
	psi = angleInt*90.;
	float phi = pParams->getPhi();
	angleInt = (int)((phi/90.)+0.5);
	phi = angleInt*90.;
	pParams->setPhi(phi);
	pParams->setPsi(psi);
	pParams->setTheta(theta);
	//Force a redraw, update tab
	updateTab();
	pParams->setProbeDirty();
	PanelCommand::captureEnd(cmd,pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}	
void ProbeEventRouter::
probeAttachSeed(bool attach){
	if (attach) probeAddSeed();
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::FlowParamsType);
	
	guiAttachSeed(attach, fParams);
}


void ProbeEventRouter::
setProbeXCenter(){
	guiSetXCenter(
		xCenterSlider->value());
}
void ProbeEventRouter::
setProbeYCenter(){
	guiSetYCenter(
		yCenterSlider->value());
}
void ProbeEventRouter::
setProbeZCenter(){
	guiSetZCenter(
		zCenterSlider->value());
}
void ProbeEventRouter::
setProbeXSize(){
	guiSetXSize(
		xSizeSlider->value());
}
void ProbeEventRouter::
setProbeYSize(){
	guiSetYSize(
		ySizeSlider->value());
}
void ProbeEventRouter::
setProbeZSize(){
	guiSetZSize(
		zSizeSlider->value());
}

void ProbeEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	ProbeParams* probeParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(probeParams, "edit Probe text");
	QString strn;
	
	float thetaVal = thetaEdit->text().toFloat();
	while (thetaVal > 180.f) thetaVal -= 360.f;
	while (thetaVal < -180.f) thetaVal += 360.f;
	thetaEdit->setText(QString::number(thetaVal,'f',1));
	float phiVal = phiEdit->text().toFloat();
	while (phiVal > 180.f) phiVal -= 180.f;
	while (phiVal < 0.f) phiVal += 180.f;
	phiEdit->setText(QString::number(phiVal,'f',1));
	float psiVal = psiEdit->text().toFloat();
	while (psiVal > 180.f) psiVal -= 360.f;
	while (psiVal < -180.f) psiVal += 360.f;
	psiEdit->setText(QString::number(psiVal,'f',1));

	probeParams->setTheta(thetaVal);
	probeParams->setPhi(phiVal);
	probeParams->setPsi(psiVal);

	probeParams->setHistoStretch(histoScaleEdit->text().toFloat());

	//Set the probe size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[0] = xSizeEdit->text().toFloat();
	boxSize[1] = ySizeEdit->text().toFloat();
	boxSize[2] = zSizeEdit->text().toFloat();
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
		if (boxSize[i] > maxBoxSize[i]) boxSize[i] = maxBoxSize[i];
	}
	boxCenter[0] = xCenterEdit->text().toFloat();
	boxCenter[1] = yCenterEdit->text().toFloat();
	boxCenter[2] = zCenterEdit->text().toFloat();
	probeParams->getBox(boxmin, boxmax);
	const float* extents = DataStatus::getInstance()->getExtents();
	for (int i = 0; i<3;i++){
		if (boxCenter[i] < extents[i])boxCenter[i] = extents[i];
		if (boxCenter[i] > extents[i+3])boxCenter[i] = extents[i+3];
		boxmin[i] = boxCenter[i] - 0.5f*boxSize[i];
		boxmax[i] = boxCenter[i] + 0.5f*boxSize[i];
	}
	probeParams->setBox(boxmin,boxmax);
	adjustBoxSize(probeParams);
	//set the center sliders:
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-extents[0])/(extents[3]-extents[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-extents[1])/(extents[4]-extents[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-extents[2])/(extents[5]-extents[2])));
	resetTextureSize(probeParams);
	//probeTextureFrame->setTextureSize(voxDims[0],voxDims[1]);
	probeParams->setProbeDirty();
	if (probeParams->getMapperFunc()) {
		((TransferFunction*)probeParams->getMapperFunc())->setMinMapValue(leftMappingBound->text().toFloat());
		((TransferFunction*)probeParams->getMapperFunc())->setMaxMapValue(rightMappingBound->text().toFloat());
	
		setDatarangeDirty(probeParams);
		setEditorDirty();
		update();
		probeTextureFrame->update();
	}
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(probeParams,ProbeTextureBit,true);
	//If we are in probe mode, force a rerender of all windows using the probe:
	if (GLWindow::getCurrentMouseMode() == GLWindow::probeMode){
		VizWinMgr::getInstance()->refreshProbe(probeParams);
	}
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, probeParams);
}




//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the Probe params
void ProbeEventRouter::
probeSaveTF(void){
	ProbeParams* dParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	SaveTFDialog* saveTFDialog = new SaveTFDialog(dParams,this,
		"Save TF Dialog", true);
	int rc = saveTFDialog->exec();
	if (rc == 1) fileSaveTF(dParams);
}
void ProbeEventRouter::
fileSaveTF(ProbeParams* dParams){
	//Launch a file save dialog, open resulting file
    QString s = QFileDialog::getSaveFileName(
					Session::getInstance()->getTFFilePath().c_str(),
                    "Vapor Transfer Functions (*.vtf)",
                    0,
                    "save TF dialog",
                    "Choose a filename to save the transfer function" );
	//Did the user cancel?
	if (s.length()== 0) return;
	//Force the name to end with .vtf
	if (!s.endsWith(".vtf")){
		s += ".vtf";
	}
	QFileInfo finfo(s);
	if (finfo.exists()){
		int rc = QMessageBox::warning(0, "Transfer Function File Exists", QString("OK to replace transfer function file \n%1 ?").arg(s), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	ofstream fileout;
	fileout.open(s.ascii());
	if (! fileout) {
		QString str("Unable to save to file: \n");
		str += s;
		MessageReporter::errorMsg( str.ascii());
		return;
	}
	
	
	
	if (!((TransferFunction*)(dParams->getMapperFunc()))->saveToFile(fileout)){//Report error if can't save to file
		QString str("Failed to write output file: \n");
		str += s;
		MessageReporter::errorMsg(str.ascii());
		fileout.close();
		return;
	}
	fileout.close();
	Session::getInstance()->updateTFFilePath(&s);
}

//Respond to user request to load/save TF
//Assumes name is valid
//
void ProbeEventRouter::
sessionLoadTF(QString* name){
	
	confirmText(false);
	ProbeParams* dParams = VizWinMgr::getActiveProbeParams();
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
void ProbeEventRouter::
fileLoadTF(ProbeParams* dParams, const char* path, bool savePath){
	
	//The transfer function is indexed by firstVarNum, which is a session variable num
	int varNum = dParams->getSessionVarNum();
	//Open a file load dialog
	
    QString s = QFileDialog::getOpenFileName(
                    path,
                    "Vapor Transfer Functions (*.vtf)",
                    0,
                    "load TF dialog",
                    "Choose a transfer function file to open" );
	//Null string indicates nothing selected.
	if (s.length() == 0) return;
	//Force the name to end with .vtf
	if (!s.endsWith(".vtf")){
		s += ".vtf";
	}
	
	ifstream is;
	
	is.open(s.ascii());

	if (!is){//Report error if you can't open the file
		QString str("Unable to open file: \n");
		str+= s;
		MessageReporter::errorMsg(str.ascii());
		return;
	}
	//Start the history save:
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Load Transfer Function from file");
	
      TransferFunction* t = TransferFunction::loadFromFile(is, dParams);
	if (!t){//Report error if can't load
		QString str("Error loading transfer function. /nFailed to convert input file: \n ");
		str += s;
		MessageReporter::errorMsg(str.ascii());
		//Don't put this into history!
		delete cmd;
		return;
	}

	dParams->hookupTF(t, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	//Remember the path to the file:
	if(savePath) Session::getInstance()->updateTFFilePath(&s);
	setDatarangeDirty(dParams);
	setEditorDirty();
}

//Insert values from params into tab panel
//
void ProbeEventRouter::updateTab(){
	notNudgingSliders = true;  //don't generate nudge events

    setEnabled(!Session::getInstance()->sphericalTransform());

	if (DataStatus::getInstance()->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);
	
	ProbeParams* probeParams = VizWinMgr::getActiveProbeParams();
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	
	deleteInstanceButton->setEnabled(vizMgr->getNumProbeInstances(winnum) > 1);
	if (planarCheckbox->isChecked() != probeParams->isPlanar()){
		planarCheckbox->setChecked(probeParams->isPlanar());
	}

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
	
	resetTextureSize(probeParams);
	
	QString strn;
	Session* ses = Session::getInstance();
	ses->blockRecording();

    transferFunctionFrame->setMapperFunction(probeParams->getMapperFunc());
    transferFunctionFrame->update();

    if (ses->getNumSessionVariables())
    {
      int varnum = probeParams->getSessionVarNum();
      const std::string& varname = ses->getVariableName(varnum);
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("");
    }
	int numRefs = probeParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentItem(numRefs);
	
	histoScaleEdit->setText(QString::number(probeParams->GetHistoStretch()));

	//Check if planar:
	bool isPlanar = probeParams->isPlanar();
	if (isPlanar){
		zSizeSlider->setEnabled(false);
		zSizeEdit->setEnabled(false);
	} else {
		zSizeSlider->setEnabled(true);
		zSizeEdit->setEnabled(true);
	}
	//setup the size sliders 
	adjustBoxSize(probeParams);

	//And the center sliders/textboxes:
	float boxmin[3],boxmax[3],boxCenter[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	probeParams->getBox(boxmin, boxmax);
	for (int i = 0; i<3; i++) boxCenter[i] = (boxmax[i]+boxmin[i])*0.5f;
	xCenterSlider->setValue((int)(256.f*(boxCenter[0]-extents[0])/(extents[3]-extents[0])));
	yCenterSlider->setValue((int)(256.f*(boxCenter[1]-extents[1])/(extents[4]-extents[1])));
	zCenterSlider->setValue((int)(256.f*(boxCenter[2]-extents[2])/(extents[5]-extents[2])));
	xCenterEdit->setText(QString::number(boxCenter[0]));
	yCenterEdit->setText(QString::number(boxCenter[1]));
	zCenterEdit->setText(QString::number(boxCenter[2]));
	
	thetaEdit->setText(QString::number(probeParams->getTheta(),'f',1));
	phiEdit->setText(QString::number(probeParams->getPhi(),'f',1));
	psiEdit->setText(QString::number(probeParams->getPsi(),'f',1));
	const float* selectedPoint = probeParams->getSelectedPoint();
	selectedXLabel->setText(QString::number(selectedPoint[0]));
	selectedYLabel->setText(QString::number(selectedPoint[1]));
	selectedZLabel->setText(QString::number(selectedPoint[2]));
	attachSeedCheckbox->setChecked(seedAttached);
	float val = calcCurrentValue(probeParams,selectedPoint);

	if (val == OUT_OF_BOUNDS)
		valueMagLabel->setText(QString(" "));
	else valueMagLabel->setText(QString::number(val));

	//Set the selection in the variable listbox.
	//Turn off listBox message-listening
	ignoreListboxChanges = true;
	for (int i = 0; i< ses->getNumMetadataVariables(); i++){
		if (variableListBox->isSelected(i) != probeParams->variableIsSelected(ses->mapMetadataToSessionVarNum(i)))
			variableListBox->setSelected(i, probeParams->variableIsSelected(ses->mapMetadataToSessionVarNum(i)));
	}
	ignoreListboxChanges = false;

	updateMapBounds(probeParams);
	
	float sliderVal = probeParams->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	
	//Set the mode buttons:
	
	if (probeParams->getEditMode()){
		
		editButton->setOn(true);
		navigateButton->setOn(false);
	} else {
		editButton->setOn(false);
		navigateButton->setOn(true);
	}
		
	
	probeTextureFrame->setParams(probeParams);

	update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	vizMgr->getTabManager()->update();
	notNudgingSliders = false;
}
//Make region match probe.  Responds to button in region panel
void ProbeEventRouter::
guiCopyRegionToProbe(){
	confirmText(false);
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "copy region to probe");
	if (pParams->isPlanar()){//maybe need to turn off planar:
		if (rParams->getRegionMin(2) < rParams->getRegionMax(2)){
			pParams->setPlanar(false);
		}
	}
	for (int i = 0; i< 3; i++){
		pParams->setProbeMin(i, rParams->getRegionMin(i));
		pParams->setProbeMax(i, rParams->getRegionMax(i));
	}
	//Note:  the probe may not fit in the region.  
	updateTab();
	pParams->setProbeDirty();
	
	PanelCommand::captureEnd(cmd,pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	
}



//Reinitialize Probe tab settings, session has changed.
//Note that this is called after the globalProbeParams are set up, but before
//any of the localProbeParams are setup.
void ProbeEventRouter::
reinitTab(bool doOverride){
	Session* ses = Session::getInstance();
	
    setEnabled(!ses->sphericalTransform());

	numVariables = DataStatus::getInstance()->getNumSessionVariables();
	//Set the names in the variable listbox
	ignoreListboxChanges = true;
	variableListBox->clear();
	for (int i = 0; i< DataStatus::getInstance()->getNumMetadataVariables(); i++){
		const std::string& s = DataStatus::getInstance()->getMetadataVarName(i);
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
void ProbeEventRouter::
guiSetEditMode(bool mode){
	confirmText(false);
	ProbeParams* dParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set edit/navigate mode");
	dParams->setEditMode(mode);
	PanelCommand::captureEnd(cmd, dParams); 
}
void ProbeEventRouter::
guiSetAligned(){
	ProbeParams* dParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
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
 * Probe settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void ProbeEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,   bool newWindow){

	ProbeParams* pParams = (ProbeParams*)rParams;
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


		ProbeRenderer* myRenderer = new ProbeRenderer (viz->getGLWindow(), pParams);
		viz->getGLWindow()->prependRenderer(pParams,myRenderer);

		pParams->setProbeDirty();
		return;
	}
	
	
	
	//case 6, disable 
	assert(prevEnabled && !nowEnabled); 
	viz->getGLWindow()->removeRenderer(pParams);
	return;
}
void ProbeEventRouter::
setEditorDirty(RenderParams* p){
	ProbeParams* dp = (ProbeParams*)p;
	if(!dp) dp = VizWinMgr::getInstance()->getActiveProbeParams();
	if(dp->getMapperFunc())dp->getMapperFunc()->setParams(dp);
    transferFunctionFrame->setMapperFunction(dp->getMapperFunc());
    transferFunctionFrame->update();

    Session *session = Session::getInstance();

    if (session->getNumSessionVariables())
    {
      int varnum = dp->getSessionVarNum();
      const std::string& varname = session->getVariableName(varnum);
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("");
    }
}


void ProbeEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	ProbeParams* pParams = VizWinMgr::getInstance()->getProbeParams(winnum, instance);    
	confirmText(false);
	assert(value != pParams->isEnabled());
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "toggle probe enabled",instance);
	pParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, pParams);
	//Need to rerender the texture:
	pParams->setProbeDirty();
	//and refresh the gui
	updateTab();
	setDatarangeDirty(pParams);
	setEditorDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	update();
}


//Respond to a change in opacity scale factor
void ProbeEventRouter::
guiSetOpacityScale(int val){
	ProbeParams* pp = VizWinMgr::getActiveProbeParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(pp, "modify opacity scale slider");
	pp->setOpacityScale( ((float)(256-val))/256.f);
	float sliderVal = pp->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	

	pp->setProbeDirty();
	probeTextureFrame->update();
	
	PanelCommand::captureEnd(cmd,pp);
	
	VizWinMgr::getInstance()->setVizDirty(pp,ProbeTextureBit,true);
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void ProbeEventRouter::
guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	ProbeParams* pp = VizWinMgr::getInstance()->getActiveProbeParams();
    savedCommand = PanelCommand::captureStart(pp, qstr.latin1());
}
void ProbeEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand::captureEnd(savedCommand,pParams);
	savedCommand = 0;
	pParams->setProbeDirty();
	setDatarangeDirty(pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}

void ProbeEventRouter::
guiBindColorToOpac(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Color to Opacity");
    transferFunctionFrame->bindColorToOpacity();
	PanelCommand::captureEnd(cmd, pParams);
}
void ProbeEventRouter::
guiBindOpacToColor(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Opacity to Color");
    transferFunctionFrame->bindOpacityToColor();
	PanelCommand::captureEnd(cmd, pParams);
}
//Make the probe center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void ProbeEventRouter::guiCenterProbe(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center Probe to Selected Point");
	const float* selectedPoint = pParams->getSelectedPoint();
	float probeMin[3],probeMax[3];
	pParams->getBox(probeMin,probeMax);
	for (int i = 0; i<3; i++)
		textToSlider(pParams,i,selectedPoint[i], probeMax[i]-probeMin[i]);
	PanelCommand::captureEnd(cmd, pParams);
	updateTab();
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}
//Following method sets up (or releases) a connection to the Flow 
void ProbeEventRouter::
guiAttachSeed(bool attach, FlowParams* fParams){
	confirmText(false);
	//Don't capture the attach/detach event.
	//This cannot be easily undone/redone because it requires maintaining the
	//state of both the flowparams and the probeparams.
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
void ProbeEventRouter::
guiChangeVariables(){
	//Don't react if the listbox is being reset programmatically:
	if (ignoreListboxChanges) return;
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "change probe-selected variable(s)");
	int firstVar = -1;
	int numSelected = 0;
	//Session* ses = Session::getInstance();
	for (int i = 0; i< DataStatus::getInstance()->getNumMetadataVariables(); i++){
		//Index by session variable num:
		int varnum = DataStatus::getInstance()->mapMetadataToSessionVarNum(i);
		if (variableListBox->isSelected(i)){
			pParams->setVariableSelected(varnum,true);
			
			if(firstVar == -1) firstVar = varnum;
			numSelected++;
		}
		else 
			pParams->setVariableSelected(varnum,false);
	}
	//If nothing is selected, select the first one:
	if (firstVar == -1) {
		firstVar = 0;
		numSelected = 1;
	}
	pParams->setNumVariablesSelected(numSelected);
	pParams->setFirstVarNum(firstVar);
	//reset the editing display range shown on the tab, 
	//this also sets dirty flag
	updateMapBounds(pParams);
	
	//Force a redraw of the transfer function frame
    setEditorDirty();
	   
	
	PanelCommand::captureEnd(cmd, pParams);
	//Need to update the selected point for the new variables
	updateTab();
	
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
void ProbeEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide probe X center");
	setXCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	
}
void ProbeEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide probe Y center");
	setYCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	
}
void ProbeEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide probe Z center");
	setZCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}
void ProbeEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide probe X size");
	setXSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	//setup the texture:
	resetTextureSize(pParams);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}
void ProbeEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide probe Y size");
	setYSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	resetTextureSize(pParams);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}
void ProbeEventRouter::
guiSetZSize(int sliderval){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide probe Z size");
	setZSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

}

void ProbeEventRouter::
guiSetNumRefinements(int n){
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	confirmText(false);
	int maxNumRefinements = 0;
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "set number Refinements for probe");
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
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
	
//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//Should not change values in params unless the text is invalid.
//
void ProbeEventRouter::
textToSlider(ProbeParams* pParams, int coord, float newCenter, float newSize){
	pParams->setProbeMin(coord, newCenter-0.5f*newSize);
	pParams->setProbeMax(coord, newCenter+0.5f*newSize);
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
		pParams->setProbeMin(coord, boxMin);
		pParams->setProbeMax(coord, boxMax);
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
			oldSliderSize = zSizeSlider->value();
			oldSliderCenter = zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				zSizeSlider->setValue(sliderSize);
			
			
			if (oldSliderCenter != sliderCenter)
				zCenterSlider->setValue(sliderCenter);
			if(centerChanged) zCenterEdit->setText(QString::number(newCenter,'g',7));
			lastZSizeSlider = sliderSize;
			lastZCenterSlider = sliderCenter;
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	if(centerChanged) {
		pParams->setProbeDirty();
		probeTextureFrame->update();
		VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	}
	update();
	return;
}
//Set text when a slider changes.
//
void ProbeEventRouter::
sliderToText(ProbeParams* pParams, int coord, int slideCenter, int slideSize){
	
	const float* extents = DataStatus::getInstance()->getExtents();
	float newCenter = extents[coord] + ((float)slideCenter)*(extents[coord+3]-extents[coord])/256.f;
	float newSize = maxBoxSize[coord]*(float)slideSize/256.f;
	pParams->setProbeMin(coord, newCenter-0.5f*newSize);
	pParams->setProbeMax(coord, newCenter+0.5f*newSize);
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
			zSizeEdit->setText(QString::number(newSize,'g',7));
			zCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedZLabel->setText(QString::number(selectedPoint[coord]));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	resetTextureSize(pParams);
	update();
	//force a new render with new Probe data
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	return;
	
}	
/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the TFE editor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void ProbeEventRouter::
updateMapBounds(RenderParams* params){
	ProbeParams* probeParams = (ProbeParams*)params;
	QString strn;
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	minDataBound->setText(strn.setNum(probeParams->getDataMinBound(currentTimeStep)));
	maxDataBound->setText(strn.setNum(probeParams->getDataMaxBound(currentTimeStep)));
	if (probeParams->getMapperFunc()){
		leftMappingBound->setText(strn.setNum(probeParams->getMapperFunc()->getMinColorMapValue(),'g',4));
		rightMappingBound->setText(strn.setNum(probeParams->getMapperFunc()->getMaxColorMapValue(),'g',4));
	} else {
		leftMappingBound->setText("0.0");
		rightMappingBound->setText("1.0");
	}
	
	probeParams->setProbeDirty();
	setDatarangeDirty(probeParams);
	setEditorDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(probeParams,ProbeTextureBit,true);
	
}

void ProbeEventRouter::setBindButtons(bool canbind)
{
  OpacityBindButton->setEnabled(canbind);
  ColorBindButton->setEnabled(canbind);
}

//Save undo/redo state when user grabs a probe handle, or maybe a probe face (later)
//
void ProbeEventRouter::
captureMouseDown(){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	savedCommand = PanelCommand::captureStart(pParams,  "slide probe handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshProbe(pParams);
}
//The Manip class will have already changed the box?..
void ProbeEventRouter::
captureMouseUp(){
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	//float boxMin[3],boxMax[3];
	//pParams->getBox(boxMin,boxMax);
	//probeTextureFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	resetTextureSize(pParams);
	pParams->setProbeDirty();
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getProbeParams(viznum)))
			updateTab();
	}
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	
}
//When the center slider moves, set the ProbeMin and ProbeMax
void ProbeEventRouter::
setXCenter(ProbeParams* pParams,int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(pParams,0, sliderval, xSizeSlider->value());
	pParams->setProbeDirty();
}
void ProbeEventRouter::
setYCenter(ProbeParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval, ySizeSlider->value());
	pParams->setProbeDirty();
}
void ProbeEventRouter::
setZCenter(ProbeParams* pParams,int sliderval){
	sliderToText(pParams,2, sliderval, zSizeSlider->value());
	pParams->setProbeDirty();
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void ProbeEventRouter::
setXSize(ProbeParams* pParams,int sliderval){
	sliderToText(pParams,0, xCenterSlider->value(),sliderval);
	pParams->setProbeDirty();
}
void ProbeEventRouter::
setYSize(ProbeParams* pParams,int sliderval){
	sliderToText(pParams,1, yCenterSlider->value(),sliderval);
	pParams->setProbeDirty();
}
void ProbeEventRouter::
setZSize(ProbeParams* pParams,int sliderval){
	sliderToText(pParams,2, zCenterSlider->value(),sliderval);
	pParams->setProbeDirty();
}

//Save undo/redo state when user clicks cursor
//
void ProbeEventRouter::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(VizWinMgr::getActiveProbeParams(),  "move probe cursor");
}
void ProbeEventRouter::
guiEndCursorMove(){
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
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
//Returns the OUT_OF_BOUNDS flag if point is not (in region and in probe).
//


float ProbeEventRouter::
calcCurrentValue(ProbeParams* pParams, const float point[3]){
	double regMin[3],regMax[3];
	if (numVariables <= 0) return OUT_OF_BOUNDS;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) return 0.f;
	if (!pParams->isEnabled()) return 0.f;
	int arrayCoord[3];
	
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	
	//Get the data dimensions (at current resolution):
	
	int numRefinements = pParams->getNumRefinements();
	
	//Find the region that contains the probe.

	//List the variables we are interested in
	int* sessionVarNums = new int[numVariables];
	int totVars = 0;
	for (int varnum = 0; varnum < ds->getNumSessionVariables(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		sessionVarNums[totVars++] = varnum;
	}
	
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	if (!rParams->getAvailableVoxelCoords(numRefinements, coordMin, coordMax, blkMin, blkMax, timeStep,
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
	//Now obtain all of the volumes needed for this probe:
	totVars = 0;
	size_t fullHeight = rParams->getFullGridHeight();
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for (int varnum = 0; varnum < ds->getNumSessionVariables(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		volData[totVars] = pParams->getContainingVolume(blkMin, blkMax, numRefinements, varnum, timeStep, fullHeight);
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
void ProbeEventRouter::
refreshHistogram(RenderParams* p){
	ProbeParams* pParams = (ProbeParams*)p;
	int firstVarNum = pParams->getFirstVarNum();
	const float* currentDatarange = pParams->getCurrentDatarange();
	if (!DataStatus::getInstance()->getDataMgr()) return;
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
	//create the smallest containing box
	size_t blkMin[3],blkMax[3];
	size_t boxMin[3],boxMax[3];
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	size_t fullHeight = VizWinMgr::getActiveRegionParams()->getFullGridHeight();
	pParams->getAvailableBoundingBox(timeStep, blkMin, blkMax, boxMin, boxMax, fullHeight);
	//Make sure this box will fit in current 
	//Check if the region/resolution is too big:
	int numRefinements = pParams->getNumRefinements();
	float boxExts[6];
	RegionParams::convertToBoxExtents(numRefinements,boxMin, boxMax,boxExts); 
	int numMBs = RegionParams::getMBStorageNeeded(boxExts, boxExts+3, numRefinements);
	//Check how many variables are needed:
	int varCount = 0;
	for (int varnum = 0; varnum < (int)DataStatus::getInstance()->getNumSessionVariables(); varnum++){
		if (pParams->variableIsSelected(varnum)) varCount++;
	}
	int cacheSize = DataStatus::getInstance()->getCacheMB();
	if (numMBs*varCount > (int)(cacheSize*0.75)){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current probe and resolution.\n%s \n%s",
			"Lower the refinement level, reduce the probe size, or increase the cache size.",
			"Rendering has been disabled.");
		pParams->setEnabled(false);
		updateTab();
		return;
	}
	int bSize =  *(DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize());
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then histogram rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	
	for (int varnum = 0; varnum < (int)DataStatus::getInstance()->getNumSessionVariables(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		assert(varnum >= firstVarNum);
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		volData[totVars] = pParams->getContainingVolume(blkMin, blkMax, numRefinements, varnum, timeStep, fullHeight);
		QApplication::restoreOverrideCursor();
		if (!volData[totVars]) return;
		totVars++;
	}
	
	//Get the data dimensions (at current resolution):
	size_t dataSize[3];
	float gridSpacing[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	
	for (int i = 0; i< 3; i++){
		dataSize[i] = DataStatus::getInstance()->getFullSizeAtLevel(numRefinements,i,fullHeight);
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
				//test if x,y,z is in probe:
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
void ProbeEventRouter::
makeCurrent(Params* prevParams, Params* nextParams, bool newWin, int instance) {

	assert(instance >= 0);
	ProbeParams* pParams = (ProbeParams*)(nextParams->deepCopy());
	int vizNum = pParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumProbeInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::ProbeParamsType, instance);
	setEditorDirty();
	updateTab();
	ProbeParams* formerParams = (ProbeParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled  changed:
	if (newWin || (formerParams->isEnabled() != pParams->isEnabled())){
		updateRenderer(pParams, wasEnabled,  newWin);
	}
	setDatarangeDirty(pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void ProbeEventRouter::
setDatarangeDirty(RenderParams* params)
{
	ProbeParams* pParams = (ProbeParams*)params;
	if (!pParams->getMapperFunc()) return;
	const float* currentDatarange = pParams->getCurrentDatarange();
	float minval = pParams->getMapperFunc()->getMinColorMapValue();
	float maxval = pParams->getMapperFunc()->getMaxColorMapValue();
	if (currentDatarange[0] != minval || currentDatarange[1] != maxval){
			pParams->setCurrentDatarange(minval, maxval);
			VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	}
	
}

void ProbeEventRouter::cleanParams(Params* p) 
{
  transferFunctionFrame->setMapperFunction(NULL);
  transferFunctionFrame->setVariableName("");
  transferFunctionFrame->update();
}
	
//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void ProbeEventRouter::captureImage() {
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
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory(fileInfo->dirPath(true).ascii());
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	size_t fullHeight = rParams->getFullGridHeight();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	//Make sure we have created a probe texture already...
	unsigned char* probeTex = pParams->getProbeTexture(timestep,fullHeight);
	if (!probeTex){
		MessageReporter::errorMsg("Image Capture Error;\nNo image to capture");
		return;
	}
	//Determine the image size.  Start with the texture dimensions in 
	// the scene, then increase x or y to make the aspect ratio match the
	// aspect ratio in the scene
	float aspRatio = pParams->getRealImageHeight()/pParams->getRealImageWidth();
	int wid = pParams->getImageWidth();
	int ht = pParams->getImageHeight();
	//Make sure the image is at least 300x300
	if (wid < 200) wid = 200;
	if (ht < 200) ht = 200;
	float imAspect = (float)ht/(float)wid;
	if (imAspect > aspRatio){
		//want ht/wid = asp, but ht/wid > asp; make wid larger:
		wid = (int)(0.5f+ (imAspect/aspRatio)*(float)wid);
	} else { //Make ht larger:
		ht = (int) (0.5f + (aspRatio/imAspect)*(float)ht);
	}
	//Construct the probe texture of the desired dimensions:
	probeTex = pParams->calcProbeTexture(timestep,wid,ht, fullHeight);
	//Construct an RGB image from this.  Ignore alpha.
	unsigned char* buf = new unsigned char[3*wid*ht];
	for (int i = 0; i< wid*ht; i++){
		for (int k = 0; k<3; k++){
			buf[(wid*ht-i-1)*3+k] = probeTex[4*i+k];
		}
	}
	delete probeTex;
	//Now open the jpeg file:
	FILE* jpegFile = fopen(filename.ascii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: Error opening output Jpeg file: %s",filename.ascii());
		return;
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = GLWindow::getJpegQuality();
	int rc = write_JPEG_file(jpegFile, wid, ht, buf, quality);
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; Error writing jpeg file %s",
			filename.ascii());
		delete buf;
		return;
	}
	delete buf;
	//Provide a message stating the capture in effect.
	MessageReporter::infoMsg("Image is captured to %s",
			filename.ascii());
}

void ProbeEventRouter::guiNudgeXSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	
	//ignore if change is not 1 
	if(abs(val - lastXSizeSlider) != 1) {
		lastXSizeSlider = val;
		return;
	}
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge probe X size");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 0, rParams->getFullGridHeight());
	float pmin = pParams->getProbeMin(0);
	float pmax = pParams->getProbeMax(0);
	float maxExtent = ds->getExtents()[3];
	float minExtent = ds->getExtents()[0];
	float newSize = pmax - pmin;
	if (val > lastXSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastXSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setProbeMin(0, pmin-voxelSize);
			pParams->setProbeMax(0, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastXSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setProbeMin(0, pmin+voxelSize);
			pParams->setProbeMax(0, pmax-voxelSize);
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
void ProbeEventRouter::guiNudgeXCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastXCenterSlider) != 1) {
		lastXCenterSlider = val;
		return;
	}
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge probe X center");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 0, rParams->getFullGridHeight());
	float pmin = pParams->getProbeMin(0);
	float pmax = pParams->getProbeMax(0);
	float maxExtent = ds->getExtents()[3];
	float minExtent = ds->getExtents()[0];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastXCenterSlider){//move by 1 voxel, but don't move past end
		lastXCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setProbeMin(0, pmin+voxelSize);
			pParams->setProbeMax(0, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastXCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setProbeMin(0, pmin-voxelSize);
			pParams->setProbeMax(0, pmax-voxelSize);
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
void ProbeEventRouter::guiNudgeYCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYCenterSlider) != 1) {
		lastYCenterSlider = val;
		return;
	}
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge probe Y center");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 1, rParams->getFullGridHeight());
	float pmin = pParams->getProbeMin(1);
	float pmax = pParams->getProbeMax(1);
	float maxExtent = ds->getExtents()[4];
	float minExtent = ds->getExtents()[1];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastYCenterSlider){//move by 1 voxel, but don't move past end
		lastYCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setProbeMin(1, pmin+voxelSize);
			pParams->setProbeMax(1, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastYCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setProbeMin(1, pmin-voxelSize);
			pParams->setProbeMax(1, pmax-voxelSize);
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
void ProbeEventRouter::guiNudgeZCenter(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastZCenterSlider) != 1) {
		lastZCenterSlider = val;
		return;
	}
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge probe Z center");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 2, rParams->getFullGridHeight());
	float pmin = pParams->getProbeMin(2);
	float pmax = pParams->getProbeMax(2);
	float maxExtent = ds->getExtents()[5];
	float minExtent = ds->getExtents()[2];
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastZCenterSlider){//move by 1 voxel, but don't move past end
		lastZCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setProbeMin(2, pmin+voxelSize);
			pParams->setProbeMax(2, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastZCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setProbeMin(2, pmin-voxelSize);
			pParams->setProbeMax(2, pmax-voxelSize);
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

void ProbeEventRouter::guiNudgeYSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYSizeSlider) != 1) {
		lastYSizeSlider = val;
		return;
	}
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge probe Y size");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 1, rParams->getFullGridHeight());
	float pmin = pParams->getProbeMin(1);
	float pmax = pParams->getProbeMax(1);
	float maxExtent = ds->getExtents()[4];
	float minExtent = ds->getExtents()[1];
	float newSize = pmax - pmin;
	if (val > lastYSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastYSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setProbeMin(1, pmin-voxelSize);
			pParams->setProbeMax(1, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastYSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setProbeMin(1, pmin+voxelSize);
			pParams->setProbeMax(1, pmax-voxelSize);
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
void ProbeEventRouter::guiNudgeZSize(int val) {
	if (notNudgingSliders) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastZSizeSlider) != 1) {
		lastZSizeSlider = val;
		return;
	}
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge probe Z size");
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->getNumRefinements(), 2, rParams->getFullGridHeight());
	float pmin = pParams->getProbeMin(2);
	float pmax = pParams->getProbeMax(2);
	float maxExtent = ds->getExtents()[5];
	float minExtent = ds->getExtents()[2];
	float newSize = pmax - pmin;
	if (val > lastZSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastZSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setProbeMin(2, pmin-voxelSize);
			pParams->setProbeMax(2, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastZSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setProbeMin(2, pmin+voxelSize);
			pParams->setProbeMax(2, pmax-voxelSize);
			newSize = newSize - 2.*voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*newSize/(maxExtent-minExtent) +0.5f);
	if(lastZSizeSlider != newSliderPos){
		lastZSizeSlider = newSliderPos;
		zSizeSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
}
void ProbeEventRouter::
adjustBoxSize(ProbeParams* pParams){
	//Determine the max x, y, z sizes of probe:
	
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
	
	
	//Now make sure the probe box fits
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
	zSizeEdit->setText(QString::number(boxmax[2]-boxmin[2]));
	xSizeSlider->setValue((int)(256.f*(boxmax[0]-boxmin[0])/(maxBoxSize[0])));
	ySizeSlider->setValue((int)(256.f*(boxmax[1]-boxmin[1])/(maxBoxSize[1])));
	zSizeSlider->setValue((int)(256.f*(boxmax[2]-boxmin[2])/(maxBoxSize[2])));
	
	//Cancel any response to text events generated in this method:
	//
	guiSetTextChanged(false);
}
void ProbeEventRouter::resetTextureSize(ProbeParams* probeParams){
	//setup the texture:
	size_t fullHeight = VizWinMgr::getActiveRegionParams()->getFullGridHeight();
	float voxDims[2];
	probeParams->getProbeVoxelExtents(fullHeight, voxDims);
	probeTextureFrame->setTextureSize(voxDims[0],voxDims[1]);
}
