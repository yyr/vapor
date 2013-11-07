//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		dvreventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the DvrEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the dvr tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <QScrollArea>
#include <QScrollBar>
#include <qapplication.h>
#include <qcursor.h>
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
#include <qtimer.h>
#include <qtooltip.h>
#include "instancetable.h"
#include "mappingframe.h"
#include "regionparams.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
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
#include "dvr.h"
#include "transferfunction.h"
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "dvrparams.h"
#include "dvreventrouter.h"
#include "animationeventrouter.h"
#include "eventrouter.h"
#include "colorpickerframe.h"
#include "VolumeRenderer.h"


using namespace VAPoR;


DvrEventRouter::DvrEventRouter(QWidget* parent): QWidget(parent), Ui_DVR(), EventRouter(){
        setupUi(this);
	
	myParamsBaseType = Params::GetTypeFromTag(Params::_dvrParamsTag);
	savedCommand = 0;
    benchmark = DONE;
    benchmarkTimer = 0;
	MessageReporter::infoMsg("DvrEventRouter::DvrEventRouter()");
#ifndef BENCHMARKING
	benchmarkGroup->setMaximumSize(0,0);
#endif
#if defined(Darwin) && (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	transferFunctionFrame->hide();
	opacityMapShown = false;
#endif
}


DvrEventRouter::~DvrEventRouter(){
	if (savedCommand) delete savedCommand;
}
/**********************************************************
 * Whenever a new Dvrtab is created it must be hooked up here
 ************************************************************/
void
DvrEventRouter::hookUpTab()
{
	
	connect (typeCombo, SIGNAL(activated(int)), this, SLOT(guiSetType(int)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (loadButton, SIGNAL(clicked()), this, SLOT(dvrLoadTF()));
	connect (loadInstalledButton, SIGNAL(clicked()), this, SLOT(dvrLoadInstalledTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(dvrSaveTF()));
	connect (savePrefsButton, SIGNAL(clicked()), this, SLOT(dvrSavePrefs()));
	connect (fidelitySlider,SIGNAL(sliderReleased()), this, SLOT(guiSetFidelity()));
	connect (fidelitySlider,SIGNAL(sliderMoved(int)), this, SLOT(fidelityChanging(int)));
	connect (fidelityDefaultButton, SIGNAL(clicked()), this, SLOT(guiSetFidelityDefault()));
	
	connect (variableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetComboVarNum(int) ) );
	connect (lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( guiSetLighting(bool) ) );
	connect (preintegratedCheckbox, SIGNAL( toggled(bool) ), 
             this, SLOT( guiSetPreintegration(bool) ) );
 
	connect (histoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
	connect (fidelityEdit, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
	connect (fidelityEdit, SIGNAL(textChanged(const QString&) ), this,SLOT(setDvrTabTextChanged(const QString&)));
	connect (fidelityEdit, SIGNAL(textChanged(const QString&) ), this,SLOT(setFidelityTextChanged(const QString&)));
	connect (numBitsCombo, SIGNAL (activated(int)), this, SLOT(guiSetNumBits(int)));
	
	// Transfer function controls:

	connect (leftMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (rightMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	
	connect (opacityScaleSlider, SIGNAL(valueChanged(int)), this, SLOT (guiSetOpacityScale(int)));
	connect (ColorBindButton, SIGNAL(pressed()), this, SLOT(guiBindColorToOpac()));
	connect (OpacityBindButton, SIGNAL(pressed()), this, SLOT(guiBindOpacToColor()));
	connect (navigateButton, SIGNAL(toggled(bool)), this, SLOT(setDvrNavigateMode(bool)));
	
	connect (editButton, SIGNAL(toggled(bool)), this, SLOT(setDvrEditMode(bool)));
	
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshHisto()));

    //
    // New Transfer Function Frame
    //
	connect(editButton, SIGNAL(toggled(bool)), 
            transferFunctionFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()),
            transferFunctionFrame, SLOT(fitToView()));
    connect(transferFunctionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeMapFcn(QString)));
    connect(transferFunctionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeMapFcn()));
    connect(transferFunctionFrame, SIGNAL(sendRgb(QRgb)),
            colorPickerFrame, SLOT(newColorTypedIn(QRgb)));
    connect(colorPickerFrame, SIGNAL(hsvOut(int,int,int)), 
            transferFunctionFrame, SLOT(newHsv(int,int,int)));
    connect(transferFunctionFrame, SIGNAL(canBindControlPoints(bool)),
            this, SLOT(setBindButtons(bool)));
	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setDvrEnabled(bool,int)));
	connect (fitDataButton, SIGNAL(clicked()), this, SLOT(guiFitTFToData()));
	connect(colorInterpCheckbox,SIGNAL(toggled(bool)), this, SLOT(guiToggleColorInterpType(bool)));

#ifdef BENCHMARKING
    connect(benchmarkButton, SIGNAL(clicked()),
            this, SLOT(runBenchmarks()));
#else
    benchmarkGroup->hide();
#endif	
}

/*********************************************************************************
 * Slots associated with DvrTab:
 *********************************************************************************/
