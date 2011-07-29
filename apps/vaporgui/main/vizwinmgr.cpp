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

#include "params.h"

#include "vapor/ExtensionClasses.h"
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
    previousClass = 0;
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
	setSelectionMode(GLWindow::navigateMode);
}
//Create the global params and the default renderer params:
void VizWinMgr::
createAllDefaultParams() {
	ParamsBase::RegisterParamsBaseClass(Box::_boxTag, Box::CreateDefaultInstance, false);
	ParamsBase::RegisterParamsBaseClass(TransferFunction::_transferFunctionTag, TransferFunction::CreateDefaultInstance, false);
	ParamsBase::RegisterParamsBaseClass(MapperFunctionBase::_mapperFunctionTag, MapperFunction::CreateDefaultInstance, false);
	ParamsBase::RegisterParamsBaseClass(ParamsIso::_IsoControlTag, IsoControl::CreateDefaultInstance, false);
	//Animation comes first because others will refer to it.
	ParamsBase::RegisterParamsBaseClass(Params::_animationParamsTag, AnimationParams::CreateDefaultInstance, true);
	InstallTab(Params::_animationParamsTag, AnimationEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_viewpointParamsTag, ViewpointParams::CreateDefaultInstance, true);
	InstallTab(Params::_viewpointParamsTag, ViewpointEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_regionParamsTag, RegionParams::CreateDefaultInstance, true);
	InstallTab(Params::_regionParamsTag, RegionEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_twoDDataParamsTag, TwoDDataParams::CreateDefaultInstance, true);
	//For backwards compatibility; the tag changed in vapor 1.5
	ParamsBase::ReregisterParamsBaseClass(Params::_twoDParamsTag, Params::_twoDDataParamsTag, true);
	InstallTab(Params::_twoDDataParamsTag, TwoDDataEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_twoDImageParamsTag, TwoDImageParams::CreateDefaultInstance, true);
	InstallTab(Params::_twoDImageParamsTag, TwoDImageEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_probeParamsTag, ProbeParams::CreateDefaultInstance, true);
	InstallTab(Params::_probeParamsTag, ProbeEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_isoParamsTag, ParamsIso::CreateDefaultInstance, true);
	InstallTab(Params::_isoParamsTag, IsoEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_flowParamsTag, FlowParams::CreateDefaultInstance, true);
	InstallTab(Params::_flowParamsTag, FlowEventRouter::CreateTab);
	ParamsBase::RegisterParamsBaseClass(Params::_dvrParamsTag, DvrParams::CreateDefaultInstance, true);
	InstallTab(Params::_dvrParamsTag, DvrEventRouter::CreateTab);

	//Install Extension Classes:
	InstallExtensions();
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
			delete vizMdiWin[i];
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
	vizMdiWin[i] = 0;
	
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
	vizMdiWin[useWindowNum]=MainForm::getInstance()->getMDIArea()->addSubWindow(vizWin[useWindowNum]);
	
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
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		getEventRouter(i)->updateTab();
	}
	
	//Also update the activeParams in the GLWindow:
	GLWindow* glwin = getVizWin(activeViz)->getGLWindow();
	
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
 *  Methods for changing the parameter panels.  Only done during undo/redo.
 *  If instance is -1, then changes the current instance of renderParams
 *  For enabled render params, change the rendererMapping so that it will
 *  map the new params to the existing renderer.
 */

