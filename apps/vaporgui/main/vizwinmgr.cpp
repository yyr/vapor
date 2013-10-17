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
	numVizWins = 0;
   
	VizWindow.clear();
	VizMdiWin.clear();
	VizName.clear();
	
	ActivationOrder.clear();
	
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

}

/***********************************************************************
 *  Destroys the object and frees any allocated resources
 ***********************************************************************/
VizWinMgr::~VizWinMgr()
{
	//qWarning("vizwinmgr destructor start");
	for (int i = 0; i<VizWindow.size(); i++){
		if (VizWindow[i]) {
			delete VizWindow[i];
			delete VizMdiWin[i];
		}
	}
    
	
}
void VizWinMgr::
vizAboutToDisappear(int i)  {
   
    if (VizWindow[i] == 0) return;

	//Count how many vizWins are left
	int numVizWins = 0;
	for (int j = 0; j<VizWindow.size(); j++){
		if ((j != i) && VizWindow[j]) {
			numVizWins++;
		}
	}
	//Remember the vizwin just to save its color:
	VizWindow[i] = 0;
	VizMdiWin[i] = 0;
	
	//If we are deleting the active viz, revert to the 
	//most recent active viz.
	if (activeViz == i) {
		int newActive = getLastActive();
		if (newActive >= 0 && VizWindow[newActive]) setActiveViz(newActive);
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
    
    VizName[i] = "";

	EventRouter* ev = tabManager->getFrontEventRouter();
	ev->updateTab();
}
//When a visualizer is launched, the index is assigned based on the Control Executive visualizer number.
//That value is returned.
//
int VizWinMgr::
launchVisualizer()
{
	ControlExecutive* ce = ControlExecutive::getInstance();
	int useWindowNum = ce->NewVisualizer();
	
	numVizWins++;
	
	createDefaultParams(useWindowNum);
		
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here:  
    
	VizName[useWindowNum] = ((QString("Visualizer No. ")+QString::number(useWindowNum)));
	emit (newViz(VizName[useWindowNum], useWindowNum));
	VizWindow[useWindowNum] = new VizWin (MainForm::getInstance(), VizName[useWindowNum], 0, this, newRect, useWindowNum);
	QMdiSubWindow* subwin = new QMdiSubWindow;
	subwin->setWidget(VizWindow[useWindowNum]);
	subwin->setAttribute(Qt::WA_DeleteOnClose);
	QMdiSubWindow* qsbw = MainForm::getInstance()->getMDIArea()->addSubWindow(subwin);
	VizMdiWin[useWindowNum]=qsbw;
	//VizMdiWin[useWindowNum]=MainForm::getInstance()->getMDIArea()->addSubWindow(VizWindow[useWindowNum]);
	VizWindow[useWindowNum]->setWindowNum(useWindowNum);
	VizWindow[useWindowNum]->setFocusPolicy(Qt::ClickFocus);


	setActiveViz(useWindowNum);

	VizWindow[useWindowNum]->showMaximized();
	
	//Tile if more than one visualizer:
	if(numVizWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	
	
	emit activateViz(useWindowNum);
	VizWindow[useWindowNum]->show();

	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numVizWins > 1){
		emit enableMultiViz(true);
	}

	//Initialize axis annotation to use full user extents:
	
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
	for (int i = 0; i< VizWindow.size(); i++){
		if (VizWindow[i]) {
			VizWindow[i]->close();
			//qWarning("closing viz window %d", i);
			
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
	for (int i = 0; i< VizWindow.size(); i++){
		if(VizWindow[i]) {
			VizWindow[i]->resize(400,400);
		}	
	} 
	
}
void 
VizWinMgr::coverRight(){
	
    for (int i = 0; i< VizWindow.size(); i++){
		if(VizWindow[i]) {
			VizWindow[i]->showMaximized();
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
    for (int i = 0; i< VizWindow.size(); i++){
        if (VizWindow[i]) num++;
    }
    return num;
}
void VizWinMgr::
setVizWinName(int winNum, QString& qs) {
		VizName[winNum] = qs;
		VizWindow[winNum]->setWindowTitle(qs);
}

/**********************************************************************
 * Method called when the user makes a visualizer active:
 **********************************************************************/
void VizWinMgr::
setActiveViz(int vizNum){
	if (activeViz != vizNum){
	
		emit(activateViz(vizNum));
		
		activeViz = vizNum;
		ActivationOrder[vizNum]= (++activationCount);
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
			for (int i = 0; i< VizWindow.size(); i++){
				if (VizWindow[i]) VizWindow[i]->updateGL();
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
}

//Method to enable closing of a vizWin for Undo/Redo
void VizWinMgr::
killViz(int viznum){
	assert(VizWindow[viznum]);
	MainForm::getInstance()->getMDIArea()->removeSubWindow(VizMdiWin[viznum]);
	VizWindow[viznum]->close();
	VizWindow[viznum] = 0;
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
	
	VizWindow[winNum]->setFocus();
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

}
void VizWinMgr::
viewRegion()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	getViewpointRouter()->guiCenterSubRegion(getRegionParams(activeViz));
	
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
		VizWindow[activeViz]->updateGL();
	}
	//Is another viz using these region params?
	if (rParams->isLocal()) return;
	for (int i = 0; i< VizWindow.size(); i++){
		if (!VizWindow[i]) continue;
		RegionParams* rgParams = (RegionParams*)Params::GetParamsInstance(Params::_regionParamsTag,i);
		if  ( (i != vizNum)  &&((!rgParams)||!rgParams->isLocal())){
			VizWindow[i]->updateGL();
		}
	}
}
//Trigger a re-render of the windows that share a viewpoint params
void VizWinMgr::refreshViewpoint(ViewpointParams* vParams){
	int vizNum = vParams->getVizNum();
	if (vizNum >= 0){
		VizWindow[activeViz]->updateGL();
	}
	//Is another viz using these viewpoint params?
	if (vParams->isLocal()) return;
	for (int i = 0; i< VizWindow.size(); i++){
		if (!VizWindow[i]) continue;
		ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag,i);
		if  ( (i != vizNum)  &&
				((!vpParams)||!vpParams->isLocal())
			){
			VizWindow[i]->updateGL();
		}
	}
}



//set the changed bits for each of the visualizers associated with the
//specified animationParams, preventing them from updating the frame counter.
//Also set the lat/lon or local coordinates in the viewpoint params
//


//Reset the near/far distances for all the windows that
//share a viewpoint, based on region in specified regionparams
//
void VizWinMgr::
resetViews(ViewpointParams* vp){
	int vizNum = vp->getVizNum();
	if (vizNum>=0) {
		Visualizer* glw = VizWindow[vizNum]->getVisualizer();
		if(glw) glw->resetView(vp);
	}
	if(vp->isLocal()) return;
	for (int i = 0; i< VizWindow.size(); i++){
		if (!VizWindow[i]) continue;
		Params* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, i);
		if  ( (i != vizNum)  && ((!vpParams)||!vpParams->isLocal())){
			Visualizer* glw = VizWindow[i]->getVisualizer();
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
	
		VizWindow[activeViz]->setGlobalViewpoint(true);
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
		VizWindow[activeViz]->setGlobalViewpoint(false);
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

	}
	//Specify if the active viz is sharing the region
	
	Visualizer::setRegionShareFlag(val == 0);
	
	//in region mode, refresh the global windows and this one
	if (Visualizer::getCurrentMouseMode() == Visualizer::regionMode){
		for (int i = 0; i<VizWindow.size(); i++){
			if(VizWindow[i] && (!getRegionParams(i)->isLocal()|| (i == activeViz)))
				VizWindow[i]->updateGL();
		}
	}
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
	for (int i = 0; i< VizWindow.size(); i++){
		if (VizWindow[i]) {
			ViewpointParams* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag,i);
			if (vpParams->isLocal()){
				VizWindow[i]->setValuesFromGui(vpParams);
			}
			else VizWindow[i]->setValuesFromGui(getGlobalVPParams());
		}
	}
}
// force all the existing params to restart (return to initial state)
//
void VizWinMgr::
restartParams(){
	for (int i = 0; i< VizWindow.size(); i++){
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
	
	for (int i = 0; i< VizWindow.size(); i++){
		if (!VizWindow[i]) continue;
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
		if (getRealVPParams(i)) VizWindow[i]->getVisualizer()->resetView(getViewpointParams(i));
		if(!doOverride && VizWindow[i]) VizWindow[i]->getVisualizer()->removeAllRenderers();
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
	
	for (int i = 0; i< VizWindow.size(); i++){
		if (!VizWindow[i]) continue;
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
	ControlExecutive* ce = ControlExecutive::getInstance();
	for (int i = 0; i<ce->GetNumVisualizers(); i++){
	
		Visualizer* viz= ce->GetVisualizer(i);
		for (int j = 0; j< viz->getNumRenderers(); j++){
			Renderer* ren = viz->getRenderer(j);
			ren->setAllDataDirty();
		}
		VizWindow[i]->updateGL();	
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
	for (int i = 0; i<VizWindow.size(); i++){
		if(VizWindow[i]) VizWindow[i]->updateGL();
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
	VizWindow[viznum]->updateGL();
}
