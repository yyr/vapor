//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizwinmgr.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Sept 2004
//
//	Description:  Implementation of VizWinMgr class	
//		This class manages the VizWin visualizers
//		Its main function is to catch events from the visualizers and
//		to route them to the appropriate params class, and in reverse,
//		to route events from tab panels to the appropriate visualizer.
//
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

#include "vizwin.h"
#include "vizwinmgr.h"
#include "vizmgrdialog.h"
#include "math.h"
#include "mainform.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "viztab.h"
#include "regiontab.h"
#include "tabmanager.h"
#include "dvrparams.h"
#include "contourparams.h"
#include "dvr.h"
#include "contourplanetab.h"
#include "isosurfaceparams.h"
#include "isotab.h"
#include "assert.h"
#include "trackball.h"
#include "glbox.h"
#include "vizactivatecommand.h"
#include "session.h"

#ifdef VOLUMIZER
#include "volumizerrenderer.h"
#endif
using namespace VAPoR;
/******************************************************************
 *  Constructor sets up an array of MAXVIZWINS windows
 *
 *****************************************************************/
VizWinMgr::VizWinMgr(MainForm* mainWindow) 
{
    myMainWindow = mainWindow;
    myWorkspace = mainWindow->getWorkspace();
    tabManager = mainWindow->getTabManager();
    currentVizMgrDialog = 0;
	activeViz = -1;
	activationCount = 0;
    for (int i = 0; i< MAXVIZWINS; i++){
        vizWin[i] = 0;
        vizName[i] = 0;
        vizRect[i] = 0;
        isMax[i] = false;
        isMin[i] = false;
		vpParams[i] = 0;
		rgParams[i] = 0;
		dvrParams[i] = 0;
		isoParams[i] = 0;
		contourParams[i] = 0;
		activationOrder[i] = -1;
    }
	//Create a default (global) parameter set
	globalVPParams = new ViewpointParams(myMainWindow, -1);
	
	globalRegionParams = new RegionParams(myMainWindow, -1);
	
	globalDvrParams = new DvrParams(myMainWindow, -1);
	
	globalIsoParams = new IsosurfaceParams(myMainWindow, -1);
	
	globalContourParams = new ContourParams(myMainWindow, -1);
	

	globalTrackball = new Trackball();
	//Initialize in selection mode
	setSelectionMode(Command::navigateMode);
}

/***********************************************************************
 *  Destroys the object and frees any allocated resources
 ***********************************************************************/
VizWinMgr::~VizWinMgr()
{
	//qWarning("vizwinmgr destructor start");
    for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			delete vizWin[i];
		}
	}
	
	
}
void VizWinMgr::
vizAboutToDisappear(int i)  {
    //qWarning("Viz %d about to disappear", i);
    if (vizWin[i] == 0) return;

	

	//Count how many vizWins are left
	int numVizWins = 0;
	for (int j = 0; j<MAXVIZWINS; j++){
		if ((j != i) && vizWin[j]) {
			numVizWins++;
		}
	}
	vizWin[i] = 0;
	//If we are deleting the active viz, revert to the 
	//most recent active viz.
	if (activeViz == i) activeViz = getLastActive();
	
    
    delete vizRect[i];
    vizRect[i] = 0;
    //getLastActive will become the new active Viz.
	if (activeViz == i) activeViz = getLastActive();
	//Save the state in history before deleting the params,
	//the next activeViz is the most recent active visualizer 
	if (myMainWindow->getSession()->isRecording()){
		myMainWindow->getSession()->addToHistory(new VizActivateCommand(
			i, activeViz, Command::remove, myMainWindow->getSession()));
	}

	if(vpParams[i]) delete vpParams[i];
	if(rgParams[i]) delete rgParams[i];
	
	if (dvrParams[i])delete dvrParams[i];
	if(isoParams[i]) delete isoParams[i];
	if(contourParams[i]) delete contourParams[i];
	vpParams[i] = 0;
	rgParams[i] = 0;
	dvrParams[i] = 0;
	isoParams[i] = 0;
	contourParams[i] = 0;
    
	emit (removeViz(i));
	
	
	//When the number is reduced to 1, disable multiviz options.
	if (numVizWins == 1) emit enableMultiViz(false);
	
	//Don't delete vizName, it's handled by ref counts
    
    vizName[i] = 0;
}
//When a visualizer is launched, we may optionally specify a number and a name
//if this is a relaunch of a previously created visualizer.  If newWindowNum == -1,
//this is a brand new visualizer.  Otherwise it's a relaunch.
void VizWinMgr::
launchVisualizer(int newWindowNum, const char* newName)
{
	//launch to right of tab dialog, maximize
	//Look for first unused pointer, also count how many viz windows exist
	int prevActiveViz = activeViz;
	bool brandNew = true;
	int numVizWins = 0;
	if (newWindowNum != -1) {
		assert(!vizWin[newWindowNum]);
		brandNew = false;
	}
	//, count visualizers, find what to use for a new window num
	for (int i = 0; i< MAXVIZWINS; i++){
		
		if (!vizWin[i]) {
			//Find first unused position
			if(newWindowNum == -1 && brandNew) newWindowNum = i;
		} else {
			numVizWins++;
		}
		
		if (i == MAXVIZWINS -1 && newWindowNum == -1) {
			QMessageBox::information((QWidget*)myMainWindow, "VAPoR", "Unable to create additional visualizers");
			return;
		}	
	}
	
	numVizWins++;

	if (brandNew) createDefaultRendererPanels(newWindowNum);
		
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here:  
    myMainWindow->getSession()->blockRecording();
	//Default name is just "Visualizer No. N"
    //qWarning("Creating new visualizer in position %d",newWindowNum);
	if (strlen(newName) != 0) vizName[newWindowNum] = new QString(newName);
	else vizName[newWindowNum] = new QString((QString("Visualizer No. ")+QString::number(newWindowNum)));
	emit (newViz(*vizName[newWindowNum], newWindowNum));
	vizWin[newWindowNum] = new VizWin (myWorkspace, vizName[newWindowNum]->ascii(), 0/*Qt::WType_TopLevel*/, this, newRect, newWindowNum);
	vizWin[newWindowNum]->setWindowNum(newWindowNum);
	//qWarning("Showing new visualizer in position %d",newWindowNum);

	vizWin[newWindowNum]->setFocusPolicy(QWidget::ClickFocus);

	vizWin[newWindowNum]->showMaximized();
	maximize(newWindowNum);
	vizWin[newWindowNum]->show();
	
	//qWarning("New window corner %d %d size %d %d", newRect->x(), newRect->y(), newRect->width(), newRect->height());
	vizRect[newWindowNum] = newRect;
	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numVizWins > 1){
		emit enableMultiViz(true);
	}
	myMainWindow->getSession()->unblockRecording();
	myMainWindow->getSession()->addToHistory(new VizActivateCommand(
		prevActiveViz, newWindowNum, Command::create, myMainWindow->getSession()));
		
}
/*
 *  Create renderers, for a new visualizer:
 */
