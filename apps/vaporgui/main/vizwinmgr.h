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



class QMdiArea;
class QMdiSubWindow;
class QWidget;
class QDesktopWidget;
class QMainWidget;
class QTimer;

#include <qobject.h>
#include "twoDdataparams.h"
#include "twoDimageparams.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "probeparams.h"

#include "flowparams.h"
#include "dvrparams.h"
#include "animationparams.h"
#include <vapor/common.h>
#include "params.h"
#include "command.h"





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
class TwoDDataParams;
class TwoDImageParams;
class Trackball;
class VizWin;
class AnimationEventRouter;
class RegionEventRouter;
class DvrEventRouter;
class ProbeEventRouter;
class TwoDDataEventRouter;
class TwoDImageEventRouter;
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
		return((DvrParams*)Params::GetParamsInstance(Params::_dvrParamsTag,getInstance()->activeViz,-1));
	}
	
	static ProbeParams* getActiveProbeParams(){
		return ((ProbeParams*)Params::GetParamsInstance(Params::_probeParamsTag,getInstance()->activeViz,-1));}
	static TwoDDataParams* getActiveTwoDDataParams(){
		return ((TwoDDataParams*)Params::GetParamsInstance(Params::_twoDDataParamsTag,getInstance()->activeViz,-1));}
	static TwoDImageParams* getActiveTwoDImageParams(){
		return ((TwoDImageParams*)Params::GetParamsInstance(Params::_twoDImageParamsTag,getInstance()->activeViz,-1));}
	static RegionParams* getActiveRegionParams(){
		return (getInstance()->getRegionParams(getInstance()->activeViz));
	}
	static FlowParams* getActiveFlowParams(){
		return ((FlowParams*)Params::GetParamsInstance(Params::_flowParamsTag,getInstance()->activeViz,-1));}
	static ViewpointParams* getActiveVPParams(){
		return (getInstance()->getViewpointParams(getInstance()->activeViz));
	}
	static AnimationParams* getActiveAnimationParams(){
		return (getInstance()->getAnimationParams(getInstance()->activeViz));
	}

	//Each event router registers itself in its constructor with the vizwinmgr by calling the following
	static Params::ParamsBaseType RegisterEventRouter(const std::string tag, EventRouter* router);
	
	void createAllDefaultParams();
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
	
	static EventRouter* getEventRouter(Params::ParamsBaseType typeId);
	static EventRouter* getEventRouter(const std::string& tag){
		return getEventRouter(ParamsBase::GetTypeFromTag(tag));
	}

	Params* getGlobalParams(Params::ParamsBaseType ptype);
	void setGlobalParams(Params* p, ParamsBase::ParamsBaseType t);
	//Method that returns the current params that apply in the current
	//active visualizer if it is local.
	Params* getLocalParams(Params::ParamsBaseType t);
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
	
	Params* getIsoParams(int winnum, int instance = -1){
		return Params::GetParamsInstance(Params::GetTypeFromTag(Params::_isoParamsTag),winnum, instance);
	}
	Params* getDvrParams(int winnum, int instance = -1){
		return Params::GetParamsInstance(Params::GetTypeFromTag(Params::_dvrParamsTag),winnum, instance);
	}
	ProbeParams* getProbeParams(int winNum, int instance = -1);
	TwoDDataParams* getTwoDDataParams(int winNum, int instance = -1);
	TwoDImageParams* getTwoDImageParams(int winNum, int instance = -1);

	Params* getParams(int winNum, Params::ParamsBaseType pType, int instance = -1);
	
	int getNumFlowInstances(int winnum){return Params::GetNumParamsInstances(Params::_flowParamsTag,winnum);}
	int getNumProbeInstances(int winnum){return Params::GetNumParamsInstances(Params::_probeParamsTag,winnum);}
	int getNumTwoDDataInstances(int winnum){return Params::GetNumParamsInstances(Params::_twoDDataParamsTag,winnum);}
	int getNumTwoDImageInstances(int winnum){return Params::GetNumParamsInstances(Params::_twoDImageParamsTag,winnum);}
	int getNumDvrInstances(int winnum){
		return Params::GetNumParamsInstances(Params::_dvrParamsTag, winnum);
	}
	int getNumIsoInstances(int winnum){
		return Params::GetNumParamsInstances(Params::_isoParamsTag, winnum);
	}
	int getNumInstances(int winnum, Params::ParamsBaseType pType);
	
	int getCurrentFlowInstIndex(int winnum) {return Params::GetCurrentParamsInstanceIndex(Params::_flowParamsTag,winnum);}
	int getCurrentDvrInstIndex(int winnum) {
		return Params::GetCurrentParamsInstanceIndex(Params::_dvrParamsTag,winnum);
	}
	int getCurrentIsoInstIndex(int winnum) {
		return Params::GetCurrentParamsInstanceIndex(Params::GetTypeFromTag(Params::_isoParamsTag),winnum);
	}
	int getCurrentProbeInstIndex(int winnum) {return Params::GetCurrentParamsInstanceIndex(Params::_probeParamsTag,winnum);}
	int getCurrentTwoDDataInstIndex(int winnum) {return Params::GetCurrentParamsInstanceIndex(Params::_twoDDataParamsTag,winnum);}
	int getCurrentTwoDImageInstIndex(int winnum) {return Params::GetCurrentParamsInstanceIndex(Params::_twoDImageParamsTag,winnum);}
	
	void setCurrentInstanceIndex(int winnum, int inst, Params::ParamsBaseType t);
	int getCurrentInstanceIndex(int winnum, Params::ParamsBaseType t);
	int findInstanceIndex(int winnum, Params* params, Params::ParamsBaseType t);
	int getActiveInstanceIndex(Params::ParamsBaseType t){
		return getCurrentInstanceIndex(activeViz,t);
	}

	
	void appendInstance(int winnum, Params* p);
	
	void insertInstance(int winnum, int posn, Params* p);
	
	
	void removeInstance(int winnum, int instance, Params::ParamsBaseType t);
	FlowParams* getFlowParams(int winNum, int instance = -1);
	AnimationParams* getAnimationParams(int winNum);

	RegionEventRouter* getRegionRouter() {return regionEventRouter;}
	AnimationEventRouter* getAnimationRouter() {return animationEventRouter;}
	DvrEventRouter* getDvrRouter() {return dvrEventRouter;}
	IsoEventRouter* getIsoRouter() {return isoEventRouter;}
	ProbeEventRouter* getProbeRouter() {return probeEventRouter;}
	TwoDDataEventRouter* getTwoDDataRouter() {return twoDDataEventRouter;}
	TwoDImageEventRouter* getTwoDImageRouter() {return twoDImageEventRouter;}
	ViewpointEventRouter* getViewpointRouter() {return viewpointEventRouter;}
	FlowEventRouter* getFlowRouter() {return flowEventRouter;}

	//Get whatever params apply to a particular tab
	Params* getApplicableParams(Params::ParamsBaseType t);
	Params* getApplicableParams(const std::string& tag){
		return getApplicableParams(ParamsBase::GetTypeFromTag(tag));
	}
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
	void hookUpTwoDDataTab(TwoDDataEventRouter*);
	void hookUpTwoDImageTab(TwoDImageEventRouter*);
	void hookUpAnimationTab(AnimationEventRouter*);
	void hookUpFlowTab(FlowEventRouter*);
	//set/get Data describing window states
	VizWin* getVizWin(int i) {return vizWin[i];}
	//Direct access to actual params object:
	ViewpointParams* getRealVPParams(int win) {
		Params* p = Params::GetParamsInstance(Params::_viewpointParamsTag,win,-1);
		if (p->isLocal()) return (ViewpointParams*)p;
		return (ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag);
	}
	RegionParams* getRealRegionParams(int win) {
		Params* p = Params::GetParamsInstance(Params::_regionParamsTag,win,-1);
		if (p->isLocal()) return (RegionParams*)p;
		return (RegionParams*)Params::GetDefaultParams(Params::_regionParamsTag);
	}
	AnimationParams* getRealAnimationParams(int win) {
		Params* p = Params::GetParamsInstance(Params::_animationParamsTag,win,-1);
		if (p->isLocal()) return (AnimationParams*)p;
		return (AnimationParams*)Params::GetDefaultParams(Params::_animationParamsTag);
	}
	ViewpointParams* getGlobalVPParams(){return (ViewpointParams*)(Params::GetDefaultParams(Params::_viewpointParamsTag));}
	RegionParams* getGlobalRegionParams(){
		return (RegionParams*)(Params::GetDefaultParams(Params::_regionParamsTag));
	}
	AnimationParams* getGlobalAnimationParams(){return (AnimationParams*)(Params::GetDefaultParams(Params::_animationParamsTag));}

	
	void setVizWinName(int winNum, QString& qs);
	QString& getVizWinName(int winNum) {return vizName[winNum];}
	bool isMinimized(int winNum) {return isMin[winNum];}
	bool isMaximized(int winNum) {return isMax[winNum];}
	//Setting a params changes the previous params to the
	//specified one, performs needed ref/unref
	void setParams(int winNum, Params* p, ParamsBase::ParamsBaseType typeId, int instance = -1);
	
	void setViewpointParams(int winNum, ViewpointParams* p){setParams(winNum, (Params*)p, Params::GetTypeFromTag(Params::_viewpointParamsTag));}
	void setRegionParams(int winNum, RegionParams* p){setParams(winNum, (Params*)p, Params::GetTypeFromTag(Params::_regionParamsTag));}
	void setAnimationParams(int winNum, AnimationParams* p){setParams(winNum, (Params*)p, Params::GetTypeFromTag(Params::_animationParamsTag));}
		
	void replaceGlobalParams(Params* p, ParamsBase::ParamsBaseType t);
	void createDefaultParams(int winnum);
	
	void setSelectionMode( GLWindow::mouseModeType m);
	
	Trackball* getGlobalTrackball() {return globalTrackball;}
	//For a specific coordinate, and a specific window num, determine
	//whether or not the camera is situated further from the origin than
	//the current region center
	//
	bool cameraBeyondRegionCenter(int coord, int vizWinNum);

	//Force all renderers to re-obtain render data
	void refreshRenderData();
	//
	//Disable all renderers that use specified variables
	void disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D);
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
	void refreshViewpoint(ViewpointParams* vParams);
	void refreshRegion(RegionParams* rParams);
	void refreshProbe(ProbeParams* pParams);
	void refreshTwoDData(TwoDDataParams* pParams);
	void refreshTwoDImage(TwoDImageParams* pParams);
	
	//Force dvr renderer to get latest CLUT
	//
	void setClutDirty(RenderParams* p);
	
	//Force dvr renderers to get latest DataRange
	//
	void setDatarangeDirty(RenderParams* p); 
	void setFlowGraphicsDirty(FlowParams* p);
	void setFlowDataDirty(FlowParams* p, bool doInterrupt = true);
	void setFlowDisplayListDirty(FlowParams* p);
	
	bool flowDataIsDirty(FlowParams* p);

	//Force reconstructing of all elevation grids
	void setAllTwoDElevDirty();
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
	//Reinitialize params and tabs for 
	//change in active variables
	//
	void reinitializeVariables();
	//reset to starting state
	//
	void restartParams();

	//Make each window use its viewpoint params
	void initViews();
	
	//Methods to handle save/restore
	ParamNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//this tag needs to be visible in session class
	static const string _visualizersTag;

	//General function for all dirty bit setting:
	void setVizDirty(Params* p, DirtyBitType bittype, bool bit = true, bool refresh = true);


	Params* getCorrespondingGlobalParams(Params* p) {
		return getGlobalParams(p->GetParamsBaseTypeId());
	}
	Params* getCorrespondingLocalParams(Params* p) {
		return getLocalParams(p->GetParamsBaseTypeId());
	}
	void setInteractiveNavigating(int level);
	bool findCoincident2DSurface(int viznum, int orientation, float coordinate, bool terrainMapped);
	void stopFlowIntegration();

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
	QMdiSubWindow* vizMdiWin[MAXVIZWINS];
   
    QString vizName[MAXVIZWINS];
    bool isMax[MAXVIZWINS];
    bool isMin[MAXVIZWINS];
	
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

	RegionEventRouter* regionEventRouter;
	DvrEventRouter* dvrEventRouter;
	IsoEventRouter* isoEventRouter;
	ProbeEventRouter* probeEventRouter;
	TwoDDataEventRouter* twoDDataEventRouter;
	TwoDImageEventRouter* twoDImageEventRouter;
	ViewpointEventRouter* viewpointEventRouter;
	AnimationEventRouter* animationEventRouter;
	FlowEventRouter* flowEventRouter;
	static std::map<ParamsBase::ParamsBaseType, EventRouter*> eventRouterMap;
    MainForm* myMainWindow;
    TabManager* tabManager;
   
    int activeViz;
	vector<int> parsingInstance;
	int parsingVizNum, parsingDvrInstance, parsingIsoInstance,parsingFlowInstance, parsingProbeInstance,parsingTwoDDataInstance,parsingTwoDImageInstance;
	
    QMdiArea* myMDIArea;
	
	
	
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