void DvrEventRouter::guiChangeInstance(int newCurrent){
	
	//
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int numInst = vizMgr->getNumDvrInstances(winnum);
	for (int i = 0; i< numInst; i++){
		//If another instance is enabled, 
		//then enable newCurrent (which will disable the other one)
		if (((DvrParams*)(vizMgr->getDvrParams(winnum, i)))->isEnabled() && 
			i != newCurrent)
			setDvrEnabled(true, newCurrent);
	}
	performGuiChangeInstance(newCurrent);
}
void DvrEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void DvrEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}

void DvrEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1) {performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentIndex(0);
	performGuiCopyInstanceToViz(viznum);
}

void DvrEventRouter::
setDvrTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void DvrEventRouter::
setFidelityTextChanged(const QString& ){
	fidelityTextChanged = true;
}
void DvrEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_dvrParamsTag);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "edit Dvr text");
	QString strn;
	
	dParams->setHistoStretch(histoScaleEdit->text().toFloat());

	if (dParams->getNumVariables() > 0) {
		TransferFunction* tf = (TransferFunction*)dParams->GetMapperFunc();
		tf->setMinMapValue(leftMappingBound->text().toFloat());
		tf->setMaxMapValue(rightMappingBound->text().toFloat());
		setEditorDirty();
	}
	if(fidelityTextChanged) {
		dParams->setIgnoreFidelity(false);
		float qual = fidelityEdit->text().toFloat();
		if (qual < 0. || qual > 1.f) qual = 0.5f;
		int sliderval = (int) (qual*256.);
		fidelitySlider->setValue(sliderval);
		dParams->setFidelity(qual);
	}
	
	if (!dParams->getIgnoreFidelity()){
		float qual = dParams->getFidelity();
		RegionParams* rParams = VizWinMgr::getActiveRegionParams();
		float regionMBs = rParams->fullRegionMBs(-1);
		int lod, reflevel;
		calcLODRefLevel(3, qual, regionMBs, &lod, &reflevel);
		if (lod != dParams->GetCompressionLevel() || reflevel != dParams->GetRefinementLevel()){
			dParams->SetCompressionLevel(lod);
			dParams->SetRefinementLevel(reflevel);
			refinementCombo->setCurrentIndex(reflevel);
			lodCombo->setCurrentIndex(lod);
		}
	}
	guiSetTextChanged(false);
	setDatarangeDirty(dParams);
    VizWinMgr::getInstance()->setVizDirty(dParams, LightingBit, true);		
	PanelCommand::captureEnd(cmd, dParams);
}
void DvrEventRouter::
dvrReturnPressed(void){
	confirmText(true);
}


void DvrEventRouter::
setDvrEnabled(bool val, int instance){

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	DvrParams* dParams = (DvrParams*)(vizMgr->getDvrParams(activeViz, instance));
	//Make sure this is a change:
	if (dParams->isEnabled() == val ) return;
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val,instance);
	typeCombo->setEnabled(!val);
	
}

void DvrEventRouter::
setDvrEditMode(bool mode){
	navigateButton->setChecked(!mode);
	editButton->setChecked(mode);
	guiSetEditMode(mode);
}
void DvrEventRouter::
setDvrNavigateMode(bool mode){
	editButton->setChecked(!mode);
	navigateButton->setChecked(mode);
	guiSetEditMode(!mode);
}

void DvrEventRouter::
refreshHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_dvrParamsTag);
	if (dParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	//Refresh data range:
	//dParams->setDatarangeDirty();
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		const float* datarange = dParams->getCurrentDatarange();
		refreshHistogram(dParams,dParams->getSessionVarNum(),datarange);
		setEditorDirty();
	}
}

//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void DvrEventRouter::
dvrSaveTF(void){
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_dvrParamsTag);
	saveTF(dParams);
}

void DvrEventRouter::
dvrLoadInstalledTF(){
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_dvrParamsTag);
	TransferFunction* tf = dParams->GetTransFunc();
	if (!tf) return;
	float minb = tf->getMinMapValue();
	float maxb = tf->getMaxMapValue();
	if (minb >= maxb){ minb = 0.0; maxb = 1.0;}
	loadInstalledTF(dParams,dParams->getSessionVarNum());
	tf = dParams->GetTransFunc();
	tf->setMinMapValue(minb);
	tf->setMaxMapValue(maxb);
	setEditorDirty();
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
void DvrEventRouter::
dvrLoadTF(void){
	//If there are no TF's currently in Session, just launch file load dialog.
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_dvrParamsTag);
	loadTF(dParams,dParams->getSessionVarNum());
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
//Respond to user request to load/save TF
//Assumes name is valid
//
void DvrEventRouter::
sessionLoadTF(QString* name){
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	if (dParams-> getNumVariables() <= 0) return;
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->toStdString());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getSessionVarNum();
	dParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	setEditorDirty();
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}

