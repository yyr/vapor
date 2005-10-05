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
#include "contourparams.h"
#include "dvr.h"
#include "contourplanetab.h"
#include "isosurfaceparams.h"
#include "animationtab.h"
#include "animationparams.h"
#include "animationcontroller.h"
#include "isotab.h"
#include "flowtab.h"
#include "assert.h"
#include "trackball.h"
#include "glbox.h"
#include "vizactivatecommand.h"
#include "session.h"
#include "loadtfdialog.h"
#include "savetfdialog.h"

#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"

#ifdef VOLUMIZER
#include "volumizerrenderer.h"
#endif
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
    for (int i = 0; i< MAXVIZWINS; i++){
        vizWin[i] = 0;
        vizName[i] = "";
        //vizRect[i] = 0;
        isMax[i] = false;
        isMin[i] = false;
		vpParams[i] = 0;
		rgParams[i] = 0;
		dvrParams[i] = 0;
		isoParams[i] = 0;
		flowParams[i] = 0;
		contourParams[i] = 0;
		animationParams[i] = 0;
		activationOrder[i] = -1;
    }
	setSelectionMode(Command::navigateMode);
}
void VizWinMgr::
createGlobalParams() {
	//Create a default (global) parameter set
	globalVPParams = new ViewpointParams( -1);
	globalRegionParams = new RegionParams( -1);
	globalDvrParams = new DvrParams( -1);
	globalIsoParams = new IsosurfaceParams( -1);
	globalFlowParams = new FlowParams(-1);
	globalContourParams = new ContourParams( -1);
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
	if (isoParams[i]) delete isoParams[i];
	if (flowParams[i]) delete flowParams[i];
	if (contourParams[i]) delete contourParams[i];
	if (animationParams[i]) delete animationParams[i];
	vpParams[i] = 0;
	rgParams[i] = 0;
	dvrParams[i] = 0;
	isoParams[i] = 0;
	contourParams[i] = 0;
	animationParams[i] = 0;
	flowParams[i] = 0;
    
	emit (removeViz(i));
	
	
	//When the number is reduced to 1, disable multiviz options.
	if (numVizWins == 1) emit enableMultiViz(false);
	
	//Don't delete vizName, it's handled by ref counts
    
    vizName[i] = "";
}
//When a visualizer is launched, we may optionally specify a number and a name
//if this is a relaunch of a previously created visualizer.  If newWindowNum == -1,
//this is a brand new visualizer.  Otherwise it's a relaunch.
int VizWinMgr::
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
			MessageReporter::errorMsg("Unable to create additional visualizers");
			return -1;
		}	
	}
	
	numVizWins++;
	
	if (brandNew) createDefaultParams(newWindowNum);
		
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here:  
    Session::getInstance()->blockRecording();
	//Default name is just "Visualizer No. N"
    //qWarning("Creating new visualizer in position %d",newWindowNum);
	if (strlen(newName) != 0) vizName[newWindowNum] = newName;
	else vizName[newWindowNum] = ((QString("Visualizer No. ")+QString::number(newWindowNum)));
	emit (newViz(vizName[newWindowNum], newWindowNum));
	vizWin[newWindowNum] = new VizWin (myWorkspace, vizName[newWindowNum].ascii(), 0/*Qt::WType_TopLevel*/, this, newRect, newWindowNum);
	vizWin[newWindowNum]->setWindowNum(newWindowNum);
	

	vizWin[newWindowNum]->setFocusPolicy(QWidget::ClickFocus);

	vizWin[newWindowNum]->showMaximized();
	maximize(newWindowNum);
	//Tile if more than one visualizer:
	if(numVizWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	/*if (vpParams[newWindowNum])
		vpParams[newWindowNum]->setLocal(false);
	if (rgParams[newWindowNum])
		rgParams[newWindowNum]->setLocal(false);
	if (animationParams[newWindowNum])
		animationParams[newWindowNum]->setLocal(false);
		*/
	//Following seems to be unnecessary on windows and irix:
	activeViz = newWindowNum;
	vizWin[newWindowNum]->show();

	
	
	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numVizWins > 1){
		emit enableMultiViz(true);
	}

	//Prepare the visualizer for animation control.  It won't animate until user clicks
	//play
	//
	AnimationController::getInstance()->initializeVisualizer(newWindowNum);
	Session::getInstance()->unblockRecording();
	Session::getInstance()->addToHistory(new VizActivateCommand(
		vizWin[newWindowNum],prevActiveViz, newWindowNum, Command::create));
	return newWindowNum;
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
	
	isoParams[winnum] = (IsosurfaceParams*)globalIsoParams->deepCopy();
	isoParams[winnum]->setVizNum(winnum);
	isoParams[winnum]->setLocal(true);
	isoParams[winnum]->setEnabled(false);

	flowParams[winnum] = (FlowParams*)globalFlowParams->deepCopy();
	flowParams[winnum]->setVizNum(winnum);
	flowParams[winnum]->setLocal(true);
	flowParams[winnum]->setEnabled(false);

	contourParams[winnum] = (ContourParams*)globalContourParams->deepCopy();
	contourParams[winnum]->setVizNum(winnum);
	contourParams[winnum]->setLocal(true);
	contourParams[winnum]->setEnabled(false);

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
		case (Params::ContourParamsType):
			if(globalContourParams) delete globalContourParams;
			globalContourParams = (ContourParams*)p;
			return;
		case (Params::IsoParamsType):
			if(globalIsoParams) delete globalIsoParams;
			globalIsoParams = (IsosurfaceParams*)p;
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
	getViewpointParams(activeViz)->updateDialog();
	getRegionParams(activeViz)->updateDialog();
	getDvrParams(activeViz)->updateDialog();
	getContourParams(activeViz)->updateDialog();
	getIsoParams(activeViz)->updateDialog();
	getFlowParams(activeViz)->updateDialog();
	getAnimationParams(activeViz)->updateDialog();
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
void VizWinMgr::
setContourParams(int winnum, ContourParams* p){
	if (winnum < 0) { //global params!
		if (globalContourParams) delete globalContourParams;
		if (p) globalContourParams = (ContourParams*)p->deepCopy();
		else globalContourParams = 0;
	} else {
		if(contourParams[winnum]) delete contourParams[winnum];
		if (p) contourParams[winnum] = (ContourParams*)p->deepCopy();
		else contourParams[winnum] = 0;
	}
}
void VizWinMgr::
setIsoParams(int winnum, IsosurfaceParams* p){
	if (winnum < 0) { //global params!
		if (globalIsoParams) delete globalIsoParams;
		if (p) globalIsoParams = (IsosurfaceParams*)p->deepCopy();
		else globalIsoParams = 0;
	} else {
		if (isoParams[winnum]) delete isoParams[winnum];
		if (p) isoParams[winnum] = (IsosurfaceParams*)p->deepCopy();
		else isoParams[winnum] = 0;
	}
}
void VizWinMgr::
setFlowParams(int winnum, FlowParams* p){
	if (winnum < 0) { //global params!
		if (globalFlowParams) delete globalFlowParams;
		if (p) globalFlowParams = (FlowParams*)p->deepCopy();
		else globalFlowParams = 0;
	} else {
		if (flowParams[winnum]) delete flowParams[winnum];
		if (p) flowParams[winnum] = (FlowParams*)p->deepCopy();
		else flowParams[winnum] = 0;
	}
}
void VizWinMgr::
setDvrParams(int winnum, DvrParams* p){
	if (winnum < 0) { //global params!
		if (globalDvrParams) delete globalDvrParams;
		if (p) globalDvrParams = (DvrParams*)p->deepCopy();
		else globalDvrParams = p;
	} else {
		if(dvrParams[winnum]) delete dvrParams[winnum];
		if (p) dvrParams[winnum] = (DvrParams*)p->deepCopy();
		else dvrParams[winnum] = 0;
	}
}
void VizWinMgr::
setAnimationParams(int winnum, AnimationParams* p){
	if (winnum < 0) { //global params!
		if (globalAnimationParams) delete globalAnimationParams;
		if (p) globalAnimationParams = (AnimationParams*)p->deepCopy();
		else globalAnimationParams = 0;
	} else {
		if (animationParams[winnum]) delete animationParams[winnum];
		if (p) animationParams[winnum] = (AnimationParams*)p->deepCopy();
		else animationParams[winnum] = 0;
	}
}
void VizWinMgr::
setRegionParams(int winnum, RegionParams* p){
	if (winnum < 0) { //global params!
		if (globalRegionParams) delete globalRegionParams;
		if (p) globalRegionParams = (RegionParams*)p->deepCopy();
		else globalRegionParams = 0;
	} else {
		if (rgParams[winnum]) delete rgParams[winnum];
		if (p) rgParams[winnum] = (RegionParams*)p->deepCopy();
		else rgParams[winnum] = 0;
	}
}
void VizWinMgr::
setViewpointParams(int winnum, ViewpointParams* p){
	if (winnum < 0) { //global params!
		if (globalVPParams) delete globalVPParams;
		if (p) globalVPParams = (ViewpointParams*)p->deepCopy();
		else globalVPParams = 0;
	} else {
		if(vpParams[winnum]) delete vpParams[winnum];
		if (p) vpParams[winnum] = (ViewpointParams*)p->deepCopy();
		else vpParams[winnum] = 0;
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
	connect (vTab->lightPos00, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos01, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos02, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos10, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos11, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos12, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos20, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos21, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->lightPos22, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->camPos0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->camPos1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->camPos2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->viewDir0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->viewDir1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->viewDir2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->upVec0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->upVec1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->upVec2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->rotCenter0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->rotCenter1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (vTab->rotCenter2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (vTab->lightPos00, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos01, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos02, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos10, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos11, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos12, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos20, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos21, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->lightPos22, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (vTab->camPos0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->camPos1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->camPos2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->viewDir0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->viewDir1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->viewDir2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->upVec0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->upVec1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->upVec2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->rotCenter0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->rotCenter1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->rotCenter2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->numLights, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (vTab->centerFullViewButton, SIGNAL(clicked()), this, SLOT(regionCenterFull()));
	connect (vTab->centerRegionViewButton, SIGNAL(clicked()), this, SLOT(regionCenterRegion()));
	connect (this, SIGNAL(enableMultiViz(bool)), vTab->LocalGlobal, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), vTab->copyToButton, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), vTab->copyTargetCombo, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);
}
/**********************************************************
 * Whenever a new regiontab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpRegionTab(RegionTab* rTab)
{
	
	//Signals and slots:
	
	connect (rTab->numTransSpin, SIGNAL( valueChanged(int) ), this, SLOT( setRegionNumTrans(int) ) );
 	connect (rTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setRgLocalGlobal(int)));

	connect (rTab->xCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->yCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->zCntrEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->xSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->ySizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->zSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));
	connect (rTab->maxSizeEdit, SIGNAL( textChanged(const QString&) ), this, SLOT(setRegionTabTextChanged(const QString&)));

	connect (rTab->xCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->yCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->zCntrEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->xSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->ySizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->zSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->maxSizeEdit, SIGNAL( returnPressed() ), this, SLOT(regionReturnPressed()));
	connect (rTab->centerFullViewButton, SIGNAL(clicked()), this, SLOT(regionCenterFull()));
	connect (rTab->centerRegionViewButton, SIGNAL(clicked()), this, SLOT(regionCenterRegion()));
	connect (rTab->xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionXCenter()));
	connect (rTab->yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionYCenter()));
	connect (rTab->zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionZCenter()));
	connect (rTab->xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionXSize()));
	connect (rTab->ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionYSize()));
	connect (rTab->zSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionZSize()));
	connect (rTab->maxSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setRegionMaxSize()));
	
	connect (this, SIGNAL(enableMultiViz(bool)), rTab->LocalGlobal, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), rTab->copyToButton, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), rTab->copyTargetCombo, SLOT(setEnabled(bool)));

	emit enableMultiViz(getNumVisualizers() > 1);
}
void
VizWinMgr::hookUpDvrTab(Dvr* dvrTab)
{
	myDvrTab = dvrTab;
	connect (dvrTab->loadButton, SIGNAL(clicked()), this, SLOT(dvrLoadTF()));
	connect (dvrTab->saveButton, SIGNAL(clicked()), this, SLOT(dvrSaveTF()));
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

	connect (dvrTab->specularShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));
	connect (dvrTab->diffuseShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->ambientShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->exponentShading, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->specularAttenuation, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->diffuseAttenuation, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed() ) );
	connect (dvrTab->ambientAttenuation, SIGNAL( returnPressed() ), this, SLOT( dvrReturnPressed()));

	connect (dvrTab->numBitsSpin, SIGNAL (valueChanged(int)), this, SLOT(setDvrNumBits(int)));
	connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->LocalGlobal, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->copyToButton, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), dvrTab->copyTargetCombo, SLOT(setEnabled(bool)));
	//TFE Editor controls:
	connect (dvrTab->leftMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	connect (dvrTab->rightMappingBound, SIGNAL(returnPressed()), this, SLOT(dvrReturnPressed()));
	
	connect (dvrTab->histoStretchSlider, SIGNAL(sliderReleased()), this, SLOT (dvrHistoStretch()));
	connect (dvrTab->ColorBindButton, SIGNAL(pressed()), this, SLOT(dvrColorBind()));
	connect (dvrTab->OpacityBindButton, SIGNAL(pressed()), this, SLOT(dvrOpacBind()));
	connect (dvrTab->navigateButton, SIGNAL(toggled(bool)), this, SLOT(setDvrNavigateMode(bool)));
	
	connect (dvrTab->editButton, SIGNAL(toggled(bool)), this, SLOT(setDvrEditMode(bool)));
	
	connect(dvrTab->alignButton, SIGNAL(clicked()), this, SLOT(setDvrAligned()));
	
	connect(dvrTab->newHistoButton, SIGNAL(clicked()), this, SLOT(refreshHisto()));
	
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
	//connect (this, SIGNAL(enableMultiViz(bool)), cptTab->copyToButton, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), cptTab->copyTargetCombo, SLOT(setEnabled(bool)));
	emit enableMultiViz(getNumVisualizers() > 1);

}
/**********************************************************
 * Whenever a new tab is created it must be hooked up here
 ************************************************************/
void
VizWinMgr::hookUpAnimationTab(AnimationTab* aTab)
{
	//Signals and slots:
	
 	connect (aTab->LocalGlobal, SIGNAL (activated (int)), this, SLOT (setAnimationLocalGlobal(int)));
	
	connect (aTab->startFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (aTab->currentFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (aTab->endFrameEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (aTab->frameStepEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	connect (aTab->maxFrameRateEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setAtabTextChanged(const QString&)));
	
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (aTab->startFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (aTab->currentFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (aTab->endFrameEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (aTab->frameStepEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));
	connect (aTab->maxFrameRateEdit, SIGNAL( returnPressed()) , this, SLOT(animationReturnPressed()));

	//Sliders only do anything when released
	connect (aTab->frameStepSlider, SIGNAL(sliderReleased()), this, SLOT (animationSetFrameStep()));
	connect (aTab->animationSlider, SIGNAL(sliderReleased()), this, SLOT (animationSetPosition()));

	//Button clicking for toggle buttons:
	connect(aTab->pauseButton, SIGNAL(clicked()), this, SLOT(animationPauseClick()));
	connect(aTab->playReverseButton, SIGNAL(clicked()), this, SLOT(animationPlayReverseClick()));
	connect(aTab->playForwardButton, SIGNAL(clicked()), this, SLOT(animationPlayForwardClick()));
	connect(aTab->replayButton, SIGNAL(clicked()), this, SLOT(animationReplayClick()));
	//and non-toggle buttons:
	connect(aTab->toBeginButton, SIGNAL(clicked()), this, SLOT(animationToBeginClick()));
	connect(aTab->toEndButton, SIGNAL(clicked()), this, SLOT(animationToEndClick()));
	connect(aTab->stepReverseButton, SIGNAL(clicked()), this, SLOT(animationStepReverseClick()));
	connect(aTab->stepForwardButton, SIGNAL(clicked()), this, SLOT(animationStepForwardClick()));

	connect (this, SIGNAL(enableMultiViz(bool)), aTab->LocalGlobal, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), aTab->copyToButton, SLOT(setEnabled(bool)));
	//connect (this, SIGNAL(enableMultiViz(bool)), aTab->copyTargetCombo, SLOT(setEnabled(bool)));
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
void
VizWinMgr::hookUpFlowTab(FlowTab* flowTab)
{
	myFlowTab = flowTab;
	connect (flowTab->EnableDisable, SIGNAL(activated(int)), this, SLOT(setFlowEnabled(int)));
	connect (flowTab->instanceSpin, SIGNAL(valueChanged(int)), this, SLOT(setFlowInstance(int)));
	connect (flowTab->flowTypeCombo, SIGNAL( activated(int) ), this, SLOT( setFlowType(int) ) );
	connect (flowTab->numTransSpin, SIGNAL(valueChanged(int)),this, SLOT(setFlowNumTrans(int)));
	connect (flowTab->xCoordVarCombo,SIGNAL(activated(int)), this, SLOT(setFlowXVar(int)));
	connect (flowTab->yCoordVarCombo,SIGNAL(activated(int)), this, SLOT(setFlowYVar(int)));
	connect (flowTab->zCoordVarCombo,SIGNAL(activated(int)), this, SLOT(setFlowZVar(int)));
	connect (flowTab->recalcButton,SIGNAL(clicked()),this,SLOT(clickFlowRecalc()));
	connect (flowTab->randomCheckbox,SIGNAL(toggled(bool)),this, SLOT(checkFlowRandom(bool)));
	connect (flowTab->xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowXCenter()));
	connect (flowTab->yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowYCenter()));
	connect (flowTab->zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowZCenter()));
	connect (flowTab->xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowXSize()));
	connect (flowTab->ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowYSize()));
	connect (flowTab->zSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setFlowZSize()));
	connect (flowTab->geometrySamplesSlider, SIGNAL(sliderReleased()), this, SLOT(setFlowGeomSamples()));
	connect (flowTab->generatorDimensionCombo,SIGNAL(activated(int)), SLOT(setFlowGeneratorDimension(int)));
	connect (flowTab->geometryCombo, SIGNAL(activated(int)),SLOT(setFlowGeometry(int)));
	connect (flowTab->constantColorButton, SIGNAL(clicked()), this, SLOT(setFlowConstantColor()));
	
	connect (flowTab->colormapEntityCombo,SIGNAL(activated(int)),SLOT(setFlowColorMapEntity(int)));
	connect (flowTab->opacmapEntityCombo,SIGNAL(activated(int)),SLOT(setFlowOpacMapEntity(int)));

	connect (flowTab->constantOpacityEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->constantOpacityEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (flowTab->integrationAccuracyEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->integrationAccuracyEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->scaleFieldEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->scaleFieldEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->timeSampleEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->timeSampleEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->randomSeedEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->randomSeedEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->xCenterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->xCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->yCenterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->yCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->zCenterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->zCenterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->xSizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->xSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->ySizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->ySizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->zSizeEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->zSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->generatorCountEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->generatorCountEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->seedtimeStartEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->seedtimeStartEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->seedtimeEndEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->seedtimeEndEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->seedtimeIncrementEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->seedtimeIncrementEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->geometrySamplesEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->geometrySamplesEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->firstDisplayFrameEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->firstDisplayFrameEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (flowTab->lastDisplayFrameEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->lastDisplayFrameEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabFlowTextChanged(const QString&)));
	connect (flowTab->diameterEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->diameterEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabGraphicsTextChanged(const QString&)));
	connect (flowTab->minColormapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->minColormapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	connect (flowTab->maxColormapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->maxColormapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	connect (flowTab->minOpacmapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->minOpacmapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	connect (flowTab->maxOpacmapEdit,SIGNAL(returnPressed()), this, SLOT(flowTabReturnPressed()));
	connect (flowTab->maxOpacmapEdit,SIGNAL(textChanged(const QString&)), this, SLOT(setFlowTabRangeTextChanged(const QString&)));
	
	connect (flowTab->navigateButton, SIGNAL(toggled(bool)), this, SLOT(setFlowNavigateMode(bool)));
	connect (flowTab->editButton, SIGNAL(toggled(bool)), this, SLOT(setFlowEditMode(bool)));
	connect(flowTab->alignButton, SIGNAL(clicked()), this, SLOT(setFlowAligned()));

	
	connect (this, SIGNAL(enableMultiViz(bool)), flowTab->LocalGlobal, SLOT(setEnabled(bool)));
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
//Center region sets viewpoint values!
void VizWinMgr::
regionCenterRegion(){
	getRegionParams(activeViz)->guiCenterRegion(getViewpointParams(activeViz));
}
//Region centering buttons are a collaboration between viewpoint and region params
//
void VizWinMgr::
regionCenterFull(){
	getRegionParams(activeViz)->guiCenterFull(getViewpointParams(activeViz));
}
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
setRegionNumTrans(int nt){
	//Dispatch the signal to the current active region parameter panel, or to the
	//global panel:
	getRegionParams(activeViz)->guiSetNumTrans(nt);
	
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


void VizWinMgr::
setRegionDirty(RegionParams* rParams){
	int vizNum = rParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->setRegionDirty(true);
		getFlowParams(vizNum)->setFlowDataDirty();
		vizWin[activeViz]->updateGL();
	}
	//If another viz is using these region params, set their region dirty, too
	if (rParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!rgParams[i])||!rgParams[i]->isLocal())
			){
			vizWin[i]->setRegionDirty(true);
			getFlowParams(i)->setFlowDataDirty();
			vizWin[i]->updateGL();
		}
	}
}
//Force all windows that share a dvrParams to update
//their region (i.e. get new data)
void VizWinMgr::
setRegionDirty(DvrParams* dParams){
	int vizNum = dParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->setRegionDirty(true);
		vizWin[activeViz]->updateGL();
	}
	//If another viz is sharing these dvr params, set their region dirty, too
	if (dParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!dvrParams[i])||!dvrParams[i]->isLocal())
			){
			vizWin[i]->setRegionDirty(true);
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
//To force the animation to rerender, we set the region dirty.  We will need
//to load the data for another frame
//
void VizWinMgr::
setAnimationDirty(AnimationParams* aParams){
	int vizNum = aParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->setRegionDirty(true);
		vizWin[activeViz]->updateGL();
	}
	//If another viz is using these animation params, set their region dirty, too
	if (aParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!animationParams[i])||!animationParams[i]->isLocal())
			){
			vizWin[i]->setRegionDirty(true);
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
}

