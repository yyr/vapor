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
#include <qapplication.h>
#include <qcursor.h>
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
#include <qtimer.h>
#include <qtooltip.h>
#include "instancetable.h"
#include "MappingFrame.h"
#include "regionparams.h"
#include "regiontab.h"
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
#include "vapor/Metadata.h"
#include "vapor/XmlNode.h"
#include "vapor/VDFIOBase.h"
#include "tabmanager.h"
#include "glutil.h"
#include "dvrparams.h"
#include "dvreventrouter.h"
#include "animationeventrouter.h"
#include "eventrouter.h"
#include "savetfdialog.h"
#include "loadtfdialog.h"
#include "colorpicker.h"
#include "VolumeRenderer.h"
#include "vapor/errorcodes.h"

using namespace VAPoR;


DvrEventRouter::DvrEventRouter(QWidget* parent,const char* name): Dvr(parent, name), EventRouter(){
	myParamsType = Params::DvrParamsType;
	savedCommand = 0;
    benchmark = DONE;
    benchmarkTimer = 0;
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
	connect (loadButton, SIGNAL(clicked()), this, SLOT(dvrLoadTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(dvrSaveTF()));
	
	connect (variableCombo, SIGNAL( activated(int) ), this, SLOT( guiSetComboVarNum(int) ) );
	connect (lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( guiSetLighting(bool) ) );
	connect (preintegratedCheckbox, SIGNAL( toggled(bool) ), 
             this, SLOT( guiSetPreintegration(bool) ) );
 
	connect (histoScaleEdit,SIGNAL(textChanged(const QString&)),this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
	connect (numBitsSpin, SIGNAL (valueChanged(int)), this, SLOT(guiSetNumBits(int)));
	
	// Transfer function controls:

	connect (leftMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (rightMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	
	connect (opacityScaleSlider, SIGNAL(sliderReleased()), this, SLOT (dvrOpacityScale()));
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
		if (vizMgr->getDvrParams(winnum, i)->isEnabled() && 
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
	copyCombo->setCurrentItem(0);
	performGuiCopyInstanceToViz(viznum);
}

void DvrEventRouter::
setDvrTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void DvrEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::DvrParamsType);
	
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
	
	DvrParams* dParams = vizMgr->getDvrParams(activeViz, instance);
	//Make sure this is a change:
	if (dParams->isEnabled() == val ) return;
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val,instance);
	typeCombo->setEnabled(!val);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(dParams,!val, false);
	
	setEditorDirty();
	updateTab();
}





void DvrEventRouter::
setDvrEditMode(bool mode){
	navigateButton->setOn(!mode);
	guiSetEditMode(mode);
}
void DvrEventRouter::
setDvrNavigateMode(bool mode){
	editButton->setOn(!mode);
	guiSetEditMode(!mode);
}

void DvrEventRouter::
refreshHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::DvrParamsType);
	//Refresh data range:
	//dParams->setDatarangeDirty();
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(dParams);
		setEditorDirty();
	}
}
/*
 * Respond to a slider release
 */
