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
#include "glutil.h"
#include "dvrparams.h"
#include "dvreventrouter.h"
#include "animationeventrouter.h"
#include "eventrouter.h"
#include "colorpickerframe.h"
#include "VolumeRenderer.h"


using namespace VAPoR;


DvrEventRouter::DvrEventRouter(QWidget* parent,const char* name): QWidget(parent), Ui_DVR(), EventRouter(){
        setupUi(this);
	
	myParamsBaseType = VizWinMgr::RegisterEventRouter(Params::_dvrParamsTag, this);
	savedCommand = 0;
    benchmark = DONE;
    benchmarkTimer = 0;
	MessageReporter::infoMsg("DvrEventRouter::DvrEventRouter()");
#ifndef BENCHMARKING
	benchmarkGroup->setMaximumSize(0,0);
#endif
#ifdef Darwin
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
	
	connect (variableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetComboVarNum(int) ) );
	connect (lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( guiSetLighting(bool) ) );
	connect (preintegratedCheckbox, SIGNAL( toggled(bool) ), 
             this, SLOT( guiSetPreintegration(bool) ) );
 
	connect (histoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
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
    connect(transferFunctionFrame, SIGNAL(mappingChanged()),
            this, SLOT(setClutDirty()));

	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setDvrEnabled(bool,int)));
	connect (fitDataButton, SIGNAL(clicked()), this, SLOT(guiFitTFToData()));

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
void DvrEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_dvrParamsTag);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "edit Dvr text");
	QString strn;
	
	dParams->setHistoStretch(histoScaleEdit->text().toFloat());

	if (dParams->getNumVariables() > 0) {
		TransferFunction* tf = (TransferFunction*)dParams->getMapperFunc();
		tf->setMinMapValue(leftMappingBound->text().toFloat());
		tf->setMaxMapValue(rightMappingBound->text().toFloat());
		setEditorDirty();
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
	if (dParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber())){
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
	TransferFunction* tf = dParams->getTransFunc();
	if (!tf) return;
	float minb = tf->getMinMapValue();
	float maxb = tf->getMaxMapValue();
	if (minb >= maxb){ minb = 0.0; maxb = 1.0;}
	loadInstalledTF(dParams,dParams->getSessionVarNum());
	tf = dParams->getTransFunc();
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
	if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
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
    
	if (dvrParams->getMapperFunc()){
		dvrParams->getMapperFunc()->setParams(dvrParams);
		transferFunctionFrame->setMapperFunction(dvrParams->getMapperFunc());
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
	//Force the dvr to refresh  WHY?
	//VizWinMgr::getInstance()->setClutDirty(dvrParams);
    //VizWinMgr::getInstance()->setDatarangeDirty(dvrParams);
	//VizWinMgr::getInstance()->setVizDirty(dvrParams,DvrRegionBit,true, true);

	
	//Disable the typeCombo and bits per pixel whenever the renderer is enabled:
	typeCombo->setEnabled(!(dvrParams->isEnabled()));
	
	typeCombo->setCurrentIndex(typemapi[dvrParams->getType()]);

	int numRefs = dvrParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentIndex(numRefs);
	variableCombo->setCurrentIndex(dvrParams->getComboVarNum());
	
	lodCombo->setCurrentIndex(dvrParams->GetCompressionLevel());
	
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
	updateTab();
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
	
	dParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
}

void DvrEventRouter::
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are changing it
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	if (num == dParams->getNumRefinements()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set number of refinements");
		
	dParams->setNumRefinements(num);
	refinementCombo->setCurrentIndex(num);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
}
void DvrEventRouter::
guiSetEnabled(bool value, int instance){
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
			PanelCommand* cmd = PanelCommand::captureStart(prevParams, "disable existing dvr", prevInstance);
			prevParams->setEnabled(false);
			//turn it off in the instanceTable
			instanceTable->checkEnabledBox(false, prevInstance);
			PanelCommand::captureEnd(cmd, prevParams);
			updateRenderer(prevParams,true,false);
		}
	}
	//now continue with the current instance:
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "toggle dvr enabled", instance);
	dParams->setEnabled(value);
	PanelCommand::captureEnd(cmd, dParams);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!value, false);
	
	setEditorDirty();
	updateTab();
	setDatarangeDirty(dParams);
	
	vizWinMgr->setClutDirty(dParams);
	vizWinMgr->setVizDirty(dParams,DvrRegionBit,true);
	vizWinMgr->setVizDirty(dParams,LightingBit, true);

	if (dParams->getMapperFunc())
    {
      QString strn;

      strn.setNum(dParams->getMapperFunc()->getMinColorMapValue(),'g',7);
      leftMappingBound->setText(strn);

      strn.setNum(dParams->getMapperFunc()->getMaxColorMapValue(),'g',7);
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
	VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
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
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentFrameNumber();
	minDataBound->setText(strn.setNum(dParams->getDataMinBound(currentTimeStep)));
	maxDataBound->setText(strn.setNum(dParams->getDataMaxBound(currentTimeStep)));
	if (dParams->getMapperFunc()){
		leftMappingBound->setText(strn.setNum(dParams->getMapperFunc()->getMinColorMapValue(),'g',7));
		rightMappingBound->setText(strn.setNum(dParams->getMapperFunc()->getMaxColorMapValue(),'g',7));
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
		dParams->setType(VizWinMgr::getInstance()->getDvrRouter()->getType(0));
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
		VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
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
		
		VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
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
	if(dp->getMapperFunc())dp->getMapperFunc()->setParams(dp);
    transferFunctionFrame->setMapperFunction(dp->getMapperFunc());
    transferFunctionFrame->updateParams();

    DataStatus *ds;
	ds = DataStatus::getInstance();

    if (ds->getNumSessionVariables())
    {
      int varnum = dp->getSessionVarNum();
      const std::string& varname = ds->getVariableName(varnum);
      
      transferFunctionFrame->setVariableName(varname);
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
	if (!dParams->getMapperFunc()) return;
	const float* currentDatarange = dParams->getCurrentDatarange();
	float minval = dParams->getMapperFunc()->getMinColorMapValue();
	float maxval = dParams->getMapperFunc()->getMaxColorMapValue();
	if (currentDatarange[0] != minval || currentDatarange[1] != maxval){
			dParams->setCurrentDatarange(minval, maxval);
			VizWinMgr::getInstance()->setDatarangeDirty(dParams);
	}
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
  size_t max_bdim[3];
  size_t min_bdim[3];

  const DataMgr *dataMgr      = Session::getInstance()->getDataMgr();
  RegionParams    *regionParams = VizWinMgr::getActiveRegionParams();
  DvrParams       *dvrParams    = VizWinMgr::getActiveDvrParams();
	
  int timeStep     = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
  int varNum       = dvrParams->getSessionVarNum();
  int numxforms    = dvrParams->getNumRefinements();
  size_t bs[3];
  dataMgr->GetBlockSize(bs,numxforms);

  if(regionParams->getAvailableVoxelCoords(numxforms, min_dim, max_dim, min_bdim, max_bdim, 
          timeStep, &varNum, 1) < 0 ) return;
  
  int nx = (max_bdim[0] - min_bdim[0] + 1) * bs[0];
  int ny = (max_bdim[1] - min_bdim[1] + 1) * bs[1];
  int nz = (max_bdim[2] - min_bdim[2] + 1) * bs[2];

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
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int sesvarnum = pParams->getSessionVarNum();
	float minBound = ds->getDataMin(sesvarnum,ts);
	float maxBound = ds->getDataMax(sesvarnum,ts);
	
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

//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widget 

#ifdef Darwin
void DvrEventRouter::paintEvent(QPaintEvent* ev){
	if(!opacityMapShown){
		QScrollArea* sArea = (QScrollArea*)MainForm::getInstance()->getTabManager()->currentWidget();
		sArea->ensureWidgetVisible(tfFrame);
		transferFunctionFrame->show();
		opacityMapShown = true;
		updateGeometry();
		update();
	}
	QWidget::paintEvent(ev);
}
#endif