//Insert values from params into tab panel
//
void DvrEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	MessageReporter::infoMsg("DvrEventRouter::updateTab()");
	if (!isEnabled()) return;
	if (GLWindow::isRendering())return;
	Session *session = Session::getInstance();
	session->blockRecording();

	if (DataStatus::getInstance()->getDataMgr()) instanceTable->setEnabled(true);
	else instanceTable->setEnabled(false);
	instanceTable->rebuild(this);

	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
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


	initTypes();
	DvrParams* dvrParams = (DvrParams*) VizWinMgr::getActiveDvrParams();
	
	deleteInstanceButton->setEnabled(vizMgr->getNumDvrInstances(winnum) > 1);
	
	QString strn;
    
	if (dvrParams->GetMapperFunc()){
		dvrParams->GetMapperFunc()->setParams(dvrParams);
		transferFunctionFrame->setMapperFunction(dvrParams->GetMapperFunc());
		TFInterpolator::type t = dvrParams->GetMapperFunc()->colorInterpType();
		if (colorInterpCheckbox->isChecked() && t != TFInterpolator::discrete) 
			colorInterpCheckbox->setChecked(false);
		if (!colorInterpCheckbox->isChecked() && t == TFInterpolator::discrete) 
			colorInterpCheckbox->setChecked(true);
	}
	transferFunctionFrame->updateParams();

    if (session->getNumSessionVariables()&&DataStatus::getInstance()->getDataMgr())
    {
      int varnum = dvrParams->getSessionVarNum();
      const std::string& varname = session->getVariableName(varnum);
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("");
    }
	
	
	//Disable the typeCombo and bits per pixel whenever the renderer is enabled:
	typeCombo->setEnabled(!(dvrParams->isEnabled()));
	
	typeCombo->setCurrentIndex(typemapi[dvrParams->getType()]);

	
	variableCombo->setCurrentIndex(dvrParams->getComboVarNum());
	
	
	float fidelity = dvrParams->getFidelity();
	fidelitySlider->setValue((int)(fidelity*256.));
	fidelityEdit->setText(QString::number(fidelity));
	if (!dvrParams->getIgnoreFidelity()){
		RegionParams* rParams = VizWinMgr::getActiveRegionParams();
		float regionMBs = rParams->fullRegionMBs(-1);
		int lod, reflevel;
		calcLODRefLevel(3, fidelity, regionMBs, &lod, &reflevel);
		lodCombo->setCurrentIndex(lod);
		refinementCombo->setCurrentIndex(reflevel);
	} else {
		int numRefs = dvrParams->GetRefinementLevel();
		if(numRefs <= refinementCombo->count())
			refinementCombo->setCurrentIndex(numRefs);
		lodCombo->setCurrentIndex(dvrParams->GetCompressionLevel());
	}
	lightingCheckbox->setChecked(dvrParams->getLighting());
	preintegratedCheckbox->setChecked(dvrParams->getPreIntegration());

	numBitsCombo->setCurrentIndex((dvrParams->getNumBits())>>4);
	histoScaleEdit->setText(QString::number(dvrParams->GetHistoStretch()));
	

	updateMapBounds(dvrParams);
	
	float sliderVal = dvrParams->getOpacityScale();
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(sliderVal));
	//Make slider posited based on square root of slider value, to make it easier to manipulate:
	sliderVal = sqrt(sliderVal);
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	
	//Set the mode buttons:
	
	if (dvrParams->getEditMode()){
		
		editButton->setChecked(true);
		navigateButton->setChecked(false);
	} else {
		editButton->setChecked(false);
		navigateButton->setChecked(true);
	}
		
	setEditorDirty();
	update();
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	vizMgr->getTabManager()->update();
}

//Reinitialize Dvr tab settings, session has changed.
//Note that this is called after the globalDvrParams are set up, but before
//any of the localDvrParams are setup.
void DvrEventRouter::
reinitTab(bool doOverride){
	
	DataStatus* ds = DataStatus::getInstance();
	if (ds->dataIsPresent3D()) {
		setEnabled(true);
		instanceTable->setEnabled(true);
		instanceTable->rebuild(this);
	}
	else setEnabled(false);
	variableCombo->clear();
	variableCombo->setMaxCount(ds->getNumActiveVariables3D());
	int i;
	for (i = 0; i< ds->getNumActiveVariables3D(); i++){
		const std::string& s = ds->getActiveVarName3D(i);
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& text = QString(s.c_str());
		variableCombo->addItem(text);
	}

	//Set up the refinement combo:
	const DataMgr *dataMgr = ds->getDataMgr();
	
	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (i = 0; i<= numRefinements; i++){
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
	
	
	if (histogramList){
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete [] histogramList;
		histogramList = 0;
		numHistograms = 0;
	}
	setBindButtons(false);
	fidelityTextChanged = false;
	updateTab();
}
void DvrEventRouter::
guiToggleColorInterpType(bool isDiscrete){
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams,  "toggle discrete color interpolation");
	if (isDiscrete)
		dParams->GetMapperFunc()->setColorInterpType(TFInterpolator::discrete);
	else 
		dParams->GetMapperFunc()->setColorInterpType(TFInterpolator::linear);
	updateTab();
	
	//Force a redraw
	
	setEditorDirty();
	PanelCommand::captureEnd(cmd,dParams);
	if (dParams->isEnabled())VizWinMgr::getInstance()->forceRender(dParams,true);
}
//Change mouse mode to specified value
//0,1,2 correspond to edit, zoom, pan
void DvrEventRouter::
guiSetEditMode(bool mode){
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set edit/navigate mode");
	dParams->setEditMode(mode);
	PanelCommand::captureEnd(cmd, dParams); 
}
void DvrEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	if (num == dParams->GetCompressionLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set compression level");
	dParams->setIgnoreFidelity(true);
	dParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,NavigatingBit,true);
}