void VizWinMgr::
setClutDirty(DvrParams* dParams){
	if (!dParams) return;
	
	if (dParams->isLocal()){
		int vn = dParams->getVizNum();
		vizWin[vn]->setClutDirty(true);
		vizWin[vn]->updateGL();
		return;
	}
		
	//If another viz is using these dvr params, force them to update, too
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && !dvrParams[i]->isLocal())
		{
			vizWin[i]->setClutDirty(true);
			vizWin[i]->updateGL();
		}
	}
}
void VizWinMgr::
setDataRangeDirty(DvrParams* dParams){
	if (activeViz>=0) {
		vizWin[activeViz]->setDataRangeDirty(true);
		vizWin[activeViz]->updateGL();
	}
	if (dParams->isLocal()) return;
	//If another viz is using these dvr params, force them to update, too
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != activeViz) &&
				( (!dvrParams[i])||!dvrParams[i]->isLocal())
			){
			vizWin[i]->setDataRangeDirty(true);
			vizWin[i]->updateGL();
		}
	}
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
		if(animationParams[activeViz])animationParams[activeViz]->guiSetLocal(false);
		globalAnimationParams->updateDialog();
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!animationParams[activeViz]){
			//create a new parameter panel, copied from global
			animationParams[activeViz] = (AnimationParams*)globalAnimationParams->deepCopy();
			animationParams[activeViz]->setVizNum(activeViz);
			animationParams[activeViz]->guiSetLocal(true);
			
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			animationParams[activeViz]->guiSetLocal(true);
			animationParams[activeViz]->updateDialog();
			
			//and then refresh the panel:
			tabManager->show();
		}
	}
}
//Version for Flow:
void VizWinMgr::
setFlowLocalGlobal(int val){
	//If changes to global, revert to global panel.
	//If changes to local, may need to create a new local panel
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		if(flowParams[activeViz])flowParams[activeViz]->guiSetLocal(false);
		globalFlowParams->updateDialog();
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!flowParams[activeViz]){
			//create a new parameter panel, copied from global
			flowParams[activeViz] = (FlowParams*)globalFlowParams->deepCopy();
			flowParams[activeViz]->setVizNum(activeViz);
			flowParams[activeViz]->guiSetLocal(true);
			
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			flowParams[activeViz]->guiSetLocal(true);
			flowParams[activeViz]->updateDialog();
			
			//and then refresh the panel:
			tabManager->show();
		}
	}
}
/*****************************************************************************
 * Called when the local/global selector is changed.
 * Separate versions for viztab, regiontab, isotab, contourtab, dvrtab, flowtab
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
			vpParams[activeViz] = (ViewpointParams*)globalVPParams->deepCopy();
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
			rgParams[activeViz] = (RegionParams*)globalRegionParams->deepCopy();
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
	//Always invoke the update on the local params
	dvrParams[activeViz]->updateRenderer(wasEnabled,!val, false);
}

/*************************************************************************************
 *  slots associated with AnimationTab
 *************************************************************************************/