void DvrEventRouter::
dvrOpacityScale() {
	guiSetOpacityScale(
		opacityScaleSlider->value());
}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void DvrEventRouter::
dvrSaveTF(void){
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::DvrParamsType);
	SaveTFDialog* saveTFDialog = new SaveTFDialog(dParams,this,
		"Save TF Dialog", true);
	int rc = saveTFDialog->exec();
	if (rc == 1) fileSaveTF(dParams);
}
void DvrEventRouter::
fileSaveTF(DvrParams* dParams){
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
void DvrEventRouter::
dvrLoadTF(void){
	//If there are no TF's currently in Session, just launch file load dialog.
	DvrParams* dParams = (DvrParams*)VizWinMgr::getInstance()->getApplicableParams(Params::DvrParamsType);
	
	if (Session::getInstance()->getNumTFs() > 0){
		LoadTFDialog* loadTFDialog = new LoadTFDialog(this, this,
			"Load TF Dialog", true);
		int rc = loadTFDialog->exec();
		if (rc == 0) return;
		if (rc == 1) fileLoadTF(dParams);
		//if rc == 2, we already (probably) loaded a tf from the session
	} else {
		fileLoadTF(dParams);
	}
	setEditorDirty();
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
	
	std::string s(name->ascii());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getSessionVarNum();
	dParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	setEditorDirty();
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
void DvrEventRouter::
fileLoadTF(DvrParams* dParams){
	if (dParams->getNumVariables() <= 0) return;
	int varNum = dParams->getSessionVarNum();
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
	Session::getInstance()->updateTFFilePath(&s);
	setEditorDirty();
	setDatarangeDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}

//Insert values from params into tab panel
//
void DvrEventRouter::updateTab(){
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


	initTypes();
	DvrParams* dvrParams = (DvrParams*) VizWinMgr::getActiveDvrParams();
	
	
	
	
	deleteInstanceButton->setEnabled(vizMgr->getNumDvrInstances(winnum) > 1);
	
	QString strn;
    

	

    transferFunctionFrame->setMapperFunction(dvrParams->getMapperFunc());
    transferFunctionFrame->update();

    if (session->getNumSessionVariables())
    {
      int varnum = dvrParams->getSessionVarNum();
      const std::string& varname = session->getVariableName(varnum);
      
      transferFunctionFrame->setVariableName(varname);
    }
    else
    {
      transferFunctionFrame->setVariableName("");
    }
	//Force the dvr to refresh
	VizWinMgr::getInstance()->setClutDirty(dvrParams);
    VizWinMgr::getInstance()->setDatarangeDirty(dvrParams);
	VizWinMgr::getInstance()->setVizDirty(dvrParams,DvrRegionBit,true, true);

	
	//Disable the typeCombo whenever the renderer is enabled:
	typeCombo->setEnabled(!(dvrParams->isEnabled()));
	typeCombo->setCurrentItem(typemapi[dvrParams->getType()]);

	int numRefs = dvrParams->getNumRefinements();
	if(numRefs <= refinementCombo->count())
		refinementCombo->setCurrentItem(numRefs);
	variableCombo->setCurrentItem(dvrParams->getComboVarNum());
	
	lightingCheckbox->setChecked(dvrParams->getLighting());
	preintegratedCheckbox->setChecked(dvrParams->getPreIntegration());

	numBitsSpin->setValue(dvrParams->getNumBits());
	histoScaleEdit->setText(QString::number(dvrParams->getHistoStretch()));
	

	updateMapBounds(dvrParams);
	
	float sliderVal = dvrParams->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	
	sliderVal = 256.f*(1.f -sliderVal);
	opacityScaleSlider->setValue((int) sliderVal);
	
	
	//Set the mode buttons:
	
	if (dvrParams->getEditMode()){
		
		editButton->setOn(true);
		navigateButton->setOn(false);
	} else {
		editButton->setOn(false);
		navigateButton->setOn(true);
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
guiSetNumRefinements(int num){
	confirmText(false);
	//make sure we are setting it to a valid number
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();
	int newNum = dParams->checkNumRefinements(num);
	if (newNum == dParams->getNumRefinements()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set number of refinements");
		
	dParams->setNumRefinements(newNum);
	if (newNum != num) refinementCombo->setCurrentItem(newNum);
	PanelCommand::captureEnd(cmd, dParams);
	VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
}
void DvrEventRouter::
guiSetEnabled(bool value, int instance){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	int winnum = vizWinMgr->getActiveViz();
	//Ignore spurious clicks.
	DvrParams* dParams = vizWinMgr->getDvrParams(winnum, instance);
	
	if (value == dParams->isEnabled()) return;
	confirmText(false);
	//On enable, disable any existing dvr renderer in this window:
	if (value){
		
		VizWin* viz = vizWinMgr->getActiveVisualizer();
		DvrParams* prevParams = (DvrParams*)viz->getGLWindow()->findARenderer(Params::DvrParamsType);
		assert(prevParams != dParams);
		if (prevParams){
			
			int prevInstance = vizWinMgr->findInstanceIndex(winnum, prevParams, Params::DvrParamsType);
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
		
	setDatarangeDirty(dParams);
	setEditorDirty();
	vizWinMgr->setClutDirty(dParams);
	vizWinMgr->setVizDirty(dParams,DvrRegionBit,true);

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
	
	
	
	dParams->setVarNum(Session::getInstance()->mapMetadataToSessionVarNum(val));
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
    typeCombo->insertItem("Volumizer", index);
    typemap[index] = DvrParams::DVR_VOLUMIZER;
    typemapi[DvrParams::DVR_VOLUMIZER] = index;
    index++;
  }
	if (VolumeRenderer::supported(DvrParams::DVR_TEXTURE3D_SHADER))
  {
    typeCombo->insertItem("3DTexture-Shader", index);
    typemap[index] = DvrParams::DVR_TEXTURE3D_SHADER;
    typemapi[DvrParams::DVR_TEXTURE3D_SHADER] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_TEXTURE3D_LOOKUP))
  {
    typeCombo->insertItem("3DTexture", index);
    typemap[index] = DvrParams::DVR_TEXTURE3D_LOOKUP;
    typemapi[DvrParams::DVR_TEXTURE3D_LOOKUP] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_SPHERICAL_SHADER))
  {
    typeCombo->insertItem("Spherical-Shader", index);
    typemap[index] = DvrParams::DVR_SPHERICAL_SHADER;
    typemapi[DvrParams::DVR_SPHERICAL_SHADER] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_DEBUG))
  {
    typeCombo->insertItem("Debug", index);
    typemap[index] = DvrParams::DVR_DEBUG;
    typemapi[DvrParams::DVR_DEBUG] = index;
    index++;
  }

  if (VolumeRenderer::supported(DvrParams::DVR_STRETCHED_GRID))
  {
    typeCombo->insertItem("Stretched Grid", index);
    typemap[index++] = DvrParams::DVR_STRETCHED_GRID;
    typemapi[DvrParams::DVR_STRETCHED_GRID] = index;
    index++;
  }
  
  typeCombo->setCurrentItem(0);
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
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "set dvr number bits");
		
	dParams->setNumBits(val);
	PanelCommand::captureEnd(cmd, dParams);
		
	
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

//Respond to a change in histogram stretch factor
void DvrEventRouter::
guiSetOpacityScale(int val){
	DvrParams* dParams = VizWinMgr::getActiveDvrParams();

	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "modify opacity scale slider");
		
	dParams->setOpacityScale(((float)(256-val))/256.f);
	float sliderVal = dParams->getOpacityScale();
	QToolTip::add(opacityScaleSlider,"Opacity Scale Value = "+QString::number(sliderVal*sliderVal));
	
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
	
	savedCommand = PanelCommand::captureStart(dParams, qstr.latin1());
		
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
                SLOT(message(const QString &)));

		
		viz->getGLWindow()->appendRenderer(dParams, myDvr);

		//force the renderer to refresh region data  (why?)
		
		VizWinMgr::getInstance()->setVizDirty(dParams,DvrRegionBit,true);
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

//Methods to support maintaining a list of histograms
//in each params (at least those with a TFE)
//Initially just revert to static methods on Histo:

//Replace with versions from probe:
//Obtain the current valid histogram.  if mustGet is false, don't build a new one.
Histo* DvrEventRouter::getHistogram(RenderParams* p, bool mustGet){
	DvrParams* dParams = (DvrParams*)p;
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	int varNum = dParams->getSessionVarNum();
	//Make sure we are using a valid variable num
	if (varNum >= numHistograms || !histogramList){
		
		if (!mustGet) return 0;
		histogramList = new Histo*[numVariables];
		numHistograms = numVariables;
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	
	const float* currentDatarange = dParams->getCurrentDatarange();
	if (histogramList[varNum]) {
		
		return histogramList[varNum];
	}
	
	if (!mustGet) return 0;
	histogramList[varNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	
	refreshHistogram(dParams);
	return histogramList[varNum];
	
}
void DvrEventRouter::refreshHistogram(RenderParams* p){
	size_t min_dim[3],max_dim[3];
	size_t min_bdim[3], max_bdim[3];
	DvrParams* dParams = (DvrParams*)p;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	DataMgr* dataMgr = Session::getInstance()->getDataMgr();
	if(!dataMgr) return;
	RegionParams* rParams;
	int vizNum = p->getVizNum();
	if (vizNum<0) {
		//It's possible that multiple different region params apply to
		//different dvr panels if they are shared.  But the current active
		//region params will apply here.
		rParams = vizWinMgr->getActiveRegionParams();
	}
	else rParams = vizWinMgr->getRegionParams(vizNum);
	size_t fullHeight = rParams->getFullGridHeight();
	int varNum = dParams->getSessionVarNum();
	
	if (!DataStatus::getInstance()->getDataMgr()) return;
	int numVariables = DataStatus::getInstance()->getNumSessionVariables();
	if (!histogramList){
		histogramList = new Histo*[numVariables];
		numHistograms = numVariables;
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	if (histogramList[varNum]){
		delete histogramList[varNum];
		histogramList[varNum] = 0;
	}
	int numTrans = dParams->getNumRefinements();
	int timeStep = vizWinMgr->getAnimationParams(vizNum)->getCurrentFrameNumber();
	float dataMin = dParams->getMinOpacMapBound();
	float dataMax = dParams->getMaxOpacMapBound();

	bool dataValid = rParams->getAvailableVoxelCoords(numTrans, min_dim, max_dim, min_bdim, max_bdim, timeStep, &varNum, 1);
	if(!dataValid){
		MessageReporter::warningMsg("Histogram data unavailable for refinement %d at timestep %d", numTrans, timeStep);
		return;
	}
	
	//Check if the region/resolution is too big:
	  int numMBs = RegionParams::getMBStorageNeeded(rParams->getRegionMin(), rParams->getRegionMax(), numTrans);
	  int cacheSize = DataStatus::getInstance()->getCacheMB();
	  if (numMBs > (int)(0.75*cacheSize)){
		  MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too small for current region and resolution.\n%s \n%s",
			  "Lower the refinement level, reduce the region size, or increase the cache size.",
			  "Rendering has been disabled.");
		  dParams->setEnabled(false);
		  updateTab();
		  return;
	  }
	//const Metadata* metaData = Session::getInstance()->getCurrentMetadata();
	//Now get the data:
	char outval = (char)0;
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	unsigned char* data = (unsigned char*) dataMgr->GetRegionUInt8(
		timeStep, (const char*)DataStatus::getInstance()->getVariableName(varNum).c_str(),
					numTrans,
					min_bdim, max_bdim,
					fullHeight,
					dParams->getCurrentDatarange(),
					outval,
					0 //Don't lock!
		);
	QApplication::restoreOverrideCursor();
	//Make sure we can build a histogram
	if (!data) {
		MessageReporter::errorMsg("Invalid/nonexistent data cannot be histogrammed");
		return;
	}

	histogramList[varNum] = new Histo(data,
			min_dim, max_dim, min_bdim, max_bdim, dataMin, dataMax);
	setEditorDirty();
	update();
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
makeCurrent(Params* prevParams, Params* newParams, bool newWin, int instance) {
	assert(instance >= 0);
	DvrParams* dParams = (DvrParams*)(newParams->deepCopy());
	int vizNum = dParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(VizWinMgr::getInstance()->getNumDvrInstances(vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, dParams, Params::DvrParamsType, instance);
	//if (dParams->getMapperFunc())dParams->getMapperFunc()->setParams(dParams);
	setEditorDirty(dParams);
	updateTab();
	DvrParams* formerParams = (DvrParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled and/or Local settings changed:
	if (newWin || (formerParams->isEnabled() != dParams->isEnabled())){
		updateRenderer(dParams, wasEnabled,  newWin);
	}

	setDatarangeDirty(dParams);
	updateClut(dParams);
    setEditorDirty();
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
  benchmarkTimer->start(0, FALSE);
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

      if (renderCheck->isOn())
      {
        renderer->resetTimer();
        break;
      }
      else
      {
        benchmark++;
      }
      
    case TEMPORAL:

      if(animationCheck->isOn())
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
      
      if (tfCheck->isOn())
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
  VizWinMgr *vizmgr = VizWinMgr::getInstance();

  size_t max_dim[3];
  size_t min_dim[3];
  size_t max_bdim[3];
  size_t min_bdim[3];

  const Metadata *metadata      = Session::getInstance()->getCurrentMetadata();
  RegionParams    *regionParams = vizmgr->getActiveRegionParams();
  DvrParams       *dvrParams    = vizmgr->getActiveDvrParams();

  const size_t *bs = metadata->GetBlockSize();
  int timeStep     = vizmgr->getActiveAnimationParams()->getCurrentFrameNumber();
  int varNum       = dvrParams->getSessionVarNum();
  int numxforms    = dvrParams->getNumRefinements();

  regionParams->getAvailableVoxelCoords(numxforms, 
                                        min_dim, max_dim, 
                                        min_bdim, max_bdim, 
                                        timeStep, &varNum, 1);
  
  int nx = (max_bdim[0] - min_bdim[0] + 1) * bs[0];
  int ny = (max_bdim[1] - min_bdim[1] + 1) * bs[1];
  int nz = (max_bdim[2] - min_bdim[2] + 1) * bs[2];

  VizWin *vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
  if (!vizWin) return;

  cout << " " << typeCombo->currentText() << " @ ";
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
  transferFunctionFrame->update();
}
	

