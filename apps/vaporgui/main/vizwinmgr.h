//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizwinmgr.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Sept 2004
//
//	Description:  Definition of VizWinMgr class	
//		This class manages the VizWin visualizers
//		Its main function is to catch events from the visualizers and
//		to route them to the appropriate params class, and in reverse,
//		to route events from tab panels to the appropriate visualizer.
//		This class knows about which visualizers are active, and what
//		state to associate with them, so other classes request the VizWinMgr
//		to perform tasks that require that information
//

#ifndef VIZWINMGR_H
#define VIZWINMGR_H



class QWorkspace;
class QWidget;
class QDesktopWidget;
class QMainWidget;
class QTimer;

#include <qobject.h>
#include "twoDparams.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "probeparams.h"

#include "flowparams.h"
#include "dvrparams.h"
#include "animationparams.h"
#include "ParamsIso.h"
#include "vaporinternal/common.h"
#include "params.h"
#include "command.h"

class QPopupMenu;




namespace VAPoR{
class AnimationController;
class AnimationParams;

class EventRouter;
class TabManager;
class MainForm;
class ViewpointParams;
class RegionParams;
class DvrParams;
class ProbeParams;
class TwoDParams;
class Trackball;
class VizWin;
class AnimationEventRouter;
class RegionEventRouter;
class DvrEventRouter;
class ProbeEventRouter;
class TwoDEventRouter;
class ViewpointEventRouter;
class FlowEventRouter;
class IsoEventRouter;


class VizWinMgr : public QObject, public ParsedXml
{
	Q_OBJECT

    enum 
    {
      PREAMBLE,
      RENDER,
      TEMPORAL,
      TFEDIT,
      DONE
    };

public:
	static VizWinMgr* getInstance() {
		if (!theVizWinMgr)
			theVizWinMgr = new VizWinMgr();
		return theVizWinMgr;
	}
	//Get active params always uses current instance on renderparams.
	static DvrParams* getActiveDvrParams(){
		return (getInstance()->getDvrParams(getInstance()->activeViz));}
	static ParamsIso* getActiveIsoParams(){
		return (getInstance()->getIsoParams(getInstance()->activeViz));}
	static ProbeParams* getActiveProbeParams(){
		return (getInstance()->getProbeParams(getInstance()->activeViz));}
	static TwoDParams* getActiveTwoDParams(){
		return (getInstance()->getTwoDParams(getInstance()->activeViz));}
	static RegionParams* getActiveRegionParams(){
		return (getInstance()->getRegionParams(getInstance()->activeViz));}
	static FlowParams* getActiveFlowParams(){
		return (getInstance()->getFlowParams(getInstance()->activeViz));}
	static ViewpointParams* getActiveVPParams(){
		return (getInstance()->getViewpointParams(getInstance()->activeViz));}
	static AnimationParams* getActiveAnimationParams(){
		return (getInstance()->getAnimationParams(getInstance()->activeViz));}
	
	void createGlobalParams();
    ~VizWinMgr();
	
    //Respond to end:
    void closeEvent();
   
    void vizAboutToDisappear(int i); 

	
    //Public Inlines:
    //Respond to user events,
    //Make sure this state tracks actual window state.
    void minimize(int i) {
        isMin[i] = true;
        isMax[i] = false;
    }
    void maximize(int i) {
        isMin[i] = false;
        isMax[i] = true;
    
    }
    void normalize(int i) {
        isMin[i] = false;
        isMax[i] = false;
    }
	static EventRouter* getEventRouter(Params::ParamType routerType);

	Params* getGlobalParams(Params::ParamType);
	void setGlobalParams(Params* p, Params::ParamType t);
	//Method that returns the current params that apply in the current
	//active visualizer if it is local.
	Params* getLocalParams(Params::ParamType);
    //method to launch a viz window, returns vizNum
	//Default values create a new vis use whatever available number.
	//If usenum is >= 0 it relaunches a previously used visualizer.
	//If usenum is < 0 and newNum is >=0, it creates a new visualizer
	//at position "newNum"
    int launchVisualizer(int usenum = -1, const char* name = "", int newNum = -1);
    
