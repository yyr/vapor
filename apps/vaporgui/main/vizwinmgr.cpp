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
#include <QMdiArea>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qlabel.h>
#include <qtimer.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "GL/glew.h"
#include "glwindow.h"
#include "histo.h"
#include "vizwin.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
#include "math.h"
#include "mainform.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "viztab.h"

#include "tabmanager.h"
#include "dvrparams.h"
#include "ParamsIso.h"
#include "params.h"
#include "twoDimageparams.h"
#include "twoDdataparams.h"
#include "animationparams.h"
#include "probeparams.h"
#include "animationcontroller.h"

#include "assert.h"
#include "trackball.h"
#include "vizactivatecommand.h"
#include "session.h"
#include "animationeventrouter.h"
#include "regioneventrouter.h"
#include "dvreventrouter.h"
#include "isoeventrouter.h"
#include "viewpointeventrouter.h"
#include "probeeventrouter.h"
#include "twoDimageeventrouter.h"
#include "twoDdataeventrouter.h"
#include "floweventrouter.h"
#include "panelcommand.h"
#include "eventrouter.h"
#include "VolumeRenderer.h"
#include "flowrenderer.h"

#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"


using namespace VAPoR;
VizWinMgr* VizWinMgr::theVizWinMgr = 0;

const string VizWinMgr::_visualizersTag = "Visualizers";
const string VizWinMgr::_vizWinTag = "VizWindow";
const string VizWinMgr::_vizWinNameAttr = "WindowName";
const string VizWinMgr::_vizBgColorAttr = "BackgroundColor";
const string VizWinMgr::_vizTimeAnnotColorAttr = "TimeAnnotationColor";
const string VizWinMgr::_vizTimeAnnotTypeAttr = "TimeAnnotationType";
const string VizWinMgr::_vizTimeAnnotCoordsAttr = "TimeAnnotationCoords";
const string VizWinMgr::_vizTimeAnnotTextSizeAttr = "TimeAnnotationTextSize";
const string VizWinMgr::_vizColorbarBackgroundColorAttr = "ColorbarBackgroundColor";
const string VizWinMgr::_vizRegionColorAttr = "RegionFrameColor";
const string VizWinMgr::_vizSubregionColorAttr = "SubregionFrameColor";
const string VizWinMgr::_vizAxisPositionAttr = "AxisPosition";
const string VizWinMgr::_vizAxisOriginAttr = "AxisOriginPosition";
const string VizWinMgr::_vizMinTicAttr = "MinTicPositions";
const string VizWinMgr::_vizMaxTicAttr = "MaxTicPositions";
const string VizWinMgr::_vizTicLengthAttr = "AxisTicLengths";
const string VizWinMgr::_vizTicDirAttr = "TicDirections";
const string VizWinMgr::_vizNumTicsAttr = "NumTicMarks";
const string VizWinMgr::_vizAxisColorAttr = "AxisAnnotationColor";
const string VizWinMgr::_vizTicWidthAttr = "TicWidth";
const string VizWinMgr::_vizLabelHeightAttr = "AxisLabelHeight";
const string VizWinMgr::_vizLabelDigitsAttr = "AxisLabelDigits";
const string VizWinMgr::_vizColorbarLLPositionAttr = "ColorbarLLPosition";
const string VizWinMgr::_vizColorbarURPositionAttr = "ColorbarURPosition";
const string VizWinMgr::_vizColorbarNumTicsAttr = "ColorbarNumTics";
const string VizWinMgr::_vizAxisArrowsEnabledAttr = "AxesEnabled";
const string VizWinMgr::_vizAxisAnnotationEnabledAttr = "AxisAnnotationEnabled";
const string VizWinMgr::_vizColorbarEnabledAttr = "ColorbarEnabled";
const string VizWinMgr::_vizElevGridEnabledAttr = "ElevGridRenderingEnabled";
const string VizWinMgr::_vizElevGridTexturedAttr = "ElevGridTextured";
const string VizWinMgr::_vizElevGridColorAttr = "ElevGridColor";
const string VizWinMgr::_vizElevGridRefinementAttr = "ElevGridRefinement";
const string VizWinMgr::_vizRegionFrameEnabledAttr = "RegionFrameEnabled";
const string VizWinMgr::_vizSubregionFrameEnabledAttr = "SubregionFrameEnabled";
const string VizWinMgr::_visualizerNumAttr = "VisualizerNum";
const string  VizWinMgr::_vizElevGridInvertedAttr = "ElevGridInverted";
const string  VizWinMgr::_vizElevGridDisplacementAttr = "ElevGridDisplacement";
const string  VizWinMgr::_vizElevGridRotationAttr = "ElevGridRotation";
const string  VizWinMgr::_vizElevGridTextureNameAttr = "ElevGridTextureFilename";


/******************************************************************
 *  Constructor sets up an array of MAXVIZWINS windows
 *
 *****************************************************************/
VizWinMgr::VizWinMgr() 
{
	myMainWindow = MainForm::getInstance();
    myMDIArea = myMainWindow->getMDIArea();
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

	dvrParamsInstances[i].clear();
	isoParamsInstances[i].clear();
	probeParamsInstances[i].clear();
	twoDImageParamsInstances[i].clear();
	twoDDataParamsInstances[i].clear();
	flowParamsInstances[i].clear();
	currentDvrInstance[i] = -1;
	currentIsoInstance[i] = -1;
	currentFlowInstance[i] = -1;
	currentProbeInstance[i] = -1;
	currentTwoDImageInstance[i] = -1;
	currentTwoDDataInstance[i] = -1;

	animationParams[i] = 0;
	activationOrder[i] = -1;
    }
	setSelectionMode(GLWindow::navigateMode);
	dvrEventRouter = 0;
	isoEventRouter = 0;
	regionEventRouter = 0;
	viewpointEventRouter = 0;
	probeEventRouter = 0;
	twoDImageEventRouter = 0;
	twoDDataEventRouter = 0;
	animationEventRouter = 0;
	flowEventRouter = 0;
}
//Create the global params and the default renderer params:
void VizWinMgr::
createGlobalParams() {
	
	//Create a default (global) parameter set
	globalVPParams = new ViewpointParams( -1);
	globalRegionParams = new RegionParams( -1);
	globalAnimationParams = new AnimationParams( -1);
	defaultDvrParams = new DvrParams(-1);
	defaultIsoParams = new ParamsIso(NULL, -1);
	defaultFlowParams = new FlowParams(-1);
	defaultProbeParams = new ProbeParams(-1);
	defaultTwoDImageParams = new TwoDImageParams(-1);
	defaultTwoDDataParams = new TwoDDataParams(-1);
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
	
	for (int inst = 0; inst < getNumDvrInstances(i); inst++){
		dvrEventRouter->cleanParams(getDvrParams(i,inst));
		delete getDvrParams(i,inst);
	}
	dvrParamsInstances[i].clear();

	for (int inst = 0; inst < getNumIsoInstances(i); inst++){
		isoEventRouter->cleanParams(getIsoParams(i,inst));
		delete getIsoParams(i,inst);
	}
	isoParamsInstances[i].clear();
	
	for (int inst = 0; inst < getNumProbeInstances(i); inst++){
		probeEventRouter->cleanParams(getProbeParams(i,inst));
		delete getProbeParams(i,inst);
	}
	probeParamsInstances[i].clear();
	for (int inst = 0; inst < getNumTwoDImageInstances(i); inst++){
		twoDImageEventRouter->cleanParams(getTwoDImageParams(i,inst));
		delete getTwoDImageParams(i,inst);
	}
	twoDImageParamsInstances[i].clear();
	for (int inst = 0; inst < getNumTwoDDataInstances(i); inst++){
		twoDDataEventRouter->cleanParams(getTwoDDataParams(i,inst));
		delete getTwoDDataParams(i,inst);
	}
	twoDDataParamsInstances[i].clear();
	for (int inst = 0; inst < getNumFlowInstances(i); inst++){
		flowEventRouter->cleanParams(getFlowParams(i,inst));
		delete getFlowParams(i,inst);
	}
	flowParamsInstances[i].clear();

	if (animationParams[i]) delete animationParams[i];
	vpParams[i] = 0;
	rgParams[i] = 0;
	animationParams[i] = 0;
	
    
	emit (removeViz(i));
	
	
	//When the number is reduced to 1, disable multiviz options.
	if (numVizWins == 1) emit enableMultiViz(false);
	
	//Don't delete vizName, it's handled by ref counts
    
    vizName[i] = "";

	EventRouter* ev = tabManager->getFrontEventRouter();
	ev->updateTab();
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
	if (useWindowNum != -1 && newNum == -1) {
		assert(!vizWin[useWindowNum]);
		brandNew = false;
	}
	// count visualizers, find what to use for a new window num
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
	vizWin[useWindowNum] = new VizWin (MainForm::getInstance(), vizName[useWindowNum], 0, this, newRect, useWindowNum);
	MainForm::getInstance()->getMDIArea()->addSubWindow(vizWin[useWindowNum]);
	
	vizWin[useWindowNum]->setWindowNum(useWindowNum);
	

	vizWin[useWindowNum]->setFocusPolicy(Qt::ClickFocus);

	//Following seems to be unnecessary on windows and irix:
	activeViz = useWindowNum;
	setActiveViz(useWindowNum);

	vizWin[useWindowNum]->showMaximized();
	maximize(useWindowNum);
	//Tile if more than one visualizer:
	if(numVizWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	
	
	emit activateViz(useWindowNum);
	vizWin[useWindowNum]->show();

	
	
	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numVizWins > 1){
		emit enableMultiViz(true);
	}

	//Initialize axis annotation to use full extents:
	const float *extents = Session::getInstance()->getExtents();
	vizWin[useWindowNum]->setAxisExtents(extents);
	

	//Prepare the visualizer for animation control.  It won't animate until user clicks
	//play
	AnimationController::getInstance()->initializeVisualizer(useWindowNum);
	Session::getInstance()->unblockRecording();
	Session::getInstance()->addToHistory(new VizActivateCommand(
		vizWin[useWindowNum],prevActiveViz, useWindowNum, Command::create));
	return useWindowNum;
}
/*
 *  Create params, for a new visualizer.  Just create one instance of renderparams
 */
