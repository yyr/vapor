//************************************************************************
//																		*
//		     Copyright (C)  2007										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isoeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2007
//
//	Description:	Implements the IsoEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the iso tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <qapplication.h>
#include <qcursor.h>
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qworkspace.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qlistbox.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include "instancetable.h"
#include "MappingFrame.h"
#include "regionparams.h"
#include "regiontab.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "isorenderer.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include "isotab.h"

#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"
#include "ParamsIso.h"
#include "isoeventrouter.h"
#include "animationeventrouter.h"
#include "eventrouter.h"

#include "vapor/errorcodes.h"

using namespace VAPoR;


IsoEventRouter::IsoEventRouter(QWidget* parent,const char* name): IsoTab(parent, name), EventRouter(){
	myParamsType = Params::IsoParamsType;
	savedCommand = 0;  
	renderTextChanged = false;
	isoSelectionFrame->setOpacityMapping(true);
	isoSelectionFrame->setColorMapping(false);
	isoSelectionFrame->setIsoSlider(true);
}


IsoEventRouter::~IsoEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Isotab is created it must be hooked up here
 ************************************************************/
void
IsoEventRouter::hookUpTab()
{
	
	
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (variableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetComboVarNum(int) ) );
	connect (numBitsCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumBits(int)));
	connect (lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( guiSetLighting(bool) ) );
 
	//Line edits:
	connect (histoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL(returnPressed() ), this, SLOT( isoReturnPressed()));
	connect (isoValueEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (isoValueEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (leftHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (leftHistoEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (rightHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (rightHistoEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (selectedXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (selectedXEdit, SIGNAL( returnPressed()), this, SLOT( isoReturnPressed()));
	connect (selectedYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (selectedYEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (selectedZEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (selectedZEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (constantOpacityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (constantOpacityEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshHisto()));
	connect(editButton, SIGNAL(toggled(bool)), this, SLOT(setIsoEditMode(bool)));
	connect(navigateButton, SIGNAL(toggled(bool)), this, SLOT(setIsoNavigateMode(bool)));
	connect (constantColorButton, SIGNAL(clicked()), this, SLOT(setConstantColor()));
	connect (passThruButton, SIGNAL(clicked()), this, SLOT(guiPassThruPoint()));
	connect (copyPointButton, SIGNAL(clicked()), this, SLOT(guiCopyProbePoint()));
	
	//Instance table stuff:
	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setIsoEnabled(bool,int)));
	
	// isoSelectionFrame controls:
	connect(editButton, SIGNAL(toggled(bool)), 
            isoSelectionFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()),
            isoSelectionFrame, SLOT(fitToView()));
    connect(isoSelectionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeIsoSelection(QString)));
    connect(isoSelectionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeIsoSelection()));

}
//Insert values from params into tab panel
//
void IsoEventRouter::updateTab(){
	Session *session = Session::getInstance();
	session->blockRecording();

	if (DataStatus::getInstance()->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);

	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
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


	ParamsIso* isoParams = (ParamsIso*) VizWinMgr::getActiveIsoParams();
	deleteInstanceButton->setEnabled(vizMgr->getNumIsoInstances(winnum) > 1);

	QString strn;
    
	//Force the iso to refresh
	numBitsCombo->setEnabled(!isoParams->isEnabled());
	numBitsCombo->setCurrentItem((isoParams->GetNumBits())>>4);
	int numRefs = isoParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentItem(numRefs);

	int comboVarNum = DataStatus::getInstance()->getMetadataVarNum(
		isoParams->GetVariableName());
	variableCombo->setCurrentItem(comboVarNum);
	lightingCheckbox->setChecked(isoParams->GetNormalOnOff());
	histoScaleEdit->setText(QString::number(isoParams->GetHistoStretch()));
	isoValueEdit->setText(QString::number(isoParams->GetIsoValue()));
	const vector<double>& coords = isoParams->GetSelectedPoint();
	const float* bnds = isoParams->GetHistoBounds();
	const float* clr = isoParams->GetConstantColor();
	leftHistoEdit->setText(QString::number(bnds[0]));
	rightHistoEdit->setText(QString::number(bnds[1]));
	constantOpacityEdit->setText(QString::number(clr[3]));
	selectedXEdit->setText(QString::number(coords[0]));
	selectedYEdit->setText(QString::number(coords[1]));
	selectedZEdit->setText(QString::number(coords[2]));
	constantColorButton->setPaletteBackgroundColor(QColor((int)(.5+clr[0]*255.),(int)(.5+clr[1]*255.),(int)(.5+clr[2]*255.)));
	
	assert(isoParams->getMapperFunc()->getParams() == isoParams);
	
	isoSelectionFrame->setMapperFunction(isoParams->getMapperFunc());
	
    isoSelectionFrame->setVariableName(isoParams->GetVariableName());
	updateMapBounds(isoParams);

	isoSelectionFrame->update();

	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		variableValueLabel->setText(QString::number(val));
	else variableValueLabel->setText("");
   
	update();
	guiSetTextChanged(false);
	renderTextChanged = false;
	Session::getInstance()->unblockRecording();
	vizMgr->getTabManager()->update();
}
//Update the params based on changes in text boxes
void IsoEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "edit Iso text");
	QString strn;
	
	iParams->SetHistoStretch(histoScaleEdit->text().toFloat());
	float opac = constantOpacityEdit->text().toFloat();
	const float *clr = iParams->GetConstantColor();
	
	if (clr[3] != opac){
		float newColor[4];
		newColor[3] = opac;
		for (int i = 0; i< 3; i++) newColor[i] = clr[i];
		iParams->SetConstantColor(newColor);
	}
	float bnds[2];
	const float* oldBnds = iParams->GetHistoBounds();
	bnds[0] = leftHistoEdit->text().toFloat();
	bnds[1] = rightHistoEdit->text().toFloat();
	if (bnds[0] != oldBnds[0] || bnds[1] != oldBnds[1]){
		iParams->SetHistoBounds(bnds);
		setDatarangeDirty(iParams);
	}

	if (iParams->getMapperFunc()) {
		(iParams->getMapperFunc())->setMinOpacMapValue(bnds[0]);
		(iParams->getMapperFunc())->setMaxOpacMapValue(bnds[1]);
	}
	float coords[3];
	coords[0] = selectedXEdit->text().toFloat();
	coords[1] = selectedYEdit->text().toFloat();
	coords[2] = selectedZEdit->text().toFloat();
	iParams->SetSelectedPoint(coords);
	iParams->SetIsoValue(isoValueEdit->text().toFloat());
	
	guiSetTextChanged(false);
	setEditorDirty(iParams);
	
	PanelCommand::captureEnd(cmd, iParams);
	if (renderTextChanged)
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
		
	renderTextChanged = false;
}
/*********************************************************************************
 * Slots associated with IsoTab:
 *********************************************************************************/
void IsoEventRouter::
guiStartChangeIsoSelection(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	ParamsIso* pi = VizWinMgr::getInstance()->getActiveIsoParams();
    savedCommand = PanelCommand::captureStart(pi, qstr.latin1());
}
//This will set dirty bits and undo/redo changes to histo bounds and eventually iso value
void IsoEventRouter::
guiEndChangeIsoSelection(){
	if (!savedCommand) return;
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	iParams->updateHistoBounds();
	PanelCommand::captureEnd(savedCommand,iParams);
	updateTab();
	savedCommand = 0;
	setDatarangeDirty(iParams);
}
void IsoEventRouter::
setIsoEditMode(bool mode){
	navigateButton->setOn(!mode);
	editMode = mode;
}
void IsoEventRouter::
setIsoNavigateMode(bool mode){
	editButton->setOn(!mode);
	editMode = !mode;
}
void IsoEventRouter::guiChangeInstance(int newCurrent){
	performGuiChangeInstance(newCurrent);
}
void IsoEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void IsoEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void IsoEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1) {performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentItem(0);
	performGuiCopyInstanceToViz(viznum);
}

void IsoEventRouter::
setIsoTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void IsoEventRouter::
setIsoTabRenderTextChanged(const QString& ){
	guiSetTextChanged(true);
	renderTextChanged = true;
}

void IsoEventRouter::
isoReturnPressed(void){
	confirmText(true);
}


void IsoEventRouter::
setIsoEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	ParamsIso* iParams = vizMgr->getIsoParams(activeViz, instance);
	//Make sure this is a change:
	if (iParams->isEnabled() == val ) return;
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val,instance);
	
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(iParams,!val, false);
	
	
	updateTab();
}
//Slot that changes the isovalue to be the function value at the selectionPoint
//Must Evaluate the current variable at the selection point, 
//then change the iso value to that value.
//Has no effect if there is no dataMgr
void IsoEventRouter::guiPassThruPoint(){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "pass iso thru point");
	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		iParams->SetIsoValue(val);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab(); 
	VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
