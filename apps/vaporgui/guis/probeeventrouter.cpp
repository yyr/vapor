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

#include "regionparams.h"
#include "regiontab.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "probeframe.h"
#include "floweventrouter.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "probetab.h"
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
#include "tfframe.h"
#include "tfeditor.h"
#include "VolumeRenderer.h"

using namespace VAPoR;


ProbeEventRouter::ProbeEventRouter(QWidget* parent,const char* name): ProbeTab(parent, name), EventRouter(){
	myParamsType = Params::ProbeParamsType;
	savedCommand = 0;
	ignoreListboxChanges = false;
	numVariables = 0;
	seedAttached = false;

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
	connect (xCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (yCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (zCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (thetaEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (phiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
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
	connect (instanceCombo, SIGNAL(activated(int)), this, SLOT(guiChangeInstance(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (copyInstanceButton, SIGNAL(clicked()), this, SLOT(guiCopyInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (histoScaleEdit, SIGNAL(returnPressed()), this, SLOT(probeReturnPressed()));
	connect (regionCenterButton, SIGNAL(clicked()), this, SLOT(probeCenterRegion()));
	connect (viewCenterButton, SIGNAL(clicked()), this, SLOT(probeCenterView()));
	connect (rakeCenterButton, SIGNAL(clicked()), this, SLOT(probeCenterRake()));
	connect (probeCenterButton, SIGNAL(clicked()), this, SLOT(guiCenterProbe()));
	connect (addSeedButton, SIGNAL(clicked()), this, SLOT(probeAddSeed()));
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
	connect (saveButton, SIGNAL(clicked()), this, SLOT(probeSaveTF()));
	connect (EnableDisable, SIGNAL(activated(int)), this, SLOT(setProbeEnabled(int)));
	

	connect (leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));
	connect (rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setProbeTabTextChanged(const QString&)));

	connect (opacityScaleSlider, SIGNAL(sliderReleased()), this, SLOT (probeOpacityScale()));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setProbeNavigateMode(bool)));
	
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setProbeEditMode(bool)));
	
	connect(alignButton, SIGNAL(clicked()), this, SLOT(guiSetAligned()));
	
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshProbeHisto()));
	
	
}

/*********************************************************************************
 * Slots associated with ProbeTab:
 *********************************************************************************/
void ProbeEventRouter::guiChangeInstance(int){}
void ProbeEventRouter::guiNewInstance(){}
void ProbeEventRouter::guiDeleteInstance(){}
void ProbeEventRouter::guiCopyInstance(){}
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
setProbeEnabled(int val){

	//If there's no window, or no datamgr, ignore this.
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	if ((activeViz < 0) || Session::getInstance()->getDataMgr() == 0) {
		EnableDisable->setCurrentItem(0);
		return;
	}
	ProbeParams* pParams = vizMgr->getProbeParams(activeViz);
	//Make sure this is a change:
	if (pParams->isEnabled() == (val==1) ) return;
	guiSetEnabled(val==1);
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
	ProbeTFFrame->update();
}
/*
 * Respond to a slider release
 */
void ProbeEventRouter::
probeOpacityScale() {
	guiSetOpacityScale(opacityScaleSlider->value());
}

void ProbeEventRouter::
probeLoadTF(void){
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(Params::ProbeParamsType);
	
	//If there are no TF's currently in Session, just launch file load dialog.
	if (Session::getInstance()->getNumTFs() > 0){
		LoadTFDialog* loadTFDialog = new LoadTFDialog(pParams,this,
			"Load TF Dialog", true);
		int rc = loadTFDialog->exec();
		if (rc == 0) return;
		if (rc == 1) fileLoadTF(pParams);
		//if rc == 2, we already (probably) loaded a tf from the session
	} else {
		fileLoadTF(pParams);
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
	fRouter->guiAddSeed(pt);
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
	
	probeParams->setTheta(thetaEdit->text().toFloat());
	probeParams->setPhi(phiEdit->text().toFloat());
	probeParams->setHistoStretch(histoScaleEdit->text().toFloat());

	//Get slider positions from text boxes:
		
	float boxCtr = xCenterEdit->text().toFloat();
	float boxSize[3];
	boxSize[0] = xSizeEdit->text().toFloat();
	probeParams->setProbeMin(0,boxCtr-0.5*boxSize[0]);
	probeParams->setProbeMax(0,boxCtr+0.5*boxSize[0]);
	
	textToSlider(probeParams,0, boxCtr, boxSize[0]);
	boxCtr = yCenterEdit->text().toFloat();
	boxSize[1] = ySizeEdit->text().toFloat();
	probeParams->setProbeMin(1,boxCtr-0.5*boxSize[1]);
	probeParams->setProbeMax(1,boxCtr+0.5*boxSize[1]);
	textToSlider(probeParams,1, boxCtr, boxSize[0]);
	boxCtr = zCenterEdit->text().toFloat();
	boxSize[2] = zSizeEdit->text().toFloat();
	probeParams->setProbeMin(2,boxCtr-0.5*boxSize[2]);
	probeParams->setProbeMax(2,boxCtr+0.5*boxSize[2]);
	textToSlider(probeParams,2, boxCtr, boxSize[2]);
	probeTextureFrame->setTextureSize(boxSize[0],boxSize[1]);
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
sessionLoadTF(ProbeParams* dParams, QString* name){
	
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->ascii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getVarNum();
	dParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	setDatarangeDirty(dParams);
	setEditorDirty();
}
void ProbeEventRouter::
fileLoadTF(ProbeParams* dParams){
	
	int varNum = dParams->getVarNum();
	//Open a file load dialog
	
    QString s = QFileDialog::getOpenFileName(
                    Session::getInstance()->getTFFilePath().c_str(),
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
	
	TransferFunction* t = TransferFunction::loadFromFile(is);
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
	Session::getInstance()->updateTFFilePath(&s);
	setDatarangeDirty(dParams);
	setEditorDirty();
}

//Insert values from params into tab panel
//
void ProbeEventRouter::updateTab(Params* params){
	ProbeParams* probeParams = (ProbeParams*) params;
	
	
	QString strn;
	Session* ses = Session::getInstance();
	ses->blockRecording();
	ProbeTFFrame->setEditor(getTFEditor(probeParams));
	EnableDisable->setCurrentItem((probeParams->isEnabled()) ? 1 : 0);
	refinementCombo->setCurrentItem(probeParams->getNumRefinements());
	histoScaleEdit->setText(QString::number(probeParams->getHistoStretch()));

	//Set the values of box extents:
	float probeMin[3],probeMax[3];
	probeParams->getBox(probeMin,probeMax);
	for (int i = 0; i<3; i++){
		textToSlider(probeParams, i, (probeMin[i]+probeMax[i])*0.5f,
			probeMax[i]-probeMin[i]);
	}
	xSizeEdit->setText(QString::number(probeMax[0]-probeMin[0],'g', 4));
	xCenterEdit->setText(QString::number(0.5f*(probeMax[0]+probeMin[0]),'g',5));
	ySizeEdit->setText(QString::number(probeMax[1]-probeMin[1],'g', 4));
	yCenterEdit->setText(QString::number(0.5f*(probeMax[1]+probeMin[1]),'g',5));
	zSizeEdit->setText(QString::number(probeMax[2]-probeMin[2],'g', 4));
	zCenterEdit->setText(QString::number(0.5f*(probeMax[2]+probeMin[2]),'g',5));
	thetaEdit->setText(QString::number(probeParams->getTheta()));
	phiEdit->setText(QString::number(probeParams->getPhi()));
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
		if (variableListBox->isSelected(i) != probeParams->variableIsSelected(ses->mapMetadataToRealVarNum(i)))
			variableListBox->setSelected(i, probeParams->variableIsSelected(ses->mapMetadataToRealVarNum(i)));
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
	ProbeTFFrame->setEditor(probeParams->getTFEditor());
	ProbeTFFrame->update();
	update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
}
//Make region match probe.  Responds to button in region panel
void ProbeEventRouter::
guiCopyRegionToProbe(){
	confirmText(false);
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "copy region to probe");
	
	for (int i = 0; i< 3; i++){
		pParams->setProbeMin(i, rParams->getRegionMin(i));
		pParams->setProbeMax(i, rParams->getRegionMax(i));
	}
	//Note:  the probe may not fit in the region.  
	updateTab(pParams);
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
	
	numVariables = DataStatus::getInstance()->getNumVariables();
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
		
	getTFEditor(dParams)->setMinEditValue(dParams->getMinColorMapBound());
	getTFEditor(dParams)->setMaxEditValue(dParams->getMaxColorMapBound());
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
updateRenderer(ProbeParams* pParams, bool prevEnabled,   bool newWindow){
	
	if (newWindow) {
		prevEnabled = false;	
	}
	
	bool nowEnabled = pParams->isEnabled();
	
	if (prevEnabled == nowEnabled ) return;
	setDatarangeDirty(pParams);
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);

	return;
}
void ProbeEventRouter::
setEditorDirty(){
	ProbeParams* dp = VizWinMgr::getInstance()->getActiveProbeParams();
	ProbeTFFrame->setEditor(getTFEditor(dp));
	ProbeTFFrame->update();
}


void ProbeEventRouter::
guiSetEnabled(bool value){
	ProbeParams* pParams = VizWinMgr::getInstance()->getActiveProbeParams();
	confirmText(false);
	assert(value != pParams->isEnabled());
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "toggle probe enabled");
	pParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, pParams);
	//Need to rerender the texture:
	pParams->setProbeDirty(true);
	//and refresh the gui
	updateTab(pParams);
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
	if (!pp->getTFEditor()) return;
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
guiStartChangeMapFcn(char* str){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	ProbeParams* pp = VizWinMgr::getInstance()->getActiveProbeParams();
	savedCommand = PanelCommand::captureStart(pp, str);
}
void ProbeEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand::captureEnd(savedCommand,pParams);
	savedCommand = 0;
	pParams->setProbeDirty();
	setEditorDirty();
	setDatarangeDirty(pParams);
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
}

void ProbeEventRouter::
guiBindColorToOpac(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Color to Opacity");
	pParams->getTFEditor()->bindColorToOpac();
	PanelCommand::captureEnd(cmd, pParams);
	setEditorDirty();
}
void ProbeEventRouter::
guiBindOpacToColor(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "bind Opacity to Color");
	pParams->getTFEditor()->bindOpacToColor();
	PanelCommand::captureEnd(cmd, pParams);
	setEditorDirty();
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
	updateTab(pParams);
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
		int varnum = DataStatus::getInstance()->mapMetadataToRealVarNum(i);
		if (variableListBox->isSelected(i)){
			pParams->setVariableSelected(i,true);
			
			if(firstVar == -1) firstVar = varnum;
			numSelected++;
		}
		else 
			pParams->setVariableSelected(i,false);
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
	
	//Force a redraw of tfframe 
	if (pParams->getTFEditor()) {
		
		pParams->connectMapperFunction(pParams->getMapperFunc(),pParams->getTFEditor());
		setEditorDirty();
	}
		
	
	PanelCommand::captureEnd(cmd, pParams);
	//Need to update the selected point for the new variables
	updateTab(pParams);
	
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
	float probeMin[3],probeMax[3];
	pParams->getBox(probeMin,probeMax);
	PanelCommand::captureEnd(cmd, pParams);
	probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
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
	float probeMin[3],probeMax[3];
	pParams->getBox(probeMin,probeMax);
	PanelCommand::captureEnd(cmd, pParams);
	probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
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
	float probeMin[3],probeMax[3];
	pParams->getBox(probeMin,probeMax);
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
//
void ProbeEventRouter::
textToSlider(ProbeParams* pParams, int coord, float newCenter, float newSize){
	//force the new center to fit in the full domain,
	
	bool centerChanged = false;
	DataStatus* ds = DataStatus::getInstance();
	const float* extents; 
	float regMin = 0.f;
	float regMax = 1.f;
	float boxMin,boxMax;
	if (ds){
		extents = DataStatus::getInstance()->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	}
	if (newCenter < regMin) {
		newCenter = regMin;
		centerChanged = true;
	}
	if (newCenter > regMax) {
		newCenter = regMax;
		centerChanged = true;
	}
	
	boxMin = newCenter - newSize*0.5f; 
	boxMax= newCenter + newSize*0.5f; 
	pParams->setProbeMin(coord, boxMin);
	pParams->setProbeMax(coord, boxMax);
	
	int sliderSize = (int)(0.5f+ 256.f*newSize/(regMax - regMin));
	int sliderCenter = (int)(0.5f+ 256.f*(newCenter - regMin)/(regMax - regMin));
	int oldSliderSize, oldSliderCenter;
	switch(coord) {
		case 0:
			oldSliderSize = xSizeSlider->value();
			oldSliderCenter = xCenterSlider->value();
			if (oldSliderSize != sliderSize)
				xSizeSlider->setValue(sliderSize);
			
			if (oldSliderCenter != sliderCenter)
				xCenterSlider->setValue(sliderCenter);
			if(centerChanged) xCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 1:
			oldSliderSize = ySizeSlider->value();
			oldSliderCenter = yCenterSlider->value();
			if (oldSliderSize != sliderSize)
				ySizeSlider->setValue(sliderSize);
			
			if (oldSliderCenter != sliderCenter)
				yCenterSlider->setValue(sliderCenter);
			if(centerChanged) yCenterEdit->setText(QString::number(newCenter));
			
			break;
		case 2:
			oldSliderSize = zSizeSlider->value();
			oldSliderCenter = zCenterSlider->value();
			if (oldSliderSize != sliderSize)
				zSizeSlider->setValue(sliderSize);
			
			
			if (oldSliderCenter != sliderCenter)
				zCenterSlider->setValue(sliderCenter);
			if(centerChanged) zCenterEdit->setText(QString::number(newCenter));
			
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	pParams->setProbeDirty();
	probeTextureFrame->update();
	VizWinMgr::getInstance()->setVizDirty(pParams,ProbeTextureBit,true);
	update();
	return;
}
//Set text when a slider changes.
//
void ProbeEventRouter::
sliderToText(ProbeParams* pParams, int coord, int slideCenter, int slideSize){
	
	//force the center to fit in the region.
	DataStatus* ds = DataStatus::getInstance();
	const float* extents; 
	float regMin, regMax;
	if (ds){
		extents = DataStatus::getInstance()->getExtents();
		regMin = extents[coord];
		regMax = extents[coord+3];
	} else {
		regMin = 0.f;
		regMax = 1.f;
	}
	float newSize = slideSize*(regMax-regMin)/256.f;
	float newCenter = regMin+ slideCenter*(regMax-regMin)/256.f;
	
	
	pParams->setProbeMin(coord, newCenter - newSize*0.5f);
	pParams->setProbeMax(coord, newCenter + newSize*0.5f);
	const float* selectedPoint = pParams->getSelectedPoint();
	
	switch(coord) {
		case 0:
			xSizeEdit->setText(QString::number(newSize));
			xCenterEdit->setText(QString::number(newCenter));
			selectedXLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 1:
			ySizeEdit->setText(QString::number(newSize));
			yCenterEdit->setText(QString::number(newCenter));
			selectedYLabel->setText(QString::number(selectedPoint[coord]));
			break;
		case 2:
			zSizeEdit->setText(QString::number(newSize));
			zCenterEdit->setText(QString::number(newCenter));
			selectedZLabel->setText(QString::number(selectedPoint[coord]));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	float probeMin[3],probeMax[3];
	pParams->getBox(probeMin,probeMax);
	probeTextureFrame->setTextureSize(probeMax[0]-probeMin[0],probeMax[1]-probeMin[1]);
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
void ProbeEventRouter::
setBindButtons(ProbeParams* pParams){
	bool enable = false;
	if (pParams->getTFEditor())
		enable = pParams->getTFEditor()->canBind();
	OpacityBindButton->setEnabled(enable);
	ColorBindButton->setEnabled(enable);
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
	float boxMin[3],boxMax[3];
	pParams->getBox(boxMin,boxMax);
	probeTextureFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	pParams->setProbeDirty();
	//Update the tab if it's in front:
	if(MainForm::getInstance()->getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getProbeParams(viznum)))
			updateTab(pParams);
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
float* ProbeEventRouter::
getContainingVolume(ProbeParams* pParams, size_t blkMin[3], size_t blkMax[3], int varNum, int timeStep){
	//Get the region (int coords) associated with the specified variable at the
	//current probe extents
	int numRefinements = pParams->getNumRefinements();
	int maxRes = DataStatus::getInstance()->maxXFormPresent(varNum,timeStep);
	if (maxRes < numRefinements){
		MessageReporter::warningMsg("Probe data unavailable for refinement level %d at timestep %d",
			numRefinements, timeStep);
		return 0;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//	qWarning("Fetching region: %d %d %d , %d %d %d ",blkMin[0],blkMin[1],blkMin[2],
	//	blkMax[0],blkMax[1],blkMax[2]);
	float* reg = ((DataMgr*)(DataStatus::getInstance()->getDataMgr()))->GetRegion((size_t)timeStep,
		DataStatus::getInstance()->getVariableName(varNum).c_str(),
		numRefinements, blkMin, blkMax, 0);
	QApplication::restoreOverrideCursor();
	return reg;
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
	updateTab(pParams);
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	setDatarangeDirty(pParams);
}
//calculate the variable, or rms of the variables, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not (in region and in probe).
//In order to exploit the datamgr cache, always obtain the data from the same
//regions that are accessed in building the probe texture.
//


float ProbeEventRouter::
calcCurrentValue(ProbeParams* pParams, const float point[3]){
	if (numVariables <= 0) return OUT_OF_BOUNDS;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) return 0.f;
	if (!pParams->isEnabled()) return 0.f;
	int arrayCoord[3];
	const float* extents = ds->getExtents();

	//Get the data dimensions (at current resolution):
	int dataSize[3];
	const size_t totTransforms = ds->getCurrentMetadata()->GetNumTransforms();
	int numRefinements = pParams->getNumRefinements();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (ds->getFullDataSize(i))>>(totTransforms - numRefinements);
	}
	
	//Find the region that contains the probe.
	
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	if (!pParams->getAvailableBoundingBox(timeStep, blkMin, blkMax, coordMin, coordMax)) return OUT_OF_BOUNDS;
	for (int i = 0; i< 3; i++){
		if ((point[i] < extents[i]) || (point[i] > extents[i+3])) return OUT_OF_BOUNDS;
		arrayCoord[i] = (int) (0.5f+((float)dataSize[i])*(point[i] - extents[i])/(extents[i+3]-extents[i]));
		//Make sure the transformed coords are in the probe region
		if (arrayCoord[i] < (int)coordMin[i] || arrayCoord[i] > (int)coordMax[i] ) {
			return OUT_OF_BOUNDS;
		} 
	}
	
	int bSize =  *(ds->getCurrentMetadata()->GetBlockSize());
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (int varnum = 0; varnum < ds->getNumVariables(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		volData[totVars] = getContainingVolume(pParams, blkMin, blkMax, varnum, timeStep);
		totVars++;
	}
			
	int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize) +
		(arrayCoord[1] - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
		(arrayCoord[2] - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
	
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
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
Histo* ProbeEventRouter::getHistogram(RenderParams* p, bool mustGet){
	ProbeParams* pParams = (ProbeParams*)p;
	if (!histogramList){
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	int firstVarNum = pParams->getFirstVarNum();
	const float* currentDatarange = pParams->getCurrentDatarange();
	if (histogramList[firstVarNum]) return histogramList[firstVarNum];
	
	if (!mustGet) return 0;
	histogramList[firstVarNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	refreshHistogram(pParams);
	return histogramList[firstVarNum];
	
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
	
	pParams->getAvailableBoundingBox(timeStep, blkMin, blkMax, boxMin, boxMax);

	int bSize =  *(DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize());
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then histogram rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (int varnum = 0; varnum < (int)DataStatus::getInstance()->getNumVariables(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		assert(varnum >= firstVarNum);
		volData[totVars] = getContainingVolume(pParams, blkMin, blkMax, varnum, timeStep);
		if (!volData[totVars]) return;
		totVars++;
	}

	//Get the data dimensions (at current resolution):
	size_t dataSize[3];
	float gridSpacing[3];
	const size_t totTransforms = DataStatus::getInstance()->getCurrentMetadata()->GetNumTransforms();
	const float* extents = DataStatus::getInstance()->getExtents();
	int numRefinements = pParams->getNumRefinements();
	for (int i = 0; i< 3; i++){
		dataSize[i] = (DataStatus::getInstance()->getFullDataSize(i))>>(totTransforms - numRefinements);
		gridSpacing[i] = (extents[i+3]-extents[i])/(float)(dataSize[i]-1);
		if (boxMin[i]< 0) boxMin[i] = 0;
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
		for (size_t j = boxMin[1]; j <= boxMax[1]; j++){
			xyz[1] = extents[1] + (((float)j)/(float)(dataSize[1]-1))*(extents[4]-extents[1]);
			for (size_t i = boxMin[0]; i <= boxMax[0]; i++){
				xyz[0] = extents[0] + (((float)i)/(float)(dataSize[0]-1))*(extents[3]-extents[0]);
				
				//test if x,y,z is in probe:
				if (pParams->distanceToCube(xyz, normals, corner)<=0.5f*voxSize){
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
	setEditorDirty();
	update();
}
//Calculate the probe texture (if it needs refreshing).
//It's kept (saved) in the probe params --togo! 
unsigned char* ProbeEventRouter::
getProbeTexture(ProbeParams* pParams){
	if (!pParams->isEnabled()) return 0;
	if (!pParams->probeIsDirty())
		return pParams->getCurrentProbeTexture();
	
	if (!DataStatus::getInstance()->getDataMgr()) return 0;
	unsigned char* probeTexture = new unsigned char[128*128*4];
	
	float transformMatrix[12];
	
	//Set up to transform from probe into volume:
	pParams->buildCoordTransform(transformMatrix);
	int numRefinements = pParams->getNumRefinements();
	//Get the data dimensions (at this resolution):
	int dataSize[3];
	const size_t totTransforms = DataStatus::getInstance()->getCurrentMetadata()->GetNumTransforms();
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (DataStatus::getInstance()->getFullDataSize(i))>>(totTransforms - numRefinements);
	}
	//Determine the integer extents of the containing cube, truncate to
	//valid integer coords:

	
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	
	pParams->getAvailableBoundingBox(timeStep,blkMin, blkMax, coordMin, coordMax);
	int bSize =  *(DataStatus::getInstance()->getCurrentMetadata()->GetBlockSize());
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** volData = new float*[numVariables];
	//Now obtain all of the volumes needed for this probe:
	int totVars = 0;
	for (int varnum = 0; varnum < DataStatus::getInstance()->getNumVariables(); varnum++){
		if (!pParams->variableIsSelected(varnum)) continue;
		volData[totVars] = getContainingVolume(pParams, blkMin, blkMax, varnum, timeStep);
		if (!volData[totVars]) {
			delete probeTexture;
			return 0;
		}
		totVars++;
	}
	//Now calculate the texture.
	//
	//For each pixel, map it into the volume.
	//The blkMin values tell you the offset to use.
	//The blkMax values tell how to stride through the data
	//The coordMin and coordMax values are used to check validity
	//We first map the coords in the probe to the volume.  
	//Then we map the volume into the region provided by dataMgr
	//This is done for each of the variables,
	//The RMS of the result is then mapped using the transfer function.
	float clut[256*4];
	TransferFunction* transFunc = pParams->getTransFunc();
	assert(transFunc);
	transFunc->makeLut(clut);
	
	float probeCoord[3];
	float dataCoord[3];
	size_t arrayCoord[3];
	const float* extents = DataStatus::getInstance()->getExtents();
	//Can ignore depth, just mapping center plane
	probeCoord[2] = 0.f;
	
	//Loop over pixels in texture
	for (int iy = 0; iy < 128; iy++){
		//Map iy to a value between -1 and 1
		probeCoord[1] = -1.f + 2.f*(float)iy/127.f;
		for (int ix = 0; ix < 128; ix++){
			
			//find the coords that the texture maps to
			//probeCoord is the coord in the probe, dataCoord is in data volume 
			probeCoord[0] = -1.f + 2.f*(float)ix/127.f;
			vtransform(probeCoord, transformMatrix, dataCoord);
			for (int i = 0; i<3; i++){
				arrayCoord[i] = (size_t) (0.5f+((float)dataSize[i])*(dataCoord[i] - extents[i])/(extents[i+3]-extents[i]));
			}
			
			//Make sure the transformed coords are in the probe region
			//This should only fail if they aren't even in the full volume.
			bool dataOK = true;
			for (int i = 0; i< 3; i++){
				if (arrayCoord[i] < coordMin[i] ||
					arrayCoord[i] > coordMax[i] ) {
						dataOK = false;
					} //outside; color it black
			}
			
			if(dataOK) { //find the coordinate in the data array
				int xyzCoord = (arrayCoord[0] - blkMin[0]*bSize) +
					(arrayCoord[1] - blkMin[1]*bSize)*(bSize*(blkMax[0]-blkMin[0]+1)) +
					(arrayCoord[2] - blkMin[2]*bSize)*(bSize*(blkMax[1]-blkMin[1]+1))*(bSize*(blkMax[0]-blkMin[0]+1));
				float varVal;
				

				//use the intDataCoords to index into the loaded data
				if (totVars == 1) varVal = volData[0][xyzCoord];
				else { //Add up the squares of the variables
					varVal = 0.f;
					for (int k = 0; k<totVars; k++){
						varVal += volData[k][xyzCoord]*volData[k][xyzCoord];
					}
					varVal = sqrt(varVal);
				}
				
				//Use the transfer function to map the data:
				int lutIndex = transFunc->mapFloatToIndex(varVal);
				
				probeTexture[4*(ix+128*iy)] = (unsigned char)(0.5+ clut[4*lutIndex]*255.f);
				probeTexture[4*(ix+128*iy)+1] = (unsigned char)(0.5+ clut[4*lutIndex+1]*255.f);
				probeTexture[4*(ix+128*iy)+2] = (unsigned char)(0.5+ clut[4*lutIndex+2]*255.f);
				probeTexture[4*(ix+128*iy)+3] = (unsigned char)(0.5+ clut[4*lutIndex+3]*255.f);
				
			}
			else {//point out of region
				probeTexture[4*(ix+128*iy)] = 0;
				probeTexture[4*(ix+128*iy)+1] = 0;
				probeTexture[4*(ix+128*iy)+2] = 0;
				probeTexture[4*(ix+128*iy)+3] = 0;
			}

		}//End loop over ix
	}//End loop over iy
	
	pParams->setProbeDirty(false);
	pParams->setProbeTexture(probeTexture);
	return probeTexture;
}
	
//Method called when undo/redo changes params.  It does the following:
//  puts the new params into the vizwinmgr, deletes the old one
//  Updates the tab if it's the current instance
//  Calls updateRenderer to rebuild renderer 
//	Makes the vizwin update.
void ProbeEventRouter::
makeCurrent(Params* prevParams, Params* nextParams, bool newWin, int instance) {

	assert(instance >= 0);
	ProbeParams* pParams = (ProbeParams*)nextParams;
	int vizNum = pParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumProbeInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::ProbeParamsType, instance);

	if( VizWinMgr::getInstance()->getCurrentProbeInstIndex(vizNum) == instance) updateTab(pParams);
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
			ProbeTFFrame->update();
	}
	
}

void ProbeEventRouter::cleanParams(Params* p) {
		if(ProbeTFFrame->getEditor() && ProbeTFFrame->getEditor()->getParams() == p)
			ProbeTFFrame->setEditor(0);
}
	