void VizWinMgr::
createDefaultRendererPanels(int winnum){
	dvrParams[winnum] = (DvrParams*)globalDvrParams->deepCopy();
	dvrParams[winnum]->setVizNum(winnum);
	dvrParams[winnum]->setLocal(true);
	dvrParams[winnum]->setEnabled(false);
	

	isoParams[winnum] = (IsosurfaceParams*)globalIsoParams->deepCopy();
	isoParams[winnum]->setVizNum(winnum);
	isoParams[winnum]->setLocal(true);
	isoParams[winnum]->setEnabled(false);

	contourParams[winnum] = (ContourParams*)globalContourParams->deepCopy();
	contourParams[winnum]->setVizNum(winnum);
	contourParams[winnum]->setLocal(true);
	contourParams[winnum]->setEnabled(false);
}
void VizWinMgr::
closeEvent()
{
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]) {
			vizWin[i]->close();
			//qWarning("closing viz window %d", i);
			delete vizRect[i];
			
		}
	}
}

/***********************************************************************
 * The following called by the window when its (max,min,norm) status is updated
 **************************************************************************/
void VizWinMgr::
comboChanged(int newSetting, int vizNum)
{
	//qWarning("checking combo");
	if (currentVizMgrDialog){
		//qWarning("setting combo");
		currentVizMgrDialog->setCombo(newSetting, vizNum);
	}
}

/**************************************************************
 * Methods that arrange the viz windows:
 **************************************************************/
void 
VizWinMgr::cascade(){
	myMainWindow->getSession()->blockRecording();
    myWorkspace->cascade();
	//Now size them up to a reasonable size:
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			vizWin[i]->resize(400,400);
		}	
	} 
	myMainWindow->getSession()->unblockRecording();
}
void 
VizWinMgr::coverRight(){
	myMainWindow->getSession()->blockRecording();
    for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			vizWin[i]->showMaximized();
			maximize(i);
		}
	} 
	myMainWindow->getSession()->unblockRecording();
}

void 
VizWinMgr::fitSpace(){
	myMainWindow->getSession()->blockRecording();
    myWorkspace->tile();
	myMainWindow->getSession()->unblockRecording();
}


int VizWinMgr::
getNumVisualizers(){
    int num = 0;
    for (int i = 0; i< MAXVIZWINS; i++){
        if (vizWin[i]) num++;
    }
    return num;
}
void VizWinMgr::
setVizWinName(int winNum, QString qs) {
		vizName[winNum] = &qs;
		vizWin[winNum]->setCaption(qs);
		nameChanged(qs, winNum);
}