void DvrEventRouter::
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are changing it
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	if (num == dParams->GetRefinementLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set number of refinements");
	dParams->setIgnoreFidelity(true);
	dParams->SetRefinementLevel(num);
	refinementCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,NavigatingBit,true);
}
void DvrEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	DvrParams* dParams = (DvrParams*)(vizWinMgr->getDvrParams(winnum, instance));
	
	if (value == dParams->isEnabled()) return;
	confirmText(false);
	//On enable, disable any existing dvr renderer in this window:
	if (value){
		
		VizWin* viz = vizWinMgr->getActiveVisualizer();
		DvrParams* prevParams = (DvrParams*)viz->getGLWindow()->findARenderer(Params::GetTypeFromTag(Params::_dvrParamsTag));
		assert(prevParams != dParams);
		if (prevParams){
			
			int prevInstance = vizWinMgr->findInstanceIndex(winnum, prevParams, 
				Params::GetTypeFromTag(Params::_dvrParamsTag));
			assert (prevInstance >= 0);
			//Put the disable in the history:
			PanelCommand* cmd;
			if(undoredo) cmd= PanelCommand::captureStart(prevParams, "disable existing dvr", prevInstance);
			prevParams->setEnabled(false);
			//turn it off in the instanceTable
			instanceTable->checkEnabledBox(false, prevInstance);
			if(undoredo) PanelCommand::captureEnd(cmd, prevParams);
			updateRenderer(prevParams,true,false);
		}
	}
	//now continue with the current instance:
//	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle dvr enabled", instance);
	dParams->setEnabled(value);
//	PanelCommand::captureEnd(cmd, dParams);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!value, false);
	
	setEditorDirty();
	updateTab();
	setDatarangeDirty(dParams);
	
	vizWinMgr->setClutDirty(dParams);
	vizWinMgr->setVizDirty(dParams,NavigatingBit,true);
	vizWinMgr->setVizDirty(dParams,LightingBit, true);

	if (dParams->GetMapperFunc())
    {
      QString strn;

      strn.setNum(dParams->GetMapperFunc()->getMinColorMapValue(),'g',7);
      leftMappingBound->setText(strn);

      strn.setNum(dParams->GetMapperFunc()->getMaxColorMapValue(),'g',7);
      rightMappingBound->setText(strn);
	} 
    else 
    {
      leftMappingBound->setText("0.0");
      rightMappingBound->setText("1.0");
	}
}

void DvrEventRouter::
guiSetType(int val)
{ 
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "change renderer type");
		
	dParams->setType(typemap[val]); 
	PanelCommand::captureEnd(cmd, dParams);
}


