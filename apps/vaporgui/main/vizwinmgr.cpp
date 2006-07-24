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
#ifdef WIN32
#pragma warning(disable : 4251 4100)
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
#include <iostream>
#include <fstream>
#include <sstream>

#include "histo.h"
#include "vizwin.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
#include "math.h"
#include "mainform.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "viztab.h"
#include "regiontab.h"
#include "tabmanager.h"
#include "dvrparams.h"
#include "probeparams.h"

#include "dvr.h"
#include "probetab.h"

#include "animationtab.h"
#include "animationparams.h"
#include "animationcontroller.h"
#include "flowtab.h"
#include "assert.h"
#include "trackball.h"
#include "vizactivatecommand.h"
#include "session.h"
#include "loadtfdialog.h"
#include "savetfdialog.h"
#include "animationeventrouter.h"
#include "regioneventrouter.h"
#include "dvreventrouter.h"
#include "viewpointeventrouter.h"
#include "probeeventrouter.h"
#include "floweventrouter.h"
#include "panelcommand.h"
#include "eventrouter.h"

#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"


using namespace VAPoR;
VizWinMgr* VizWinMgr::theVizWinMgr = 0;
const string VizWinMgr::_visualizersTag = "Visualizers";
const string VizWinMgr::_vizWinTag = "VizWindow";
const string VizWinMgr::_vizWinNameAttr = "WindowName";
const string VizWinMgr::_vizBgColorAttr = "BackgroundColor";
const string VizWinMgr::_vizColorbarBackgroundColorAttr = "ColorbarBackgroundColor";
const string VizWinMgr::_vizRegionColorAttr = "RegionFrameColor";
const string VizWinMgr::_vizSubregionColorAttr = "SubregionFrameColor";
const string VizWinMgr::_vizAxisPositionAttr = "AxisPosition";
const string VizWinMgr::_vizColorbarLLPositionAttr = "ColorbarLLPosition";
const string VizWinMgr::_vizColorbarURPositionAttr = "ColorbarURPosition";
const string VizWinMgr::_vizColorbarNumTicsAttr = "ColorbarNumTics";
const string VizWinMgr::_vizAxesEnabledAttr = "AxesEnabled";
const string VizWinMgr::_vizColorbarEnabledAttr = "ColorbarEnabled";
const string VizWinMgr::_vizRegionFrameEnabledAttr = "RegionFrameEnabled";
const string VizWinMgr::_vizSubregionFrameEnabledAttr = "SubregionFrameEnabled";
const string VizWinMgr::_visualizerNumAttr = "VisualizerNum";


/******************************************************************
 *  Constructor sets up an array of MAXVIZWINS windows
 *
 *****************************************************************/