/**********************************************************************
 * Method called when the user makes a visualizer active:
 **********************************************************************/
void VizWinMgr::
setActiveViz(int vizNum){
	if (activeViz != vizNum){
		int prevActiveViz = activeViz;
		emit(activateViz(vizNum));
		
		activeViz = vizNum;
		activationOrder[vizNum]= (++activationCount);
		//Determine if the local viewpoint dialog applies, update the tab dialog
		//appropriately
		
		getViewpointParams(activeViz)->updateDialog();
		getRegionParams(activeViz)->updateDialog();
		getDvrParams(activeViz)->updateDialog();
		getContourParams(activeViz)->updateDialog();
		getIsoParams(activeViz)->updateDialog();

		tabManager->show();
		//Add to history if this is not during initial creation.
		if (prevActiveViz >= 0){
			myMainWindow->getSession()->addToHistory(new VizActivateCommand(
				prevActiveViz, vizNum, Command::activate, myMainWindow->getSession()));
		}
	}
}
//Method to enable closing of a vizWin for Undo/Redo
void VizWinMgr::
killViz(int viznum){
	vizWin[viznum]->close();
	vizWin[viznum] = 0;
}
/*
 *  Methods for changing the parameter panels.
 */
void VizWinMgr::
setContourParams(int winnum, ContourParams* p){
	if (winnum < 0) { //global params!
		globalContourParams = p;
	} else {
		contourParams[winnum] = p;
	}
}
void VizWinMgr::
setIsoParams(int winnum, IsosurfaceParams* p){
	if (winnum < 0) { //global params!
		globalIsoParams = p;
	} else {
		isoParams[winnum] = p;
	}
}
void VizWinMgr::
setDvrParams(int winnum, DvrParams* p){
	if (winnum < 0) { //global params!
		globalDvrParams = p;
	} else {
		dvrParams[winnum] = p;
	}
}
void VizWinMgr::
setRegionParams(int winnum, RegionParams* p){
	if (winnum < 0) { //global params!
		globalRegionParams = p;
	} else {
		rgParams[winnum] = p;
	}
}
void VizWinMgr::
setViewpointParams(int winnum, ViewpointParams* p){
	if (winnum < 0) { //global params!
		globalVPParams = p;
	} else {
		vpParams[winnum] = p;
	}
}

