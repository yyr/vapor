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
#include "vizselectcombo.h"

#include "math.h"
#include "mainform.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "mousemodeparams.h"
#include "vizwinparams.h"
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





VizWinMgr::VizWinMgr() 
{
	myMainWindow = MainForm::getInstance();
    myMDIArea = myMainWindow->getMDIArea();
	tabManager = MainForm::getTabManager();
    activationCount = 0;
	setActiveViz(-1);
   
	VizWindow.clear();
	VizMdiWin.clear();
	
	ActivationOrder.clear();
	
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
	
	//set up tabs
	tabManager->orderTabs();
	

}

/***********************************************************************
 *  Destroys the object and frees any allocated resources
 ***********************************************************************/
VizWinMgr::~VizWinMgr()
{
	
	std::map<int, VizWin*>::iterator it;
	for(it = VizWindow.begin(); it != VizWindow.end(); it++){
		VizWin* win = it->second;
		int indx = it->first;
		assert(win != 0);
		delete win;
		delete VizMdiWin[indx];
		
	}
    
	
}
void VizWinMgr::
vizAboutToDisappear(int i)  {
    int activeViz = getActiveViz();
	std::map<int, VizWin*>::iterator it;
	it = VizWindow.find(i);
	if (it == VizWindow.end()) return;
	
	int indexToGo = it->first;
	//Count how many vizWins are left
	int numVizWins = 0;
	for (it = VizWindow.begin(); it != VizWindow.end(); it++){
		VizWin* win = it->second;
		int indx = it->first;
		if (indx != indexToGo && win){
			numVizWins++;
		}
	}
	//Remove the vizwin and the vizmdiwin
	it = VizWindow.find(i);
	VizWindow.erase(it);
	std::map<int,QMdiSubWindow*>::iterator it2;
	it2 = VizMdiWin.find(i);
	VizMdiWin.erase(it2);
	
	//If we are deleting the active viz, revert to the 
	//most recent active viz.
	if (activeViz == i) {
		int newActive = getLastActive();
		if (newActive < 0) setActiveViz(-1);
		else {
			//Make sure it is OK
			it = VizWindow.find(newActive);
			if (it == VizWindow.end()) setActiveViz(-1);
			else setActiveViz(newActive);
		}
	}
	
	
	//Delete all the params associated with the visualizer
	for (int j = 1+ControlExec::GetNumBasicParamsClasses(); j<= ControlExec::GetNumParamsClasses(); j++){
		string tag = ControlExec::GetTagFromType(j);
		for (int k = ControlExec::GetNumParamsInstances(i,tag)-1; k>=0; k--){
			getEventRouter(j)->cleanParams(ControlExec::GetParams(i,tag,k));
			ControlExec::RemoveParams(i,tag,k);
		}
	}
	//Remove the visualizer
	ControlExec::RemoveVisualizer(i);

	//Remove the visualizer from the vizSelectCombo
	emit (removeViz(i));
	
	//When the number is reduced to 1, disable multiviz options.
	if (numVizWins == 1) emit enableMultiViz(false);
	
	
	EventRouter* ev = tabManager->getFrontEventRouter();
	ev->updateTab();
}
//When a visualizer is launched from the GUI, the index is assigned based on the Control Executive visualizer number.
//That value is returned.
//
int VizWinMgr::
launchVisualizer()
{
	
	Command* cmd = Command::CaptureStart(ControlExec::GetCurrentParams(-1,VizWinParams::_vizWinParamsTag), "launch Visualizer", VizWinMgr::UndoRedo);
	int useWindowNum = ControlExec::NewVisualizer();
	
	createDefaultParams(useWindowNum);
		
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here: 
	std::stringstream out;
	out << useWindowNum;
	string win = out.str();
	
    string vizname = string("Visualizer No. ") +win;
	QString qvizname = QString::fromStdString(vizname);
	VizWinParams::SetVizName(useWindowNum, vizname);
	//notify the window selector:
	emit (newViz(qvizname, useWindowNum));
	
	VizWindow[useWindowNum] = new VizWin (MainForm::getInstance(), qvizname, this, newRect, useWindowNum);
	
	QMdiSubWindow* qsbw = MainForm::getInstance()->getMDIArea()->addSubWindow(VizWindow[useWindowNum]);
	VizMdiWin[useWindowNum]=qsbw;
	
	VizWindow[useWindowNum]->setWindowNum(useWindowNum);
	VizWindow[useWindowNum]->setFocusPolicy(Qt::ClickFocus);
	VizWindow[useWindowNum]->setWindowTitle(qvizname);


	setActiveViz(useWindowNum);

	VizWindow[useWindowNum]->showMaximized();
	
	int numWins = VizWinParams::GetNumVizWins();
	//Tile if more than one visualizer:
	if(numWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	
	emit activateViz(useWindowNum);
	VizWindow[useWindowNum]->show();

	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numWins > 1){
		emit enableMultiViz(true);
	}
	Command::CaptureEnd(cmd,ControlExec::GetCurrentParams(-1,VizWinParams::_vizWinParamsTag));
	return useWindowNum;
}
//When a new session is opened, visualizers are created to use the params from the session.
//
int VizWinMgr::
attachVisualizer(int useVizNum)
{
	
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here: 
	std::stringstream out;
	out << useVizNum;
	string win = out.str();
	
    string vizname = string("Visualizer No. ") +win;
	QString qvizname = QString::fromStdString(vizname);
	VizWinParams::SetVizName(useVizNum, vizname);
	//notify the window selector:
	emit (newViz(qvizname, useVizNum));
	
	VizWindow[useVizNum] = new VizWin (MainForm::getInstance(), qvizname, this, newRect, useVizNum);
	
	QMdiSubWindow* qsbw = MainForm::getInstance()->getMDIArea()->addSubWindow(VizWindow[useVizNum]);
	VizMdiWin[useVizNum]=qsbw;
	VizWindow[useVizNum]->setWindowNum(useVizNum);
	VizWindow[useVizNum]->setFocusPolicy(Qt::ClickFocus);
	VizWindow[useVizNum]->setWindowTitle(qvizname);


	setActiveViz(useVizNum);

	VizWindow[useVizNum]->showMaximized();
	
	int numWins = VizWinParams::GetNumVizWins();
	//Tile if more than one visualizer:
	if(numWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	
	emit activateViz(useVizNum);
	VizWindow[useVizNum]->show();

	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numWins > 1){
		emit enableMultiViz(true);
	}

	return useVizNum;
}
/*
 *  Create params, for a new visualizer.  
 */
