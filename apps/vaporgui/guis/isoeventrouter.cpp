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
#include "DVRRayCaster.h"

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
	connect (mapVariableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetMapComboVarNum(int) ) );
	connect (numBitsCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumBits(int)));
	connect (lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( guiSetLighting(bool) ) );
 
	//Line edits:
	connect (histoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL(returnPressed() ), this, SLOT( isoReturnPressed()));
	connect (TFHistoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (TFHistoScaleEdit, SIGNAL(returnPressed() ), this, SLOT( isoReturnPressed()));
	connect (isoValueEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (isoValueEdit, SIGNAL(returnPressed()), this, SLOT( isoReturnPressed()));
	connect (leftHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (leftHistoEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (rightHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (rightHistoEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (leftMappingEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (rightMappingEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsoTabRenderTextChanged(const QString&)));
	connect (leftMappingEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
	connect (rightMappingEdit, SIGNAL(returnPressed()), this, SLOT(isoReturnPressed()));
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
	// Transfer function controls:
	connect (loadButton, SIGNAL(clicked()), this, SLOT(isoLoadTF()));
	connect (loadInstalledButton, SIGNAL(clicked()), this, SLOT(isoLoadInstalledTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(isoSaveTF()));
	
	connect (opacityScaleSlider, SIGNAL(sliderReleased()), this, SLOT (isoOpacityScale()));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (TFnavigateButton, SIGNAL(toggled(bool)), this, SLOT(setTFNavigateMode(bool)));
	
	connect (TFeditButton, SIGNAL(toggled(bool)), this, SLOT(setTFEditMode(bool)));
	
	connect(TFalignButton, SIGNAL(clicked()), this, SLOT(guiSetTFAligned()));
	
	connect(TFHistoButton, SIGNAL(clicked()), this, SLOT(refreshTFHisto()));
	connect(TFeditButton, SIGNAL(toggled(bool)), 
            transferFunctionFrame, SLOT(setEditMode(bool)));

	connect(TFalignButton, SIGNAL(clicked()),
            transferFunctionFrame, SLOT(fitToView()));

    connect(transferFunctionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeMapFcn(QString)));

    connect(transferFunctionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeMapFcn()));

    connect(transferFunctionFrame, SIGNAL(canBindControlPoints(bool)),
            this, SLOT(setBindButtons(bool)));

	//Instance table stuff:
	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setIsoEnabled(bool,int)));
	
	// isoSelectionFrame controls:
	connect(editButton, SIGNAL(toggled(bool)), 
            isoSelectionFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()), this, SLOT(guiSetIsoAligned()));
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
	setEnabled(!session->sphericalTransform());
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
	const std::string& varname = isoParams->GetMapVariableName();
    int mapComboVarNum = 0;
	if (StrCmpNoCase(varname, "Constant") != 0) {
		//1 extra for constant
		mapComboVarNum = 1+DataStatus::getInstance()->getMetadataVarNum(varname);
	}
		
	
	mapVariableCombo->setCurrentItem(mapComboVarNum);
	//setup the transfer function editor:
	if(mapComboVarNum > 0) {
		transferFunctionFrame->setMapperFunction(isoParams->getMapperFunc());
		updateMapBounds(isoParams);
	}
	else transferFunctionFrame->setMapperFunction(0);

    transferFunctionFrame->updateParams();

	
    if (session->getDataMgr())
    {
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("N/A");
	  Session::getInstance()->unblockRecording();
	  return;
    }
	
	
	numBitsCombo->setCurrentItem((isoParams->GetNumBits())>>4);
	int numRefs = isoParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentItem(numRefs);

	int comboVarNum = DataStatus::getInstance()->getMetadataVarNum(
		isoParams->GetVariableName());
	variableCombo->setCurrentItem(comboVarNum);
	lightingCheckbox->setChecked(isoParams->GetNormalOnOff());
	histoScaleEdit->setText(QString::number(isoParams->GetIsoHistoStretch()));
	TFHistoScaleEdit->setText(QString::number(isoParams->GetHistoStretch()));
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
	
	if(isoParams->getIsoControl()){
		assert(isoParams->getIsoControl()->getParams() == isoParams);
	}
	isoSelectionFrame->setMapperFunction(isoParams->getIsoControl());
	
    isoSelectionFrame->setVariableName(isoParams->GetVariableName());
	updateHistoBounds(isoParams);
	isoSelectionFrame->updateParams();


	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		variableValueLabel->setText(QString::number(val));
	else variableValueLabel->setText("");
	passThruButton->setEnabled((val!= OUT_OF_BOUNDS) && isoParams->isEnabled());
   
	update();
	guiSetTextChanged(false);
	//if (renderTextChanged)
	//	VizWinMgr::getInstance()->getVizWin(isoParams->getVizNum())->updateGL();
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
	
	iParams->SetIsoHistoStretch(histoScaleEdit->text().toFloat());
	iParams->SetHistoStretch(TFHistoScaleEdit->text().toFloat());
	float opac = constantOpacityEdit->text().toFloat();
	const float *clr = iParams->GetConstantColor();
	
	if (clr[3] != opac){
		float newColor[4];
		newColor[3] = opac;
		for (int i = 0; i< 3; i++) newColor[i] = clr[i];
		iParams->SetConstantColor(newColor);
	}
	float bnds[2];
	//const float* oldBnds = iParams->GetHistoBounds();
	bnds[0] = leftHistoEdit->text().toFloat();
	bnds[1] = rightHistoEdit->text().toFloat();

	if (iParams->getIsoControl()) {
		(iParams->getIsoControl())->setMinHistoValue(bnds[0]);
		(iParams->getIsoControl())->setMaxHistoValue(bnds[1]);
	}
	
	if (iParams->getMapperFunc()&& iParams->GetMapVariableNum()>=0) {
		((TransferFunction*)iParams->getMapperFunc())->setMinMapValue(leftMappingEdit->text().toFloat());
		((TransferFunction*)iParams->getMapperFunc())->setMaxMapValue(rightMappingEdit->text().toFloat());
	
		setDatarangeDirty(iParams);
		setEditorDirty();
		update();
	}
	float coords[3];
	coords[0] = selectedXEdit->text().toFloat();
	coords[1] = selectedYEdit->text().toFloat();
	coords[2] = selectedZEdit->text().toFloat();
	iParams->SetSelectedPoint(coords);
	if(iParams->getIsoControl())iParams->getIsoControl()->setIsoValue(isoValueEdit->text().toFloat());

	float val = evaluateSelectedPoint();
	if (val != OUT_OF_BOUNDS)
		variableValueLabel->setText(QString::number(val));
	else variableValueLabel->setText("");
	passThruButton->setEnabled((val!= OUT_OF_BOUNDS) && iParams->isEnabled());
	
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

void IsoEventRouter::isoLoadTF(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	if (iParams->GetMapVariableNum() < 0) return;
	loadTF(iParams, iParams->GetMapVariableNum());
}
void IsoEventRouter::isoLoadInstalledTF(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	if (iParams->GetMapVariableNum() < 0) return;
	loadInstalledTF(iParams,iParams->GetMapVariableNum());
}

void IsoEventRouter::isoSaveTF(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	if (iParams->GetMapVariableNum() < 0) return;
	saveTF(iParams);
}
//Respond to user request to load/save TF
//Assumes name is valid
//
void IsoEventRouter::
sessionLoadTF(QString* name){
	
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->ascii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = iParams->GetMapVariableNum();
	iParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, iParams);
	VizWinMgr::getInstance()->setClutDirty(iParams);
	setEditorDirty();
}

void IsoEventRouter::isoOpacityScale(){
	guiSetOpacityScale(opacityScaleSlider->value());
}
//Respond to a change in opacity scale factor
void IsoEventRouter::
guiSetOpacityScale(int val){
	ParamsIso* pi = VizWinMgr::getActiveIsoParams();
	if(pi->GetMapVariableNum() < 0) return;
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(pi, "modify opacity scale slider");
	pi->setOpacityScale( ((float)(256-val))/256.f);
	float sliderVal = pi->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	PanelCommand::captureEnd(cmd,pi);
	VizWinMgr::getInstance()->setClutDirty(pi);
}
void IsoEventRouter::guiBindColorToOpac(){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "bind Color to Opacity");
    transferFunctionFrame->bindColorToOpacity();
	PanelCommand::captureEnd(cmd, iParams);
}
void IsoEventRouter::guiBindOpacToColor(){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "bind Opacity to Color");
    transferFunctionFrame->bindOpacityToColor();
	PanelCommand::captureEnd(cmd, iParams);
}
void IsoEventRouter::setTFNavigateMode(bool mode){
	TFeditButton->setOn(!mode);
	guiSetTFEditMode(!mode);
}
void IsoEventRouter::setTFEditMode(bool mode){
	TFnavigateButton->setOn(!mode);
	guiSetTFEditMode(mode);
}
void IsoEventRouter::guiSetTFAligned(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "align tf in edit frame");
	setEditorDirty();
	update();
	PanelCommand::captureEnd(cmd, iParams);
}void IsoEventRouter::guiSetIsoAligned(){
	ParamsIso* iParams = (ParamsIso*)VizWinMgr::getInstance()->getApplicableParams(Params::IsoParamsType);
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "fit iso selection window to view");
	setEditorDirty();
	update();
	PanelCommand::captureEnd(cmd, iParams);
}
void IsoEventRouter::refreshTFHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	if (iParams->GetMapVariableNum()<0) return;
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		const float* bnds = iParams->GetMapBounds();
		refreshHistogram(iParams, iParams->GetMapVariableNum(),bnds);
	}
	setEditorDirty();
}
void IsoEventRouter::guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	ParamsIso* pi = VizWinMgr::getInstance()->getActiveIsoParams();
    savedCommand = PanelCommand::captureStart(pi, qstr.latin1());
}