/**********************************************************
 * Whenever a new tab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpVizTab(VizTab* vTab)
{
	//Signals and slots:
	
 	connect (vTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setVpLocalGlobal(int)));
	connect (vTab->perspectiveCombo, SIGNAL (activated(int)), this, SLOT (setVPPerspective(int)));
	connect (vTab->numLights, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->camPos0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->camPos1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->camPos2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->viewDir0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->viewDir1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->viewDir2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->upVec0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->upVec1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->upVec2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (vTab->camPos0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->camPos1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->camPos2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->viewDir0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->viewDir1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->viewDir2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->upVec0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->upVec1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->upVec2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->numLights, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));

	connect (this, SIGNAL(enableMultiViz(bool)), vTab->LocalGlobal, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), vTab->copyToButton, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), vTab->copyTargetCombo, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
}
/**********************************************************
 * Whenever a new regiontab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpRegionTab(RegionTab* rTab)
{
	
	//Signals and slots:
	
	connect (rTab->strideSpin, SIGNAL( valueChanged(int) ), this, SLOT( setRegionStride(int) ) );
 	connect (rTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setRgLocalGlobal(int)));

	connect (rTab->xCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->yCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->zCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->xSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->ySizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->zSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->maxSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->timestepEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));

	connect (rTab->xCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->yCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->zCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->xSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->ySizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->zSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->maxSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->timestepEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));

	connect (rTab->xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionXCenter()));
	connect (rTab->yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionYCenter()));
	connect (rTab->zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionZCenter()));
	connect (rTab->xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionXSize()));
	connect (rTab->ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionYSize()));
	connect (rTab->zSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionZSize()));
	connect (rTab->maxSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionMaxSize()));
	
	connect (this, SIGNAL(enableMultiViz(bool)), rTab->LocalGlobal, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), rTab->copyToButton, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), rTab->copyTargetCombo, SLOT(setEnabled(bool)));
	connect (rTab->timestepSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionTimestep()));
	
	emit enableMultiViz(getNumVisualizers() > 1);
}
void
VizWinMgr::hookUpDvrTab(Dvr* dvrTab)
{
	myDvrTab = dvrTab;
	connect (dvrTab->EnableDisable, SIGNAL(activated(int)), this, SLOT(setDvrEnabled(int)));
	connect (dvrTab->variableCombo, SIGNAL( activated(int) ), this, SLOT( setDvrVariableNum(int) ) );
	connect (dvrTab->lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( setDvrLighting(bool) ) );
 
	connect (dvrTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setDvrLocalGlobal(int)));
	connect (dvrTab->specularShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));
	connect (dvrTab->diffuseShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));
	connect (dvrTab->ambientShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));
	connect (dvrTab->exponentShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));
	connect (dvrTab->specularAttenuation, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));
	connect (dvrTab->diffuseAttenuation, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));
	connect (dvrTab->ambientAttenuation, SIGNAL( textChanged(const QString&) ), this, SLOT( setDvrTabTextChanged(const QString&)));

	connect (dvrTab->leftMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (dvrTab->rightMappingBound, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (dvrTab->editCenter, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (dvrTab->editSize, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (dvrTab->tfDomainSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));
	connect (dvrTab->tfDomainCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setDvrTabTextChanged(const QString&)));

	connect (dvrTab->specularShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
	connect (dvrTab->diffuseShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->ambientShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->exponentShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->specularAttenuation, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->diffuseAttenuation, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->ambientAttenuation, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
	connect (dvrTab->tfDomainSizeEdit, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (dvrTab->tfDomainCenterEdit, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));

	connect (dvrTab->numBitsSpin, SIGNAL (valueChanged(int)), this, SLOT(setDvrNumBits(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->LocalGlobal, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->copyToButton, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->copyTargetCombo, SLOT(setEnabled(bool)));
	//TFE Editor controls:
	connect (dvrTab->leftMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (dvrTab->rightMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (dvrTab->editCenter, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (dvrTab->editSize, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (dvrTab->editCenterSlider, SIGNAL(sliderPressed()), this, SLOT(beginTFECenterSlide()));
	connect (dvrTab->editCenterSlider, SIGNAL(sliderReleased()), this, SLOT (endTFECenterSlide()));
	connect (dvrTab->editCenterSlider, SIGNAL(sliderMoved(int)), this, SLOT(moveTFECenter(int)));
	connect (dvrTab->editSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setTFESize()));

	connect (dvrTab->tfDomainCenterSlider, SIGNAL(sliderPressed()), this, SLOT(beginTFDomainCenterSlide()));
	connect (dvrTab->tfDomainCenterSlider, SIGNAL(sliderReleased()), this, SLOT (endTFDomainCenterSlide()));
	connect (dvrTab->tfDomainCenterSlider, SIGNAL(sliderMoved(int)), this, SLOT(moveTFDomainCenter(int)));
	connect (dvrTab->tfDomainSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setTFDomainSize()));
	
	connect (dvrTab->histoStretchSlider, SIGNAL(sliderReleased()), this, SLOT (dvrHistoStretch()));
	connect (dvrTab->ColorBindButton, SIGNAL(pressed()), this, SLOT(dvrColorBind()));
	connect (dvrTab->OpacityBindButton, SIGNAL(pressed()), this, SLOT(dvrOpacBind()));
	emit enableMultiViz(getNumVisualizers() > 1);
}
void
VizWinMgr::hookUpContourTab(ContourPlaneTab* cptTab)
{
	myContourPlaneTab = cptTab;
	connect (cptTab->EnableDisable, SIGNAL(activated(int)), this, SLOT(setContourEnabled(int)));
	connect (cptTab->variableCombo1, SIGNAL( activated(int) ), this, SLOT( setContourVariableNum(int) ) );
	connect (cptTab->lightingCheckbox, SIGNAL( toggled(bool) ), this, SLOT( setContourLighting(bool) ) );
 
	connect (cptTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setContourLocalGlobal(int)));
	connect (cptTab->specularShading, SIGNAL( returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->diffuseShading, SIGNAL( returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->ambientShading, SIGNAL( returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->exponentShading, SIGNAL( returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->normalx, SIGNAL(returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->normaly, SIGNAL(returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->normalz, SIGNAL(returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->pointx, SIGNAL(returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->pointy, SIGNAL(returnPressed()), this, SLOT( contourReturnPressed() ) );
	connect (cptTab->pointz, SIGNAL(returnPressed()), this, SLOT( contourReturnPressed() ) );

	connect (cptTab->specularShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->diffuseShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->ambientShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->exponentShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->normalx, SIGNAL(textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->normaly, SIGNAL(textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->normalz, SIGNAL(textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->pointx, SIGNAL(textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->pointy, SIGNAL(textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));
	connect (cptTab->pointz, SIGNAL(textChanged(const QString&) ), this, SLOT( setContourTabTextChanged(const QString&)));

	connect (this, SIGNAL(enableMultiViz(bool)), cptTab->LocalGlobal, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), cptTab->copyToButton, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), cptTab->copyTargetCombo, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);

}
void
VizWinMgr::hookUpIsoTab(IsoTab* isoTab)
{
	connect (isoTab->EnableDisable, SIGNAL(activated(int)), this, SLOT(setIsoEnabled(int)));
	connect (isoTab->variableCombo1, SIGNAL( activated(int) ), this, SLOT( setIsoVariable1Num(int) ) );
	connect (isoTab->colorButton1, SIGNAL(clicked()), this, SLOT(setIsoColor1()));
	
	connect (isoTab->opacSlider1, SIGNAL(sliderReleased()), this, SLOT (setIsoOpac1()));
	connect (isoTab->valueSlider1, SIGNAL(sliderReleased()), this, SLOT (setIsoValue1()));
	
	connect (isoTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setIsoLocalGlobal(int)));

	connect (isoTab->opacEdit1, SIGNAL( textChanged(const QString&) ), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (isoTab->valueEdit1, SIGNAL( textChanged(const QString&) ), this, SLOT(setIsoTabTextChanged(const QString&)));
	connect (isoTab->specularShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setIsoTabTextChanged(const QString&) ) );
	connect (isoTab->diffuseShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setIsoTabTextChanged(const QString&) ) );
	connect (isoTab->ambientShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setIsoTabTextChanged(const QString&) ) );
	connect (isoTab->exponentShading, SIGNAL( textChanged(const QString&) ), this, SLOT( setIsoTabTextChanged(const QString&) ) );
	
	connect (isoTab->valueEdit1, SIGNAL( returnPressed()), this, SLOT(isoReturnPressed()));
	connect (isoTab->opacEdit1, SIGNAL( returnPressed()), this, SLOT(isoReturnPressed()));
	connect (isoTab->specularShading, SIGNAL( returnPressed() ), this, SLOT(isoReturnPressed()));
	connect (isoTab->diffuseShading, SIGNAL( returnPressed() ), this, SLOT(isoReturnPressed()));
	connect (isoTab->ambientShading, SIGNAL( returnPressed() ), this, SLOT(isoReturnPressed()));
	connect (isoTab->exponentShading, SIGNAL( returnPressed() ), this, SLOT(isoReturnPressed()));

	connect (this, SIGNAL(enableMultiViz(bool)), isoTab->LocalGlobal, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), isoTab->copyButton, SLOT(setEnabled(bool)));
	connect (this, SIGNAL(enableMultiViz(bool)), isoTab->copyTarget, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
}
/*
 * Tell the parameter panels when there are or are not multiple viz's
 */

