//************************************************************************
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
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
#include "glutil.h"	// Must be included first!!!
#include <qapplication.h>
#include <QIcon>
#include <qcursor.h>
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <QMdiArea>
#include <QMdiSubWindow>
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
#include <typeinfo>
#include "GL/glew.h"
#include "visualizer.h"
#include "vapor/ControlExecutive.h"
#include "vizwin.h"
#include "vizwinmgr.h"

#include "math.h"
#include "mainform.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "viztab.h"
#include "tabmanager.h"
#include "params.h"
#include "renderer.h"

#include "vapor/ExtensionClasses.h"

#include "animationparams.h"

#include "assert.h"


#include "animationeventrouter.h"
#include "regioneventrouter.h"

#include "viewpointeventrouter.h"

#include "eventrouter.h"
//Extension tabs also included (until we find a nicer way to separate extensions)
#include "arroweventrouter.h"
#include <vapor/XmlNode.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/ParamNode.h>
#include "images/probe.xpm"
#include "images/twoDData.xpm"
#include "images/twoDImage.xpm"
#include "images/rake.xpm"
#include "images/wheel.xpm"
#include "images/cube.xpm"

using namespace VAPoR;
TabManager* VizWinMgr::tabManager = 0;
VizWinMgr* VizWinMgr::theVizWinMgr = 0;
std::map<ParamsBase::ParamsBaseType, EventRouter*> VizWinMgr::eventRouterMap;


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
const string VizWinMgr::_vizColorbarFontsizeAttr = "ColorbarFontsize";
const string VizWinMgr::_vizColorbarDigitsAttr = "ColorbarDigits";
const string VizWinMgr::_vizColorbarLLPositionAttr = "ColorbarLLPosition";
const string VizWinMgr::_vizColorbarURPositionAttr = "ColorbarURPosition";
const string VizWinMgr::_vizColorbarNumTicsAttr = "ColorbarNumTics";
const string VizWinMgr::_vizAxisArrowsEnabledAttr = "AxesEnabled";
const string VizWinMgr::_vizAxisAnnotationEnabledAttr = "AxisAnnotationEnabled";
const string VizWinMgr::_vizColorbarEnabledAttr = "ColorbarEnabled";
const string VizWinMgr::_vizColorbarParamsNameAttr = "ColorbarParamsTag";
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
	tabManager = MainForm::getTabManager();
   
	activeViz = -1;
	activationCount = 0;
    benchmark = DONE;
    benchmarkTimer = NULL;
	
    for (int i = 0; i< MAXVIZWINS; i++){
        vizWin[i] = 0;
		vizMdiWin[i] = 0;
        vizName[i] = "";
        //vizRect[i] = 0;
        isMax[i] = false;
        isMin[i] = false;
		
		for (int j = 1; j<= Params::GetNumParamsClasses(); j++){
			Params::SetCurrentParamsInstanceIndex(j,i,-1);
		}
		
		activationOrder[i] = -1;
    }
	setSelectionMode(Visualizer::navigateMode);
}
//Create the global params and the default renderer params:
void VizWinMgr::
createAllDefaultTabs() {

	//Install Extension Tabs
	InstallTab(ArrowParams::_arrowParamsTag, ArrowEventRouter::CreateTab);
	//Install built-in tabs
	InstallTab(Params::_animationParamsTag, AnimationEventRouter::CreateTab);
	InstallTab(Params::_viewpointParamsTag, ViewpointEventRouter::CreateTab);
	InstallTab(Params::_regionParamsTag, RegionEventRouter::CreateTab);
	
	
	
	//Provide default tab ordering if needed:
	vector<long> defaultOrdering = TabManager::getTabOrdering();
	vector<long> tempOrdering;
	//The tab manager needs to be refreshed once with all the tabs in place;  
	tabManager->setTabOrdering(tempOrdering);
	tabManager->orderTabs();
	//Now insert just the desired tabs:
	tabManager->setTabOrdering(defaultOrdering);
	tabManager->orderTabs();
	//Create a default parameter set
	//For non-renderer params, the default ones are the global ones.
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params::SetDefaultParams(i, Params::CreateDefaultParams(i));
	}
	
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
			delete vizMdiWin[i];
		}
	}
	
	
}
void VizWinMgr::
vizAboutToDisappear(int i)  {
   

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
	vizMdiWin[i] = 0;
	
	//If we are deleting the active viz, revert to the 
	//most recent active viz.
	if (activeViz == i) {
		int newActive = getLastActive();
		if (newActive >= 0 && vizWin[newActive]) setActiveViz(newActive);
		else activeViz = -1;
	}
	
	
	if(activeViz >= 0) setActiveViz(activeViz);
	
	for (int j = 1; j<= Params::GetNumParamsClasses(); j++){
		for (int k = Params::GetNumParamsInstances(j,i)-1; k >=0; k--){
			getEventRouter(j)->cleanParams(Params::GetParamsInstance(j,i,k));
			Params::RemoveParamsInstance(j,i,k);
		}
	}


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
    
	//Default name is just "Visualizer No. N"
    	//qWarning("Creating new visualizer in position %d",useWindowNum);
	if (strlen(newName) != 0) vizName[useWindowNum] = newName;
	else vizName[useWindowNum] = ((QString("Visualizer No. ")+QString::number(useWindowNum)));
	emit (newViz(vizName[useWindowNum], useWindowNum));
	vizWin[useWindowNum] = new VizWin (MainForm::getInstance(), vizName[useWindowNum], 0, this, newRect, useWindowNum);
	vizMdiWin[useWindowNum]=MainForm::getInstance()->getMDIArea()->addSubWindow(vizWin[useWindowNum]);
	
	vizWin[useWindowNum]->setWindowNum(useWindowNum);
	

	vizWin[useWindowNum]->setFocusPolicy(Qt::ClickFocus);

	//Following seems to be unnecessary on windows and irix:
	activeViz = useWindowNum;
	setActiveViz(useWindowNum);

	vizWin[useWindowNum]->showMaximized();
	
	//Tile if more than one visualizer:
	if(numVizWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	
	
	emit activateViz(useWindowNum);
	vizWin[useWindowNum]->show();

	
	
	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numVizWins > 1){
		emit enableMultiViz(true);
	}

	//Initialize axis annotation to use full user extents:
	const float *extents = DataStatus::getInstance()->getLocalExtents();
	vector<double>tvExts;

	if (DataStatus::getInstance()->getDataMgr())
		tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents();
	else {
		for (int i = 0; i<3; i++) tvExts.push_back(0.0);
		for (int i = 0; i<3; i++) tvExts.push_back(1.0);
	}
	

	
	return useWindowNum;
}
/*
 *  Create params, for a new visualizer.  
 */