	void nameChanged(QString& name, int num);
    
    
    int getNumVisualizers(); 
    
    TabManager* getTabManager() { return tabManager;}
    
	//activeViz is -1 if none is active, otherwise is no. of active viz win.
	int getActiveViz() {return activeViz;}
	VizWin* getActiveVisualizer() {
		if(activeViz>=0)return vizWin[activeViz];
		else return 0;
	}
	//Method that is called when a window is made active:
	void setActiveViz(int vizNum); 
	//Obtain the parameters that currently apply in specified window:
	ViewpointParams* getViewpointParams(int winNum);
	RegionParams* getRegionParams(int winNum);
	//For a renderer, there will only exist a local version.
	// If instance is negative, return the current instance.
	DvrParams* getDvrParams(int winNum, int instance = -1);
	ParamsIso* getIsoParams(int winNum, int instance = -1);
	ProbeParams* getProbeParams(int winNum, int instance = -1);
	TwoDParams* getTwoDParams(int winNum, int instance = -1);

	Params* getParams(int winNum, Params::ParamType pType, int instance = -1);
	
	int getNumFlowInstances(int winnum){return flowParamsInstances[winnum].size();}
	int getNumProbeInstances(int winnum){return probeParamsInstances[winnum].size();}
	int getNumTwoDInstances(int winnum){return twoDParamsInstances[winnum].size();}
	int getNumDvrInstances(int winnum){return dvrParamsInstances[winnum].size();}
	int getNumIsoInstances(int winnum){return isoParamsInstances[winnum].size();}
	int getNumInstances(int winnum, Params::ParamType pType);
	int getCurrentFlowInstIndex(int winnum) {return currentFlowInstance[winnum];}
	int getCurrentDvrInstIndex(int winnum) {return currentDvrInstance[winnum];}
	int getCurrentIsoInstIndex(int winnum) {return currentIsoInstance[winnum];}
	int getCurrentProbeInstIndex(int winnum) {return currentProbeInstance[winnum];}
	int getCurrentTwoDInstIndex(int winnum) {return currentTwoDInstance[winnum];}
	void setCurrentFlowInstIndex(int winnum, int inst) {currentFlowInstance[winnum] = inst;}
	void setCurrentDvrInstIndex(int winnum, int inst) {currentDvrInstance[winnum] = inst;}
	void setCurrentIsoInstIndex(int winnum, int inst) {currentIsoInstance[winnum] = inst;}
	void setCurrentProbeInstIndex(int winnum, int inst) {currentProbeInstance[winnum] = inst;}
	void setCurrentTwoDInstIndex(int winnum, int inst) {currentTwoDInstance[winnum] = inst;}
	void setCurrentInstanceIndex(int winnum, int inst, Params::ParamType t);
	int getCurrentInstanceIndex(int winnum, Params::ParamType t);
	int findInstanceIndex(int winnum, Params* params, Params::ParamType t);
	int getActiveInstanceIndex(Params::ParamType t){
		return getCurrentInstanceIndex(activeViz,t);
	}