void VizWinMgr::setParams(int winnum, Params* p, ParamsBase::ParamsBaseType clsId, int inst){
	
	assert(clsId > 0);
	
	if (!p || (!p->isRenderParams())){
		assert (inst == -1);
		
		if (winnum < 0) { //global params!
			Params* globalParams = getGlobalParams(clsId);
			if (globalParams) delete globalParams;
			if (p) setGlobalParams(p->deepCopy(),clsId);
			else setGlobalParams(0,clsId);
			globalParams = getGlobalParams(clsId);
			//Set all windows that use global params:
			for (int i = 0; i<MAXVIZWINS; i++){
				if (!vizWin[i]) continue;
				Params* realParams = Params::GetParamsInstance(clsId,i,-1);
				if (realParams && !realParams->isLocal()){
					vizWin[i]->getGLWindow()->setActiveParams(globalParams,clsId);
				}
			}
		} else {
			Params* realParams = Params::GetParamsInstance(clsId,winnum,-1);
			if(realParams) delete realParams;
			if (p) realParams = p->deepCopy();
			else realParams = 0;
			vizWin[winnum]->getGLWindow()->setActiveParams(realParams,clsId);
		}
	} else {  //Handle render params:
		assert(p);
		
		if (inst == -1) inst = getCurrentInstanceIndex(winnum, clsId);
		//Save the previous render params pointer.  It might be needed
		//to enable/disable renderer
		RenderParams* prevRParams = (RenderParams*)getParams(winnum,clsId,inst);
		bool isRendering = (prevRParams != 0) && prevRParams->isEnabled();
		RenderParams* newRParams = 0;
		GLWindow* glwin = vizWin[winnum]->getGLWindow();
		
		
		if (Params::GetNumParamsInstances(clsId,winnum) > inst){
			if (Params::GetParamsInstance(clsId, winnum, inst)){
				delete Params::GetParamsInstance(clsId, winnum, inst);
			}
			Params::GetAllParamsInstances(clsId,winnum)[inst] = p;
		} else {
			Params::AppendParamsInstance(clsId,winnum, p);
		}
		newRParams = (RenderParams*)p;
		
		
		if (inst == getCurrentInstanceIndex(winnum, clsId))
			glwin->setActiveParams(newRParams, clsId);
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
		if (!vizWin[i]) continue;
		AnimationParams* animationParams = (AnimationParams*)Params::GetParamsInstance(Params::_animationParamsTag,i);
		if  (( i != vizNum)  &&  (!animationParams)||!animationParams->isLocal()){
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
		if (!vizWin[i]) continue;
		Params* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, i);
		if  (  (i != vizNum)  && ((!vpParams)||!vpParams->isLocal())
			){
			vizWin[i]->getGLWindow()->setViewerCoordsChanged(true);
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
		GLWindow* glw = vizWin[vizNum]->getGLWindow();
		if(glw) glw->resetView(vp);
	}
	if(vp->isLocal()) return;
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!vizWin[i]) continue;
		Params* vpParams = (ViewpointParams*)Params::GetParamsInstance(Params::_viewpointParamsTag, i);
		if  ( (i != vizNum)  && ((!vpParams)||!vpParams->isLocal())){
			GLWindow* glw = vizWin[i]->getGLWindow();
			if(glw) glw->resetView(vp);
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
			(getRealAnimationParams(i) == aParams)
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
		if(getRealAnimationParams(activeViz))getAnimationRouter()->guiSetLocal(getRealAnimationParams(activeViz),false);
		getAnimationRouter()->updateTab();
		vizWin[activeViz]->getGLWindow()->setActiveAnimationParams(getGlobalAnimationParams());
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
		//set the local params in the glwindow:
		vizWin[activeViz]->getGLWindow()->setActiveAnimationParams(getRealAnimationParams(activeViz));
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
		vizWin[activeViz]->getGLWindow()->setActiveViewpointParams((ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag));
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
		vizWin[activeViz]->getGLWindow()->setActiveViewpointParams(vpParams);
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
		vizWin[activeViz]->getGLWindow()->setActiveRegionParams(getGlobalRegionParams());
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
		vizWin[activeViz]->getGLWindow()->setActiveRegionParams(getRealRegionParams(activeViz));
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



//For a renderer, there should exist a local version.
ProbeParams* VizWinMgr::
getProbeParams(int winNum, int instance){
	return (ProbeParams*)Params::GetParamsInstance(Params::_probeParamsTag,winNum,instance);
}
//For a renderer, there should exist a local version.
TwoDImageParams* VizWinMgr::
getTwoDImageParams(int winNum, int instance){
	return (TwoDImageParams*)Params::GetParamsInstance(Params::_twoDImageParamsTag,winNum,instance);
}
TwoDDataParams* VizWinMgr::
getTwoDDataParams(int winNum, int instance){
	return (TwoDDataParams*)Params::GetParamsInstance(Params::_twoDDataParamsTag,winNum,instance);
}
AnimationParams* VizWinMgr::
getAnimationParams(int winNum){
	if (winNum < 0) return getGlobalAnimationParams();
	if (getRealAnimationParams(winNum) && getRealAnimationParams(winNum)->isLocal()) return getRealAnimationParams(winNum);
	return getGlobalAnimationParams();
}

FlowParams* VizWinMgr::
getFlowParams(int winNum, int instance){
	return (FlowParams*)Params::GetParamsInstance(Params::_flowParamsTag,winNum,instance);
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
			vizWin[i]->getGLWindow()->setViewerCoordsChanged(true);
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

	getRegionRouter()->refreshRegionInfo(getGlobalRegionParams());
	//NOTE that the vpparams need to be initialized after
	//the global region params, since they use its settings..
	//
	getGlobalVPParams()->reinit(doOverride);
	
	for (int i = 0; i< MAXVIZWINS; i++){
		if (!vizWin[i]) continue;
		if(getRealVPParams(i)) getRealVPParams(i)->reinit(doOverride);
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
		
		if(getRealAnimationParams(i)) getRealAnimationParams(i)->reinit(doOverride);
		//setup near/far
		if (getRealVPParams(i)) vizWin[i]->getGLWindow()->resetView(getViewpointParams(i));
		if(!doOverride && vizWin[i]) vizWin[i]->getGLWindow()->removeAllRenderers();
	}

    //
    // Reinitialize all tabs
    //
	for (int pType = 1; pType <= Params::GetNumParamsClasses(); pType++){
		EventRouter* eRouter = getEventRouter(pType);
		eRouter->reinitTab(doOverride);
	}
	
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
	getRegionRouter()->refreshRegionInfo(getGlobalRegionParams());
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
			GLWindow* glwin= vizWin[i]->getGLWindow();
			for (int j = 0; j< glwin->getNumRenderers(); j++){
				Renderer* ren = glwin->getRenderer(j);
				ren->setAllDataDirty();
			}
			vizWin[i]->updateGL();
		}
	}
}
//Force all renderers to reload their shader data
bool VizWinMgr::reloadShaders(){
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]){
			GLWindow* glwin= vizWin[i]->getGLWindow();
			for (int j = 0; j< glwin->getNumRenderers(); j++){
				Renderer* ren = glwin->getRenderer(j);
				if (std::string::npos != std::string(typeid(*ren).name()).find("IsoRenderer") || std::string::npos != std::string(typeid(*ren).name()).find("VolumeRenderer")) {
					//found iso or volume renderer
					return false;
				}
			}
			if (glwin->getShaderMgr()->reloadShaders() == false){
				return false;
			}
		}
	}
	return true;
}
//
//Disable all renderers that use specified variables
void VizWinMgr::disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D){
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]){
			GLWindow* glwin= vizWin[i]->getGLWindow();
			int numRens = glwin->getNumRenderers();
			for (int j = numRens-1; j >= 0; j--){
				Renderer* ren = glwin->getRenderer(j);
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
}
//Disable all renderers 
void VizWinMgr::disableAllRenderers(){
	int firstwin = -1;
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]){
			if (firstwin == -1) firstwin = i;
			GLWindow* glwin= vizWin[i]->getGLWindow();
			setActiveViz(i);
			for (int j = glwin->getNumRenderers()-1; j>= 0; j--){
				Renderer* ren = glwin->getRenderer(j);
				RenderParams* rParams = ren->getRenderParams();
				Params::ParamsBaseType t = rParams->GetParamsBaseTypeId();
				tabManager->moveToFront(t);
				int instance = findInstanceIndex(i, rParams, t);
				EventRouter* er = getEventRouter(t);
				
				er->performGuiChangeInstance(instance, false);
				
				er->guiSetEnabled(false,instance,false);	
			}
			//In case a renderer was left behind due to undo/redo
			glwin->removeAllRenderers();
		}
	}
	if (firstwin != -1){
		//Set the "first" one to be active:
		setActiveViz(firstwin);
		GLWindow::setActiveWinNum(firstwin);
	}
}
void VizWinMgr::
setSelectionMode( int m){ 
	GLWindow::setCurrentMouseMode(m);
	//Update all visualizers:
	for (int i = 0; i<MAXVIZWINS; i++){
		if(vizWin[i]) vizWin[i]->updateGL();
	}
}