void VizWinMgr::
createDefaultParams(int winnum){
	
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params* q = Params::GetDefaultParams(i);
		//if (!q->isRenderParams()) continue;
		Params* p = q->deepCopy();
		p->setVizNum(winnum);
		//Check for strange error:
		assert( Params::GetNumParamsInstances(i,winnum) == 0);
		Params::AppendParamsInstance(i,winnum,p);
		Params::SetCurrentParamsInstanceIndex(i,winnum,0);
		if (!p->isRenderParams())p->setLocal(false);
		else if(DataStatus::getInstance()->getDataMgr()){
			EventRouter* eRouter = getEventRouter(i);
			eRouter->setEditorDirty((RenderParams*)p);
		}
	}
	
	
}
//Replace a specific global params with specified one
//
void VizWinMgr::replaceGlobalParams(Params* p, ParamsBase::ParamsBaseType t){
	
	Params* oldP = Params::GetDefaultParams(t);
	if (oldP) delete oldP;
	Params::SetDefaultParams(t,p);
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


/**************************************************************
 * Methods that arrange the viz windows:
 **************************************************************/
void 
VizWinMgr::cascade(){
	
   	myMDIArea->cascadeSubWindows(); 
	//Now size them up to a reasonable size:
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			vizWin[i]->resize(400,400);
		}	
	} 
	
}
void 
VizWinMgr::coverRight(){
	
    for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]) {
			vizWin[i]->showMaximized();
			
		}
	} 
	
}