VizWinMgr::VizWinMgr() 
{
	myMainWindow = MainForm::getInstance();
    myWorkspace = myMainWindow->getWorkspace();
    tabManager = myMainWindow->getTabManager();
    previousClass = 0;
	activeViz = -1;
	activationCount = 0;
    benchmark = DONE;
    benchmarkTimer = NULL;
    for (int i = 0; i< MAXVIZWINS; i++){
        vizWin[i] = 0;
        vizName[i] = "";
        //vizRect[i] = 0;
        isMax[i] = false;
        isMin[i] = false;
		vpParams[i] = 0;
		rgParams[i] = 0;
		dvrParams[i] = 0;
		probeParams[i] = 0;
		flowParams[i] = 0;
		animationParams[i] = 0;
		activationOrder[i] = -1;
    }
	setSelectionMode(GLWindow::navigateMode);
	dvrEventRouter = 0;
	regionEventRouter = 0;
	viewpointEventRouter = 0;
	probeEventRouter = 0;
	animationEventRouter = 0;
	flowEventRouter = 0;
}
//Create the global params and the routing classes
void VizWinMgr::
createGlobalParams() {
	//regionEventRouter = new RegionEventRouter();
	//dvrEventRouter = new DvrEventRouter();
	
	//Create a default (global) parameter set
	globalVPParams = new ViewpointParams( -1);
	globalRegionParams = new RegionParams( -1);
	globalDvrParams = new DvrParams( -1);
	globalProbeParams = new ProbeParams(-1);
	
	globalFlowParams = new FlowParams(-1);
	
	globalAnimationParams = new AnimationParams( -1);
	globalTrackball = new Trackball();
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
   
	//Tell the animation controller to drop this viz:
	AnimationController::getInstance()->finishVisualizer(i);

    if (vizWin[i] == 0) return;

	

	//Count how many vizWins are left
	int numVizWins = 0;
	for (int j = 0; j<MAXVIZWINS; j++){
		if ((j != i) && vizWin[j]) {
			numVizWins++;
		}
	}
	//Remember the vizwin just to save its color:
	VizWin* vwin = vizWin[i];
	vizWin[i] = 0;
	
	//If we are deleting the active viz, revert to the 
	//most recent active viz.
	if (activeViz == i) {
		int newActive = getLastActive();
		if (newActive >= 0 && vizWin[newActive]) setActiveViz(newActive);
		else activeViz = -1;
	}
	
    
   
	//Save the state in history before deleting the params,
	//the next activeViz is the most recent active visualizer 
	if (Session::getInstance()->isRecording()){
		Session::getInstance()->addToHistory(new VizActivateCommand(
			vwin, i, activeViz, Command::remove));
	}
	if(activeViz >= 0) setActiveViz(activeViz);
	if(vpParams[i]) delete vpParams[i];
	if(rgParams[i]) delete rgParams[i];
	
	if (dvrParams[i]) delete dvrParams[i];
	if (probeParams[i]) delete probeParams[i];
	
	if (flowParams[i]) delete flowParams[i];
	if (animationParams[i]) delete animationParams[i];
	vpParams[i] = 0;
	rgParams[i] = 0;
	probeParams[i] = 0;
	dvrParams[i] = 0;
	animationParams[i] = 0;
	flowParams[i] = 0;
    
	emit (removeViz(i));
	
	
	//When the number is reduced to 1, disable multiviz options.
	if (numVizWins == 1) emit enableMultiViz(false);
	
	//Don't delete vizName, it's handled by ref counts
    
    vizName[i] = "";
}
//When a visualizer is launched, we may optionally specify a number and a name
//if this is a relaunch of a previously created visualizer.  If useWindowNum == -1,
//this is a brand new visualizer.  Otherwise it's a relaunch.
//If useWindowNum == -1, the value of newNum >=0 specifies the number of the brand new
//visualizer, which must not already be used!
//
int VizWinMgr::
launchVisualizer(int useWindowNum, const char* newName, int newNum)
{
	//launch to right of tab dialog, maximize
	//Look for first unused pointer, also count how many viz windows exist
	int prevActiveViz = activeViz;
	bool brandNew = true;
	int numVizWins = 0;
	if (useWindowNum != -1) {
		assert(!vizWin[useWindowNum]);
		brandNew = false;
	}
	//, count visualizers, find what to use for a new window num
	if (brandNew && newNum >= 0) {
		useWindowNum = newNum;
		assert(!vizWin[useWindowNum]);
	} else { //find the first available slot:
		for (int i = 0; i< MAXVIZWINS; i++){
			
			if (!vizWin[i]) {
				//Find first unused position
				if(useWindowNum == -1 && brandNew) useWindowNum = i;
			} else {
				numVizWins++;
			}
			
			if (i == MAXVIZWINS -1 && useWindowNum == -1) {
				MessageReporter::errorMsg("Unable to create additional visualizers");
				return -1;
			}	
		}
	}
	
	numVizWins++;
	
	if (brandNew) createDefaultParams(useWindowNum);
		
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here:  
    Session::getInstance()->blockRecording();
	//Default name is just "Visualizer No. N"
    //qWarning("Creating new visualizer in position %d",useWindowNum);
	if (strlen(newName) != 0) vizName[useWindowNum] = newName;
	else vizName[useWindowNum] = ((QString("Visualizer No. ")+QString::number(useWindowNum)));
	emit (newViz(vizName[useWindowNum], useWindowNum));
	vizWin[useWindowNum] = new VizWin (myWorkspace, vizName[useWindowNum].ascii(), 0/*Qt::WType_TopLevel*/, this, newRect, useWindowNum);
	vizWin[useWindowNum]->setWindowNum(useWindowNum);
	

	vizWin[useWindowNum]->setFocusPolicy(QWidget::ClickFocus);

	vizWin[useWindowNum]->showMaximized();
	maximize(useWindowNum);
	//Tile if more than one visualizer:
	if(numVizWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	/*if (vpParams[useWindowNum])
		vpParams[useWindowNum]->setLocal(false);
	if (rgParams[useWindowNum])
		rgParams[useWindowNum]->setLocal(false);
	if (animationParams[useWindowNum])
		animationParams[useWindowNum]->setLocal(false);
		*/
	//Following seems to be unnecessary on windows and irix:
	activeViz = useWindowNum;
	setActiveViz(useWindowNum);
	emit activateViz(useWindowNum);
	vizWin[useWindowNum]->show();

	
	
	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numVizWins > 1){
		emit enableMultiViz(true);
	}

	//Prepare the visualizer for animation control.  It won't animate until user clicks
	//play
	//
	AnimationController::getInstance()->initializeVisualizer(useWindowNum);
	Session::getInstance()->unblockRecording();
	Session::getInstance()->addToHistory(new VizActivateCommand(
		vizWin[useWindowNum],prevActiveViz, useWindowNum, Command::create));
	return useWindowNum;
}
/*
 *  Create params, for a new visualizer:
 */
void VizWinMgr::
createDefaultParams(int winnum){
	dvrParams[winnum] = (DvrParams*)globalDvrParams->deepCopy();
	dvrParams[winnum]->setVizNum(winnum);
	dvrParams[winnum]->setLocal(true);
	dvrParams[winnum]->setEnabled(false);

	probeParams[winnum] = (ProbeParams*)globalProbeParams->deepCopy();
	probeParams[winnum]->setVizNum(winnum);
	probeParams[winnum]->setLocal(true);
	probeParams[winnum]->setEnabled(false);
	
	
	flowParams[winnum] = (FlowParams*)globalFlowParams->deepCopy();
	flowParams[winnum]->setVizNum(winnum);
	flowParams[winnum]->setLocal(true);
	flowParams[winnum]->setEnabled(false);

	assert(0==vpParams[winnum]);
	vpParams[winnum] = (ViewpointParams*)globalVPParams->deepCopy();
	vpParams[winnum]->setVizNum(winnum);
	vpParams[winnum]->setLocal(false);

	assert(0==animationParams[winnum]);
	animationParams[winnum] = (AnimationParams*)globalAnimationParams->deepCopy();
	animationParams[winnum]->setVizNum(winnum);
	animationParams[winnum]->setLocal(false);

	assert(0==rgParams[winnum]);
	rgParams[winnum] = (RegionParams*)globalRegionParams->deepCopy();
	rgParams[winnum]->setVizNum(winnum);
	rgParams[winnum]->setLocal(false);
	
}
//Replace a specific global params with specified one
//
void VizWinMgr::replaceGlobalParams(Params* p, Params::ParamType typ){
	switch (typ){
		case (Params::DvrParamsType):
			if(globalDvrParams) delete globalDvrParams;
			globalDvrParams = (DvrParams*)p;
			return;
		case (Params::ProbeParamsType):
			if(globalProbeParams) delete globalProbeParams;
			globalProbeParams = (ProbeParams*)p;
			return;
		case (Params::RegionParamsType):
			if(globalRegionParams) delete globalRegionParams;
			globalRegionParams = (RegionParams*)p;
			return;
		case (Params::ViewpointParamsType):
			if(globalVPParams) delete globalVPParams;
			globalVPParams = (ViewpointParams*)p;
			return;
		case (Params::AnimationParamsType):
			if(globalAnimationParams) delete globalAnimationParams;
			globalAnimationParams = (AnimationParams*)p;
			return;
		case (Params::FlowParamsType):
			if(globalFlowParams) delete globalFlowParams;
			globalFlowParams = (FlowParams*)p;
			return;
		default:  assert(0); return;
	}
}
void VizWinMgr::
closeEvent()
{
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]) {
			vizWin[i]->close();
			//qWarning("closing viz window %d", i);
			//delete vizRect[i];
			
		}
	}
}