float IsoEventRouter::evaluateSelectedPoint(){
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	if (!iParams->isEnabled()) return OUT_OF_BOUNDS;
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	const vector<double> pnt = iParams->GetSelectedPoint();
	float fltpnt[3];
	int numRefinements = iParams->GetRefinementLevel();
	int sessionVarNum = iParams->getSessionVarNum();
	for (int i = 0; i<3; i++) fltpnt[i] = pnt[i];
	float val = rParams->calcCurrentValue(sessionVarNum, fltpnt, numRefinements, timeStep);
	return val;
}


void IsoEventRouter::
refreshHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	
	
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(iParams);
	}
	setEditorDirty(iParams);
}



//Reinitialize Iso tab settings, session has changed.
//Note that this is called after the globalIsoParams are set up, but before
//any of the localIsoParams are setup.
void IsoEventRouter::
reinitTab(bool doOverride){
	Session* ses = Session::getInstance();
	
	variableCombo->clear();
	variableCombo->setMaxCount(ses->getNumMetadataVariables());
	int i;
	for (i = 0; i< ses->getNumMetadataVariables(); i++){
		const std::string& s = ses->getMetadataVarName(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		variableCombo->insertItem(text);
	}

	//Set up the refinement combo:
	const Metadata* md = ses->getCurrentMetadata();
	
	int numRefinements = md->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
		refinementCombo->insertItem(QString::number(i));
	}
	if (histogramList){
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete histogramList;
		histogramList = 0;
		numHistograms = 0;
	}
	
	isoSelectionFrame->update();
	updateTab();
}