void VizWinMgr::
createDefaultParams(int winnum){
	DvrParams* dParams = (DvrParams*)(defaultDvrParams->deepCopy());
	dParams->setVizNum(winnum);
	dParams->setEnabled(false);
	assert (getNumDvrInstances(winnum) == 0);
	dvrParamsInstances[winnum].push_back(dParams);
	setCurrentDvrInstIndex(winnum,0);

	ParamsIso* iParams = (ParamsIso*)(defaultIsoParams->deepCopy());
	iParams->SetVisualizerNum(winnum);
	iParams->setEnabled(false);
	assert (getNumIsoInstances(winnum) == 0);
	isoParamsInstances[winnum].push_back(iParams);
	setCurrentIsoInstIndex(winnum,0);
	
	ProbeParams* pParams = (ProbeParams*)(defaultProbeParams->deepCopy());
	pParams->setVizNum(winnum);
	pParams->setEnabled(false);
	assert (getNumProbeInstances(winnum) == 0);
	probeParamsInstances[winnum].push_back(pParams);
	setCurrentProbeInstIndex(winnum,0);

	TwoDImageParams* tParams = (TwoDImageParams*)(defaultTwoDImageParams->deepCopy());
	tParams->setVizNum(winnum);
	tParams->setEnabled(false);
	assert (getNumTwoDImageInstances(winnum) == 0);
	twoDImageParamsInstances[winnum].push_back(tParams);
	setCurrentTwoDImageInstIndex(winnum,0);

	TwoDDataParams* tdParams = (TwoDDataParams*)(defaultTwoDDataParams->deepCopy());
	tdParams->setVizNum(winnum);
	tdParams->setEnabled(false);
	assert (getNumTwoDDataInstances(winnum) == 0);
	twoDDataParamsInstances[winnum].push_back(tdParams);
	setCurrentTwoDDataInstIndex(winnum,0);

	FlowParams* fParams = (FlowParams*)(defaultFlowParams->deepCopy());
	fParams->setVizNum(winnum);
	fParams->setEnabled(false);
	assert (getNumFlowInstances(winnum) == 0);
	flowParamsInstances[winnum].push_back(fParams);
	setCurrentFlowInstIndex(winnum,0);

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
			if(defaultDvrParams) delete defaultDvrParams;
			defaultDvrParams = (DvrParams*)p;
			return;
		case (Params::IsoParamsType):
			if(defaultIsoParams) delete defaultIsoParams;
			defaultIsoParams = (ParamsIso*)p;
			return;
		case (Params::ProbeParamsType):
			if(defaultProbeParams) delete defaultProbeParams;
			defaultProbeParams = (ProbeParams*)p;
			return;
		case (Params::TwoDImageParamsType):
			if(defaultTwoDImageParams) delete defaultTwoDImageParams;
			defaultTwoDImageParams = (TwoDImageParams*)p;
			return;
		case (Params::TwoDDataParamsType):
			if(defaultTwoDDataParams) delete defaultTwoDDataParams;
			defaultTwoDDataParams = (TwoDDataParams*)p;
			return;
		case (Params::FlowParamsType):
			if(defaultFlowParams) delete defaultFlowParams;
			defaultFlowParams = (FlowParams*)p;
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
	int timestep = getActiveAnimationParams()->getCurrentFrameNumber();
	float regionMid = getRegionParams(vizWinNum)->getRegionCenter(coord,timestep);
	float cameraPos = getViewpointParams(vizWinNum)->getCameraPos(coord);
	return( regionMid < cameraPos);
		

}
/**************************************************************
 * Methods that arrange the viz windows:
 **************************************************************/
void 
VizWinMgr::cascade(){
	Session::getInstance()->blockRecording();
   	myMDIArea->cascadeSubWindows(); 
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
    myMDIArea->tileSubWindows();
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
		vizWin[winNum]->setWindowTitle(qs);
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
		//Set the animation toolbar to the correct timestep:
		int currentFrame = getActiveAnimationParams()->getCurrentFrameNumber();
		MainForm::getInstance()->setCurrentTimestep(currentFrame);
		//Tell the glwindows who is active
		GLWindow::setActiveWinNum(vizNum);
		//Determine if the active viz is sharing the region
		GLWindow::setRegionShareFlag(!getRegionParams(vizNum)->isLocal());

		tabManager->show();
		//Add to history if this is not during initial creation.
		if (prevActiveViz >= 0){
			Session::getInstance()->addToHistory(new VizActivateCommand(
				vizWin[vizNum],prevActiveViz, vizNum, Command::activate));
		}
		//Need to cause a redraw in all windows if we are not in navigate mode,
		//So that the manips will change where they are drawn:
		if (GLWindow::getCurrentMouseMode() != GLWindow::navigateMode){
			for (int i = 0; i< MAXVIZWINS; i++){
				if (vizWin[i]) vizWin[i]->updateGL();
			}
		}

	}
}
//Method to cause all the params to update their tab panels for the active
//Viz window
void VizWinMgr::
updateActiveParams(){
	if (activeViz < 0 ||!getVizWin(activeViz)) return;
	viewpointEventRouter->updateTab();
	regionEventRouter->updateTab();
	dvrEventRouter->updateTab();
	isoEventRouter->updateTab();
	probeEventRouter->updateTab();
	twoDImageEventRouter->updateTab();
	twoDDataEventRouter->updateTab();
	flowEventRouter->updateTab();
	animationEventRouter->updateTab();
	//Also update the activeParams in the GLWindow:
	GLWindow* glwin = getVizWin(activeViz)->getGLWindow();
	
	glwin->setActiveFlowParams(getFlowParams(activeViz));
	glwin->setActiveDvrParams(getDvrParams(activeViz));
	glwin->setActiveIsoParams(getIsoParams(activeViz));
	glwin->setActiveProbeParams(getProbeParams(activeViz));
	glwin->setActiveTwoDImageParams(getTwoDImageParams(activeViz));
	glwin->setActiveTwoDDataParams(getTwoDDataParams(activeViz));
	glwin->setActiveAnimationParams(getAnimationParams(activeViz));
	glwin->setActiveRegionParams(getRegionParams(activeViz));
	glwin->setActiveViewpointParams(getViewpointParams(activeViz));
}

//Method to enable closing of a vizWin for Undo/Redo
void VizWinMgr::
killViz(int viznum){
	vizWin[viznum]->close();
	vizWin[viznum] = 0;
}
/*
 *  Methods for changing the parameter panels.  Only done during undo/redo.
 *  If instance is -1, then changes the current instance of renderParams
 *  For enabled render params, change the rendererMapping so that it will
 *  map the new params to the existing renderer.
 */