void DvrEventRouter::
guiSetComboVarNum(int val){
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	if (val == dParams->getComboVarNum()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set dvr variable");
		
	
	//reset the editing display range shown on the tab, 
	//also set dirty flag
	
	//Force a redraw of transfer function
	setEditorDirty();
	
	
	
	dParams->setVarNum(DataStatus::getInstance()->mapActiveToSessionVarNum3D(val));
	updateMapBounds(dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,NavigatingBit,true);
	VizWinMgr::getInstance()->setClutDirty(dParams);
	setDatarangeDirty(dParams);
		
	PanelCommand::captureEnd(cmd, dParams);
		
}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void DvrEventRouter::
updateMapBounds(RenderParams* params){
	QString strn;
	DvrParams* dParams = (DvrParams*)params;
	//Find out what timestep is current:
	int viznum = dParams->getVizNum();
	if (viznum < 0) return;
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentTimestep();
	float minbnd, maxbnd;
	int varnum = dParams->getSessionVarNum();
	if (params->isEnabled()){
		minbnd = dParams->getDataMinBound(currentTimeStep);
		maxbnd = dParams->getDataMaxBound(currentTimeStep);
	} else {
		minbnd = DataStatus::getInstance()->getDefaultDataMin3D(varnum);
		maxbnd = DataStatus::getInstance()->getDefaultDataMax3D(varnum);
	}
	minDataBound->setText(strn.setNum(minbnd));
	maxDataBound->setText(strn.setNum(maxbnd));
	if (dParams->GetMapperFunc()){
		leftMappingBound->setText(strn.setNum(dParams->GetMapperFunc()->getMinColorMapValue(),'g',7));
		rightMappingBound->setText(strn.setNum(dParams->GetMapperFunc()->getMaxColorMapValue(),'g',7));
	} else {
		leftMappingBound->setText("0.0");
		rightMappingBound->setText("1.0");
	}
	
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
	setEditorDirty();
}
// Method to initialize the DVR types available to the user.
void DvrEventRouter::initTypes()
{
  int index = 0;

  typemap.clear();
  typemapi.clear();
  
  typeCombo->clear();


  if (VolumeRenderer::supported(DvrParams::DVR_VOLUMIZER))
  {
    typeCombo->insertItem(index,"Volumizer");
    typemap[index] = DvrParams::DVR_VOLUMIZER;
    typemapi[DvrParams::DVR_VOLUMIZER] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_TEXTURE3D_SHADER))
  {
    typeCombo->insertItem(index,"3DTexture-Shader");
    typemap[index] = DvrParams::DVR_TEXTURE3D_SHADER;
    typemapi[DvrParams::DVR_TEXTURE3D_SHADER] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_TEXTURE3D_LOOKUP))
  {
    typeCombo->insertItem(index,"3DTexture");
    typemap[index] = DvrParams::DVR_TEXTURE3D_LOOKUP;
    typemapi[DvrParams::DVR_TEXTURE3D_LOOKUP] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_SPHERICAL_SHADER))
  {
    typeCombo->insertItem(index,"Spherical-Shader");
    typemap[index] = DvrParams::DVR_SPHERICAL_SHADER;
    typemapi[DvrParams::DVR_SPHERICAL_SHADER] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_DEBUG))
  {
    typeCombo->insertItem(index,"Debug" );
    typemap[index] = DvrParams::DVR_DEBUG;
    typemapi[DvrParams::DVR_DEBUG] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_STRETCHED_GRID))
  {
    typeCombo->insertItem(index,"Stretched Grid");
    typemap[index++] = DvrParams::DVR_STRETCHED_GRID;
    typemapi[DvrParams::DVR_STRETCHED_GRID] = index;
    index++;
  }
  
  typeCombo->setCurrentIndex(0);
  //Set all dvr's to the first available type:
  //type = typemap[0];
}

void DvrEventRouter::setBindButtons(bool canbind)
{
  OpacityBindButton->setEnabled(canbind);
  ColorBindButton->setEnabled(canbind);
}

void DvrEventRouter::
guiSetNumBits(int val){
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	confirmText(false);
	if (dParams->isEnabled()){
		ReenablePanelCommand* cmd = ReenablePanelCommand::captureStart(dParams, "set dvr voxel bits");
		//Disable and enable:
		dParams->setEnabled(false);
		updateRenderer(dParams,true,false);
		//Value is 0 or 1, corresponding to 8 or 16 bits
		dParams->setNumBits(1<<(val+3));
		dParams->setEnabled(true);
		updateRenderer(dParams,false,false);
		ReenablePanelCommand::captureEnd(cmd, dParams);
	} else {
		//Otherwise just use a normal setting
		PanelCommand* cmd = PanelCommand::captureStart(dParams, "set dvr voxel bits");
		//Value is 0 or 1, corresponding to 8 or 16 bits
		dParams->setNumBits(1<<(val+3));
		PanelCommand::captureEnd(cmd, dParams);
	}
	VizWinMgr::getInstance()->setVizDirty(dParams,RegionBit);
	
}
void DvrEventRouter::
guiSetLighting(bool val){
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams,  "toggle dvr lighting");
		
	dParams->setLighting(val);
	PanelCommand::captureEnd(cmd, dParams);

	VizWinMgr::getInstance()->setClutDirty(dParams);
    VizWinMgr::getInstance()->setVizDirty(dParams,LightingBit,true);		
}

void DvrEventRouter::guiSetPreintegration(bool val)
{
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle dvr pre-intgrated transfer functions");
		
	dParams->setPreIntegration(val);
	PanelCommand::captureEnd(cmd, dParams);

	VizWinMgr::getInstance()->setClutDirty(dParams);
    VizWinMgr::getInstance()->setVizDirty(dParams, LightingBit, true);		
}

//Respond to a change in opacity slider
void DvrEventRouter::
guiSetOpacityScale(int val){
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();

	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "modify opacity scale slider");
		
	float opacScale = (float)(256-val)/256.f;
	//Square it, for better control over low opacities:
	opacScale = opacScale*opacScale;
	dParams->setOpacityScale(opacScale);
	
	opacityScaleSlider->setToolTip("Opacity Scale Value = "+QString::number(opacScale));
	
	VizWinMgr::getInstance()->setClutDirty(dParams);
	setEditorDirty();
	
	PanelCommand::captureEnd(cmd,dParams);
		
}
//Respond to a change in transfer function (from color selection or mouse down/release events)
//These are just for undo/redo.  Also may need to update visualizer and/or editor
//
void DvrEventRouter::
guiStartChangeMapFcn(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	
	savedCommand = PanelCommand::captureStart(dParams, qstr.toLatin1());
		
}
void DvrEventRouter::
guiEndChangeMapFcn(){
	if (!savedCommand) return;
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	PanelCommand::captureEnd(savedCommand,dParams);
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
	savedCommand = 0;
}