	void appendFlowInstance(int winnum, FlowParams* fp){
		flowParamsInstances[winnum].push_back(fp);
	}
	void appendDvrInstance(int winnum, DvrParams* dp){
		dvrParamsInstances[winnum].push_back(dp);
	}
	void appendIsoInstance(int winnum, ParamsIso* ip){
		isoParamsInstances[winnum].push_back(ip);
	}
	void appendProbeInstance(int winnum, ProbeParams* pp){
		probeParamsInstances[winnum].push_back(pp);
	}
	void appendTwoDInstance(int winnum, TwoDParams* pp){
		twoDParamsInstances[winnum].push_back(pp);
	}
	void appendInstance(int winnum, Params* p);
	void insertFlowInstance(int winnum, int posn, FlowParams* fp){
		flowParamsInstances[winnum].insert(flowParamsInstances[winnum].begin()+posn, fp);
	}
	void insertDvrInstance(int winnum, int posn, DvrParams* dp){
		dvrParamsInstances[winnum].insert(dvrParamsInstances[winnum].begin()+posn, dp);
	}
	void insertIsoInstance(int winnum, int posn, ParamsIso* dp){
		isoParamsInstances[winnum].insert(isoParamsInstances[winnum].begin()+posn, dp);
	}
	void insertProbeInstance(int winnum, int posn, ProbeParams* pp){
		probeParamsInstances[winnum].insert(probeParamsInstances[winnum].begin()+posn, pp);
	}
	void insertTwoDInstance(int winnum, int posn, TwoDParams* pp){
		twoDParamsInstances[winnum].insert(twoDParamsInstances[winnum].begin()+posn, pp);
	}
	void insertInstance(int winnum, int posn, Params* p);
	void removeFlowInstance(int winnum, int instance){
		FlowParams* fParams = flowParamsInstances[winnum].at(instance);
		if (currentFlowInstance[winnum] > instance)
			currentFlowInstance[winnum]--;
		flowParamsInstances[winnum].erase(flowParamsInstances[winnum].begin()+instance);
		//Did we remove last one?
		if (currentFlowInstance[winnum] >= (int)flowParamsInstances[winnum].size())
			currentFlowInstance[winnum]--;
		delete fParams;
	}
	void removeDvrInstance(int winnum, int instance){
		DvrParams* dParams = dvrParamsInstances[winnum].at(instance);
		if (currentDvrInstance[winnum] > instance)
			currentDvrInstance[winnum]--;
		dvrParamsInstances[winnum].erase(dvrParamsInstances[winnum].begin()+instance);
		//Did we remove last one?
		if (currentDvrInstance[winnum] >= (int)dvrParamsInstances[winnum].size())
			currentDvrInstance[winnum]--;
		delete dParams;
	}
	void removeIsoInstance(int winnum, int instance){
		ParamsIso* iParams = isoParamsInstances[winnum].at(instance);
		if (currentIsoInstance[winnum] > instance)
			currentIsoInstance[winnum]--;
		isoParamsInstances[winnum].erase(isoParamsInstances[winnum].begin()+instance);
		//Did we remove last one?
		if (currentIsoInstance[winnum] >= (int)isoParamsInstances[winnum].size())
			currentIsoInstance[winnum]--;
		delete iParams;
	}
	void removeProbeInstance(int winnum, int instance){
		ProbeParams* pParams = probeParamsInstances[winnum].at(instance);
		if (currentProbeInstance[winnum] > instance)
			currentProbeInstance[winnum]--;
		probeParamsInstances[winnum].erase(probeParamsInstances[winnum].begin()+instance);
		if (currentProbeInstance[winnum] >= (int)probeParamsInstances[winnum].size())
			currentProbeInstance[winnum]--;
		delete pParams;
	}
	
	void removeTwoDInstance(int winnum, int instance){
		TwoDParams* pParams = twoDParamsInstances[winnum].at(instance);
		if (currentTwoDInstance[winnum] > instance)
			currentTwoDInstance[winnum]--;
		twoDParamsInstances[winnum].erase(twoDParamsInstances[winnum].begin()+instance);
		if (currentTwoDInstance[winnum] >= (int)twoDParamsInstances[winnum].size())
			currentTwoDInstance[winnum]--;
		delete pParams;
	}
	
	void removeInstance(int winnum, int instance, Params::ParamType t);
	FlowParams* getFlowParams(int winNum, int instance = -1);
	AnimationParams* getAnimationParams(int winNum);