/***********************************************************************
 * The following tells relationship of region to viewpoint in a window:
 **************************************************************************/
bool VizWinMgr::
cameraBeyondRegionCenter(int coord, int vizWinNum){
	float regionMid = getRegionParams(vizWinNum)->getRegionCenter(coord);
	float cameraPos = getViewpointParams(vizWinNum)->getCameraPos(coord);
	return( regionMid < cameraPos);
		

}
/**************************************************************
 * Methods that arrange the viz windows:
 **************************************************************/
void 
VizWinMgr::cascade(){
	Session::getInstance()->blockRecording();
    myWorkspace->cascade();
	//Now size them up to a reasonable size:
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			vizWin[i]->resize(400,400);
		}	
	} 
	Session::getInstance()->unblockRecording();
}
void 
VizWinMgr::coverRight(){
	Session::getInstance()->blockRecording();
    for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			vizWin[i]->showMaximized();
			maximize(i);
		}
	} 
	Session::getInstance()->unblockRecording();
}

void 
VizWinMgr::fitSpace(){
	Session::getInstance()->blockRecording();
    myWorkspace->tile();
	Session::getInstance()->unblockRecording();
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
setVizWinName(int winNum, QString& qs) {
		vizName[winNum] = qs;
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
		updateActiveParams();
		
		tabManager->show();
		//Add to history if this is not during initial creation.
		if (prevActiveViz >= 0){
			Session::getInstance()->addToHistory(new VizActivateCommand(
				vizWin[vizNum],prevActiveViz, vizNum, Command::activate));
		}
	}
}
//Method to cause all the params to update their tab panels for the active
//Viz window
void VizWinMgr::
updateActiveParams(){
	if (activeViz < 0) return;
	viewpointEventRouter->updateTab(getViewpointParams(activeViz));
	regionEventRouter->updateTab(getRegionParams(activeViz));
	dvrEventRouter->updateTab(getDvrParams(activeViz));
	probeEventRouter->updateTab(getProbeParams(activeViz));
	flowEventRouter->updateTab(getFlowParams(activeViz));
	animationEventRouter->updateTab(getAnimationParams(activeViz));

}

//Method to enable closing of a vizWin for Undo/Redo
void VizWinMgr::
killViz(int viznum){
	vizWin[viznum]->close();
	vizWin[viznum] = 0;
}
/*
 *  Methods for changing the parameter panels.  Only done during undo/redo.
 */


void VizWinMgr::setParams(int winnum, Params* p, Params::ParamType t){
	Params** paramsArray = getParamsArray(t);
	if (winnum < 0) { //global params!
		Params* globalParams = getGlobalParams(t);
		if (globalParams) delete globalParams;
		if (p) setGlobalParams(p->deepCopy(),t);
		else setGlobalParams(0,t);
		//Set all windows that use global params:
		for (int i = 0; i<MAXVIZWINS; i++){
			if (paramsArray[i] && !paramsArray[i]->isLocal()){
				vizWin[i]->getGLWindow()->setParams(globalProbeParams,t);
			}
		}
	} else {
		if(paramsArray[winnum]) delete paramsArray[winnum];
		if (p) paramsArray[winnum] = p->deepCopy();
		else paramsArray[winnum] = 0;
		vizWin[winnum]->getGLWindow()->setParams(paramsArray[winnum],t);
	}
}