void DvrEventRouter::
guiBindColorToOpac(){
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "bind Color to Opacity");
		
    transferFunctionFrame->bindColorToOpacity();
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
void DvrEventRouter::
guiBindOpacToColor(){
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "bind Opacity to Color");
		
    transferFunctionFrame->bindOpacityToColor();
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
/* Handle the change of status associated with change of enablement 
 
 * If the window is new, (i.e. we are just creating a new window, use 
 * prevEnabled = false
 
 */
void DvrEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled, bool newWindow){
	
	
	DvrParams* dParams = (DvrParams*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = dParams->getVizNum();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	bool nowEnabled = rParams->isEnabled();
	
	if (prevEnabled == nowEnabled) return;

	//If we are enabling, make sure the dvrparams are set to a valid type:
	if (nowEnabled && dParams->getType() == DvrParams::DVR_INVALID_TYPE){
		dParams->setType(getType(0));
	}
	
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
		setDatarangeDirty(dParams);
		VizWinMgr::getInstance()->setVizDirty(dParams,NavigatingBit,true);
		return;
	}
	
	//2.  Change of disable->enable .  Create a new renderer in active window.
	
	
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	
	
	//For a new renderer

	
	if (nowEnabled && !prevEnabled ){//For case 2.:  create a renderer in the active window:

		VolumeRenderer* myDvr = new VolumeRenderer(viz->getGLWindow(), dParams->getType(),dParams);

        connect((QObject*)myDvr, 
                SIGNAL(statusMessage(const QString&)),
                (QObject*)MainForm::getInstance()->statusBar(), 
                SLOT(showMessage(const QString &)));

		
		viz->getGLWindow()->insertSortedRenderer(dParams,myDvr);

		//force the renderer to refresh region data  (why?)
		
		VizWinMgr::getInstance()->setVizDirty(dParams,NavigatingBit,true);
		VizWinMgr::getInstance()->setClutDirty(dParams);
		VizWinMgr::getInstance()->setVizDirty(dParams,LightingBit, true);
		setDatarangeDirty(dParams);
		
        lightingCheckbox->setEnabled(myDvr->hasLighting());
        preintegratedCheckbox->setEnabled(myDvr->hasPreintegration());

		//Quit 
		return;
	}
	
	assert(prevEnabled && !nowEnabled); //case 6, disable 
	viz->getGLWindow()->removeRenderer(dParams);

	return;
}
void DvrEventRouter::
setEditorDirty(RenderParams *p){
	DvrParams* dp = (DvrParams*) p;
	if (!dp) dp = VizWinMgr::getInstance()->getActiveDvrParams();
	if(dp->GetMapperFunc())dp->GetMapperFunc()->setParams(dp);
    transferFunctionFrame->setMapperFunction(dp->GetMapperFunc());
    transferFunctionFrame->updateParams();

    DataStatus *ds;
	ds = DataStatus::getInstance();
	int varnum = dp->getSessionVarNum();
    if (ds->getNumSessionVariables() && varnum >= 0)
    {
      const std::string& varname = ds->getVariableName3D(varnum);
      transferFunctionFrame->setVariableName(varname);
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
			TFInterpolator::type t = mf->colorInterpType();
			if (colorInterpCheckbox->isChecked() && t != TFInterpolator::discrete) 
				colorInterpCheckbox->setChecked(false);
			if (!colorInterpCheckbox->isChecked() && t == TFInterpolator::discrete) 
				colorInterpCheckbox->setChecked(true);
		}
	}
}



void DvrEventRouter::setClutDirty()
{
  DvrParams* dParams = VizWinMgr::getActiveDvrParams();
  VizWinMgr::getInstance()->setClutDirty(dParams);
}