void VizWinMgr::setParams(int winnum, Params* p, Params::ParamType t, int inst){
	
	if (!p || (!p->isRenderParams())){
		assert (inst == -1);
		Params** paramsArray = getParamsArray(t);

		if (winnum < 0) { //global params!
			Params* globalParams = getGlobalParams(t);
			if (globalParams) delete globalParams;
			if (p) setGlobalParams(p->deepCopy(),t);
			else setGlobalParams(0,t);
			globalParams = getGlobalParams(t);
			//Set all windows that use global params:
			for (int i = 0; i<MAXVIZWINS; i++){
				if (paramsArray[i] && !paramsArray[i]->isLocal()){
					vizWin[i]->getGLWindow()->setActiveParams(globalParams,t);
				}
			}
		} else {
			if(paramsArray[winnum]) delete paramsArray[winnum];
			if (p) paramsArray[winnum] = p->deepCopy();
			else paramsArray[winnum] = 0;
			vizWin[winnum]->getGLWindow()->setActiveParams(paramsArray[winnum],t);
		}
	} else {  //Handle render params:
		assert(p);
		
		if (inst == -1) inst = getCurrentInstanceIndex(winnum, t);
		//Save the previous render params pointer.  It might be needed
		//to enable/disable renderer
		RenderParams* prevRParams = (RenderParams*)getParams(winnum,t,inst);
		bool isRendering = (prevRParams != 0) && prevRParams->isEnabled();
		RenderParams* newRParams = 0;
		GLWindow* glwin = vizWin[winnum]->getGLWindow();
		switch (t) {
			FlowParams* fp; DvrParams* dp; ProbeParams* pp; ParamsIso* ip; TwoDDataParams *tp; TwoDImageParams *tip;
			
			case Params::FlowParamsType :
				fp = (FlowParams*)p;
				if (getNumFlowInstances(winnum)> inst) {
					if (getFlowParams(winnum,inst)) delete getFlowParams(winnum,inst);
					getAllFlowParams(winnum)[inst] = fp;
				} else {
					appendFlowInstance(winnum, fp);
				}
				
				newRParams = fp;
				break;

			case Params::DvrParamsType :
				dp = (DvrParams*)p;
				if (getNumDvrInstances(winnum)> inst) {
					if (getDvrParams(winnum,inst)) delete getDvrParams(winnum,inst);
					getAllDvrParams(winnum)[inst] = dp;
				} else {
					appendDvrInstance(winnum, dp);
				}
				
				newRParams = dp;
				break;

			case Params::IsoParamsType :
				ip = (ParamsIso*)p;
				if (getNumIsoInstances(winnum)> inst) {
					if (getIsoParams(winnum,inst)) delete getIsoParams(winnum,inst);
					getAllIsoParams(winnum)[inst] = ip;
				} else {
					appendIsoInstance(winnum, ip);
				}
				newRParams = ip;
				break;

			case Params::ProbeParamsType :
				
				pp = (ProbeParams*)p;
				if (getNumProbeInstances(winnum)> inst) {
					if (getProbeParams(winnum,inst)) delete getProbeParams(winnum,inst);
					getAllProbeParams(winnum)[inst] = pp;
				} else {
					appendProbeInstance(winnum, pp);
				}
				
				newRParams = pp;
				break;
			case Params::TwoDDataParamsType :
				
				tp = (TwoDDataParams*)p;
				if (getNumTwoDDataInstances(winnum)> inst) {
					if (getTwoDDataParams(winnum,inst)) delete getTwoDDataParams(winnum,inst);
					getAllTwoDDataParams(winnum)[inst] = tp;
				} else {
					appendTwoDDataInstance(winnum, tp);
				}
				
				newRParams = tp;
				break;
			case Params::TwoDImageParamsType :
				
				tip = (TwoDImageParams*)p;
				if (getNumTwoDImageInstances(winnum)> inst) {
					if (getTwoDImageParams(winnum,inst)) delete getTwoDImageParams(winnum,inst);
					getAllTwoDImageParams(winnum)[inst] = tip;
				} else {
					appendTwoDImageInstance(winnum, tip);
				}
				
				newRParams = tip;
				break;
			default:
				assert(0);
				break;
		}
		if (inst == getCurrentInstanceIndex(winnum, t))
			glwin->setActiveParams(newRParams, t);
		//May need to remap renderer:
		Renderer* rend = 0;
		if (isRendering){
			rend = glwin->getRenderer(prevRParams);
			assert(rend);
			rend->setRenderParams(newRParams);
			glwin->mapRenderer(newRParams,rend);
			bool ok = glwin->unmapRenderer(prevRParams);
			if(!ok) assert(ok);
		}

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
	dvrEventRouter->hookUpTab();
}
void
VizWinMgr::hookUpIsoTab(IsoEventRouter* isoTab)
{
	isoEventRouter = isoTab;
	isoEventRouter->hookUpTab();
}
void
VizWinMgr::hookUpProbeTab(ProbeEventRouter* probeTab)
{
	probeEventRouter = probeTab;
	probeEventRouter->hookUpTab();
}
void
VizWinMgr::hookUpTwoDImageTab(TwoDImageEventRouter* twoDTab)
{
	twoDImageEventRouter = twoDTab;
	twoDImageEventRouter->hookUpTab();
}
void
VizWinMgr::hookUpTwoDDataTab(TwoDDataEventRouter* twoDTab)
{
	twoDDataEventRouter = twoDTab;
	twoDDataEventRouter->hookUpTab();
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
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	vw->endSpin();
	getViewpointRouter()->useHomeViewpoint();
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
sethome()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	vw->endSpin();
	getViewpointRouter()->setHomeViewpoint();
}
void VizWinMgr::
viewAll()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	vw->endSpin();
	getViewpointRouter()->guiCenterFullRegion(getRegionParams(activeViz));
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
viewRegion()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	vw->endSpin();
	getViewpointRouter()->guiCenterSubRegion(getRegionParams(activeViz));
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
alignView(int axis)
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	vw->endSpin();
	if (axis < 1) return;
	//Always reset current item to first.
	myMainWindow->alignViewCombo->setCurrentIndex(0);
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
//Trigger a re-render of the windows that share a viewpoint params
void VizWinMgr::refreshViewpoint(ViewpointParams* vParams){
	int vizNum = vParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	}
	//Is another viz using these viewpoint params?
	if (vParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
				((!vpParams[i])||!vpParams[i]->isLocal())
			){
			vizWin[i]->updateGL();
		}
	}
}

//Force the windows that uses  a flow params to rerender
//(possibly with new data)
void VizWinMgr::
refreshFlow(FlowParams* fParams){
	int vizNum = fParams->getVizNum();
	//There should only be one!
	
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	} else assert(0);
}

//Force the window that uses a probe params to rerender
//(possibly with new data)
void VizWinMgr::
refreshProbe(ProbeParams* pParams){
	int vizNum = pParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	} else assert(0);
}
//Force the window that uses a TwoDImage params to rerender
//(possibly with new data)
void VizWinMgr::
refreshTwoDImage(TwoDImageParams* pParams){
	int vizNum = pParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	} else assert(0);
}
//Force the window that uses a TwoDData params to rerender
//(possibly with new data)
void VizWinMgr::
refreshTwoDData(TwoDDataParams* pParams){
	int vizNum = pParams->getVizNum();
	if (vizNum >= 0){
		vizWin[activeViz]->updateGL();
	} else assert(0);
}

//set the changed bits for each of the visualizers associated with the
//specified animationParams, preventing them from updating the frame counter.
//Also set the lat/lon or local coordinates in the viewpoint params
//
void VizWinMgr::
animationParamsChanged(AnimationParams* aParams){
	AnimationController* ac = AnimationController::getInstance();
	int vizNum = aParams->getVizNum();
	ViewpointParams* vpp = 0;
	if (vizNum>=0) {
		ac->paramsChanged(vizNum);
		vpp = getViewpointParams(vizNum);
		if (vpp->isLatLon())
			vpp->convertFromLatLon(aParams->getCurrentFrameNumber());
		else 
			vpp->convertToLatLon(aParams->getCurrentFrameNumber());
	}
	//If another viz is using these animation params, set their region dirty, too
	//and set their latlon or local coords
	if (aParams->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if  ( vizWin[i] && (i != vizNum)  &&
			((!animationParams[i])||!animationParams[i]->isLocal())){
			ac->paramsChanged(i);
			ViewpointParams* vp2 = getViewpointParams(i);
			if (vp2!= vpp){
				if (vp2->isLatLon())
					vp2->convertFromLatLon(aParams->getCurrentFrameNumber());
				else 
					vp2->convertToLatLon(aParams->getCurrentFrameNumber());
			}
		}
	}
	MainForm::getInstance()->setCurrentTimestep(aParams->getCurrentFrameNumber());
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
		animationEventRouter->updateTab();
		vizWin[activeViz]->getGLWindow()->setActiveAnimationParams(globalAnimationParams);
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
			animationEventRouter->updateTab();
			
		}
		//set the local params in the glwindow:
		vizWin[activeViz]->getGLWindow()->setActiveAnimationParams(animationParams[activeViz]);
		//and then refresh the panel:
		tabManager->show();
	}
}