/********************************************************************
 *					SLOTS
 ********************************************************************/
/*
 * Slot that responds to user setting of vizselectcombo
 */
void VizWinMgr::
winActivated(int winNum){
	//Truly make specified visualizer active:
	
	vizWin[winNum]->setFocus();
	
	
}
//REspond to the name setting in the dialog
void VizWinMgr::
nameChanged(QString& name, int num){
	emit (changeName(name, num));
}
/*******************************************************************
 *	Slots associated with VizTab:
 ********************************************************************/

void VizWinMgr::
setVPPerspective(int isOn){
	getViewpointParams(activeViz)->guiSetPerspective(isOn);
	//Immediately force update:
	getViewpointParams(activeViz)->updateRenderer(false, false, false);
}

void VizWinMgr::
setVtabTextChanged(const QString& ){
	getViewpointParams(activeViz)->guiSetTextChanged(true);
}


/* 
 * Following message is sent whenever user presses "return" on a textbox.
 * The changes are then sent to the visualizer:
 */
void VizWinMgr::
viewpointReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	getViewpointParams(activeViz)->confirmText(true);
}
void VizWinMgr::
home()
{
	getViewpointParams(activeViz)->useHomeViewpoint();
}
void VizWinMgr::
sethome()
{
	getViewpointParams(activeViz)->setHomeViewpoint();
}
/*********************************************************************************
 * Slots associated with RegionTab:
 *********************************************************************************/
void VizWinMgr::
setRegionTabTextChanged(const QString& ){
	getRegionParams(activeViz)->guiSetTextChanged(true);
}
void VizWinMgr::
regionReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	getRegionParams(activeViz)->confirmText(true);
}
void VizWinMgr::
setRegionStride(int str){
	//Dispatch the signal to the current active region parameter panel, or to the
	//global panel:
	getRegionParams(activeViz)->guiSetStride(str);
	
}
/* 
 * Respond to a release of the max size slider
 *
 */
void VizWinMgr::
setRegionMaxSize(){
	getRegionParams(activeViz)->guiSetMaxSize(
		myMainWindow->getRegionTab()->maxSizeSlider->value());
}



/*
 * Respond to a release of slider 
 */
void VizWinMgr::
setRegionXCenter(){
	getRegionParams(activeViz)->guiSetXCenter(
		myMainWindow->getRegionTab()->xCenterSlider->value());
}
void VizWinMgr::
setRegionYCenter(){
	getRegionParams(activeViz)->guiSetYCenter(
		myMainWindow->getRegionTab()->yCenterSlider->value());
}
void VizWinMgr::
setRegionZCenter(){
	getRegionParams(activeViz)->guiSetZCenter(
		myMainWindow->getRegionTab()->zCenterSlider->value());
}