void 
VizWinMgr::fitSpace(){
	
    myMDIArea->tileSubWindows();
	
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
		//Set the animation toolbar to the correct frame number:
		int currentTS = getActiveAnimationParams()->getCurrentTimestep();
		MainForm::getInstance()->setCurrentTimestep(currentTS);
		//Tell the Visualizers who is active
		Visualizer::setActiveWinNum(vizNum);
		//Determine if the active viz is sharing the region
		Visualizer::setRegionShareFlag(!getRegionParams(vizNum)->isLocal());

		tabManager->show();
		//Add to history if this is not during initial creation.
		
		//Need to cause a redraw in all windows if we are not in navigate mode,
		//So that the manips will change where they are drawn:
		if (Visualizer::getCurrentMouseMode() != Visualizer::navigateMode){
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
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		getEventRouter(i)->updateTab();
	}
	
	//Also update the activeParams in the Visualizer:
	Visualizer* glwin = getVizWin(activeViz)->getVisualizer();
	
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params* p = Params::GetCurrentParamsInstance(i, activeViz);
		if (!p->isLocal()) p = Params::GetDefaultParams(i);
		glwin->setActiveParams(p, i);
	}
	glwin->setActiveViewpointParams(getViewpointParams(activeViz));
}

//Method to enable closing of a vizWin for Undo/Redo
void VizWinMgr::
killViz(int viznum){
	
	MainForm::getInstance()->getMDIArea()->removeSubWindow(vizMdiWin[viznum]);
	vizWin[viznum]->close();
	
	vizWin[viznum] = 0;
	
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

/*******************************************************************
 *	Slots associated with VizTab:
 ********************************************************************/

void VizWinMgr::
home()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	
	getViewpointRouter()->useHomeViewpoint();
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
sethome()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	
	getViewpointRouter()->setHomeViewpoint();
}
void VizWinMgr::
viewAll()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	
	getViewpointRouter()->guiCenterFullRegion(getRegionParams(activeViz));
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
viewRegion()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	
	getViewpointRouter()->guiCenterSubRegion(getRegionParams(activeViz));
	setViewerCoordsChanged(getViewpointParams(activeViz));
}
void VizWinMgr::
alignView(int axis)
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
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
		if (!vizWin[i]) continue;
		RegionParams* rgParams = (RegionParams*)Params::GetParamsInstance(Params::_regionParamsTag,i);
		if  ( (i != vizNum)  &&((!rgParams)||!rgParams->isLocal())){
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
		if (!vizWin[i]) continue;
		ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag,i);
		if  ( (i != vizNum)  &&
				((!vpParams)||!vpParams->isLocal())
			){
			vizWin[i]->updateGL();
		}
	}
}



//set the changed bits for each of the visualizers associated with the
//specified animationParams, preventing them from updating the frame counter.
//Also set the lat/lon or local coordinates in the viewpoint params
//