	RegionEventRouter* getRegionRouter() {return regionEventRouter;}
	AnimationEventRouter* getAnimationRouter() {return animationEventRouter;}
	DvrEventRouter* getDvrRouter() {return dvrEventRouter;}
	IsoEventRouter* getIsoRouter() {return isoEventRouter;}
	ProbeEventRouter* getProbeRouter() {return probeEventRouter;}
	TwoDEventRouter* getTwoDRouter() {return twoDEventRouter;}
	ViewpointEventRouter* getViewpointRouter() {return viewpointEventRouter;}
	FlowEventRouter* getFlowRouter() {return flowEventRouter;}

	//Get whatever params apply to a particular tab
	Params* getApplicableParams(Params::ParamType t);
	//Make all the params for the current window update their tabs:
	void updateActiveParams();
	//Establish connections to this from the viztab.
	//This class has the responsibility of rerouting messages from
	//the viz tab to the various viz parameter panels
	void hookUpViewpointTab(ViewpointEventRouter*);
	void hookUpRegionTab(RegionEventRouter*);
	void hookUpDvrTab(DvrEventRouter*);
	void hookUpIsoTab(IsoEventRouter*);
	void hookUpProbeTab(ProbeEventRouter*);
	void hookUpTwoDTab(TwoDEventRouter*);
	void hookUpAnimationTab(AnimationEventRouter*);
	void hookUpFlowTab(FlowEventRouter*);
	//set/get Data describing window states
	VizWin* getVizWin(int i) {return vizWin[i];}
	//Direct access to actual params object:
	ViewpointParams* getRealVPParams(int i) {return vpParams[i];}
	
	std::vector<FlowParams*>& getAllFlowParams(int i) {return flowParamsInstances[i];}
	std::vector<DvrParams*>& getAllDvrParams(int i) {return dvrParamsInstances[i];}
	std::vector<ParamsIso*>& getAllIsoParams(int i) {return isoParamsInstances[i];}
	std::vector<ProbeParams*>& getAllProbeParams(int i) {return probeParamsInstances[i];}
	std::vector<TwoDParams*>& getAllTwoDParams(int i) {return twoDParamsInstances[i];}
	

	RegionParams* getRealRegionParams(int i) {return rgParams[i];}
	AnimationParams* getRealAnimationParams(int i) {return animationParams[i];}

	void setVizWinName(int winNum, QString& qs);
	QString& getVizWinName(int winNum) {return vizName[winNum];}
	bool isMinimized(int winNum) {return isMin[winNum];}
	bool isMaximized(int winNum) {return isMax[winNum];}
	//Setting a params changes the previous params to the
	//specified one, performs needed ref/unref
	void setParams(int winNum, Params* p, Params::ParamType t, int instance = -1);
	
	void setViewpointParams(int winNum, ViewpointParams* p){setParams(winNum, (Params*)p, Params::ViewpointParamsType);}
	void setRegionParams(int winNum, RegionParams* p){setParams(winNum, (Params*)p, Params::RegionParamsType);}
	void setAnimationParams(int winNum, AnimationParams* p){setParams(winNum, (Params*)p, Params::AnimationParamsType);}
		
	void replaceGlobalParams(Params* p, Params::ParamType typ);
	void createDefaultParams(int winnum);
	
	void setSelectionMode( GLWindow::mouseModeType m);
	
	Trackball* getGlobalTrackball() {return globalTrackball;}
	//For a specific coordinate, and a specific window num, determine
	//whether or not the camera is situated further from the origin than
	//the current region center
	//
	bool cameraBeyondRegionCenter(int coord, int vizWinNum);

	//Following methods notify all params that are associated with a
	//specific window.
	//Set the dirty bits in all the visualizers that use a
	//region parameter setting
	//
	
	void setRegionDirty(RegionParams* p){setVizDirty(p, RegionBit, true);}
	