//Method to invalidate a datarange, and to force a rendering
//with new data quantization
void DvrEventRouter::
setDatarangeDirty(RenderParams* params)
{
	DvrParams* dParams = (DvrParams*)params;
	if (!dParams->GetMapperFunc()) return;
	float minval = dParams->GetMapperFunc()->getMinColorMapValue();
	float maxval = dParams->GetMapperFunc()->getMaxColorMapValue();
	dParams->setCurrentDatarange(minval, maxval);
	VizWinMgr::getInstance()->setDatarangeDirty(dParams);
}
//Method called when undo/redo changes params.  If prevParams is null, the
//vizwinmgr will create a new instance.
void DvrEventRouter::
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance, bool reEnable) {
	assert(instance >= 0);

	DvrParams* dParams = (DvrParams*)(newParams->deepCopy());
	int vizNum = dParams->getVizNum();
	
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumDvrInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, dParams, Params::GetTypeFromTag(Params::_dvrParamsTag), instance);
	
	//setEditorDirty(dParams);
	//updateTab();
	DvrParams* formerParams = (DvrParams*)prevParams;
	if (reEnable){
		//Need to disable the current instance
		assert(dParams->isEnabled() && formerParams->isEnabled());
		assert(!newWin);
		dParams->setEnabled(false);
		//Install dParams disabled:
		updateRenderer(dParams, true, false);
		//Then enable it
		dParams->setEnabled(true);
		updateRenderer(dParams, false, false);

	} else {
		bool wasEnabled = false;
		if (formerParams) wasEnabled = formerParams->isEnabled();
		//Check if the enabled and/or Local settings changed:
		if (newWin || (formerParams->isEnabled() != dParams->isEnabled())){
			updateRenderer(dParams, wasEnabled,  newWin);
		}
	}

	setDatarangeDirty(dParams);
	updateClut(dParams);
    setEditorDirty();
	updateTab();
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DvrEventRouter::runBenchmarks()
{
  VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  DvrParams *dvrParams     = VizWinMgr::getInstance()->getActiveDvrParams();

  VolumeRenderer* renderer = 
    (VolumeRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(dvrParams);

  if (benchmarkTimer == NULL)
  {
    benchmarkTimer = new QTimer(this);
    connect(benchmarkTimer, SIGNAL(timeout()), this, SLOT(benchmarkTimeout()));
  }

  //
  // Start benchmarks
  //
  renderer->resetTimer();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  benchmark = PREAMBLE;
  benchmarkTimer->start(0);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DvrEventRouter::benchmarkTimeout()
{
  switch (benchmark)
  {
    case PREAMBLE:
      benchmarkPreamble();
      break;
    case RENDER:
      renderBenchmark();
      break;
    case TEMPORAL:
      temporalBenchmark();
      break;
    case TFEDIT:
      tfeditBenchmark();
      break;
    default:
      benchmarkTimer->stop();
      QApplication::restoreOverrideCursor();
  }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DvrEventRouter::nextBenchmark()
{
  VizWinMgr *mgr = VizWinMgr::getInstance();
  VizWin *vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  DvrParams *dvrParams     = VizWinMgr::getInstance()->getActiveDvrParams();
  VolumeRenderer* renderer = 
    (VolumeRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(dvrParams);

  benchmark++;

  switch (benchmark)
  {
    case RENDER:

      if (renderCheck->isChecked())
      {
        renderer->resetTimer();
        break;
      }
      else
      {
        benchmark++;
      }
      
    case TEMPORAL:

      if(animationCheck->isChecked())
      {
        AnimationEventRouter *animation = mgr->getAnimationRouter();
        
        //
        // Start the benchmark
        //
        renderer->resetTimer();
        animation->guiToggleReplay(true);
        animation->guiSetPlay(1);
        
        break;
      }
      else
      {
        benchmark++;
      }

    case TFEDIT:
      
      if (tfCheck->isChecked())
      {
        renderer->resetTimer();
        guiSetOpacityScale(0);
        break;
      }
      else
      {
        benchmark++;
      }
  }
}


//----------------------------------------------------------------------------
// Print out a premable with type and size information
//----------------------------------------------------------------------------
void DvrEventRouter::benchmarkPreamble()
{
  

  size_t max_dim[3];
  size_t min_dim[3];
 
  RegionParams    *regionParams = VizWinMgr::getActiveRegionParams();
  DvrParams       *dvrParams    = VizWinMgr::getActiveDvrParams();
	
  int timeStep     = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
  int varNum       = dvrParams->getSessionVarNum();
  int numxforms    = dvrParams->GetRefinementLevel();
  int lod			= dvrParams->GetCompressionLevel();
  
  if(regionParams->getAvailableVoxelCoords(numxforms, lod,min_dim, max_dim, 
          timeStep, &varNum, 1) < 0 ) return;
  
  int nx = (max_dim[0] - min_dim[0] + 1);
  int ny = (max_dim[1] - min_dim[1] + 1);
  int nz = (max_dim[2] - min_dim[2] + 1);

  VizWin *vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  cout << " " << typeCombo->currentText().data() << " @ ";
  cout << nx << "x" << ny << "x" << nz << endl;

  nextBenchmark();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DvrEventRouter::renderBenchmark()
{
  VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  DvrParams *dvrParams     = VizWinMgr::getInstance()->getActiveDvrParams();

  VolumeRenderer* renderer = 
    (VolumeRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(dvrParams);


  if (renderer->renderedFrames() < 100 && renderer->elapsedTime() < 10.0)
  {
    vizWin->updateGL();
  } 
  else
  {
    cout << "  Render Benchmark: " 
         << (float)renderer->renderedFrames()/renderer->elapsedTime() 
         << " fps" << endl;

    nextBenchmark();
  }
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DvrEventRouter::temporalBenchmark()
{
  VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  DvrParams *dvrParams     = VizWinMgr::getInstance()->getActiveDvrParams();
  VolumeRenderer* renderer = 
    (VolumeRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(dvrParams);

  if (renderer->renderedFrames() > 3 && 
      (renderer->renderedFrames() > 100 || renderer->elapsedTime() > 10.0))
  {
    VizWinMgr::getInstance()->getAnimationRouter()->guiSetPlay(0);

    cout << "  Animation (temporal) Benchmark: " 
         << (float)renderer->renderedFrames()/renderer->elapsedTime() 
         << " fps" << endl;

    nextBenchmark();
  }
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DvrEventRouter::tfeditBenchmark()
{
  VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  DvrParams *dvrParams     = VizWinMgr::getInstance()->getActiveDvrParams();
  VolumeRenderer* renderer = 
    (VolumeRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(dvrParams);

  if (renderer->renderedFrames() < 255 && renderer->elapsedTime() < 10.0)
  {
    guiSetOpacityScale(renderer->renderedFrames());
    vizWin->updateGL();
  }
  else
  {
    cout << "  Transfer Function Benchmark: " 
         << (float)renderer->renderedFrames()/renderer->elapsedTime() 
         << " fps" << endl;

    guiSetOpacityScale(0);
    vizWin->updateGL();

    nextBenchmark();
  }
}
void DvrEventRouter::cleanParams(Params* p) 
{
  transferFunctionFrame->setMapperFunction(NULL);
  transferFunctionFrame->setVariableName("");
  transferFunctionFrame->updateParams();
}
	
void DvrEventRouter::refreshTab(){
	bigDVRFrame->hide();
	bigDVRFrame->show();
}
void DvrEventRouter::guiFitTFToData(){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	confirmText(false);
	DvrParams* pParams = VizWinMgr::getActiveDvrParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "fit TF to data");
	//Get bounds from DataStatus:
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	int sesvarnum = pParams->getSessionVarNum();
	float minBound = ds->getDataMin3D(sesvarnum,ts);
	float maxBound = ds->getDataMax3D(sesvarnum,ts);
	
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
//Occurs when user releases fidelity slider
void DvrEventRouter::guiSetFidelity(){
	// Recalculate LOD and refinement based on current slider value and/or current text value
	//.  If they don't change, then don't 
	// generate an event.
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	
	//Determine region size
	RegionParams    *regionParams = VizWinMgr::getActiveRegionParams();
	float regionMBs = regionParams->fullRegionMBs(-1);
	int q = fidelitySlider->value();
	float fidelity = (float)q/256.;
	fidelityEdit->setText(QString::number(fidelity));
	int lod, reflevel;
	calcLODRefLevel(3, fidelity, regionMBs, &lod, &reflevel);
	if (lod == lodSet && refSet == reflevel) return;
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Set Data Fidelity");
	dParams->SetCompressionLevel(lod);
	dParams->SetRefinementLevel(reflevel);
	dParams->setFidelity(fidelity);
	dParams->setIgnoreFidelity(false);
	//change values of LOD and refinement combos using setCurrentIndex().
	lodCombo->setCurrentIndex(lod);
	refinementCombo->setCurrentIndex(reflevel);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->forceRender(dParams, true);
}
//As fidelity slider moves, update the LOD and refinement combos without causing
//change in data
void DvrEventRouter::fidelityChanging(int val){
	//Determine region size
	RegionParams    *regionParams = VizWinMgr::getActiveRegionParams();
	float regionMBs = regionParams->fullRegionMBs(-1);
	float fidelity = (float)val/256.;
	fidelityEdit->setText(QString::number(fidelity));
	int lod, reflevel;
	calcLODRefLevel(3, fidelity, regionMBs, &lod, &reflevel);
	//change values of LOD and refinement combos using setCurrentIndex().
	lodCombo->setCurrentIndex(lod);
	refinementCombo->setCurrentIndex(reflevel);
}
//User clicks on SetDefault button, need to make current fidelity settings the default.
void DvrEventRouter::guiSetFidelityDefault(){
	//Check current values of LOD and refinement and their combos.
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	confirmText(false);
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Set Fidelity Default LOD and refinement");
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float regionMBs = rParams->fullRegionMBs(-1);
	//Adjust lod, ref based on actual region size
	const vector<size_t> comprs = dataMgr->GetCRatios();
	float lodDefault = (float)comprs[lodSet]/regionMBs;
	float refDefault = (float)refSet+log(regionMBs)/log(2.);
	//Set defaultLOD3d and defaultRefinement3D values
	UserPreferences::setDefaultLODFidelity3D(lodDefault);
	UserPreferences::setDefaultRefinementFidelity3D(refDefault);
	
	//Clear ignoreFidelity flag
	dParams->setIgnoreFidelity(false);
	//Set Fidelity to 0.5.
	dParams->setFidelity(0.5f);
	fidelitySlider->setValue(128);
	fidelityEdit->setText("0.5");
	PanelCommand::captureEnd(cmd, dParams);
	//Need undo/redo to include preference settings!
}
void DvrEventRouter::dvrSavePrefs(){
	UserPreferences::requestSave(false);
}
//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widget 

#ifdef Darwin
void DvrEventRouter::paintEvent(QPaintEvent* ev){

#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	if(!opacityMapShown){
		QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
		sArea->ensureWidgetVisible(tfFrame);
		opacityMapShown = true;
	}
#endif

	QWidget::paintEvent(ev);
	transferFunctionFrame->show();
}
#endif