//Set the viewer coords changed flag for all vizwin's using these params:
void VizWinMgr::
setViewerCoordsChanged(ViewpointParams* vp){
	int vizNum = vp->getVizNum();
	if (vizNum>=0) {
		vizWin[vizNum]->getVisualizer()->setViewerCoordsChanged(true);
	}
	if(vp->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!vizWin[i]) continue;
		Params* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, i);
		if  (  (i != vizNum)  && ((!vpParams)||!vpParams->isLocal())
			){
			vizWin[i]->getVisualizer()->setViewerCoordsChanged(true);
		}
	}
}
//Reset the near/far distances for all the windows that
//share a viewpoint, based on region in specified regionparams
//
void VizWinMgr::
resetViews(ViewpointParams* vp){
	int vizNum = vp->getVizNum();
	if (vizNum>=0) {
		Visualizer* glw = vizWin[vizNum]->getVisualizer();
		if(glw) glw->resetView(vp);
	}
	if(vp->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!vizWin[i]) continue;
		Params* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, i);
		if  ( (i != vizNum)  && ((!vpParams)||!vpParams->isLocal())){
			Visualizer* glw = vizWin[i]->getVisualizer();
			if(glw) glw->resetView(vp);
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
		if(getRealAnimationParams(activeViz))getAnimationRouter()->guiSetLocal(getRealAnimationParams(activeViz),false);
		getAnimationRouter()->updateTab();
		vizWin[activeViz]->getVisualizer()->setActiveAnimationParams(getGlobalAnimationParams());
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!getRealAnimationParams(activeViz)){
			//create a new parameter panel, copied from global
			Params::AppendParamsInstance(Params::_animationParamsTag, activeViz, (AnimationParams*)getGlobalAnimationParams()->deepCopy());
			getRealAnimationParams(activeViz)->setVizNum(activeViz);
			getAnimationRouter()->confirmText(true);
			
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			getAnimationRouter()->guiSetLocal(getRealAnimationParams(activeViz),true);
			getAnimationRouter()->updateTab();
			
		}
		//set the local params in the Visualizer:
		vizWin[activeViz]->getVisualizer()->setActiveAnimationParams(getRealAnimationParams(activeViz));
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
		getViewpointRouter()->guiSetLocal((ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, activeViz),false);
		getViewpointRouter()->updateTab();
		vizWin[activeViz]->getVisualizer()->setActiveViewpointParams((ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag));
		vizWin[activeViz]->setGlobalViewpoint(true);
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, activeViz);
		if (!vpParams){
			//create a new parameter panel, copied from global
			vpParams = (ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag)->deepCopy();
			vpParams->setVizNum(activeViz);
			getViewpointRouter()->guiSetLocal(vpParams,true);
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			getViewpointRouter()->guiSetLocal(vpParams,true);
			getViewpointRouter()->updateTab();
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getVisualizer()->setActiveViewpointParams(vpParams);
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
		getRegionRouter()->guiSetLocal(getRegionParams(activeViz),false);
		getRegionRouter()->updateTab();
		vizWin[activeViz]->getVisualizer()->setActiveRegionParams(getGlobalRegionParams());
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		if (!getRealRegionParams(activeViz)){
			//create a new parameter panel, copied from global
			Params::AppendParamsInstance(Params::_regionParamsTag, activeViz, (RegionParams*)getGlobalRegionParams()->deepCopy());
			getRealRegionParams(activeViz)->setVizNum(activeViz);
			getRegionRouter()->guiSetLocal(getRealRegionParams(activeViz),true);
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			getRegionRouter()->guiSetLocal(getRegionParams(activeViz),true);
			getRegionRouter()->updateTab();
			
			//and then refresh the panel:
			tabManager->show();
		}
		vizWin[activeViz]->getVisualizer()->setActiveRegionParams(getRealRegionParams(activeViz));
	}
	//Specify if the active viz is sharing the region
	
	Visualizer::setRegionShareFlag(val == 0);
	
	//in region mode, refresh the global windows and this one
	if (Visualizer::getCurrentMouseMode() == Visualizer::regionMode){
		for (int i = 0; i<MAXVIZWINS; i++){
			if(vizWin[i] && (!getRegionParams(i)->isLocal()|| (i == activeViz)))
				vizWin[i]->updateGL();
		}
	}
}

ViewpointParams* VizWinMgr::
getViewpointParams(int winNum){
	if (winNum < 0) return (ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag);
	Params* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, winNum);
	if (vpParams && vpParams->isLocal()) return ((ViewpointParams*) vpParams);
	return (ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag);
}
RegionParams* VizWinMgr::
getRegionParams(int winNum){
	if (winNum < 0) return getGlobalRegionParams();
	if (getRealRegionParams(winNum)&&getRealRegionParams(winNum)->isLocal()) return getRealRegionParams(winNum);
	return getGlobalRegionParams();
}




AnimationParams* VizWinMgr::
getAnimationParams(int winNum){
	if (winNum < 0) return getGlobalAnimationParams();
	if (getRealAnimationParams(winNum) && getRealAnimationParams(winNum)->isLocal()) return getRealAnimationParams(winNum);
	return getGlobalAnimationParams();
}


//Return global ie default params
Params* VizWinMgr::
getGlobalParams(Params::ParamsBaseType pType){
	return Params::GetDefaultParams(pType);
}
void VizWinMgr::
setGlobalParams(Params* p, Params::ParamsBaseType t){
	Params::SetDefaultParams(t,p);
}