void VizWinMgr::
animationReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	getAnimationParams(activeViz)->confirmText(true);
}
void VizWinMgr::
setAtabTextChanged(const QString& ){
	getAnimationParams(activeViz)->guiSetTextChanged(true);
}
//Respond to release of frame-step slider
void VizWinMgr::
animationSetFrameStep(){
	getAnimationParams(activeViz)->guiSetFrameStep(
		myMainWindow->getAnimationTab()->frameStepSlider->value());
}
//Respond to release of animation position slider
void VizWinMgr::
animationSetPosition(){
	getAnimationParams(activeViz)->guiSetPosition(
		myMainWindow->getAnimationTab()->animationSlider->value());
}
//Respond to pause button press.

void VizWinMgr::
animationPauseClick(){
	myMainWindow->getAnimationTab()->playForwardButton->setDown(false);
	myMainWindow->getAnimationTab()->playReverseButton->setDown(false);
	getAnimationParams(activeViz)->guiSetPlay(0);
}
void VizWinMgr::
animationPlayReverseClick(){
	if (!myMainWindow->getAnimationTab()->playReverseButton->isDown()){
		myMainWindow->getAnimationTab()->playForwardButton->setDown(false);
		myMainWindow->getAnimationTab()->playReverseButton->setDown(true);
		getAnimationParams(activeViz)->guiSetPlay(-1);
	}
}
void VizWinMgr::
animationPlayForwardClick(){
	if (!myMainWindow->getAnimationTab()->playForwardButton->isDown()){
		myMainWindow->getAnimationTab()->playForwardButton->setDown(true);
		myMainWindow->getAnimationTab()->playReverseButton->setDown(false);
		getAnimationParams(activeViz)->guiSetPlay(1);
	}
}
void VizWinMgr::
animationReplayClick(){
	getAnimationParams(activeViz)->guiToggleReplay(
		myMainWindow->getAnimationTab()->replayButton->isOn());
}
void VizWinMgr::
animationToBeginClick(){
	getAnimationParams(activeViz)->guiJumpToBegin();
}
void VizWinMgr::
animationToEndClick(){
	getAnimationParams(activeViz)->guiJumpToEnd();
}
void VizWinMgr::
animationStepForwardClick(){
	getAnimationParams(activeViz)->guiSingleStep(true);
}
void VizWinMgr::
animationStepReverseClick(){
	getAnimationParams(activeViz)->guiSingleStep(false);
}
/*************************************************************************************
 *  slots associated with DvrTab
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
	
	if ((activeViz < 0) || Session::getInstance()->getDataMgr() == 0) {
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
void VizWinMgr::
setDvrEditMode(bool mode){
	myMainWindow->getDvrTab()->navigateButton->setOn(!mode);
	getDvrParams(activeViz)->guiSetEditMode(mode);
}
void VizWinMgr::
setDvrNavigateMode(bool mode){
	myMainWindow->getDvrTab()->editButton->setOn(!mode);
	getDvrParams(activeViz)->guiSetEditMode(!mode);
}
void VizWinMgr::
setDvrAligned(){
	getDvrParams(activeViz)->guiSetAligned();
}
void VizWinMgr::
refreshHisto(){
	VizWin* vizWin = getActiveVisualizer();
	if (!vizWin) return;
	DvrParams* dParams = getDvrParams(activeViz);
	//Refresh data range:
	//dParams->setDatarangeDirty();
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		Histo::refreshHistogram(activeViz);
	}
	dParams->refreshTFFrame();
}
/*
 * Respond to a slider release
 */