/*****************************************************************************
 * Called when the local/global selector is changed.
 * Separate versions for viztab, regiontab
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
		viewpointEventRouter->updateTab();
		vizWin[activeViz]->getGLWindow()->setActiveViewpointParams(globalVPParams);
		vizWin[activeViz]->setGlobalViewpoint(true);
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
			viewpointEventRouter->updateTab();
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getGLWindow()->setActiveViewpointParams(vpParams[activeViz]);
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
	//In either case need to force redraw.
	//If change to local, and in region mode, need to redraw all global windows
	
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		regionEventRouter->guiSetLocal(rgParams[activeViz],false);
		regionEventRouter->updateTab();
		vizWin[activeViz]->getGLWindow()->setActiveRegionParams(globalRegionParams);
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
			regionEventRouter->updateTab();
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getGLWindow()->setActiveRegionParams(rgParams[activeViz]);
	}
	//Specify if the active viz is sharing the region
	
	GLWindow::setRegionShareFlag(val == 0);
	
	//in region mode, refresh the global windows and this one
	if (GLWindow::getCurrentMouseMode() == GLWindow::regionMode){
		for (int i = 0; i<MAXVIZWINS; i++){
			if(vizWin[i] && (!getRegionParams(i)->isLocal()|| (i == activeViz)))
				vizWin[i]->updateGL();
		}
	}
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
//If the instance is <0, return the current instance.
DvrParams* VizWinMgr::
getDvrParams(int winNum, int instance){
	if(winNum < 0) return defaultDvrParams;
	if (instance < 0) instance = currentDvrInstance[winNum];
	if (instance >= getNumDvrInstances(winNum)) return 0;
	return (dvrParamsInstances[winNum])[instance];
}
ParamsIso* VizWinMgr::
getIsoParams(int winNum, int instance){
	if(winNum < 0) return defaultIsoParams;
	if (instance < 0) instance = currentIsoInstance[winNum];
	if (instance >= getNumIsoInstances(winNum)) return 0;
	return (isoParamsInstances[winNum])[instance];
}
//For a renderer, there should exist a local version.
ProbeParams* VizWinMgr::
getProbeParams(int winNum, int instance){
	if(winNum < 0) return defaultProbeParams;
	if (instance < 0) instance = currentProbeInstance[winNum];
	if (instance >= getNumProbeInstances(winNum)) return 0;
	return (probeParamsInstances[winNum])[instance];
}
//For a renderer, there should exist a local version.
TwoDImageParams* VizWinMgr::
getTwoDImageParams(int winNum, int instance){
	if(winNum < 0) return defaultTwoDImageParams;
	if (instance < 0) instance = currentTwoDImageInstance[winNum];
	if (instance >= getNumTwoDImageInstances(winNum)) return 0;
	return (twoDImageParamsInstances[winNum])[instance];
}
TwoDDataParams* VizWinMgr::
getTwoDDataParams(int winNum, int instance){
	if(winNum < 0) return defaultTwoDDataParams;
	if (instance < 0) instance = currentTwoDDataInstance[winNum];
	if (instance >= getNumTwoDDataInstances(winNum)) return 0;
	return (twoDDataParamsInstances[winNum])[instance];
}
AnimationParams* VizWinMgr::
getAnimationParams(int winNum){
	if (winNum < 0) return globalAnimationParams;
	if (animationParams[winNum] && animationParams[winNum]->isLocal()) return animationParams[winNum];
	return globalAnimationParams;
}

FlowParams* VizWinMgr::
getFlowParams(int winNum, int instance){
	if(winNum < 0) return defaultFlowParams;
	if (instance < 0) instance = currentFlowInstance[winNum];
	if (instance >= getNumFlowInstances(winNum)) return 0;
	return (flowParamsInstances[winNum])[instance];
}
//Return global or default params
Params* VizWinMgr::
getGlobalParams(Params::ParamType t){
	switch (t){
		case (Params::ViewpointParamsType):
			return globalVPParams;
		case (Params::RegionParamsType):
			return globalRegionParams;
		case (Params::AnimationParamsType):
			return globalAnimationParams;
		case (Params::DvrParamsType):
			return defaultDvrParams;
		case (Params::IsoParamsType):
			return defaultIsoParams;
		case (Params::FlowParamsType):
			return defaultFlowParams;
		case (Params::ProbeParamsType):
			return defaultProbeParams;
		case (Params::TwoDImageParamsType):
			return defaultTwoDImageParams;
		case (Params::TwoDDataParamsType):
			return defaultTwoDDataParams;
		default:  
			qWarning(" invalid ParamType %d",t);
			assert(0);
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
		case (Params::AnimationParamsType):
			globalAnimationParams=(AnimationParams*)p;
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
			return getDvrParams(activeViz);
		case (Params::IsoParamsType):
			return getIsoParams(activeViz);
		case (Params::ProbeParamsType):
			return getProbeParams(activeViz);
		case (Params::TwoDImageParamsType):
			return getTwoDImageParams(activeViz);
		case (Params::TwoDDataParamsType):
			return getTwoDDataParams(activeViz);
		case (Params::AnimationParamsType):
			return getRealAnimationParams(activeViz);
		case (Params::FlowParamsType):
			return getFlowParams(activeViz);
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
		case (Params::IsoParamsType):
			return getIsoParams(activeViz);
		case (Params::ProbeParamsType):
			return getProbeParams(activeViz);
		case (Params::TwoDImageParamsType):
			return getTwoDImageParams(activeViz);
		case (Params::TwoDDataParamsType):
			return getTwoDDataParams(activeViz);
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
		case (Params::IsoParamsType):
			return getInstance()->getIsoRouter();
		case (Params::ProbeParamsType):
			return getInstance()->getProbeRouter();
		case (Params::TwoDImageParamsType):
			return getInstance()->getTwoDImageRouter();
		case (Params::TwoDDataParamsType):
			return getInstance()->getTwoDDataRouter();
		case (Params::AnimationParamsType):
			return getInstance()->getAnimationRouter();
		case (Params::FlowParamsType):
			return getInstance()->getFlowRouter();
		default:  assert(0);
			return 0;
	}
}
void VizWinMgr::
initViews(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]) {
			if (vpParams[i]->isLocal()){
				vizWin[i]->setValuesFromGui(vpParams[i]);
				
			}
			else vizWin[i]->setValuesFromGui(globalVPParams);

			vizWin[i]->getGLWindow()->setViewerCoordsChanged(true);
		}
	}
}
// force all the existing params to restart (return to initial state)
//
void VizWinMgr::
restartParams(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vpParams[i]) vpParams[i]->restart();
		if(rgParams[i]) rgParams[i]->restart();
		for (int inst = 0; inst<getNumDvrInstances(i); inst++){
			dvrEventRouter->cleanParams(getDvrParams(i,inst));
			getDvrParams(i,inst)->restart();
		}
		for (int inst = 0; inst<getNumIsoInstances(i); inst++){
			isoEventRouter->cleanParams(getIsoParams(i,inst));
			getIsoParams(i,inst)->restart();
		}
		for (int inst = 0; inst<getNumProbeInstances(i); inst++){
			probeEventRouter->cleanParams(getProbeParams(i,inst));
			getProbeParams(i,inst)->restart();
		}
		for (int inst = 0; inst<getNumTwoDImageInstances(i); inst++){
			twoDImageEventRouter->cleanParams(getTwoDImageParams(i,inst));
			getTwoDImageParams(i,inst)->restart();
		}
		for (int inst = 0; inst<getNumTwoDDataInstances(i); inst++){
			twoDDataEventRouter->cleanParams(getTwoDDataParams(i,inst));
			getTwoDDataParams(i,inst)->restart();
		}
		for (int inst = 0; inst<getNumFlowInstances(i); inst++){
			flowEventRouter->cleanParams(getFlowParams(i,inst));
			getFlowParams(i,inst)->restart();
		}
		
		if(animationParams[i]) animationParams[i]->restart();
	}
	globalVPParams->restart();
	globalRegionParams->restart();
	globalAnimationParams->restart();
}
// force all the existing params to reinitialize, i.e. make minimal
// changes to use new metadata.  If doOverride is true, we can
// ignore previous settings
//
void VizWinMgr::
reinitializeParams(bool doOverride){
	// Default render params should override
	defaultDvrParams->reinit(true);
	defaultIsoParams->reinit(true);
	defaultFlowParams->reinit(true);
	defaultProbeParams->reinit(true);
	defaultTwoDImageParams->reinit(true);
	defaultTwoDDataParams->reinit(true);

	globalRegionParams->reinit(doOverride);
	regionEventRouter->reinitTab(doOverride);
	regionEventRouter->refreshRegionInfo(globalRegionParams);
	//NOTE that the vpparams need to be initialized after
	//the global region params, since they use its settings..
	//
	globalVPParams->reinit(doOverride);
	
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vpParams[i]) vpParams[i]->reinit(doOverride);
		if(rgParams[i]) rgParams[i]->reinit(doOverride);
		for (int inst = 0; inst < getNumDvrInstances(i); inst++){
			DvrParams* dParams = getDvrParams(i,inst);
			dParams->reinit(doOverride);
			//Turn off rendering:
			bool wasEnabled = dParams->isEnabled();
			dParams->setEnabled(false);
			dvrEventRouter->updateRenderer(dParams,wasEnabled,false);
		}
		for (int inst = 0; inst < getNumIsoInstances(i); inst++){
			ParamsIso* iParams = getIsoParams(i,inst);
			iParams->reinit(doOverride);
			//Turn off rendering:
			bool wasEnabled = iParams->isEnabled();
			iParams->setEnabled(false);
			isoEventRouter->updateRenderer(iParams,wasEnabled,false);
		}
		for (int inst = 0; inst < getNumProbeInstances(i); inst++){
			ProbeParams* pParams = getProbeParams(i,inst);
			pParams->reinit(doOverride);
			probeEventRouter->updateRenderer(pParams,pParams->isEnabled(),false);
		}
		for (int inst = 0; inst < getNumTwoDImageInstances(i); inst++){
			TwoDImageParams* pParams = getTwoDImageParams(i,inst);
			pParams->reinit(doOverride);
			twoDImageEventRouter->updateRenderer(pParams,pParams->isEnabled(),false);
		}
		for (int inst = 0; inst < getNumTwoDDataInstances(i); inst++){
			TwoDDataParams* pParams = getTwoDDataParams(i,inst);
			pParams->reinit(doOverride);
			twoDDataEventRouter->updateRenderer(pParams,pParams->isEnabled(),false);
		}
		for (int inst = 0; inst < getNumFlowInstances(i); inst++){
			FlowParams* fParams = getFlowParams(i,inst);
			fParams->reinit(doOverride);
			bool wasEnabled = fParams->isEnabled();
			fParams->setEnabled(false);
			flowEventRouter->updateRenderer(fParams,wasEnabled,false);
		}
		if(animationParams[i]) animationParams[i]->reinit(doOverride);
		//setup near/far
		if (vpParams[i] && rgParams[i]) vizWin[i]->getGLWindow()->resetView(getRegionParams(i), getViewpointParams(i));
		if(!doOverride && vizWin[i]) vizWin[i]->getGLWindow()->removeAllRenderers();
	}

    //
    // Reinitialize tabs
    //
	flowEventRouter->reinitTab(doOverride);
	dvrEventRouter->reinitTab(doOverride);
	isoEventRouter->reinitTab(doOverride);
	probeEventRouter->reinitTab(doOverride);
	twoDImageEventRouter->reinitTab(doOverride);
	twoDDataEventRouter->reinitTab(doOverride);
	animationEventRouter->reinitTab(doOverride);
	viewpointEventRouter->reinitTab(doOverride);
	globalAnimationParams->reinit(doOverride);
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
			attrs[_vizWinNameAttr] = vizName[i].toAscii();
			oss.str(empty);

			QColor clr = vizWin[i]->getColorbarBackgroundColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizColorbarBackgroundColorAttr] = oss.str();

			oss.str(empty);
			clr = vizWin[i]->getElevGridColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizElevGridColorAttr] = oss.str();

			oss.str(empty);
			clr = vizWin[i]->getTimeAnnotColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizTimeAnnotColorAttr] = oss.str();
			
			oss.str(empty);
			int tt = vizWin[i]->getTimeAnnotType();
			if (tt == 2) oss << "timestamp";
			else if (tt == 1) oss << "timestep";
			else  oss << "none";
			attrs[_vizTimeAnnotTypeAttr] = oss.str();

			oss.str(empty);
			int ts = vizWin[i]->getTimeAnnotTextSize();
			oss << (long)ts;
			attrs[_vizTimeAnnotTextSizeAttr] = oss.str();

			oss.str(empty);
			float xpos = vizWin[i]->getTimeAnnotCoord(0);
			float ypos = vizWin[i]->getTimeAnnotCoord(1);
			oss << xpos << " " << ypos;
			attrs[_vizTimeAnnotCoordsAttr] = oss.str();
	
			oss.str(empty);
			if (vizWin[i]->axisArrowsAreEnabled()) oss<<"true";
				else oss << "false";
			attrs[_vizAxisArrowsEnabledAttr] = oss.str();

			oss.str(empty);
			if (vizWin[i]->elevGridRenderingEnabled()) oss<<"true";
				else oss << "false";
			attrs[_vizElevGridEnabledAttr] = oss.str();

			oss.str(empty);
			if (vizWin[i]->elevGridTextureEnabled()) oss<<"true";
				else oss << "false";
			attrs[_vizElevGridTexturedAttr] = oss.str();

			oss.str(empty);
				oss << vizWin[i]->getDisplacement();
			attrs[_vizElevGridDisplacementAttr] = oss.str();

			oss.str(empty);
			if (vizWin[i]->textureInverted()) oss << "true";
				else oss << "false";
			attrs[_vizElevGridInvertedAttr] = oss.str();

			oss.str(empty);
			oss << vizWin[i]->getTextureFile().toStdString();
			attrs[_vizElevGridTextureNameAttr] = oss.str();

			oss.str(empty);
			oss << vizWin[i]->getTextureRotation();
			attrs[_vizElevGridRotationAttr] = oss.str();

			oss.str(empty);
			oss << vizWin[i]->getElevGridRefinementLevel();
			attrs[_vizElevGridRefinementAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getAxisArrowCoord(0) << " "
				<< (float)vizWin[i]->getAxisArrowCoord(1) << " "
				<< (float)vizWin[i]->getAxisArrowCoord(2);
			attrs[_vizAxisPositionAttr] = oss.str();

			oss.str(empty);
			if (vizWin[i]->axisAnnotationIsEnabled())oss<<"true";
			else oss<<"false";
			attrs[_vizAxisAnnotationEnabledAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getAxisOriginCoord(0) << " "
				<< (float)vizWin[i]->getAxisOriginCoord(1) << " "
				<< (float)vizWin[i]->getAxisOriginCoord(2);
			attrs[_vizAxisOriginAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getMinTic(0) << " "
				<< (float)vizWin[i]->getMinTic(1) << " "
				<< (float)vizWin[i]->getMinTic(2);
			attrs[_vizMinTicAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getMaxTic(0) << " "
				<< (float)vizWin[i]->getMaxTic(1) << " "
				<< (float)vizWin[i]->getMaxTic(2);
			attrs[_vizMaxTicAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getTicLength(0) << " "
				<< (float)vizWin[i]->getTicLength(1) << " "
				<< (float)vizWin[i]->getTicLength(2);
			attrs[_vizTicLengthAttr] = oss.str();

			oss.str(empty);
			oss << (long)vizWin[i]->getNumTics(0) << " "
				<< (long)vizWin[i]->getNumTics(1) << " "
				<< (long)vizWin[i]->getNumTics(2);
			attrs[_vizNumTicsAttr] = oss.str();

			oss.str(empty);
			oss << (long)vizWin[i]->getTicDir(0) << " "
				<< (long)vizWin[i]->getTicDir(1) << " "
				<< (long)vizWin[i]->getTicDir(2);
			attrs[_vizTicDirAttr] = oss.str();

			oss.str(empty);
			oss << (float)vizWin[i]->getTicWidth();
			attrs[_vizTicWidthAttr] = oss.str();
			oss.str(empty);
			oss << (long)vizWin[i]->getLabelHeight();
			attrs[_vizLabelHeightAttr] = oss.str();
			oss.str(empty);
			oss << (long)vizWin[i]->getLabelDigits();
			attrs[_vizLabelDigitsAttr] = oss.str();
			oss.str(empty);
			clr = vizWin[i]->getAxisColor();
			oss << (long)clr.red() << " "
				<< (long)clr.green() << " "
				<< (long)clr.blue();
			attrs[_vizAxisColorAttr] = oss.str();

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
			//Now add local params.  One for each instance:
			for (int inst = 0; inst< getNumDvrInstances(i); inst++){
				XmlNode* dvrNode = getDvrParams(i,inst)->buildNode();
				if(dvrNode) locals->AddChild(dvrNode);
			}
			for (int inst = 0; inst< getNumIsoInstances(i); inst++){
				XmlNode* isoNode = getIsoParams(i,inst)->buildNode();
				if(isoNode) locals->AddChild(isoNode);
			}
			for (int inst = 0; inst< getNumFlowInstances(i); inst++){
				XmlNode* flowNode = getFlowParams(i,inst)->buildNode();
				if(flowNode) locals->AddChild(flowNode);
			}
			for (int inst = 0; inst< getNumProbeInstances(i); inst++){
				XmlNode* probeNode = getProbeParams(i,inst)->buildNode();
				if(probeNode) locals->AddChild(probeNode);
			}
			for (int inst = 0; inst< getNumTwoDImageInstances(i); inst++){
				XmlNode* twoDNode = getTwoDImageParams(i,inst)->buildNode();
				if(twoDNode) locals->AddChild(twoDNode);
			}
			for (int inst = 0; inst< getNumTwoDDataInstances(i); inst++){
				XmlNode* twoDNode = getTwoDDataParams(i,inst)->buildNode();
				if(twoDNode) locals->AddChild(twoDNode);
			}
			XmlNode* rgNode = rgParams[i]->buildNode();
			if(rgNode) locals->AddChild(rgNode);
			XmlNode* animNode = animationParams[i]->buildNode();
			if(animNode) locals->AddChild(animNode);
			XmlNode* vpNode = vpParams[i]->buildNode();
			if (vpNode) locals->AddChild(vpNode);
			
		}
	}
	return vizMgrNode;
}
//To parse, just get the visualizer name, and pass on the params-parsing
//The Visualizers node starts at depth 2, each visualizer is at depth 3
bool VizWinMgr::
elementStartHandler(ExpatParseMgr* pm, int depth, std::string& tag, const char ** attrs){
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
		
		QColor winBgColor = DataStatus::getBackgroundColor();
		QColor winRgColor = DataStatus::getRegionFrameColor();
		QColor winSubrgColor = DataStatus::getSubregionFrameColor();
		QColor winColorbarColor(Qt::white);
		QColor winElevGridColor(Qt::darkRed);
		QColor winTimeAnnotColor(Qt::white);
		float winTimeAnnotCoords[2] = {0.1f, 0.1f};
		int winTimeAnnotTextSize = 10;
		int winTimeAnnotType = 0;
		float axisPos[3], axisOriginPos[3];
		float minTic[3], maxTic[3], ticLength[3];
		int numTics[3], ticDir[3];
		axisPos[0]=axisPos[1]=axisPos[2]=0.f;
		minTic[0] = minTic[1] = minTic[2] = 0.f;
		maxTic[0] = maxTic[1] = maxTic[2] = 1.f;
		numTics[0] = numTics[1] = numTics[2] = 6;
		ticDir[0] = 1; ticDir[1] = 0; ticDir[2] = 0;
		ticLength[0] = ticLength[1] = ticLength[2] = 0.05f;
		axisOriginPos[0]=axisOriginPos[1]=axisOriginPos[2]=0.f;
		QColor axisColor(Qt::white);
		int labelHeight = 10;
		int labelDigits = 4;
		float ticWidth = 2.f;
		float colorbarLLPos[2], colorbarURPos[2];
		colorbarLLPos[0]=colorbarLLPos[1]=0.f;
		colorbarURPos[0]=0.1f;
		colorbarURPos[1]=0.3f;
		int colorbarTics = 11;
		bool axesEnabled = false;
		bool axisAnnotationEnabled = false;
		bool colorbarEnabled = false;
		bool regionEnabled = DataStatus::regionFrameIsEnabled();
		bool subregionEnabled = DataStatus::subregionFrameIsEnabled();
		bool elevGridEnabled = false;
		int elevGridRefinement = 0;
		bool elevGridInverted = false;
		bool elevGridTextured = false;
		int elevGridRotation = 0;
		float elevGridDisplacement = 0;
		string elevGridTexture = "imageFile.jpg";
		int numViz = -1;
		while (*attrs) {
			string attr = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);	
			if (StrCmpNoCase(attr, _vizWinNameAttr) == 0) {
				winName = value;
			} else if (StrCmpNoCase(attr, _vizTimeAnnotColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist >> g; ist >> b;
				winTimeAnnotColor.setRgb(r,g,b);
			} else if (StrCmpNoCase(attr, _vizTimeAnnotCoordsAttr) == 0) {
				ist >> winTimeAnnotCoords[0];
				ist >> winTimeAnnotCoords[1];
			} else if (StrCmpNoCase(attr, _vizTimeAnnotTextSizeAttr) == 0) {
				ist >> winTimeAnnotTextSize;
			} else if (StrCmpNoCase(attr, _vizTimeAnnotTypeAttr) == 0){
				if (value == "timestep") winTimeAnnotType = 1;
				else if (value == "timestamp") winTimeAnnotType = 2;
				else winTimeAnnotType = 0; //value = "none"
			}
			else if (StrCmpNoCase(attr, _vizBgColorAttr) == 0) {
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
			else if (StrCmpNoCase(attr, _vizElevGridColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist>>g; ist>>b;
				winElevGridColor.setRgb(r,g,b);
			}
			else if (StrCmpNoCase(attr, _vizElevGridRefinementAttr) == 0) {
				ist >> elevGridRefinement;
			}
			else if (StrCmpNoCase(attr, _vizElevGridRotationAttr) == 0) {
				ist >> elevGridRotation;
			}
			else if (StrCmpNoCase(attr, _vizElevGridInvertedAttr) == 0) {
				if (value == "true") elevGridInverted = true; 
				else elevGridInverted = false; 
			}
			else if (StrCmpNoCase(attr, _vizElevGridTexturedAttr) == 0) {
				if (value == "true") elevGridTextured = true; 
				else elevGridTextured = false; 
			}
			else if (StrCmpNoCase(attr, _vizElevGridTextureNameAttr) == 0) {
				elevGridTexture = value;
			}
			else if (StrCmpNoCase(attr, _vizElevGridDisplacementAttr) == 0) {
				ist >> elevGridDisplacement; 
			}
			else if (StrCmpNoCase(attr, _vizAxisPositionAttr) == 0) {
				ist >> axisPos[0]; ist>>axisPos[1]; ist>>axisPos[2];
			}
			else if (StrCmpNoCase(attr, _vizAxisOriginAttr) == 0) {
				ist >> axisOriginPos[0]; ist>>axisOriginPos[1]; ist>>axisOriginPos[2];
			}
			else if (StrCmpNoCase(attr, _vizMinTicAttr) == 0) {
				ist >> minTic[0]; ist>>minTic[1]; ist>>minTic[2];
			}
			else if (StrCmpNoCase(attr, _vizMaxTicAttr) == 0) {
				ist >> maxTic[0]; ist>>maxTic[1]; ist>>maxTic[2];
			}
			else if (StrCmpNoCase(attr, _vizNumTicsAttr) == 0) {
				ist >> numTics[0]; ist>>numTics[1]; ist>>numTics[2];
			}
			else if (StrCmpNoCase(attr, _vizTicLengthAttr) == 0) {
				ist >> ticLength[0]; ist>>ticLength[1]; ist>>ticLength[2];
			}
			else if (StrCmpNoCase(attr, _vizTicDirAttr) == 0) {
				ist >> ticDir[0]; ist>>ticDir[1]; ist>>ticDir[2];
			}
			else if (StrCmpNoCase(attr, _vizLabelHeightAttr) == 0) {
				ist >> labelHeight;
			}
			else if (StrCmpNoCase(attr, _vizLabelDigitsAttr) == 0) {
				ist >> labelDigits;
			}
			else if (StrCmpNoCase(attr, _vizTicWidthAttr) == 0) {
				ist >> ticWidth;
			}
			else if (StrCmpNoCase(attr, _vizAxisColorAttr) == 0) {
				int r,g,b;
				ist >> r; ist>>g; ist>>b;
				axisColor.setRgb(r,g,b);
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
			else if (StrCmpNoCase(attr, _vizAxisArrowsEnabledAttr) == 0) {
				if (value == "true") axesEnabled = true; 
				else axesEnabled = false; 
			}
			else if (StrCmpNoCase(attr, _vizAxisAnnotationEnabledAttr) == 0) {
				if (value == "true") axisAnnotationEnabled = true; 
				else axisAnnotationEnabled = false; 
			}
			else if (StrCmpNoCase(attr, _vizElevGridEnabledAttr) == 0) {
				if (value == "true") elevGridEnabled = true; 
				else elevGridEnabled = false; 
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
		parsingDvrInstance = -1;
		parsingIsoInstance = -1;
		parsingProbeInstance = -1;
		parsingTwoDImageInstance = -1;
		parsingTwoDDataInstance = -1;
		parsingFlowInstance = -1;
		vizWin[parsingVizNum]->setBackgroundColor(winBgColor);
		vizWin[parsingVizNum]->setRegionFrameColor(winRgColor);
		vizWin[parsingVizNum]->setSubregionFrameColor(winSubrgColor);
		vizWin[parsingVizNum]->setColorbarBackgroundColor(winColorbarColor);
		vizWin[parsingVizNum]->enableAxisArrows(axesEnabled);
		vizWin[parsingVizNum]->enableAxisAnnotation(axisAnnotationEnabled);
		vizWin[parsingVizNum]->enableColorbar(colorbarEnabled);
		vizWin[parsingVizNum]->enableRegionFrame(regionEnabled);
		vizWin[parsingVizNum]->enableSubregionFrame(subregionEnabled);
		vizWin[parsingVizNum]->enableElevGridRendering(elevGridEnabled);
		vizWin[parsingVizNum]->setElevGridRefinementLevel(elevGridRefinement);
		vizWin[parsingVizNum]->setElevGridColor(winElevGridColor);
		vizWin[parsingVizNum]->setDisplacement(elevGridDisplacement);
		vizWin[parsingVizNum]->enableElevGridTexture(elevGridTextured);
		vizWin[parsingVizNum]->rotateTexture(elevGridRotation);
		vizWin[parsingVizNum]->invertTexture(elevGridInverted);
		vizWin[parsingVizNum]->setTextureFile(QString(elevGridTexture.c_str()));

		vizWin[parsingVizNum]->setTimeAnnotColor(winTimeAnnotColor);
		vizWin[parsingVizNum]->setTimeAnnotCoords(winTimeAnnotCoords);
		vizWin[parsingVizNum]->setTimeAnnotTextSize(winTimeAnnotTextSize);
		vizWin[parsingVizNum]->setTimeAnnotType(winTimeAnnotType);
		for (int j = 0; j< 3; j++){
			vizWin[parsingVizNum]->setAxisArrowCoord(j, axisPos[j]);
			vizWin[parsingVizNum]->setAxisOriginCoord(j, axisOriginPos[j]);
			vizWin[parsingVizNum]->setNumTics(j, numTics[j]);
			vizWin[parsingVizNum]->setMinTic(j, minTic[j]);
			vizWin[parsingVizNum]->setMaxTic(j, maxTic[j]);
			vizWin[parsingVizNum]->setTicLength(j, ticLength[j]);
			vizWin[parsingVizNum]->setTicDir(j, ticDir[j]);
		}
		vizWin[parsingVizNum]->setLabelHeight(labelHeight);
		vizWin[parsingVizNum]->setLabelDigits(labelDigits);
		vizWin[parsingVizNum]->setTicWidth(ticWidth);
		vizWin[parsingVizNum]->setAxisColor(axisColor);
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
			//Initially there is one dvrparams (when the window is created) 
			//If this is not the first, need to create one, and then parse it.
			parsingDvrInstance++;
			DvrParams* dParams;
			if (parsingDvrInstance > 0){
				//create a new params
				dParams = new DvrParams(parsingVizNum);
				dvrParamsInstances[parsingVizNum].push_back(dParams);
			} else {
				dParams = getDvrParams(parsingVizNum,0);
			}
			assert(getNumDvrInstances(parsingVizNum) == (parsingDvrInstance+1));
			
			//Need to "push" to dvr parser.
			//That parser will "pop" back to vizwinmgr when done.

			dvrEventRouter->cleanParams(dParams);
			pm->pushClassStack(dParams);
			dParams->elementStartHandler(pm, depth, tag, attrs);
			//Put the first instance in the GLWindow
			if(parsingDvrInstance == 0) vizWin[parsingVizNum]->getGLWindow()->setActiveDvrParams(dParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_isoParamsTag) == 0){
			//Initially there is one dvrparams (when the window is created) 
			//If this is not the first, need to create one, and then parse it.
			parsingIsoInstance++;
			ParamsIso* iParams;
			if (parsingIsoInstance > 0){
				//create a new params
				iParams = new ParamsIso(NULL, parsingVizNum);
				isoParamsInstances[parsingVizNum].push_back(iParams);
			} else {
				iParams = getIsoParams(parsingVizNum,0);
			}
			assert(getNumIsoInstances(parsingVizNum) == (parsingIsoInstance+1));
			
			//Need to "push" to iso parser.
			//That parser will "pop" back to vizwinmgr when done.

			isoEventRouter->cleanParams(iParams);
			pm->pushClassStack(iParams);
			iParams->elementStartHandler(pm, depth, tag, attrs);
			//Put the first instance in the GLWindow
			if(parsingIsoInstance == 0) vizWin[parsingVizNum]->getGLWindow()->setActiveIsoParams(iParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_probeParamsTag) == 0){
			parsingProbeInstance++;
			ProbeParams* pParams;
			if (parsingProbeInstance > 0){
				//create a new params
				pParams = new ProbeParams(parsingVizNum);
				probeParamsInstances[parsingVizNum].push_back(pParams);
			} else {
				pParams = getProbeParams(parsingVizNum,0);
			}
			assert(getNumProbeInstances(parsingVizNum) == (parsingProbeInstance+1));
			
			//Need to "push" to probe parser.
			//That parser will "pop" back to vizwinmgr when done.

			probeEventRouter->cleanParams(pParams);
			pm->pushClassStack(pParams);
			pParams->elementStartHandler(pm, depth, tag, attrs);
			if(parsingProbeInstance == 0)
				vizWin[parsingVizNum]->getGLWindow()->setActiveProbeParams(pParams);
			return true;
			
		} else if (StrCmpNoCase(tag, Params::_twoDImageParamsTag) == 0){
			parsingTwoDImageInstance++;
			TwoDImageParams* pParams;
			if (parsingTwoDImageInstance > 0){
				//create a new params
				pParams = new TwoDImageParams(parsingVizNum);
				twoDImageParamsInstances[parsingVizNum].push_back(pParams);
			} else {
				pParams = getTwoDImageParams(parsingVizNum,0);
			}
			assert(getNumTwoDImageInstances(parsingVizNum) == (parsingTwoDImageInstance+1));
			
			//Need to "push" to TwoD parser.
			//That parser will "pop" back to vizwinmgr when done.

			twoDImageEventRouter->cleanParams(pParams);
			pm->pushClassStack(pParams);
			pParams->elementStartHandler(pm, depth, tag, attrs);
			if(parsingTwoDImageInstance == 0)
				vizWin[parsingVizNum]->getGLWindow()->setActiveTwoDImageParams(pParams);
			return true;
			
		} else if (StrCmpNoCase(tag, Params::_twoDDataParamsTag) == 0 ||
			StrCmpNoCase(tag, Params::_twoDParamsTag) == 0){
			parsingTwoDDataInstance++;
			TwoDDataParams* pParams;
			if (parsingTwoDDataInstance > 0){
				//create a new params
				pParams = new TwoDDataParams(parsingVizNum);
				twoDDataParamsInstances[parsingVizNum].push_back(pParams);
			} else {
				pParams = getTwoDDataParams(parsingVizNum,0);
			}
			assert(getNumTwoDDataInstances(parsingVizNum) == (parsingTwoDDataInstance+1));
			
			//Need to "push" to TwoD parser.
			//That parser will "pop" back to vizwinmgr when done.

			twoDDataEventRouter->cleanParams(pParams);
			pm->pushClassStack(pParams);
			pParams->elementStartHandler(pm, depth, tag, attrs);
			if(parsingTwoDDataInstance == 0)
				vizWin[parsingVizNum]->getGLWindow()->setActiveTwoDDataParams(pParams);
			return true;
			
		} else if (StrCmpNoCase(tag, Params::_regionParamsTag) == 0){
			//Need to "push" to region parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(rgParams[parsingVizNum]);
			rgParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (rgParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setActiveRegionParams(rgParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setActiveRegionParams(globalRegionParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_viewpointParamsTag) == 0){
			//Need to "push" to viewpoint parser.
			//That parser will "pop" back to session when done.
			pm->pushClassStack(vpParams[parsingVizNum]);
			vpParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (vpParams[parsingVizNum]->isLocal()){
				//Necessary to reset trackball here, since
				//It was not set to local in the constructor.
				vizWin[parsingVizNum]->getGLWindow()->setActiveViewpointParams(vpParams[parsingVizNum]);
				vizWin[parsingVizNum]->setGlobalViewpoint(false);
			}
			else 
				vizWin[parsingVizNum]->getGLWindow()->setActiveViewpointParams(globalVPParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_animationParamsTag) == 0){
			//Need to "push" to animation parser.
			//That parser will "pop" back to session when done.
			
			pm->pushClassStack(animationParams[parsingVizNum]);
			animationParams[parsingVizNum]->elementStartHandler(pm, depth, tag, attrs);
			if (animationParams[parsingVizNum]->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setActiveAnimationParams(animationParams[parsingVizNum]);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setActiveAnimationParams(globalAnimationParams);
			return true;
		} else if (StrCmpNoCase(tag, Params::_flowParamsTag) == 0){
			parsingFlowInstance++;
			FlowParams* fParams;
			if (parsingFlowInstance > 0){
				//create a new params
				fParams = new FlowParams(parsingVizNum);
				flowParamsInstances[parsingVizNum].push_back(fParams);
			} else {
				fParams = getFlowParams(parsingVizNum,0);
			}
			assert(getNumFlowInstances(parsingVizNum) == (parsingFlowInstance+1));
			
			//Need to "push" to flow parser.
			//That parser will "pop" back to vizwinmgr when done.

			flowEventRouter->cleanParams(fParams);
			pm->pushClassStack(fParams);
			fParams->elementStartHandler(pm, depth, tag, attrs);
			if (parsingFlowInstance == 0)
				vizWin[parsingVizNum]->getGLWindow()->setActiveFlowParams(fParams);
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
			//If there are multiple visualizers, show them all:
			//Tile if more than one visualizer:
			if(getNumVisualizers() > 1) {
				//Make the last one active, since the vizwin will
				//try to do this anyway...
				setActiveViz(getNumVisualizers()-1);
				fitSpace();
			}
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
//General function for non-render dirty bit setting:
//Sets the dirty bit in all windows that are using the specified params.
void VizWinMgr::setVizDirty(Params* p, DirtyBitType bittype, bool bit, bool refresh){
	if (!(DataStatus::getInstance()->getDataMgr())) return;
	VizWin* vw;
	if (p->getVizNum()>= 0){
		vw = getVizWin(p->getVizNum());
		vw->setDirtyBit(bittype, bit);
		if(bit&&refresh) vw->updateGL();
	} else if (!p->isRenderParams()) {
		//Need to check all the windows whose params are global,
		Params** paramArray = getParamsArray(p->getParamType());
		for (int i = 0; i<MAXVIZWINS; i++){
			if (!paramArray[i] || paramArray[i]->isLocal()) continue;
			vw = getVizWin(i);
			vw->setDirtyBit(bittype, bit);
			if(bit&&refresh) vw->updateGL();
		}
	}
}
//Special cases for renderer dirty-bit setting
void VizWinMgr::setClutDirty(RenderParams* p){
	if (!(DataStatus::getInstance()->getDataMgr())) return;
	VizWin* vw = getVizWin(p->getVizNum());
	if (!vw) return;
	GLWindow* glwin = vw->getGLWindow();
	if (!glwin) return;
	Renderer* ren = glwin->getRenderer(p);
	if (!ren) return;
	ren->setClutDirty();
	p->setAllBypass(false);
	vw->updateGL();
}
void VizWinMgr::setDatarangeDirty(RenderParams* p){
	if (!(DataStatus::getInstance()->getDataMgr())) return;
	VizWin* vw = getVizWin(p->getVizNum());
	if (!vw) return;
	GLWindow* glwin = vw->getGLWindow();
	if (!glwin) return;
	VolumeRenderer* volRend = (VolumeRenderer*)glwin->getRenderer(p);
	if (!volRend)return;
	volRend->setDatarangeDirty();
	p->setAllBypass(false);
	vw->updateGL();
}
void VizWinMgr::setFlowGraphicsDirty(FlowParams* p){
	if (!(DataStatus::getInstance()->getDataMgr())) return;
	VizWin* vw = getVizWin(p->getVizNum());
	if (!vw) return;
	GLWindow* glwin = vw->getGLWindow();
	if (!glwin) return;
	FlowRenderer* flowRend = (FlowRenderer*)glwin->getRenderer(p);
	if (!flowRend)return;
	flowRend->setGraphicsDirty();
	p->setAllBypass(false);
	vw->updateGL();
}
//Set all the twoD (data and image) renderers dirty,
//So they will all reconstruct their terrain mappings
void VizWinMgr::setAllTwoDElevDirty(){
	for (int i = 0; i<MAXVIZWINS; i++){
		if (vizWin[i]){
			for (int j = 0; j< twoDDataParamsInstances[i].size(); j++){
				TwoDDataParams* dParams = twoDDataParamsInstances[i][j];
				dParams->setElevGridDirty(true);
			}
			for (int j = 0; j< twoDImageParamsInstances[i].size(); j++){
				TwoDImageParams* dParams = twoDImageParamsInstances[i][j];
				dParams->setElevGridDirty(true);
			}
		}
	}
}
void VizWinMgr::setFlowDataDirty(FlowParams* p, bool doInterrupt){
	if (!(DataStatus::getInstance()->getDataMgr())) return;
	VizWin* vw = getVizWin(p->getVizNum());
	if (!vw) return;
	GLWindow* glwin = vw->getGLWindow();
	if (!glwin) return;
	FlowRenderer* flowRend = (FlowRenderer*)glwin->getRenderer(p);
	if(flowRend)
		flowRend->setDataDirty(doInterrupt);
	p->setAllBypass(false);
	vw->updateGL();
}

bool VizWinMgr::flowDataIsDirty(FlowParams* p){
	VizWin* vw = getVizWin(p->getVizNum());
	if (!vw) return false;
	GLWindow* glwin = vw->getGLWindow();
	if(!glwin) return false;
	FlowRenderer* flowRend = (FlowRenderer*)glwin->getRenderer(p);
	if (!flowRend) return false;
	if (p->getFlowType()==1){
		return (flowRend->allFlowDataIsDirty());
	}
	int timeStep = getAnimationParams(p->getVizNum())->getCurrentFrameNumber();
	return flowRend->flowDataIsDirty(timeStep);
}

Params** VizWinMgr::getParamsArray(Params::ParamType t){
	switch(t){
		//Not supported for renderParams
		case Params::AnimationParamsType :
			return (Params**)animationParams;
		case Params::RegionParamsType :
			return (Params**)rgParams;
		case Params::ViewpointParamsType :
			return (Params**)vpParams;
		default:
			assert(0);
			return 0;
	}
}

void VizWinMgr::setCurrentInstanceIndex(int winnum, int inst, Params::ParamType t){
	switch(t) {
		case Params::DvrParamsType :
			setCurrentDvrInstIndex(winnum,inst);
			break;
		case Params::IsoParamsType :
			setCurrentIsoInstIndex(winnum,inst);
			break;
		case Params::ProbeParamsType :
			setCurrentProbeInstIndex(winnum, inst);
			break;
		case Params::TwoDImageParamsType :
			setCurrentTwoDImageInstIndex(winnum, inst);
			break;
		case Params::TwoDDataParamsType :
			setCurrentTwoDDataInstIndex(winnum, inst);
			break;
		case Params::FlowParamsType :
			setCurrentFlowInstIndex(winnum, inst);
			break;
		default :
			assert(0);
			break;
	}
}
int VizWinMgr::findInstanceIndex(int winnum, Params* rParams, Params::ParamType ptype){
	switch(ptype) {
		case Params::DvrParamsType :
			for (unsigned int i = 0; i< dvrParamsInstances[winnum].size(); i++){
				if (dvrParamsInstances[winnum][i] == rParams)
					return i;
			}
			return -1;
		case Params::IsoParamsType :
			for (unsigned int i = 0; i< isoParamsInstances[winnum].size(); i++){
				if (isoParamsInstances[winnum][i] == rParams)
					return i;
			}
			return -1;
		case Params::ProbeParamsType :
			for (unsigned int i = 0; i< probeParamsInstances[winnum].size(); i++){
				if (probeParamsInstances[winnum][i] == rParams)
					return i;
			}
			return -1;
		case Params::TwoDImageParamsType :
			for (unsigned int i = 0; i< twoDImageParamsInstances[winnum].size(); i++){
				if (twoDImageParamsInstances[winnum][i] == rParams)
					return i;
			}
			return -1;
		case Params::TwoDDataParamsType :
			for (unsigned int i = 0; i< twoDDataParamsInstances[winnum].size(); i++){
				if (twoDDataParamsInstances[winnum][i] == rParams)
					return i;
			}
			return -1;
		case Params::FlowParamsType :
			for (unsigned int i = 0; i< flowParamsInstances[winnum].size(); i++){
				if (flowParamsInstances[winnum][i] == rParams)
					return i;
			}
			return -1;
		default :
			assert(0);
			return -1;
	}
}
int VizWinMgr::getCurrentInstanceIndex(int winnum, Params::ParamType t){
	switch(t) {
		case Params::DvrParamsType :
			return getCurrentDvrInstIndex(winnum);
		case Params::IsoParamsType :
			return getCurrentIsoInstIndex(winnum);
		case Params::ProbeParamsType :
			return getCurrentProbeInstIndex(winnum);
		case Params::TwoDImageParamsType :
			return getCurrentTwoDImageInstIndex(winnum);
		case Params::TwoDDataParamsType :
			return getCurrentTwoDDataInstIndex(winnum);
		case Params::FlowParamsType :
			return getCurrentFlowInstIndex(winnum);
		default :
			assert(0);
			return -1;
	}
}
void VizWinMgr::insertInstance(int winnum, int inst, Params* newParams){
	
	switch(newParams->getParamType()) {
		case Params::DvrParamsType :
			insertDvrInstance(winnum, inst, (DvrParams*)newParams);
			break;
		case Params::IsoParamsType :
			insertIsoInstance(winnum, inst, (ParamsIso*)newParams);
			break;
		case Params::ProbeParamsType :
			insertProbeInstance(winnum, inst, (ProbeParams*)newParams);
			break;
		case Params::TwoDImageParamsType :
			insertTwoDImageInstance(winnum, inst, (TwoDImageParams*)newParams);
			break;
		case Params::TwoDDataParamsType :
			insertTwoDDataInstance(winnum, inst, (TwoDDataParams*)newParams);
			break;
		case Params::FlowParamsType :
			insertFlowInstance(winnum, inst, (FlowParams*)newParams);
			break;
		default :
			assert(0);
			break;
	}
}
void VizWinMgr::appendInstance(int winnum, Params* newParams){
	switch(newParams->getParamType()) {
		case Params::DvrParamsType :
			appendDvrInstance(winnum, (DvrParams*)newParams);
			break;
		case Params::IsoParamsType :
			appendIsoInstance(winnum, (ParamsIso*)newParams);
			break;
		case Params::ProbeParamsType :
			appendProbeInstance(winnum, (ProbeParams*)newParams);
			break;
		case Params::TwoDImageParamsType :
			appendTwoDImageInstance(winnum, (TwoDImageParams*)newParams);
			break;
		case Params::TwoDDataParamsType :
			appendTwoDDataInstance(winnum, (TwoDDataParams*)newParams);
			break;
		case Params::FlowParamsType :
			appendFlowInstance(winnum, (FlowParams*)newParams);
			break;
		default :
			assert(0);
			break;
	}
}
void VizWinMgr::removeInstance(int winnum, int instance, Params::ParamType t){
	switch(t) {
		case Params::DvrParamsType :
			removeDvrInstance(winnum, instance);
			break;
		case Params::IsoParamsType :
			removeIsoInstance(winnum, instance);
			break;
		case Params::ProbeParamsType :
			removeProbeInstance(winnum, instance);
			break;
		case Params::TwoDImageParamsType :
			removeTwoDImageInstance(winnum, instance);
			break;
		case Params::TwoDDataParamsType :
			removeTwoDDataInstance(winnum, instance);
			break;
		case Params::FlowParamsType :
			removeFlowInstance(winnum, instance);
			break;
		default :
			assert(0);
			break;
	}
}
Params* VizWinMgr::getParams(int winnum, Params::ParamType pType, int instance ){
	switch (pType) {
		case Params::DvrParamsType :
			return getDvrParams(winnum, instance);
		case Params::IsoParamsType :
			return getIsoParams(winnum, instance);
		case Params::ProbeParamsType :
			return getProbeParams(winnum, instance);
		case Params::TwoDImageParamsType :
			return getTwoDImageParams(winnum, instance);
		case Params::TwoDDataParamsType :
			return getTwoDDataParams(winnum, instance);
		case Params::FlowParamsType :
			return getFlowParams(winnum, instance);
		case Params::ViewpointParamsType :
			return getViewpointParams(winnum);
		case Params::RegionParamsType :
			return getRegionParams(winnum);
		case Params::AnimationParamsType :
			return getAnimationParams(winnum);
		default :
			assert(0);
			return 0;
	}
}
int VizWinMgr::getNumInstances(int winnum, Params::ParamType pType){
	switch (pType) {
		case Params::DvrParamsType :
			return getNumDvrInstances(winnum);
		case Params::IsoParamsType :
			return getNumIsoInstances(winnum);
		case Params::ProbeParamsType :
			return getNumProbeInstances(winnum);
		case Params::TwoDImageParamsType :
			return getNumTwoDImageInstances(winnum);
		case Params::TwoDDataParamsType :
			return getNumTwoDDataInstances(winnum);
		case Params::FlowParamsType :
			return getNumFlowInstances(winnum);
		default:
			assert(0);
			return -1;
	}
}
void VizWinMgr::setInteractiveNavigating(int level){
	DataStatus::setInteractiveRefinementLevel(level);
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]) vizWin[i]->setDirtyBit(DvrRegionBit, true);
	}
}
//Determine (for error checking) if there are any enabled 2d renderers in the window that match
//coordinate, terrain mapping
bool VizWinMgr::findCoincident2DSurface(int vizwin, int orientation, float coordinate, bool terrainMapped){
	vector<TwoDDataParams*> dparams = twoDDataParamsInstances[vizwin];
	vector<TwoDImageParams*> iparams = twoDImageParamsInstances[vizwin];
	const float * extents = DataStatus::getInstance()->getExtents();
	float tol = (extents[orientation+3]-extents[orientation])*0.0001f;
	for (int i = 0; i< dparams.size(); i++){
		TwoDDataParams* p = dparams[i];
		if (!p->isEnabled()) continue;
		if (p->getOrientation() != orientation) continue;
		if (p->isMappedToTerrain() != terrainMapped) continue;
		if (abs(p->getTwoDMin(orientation) - coordinate)> tol) continue;
		return true;
	}
	for (int i = 0; i< iparams.size(); i++){
		TwoDImageParams* p = iparams[i];
		if (!p->isEnabled()) continue;
		if (p->getOrientation() != orientation) continue;
		if (p->isMappedToTerrain() != terrainMapped) continue;
		if (abs(p->getTwoDMin(orientation) - coordinate)> tol) continue;
		return true;
	}
	return false;
}