Params* VizWinMgr::
getLocalParams(Params::ParamsBaseType t){
	assert(activeViz >= 0);
	return Params::GetParamsInstance(t, activeViz);
}
//Get the Params that apply.  If there is no current active viz, then
//return the global params.
Params* VizWinMgr::
getApplicableParams(Params::ParamsBaseType typId){
	if(activeViz < 0) return getGlobalParams(typId);
	Params* p = Params::GetParamsInstance(typId,activeViz,-1);
	if (p->isRenderParams() || p->isLocal()) return p;
	return Params::GetDefaultParams(typId);
}
EventRouter* VizWinMgr::
getEventRouter(Params::ParamsBaseType typeId){
	map <Params::ParamsBaseType, EventRouter*> :: const_iterator getEvIter;
    getEvIter = eventRouterMap.find(typeId);
	if (getEvIter == eventRouterMap.end()) {
		assert(0);
		return 0;
	}
	return getEvIter->second;
}


void VizWinMgr::
initViews(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]) {
			ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag,i);
			if (vpParams->isLocal()){
				vizWin[i]->setValuesFromGui(vpParams);
			}
			else vizWin[i]->setValuesFromGui(getGlobalVPParams());
			vizWin[i]->getVisualizer()->setViewerCoordsChanged(true);
		}
	}
}
// force all the existing params to restart (return to initial state)
//
void VizWinMgr::
restartParams(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!getVizWin(i)) continue;
		if(getRealVPParams(i)) getRealVPParams(i)->restart();
		if(getRealRegionParams(i)) getRealRegionParams(i)->restart();
		if(getRealAnimationParams(i)) getRealAnimationParams(i)->restart();
		//Perform restart on all renderer params instances:
		for (int k = 1; k<= Params::GetNumParamsClasses(); k++){
			Params* p = Params::GetDefaultParams(k);
			if (!p->isRenderParams())continue;
			EventRouter* eRouter = getEventRouter(k);
			for (int inst = 0; inst < Params::GetNumParamsInstances(k, i); inst++){
				Params* rpar = Params::GetParamsInstance(k,i,inst);
				eRouter->cleanParams(rpar);
				rpar->restart();
			}
		}
	}
	
	getGlobalVPParams()->restart();
	getGlobalRegionParams()->restart();
	getGlobalAnimationParams()->restart();
}
// force all the existing params to reinitialize, i.e. make minimal
// changes to use new metadata.  If doOverride is true, we can
// ignore previous settings
//
void VizWinMgr::
reinitializeParams(bool doOverride){
	// Default render params should override; non render don't necessarily:
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params* p = Params::GetDefaultParams(i);
		if (p->isRenderParams())p->reinit(true);
	}

	
	//NOTE that the vpparams need to be initialized after
	//the global region params, since they use its settings..
	//
	getGlobalVPParams()->reinit(doOverride);
	
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!vizWin[i]) continue;
		if(getRealVPParams(i) && (getRealVPParams(i)!= getGlobalVPParams())) getRealVPParams(i)->reinit(doOverride);
		if(getRealRegionParams(i)) getRealRegionParams(i)->reinit(doOverride);
		//Reinitialize all the render params for each window
		for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
			EventRouter* eRouter = getEventRouter(pType);
			for (int inst = 0; inst < Params::GetNumParamsInstances(pType, i); inst++){
				Params* p = Params::GetParamsInstance(pType,i,inst);
				p->reinit(doOverride);
				if (!p->isRenderParams()) break;
				RenderParams* rParams = (RenderParams*)p;
				p->setVizNum(i);  //needed because of iso bug in 2.0.0
				//turn off rendering
				rParams->setEnabled(false);
				eRouter->updateRenderer(rParams, rParams->isEnabled(), false);
			}
		}
		
		if(getRealAnimationParams(i) && getRealAnimationParams(i)!= getGlobalAnimationParams()) getRealAnimationParams(i)->reinit(doOverride);
		//setup near/far
		if (getRealVPParams(i)) vizWin[i]->getVisualizer()->resetView(getViewpointParams(i));
		if(!doOverride && vizWin[i]) vizWin[i]->getVisualizer()->removeAllRenderers();
	}

    //
    // Reinitialize all tabs
    //
	for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
		EventRouter* eRouter = getEventRouter(pType);
		eRouter->reinitTab(doOverride);
	}
	//Note that animation params must reinitialize after viewpoint params 
	//in order to get starting viewpoint for animation control.
	getGlobalAnimationParams()->reinit(doOverride);
}
//Update params and tabs to be aware of change of active variables
//Similar to above, but don't turn off existing renderers, DataMgr has
//not changed.
void VizWinMgr::
reinitializeVariables(){
	// Default render params do not override
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		Params* p = Params::GetDefaultParams(i);
		if (p->isRenderParams())p->reinit(false);
	}
	
	getGlobalRegionParams()->reinit(false);
	getRegionRouter()->reinitTab(false);

	//NOTE that the vpparams need to be initialized after
	//the global region params, since they use its settings..
	//
	getGlobalVPParams()->reinit(false);
	
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!vizWin[i]) continue;
		if(getRealVPParams(i)) getRealVPParams(i)->reinit(false);
		if(getRealRegionParams(i)) getRealRegionParams(i)->reinit(false);
		//Reinitialize all the render params, but not the event routers
		for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
			for (int inst = 0; inst < Params::GetNumParamsInstances(pType, i); inst++){
				Params* p = Params::GetParamsInstance(pType,i,inst);
				if (!p->isRenderParams()) break;
				RenderParams* rParams = (RenderParams*)p;
				rParams->reinit(false);
			}
		}
		if(getRealAnimationParams(i)) getRealAnimationParams(i)->reinit(false);
	}

    //
    // Reinitialize tabs
    //
	for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
		EventRouter* eRouter = getEventRouter(pType);
		eRouter->reinitTab(false);
	}
	getGlobalAnimationParams()->reinit(false);
}
//Force all renderers to re-obtain render data
void VizWinMgr::refreshRenderData(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]){
			Visualizer* glwin= vizWin[i]->getVisualizer();
			for (int j = 0; j< glwin->getNumRenderers(); j++){
				Renderer* ren = glwin->getRenderer(j);
				ren->setAllDataDirty();
			}
			vizWin[i]->updateGL();
		}
	}
}