void IsoEventRouter::guiSetTFEditMode(bool mode){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set edit/navigate mode");
	iParams->setMapEditMode(mode);
	PanelCommand::captureEnd(cmd, iParams); 
}
void IsoEventRouter::setBindButtons(bool canbind){
	OpacityBindButton->setEnabled(canbind);
	ColorBindButton->setEnabled(canbind);
}

void IsoEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	PanelCommand::captureEnd(savedCommand,iParams);
	setDatarangeDirty(iParams);
	savedCommand = 0;
}
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
	iParams->SetNodeDirty(ParamsIso::_IsoControlTag);
	
	savedCommand = 0;
	setDatarangeDirty(iParams);
	updateTab();
	
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
		const float* bnds = iParams->GetHistoBounds();
		refreshHistogram(iParams,iParams->GetIsoVariableNum(),bnds);
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

	mapVariableCombo->clear();
	mapVariableCombo->setMaxCount(ses->getNumMetadataVariables()+1);
	mapVariableCombo->insertItem("Constant");
	for (i = 0; i< ses->getNumMetadataVariables(); i++){
		const std::string& s = ses->getMetadataVarName(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		mapVariableCombo->insertItem(text);
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
	
	isoSelectionFrame->updateParams();
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
	
	
	//Check if we really can do raycasting:
	if (value && !DVRRayCaster::supported()){
		MessageReporter::errorMsg("This computer's graphics capabilities do not support isosurfacing.\n %s\n",
			"Update graphics drivers or install a supported graphics card to render isosurfaces.");
		return;
	}
	confirmText(false);

	if (iParams->getMapperFunc() && iParams->GetMapVariableNum()>=0)
    {
      QString strn;

      strn.setNum(iParams->getMapperFunc()->getMinColorMapValue(),'g',7);
      leftMappingEdit->setText(strn);

      strn.setNum(iParams->getMapperFunc()->getMaxColorMapValue(),'g',7);
      rightMappingEdit->setText(strn);
	} 
    else 
    {
      leftMappingEdit->setText("0.0");
      rightMappingEdit->setText("1.0");
	}
	//enable the current instance:
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "toggle iso enabled", instance);
	iParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, iParams);
	RegionParams* rParams = vizWinMgr->getActiveRegionParams();
	vizWinMgr->setRegionDirty(rParams);
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
	//Issue a warning if the probe resolution does not match iso resolution;
	if (pParams->getNumRefinements() != iParams->getNumRefinements()){
		MessageReporter::warningMsg("Note:  Refinement levels of probe and isosurface are different.\n%s",
			"Variable values may differ as a result");
	}
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
	
	updateHistoBounds(iParams);
	
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
	VizWinMgr::getInstance()->setVizDirty(iParams, RegionBit);
	
}
		