	//AnimationParams, force a render.  Probe & Volume renderer need to rebuild
	//
	void setAnimationDirty(AnimationParams* ap){setVizDirty(ap, AnimationBit, true);}
	//Force rerender of window using a flowParams, or region params..
	//
	void refreshFlow(FlowParams*);
	void refreshRegion(RegionParams* rParams);
	void refreshProbe(ProbeParams* pParams);
	void refreshTwoD(TwoDParams* pParams);
	
	//Force dvr renderer to get latest CLUT
	//
	void setClutDirty(RenderParams* p);
	
	//Force dvr renderers to get latest DataRange
	//
	void setDatarangeDirty(RenderParams* p); 
	void setFlowGraphicsDirty(FlowParams* p);
	void setFlowDataDirty(FlowParams* p, bool doInterrupt = true);
	
	bool flowDataIsDirty(FlowParams* p);
	//Tell the animationController that the frame counter has changed 
	//for all the associated windows.
	//
	
	void animationParamsChanged(AnimationParams* );
	//Reset the near/far distances for all the windows that
	//share a viewpoint, based on region in specified regionparams
	//
	void resetViews(RegionParams* rp, ViewpointParams* vp);
	//Set the viewer coords changed flag in all the visualizers
	//that use these vp params:
	void setViewerCoordsChanged(ViewpointParams* vp);
	//Change to play state for the specified renderers:
	//
	void startPlay(AnimationParams* aParams);
	
	//Tell all parameter panels to reinitialize (based on change of 
	//Metadata).  If the parameter is true, we can override the panels' 
	//previous state
	//
	void reinitializeParams(bool doOverride);
	//reset to starting state
	//
	void restartParams();

	//Make each window use its viewpoint params
	void initViews();
	
	//Methods to handle save/restore
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//this tag needs to be visible in session class
	static const string _visualizersTag;

	//General function for all dirty bit setting:
	void setVizDirty(Params* p, DirtyBitType bittype, bool bit = true, bool refresh = true);
	Params* getCorrespondingGlobalParams(Params* p) {
		return getGlobalParams(p->getParamType());
	}
	Params* getCorrespondingLocalParams(Params* p) {
		return getLocalParams(p->getParamType());
	}

public slots:
	//arrange the viz windows:
    void cascade();
    void coverRight();
    void fitSpace();
	//Slots to handle home viewpoint (dispatch to proper panel)
	void home();
	void sethome();
	//Slot that responds when user requests to activate a window:
	void winActivated(int);
	void killViz(int viznum);
	//Slots that set viewpoint:
	void viewAll();
	void viewRegion();
	void alignView(int axis);
	
signals:
	//Turn on/off multiple viz options:

	void enableMultiViz(bool onOff);
	//Respond to user setting the vizselectorcombo:
	void newViz(QString&, int);
	void removeViz(int);
	void activateViz(int);
	void changeName(QString&, int);
	
protected:
	static const string _vizTimeAnnotColorAttr;
	static const string _vizTimeAnnotTextSizeAttr;
	static const string _vizTimeAnnotTypeAttr;
	static const string _vizTimeAnnotCoordsAttr;
	static const string _vizElevGridEnabledAttr;
	static const string _vizElevGridColorAttr;
	static const string _vizElevGridRefinementAttr;
	static const string _vizWinTag;
	static const string _vizWinNameAttr;
	static const string _vizBgColorAttr;
	static const string _vizRegionColorAttr;
	static const string _vizSubregionColorAttr;
	static const string _vizColorbarBackgroundColorAttr;
	static const string _vizAxisPositionAttr;
	static const string _vizAxisOriginAttr;
	static const string _vizAxisAnnotationEnabledAttr;
	static const string _vizMinTicAttr;
	static const string _vizMaxTicAttr;
	static const string _vizTicLengthAttr;
	static const string _vizTicDirAttr;
	static const string _vizNumTicsAttr;
	static const string _vizAxisColorAttr;
	static const string _vizTicWidthAttr;
	static const string _vizLabelHeightAttr;
	static const string _vizLabelDigitsAttr;
	static const string _vizColorbarLLPositionAttr;
	static const string _vizColorbarURPositionAttr;
	static const string _vizColorbarNumTicsAttr;
	static const string _vizAxisArrowsEnabledAttr;
	static const string _vizColorbarEnabledAttr;
	static const string _vizRegionFrameEnabledAttr;
	static const string _vizSubregionFrameEnabledAttr;
	static const string _visualizerNumAttr;
	static const string _vizElevGridDisplacementAttr;
	static const string _vizElevGridInvertedAttr;
	static const string _vizElevGridRotationAttr;
	static const string _vizElevGridTexturedAttr;
	static const string _vizElevGridTextureNameAttr;
	static VizWinMgr* theVizWinMgr;
	VizWinMgr ();
	