//
//Disable all renderers that use specified variables
void VizWinMgr::disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D){
	ControlExecutive* ce = ControlExecutive::getInstance();
	for (int i = 0; i<ce->GetNumVisualizers(); i++){
		Visualizer* viz = ce->GetVisualizer(i);
		int numRens = viz->getNumRenderers();
		for (int j = numRens-1; j >= 0; j--){
			Renderer* ren = viz->getRenderer(j);
			RenderParams* rParams = ren->getRenderParams();
			for (int k = 0; k<vars2D.size(); k++){
				if(rParams->usingVariable(vars2D[k])){
					Params::ParamsBaseType t = rParams->GetParamsBaseTypeId();
					int instance = findInstanceIndex(i, rParams, t);
					EventRouter* er = getEventRouter(t);
					er->guiSetEnabled(false,instance, false);
				}
			}
			for (int k = 0; k<vars3D.size(); k++){
				if(rParams->usingVariable(vars3D[k])){
					Params::ParamsBaseType t = rParams->GetParamsBaseTypeId();
					int instance = findInstanceIndex(i, rParams, t);
					EventRouter* er = getEventRouter(t);
					er->guiSetEnabled(false,instance, false);
				}
			}
		}
	}
}
//Disable all renderers 
void VizWinMgr::disableAllRenderers(){
	ControlExecutive* ce = ControlExecutive::getInstance();
	int numViz = ce->GetNumVisualizers();
	for (int i = 0; i<numViz; i++){
		Visualizer* viz = ce->GetVisualizer(i);
		viz->removeAllRenderers();
	}
}
	
void VizWinMgr::
setSelectionMode( int m){ 
	Visualizer::setCurrentMouseMode(m);
	//Update all visualizers:
	for (int i = 0; i<MAXVIZWINS; i++){
		if(vizWin[i]) vizWin[i]->updateGL();
	}
}