/*
 * Respond to a slider release
 */
void VizWinMgr::
setRegionXSize(){
	getRegionParams(activeViz)->guiSetXSize(
		myMainWindow->getRegionTab()->xSizeSlider->value());
}
void VizWinMgr::
setRegionYSize(){
	getRegionParams(activeViz)->guiSetYSize(
		myMainWindow->getRegionTab()->ySizeSlider->value());
}
void VizWinMgr::
setRegionZSize(){
	getRegionParams(activeViz)->guiSetZSize(
		myMainWindow->getRegionTab()->zSizeSlider->value());
}

/*
 * Respond to a change in slider position
 */
void VizWinMgr::
setRegionTimestep(){
	getRegionParams(activeViz)->guiSetTimestep(
		myMainWindow->getRegionTab()->timestepSlider->value());
}

void VizWinMgr::
setRegionDirty(RegionParams* rParams){
	if (activeViz>=0) {
		vizWin[activeViz]->setRegionDirty(true);
		vizWin[activeViz]->updateGL();
	}
	//If another viz is using these region params, set their region dirty, too
	if (rParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != activeViz)  &&
				(rgParams[i] == rParams ||(!rgParams[i]))
			){
			vizWin[i]->setRegionDirty(true);
			vizWin[i]->updateGL();
		}
	}
}
void VizWinMgr::
setDvrDirty(DvrParams* dParams){
	if (activeViz>=0) {
		vizWin[activeViz]->updateGL();
	}
	if (dParams->isLocal()) return;
	//If another viz is using these dvr params, force them to update, too
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != activeViz) &&
				(dvrParams[i] == dParams ||(!dvrParams[i]))
			){
			vizWin[i]->updateGL();
		}
	}
}
/*****************************************************************************
 * Called when the local/global selector is changed.
 * Separate versions for viztab, regiontab, isotab, contourtab, dvrtab
 ******************************************************************************/
void VizWinMgr::
setVpLocalGlobal(int val){
	//If changes  to global, revert to global panel.
	//If changes to local, may need to create a new local panel
	//Switch local and global Trackball as appropriate
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		if(vpParams[activeViz])vpParams[activeViz]->guiSetLocal(false);
		globalVPParams->updateDialog();
		//Tell the active visualizer that it has global viewpoint
		vizWin[activeViz]->setGlobalViewpoint(true);
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!vpParams[activeViz]){
			//create a new parameter panel, copied from global
			vpParams[activeViz] = new ViewpointParams(*globalVPParams);
			vpParams[activeViz]->setVizNum(activeViz);
			vpParams[activeViz]->guiSetLocal(true);
			
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			vpParams[activeViz]->guiSetLocal(true);
			vpParams[activeViz]->updateDialog();
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->setGlobalViewpoint(false);
	}
}

/*****************************************************************************
 * Called when the region tab local/global selector is changed.
 * Affects only the front region tab
 ******************************************************************************/
void VizWinMgr::
setRgLocalGlobal(int val){
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		if(rgParams[activeViz])rgParams[activeViz]->guiSetLocal(false);
		globalRegionParams->updateDialog();
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!rgParams[activeViz]){
			//create a new parameter panel, copied from global
			rgParams[activeViz] = new RegionParams(*globalRegionParams);
			rgParams[activeViz]->setVizNum(activeViz);
			rgParams[activeViz]->guiSetLocal(true);
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			rgParams[activeViz]->guiSetLocal(true);
			rgParams[activeViz]->updateDialog();
			//and then refresh the panel:
			tabManager->show();
		}
	}
}
/*****************************************************************************
 * Called when the dvr tab local/global selector is changed.
 * Affects only the dvr tab
 ******************************************************************************/
void VizWinMgr::
setDvrLocalGlobal(int val){
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	bool wasEnabled = getDvrParams(activeViz)->isEnabled();
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
	
		dvrParams[activeViz]->guiSetLocal(false);
		globalDvrParams->updateDialog();
		tabManager->show();
		//Was there a change in enablement?
		
	} else { //Local: 
		//need to revert to existing local settings:
		dvrParams[activeViz]->guiSetLocal(true);
		dvrParams[activeViz]->updateDialog();
		//and then refresh the panel:
		tabManager->show();
	
	}
	getDvrParams(activeViz)->updateRenderer(wasEnabled,!val, false);
}
/*************************************************************************************
 * Other slots associated with DvrTab
 *************************************************************************************/