void VizWinMgr::
dvrHistoStretch() {
	getDvrParams(activeViz)->guiSetHistoStretch(
		myMainWindow->getDvrTab()->histoStretchSlider->value());
}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void VizWinMgr::
dvrSaveTF(void){
	SaveTFDialog* saveTFDialog = new SaveTFDialog(getDvrParams(activeViz),myMainWindow->getDvrTab(),
		"Save TF Dialog", true);
	int rc = saveTFDialog->exec();
	if (rc == 1) getDvrParams(activeViz)->fileSaveTF();
	//If rc=2, we already saved it to the session.
	//else if (rc == 2) getDvrParams(activeViz)->sessionSaveTF();
}
void VizWinMgr::
dvrLoadTF(void){
	//If there are no TF's currently in Session, just launch file load dialog.
	if (Session::getInstance()->getNumTFs() > 0){
		LoadTFDialog* loadTFDialog = new LoadTFDialog(getDvrParams(activeViz),myMainWindow->getDvrTab(),
			"Load TF Dialog", true);
		int rc = loadTFDialog->exec();
		if (rc == 0) return;
		if (rc == 1) getDvrParams(activeViz)->fileLoadTF();
		//if rc == 2, we already (probably) loaded a tf from the session
	} else {
		getDvrParams(activeViz)->fileLoadTF();
	}
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
/*************************************************************************************
 * slots associated with FlowTab
 *************************************************************************************/
//There are text changed events for flow (requiring rebuilding flow data),
//for graphics (requiring regenerating flow graphics), and
//for dataRange (requiring change of data mapping range, hence regenerating flow graphics)
void VizWinMgr::setFlowTabFlowTextChanged(const QString&){
	FlowParams* myFlowParams = getFlowParams(activeViz);
	myFlowParams->setFlowDataChanged(true);
	myFlowParams->guiSetTextChanged(true);
}
void VizWinMgr::setFlowTabGraphicsTextChanged(const QString&){
	FlowParams* myFlowParams = getFlowParams(activeViz);
	myFlowParams->setFlowGraphicsChanged(true);
	myFlowParams->guiSetTextChanged(true);
}
void VizWinMgr::setFlowTabRangeTextChanged(const QString&){
	FlowParams* myFlowParams = getFlowParams(activeViz);
	myFlowParams->setMapBoundsChanged(true);
	myFlowParams->guiSetTextChanged(true);
}
void VizWinMgr::
flowTabReturnPressed(void){
	getFlowParams(activeViz)->confirmText(true);
}
void VizWinMgr::
setFlowEnabled(int val){
	//If there's no window, or no datamgr, ignore this.
	
	if ((activeViz < 0) || Session::getInstance()->getDataMgr() == 0) {
		myFlowTab->EnableDisable->setCurrentItem(0);
		return;
	}
	getFlowParams(activeViz)->guiSetEnabled(val==1);
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	getFlowParams(activeViz)->updateRenderer(!val, flowParams[activeViz]->isLocal(), false);
}

void VizWinMgr::
setFlowInstance(int numInstance){
	//Not implemented yet
}
void VizWinMgr::
setFlowType(int typenum){
	getFlowParams(activeViz)->guiSetFlowType(typenum);
	//Activate/deactivate components associated with flow type:
	if(typenum == 0){//steady:
		myFlowTab->recalcButton->setEnabled(false);
		myFlowTab->seedtimeStartEdit->setEnabled(false);
		myFlowTab->seedtimeEndEdit->setEnabled(false);
		myFlowTab->seedtimeIncrementEdit->setEnabled(false);
		myFlowTab->timeSampleEdit->setEnabled(false);
	} else {//unsteady:
		myFlowTab->recalcButton->setEnabled(true);
		myFlowTab->seedtimeStartEdit->setEnabled(true);
		myFlowTab->seedtimeEndEdit->setEnabled(true);
		myFlowTab->seedtimeIncrementEdit->setEnabled(true);
		myFlowTab->timeSampleEdit->setEnabled(true);
	}
}
void VizWinMgr::
setFlowNumTrans(int numTrans){
	getFlowParams(activeViz)->guiSetNumTrans(numTrans);
}
void VizWinMgr::
setFlowXVar(int varnum){
	getFlowParams(activeViz)->guiSetXVarNum(varnum);
}
void VizWinMgr::
setFlowYVar(int varnum){
	getFlowParams(activeViz)->guiSetYVarNum(varnum);
}
void VizWinMgr::
setFlowZVar(int varnum){
	getFlowParams(activeViz)->guiSetZVarNum(varnum);
}
void VizWinMgr::
clickFlowRecalc(){
	getFlowParams(activeViz)->guiRecalc();
}
void VizWinMgr::
checkFlowRandom(bool isRandom){
	
	if (myFlowTab->randomCheckbox->isChecked()) isRandom = true;
	getFlowParams(activeViz)->guiSetRandom(isRandom);
	if (isRandom){
		myFlowTab->randomSeedEdit->setEnabled(true);
		myFlowTab->dimensionLabel->setEnabled(false);
		myFlowTab->generatorDimensionCombo->setEnabled(false);
	} else {
		myFlowTab->randomSeedEdit->setEnabled(false);
		myFlowTab->dimensionLabel->setEnabled(true);
		myFlowTab->generatorDimensionCombo->setEnabled(true);
	}
}
void VizWinMgr::
setFlowXCenter(){
	getFlowParams(activeViz)->guiSetXCenter(
		myFlowTab->xCenterSlider->value());
}
void VizWinMgr::
setFlowYCenter(){
	getFlowParams(activeViz)->guiSetYCenter(
		myFlowTab->yCenterSlider->value());
}
void VizWinMgr::
setFlowZCenter(){
	getFlowParams(activeViz)->guiSetZCenter(
		myFlowTab->zCenterSlider->value());
}
void VizWinMgr::
setFlowXSize(){
	getFlowParams(activeViz)->guiSetXSize(
		myFlowTab->xSizeSlider->value());
}
void VizWinMgr::
setFlowYSize(){
	getFlowParams(activeViz)->guiSetYSize(
		myFlowTab->ySizeSlider->value());
}
void VizWinMgr::
setFlowZSize(){
	getFlowParams(activeViz)->guiSetZSize(
		myFlowTab->zSizeSlider->value());
}
void VizWinMgr::
setFlowGeomSamples(){
	int sliderPos = myFlowTab->geometrySamplesSlider->value();
	getFlowParams(activeViz)->guiSetGeomSamples(sliderPos);
}
/*
 * Respond to user clicking the color button
 */
void VizWinMgr::
setFlowConstantColor(){
	
	//Bring up a color selector dialog:
	QColor newColor = QColorDialog::getColor(myFlowTab->constantColorButton->paletteBackgroundColor(), myFlowTab, "Constant Color Selection");
	//Set button color
	myFlowTab->constantColorButton->setPaletteBackgroundColor(newColor);
	//Set parameter value of the appropriate parameter set:
	getFlowParams(activeViz)->guiSetConstantColor(newColor);
}
void VizWinMgr::
setFlowGeneratorDimension(int flowGenDim){
	//This establishes which dimension is displayed in GUI
	getFlowParams(activeViz)->guiSetGeneratorDimension(flowGenDim);
	myFlowTab->generatorCountEdit->setText(QString::number(getFlowParams(activeViz)->getNumGenerators(flowGenDim)));
}
void VizWinMgr::
setFlowGeometry(int geomNum){
	getFlowParams(activeViz)->guiSetFlowGeometry(geomNum);
}
void VizWinMgr::
setFlowColorMapEntity(int entityNum){
	getFlowParams(activeViz)->guiSetColorMapEntity(entityNum);
}
void VizWinMgr::
setFlowOpacMapEntity(int entityNum){
	getFlowParams(activeViz)->guiSetOpacMapEntity(entityNum);
}

void VizWinMgr::
setFlowEditMode(bool mode){
	myMainWindow->getFlowTab()->navigateButton->setOn(!mode);
	getFlowParams(activeViz)->guiSetEditMode(mode);
}
void VizWinMgr::
setFlowNavigateMode(bool mode){
	myMainWindow->getFlowTab()->editButton->setOn(!mode);
	getFlowParams(activeViz)->guiSetEditMode(!mode);
}
void VizWinMgr::
setFlowAligned(){
	getFlowParams(activeViz)->guiSetAligned();
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
AnimationParams* VizWinMgr::
getAnimationParams(int winNum){
	if (winNum < 0) return globalAnimationParams;
	if (animationParams[winNum] && animationParams[winNum]->isLocal()) return animationParams[winNum];
	return globalAnimationParams;
}
ContourParams* VizWinMgr::
getContourParams(int winNum){
	if (winNum < 0) return globalContourParams;
	if (contourParams[winNum] && contourParams[winNum]->isLocal()) return contourParams[winNum];
	return globalContourParams;
}
IsosurfaceParams* VizWinMgr::
getIsoParams(int winNum){
	if (winNum < 0) return globalIsoParams;
	if (isoParams[winNum] && isoParams[winNum]->isLocal()) return isoParams[winNum];
	return globalIsoParams;
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
		case (Params::ContourParamsType):
			return globalContourParams;
		case (Params::IsoParamsType):
			return globalIsoParams;
		case (Params::DvrParamsType):
			return globalDvrParams;
		case (Params::AnimationParamsType):
			return globalAnimationParams;
		case (Params::FlowParamsType):
			return globalFlowParams;
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
		case (Params::ContourParamsType):
			return getContourParams(activeViz);
		case (Params::IsoParamsType):
			return getIsoParams(activeViz);
		case (Params::DvrParamsType):
			return getDvrParams(activeViz);
		case (Params::AnimationParamsType):
			return getAnimationParams(activeViz);
		case (Params::FlowParamsType):
			return getFlowParams(activeViz);
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
		if(isoParams[i]) isoParams[i]->restart();
		if(flowParams[i]) flowParams[i]->restart();
		if(contourParams[i]) contourParams[i]->restart();
		if(animationParams[i]) animationParams[i]->restart();
	}
	globalVPParams->restart();
	globalRegionParams->restart();
	globalIsoParams->restart();
	globalFlowParams->restart();
	globalDvrParams->restart();
	globalContourParams->restart();
	globalAnimationParams->restart();
}
// force all the existing params to reinitialize, i.e. make minimal
// changes to use new metadata.  If doOverride is false, we can
// ignore previous settings
//
void VizWinMgr::
reinitializeParams(bool doOverride){
	
	globalRegionParams->reinit(doOverride);
	//NOTE that the vpparams need to be initialized after
	//the global region params, since they use its settings..
	//
	globalVPParams->reinit(doOverride);
	globalIsoParams->reinit(doOverride);
	globalFlowParams->reinit(doOverride);
	globalDvrParams->reinit(doOverride);
	globalContourParams->reinit(doOverride);
	globalAnimationParams->reinit(doOverride);
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vpParams[i]) vpParams[i]->reinit(doOverride);
		if(rgParams[i]) rgParams[i]->reinit(doOverride);
		if(dvrParams[i]) dvrParams[i]->reinit(doOverride);
		if(isoParams[i]) isoParams[i]->reinit(doOverride);
		if(flowParams[i]) flowParams[i]->reinit(doOverride);
		if(contourParams[i]) contourParams[i]->reinit(doOverride);
		if(animationParams[i]) animationParams[i]->reinit(doOverride);
	}
}
void VizWinMgr::
setSelectionMode( Command::mouseModeType m){ 
	selectionMode = m;
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
			
			XmlNode* locals = vizMgrNode->NewChild(_vizWinTag, attrs, 5);
			//Now add local params
			XmlNode* dvrNode = dvrParams[i]->buildNode();
			if(dvrNode) locals->AddChild(dvrNode);
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
			} else return false;
		}
		//Create the window:
		parsingVizNum = launchVisualizer(-1, winName.c_str());
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
			return true;
		} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
			//Need to "push" to region parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(rgParams[parsingVizNum]);
			rgParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			return true;
		} else if (StrCmpNoCase(tag, Params::_viewpointParamsTag) == 0){
			//Need to "push" to viewpoint parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(vpParams[parsingVizNum]);
			vpParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			return true;
		} else if (StrCmpNoCase(tag, Params::_animationParamsTag) == 0){
			//Need to "push" to animation parser.
			//That parser will "pop" back to session when done.
			
			pm->pushClassStack(animationParams[parsingVizNum]);
			animationParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			return true;
		} else if (StrCmpNoCase(tag, Params::_flowParamsTag) == 0){
			//Need to "push" to flow parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(flowParams[parsingVizNum]);
			flowParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
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