/**********************************************************
 * Whenever a new tab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpViewpointTab(ViewpointEventRouter* vTab)
{
	viewpointEventRouter = vTab;
	
 	connect (vTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setVpLocalGlobal(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), vTab->LocalGlobal, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
	vTab->hookUpTab();
}
/**********************************************************
 * Whenever a new regiontab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpRegionTab(RegionEventRouter* rTab)
{
	
	//Signals and slots:
	
	regionEventRouter = rTab;
	connect (rTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setRgLocalGlobal(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), rTab->LocalGlobal, SLOT(setEnabled(bool)));
	
	regionEventRouter->hookUpTab();
	
}
void
VizWinMgr::hookUpDvrTab(DvrEventRouter* dvrTab)
{
	dvrEventRouter = dvrTab;
	connect (dvrTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setDvrLocalGlobal(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->LocalGlobal, SLOT(setEnabled(bool)));
	
	dvrEventRouter->hookUpTab();
}

void
VizWinMgr::hookUpProbeTab(ProbeEventRouter* probeTab)
{
	probeEventRouter = probeTab;
	connect (probeTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setProbeLocalGlobal(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), probeTab->LocalGlobal, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
	probeEventRouter->hookUpTab();
}

/**********************************************************
 * Whenever a new tab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpAnimationTab(AnimationEventRouter* aTab)
{
	//Signals and slots:
	animationEventRouter = aTab;
 	connect (aTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setAnimationLocalGlobal(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), aTab->LocalGlobal, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), aTab->copyToButton, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), aTab->copyTargetCombo, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
	animationEventRouter->hookUpTab();
	
}

void
VizWinMgr::hookUpFlowTab(FlowEventRouter* flowTab)
{
	flowEventRouter = flowTab;
	connect (flowTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setFlowLocalGlobal(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), flowTab->LocalGlobal, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
	flowEventRouter->hookUpTab();
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
home()
{
	getViewpointRouter()->useHomeViewpoint();
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
sethome()
{
	getViewpointRouter()->setHomeViewpoint();
}
void VizWinMgr::
viewAll()
{
	getViewpointRouter()->guiCenterFullRegion(getRegionParams(activeViz));
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
viewRegion()
{
	getViewpointRouter()->guiCenterSubRegion(getRegionParams(activeViz));
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
alignView(int axis)
{
	if (axis < 1) return;
	//Always reset current item to first.
	myMainWindow->alignViewCombo->setCurrentItem(0);
	getViewpointRouter()->guiAlignView(axis);
}


//Trigger a re-render of the windows that share a region params
void VizWinMgr::refreshRegion(RegionParams* rParams){
	int vizNum = rParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	}
	//Is another viz using these region params?
	if (rParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!rgParams[i])||!rgParams[i]->isLocal())
			){
			vizWin[i]->updateGL();
		}
	}
}



//Force all windows that share a flow params to rerender
//(possibly with new data)
void VizWinMgr::
refreshFlow(FlowParams* fParams){
	int vizNum = fParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	}
	//If another viz is sharing these flow params, make them rerender, too
	if (fParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!flowParams[i])||!flowParams[i]->isLocal())
			){
			vizWin[i]->updateGL();
		}
	}
}

//Force all windows that share a probe params to rerender
//(possibly with new data)
void VizWinMgr::
refreshProbe(ProbeParams* pParams){
	int vizNum = pParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	}
	//If another viz is sharing these flow params, make them rerender, too
	if (pParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!probeParams[i])||!probeParams[i]->isLocal())
			){
			vizWin[i]->updateGL();
		}
	}
}


//set the changed bits for each of the visualizers associated with the
//specified animationParams, preventing them from updating the frame counter.
//
void VizWinMgr::
animationParamsChanged(AnimationParams* aParams){
	AnimationController* ac = AnimationController::getInstance();
	int vizNum = aParams->getVizNum();
	if (vizNum>=0) {
		ac->paramsChanged(vizNum);
	}
	//If another viz is using these animation params, set their region dirty, too
	if (aParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!animationParams[i])||!animationParams[i]->isLocal())
			){
			ac->paramsChanged(i);
		}
	}
}
//Set the viewer coords changed flag for all vizwin's using these params:
void VizWinMgr::
setViewerCoordsChanged(ViewpointParams* vp){
	int vizNum = vp->getVizNum();
	if (vizNum>=0) {
		vizWin[vizNum]->getGLWindow()->setViewerCoordsChanged(true);
	}
	if(vp->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!vpParams[i])||!vpParams[i]->isLocal())
			){
			vizWin[i]->getGLWindow()->setViewerCoordsChanged(true);
		}
	}
}
//Reset the near/far distances for all the windows that
//share a viewpoint, based on region in specified regionparams
//
void VizWinMgr::
resetViews(RegionParams* rp, ViewpointParams* vp){
	int vizNum = vp->getVizNum();
	if (vizNum>=0) {
		GLWindow* glw = vizWin[vizNum]->getGLWindow();
		if(glw) glw->resetView(rp, vp);
	}
	if(vp->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!vpParams[i])||!vpParams[i]->isLocal())
			){
			GLWindow* glw = vizWin[i]->getGLWindow();
			if(glw) glw->resetView(rp, vp);
		}
	}
}
//Cause one or more visualizers to start play-animating, depending on
//whether or not animation params are shared.
void VizWinMgr::startPlay(AnimationParams* aParams){
	AnimationController* ac = AnimationController::getInstance();
	
	if (activeViz>=0) {
		ac->startPlay(activeViz);
	}
	//If another viz is sharing global animation params, start them playing, too
	if (aParams->isLocal()) return;
	
	for (int i = 0; i< MAXVIZWINS; i++){
		
		if  ( vizWin[i] && (i != activeViz)  &&
				((!animationParams[i])||!animationParams[i]->isLocal())
			){
			ac->startPlay(i);
			
		}
	}
	
	ac->setMaxSharedWait(aParams->getMaxWait());
}


//Local global selector for all panels are similar.  First, Animation panel:
//
void VizWinMgr::
setAnimationLocalGlobal(int val){
	//If changes to global, revert to global panel.
	//If changes to local, may need to create a new local panel
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		if(animationParams[activeViz])animationEventRouter->guiSetLocal(animationParams[activeViz],false);
		animationEventRouter->updateTab(globalAnimationParams);
		vizWin[activeViz]->getGLWindow()->setAnimationParams(globalAnimationParams);
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!animationParams[activeViz]){
			//create a new parameter panel, copied from global
			animationParams[activeViz] = (AnimationParams*)globalAnimationParams->deepCopy();
			animationParams[activeViz]->setVizNum(activeViz);
			animationEventRouter->confirmText(true);
			
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			animationEventRouter->guiSetLocal(animationParams[activeViz],true);
			animationEventRouter->updateTab(animationParams[activeViz]);
			
		}
		//set the local params in the glwindow:
		vizWin[activeViz]->getGLWindow()->setAnimationParams(animationParams[activeViz]);
		//and then refresh the panel:
		tabManager->show();
	}
}
//Version for Flow:
void VizWinMgr::
setFlowLocalGlobal(int val){
	//If changes to global, revert to global panel.
	//If changes to local, may need to create a new local panel
	bool wasEnabled = getFlowParams(activeViz)->isEnabled();
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		flowEventRouter->guiSetLocal(flowParams[activeViz],false);
		flowEventRouter->updateTab(globalFlowParams);
		vizWin[activeViz]->getGLWindow()->setFlowParams(globalFlowParams);
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!flowParams[activeViz]){
			//create a new parameter panel, copied from global
			flowParams[activeViz] = (FlowParams*)globalFlowParams->deepCopy();
			flowParams[activeViz]->setVizNum(activeViz);
			flowEventRouter->guiSetLocal(flowParams[activeViz],true);
			
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			flowEventRouter->guiSetLocal(flowParams[activeViz],true);
			flowEventRouter->updateTab(flowParams[activeViz]);
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getGLWindow()->setFlowParams(flowParams[activeViz]);
	}
	//Always invoke the update on the local params
	flowEventRouter->updateRenderer(flowParams[activeViz],wasEnabled,!val, false);
}
/*****************************************************************************
 * Called when the local/global selector is changed.
 * Separate versions for viztab, regiontab, dvrtab, flowtab, probetab
 ******************************************************************************/