void VizWinMgr::
setDvrTabTextChanged(const QString& ){
	getDvrParams(activeViz)->guiSetTextChanged(true);
}
void VizWinMgr::
dvrReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	getDvrParams(activeViz)->confirmText(true);
}
void VizWinMgr::
setDvrEnabled(int val){

	//If there's no window, or no datamgr, ignore this.
	
	if ((activeViz < 0) || myMainWindow->getSession()->getDataMgr() == 0) {
		myDvrTab->EnableDisable->setCurrentItem(0);
		return;
	}
	getDvrParams(activeViz)->guiSetEnabled(val==1);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	getDvrParams(activeViz)->updateRenderer(!val, dvrParams[activeViz]->isLocal(), false);
}

void VizWinMgr::
setDvrVariableNum(int newVal){
	getDvrParams(activeViz)->guiSetVarNum(newVal);
}
void VizWinMgr::
setDvrNumBits(int newVal){
	getDvrParams(activeViz)->guiSetNumBits(newVal);
}
void VizWinMgr::
setDvrLighting(bool isOn){
	getDvrParams(activeViz)->guiSetLighting(isOn);
}
void VizWinMgr::
dvrColorBind(){
	getDvrParams(activeViz)->guiBindColorToOpac();
}
void VizWinMgr::
dvrOpacBind(){
	getDvrParams(activeViz)->guiBindOpacToColor();
}
/*
 * Respond to a slider release
 */
void VizWinMgr::
dvrHistoStretch() {
	getDvrParams(activeViz)->guiSetHistoStretch(
		myMainWindow->getDvrTab()->histoStretchSlider->value());
}
void VizWinMgr::
beginTFECenterSlide(){
	getDvrParams(activeViz)->guiStartTFECenterSlide();
}
void VizWinMgr::
endTFECenterSlide(){
	getDvrParams(activeViz)->guiEndTFECenterSlide(
		myMainWindow->getDvrTab()->editCenterSlider->value());
}
void VizWinMgr::
moveTFECenter(int val){
	getDvrParams(activeViz)->guiMoveTFECenter(val);
}
void VizWinMgr::
setTFESize(){
	getDvrParams(activeViz)->guiSetTFESize(
		myMainWindow->getDvrTab()->editSizeSlider->value());
}
void VizWinMgr::
beginTFDomainCenterSlide(){
	getDvrParams(activeViz)->guiStartTFDomainCenterSlide();
}
void VizWinMgr::
endTFDomainCenterSlide(){
	getDvrParams(activeViz)->guiEndTFDomainCenterSlide(
		myMainWindow->getDvrTab()->tfDomainCenterSlider->value());
}
void VizWinMgr::
moveTFDomainCenter(int val){
	getDvrParams(activeViz)->guiMoveTFDomainCenter(val);
}
void VizWinMgr::
setTFDomainSize(){
	getDvrParams(activeViz)->guiSetTFDomainSize(
		myMainWindow->getDvrTab()->tfDomainSizeSlider->value());
}
/*****************************************************************************
 * Called when the dvr tab local/global selector is changed.
 * Affects only the dvr tab
 ******************************************************************************/
void VizWinMgr::
setContourTabTextChanged(const QString& ){
	getContourParams(activeViz)->guiSetTextChanged(true);
}
void VizWinMgr::
contourReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	getContourParams(activeViz)->confirmText(true);
}
void VizWinMgr::
setContourLocalGlobal(int val){
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		contourParams[activeViz]->guiSetLocal(false);
		globalContourParams->updateDialog();
		tabManager->show();
	} else { //Local: 
		//need to revert to existing local settings:
		contourParams[activeViz]->guiSetLocal(true);
		contourParams[activeViz]->updateDialog();
		//and then refresh the panel:
		tabManager->show();
		
	}
}
/*************************************************************************************
 * Other slots associated with ContourTab
 *************************************************************************************/
void VizWinMgr::
setContourEnabled(int val){
	getContourParams(activeViz)->guiSetEnabled(val == 1);
}
void VizWinMgr::
setContourVariableNum(int newVal){
	getContourParams(activeViz)->guiSetVarNum(newVal);
}

void VizWinMgr::
setContourLighting(bool isOn){
	getContourParams(activeViz)->guiSetLighting(isOn);
}


/*****************************************************************************
 * Called when the iso tab local/global selector is changed.
 * Affects only the iso tab
 ******************************************************************************/
void VizWinMgr::
setIsoLocalGlobal(int val){
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	bool wasEnabled = getIsoParams(activeViz)->isEnabled();
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
	
		isoParams[activeViz]->guiSetLocal(false);
		globalIsoParams->updateDialog();
		tabManager->show();
		//Was there a change in enablement?
		
	} else { //Local: 
		//need to revert to existing local settings:
		isoParams[activeViz]->guiSetLocal(true);
		isoParams[activeViz]->updateDialog();
		//and then refresh the panel:
		tabManager->show();
	
	}
	getIsoParams(activeViz)->updateRenderer(wasEnabled,!val, false);
	
}
/*************************************************************************************
 * Other slots associated with IsoTab
 *************************************************************************************/