ParamNode* VizWinMgr::buildNode() { 
	//Create a visualizers node, put in one child for each visualizer
	
	string empty;
	std::map <string, string> attrs;
	attrs.empty();
	ostringstream oss;
	ParamNode* vizMgrNode = new ParamNode(_visualizersTag, attrs, getNumVisualizers());
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]){
			attrs.empty();
			attrs[_vizWinNameAttr] = vizName[i].toStdString();
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
				oss << vizWin[i]->getDisplacement();
			attrs[_vizElevGridDisplacementAttr] = oss.str();

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

			int ptypeId = vizWin[i]->getColorbarParamsTypeId();
			string pname = Params::GetTagFromType(ptypeId);
			attrs[_vizColorbarParamsNameAttr] = pname;

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
			oss << (int) vizWin[i]->getColorbarDigits();
			attrs[_vizColorbarDigitsAttr] = oss.str();
			
			oss.str(empty);
			oss << (int) vizWin[i]->getColorbarFontsize();
			attrs[_vizColorbarFontsizeAttr] = oss.str();

			oss.str(empty);
			oss << (int) i;
			attrs[_visualizerNumAttr] = oss.str();
			
			ParamNode* locals = new ParamNode(_vizWinTag, attrs, 5);
			vizMgrNode->AddChild(locals);
			//Loop over all the local params that exist for this window:
			
			for (int pclass = 1; pclass<= Params::GetNumParamsClasses(); pclass++){
				for (int inst = 0; inst<Params::GetNumParamsInstances(pclass,i); inst++){
					Params* p = Params::GetParamsInstance(pclass,i,inst);
					ParamNode* pnode = p->buildNode();
					if (pnode) locals->AddChild(pnode);
				}
			}
			for (int i = 0; i< Params::getNumDummyClasses(); i++){
				Params* p = Params::getDummyParamsInstance(i);
				ParamNode* pnode = p->buildNode();
				if (pnode) locals->AddChild(pnode);
			}

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
		int colorbarParamsTypeId = Params::GetTypeFromTag(Params::_dvrParamsTag);
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
		int colorbarFontsize = 10;
		int colorbarDigits = 3;
		int colorbarTics = 11;
		bool axesEnabled = false;
		bool axisAnnotationEnabled = false;
		bool colorbarEnabled = false;
		bool regionEnabled = DataStatus::regionFrameIsEnabled();
		bool subregionEnabled = DataStatus::subregionFrameIsEnabled();
		bool elevGridEnabled = false;
		int elevGridRefinement = 0;
		float elevGridDisplacement = 0;
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
				//obsolete
			}
			else if (StrCmpNoCase(attr, _vizElevGridInvertedAttr) == 0) {
				//obsolete
			}
			else if (StrCmpNoCase(attr, _vizElevGridTexturedAttr) == 0) {
				//obsolete
			}
			else if (StrCmpNoCase(attr, _vizElevGridTextureNameAttr) == 0) {
				//obsolete
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
			else if (StrCmpNoCase(attr, _vizColorbarDigitsAttr) == 0) {
				ist >> colorbarDigits;
			}
			else if (StrCmpNoCase(attr, _vizColorbarFontsizeAttr) == 0) {
				ist >> colorbarFontsize;
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
			else if (StrCmpNoCase(attr, _vizColorbarParamsNameAttr) == 0) {
				colorbarParamsTypeId = ParamsBase::GetTypeFromTag(value);
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
		parsingInstance.clear();
		for (int i = 0; i<= Params::GetNumParamsClasses(); i++)
			parsingInstance.push_back(-1);
		
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
		vizWin[parsingVizNum]->setColorbarDigits(colorbarDigits);
		vizWin[parsingVizNum]->setColorbarFontsize(colorbarFontsize);
		vizWin[parsingVizNum]->setColorbarParamsTypeId(colorbarParamsTypeId);
		
		return true;
		}
	case(3):
		{
		//push the subsequent parsing to the params for current window 
		if (parsingVizNum < 0) return false;//we should have already created a visualizer
		Params::ParamsBaseType typeId = Params::GetTypeFromTag(tag);
		if (typeId <= 0) {
			MessageReporter::errorMsg("Unrecognized Params tag: %s",tag.c_str());
			Params* dummyParams = Params::CreateDummyParams(tag);
			Params::addDummyParamsInstance(dummyParams);
			pm->pushClassStack(dummyParams);
			dummyParams->elementStartHandler(pm,depth,tag,attrs);
			return true;
		}
		parsingInstance[typeId]++;
		Params* parsingParams;
		if (parsingInstance[typeId] > 0){
			parsingParams = Params::CreateDefaultParams(typeId);
			assert(parsingParams->isRenderParams());
			Params::AppendParamsInstance(typeId,parsingVizNum, parsingParams);
		} else {
			parsingParams = Params::GetParamsInstance(typeId,parsingVizNum, 0);
		}
		assert(Params::GetNumParamsInstances(typeId,parsingVizNum) == (parsingInstance[typeId] + 1));
		//"push" to params parser. It will pop back to vizwinmgr when done.
		EventRouter* eRouter = getEventRouter(typeId);
		eRouter->cleanParams(parsingParams);
		pm->pushClassStack(parsingParams);
		parsingParams->elementStartHandler(pm, depth, tag, attrs);
		if (parsingInstance[typeId] == 0){
			if (parsingParams->isLocal())
				vizWin[parsingVizNum]->getGLWindow()->setActiveParams(parsingParams, typeId);
			else 
				vizWin[parsingVizNum]->getGLWindow()->setActiveParams(Params::GetDefaultParams(typeId), typeId);
		}
		//Workaround 2.0.0 bug:  viznum may not be properly set in session file
		if (!parsingParams->isRenderParams()){
			parsingParams->setVizNum(parsingVizNum);
		}
		return true;
		}
		
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
		int typId = p->GetParamsBaseTypeId();
		for (int i = 0; i<MAXVIZWINS; i++){
			vw = getVizWin(i);
			if(!vw) continue;
			Params* realParams = Params::GetParamsInstance(typId,i,-1);
			if (!realParams || realParams->isLocal()) continue;
			
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
	vw->setColorbarDirty(true);
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
	vw->setColorbarDirty(true);
	flowRend->setGraphicsDirty();
	p->setAllBypass(false);
	vw->updateGL();
}
//Set all the twoD (data and image) renderers dirty,
//So they will all reconstruct their terrain mappings
void VizWinMgr::setAllTwoDElevDirty(){
	for (int i = 0; i<MAXVIZWINS; i++){
		if (vizWin[i]){
			for (int j = 0; j< Params::GetNumParamsInstances(Params::_twoDDataParamsTag,i); j++){
				TwoDDataParams* dParams = (TwoDDataParams*)Params::GetParamsInstance(Params::_twoDDataParamsTag,i,j);
				dParams->setElevGridDirty(true);
			}
			for (int j = 0; j< Params::GetNumParamsInstances(Params::_twoDImageParamsTag,i); j++){
				TwoDImageParams* dParams = (TwoDImageParams*)Params::GetParamsInstance(Params::_twoDImageParamsTag,i,j);
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
	vw->setColorbarDirty(true);
	p->setAllBypass(false);
	vw->updateGL();
}

void VizWinMgr::setFlowDisplayListDirty(FlowParams* p){
	if (!(DataStatus::getInstance()->getDataMgr())) return;
	VizWin* vw = getVizWin(p->getVizNum());
	if (!vw) return;
	GLWindow* glwin = vw->getGLWindow();
	if (!glwin) return;
	FlowRenderer* flowRend = (FlowRenderer*)glwin->getRenderer(p);
	if(flowRend) {
		flowRend->setDisplayListDirty();
		flowRend->paintGL();
	}
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
	
void VizWinMgr::setInteractiveNavigating(int level){
	DataStatus::setInteractiveRefinementLevel(level);
	for (int i = 0; i< MAXVIZWINS; i++){
		if (vizWin[i]) vizWin[i]->setDirtyBit(NavigatingBit, true);
	}
}
//Determine (for error checking) if there are any enabled 2d renderers in the window that match
//coordinate, terrain mapping
bool VizWinMgr::findCoincident2DSurface(int vizwin, int orientation, float coordinate, bool terrainMapped){
	vector<Params*>& dparams = Params::GetAllParamsInstances(Params::_twoDDataParamsTag,vizwin);
	vector<Params*>& iparams = Params::GetAllParamsInstances(Params::_twoDImageParamsTag,vizwin);
	const float * extents = DataStatus::getInstance()->getExtents();
	float tol = (extents[orientation+3]-extents[orientation])*0.0001f;
	for (int i = 0; i< dparams.size(); i++){
		TwoDDataParams* p = (TwoDDataParams*)dparams[i];
		if (!p->isEnabled()) continue;
		if (p->getOrientation() != orientation) continue;
		if (p->isMappedToTerrain() != terrainMapped) continue;
		if (abs(p->getTwoDMin(orientation) - coordinate)> tol) continue;
		return true;
	}
	for (int i = 0; i< iparams.size(); i++){
		TwoDImageParams* p = (TwoDImageParams*)iparams[i];
		if (!p->isEnabled()) continue;
		if (p->getOrientation() != orientation) continue;
		if (p->isMappedToTerrain() != terrainMapped) continue;
		if (abs(p->getTwoDMin(orientation) - coordinate)> tol) continue;
		return true;
	}
	return false;
}
//Stop all the unsteady flow integrations that are occurring in any visualizer
void VizWinMgr::stopFlowIntegration(){
	//Do the same as a stop click on the available router
	FlowEventRouter* fRouter = getFlowRouter();
	fRouter->stopClicked();
	//Then check for any other active flow rendering
	for (int i = 0; i< MAXVIZWINS; i++){
		if(vizWin[i]){
			for (int j = 0; j<Params::GetNumParamsInstances(Params::_flowParamsTag,i); j++){
				FlowParams* fParams = getFlowParams(i,j);
				if (fParams->isEnabled() && !fParams->flowIsSteady()){
					fParams->setStopFlag(true);
				}
			}
		}
	}
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
	RegisterMouseMode(Params::_flowParamsTag,1, "Flow rake",rake );
	RegisterMouseMode(Params::_probeParamsTag,3,"Probe", probe);
	RegisterMouseMode(Params::_twoDDataParamsTag,2,"2D Data", twoDData);
	RegisterMouseMode(Params::_twoDImageParamsTag,2, "Image",twoDImage);
	InstallExtensionMouseModes();
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
	int checkMode = GLWindow::AddMouseMode(paramsTag, manipType, name);
	assert(checkMode == newMode);
	return newMode;
}
void VizWinMgr::InstallTab(const std::string tag, EventRouterCreateFcn fcn){

	
	EventRouter* eRouter = fcn();
	Params::ParamsBaseType typ = RegisterEventRouter(tag, eRouter);
	eRouter->hookUpTab();
	QWidget* tabWidget = dynamic_cast<QWidget*> (eRouter);
	assert(tabWidget);
	Session::getInstance()->blockRecording();
	tabManager->addWidget(tabWidget, typ);
	Session::getInstance()->unblockRecording();
	
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
FlowEventRouter* VizWinMgr::
getFlowRouter() {
	return (FlowEventRouter*)getEventRouter(Params::_flowParamsTag);
}
ProbeEventRouter* VizWinMgr::
getProbeRouter() {
	return (ProbeEventRouter*)getEventRouter(Params::_probeParamsTag);
}
TwoDDataEventRouter* VizWinMgr::
getTwoDDataRouter() {
	return (TwoDDataEventRouter*)getEventRouter(Params::_twoDDataParamsTag);
}
TwoDImageEventRouter* VizWinMgr::
getTwoDImageRouter() {
	return (TwoDImageEventRouter*)getEventRouter(Params::_twoDImageParamsTag);
}
void VizWinMgr::forceRender(RenderParams* rp){
	if(!rp->isEnabled()) return;
	int viznum = rp->getVizNum();
	if (viznum < 0) return;
	vizWin[viznum]->updateGL();
}