void VizWinMgr::
setVpLocalGlobal(int val){
	//If changes  to global, revert to global panel.
	//If changes to local, may need to create a new local panel
	//Switch local and global Trackball as appropriate
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		viewpointEventRouter->guiSetLocal(vpParams[activeViz],false);
		viewpointEventRouter->updateTab(globalVPParams);
		vizWin[activeViz]->getGLWindow()->setViewpointParams(globalVPParams);
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!vpParams[activeViz]){
			//create a new parameter panel, copied from global
			vpParams[activeViz] = (ViewpointParams*)globalVPParams->deepCopy();
			vpParams[activeViz]->setVizNum(activeViz);
			viewpointEventRouter->guiSetLocal(vpParams[activeViz],true);
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			viewpointEventRouter->guiSetLocal(vpParams[activeViz],true);
			viewpointEventRouter->updateTab(vpParams[activeViz]);
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getGLWindow()->setViewpointParams(vpParams[activeViz]);
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
		regionEventRouter->guiSetLocal(rgParams[activeViz],false);
		regionEventRouter->updateTab(globalRegionParams);
		vizWin[activeViz]->getGLWindow()->setRegionParams(globalRegionParams);
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!rgParams[activeViz]){
			//create a new parameter panel, copied from global
			rgParams[activeViz] = (RegionParams*)globalRegionParams->deepCopy();
			rgParams[activeViz]->setVizNum(activeViz);
			regionEventRouter->guiSetLocal(rgParams[activeViz],true);
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			regionEventRouter->guiSetLocal(rgParams[activeViz],true);
			regionEventRouter->updateTab(rgParams[activeViz]);
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getGLWindow()->setRegionParams(rgParams[activeViz]);
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
	
		((EventRouter*)dvrEventRouter)->guiSetLocal(dvrParams[activeViz],false);
		dvrEventRouter->updateTab(globalDvrParams);
		vizWin[activeViz]->getGLWindow()->setDvrParams(globalDvrParams);
		tabManager->show();
		//Was there a change in enablement?
		
	} else { //Local: 
		//need to revert to existing local settings:
		((EventRouter*)dvrEventRouter)->guiSetLocal(dvrParams[activeViz],true);
		dvrEventRouter->updateTab(dvrParams[activeViz]);
		//and then refresh the panel:
		tabManager->show();
	
	}
	vizWin[activeViz]->getGLWindow()->setDvrParams(dvrParams[activeViz]);
	//Always invoke the update the renderer on the local params
	dvrEventRouter->updateRenderer(dvrParams[activeViz],wasEnabled,!val, false);
	
}

/*****************************************************************************
 * Called when the Probe tab local/global selector is changed.
 * Affects only the Probe tab
 ******************************************************************************/
void VizWinMgr::
setProbeLocalGlobal(int val){
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	bool wasEnabled = getProbeParams(activeViz)->isEnabled();
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
	
		probeEventRouter->guiSetLocal(probeParams[activeViz],false);
		probeEventRouter->updateTab(globalProbeParams);
		vizWin[activeViz]->getGLWindow()->setProbeParams(globalProbeParams);
		tabManager->show();
		//Was there a change in enablement?
		
	} else { //Local: 
		//need to revert to existing local settings:
		probeEventRouter->guiSetLocal(probeParams[activeViz],true);
		probeEventRouter->updateTab(probeParams[activeViz]);
		//and then refresh the panel:
		tabManager->show();
	
	}
	vizWin[activeViz]->getGLWindow()->setProbeParams(probeParams[activeViz]);
	//Always invoke the update the renderer on the local params
	probeEventRouter->updateRenderer(probeParams[activeViz],wasEnabled,!val, false);

	
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
	if (dvrParams[winNum] && dvrParams[winNum]->isLocal()) return dvrParams[winNum];
	return globalDvrParams;
}
//For a renderer, there should always exist a local version.
ProbeParams* VizWinMgr::
getProbeParams(int winNum){

	if (winNum < 0) return globalProbeParams;
	if (probeParams[winNum] && probeParams[winNum]->isLocal()) return probeParams[winNum];
	return globalProbeParams;
}
AnimationParams* VizWinMgr::
getAnimationParams(int winNum){
	if (winNum < 0) return globalAnimationParams;
	if (animationParams[winNum] && animationParams[winNum]->isLocal()) return animationParams[winNum];
	return globalAnimationParams;
}