void VizWinMgr::
createDefaultParams(int winnum){
	
	for (int i = 1; i<= ControlExec::GetNumParamsClasses(); i++){
		string tag = ControlExec::GetTagFromType(i);
		Params* q = ControlExec::GetDefaultParams(tag);
		if (q->isRenderParams()) continue;
		Params* p = (Params*)(q->deepCopy());
		p->SetVizNum(winnum);
		//Check for strange error:
		assert( ControlExec::GetNumParamsInstances(winnum,tag) == 0);
		ControlExec::AddParams(winnum,tag,p);
		ControlExec::SetCurrentParamsInstance(winnum,tag,0);

		if (!p->isRenderParams())p->SetLocal(false);
		else {
			if(DataStatus::getInstance()->getDataMgr()){
				EventRouter* eRouter = getEventRouter(i);
				eRouter->setEditorDirty((RenderParams*)p);
			}
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
	map<int,VizWin*>::iterator it;
	for (it = VizWindow.begin(); it != VizWindow.end(); it++){
		(it->second)->resize(400,400);
	}
}

void 
VizWinMgr::fitSpace(){
	
    myMDIArea->tileSubWindows();
	
}


int VizWinMgr::
getNumVisualizers(){
    return VizWindow.size();
}

/**********************************************************************
 * Method called when the user makes a visualizer active:
 **********************************************************************/
void VizWinMgr::
setActiveViz(int vizNum){
	if (vizNum < 0) return;
	if (getActiveViz() != vizNum){
		Command* cmd = Command::CaptureStart(ControlExec::GetActiveParams(VizWinParams::_vizWinParamsTag),"Change current active visualizer",VizWinMgr::UndoRedo);
		ControlExec::SetActiveVizIndex(vizNum);
		emit(activateViz(vizNum));
		
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
		Visualizer::setRegionShareFlag(!getRegionParams(vizNum)->IsLocal());

		tabManager->show();
		//Add to history if this is not during initial creation.
		
		//Need to cause a redraw in all windows if we are not in navigate mode,
		//So that the manips will change where they are drawn:
		if (MouseModeParams::GetCurrentMouseMode() != MouseModeParams::navigateMode){
			map<int,VizWin*>::iterator it;
			for (it = VizWindow.begin(); it != VizWindow.end(); it++){
				(it->second)->updateGL();
			}
		}
		Command::CaptureEnd(cmd,ControlExec::GetActiveParams(VizWinParams::_vizWinParamsTag));
	}
}
//Method to cause all the params to update their tab panels for the active
//Viz window
void VizWinMgr::
updateActiveParams(){
	int activeViz = getActiveViz();
	if (activeViz < 0 ||!getVizWin(activeViz)) return;
	for (int i = ControlExec::GetNumBasicParamsClasses()+1; i<= ControlExec::GetNumParamsClasses(); i++){
		if (Params::GetCurrentParamsInstance(i,activeViz))
			getEventRouter(i)->updateTab();
	}
}

//Method to enable closing of a vizWin
void VizWinMgr::
killViz(int viznum){
	
	assert(VizWindow.find(viznum) != VizWindow.end());
	VizWindow[viznum]->setEnabled(false);
	//MainForm::getInstance()->getWindowSelector()->removeWindow(viznum);
	MainForm::getInstance()->getMDIArea()->removeSubWindow(VizMdiWin[viznum]);
	VizWindow[viznum]->close();
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
	int activeViz = getActiveViz();
	getViewpointRouter()->guiCenterFullRegion(getRegionParams(activeViz));

}
void VizWinMgr::
viewRegion()
{
	VizWin* vw = getActiveVisualizer();
	if (!vw) return;
	int activeViz = getActiveViz();
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
	int vizNum = rParams->GetVizNum();
	int activeViz = getActiveViz();
	if (vizNum >= 0){
		VizWindow[activeViz]->updateGL();
	}
	//Is another viz using these region params?
	if (rParams->IsLocal()) return;
	map<int,VizWin*>::iterator it;
	for (it = VizWindow.begin(); it != VizWindow.end(); it++){
		int i = it->first;
		RegionParams* rgParams = (RegionParams*)ControlExec::GetParams(i,Params::_regionParamsTag,-1);
		if  ( (i != vizNum)  &&((!rgParams)||!rgParams->IsLocal())){
			(it->second)->updateGL();
		}
	}
}
//Trigger a re-render of the windows that share a viewpoint params
void VizWinMgr::refreshViewpoint(ViewpointParams* vParams){
	int vizNum = vParams->GetVizNum();
	int activeViz = getActiveViz();
	if (vizNum >= 0){
		VizWindow[activeViz]->updateGL();
	}
	//Is another viz using these viewpoint params?
	if (vParams->IsLocal()) return;
	map<int,VizWin*>::iterator it;
	for (it = VizWindow.begin(); it != VizWindow.end(); it++){
		int i = it->first;
		ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetParams(i,Params::_viewpointParamsTag,-1);
		if  ( (i != vizNum)  &&
				((!vpParams)||!vpParams->IsLocal())
			){
			(it->second)->updateGL();
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
	int vizNum = vp->GetVizNum();
	if (vizNum>=0) {
		Visualizer* glw = VizWindow[vizNum]->getVisualizer();
		if(glw) glw->resetView(vp);
	}
	if(vp->IsLocal()) return;
	map<int,VizWin*>::iterator it;
	for (it = VizWindow.begin(); it != VizWindow.end(); it++){
		int i = it->first;
		Params* vpParams = (ViewpointParams*)ControlExec::GetParams(i,Params::_viewpointParamsTag, -1);
		if  ( (i != vizNum)  && ((!vpParams)||!vpParams->IsLocal())){
			Visualizer* glw = (it->second)->getVisualizer();
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
	int activeViz = getActiveViz();
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		getAnimationRouter()->guiSetLocal((AnimationParams*)ControlExec::GetParams(activeViz,Params::_animationParamsTag,-1),false);
		getAnimationRouter()->updateTab();
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		 //need to revert to existing local settings:
		getAnimationRouter()->guiSetLocal((AnimationParams*)ControlExec::GetParams(activeViz,Params::_animationParamsTag,-1),true);
		getAnimationRouter()->updateTab();
			
	}
		
	//and then refresh the panel:
	tabManager->show();
	
}

/*****************************************************************************
 * Called when the local/global selector is changed.
 * Separate versions for viztab, regiontab
 ******************************************************************************/
void VizWinMgr::
setVpLocalGlobal(int val){
	int activeViz = getActiveViz();
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
		getViewpointRouter()->guiSetLocal((ViewpointParams*)ControlExec::GetParams(activeViz,Params::_viewpointParamsTag,-1),false);
		getViewpointRouter()->updateTab();
	
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetParams(activeViz,Params::_viewpointParamsTag, -1);
		if (!vpParams){
			//create a new parameter panel, copied from global
			vpParams = (ViewpointParams*)ControlExec::GetDefaultParams(Params::_viewpointParamsTag)->deepCopy();
			vpParams->SetVizNum(activeViz);
			getViewpointRouter()->guiSetLocal(vpParams,true);
			//No need to refresh anything, since the new parameters are same as old! 
		} else { //need to revert to existing local settings:
			getViewpointRouter()->guiSetLocal(vpParams,true);
			getViewpointRouter()->updateTab();
			
			//and then refresh the panel:
			tabManager->show();
		}
	}
}

/*****************************************************************************
 * Called when the region tab local/global selector is changed.
 * Affects only the front region tab
 ******************************************************************************/
void VizWinMgr::
setRgLocalGlobal(int val){
	int activeViz = getActiveViz();
	//If changes  to global, just revert to global panel.
	//If changes to local, may need to create a new local panel
	//In either case need to force redraw.
	//If change to local, and in region mode, need to redraw all global windows
	
	if (val == 0){//toGlobal.  
		//First set the global status, 
		//then put  values in tab based on global settings.
		//Note that updateDialog will trigger events changing values
		//on the current dialog
		getRegionRouter()->guiSetLocal((RegionParams*)ControlExec::GetParams(activeViz,Params::_regionParamsTag,-1),false);
		getRegionRouter()->updateTab();
		tabManager->show();
	} else { //Local: Do we need to create new parameters?
		 //need to revert to existing local settings:
		getRegionRouter()->guiSetLocal((RegionParams*)ControlExec::GetParams(activeViz,Params::_regionParamsTag,-1),true);
		getRegionRouter()->updateTab();
			
		//and then refresh the panel:
		tabManager->show();
	}

	//Specify if the active viz is sharing the region
	
	Visualizer::setRegionShareFlag(val == 0);
	
	//in region mode, refresh the global windows and this one
	if (MouseModeParams::GetCurrentMouseMode() == MouseModeParams::regionMode){
		map<int,VizWin*>::iterator it;
		for (it = VizWindow.begin(); it != VizWindow.end(); it++){
			int i = it->first;
			if((!getRegionParams(i)->IsLocal()|| (i == activeViz)))
				(it->second)->updateGL();
		}
	}
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




//Force all renderers to re-obtain render data
void VizWinMgr::refreshRenderData(){
	
	for (int i = 0; i<ControlExec::GetNumVisualizers(); i++){
	
		Visualizer* viz= ControlExec::GetVisualizer(i);
		for (int j = 0; j< viz->getNumRenderers(); j++){
			Renderer* ren = viz->getRenderer(j);
			ren->setAllDataDirty();
		}
		int winnum = viz->getWindowNum();
		VizWindow[winnum]->updateGL();	
	}
}

//
//Disable all renderers that use specified variables
void VizWinMgr::disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D){
	
	for (int i = 0; i<ControlExec::GetNumVisualizers(); i++){
		Visualizer* viz = ControlExec::GetVisualizer(i);
		int numRens = viz->getNumRenderers();
		for (int j = numRens-1; j >= 0; j--){
			Renderer* ren = viz->getRenderer(j);
			RenderParams* rParams = ren->getRenderParams();
			for (int k = 0; k<vars2D.size(); k++){
				if(rParams->usingVariable(vars2D[k])){
					Params::ParamsBaseType t = rParams->GetParamsBaseTypeId();
					int instance = ControlExec::FindInstanceIndex(i, rParams);
					EventRouter* er = getEventRouter(t);
					er->guiSetEnabled(false,instance, false);
				}
			}
			for (int k = 0; k<vars3D.size(); k++){
				if(rParams->usingVariable(vars3D[k])){
					Params::ParamsBaseType t = rParams->GetParamsBaseTypeId();
					int instance = ControlExec::FindInstanceIndex(i, rParams);
					EventRouter* er = getEventRouter(t);
					er->guiSetEnabled(false,instance, false);
				}
			}
		}
	}
}
//Disable all renderers 
void VizWinMgr::disableAllRenderers(){
	
	int numViz = ControlExec::GetNumVisualizers();
	for (int i = 0; i<numViz; i++){
		Visualizer* viz = ControlExec::GetVisualizer(i);
		viz->removeAllRenderers();
	}
}

	
Params::ParamsBaseType VizWinMgr::RegisterEventRouter(const std::string tag, EventRouter* router){
	Params::ParamsBaseType t = ControlExec::GetTypeFromTag(tag);
	if (t <= 0) return 0;
	eventRouterMap.insert(pair<int,EventRouter*>(t,router));
	return t;
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

void VizWinMgr::forceRender(const Params* p, bool always){
	if (p->isRenderParams()){
		RenderParams* rp = (RenderParams*)p;
		if (!always || !rp->IsEnabled()) return;
		int viznum = rp->GetVizNum();
		if (viznum < 0) return;
		VizWindow[viznum]->reallyUpdate();
	} else { //force all windows to update
		map<int, VizWin*>::const_iterator it;
		for (it = VizWindow.begin(); it != VizWindow.end(); it++){
			(it->second)->reallyUpdate();
		}
	}
}

int VizWinMgr::getActiveViz() {
	return ControlExec::GetActiveVizIndex();
}
ViewpointParams* VizWinMgr::getViewpointParams(int winNum){
	return (ViewpointParams*)ControlExec::GetCurrentParams(winNum,Params::_viewpointParamsTag);
}
RegionParams*  VizWinMgr::getRegionParams(int winNum){
	return (RegionParams*)ControlExec::GetCurrentParams(winNum,Params::_regionParamsTag);
}
AnimationParams*  VizWinMgr::getAnimationParams(int winNum){
	return (AnimationParams*)ControlExec::GetCurrentParams(winNum,Params::_animationParamsTag);
}

void VizWinMgr::SetToDefaults(){
	//Clear out the vizselectcombo
	//remove all vizwins
	map<int, VizWin*>::iterator it = VizWindow.begin();
	//First disable them
	for (it = VizWindow.begin(); it != VizWindow.end(); it++)
		(it->second)->setEnabled(false);
	it = VizWindow.begin();
	while (it != VizWindow.end()){
		map<int, VizWin*>::iterator it2 = it;
		++it;
		killViz(it2->first);
	}
	VizMdiWin.clear();
	VizWindow.clear();
}
void VizWinMgr::UndoRedo(bool isUndo, int /*instance*/, Params* beforeP, Params* afterP){
	//Determine if this is a creation or destruction of a visualizer
	VizWinParams* vizwinBefore = (VizWinParams*)beforeP;
	VizWinParams* vizwinAfter = (VizWinParams*)afterP;
	vector<long> beforeNums = vizwinBefore->getVisualizerNums();
	vector<long> afterNums = vizwinAfter->getVisualizerNums();
	if (beforeNums.size() != afterNums.size()){
		int createdNum = -1, destroyedNum = -1;

		//find which one was created or destroyed
		if (beforeNums.size()<afterNums.size()){ //look for an afterNum that was not in beforeNum
			for (int i = 0; i<afterNums.size(); i++){
				bool match = false;
				for (int j = 0; j<beforeNums.size(); j++){
					if (beforeNums[j] == afterNums[i]) {match = true; break;}
				}
				if (!match) { //We did not find a match to afterNums[i];
					createdNum = afterNums[i];
					break;
				}
			}
		} else { //look for a before num that is not in after nums
			for (int i = 0; i<beforeNums.size(); i++){
				bool match = false;
				for (int j = 0; j<afterNums.size(); j++){
					if (beforeNums[i] == afterNums[j]) {match = true; break;}
				}
				if (!match) { //We did not find a match to beforeNums[i];
					destroyedNum = beforeNums[i];
					break;
				}
			}
		}
		//We should have found either a visualizer creation or destruction
		assert(createdNum >= 0 || destroyedNum >= 0);
		if ((isUndo && createdNum >=0) || (!isUndo && destroyedNum >=0)){
			//Need to undo a creation or redo a destruction; i.e. need to remove a visualizer
			int loseVizNum;
			if (isUndo) loseVizNum = createdNum; else loseVizNum = destroyedNum;
			VizWinMgr* vizMgr = VizWinMgr::getInstance();
			vizMgr->removeVisualizer(loseVizNum);
			return;
		} else { //Need to add a visualizer
			int newVizNum;
			if (isUndo) newVizNum = destroyedNum; else newVizNum = createdNum;

			int useWindowNum = ControlExec::NewVisualizer(newVizNum);
			VizWinMgr* vizMgr = VizWinMgr::getInstance();
			vizMgr->addVisualizer(useWindowNum);
			return;
		}
	} else {  //deal with change of current visualizer
		
		int prevWin = vizwinBefore->getCurrentVizWin();
		int nextWin = vizwinAfter->getCurrentVizWin();
		int newCurrentWin;
		if (prevWin != nextWin){
			if (isUndo) newCurrentWin = prevWin;
			else newCurrentWin = nextWin;
			VizWinMgr* vizMgr = VizWinMgr::getInstance();
			vizMgr->emit activateViz(newCurrentWin);
			vizMgr->VizWindow[newCurrentWin]->setFocus();
			
		
			vizMgr->ActivationOrder[newCurrentWin]= (++vizMgr->activationCount);
			//Determine if the local viewpoint dialog applies, update the tab dialog
			//appropriately
			vizMgr->updateActiveParams();
			//Set the animation toolbar to the correct frame number:
			int currentTS = getActiveAnimationParams()->getCurrentTimestep();
			MainForm::getInstance()->setCurrentTimestep(currentTS);
			//Tell the Visualizers who is active
			Visualizer::setActiveWinNum(newCurrentWin);
			//Determine if the active viz is sharing the region
			Visualizer::setRegionShareFlag(!vizMgr->getRegionParams(newCurrentWin)->IsLocal());

			vizMgr->tabManager->show();
		
		
			//Need to cause a redraw in all windows if we are not in navigate mode,
			//So that the manips will change where they are drawn:
			if (MouseModeParams::GetCurrentMouseMode() != MouseModeParams::navigateMode){
				map<int,VizWin*>::iterator it;
				for (it = vizMgr->VizWindow.begin(); it !=  vizMgr->VizWindow.end(); it++){
					(it->second)->updateGL();
				}
			}
		}
	}
}
int VizWinMgr::addVisualizer(int useWindowNum){
	createDefaultParams(useWindowNum);
		
    QPoint* topLeft = new QPoint(0,0);
    QSize* minSize = new QSize(400, 400);
    QRect* newRect = new QRect (*topLeft, *minSize);

    //Don't record events generated by window activation here: 
	std::stringstream out;
	out << useWindowNum;
	string win = out.str();
	
    string vizname = string("Visualizer No. ") +win;
	QString qvizname = QString::fromStdString(vizname);
	VizWinParams::SetVizName(useWindowNum, vizname);
	//notify the window selector:
	emit (newViz(qvizname, useWindowNum));
	
	VizWindow[useWindowNum] = new VizWin (MainForm::getInstance(), qvizname, this, newRect, useWindowNum);
	
	QMdiSubWindow* qsbw = MainForm::getInstance()->getMDIArea()->addSubWindow(VizWindow[useWindowNum]);
	VizMdiWin[useWindowNum]=qsbw;
	
	VizWindow[useWindowNum]->setWindowNum(useWindowNum);
	VizWindow[useWindowNum]->setFocusPolicy(Qt::ClickFocus);
	VizWindow[useWindowNum]->setWindowTitle(qvizname);


	setActiveViz(useWindowNum);

	VizWindow[useWindowNum]->showMaximized();
	
	int numWins = VizWinParams::GetNumVizWins();
	//Tile if more than one visualizer:
	if(numWins > 1) fitSpace();

	//Set non-renderer tabbed panels to use global parameters:
	
	emit activateViz(useWindowNum);
	VizWindow[useWindowNum]->show();

	//When we go from 1 to 2 windows, need to enable multiple viz panels and signals.
	if (numWins > 1){
		emit enableMultiViz(true);
	}

	return useWindowNum;
}
void VizWinMgr::removeVisualizer(int viznum){
	killViz(viznum);
	fitSpace();
}