void VizWinMgr::
setIsoTabTextChanged(const QString&){
	getIsoParams(activeViz)->guiSetTextChanged(true);
}
//Respond to click return on any text box
void VizWinMgr::
isoReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	getIsoParams(activeViz)->confirmText(true);
}
void VizWinMgr::
setIsoEnabled(int val){
	//If there's no window, ignore this.
	if (activeViz < 0) return;
	getIsoParams(activeViz)->guiSetEnabled(val==1);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	getIsoParams(activeViz)->updateRenderer(!val, isoParams[activeViz]->isLocal(),false);
	
}
//Respond to change in variable combo
void VizWinMgr::
setIsoVariable1Num(int newVal){
	getIsoParams(activeViz)->guiSetVariable1(newVal);
}

/* 
 * This is called when opacity slider is released:
 */

/*
 * Respond to release of slider
 */
void VizWinMgr::
setIsoOpac1(){
	getIsoParams(activeViz)->guiSetOpac1(
		myMainWindow->getIsoTab()->opacSlider1->value());
		
}

void VizWinMgr::
setIsoValue1(){
	getIsoParams(activeViz)->guiSetValue1(
		myMainWindow->getIsoTab()->valueSlider1->value());
	
}

/*
 * Respond to user clicking the color button
 */
void VizWinMgr::
setIsoColor1(){
	IsoTab* theIsoTab = myMainWindow->getIsoTab();
	//Bring up a color selector dialog:
	QColor newColor = QColorDialog::getColor(theIsoTab->colorButton1->paletteBackgroundColor(), theIsoTab, "Isosurface Color Selection");
	//Set button color
	theIsoTab->colorButton1->setPaletteBackgroundColor(newColor);
	//Set parameter value of the appropriate parameter set:
	getIsoParams(activeViz)->guiSetColor1(new QColor(newColor));
}
ViewpointParams* VizWinMgr::
getViewpointParams(int winNum){
	if (winNum < 0) return globalVPParams;
	if (vpParams[winNum]&& vpParams[winNum]->isLocal()) return vpParams[winNum];
	return globalVPParams;
}
RegionParams* VizWinMgr::
getRegionParams(int winNum){
	if (winNum < 0) return globalRegionParams;
	if (rgParams[winNum]&&rgParams[winNum]->isLocal()) return rgParams[winNum];
	return globalRegionParams;
}


//For a renderer, there should always exist a local version.
DvrParams* VizWinMgr::
getDvrParams(int winNum){
	if (winNum < 0) return globalDvrParams;
	if (dvrParams[winNum]->isLocal()) return dvrParams[winNum];
	return globalDvrParams;
}
ContourParams* VizWinMgr::
getContourParams(int winNum){
	if (winNum < 0) return globalContourParams;
	if (contourParams[winNum]->isLocal()) return contourParams[winNum];
	return globalContourParams;
}
IsosurfaceParams* VizWinMgr::
getIsoParams(int winNum){
	if (winNum < 0) return globalIsoParams;
	if (isoParams[winNum]->isLocal()) return isoParams[winNum];
	return globalIsoParams;
}
Params* VizWinMgr::
getGlobalParams(Params::ParamType t){
	switch (t){
		case (Params::ViewpointParamsType):
			return globalVPParams;
		case (Params::RegionParamsType):
			return globalRegionParams;
		case (Params::ContourParamsType):
			return globalContourParams;
		case (Params::IsoParamsType):
			return globalIsoParams;
		case (Params::DvrParamsType):
			return globalDvrParams;
		default:  assert(0);
			return 0;
	}
}

Params* VizWinMgr::
getLocalParams(Params::ParamType t){
	assert(activeViz >= 0);
	switch (t){
		case (Params::ViewpointParamsType):
			return getRealVPParams(activeViz);
		case (Params::RegionParamsType):
			return getRealRegionParams(activeViz);
		case (Params::ContourParamsType):
			return getRealContourParams(activeViz);
		case (Params::IsoParamsType):
			return getRealIsoParams(activeViz);
		case (Params::DvrParamsType):
			return getRealDvrParams(activeViz);
		default:  assert(0);
			return 0;
	}
}
// force all the existing params to reinitialize
//
void VizWinMgr::
reinitializeParams(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vpParams[i]) vpParams[i]->reinit();
		if(rgParams[i]) rgParams[i]->reinit();
		if(dvrParams[i]) dvrParams[i]->reinit();
		if(isoParams[i]) isoParams[i]->reinit();
		if(contourParams[i]) contourParams[i]->reinit();
	}
	globalVPParams->reinit();
	globalRegionParams->reinit();
	globalIsoParams->reinit();
	globalDvrParams->reinit();
	globalContourParams->reinit();
}