FlowParams* VizWinMgr::
getFlowParams(int winNum){
	if (winNum < 0) return globalFlowParams;
	if (flowParams[winNum] && flowParams[winNum]->isLocal()) return flowParams[winNum];
	return globalFlowParams;
}
Params* VizWinMgr::
getGlobalParams(Params::ParamType t){
	switch (t){
		case (Params::ViewpointParamsType):
			return globalVPParams;
		case (Params::RegionParamsType):
			return globalRegionParams;
			
		case (Params::DvrParamsType):
			return globalDvrParams;
		case (Params::ProbeParamsType):
			return globalProbeParams;
		case (Params::AnimationParamsType):
			return globalAnimationParams;
		case (Params::FlowParamsType):
			return globalFlowParams;
		default:  assert(0);
			return 0;
	}
}
void VizWinMgr::
setGlobalParams(Params* p, Params::ParamType t){
	switch (t){
		case (Params::ViewpointParamsType):
			globalVPParams = (ViewpointParams*)p;
			return;
		case (Params::RegionParamsType):
			globalRegionParams= (RegionParams*)p;
			return;
		case (Params::DvrParamsType):
			globalDvrParams = (DvrParams*)p;
			return;
		case (Params::ProbeParamsType):
			globalProbeParams=(ProbeParams*)p;
			return;
		case (Params::AnimationParamsType):
			globalAnimationParams=(AnimationParams*)p;
			return;
		case (Params::FlowParamsType):
			globalFlowParams=(FlowParams*)p;
			return;
		default:  assert(0);
			return;
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
		
		case (Params::DvrParamsType):
			return getRealDvrParams(activeViz);
		case (Params::ProbeParamsType):
			return getRealProbeParams(activeViz);
		case (Params::AnimationParamsType):
			return getRealAnimationParams(activeViz);
		case (Params::FlowParamsType):
			return getRealFlowParams(activeViz);
		default:  assert(0);
			return 0;
	}
}
//Get the Params that apply.  If there is no current active viz, then
//return the global params.
Params* VizWinMgr::
getApplicableParams(Params::ParamType t){
	if(activeViz < 0) return getGlobalParams(t);
	switch (t){
		case (Params::ViewpointParamsType):
			return getViewpointParams(activeViz);
		case (Params::RegionParamsType):
			return getRegionParams(activeViz);
		case (Params::DvrParamsType):
			return getDvrParams(activeViz);
		case (Params::ProbeParamsType):
			return getProbeParams(activeViz);
		case (Params::AnimationParamsType):
			return getAnimationParams(activeViz);
		case (Params::FlowParamsType):
			return getFlowParams(activeViz);
		default:  assert(0);
			return 0;
	}
}
//Get the Params that apply.  If there is no current active viz, then
//return the global params.
EventRouter* VizWinMgr::
getEventRouter(Params::ParamType t){
	
	switch (t){
		case (Params::ViewpointParamsType):
			return getInstance()->getViewpointRouter();
		case (Params::RegionParamsType):
			return getInstance()->getRegionRouter();
		
		case (Params::DvrParamsType):
			return getInstance()->getDvrRouter();
		case (Params::ProbeParamsType):
			return getInstance()->getProbeRouter();
		case (Params::AnimationParamsType):
			return getInstance()->getAnimationRouter();
		case (Params::FlowParamsType):
			return getInstance()->getFlowRouter();
		default:  assert(0);
			return 0;
	}
}
// force all the existing params to restart (return to initial state)
//
void VizWinMgr::
restartParams(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vpParams[i]) vpParams[i]->restart();
		if(rgParams[i]) rgParams[i]->restart();
		if(dvrParams[i]) dvrParams[i]->restart();
		if(probeParams[i]) probeParams[i]->restart();
		
		if(flowParams[i]) flowParams[i]->restart();
		
		if(animationParams[i]) animationParams[i]->restart();
	}
	globalVPParams->restart();
	globalRegionParams->restart();
	
	globalFlowParams->restart();
	globalDvrParams->restart();
	globalProbeParams->restart();
	
	globalAnimationParams->restart();
}
// force all the existing params to reinitialize, i.e. make minimal
// changes to use new metadata.  If doOverride is true, we can
// ignore previous settings
//
void VizWinMgr::
reinitializeParams(bool doOverride){
	
	globalRegionParams->reinit(doOverride);
	regionEventRouter->reinitTab(doOverride);
	regionEventRouter->refreshRegionInfo(globalRegionParams);
	//NOTE that the vpparams need to be initialized after
	//the global region params, since they use its settings..
	//
	globalVPParams->reinit(doOverride);
	if (!globalFlowParams->reinit(doOverride)){
		MessageReporter::errorMsg("Flow Params: No data in specified dataset");
		return;
	}
	flowEventRouter->reinitTab(doOverride);
	//Router can reinit before the params...?
	dvrEventRouter->reinitTab(doOverride);
	if (!globalDvrParams->reinit(doOverride)){
		MessageReporter::errorMsg("DVR Params: No data in specified dataset");
		return;
	}
	probeEventRouter->reinitTab(doOverride);
	if (!globalProbeParams->reinit(doOverride)){
		MessageReporter::errorMsg("Probe Params: No data in specified dataset");
		return;
	}
	
	animationEventRouter->reinitTab(doOverride);
	globalAnimationParams->reinit(doOverride);

	for (int i = 0; i< MAXVIZWINS; i++){
		if(vpParams[i]) vpParams[i]->reinit(doOverride);
		if(rgParams[i]) rgParams[i]->reinit(doOverride);
		if(dvrParams[i]) {
			dvrParams[i]->reinit(doOverride);
			dvrEventRouter->updateRenderer(dvrParams[i],dvrParams[i]->isEnabled(),dvrParams[i]->isLocal(),false);
		}

		if(probeParams[i]) {
			probeParams[i]->reinit(doOverride);
			probeEventRouter->updateRenderer(probeParams[i],probeParams[i]->isEnabled(),probeParams[i]->isLocal(),false);
		}
		
		if(flowParams[i]) {
			flowParams[i]->reinit(doOverride);
			flowEventRouter->updateRenderer(flowParams[i],flowParams[i]->isEnabled(),flowParams[i]->isLocal(),false);
		}
		
		
		if(animationParams[i]) animationParams[i]->reinit(doOverride);
		//setup near/far
		if (vpParams[i] && rgParams[i]) vizWin[i]->getGLWindow()->resetView(getRegionParams(i), getViewpointParams(i));
	}

}
void VizWinMgr::
setSelectionMode( GLWindow::mouseModeType m){ 
	GLWindow::setCurrentMouseMode(m);
	//Update all visualizers:
	for (int i = 0; i<MAXVIZWINS; i++){
		if(vizWin[i]) vizWin[i]->updateGL();
	}
}