void IsoEventRouter::
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are setting it to a valid number
	
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	
	if (num == iParams->getNumRefinements()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set number of refinements");
	
	iParams->SetRefinementLevel(num);
	refinementCombo->setCurrentItem(num);
	PanelCommand::captureEnd(cmd, iParams);
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
	
}
void IsoEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	ParamsIso* iParams = vizWinMgr->getIsoParams(winnum, instance);
	
	if (value == iParams->isEnabled()) return;
	confirmText(false);
	
	//enable the current instance:
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "toggle iso enabled", instance);
	iParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, iParams);
}

/*
 * Method to be invoked after the user has changed a variable
 * Make the labels consistent with the new left/right data limits, but
 * don't trigger a new undo/redo event
 */
void IsoEventRouter::
updateHistoBounds(RenderParams* params){
	QString strn;
	ParamsIso* iParams = (ParamsIso*)params;
	//Find out what timestep is current:
	int viznum = iParams->getVizNum();
	DataStatus* ds = DataStatus::getInstance();
	int varnum = ds->getSessionVariableNum(iParams->GetVariableName());
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentFrameNumber();
	float minData = ds->getDataMin(varnum,currentTimeStep);
	float maxData = ds->getDataMax(varnum,currentTimeStep);

	minDataBound->setText(strn.setNum(minData));
	maxDataBound->setText(strn.setNum(maxData));
	
}
//Copy the point from the probe to the iso panel.  Don't do anything with it.
void IsoEventRouter::
guiCopyProbePoint(){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "copy point from probe");
	const float* selectedpoint = pParams->getSelectedPoint();
	iParams->SetSelectedPoint(selectedpoint);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
}
void IsoEventRouter::
guiSetComboVarNum(int val){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	//The comboVarNum is the metadata num, but the ParamsIso keeps
	//the session variable name
	int comboVarNum = DataStatus::getInstance()->getMetadataVarNum(iParams->GetVariableName());
	if (val == comboVarNum) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set iso variable");
		
	//reset the display range shown on the histo window
	//also set dirty flag
	
	iParams->SetVariableName(DataStatus::getInstance()->getMetadataVarName(val));
	updateMapBounds(iParams);
	updateHistoBounds(iParams);
		
	PanelCommand::captureEnd(cmd, iParams);
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
	
}
		