    VizWin* vizWin[MAXVIZWINS];
   
    QString vizName[MAXVIZWINS];
    bool isMax[MAXVIZWINS];
    bool isMin[MAXVIZWINS];
	ViewpointParams* vpParams[MAXVIZWINS];
	RegionParams* rgParams[MAXVIZWINS];
	//For renderparams, there is an array of vectors of params, plus an integer
	//pointing to the current instance:
	
	vector<DvrParams*> dvrParamsInstances[MAXVIZWINS];
	int currentDvrInstance[MAXVIZWINS];
	vector<ParamsIso*> isoParamsInstances[MAXVIZWINS];
	int currentIsoInstance[MAXVIZWINS];
	vector<ProbeParams*> probeParamsInstances[MAXVIZWINS];
	int currentProbeInstance[MAXVIZWINS];
	vector<TwoDParams*> twoDParamsInstances[MAXVIZWINS];
	int currentTwoDInstance[MAXVIZWINS];
	vector<FlowParams*> flowParamsInstances[MAXVIZWINS];
	int currentFlowInstance[MAXVIZWINS];
	

	AnimationParams* animationParams[MAXVIZWINS];
	Params** getParamsArray(Params::ParamType t);
	//Remember the activation order:
	int activationOrder[MAXVIZWINS];
	int activationCount;
	int getLastActive(){
		int mx = -1; int mj = -1;
		for (int j = 0; j< MAXVIZWINS; j++){
			if (vizWin[j] && activationOrder[j]>mx){ 
				mx = activationOrder[j];
				mj = j;
			}
		}
		return mj;
	}
	Trackball* globalTrackball;
	ViewpointParams* globalVPParams;
	RegionParams* globalRegionParams;
	//Default, not same as global...
	FlowParams* defaultFlowParams;
	ProbeParams* defaultProbeParams;
	TwoDParams* defaultTwoDParams;
	DvrParams* defaultDvrParams;
	ParamsIso* defaultIsoParams;
	
	AnimationParams* globalAnimationParams;

	RegionEventRouter* regionEventRouter;
	DvrEventRouter* dvrEventRouter;
	IsoEventRouter* isoEventRouter;
	ProbeEventRouter* probeEventRouter;
	TwoDEventRouter* twoDEventRouter;
	ViewpointEventRouter* viewpointEventRouter;
	AnimationEventRouter* animationEventRouter;
	FlowEventRouter* flowEventRouter;
	
    MainForm* myMainWindow;
    TabManager* tabManager;
   
    int activeViz;
	int parsingVizNum, parsingDvrInstance, parsingIsoInstance,parsingFlowInstance, parsingProbeInstance,parsingTwoDInstance;
	
    QWorkspace* myWorkspace;
	
	
	
	FlowTab* myFlowTab;

    int     benchmark;
    QTimer *benchmarkTimer;

protected slots:
	

	//The vizWinMgr has to dispatch signals from gui's to the appropriate parameter panels
	//First, the viztab slots:
	
	void setVpLocalGlobal(int val);
	void setRgLocalGlobal(int val);
	void setAnimationLocalGlobal(int val);
};
};
#endif // VIZWINMGR_H