XmlNode* VizWinMgr::buildNode() { 
	//Create a visualizers node, put in one child for each visualizer
	
	string empty;
	std::map <string, string> attrs;
	attrs.empty();
	ostringstream oss;
	XmlNode* vizMgrNode = new XmlNode(_visualizersTag, attrs, getNumVisualizers());
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]){
			attrs.empty();
			attrs[_vizWinNameAttr] = vizName[i].ascii();
			oss.str(empty);
			QColor clr = vizWin[i]->getBackgroundColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizBgColorAttr] = oss.str();

			oss.str(empty);
			clr = vizWin[i]->getRegionFrameColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizRegionColorAttr] = oss.str();

			oss.str(empty);
			clr = vizWin[i]->getSubregionFrameColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizSubregionColorAttr] = oss.str();

			oss.str(empty);
			clr = vizWin[i]->getColorbarBackgroundColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizColorbarBackgroundColorAttr] = oss.str();
			
			oss.str(empty);
			if (vizWin[i]->axesAreEnabled()) oss<<"true";
				else oss << "false";
			attrs[_vizAxesEnabledAttr] = oss.str();

			
			oss.str(empty);
			if (vizWin[i]->regionFrameIsEnabled()) oss<<"true";
				else oss<<"false";
			attrs[_vizRegionFrameEnabledAttr] = oss.str();

			oss.str(empty);
			if (vizWin[i]->subregionFrameIsEnabled()) oss<<"true";
				else oss<<"false";
			attrs[_vizSubregionFrameEnabledAttr] = oss.str();
			
			oss.str(empty);
			oss << (float)vizWin[i]->getAxisCoord(0) << " "
				<< (float)vizWin[i]->getAxisCoord(1) << " "
				<< (float)vizWin[i]->getAxisCoord(2);
			attrs[_vizAxisPositionAttr] = oss.str();

			oss.str(empty);
			if (vizWin[i]->colorbarIsEnabled()) oss << "true";
				else oss << "false";
			attrs[_vizColorbarEnabledAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getColorbarLLCoord(0) << " "
				<< (float)vizWin[i]->getColorbarLLCoord(1);
			attrs[_vizColorbarLLPositionAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getColorbarURCoord(0) << " "
				<< (float)vizWin[i]->getColorbarURCoord(1);
			attrs[_vizColorbarURPositionAttr] = oss.str();

			oss.str(empty);
			oss << (int) vizWin[i]->getColorbarNumTics();
			attrs[_vizColorbarNumTicsAttr] = oss.str();

			oss.str(empty);
			oss << (int) i;
			attrs[_visualizerNumAttr] = oss.str();
			
			XmlNode* locals = vizMgrNode->NewChild(_vizWinTag, attrs, 5);
			//Now add local params
			XmlNode* dvrNode = dvrParams[i]->buildNode();
			if(dvrNode) locals->AddChild(dvrNode);
			XmlNode* probeNode = probeParams[i]->buildNode();
			if(probeNode) locals->AddChild(probeNode);
			XmlNode* rgNode = rgParams[i]->buildNode();
			if(rgNode) locals->AddChild(rgNode);
			XmlNode* animNode = animationParams[i]->buildNode();
			if(animNode) locals->AddChild(animNode);
			XmlNode* vpNode = vpParams[i]->buildNode();
			if (vpNode) locals->AddChild(vpNode);
			XmlNode* flowNode = flowParams[i]->buildNode();
			if (flowNode) locals->AddChild(flowNode);
		}
	}
	return vizMgrNode;
}
//To parse, just get the visualizer name, and pass on the params-parsing
//The Visualizers node starts at depth 2, each visualizer is at depth 3
bool VizWinMgr::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tag, const char ** attrs){
	switch (depth){
	case(1):
		if (StrCmpNoCase(tag, _visualizersTag) == 0) return true; 
		else return false;
	case(2):
		{
		parsingVizNum = -1;
		//Expect only a vizwin tag here:
		if (StrCmpNoCase(tag, _vizWinTag) != 0) return false;
		//Create a visualizer
		//Get the name & num
		string winName;
		QColor winBgColor(black);
		QColor winRgColor(white);
		QColor winSubrgColor(red);
		QColor winColorbarColor(white);
		float axisPos[3];
		axisPos[0]=axisPos[1]=axisPos[2]=0.f;
		float colorbarLLPos[2], colorbarURPos[2];
		colorbarLLPos[0]=colorbarLLPos[1]=0.f;
		colorbarURPos[0]=0.1f;
		colorbarURPos[1]=0.3f;
		int colorbarTics = 11;
		bool axesEnabled = false;
		bool colorbarEnabled = false;
		bool regionEnabled = false;
		bool subregionEnabled = false;
		int numViz = -1;
		while (*attrs) {
			string attr = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);	
			if (StrCmpNoCase(attr, _vizWinNameAttr) == 0) {
				winName = value;
			} else if (StrCmpNoCase(attr, _vizBgColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist>>g; ist>>b;
				winBgColor.setRgb(r,g,b);
			} 
			else if (StrCmpNoCase(attr, _vizRegionColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist>>g; ist>>b;
				winRgColor.setRgb(r,g,b);
			}
			else if (StrCmpNoCase(attr, _vizSubregionColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist>>g; ist>>b;
				winSubrgColor.setRgb(r,g,b);
			}
			else if (StrCmpNoCase(attr, _vizColorbarBackgroundColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist>>g; ist>>b;
				winColorbarColor.setRgb(r,g,b);
			}
			else if (StrCmpNoCase(attr, _vizAxisPositionAttr) == 0) {
				ist >> axisPos[0]; ist>>axisPos[1]; ist>>axisPos[2];
			}
			else if (StrCmpNoCase(attr, _vizColorbarNumTicsAttr) == 0) {
				ist >> colorbarTics;
			}
			else if (StrCmpNoCase(attr, _vizColorbarLLPositionAttr) == 0) {
				ist >> colorbarLLPos[0]; ist>>colorbarLLPos[1];
			}
			else if (StrCmpNoCase(attr, _vizColorbarURPositionAttr) == 0) {
				ist >> colorbarURPos[0]; ist>>colorbarURPos[1];
			}
			else if (StrCmpNoCase(attr, _vizAxesEnabledAttr) == 0) {
				if (value == "true") axesEnabled = true; 
				else axesEnabled = false; 
			}
			else if (StrCmpNoCase(attr, _vizColorbarEnabledAttr) == 0) {
				if (value == "true") colorbarEnabled = true; 
				else colorbarEnabled = false; 
			}
			else if (StrCmpNoCase(attr, _vizRegionFrameEnabledAttr) == 0) {
			if (value == "true") regionEnabled = true; 
				else regionEnabled = false; 
			}
			else if (StrCmpNoCase(attr, _vizSubregionFrameEnabledAttr) == 0) {
				if (value == "true") subregionEnabled = true; 
				else subregionEnabled = false;

			} else if (StrCmpNoCase(attr, _visualizerNumAttr) == 0) {
				ist >> numViz;
			}
			else return false;
		}
		//Create the window:
		parsingVizNum = launchVisualizer(-1, winName.c_str(),numViz);
		vizWin[parsingVizNum]->setBackgroundColor(winBgColor);
		vizWin[parsingVizNum]->setRegionFrameColor(winRgColor);
		vizWin[parsingVizNum]->setSubregionFrameColor(winSubrgColor);
		vizWin[parsingVizNum]->setColorbarBackgroundColor(winColorbarColor);
		vizWin[parsingVizNum]->enableAxes(axesEnabled);
		vizWin[parsingVizNum]->enableColorbar(colorbarEnabled);
		vizWin[parsingVizNum]->enableRegionFrame(regionEnabled);
		vizWin[parsingVizNum]->enableSubregionFrame(subregionEnabled);
		for (int j = 0; j< 3; j++){
			vizWin[parsingVizNum]->setAxisCoord(j, axisPos[j]);
		}
		vizWin[parsingVizNum]->setColorbarLLCoord(0, colorbarLLPos[0]);
		vizWin[parsingVizNum]->setColorbarLLCoord(1, colorbarLLPos[1]);
		vizWin[parsingVizNum]->setColorbarURCoord(0, colorbarURPos[0]);
		vizWin[parsingVizNum]->setColorbarURCoord(1, colorbarURPos[1]);
		vizWin[parsingVizNum]->setColorbarNumTics(colorbarTics);
		
		return true;
		}
	case(3):
		//push the subsequent parsing to the params for current window 
		if (parsingVizNum < 0) return false;//we should have already created a visualizer
		if (StrCmpNoCase(tag, Params::_dvrParamsTag) == 0){
			//Need to "push" to dvr parser.
			//That parser will "pop" back to vizwinmgr when done.
			pm->pushClassStack(dvrParams[parsingVizNum]);
			dvrParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (dvrParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setDvrParams(dvrParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setDvrParams(globalDvrParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_probeParamsTag) == 0){
			//Need to "push" to dvr parser.
			//That parser will "pop" back to vizwinmgr when done.
			pm->pushClassStack(probeParams[parsingVizNum]);
			probeParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (probeParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setProbeParams(probeParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setProbeParams(globalProbeParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
			//Need to "push" to region parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(rgParams[parsingVizNum]);
			rgParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (rgParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setRegionParams(rgParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setRegionParams(globalRegionParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_viewpointParamsTag) == 0){
			//Need to "push" to viewpoint parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(vpParams[parsingVizNum]);
			vpParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (vpParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setViewpointParams(vpParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setViewpointParams(globalVPParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_animationParamsTag) == 0){
			//Need to "push" to animation parser.
			//That parser will "pop" back to session when done.
			
			pm->pushClassStack(animationParams[parsingVizNum]);
			animationParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (animationParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setAnimationParams(animationParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setAnimationParams(globalAnimationParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_flowParamsTag) == 0){
			//Need to "push" to flow parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(flowParams[parsingVizNum]);
			flowParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (flowParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setFlowParams(flowParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setFlowParams(globalFlowParams);
			return true;
		} else return false;
	default:
		return false;
	}
}
//End handler has nothing to do except for checking validity, except at end
//need to pop back to session.
bool VizWinMgr::elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	switch (depth) {
		case(1):
			{
			if (StrCmpNoCase(tag, _visualizersTag) != 0) return false;
			//need to pop the parse stack
			ParsedXml* px = pm->popClassStack();
			bool ok = px->elementEndHandler(pm, depth, tag);
			return ok;
			}
		case (2):
			if (StrCmpNoCase(tag, _vizWinTag) != 0) return false;
			//End of parsing all the params for a visualizer.
			//Must force the front tab to refresh:
			tabManager->newFrontTab(0);
			parsingVizNum = -1;
			return true;
		case(3):
			//End of parsing a params for a visualizer (popped back from params parsing)
			if (parsingVizNum < 0) return false;
			return true;
		default:
			return false;
	}
}
//General function for all dirty bit setting:
void VizWinMgr::setVizDirty(Params* p, DirtyBitType bittype, bool bit, bool refresh){
	VizWin* vw;
	if (p->getVizNum()>= 0){
		vw = getVizWin(p->getVizNum());
		vw->setDirtyBit(p->getParamType(),bittype, bit);
		if(bit&&refresh) vw->updateGL();
	} else {
		//Need to check all the windows whose params are global,
		Params** paramArray = getParamsArray(p->getParamType());
		for (int i = 0; i<MAXVIZWINS; i++){
			if (!paramArray[i] || paramArray[i]->isLocal()) continue;
			vw = getVizWin(i);
			vw->setDirtyBit(p->getParamType(),bittype, bit);
			if(bit&&refresh) vw->updateGL();
		}
	}
}
Params** VizWinMgr::getParamsArray(Params::ParamType t){
	switch(t){
		case Params::DvrParamsType :
			return (Params**)dvrParams;
		case Params::AnimationParamsType :
			return (Params**)animationParams;
		case Params::RegionParamsType :
			return (Params**)rgParams;
			
		case Params::FlowParamsType :
			return (Params**)flowParams;
			
		case Params::ProbeParamsType :
			return (Params**)probeParams;
		case Params::ViewpointParamsType :
			return (Params**)vpParams;
		default:
			assert(0);
			return 0;
	}
}

			