void IsoEventRouter::
guiSetLighting(bool val){
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	confirmText(false);
	if (iParams->GetNormalOnOff() == val) return;
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "toggle iso lighting");
	iParams->SetNormalOnOff(val);
	PanelCommand::captureEnd(cmd, iParams);
	if (iParams->isEnabled())
		VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();	
}




/* Handle the change of status associated with change of enablement 
 
 * If the window is new, (i.e. we are just creating a new window, use 
 * prevEnabled = false
 
 */
void IsoEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled, bool newWindow){
	
	
	ParamsIso* iParams = (ParamsIso*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = iParams->getVizNum();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	bool nowEnabled = rParams->isEnabled();
	
	if (prevEnabled == nowEnabled) return;

	
	
	VizWin* viz = 0;
	if(winnum >= 0){//Find the viz that this applies to:
		//Note that this is only for the cases below where one particular
		//visualizer is needed
		viz = vizWinMgr->getVizWin(winnum);
	} 
	
	//cases to consider:
	//1.  unchanged disabled renderer; do nothing.
	//  enabled renderer, just force refresh:
	
	if (prevEnabled == nowEnabled) {
		if (!prevEnabled) return;
		
		VizWinMgr::getInstance()->setVizDirty(iParams,DvrRegionBit,true);
		return;
	}
	
	//2.  Change of disable->enable .  Create a new renderer in active window.
	
	
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	
	
	//For a new renderer

	
	if (nowEnabled && !prevEnabled ){//For case 2.:  create a renderer in the active window:

		IsoRenderer* myIso = new IsoRenderer(viz->getGLWindow(), DvrParams::DVR_RAY_CASTER,iParams);

        
		//Render order depends on whether opaque or not; 
		//transparent isos get rendered later.
		int order = 4;
		if (iParams->GetConstantColor()[3] < 1.f) order = 6;
		viz->getGLWindow()->insertRenderer(iParams, myIso, order);

		//force the renderer to refresh region data  (why?)
		
		VizWinMgr::getInstance()->setVizDirty(iParams,DvrRegionBit,true);
		
		
        lightingCheckbox->setEnabled(myIso->hasLighting());

		//Quit 
		return;
	}
	
	assert(prevEnabled && !nowEnabled); //case 6, disable 
	viz->getGLWindow()->removeRenderer(iParams);

	return;
}



//Method called when undo/redo changes params.  If prevParams is null, the
//vizwinmgr will create a new instance.
void IsoEventRouter::
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance) {
	assert(instance >= 0);
	ParamsIso* iParams = (ParamsIso*)(newParams->deepCopy());
	int vizNum = iParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumIsoInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, iParams, Params::IsoParamsType, instance);
	
	updateTab();
	ParamsIso* formerParams = (ParamsIso*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled and/or Local settings changed:
	if (newWin || (formerParams->isEnabled() != iParams->isEnabled())){
		updateRenderer(iParams, wasEnabled,  newWin);
	}

}