void VizWinMgr::setCurrentInstanceIndex(int winnum, int inst, Params::ParamsBaseType t){
	Params::SetCurrentParamsInstanceIndex(t,winnum,inst);
}
int VizWinMgr::findInstanceIndex(int winnum, Params* rParams, Params::ParamsBaseType ptype){
	for (int i = 0; i< Params::GetNumParamsInstances(ptype,winnum); i++){
		if (Params::GetParamsInstance(ptype,winnum,i) == rParams)
			return i;
	}
	return -1;
}
int VizWinMgr::getCurrentInstanceIndex(int winnum, Params::ParamsBaseType clsId){
	return Params::GetCurrentParamsInstanceIndex(clsId,winnum);
}
void VizWinMgr::insertInstance(int winnum, int inst, Params* newParams){
	int classId = newParams->GetParamsBaseTypeId();
	Params::InsertParamsInstance(classId,winnum,inst, newParams);
}
void VizWinMgr::appendInstance(int winnum, Params* newParams){
	int classId = newParams->GetParamsBaseTypeId();
	Params::AppendParamsInstance(classId,winnum, newParams);
}
void VizWinMgr::removeInstance(int winnum, int instance, Params::ParamsBaseType classId){
	Params::RemoveParamsInstance(classId,winnum,instance);
}
Params* VizWinMgr::getParams(int winnum, Params::ParamsBaseType pType, int instance ){
	Params* p =  Params::GetParamsInstance(pType,winnum, instance);
	if (!p || p->isRenderParams()) return p;
	if (winnum < 0) return Params::GetDefaultParams(pType);
	if (p->isLocal()) return p;
	return Params::GetDefaultParams(pType);
}
int VizWinMgr::getNumInstances(int winnum, Params::ParamsBaseType pType){
	
	return (Params::GetNumParamsInstances(pType,winnum));
}
	


Params::ParamsBaseType VizWinMgr::RegisterEventRouter(const std::string tag, EventRouter* router){
	Params::ParamsBaseType t = Params::GetTypeFromTag(tag);
	if (t <= 0) return 0;
	eventRouterMap.insert(pair<int,EventRouter*>(t,router));
	return t;
}
void VizWinMgr::RegisterMouseModes(){
	RegisterMouseMode(Params::_viewpointParamsTag,0,"Navigation", wheel );
	RegisterMouseMode(Params::_regionParamsTag,1, "Region",cube );
	//RegisterMouseMode(Params::_flowParamsTag,1, "Flow rake",rake );
	//RegisterMouseMode(Params::_probeParamsTag,3,"Probe", probe);
	//RegisterMouseMode(Params::_twoDDataParamsTag,2,"2D Data", twoDData);
	//RegisterMouseMode(Params::_twoDImageParamsTag,2, "Image",twoDImage);

	//RegisterExtensionMouseModes:
	//For each class that has a manipulator associated with it, insert the method
	//VizWinMgr::RegisterMouseMode(tag, modeType, manip name, xpm (pixmap) )
	
	RegisterMouseMode(ArrowParams::_arrowParamsTag,1, "Barb rake", arrowrake );
	
}
int VizWinMgr::RegisterMouseMode(const std::string paramsTag, int manipType,  const char* name, const char* const xpmIcon[]){
	QIcon icon;
	if (xpmIcon){
		QPixmap qp = QPixmap(xpmIcon);
		icon = QIcon(qp);
	}
	int newMode; 
	QString qname(name);
	newMode =  MainForm::getInstance()->addMode(qname,icon);
	Visualizer::AddMouseMode(paramsTag, manipType, name);
	return newMode;
}
void VizWinMgr::InstallTab(const std::string tag, EventRouterCreateFcn fcn){

	
	EventRouter* eRouter = fcn();
	Params::ParamsBaseType typ = RegisterEventRouter(tag, eRouter);
	eRouter->hookUpTab();
	QWidget* tabWidget = dynamic_cast<QWidget*> (eRouter);
	assert(tabWidget);
	
	tabManager->addWidget(tabWidget, typ);
	
	
}
AnimationEventRouter* VizWinMgr::
getAnimationRouter() {
	return (AnimationEventRouter*)getEventRouter(Params::_animationParamsTag);
}
ViewpointEventRouter* VizWinMgr::
getViewpointRouter() {
	return (ViewpointEventRouter*)getEventRouter(Params::_viewpointParamsTag);
}
RegionEventRouter* VizWinMgr::
getRegionRouter() {
	return (RegionEventRouter*)getEventRouter(Params::_regionParamsTag);
}

void VizWinMgr::forceRender(RenderParams* rp, bool always){
	if (!always && !rp->isEnabled()) return;
	int viznum = rp->getVizNum();
	if (viznum < 0) return;
	vizWin[viznum]->updateGL();
}