void IsoEventRouter::
guiSetMapComboVarNum(int val){
	confirmText(false);
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	//The mapcomboVarNum is the metadata num +1, but the ParamsIso keeps
	//the session variable name
	int comboVarNum = 1+DataStatus::getInstance()->getMetadataVarNum(iParams->GetMapVariableName());
	if (val == comboVarNum) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "set iso variable");
		
	//reset the display range shown on the histo window
	//also set dirty flag
	if(val > 0){
		iParams->SetMapVariableName(DataStatus::getInstance()->getMetadataVarName(val-1));
		updateMapBounds(iParams);
	}
	else iParams->SetMapVariableName("Constant");

	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
	VizWinMgr::getInstance()->setClutDirty(iParams);
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
	iParams->GetRootNode()->SetAllFlags(true);
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
	//Set datarange dirty flag, so will need to check everything and rerender
	
	setDatarangeDirty(iParams);

}


void IsoEventRouter::cleanParams(Params* p) 
{
	isoSelectionFrame->setMapperFunction(NULL);
	isoSelectionFrame->setVariableName("");
	isoSelectionFrame->updateParams(); 
	transferFunctionFrame->setMapperFunction(NULL);
	transferFunctionFrame->setVariableName("");
	transferFunctionFrame->updateParams(); 
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
 * of the transfer function window
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void IsoEventRouter::
updateMapBounds(RenderParams* params){
	ParamsIso* isoParams = (ParamsIso*)params;
	int currentTimeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int varNum = DataStatus::getInstance()->getSessionVariableNum(isoParams->GetMapVariableName());
	QString strn;
	
	if (varNum >= 0){
	
		minTFDataBound->setText(strn.setNum(DataStatus::getInstance()->getDataMin(varNum, currentTimeStep)));
		maxTFDataBound->setText(strn.setNum(DataStatus::getInstance()->getDataMax(varNum, currentTimeStep)));
	
		if (isoParams->getMapperFunc()){
			leftMappingEdit->setText(strn.setNum(isoParams->getMapperFunc()->getMinOpacMapValue(),'g',4));
			rightMappingEdit->setText(strn.setNum(isoParams->getMapperFunc()->getMaxOpacMapValue(),'g',4));
		} else {
			leftMappingEdit->setText("0.0");
			rightMappingEdit->setText("1.0");
		}
	}
	setEditorDirty(isoParams);
}
//Set TF editor as well as isoControl editor dirty.
void IsoEventRouter::
setEditorDirty(RenderParams* p){
	ParamsIso* ip = (ParamsIso*)p;
	if(!ip) ip = VizWinMgr::getInstance()->getActiveIsoParams();
	if(ip->getIsoControl())ip->getIsoControl()->setParams(ip);
    isoSelectionFrame->setMapperFunction(ip->getIsoControl());
	isoSelectionFrame->setVariableName(ip->GetVariableName());
	isoSelectionFrame->setIsoValue(ip->GetIsoValue());
    isoSelectionFrame->updateParams();
	if(ip->getMapperFunc()&& ip->GetMapVariableNum() >= 0){
		ip->getMapperFunc()->setParams(ip);
		transferFunctionFrame->setMapperFunction(ip->getMapperFunc());
		transferFunctionFrame->updateParams();
	}

    Session *session = Session::getInstance();

    if (session->getNumSessionVariables())
    {
      const std::string& varname = ip->GetMapVariableName();
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("N/A");
    }

}
void IsoEventRouter::
guiSetNumBits(int val){
	ParamsIso* iParams = VizWinMgr::getActiveIsoParams();
	confirmText(false);
	if (iParams->isEnabled()){
		MessageReporter::warningMsg("Renderer must be disabled before changing bits per voxel");
		updateTab();
		return;
	}
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
	VizWinMgr::getInstance()->setDatarangeDirty(iParams);
}
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
//Boolean flag is only used by isoeventrouter version
Histo* IsoEventRouter::getHistogram(RenderParams* renParams, bool mustGet, bool isIsoControl ){
	
	ParamsIso* iParams = (ParamsIso*)renParams;
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	int varNum;
	if (isIsoControl) varNum = iParams->GetIsoVariableNum();
	else varNum = iParams->GetMapVariableNum();
	
	if (varNum >= numVariables || varNum < 0) return 0;
	if (varNum >= numHistograms || !histogramList){
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
		numHistograms = numVariables;
	}
	
	const float* currentDatarange;
	if (isIsoControl) currentDatarange = iParams->GetHistoBounds();
	else currentDatarange = iParams->GetMapBounds();
	if (histogramList[varNum]) return histogramList[varNum];
	
	if (!mustGet) return 0;
	histogramList[varNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	refreshHistogram(renParams, varNum,currentDatarange);
	return histogramList[varNum];
	
}