void IsoEventRouter::cleanParams(Params* p) 
{
	isoSelectionFrame->setMapperFunction(NULL);
	isoSelectionFrame->setVariableName("");
	isoSelectionFrame->update(); 
}
	
/*
 * Respond to user clicking the color button
 */
void IsoEventRouter::
setConstantColor(){
	
	//Bring up a color selector dialog:
	QColor newColor = QColorDialog::getColor(constantColorButton->paletteBackgroundColor(), this, "Constant Color Selection");
	//Set button color
	constantColorButton->setPaletteBackgroundColor(newColor);
	//Set parameter value of the appropriate parameter set:
	guiSetConstantColor(newColor);
}
void IsoEventRouter::
guiSetConstantColor(QColor& newColor){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "set constant iso color");
	const float* prevColor = iParams->GetConstantColor();
	float fColor[4];
	fColor[0] = (float)newColor.red()/255.f;
	fColor[1] = (float)newColor.green()/255.f;
	fColor[2] = (float)newColor.blue()/255.f;
	fColor[3] = prevColor[3]; //opacity
	iParams->SetConstantColor(fColor);
	PanelCommand::captureEnd(cmd, iParams);
	VizWinMgr::getInstance()->getVizWin(iParams->getVizNum())->updateGL();
}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * of the histogram window
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void IsoEventRouter::
updateMapBounds(RenderParams* params){
	ParamsIso* isoParams = (ParamsIso*)params;
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int varNum = DataStatus::getInstance()->getSessionVariableNum(isoParams->GetVariableName());
	QString strn;
	minDataBound->setText(strn.setNum(DataStatus::getInstance()->getDataMin(varNum, currentTimeStep)));
	maxDataBound->setText(strn.setNum(DataStatus::getInstance()->getDataMax(varNum, currentTimeStep)));
	
	if (isoParams->getMapperFunc()){
		leftHistoEdit->setText(strn.setNum(isoParams->getMapperFunc()->getMinOpacMapValue(),'g',4));
		rightHistoEdit->setText(strn.setNum(isoParams->getMapperFunc()->getMaxOpacMapValue(),'g',4));
	} else {
		leftHistoEdit->setText("0.0");
		rightHistoEdit->setText("1.0");
	}
	
	setEditorDirty(isoParams);
}
void IsoEventRouter::
setEditorDirty(RenderParams* p){
	ParamsIso* ip = (ParamsIso*)p;
	if(!ip) ip = VizWinMgr::getInstance()->getActiveIsoParams();
	if(ip->getMapperFunc())ip->getMapperFunc()->setParams(ip);
    isoSelectionFrame->setMapperFunction(ip->getMapperFunc());
	isoSelectionFrame->setVariableName(ip->GetVariableName());
	isoSelectionFrame->setIsoValue(ip->GetIsoValue());
    isoSelectionFrame->update();

}
void IsoEventRouter::
guiSetNumBits(int val){
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set iso voxel bits");
	//Value is 0 or 1, corresponding to 8 or 16 bits
	iParams->SetNumBits(1<<(val+3));
	PanelCommand::captureEnd(cmd, iParams);
		
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
}
//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void IsoEventRouter::
setDatarangeDirty(RenderParams* params)
{
	ParamsIso* iParams = (ParamsIso*)params;
	if (!iParams->getMapperFunc()) return;
	const float* currentDatarange = iParams->GetHistoBounds();
	float drange[2];
	drange[0] = iParams->getMapperFunc()->getMinColorMapValue();
	drange[1] = iParams->getMapperFunc()->getMaxColorMapValue();
	if (currentDatarange[0] != drange[0] || currentDatarange[1] != drange[1]){
			iParams->SetHistoBounds(drange);
			VizWinMgr::getInstance()->setDatarangeDirty(iParams);
	}
}
