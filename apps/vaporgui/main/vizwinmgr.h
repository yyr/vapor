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
#include "viewpointparams.h"
#include "regionparams.h"
#include "probeparams.h"
#include "flowparams.h"
#include "animationparams.h"
#include "vaporinternal/common.h"
#include "params.h"
#include "command.h"


class RegionTab;
class Dvr;
class ProbeTab;
class VizTab;
class FlowTab;
class AnimationTab;
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
class Trackball;
class VizWin;
class AnimationEventRouter;
class RegionEventRouter;
class DvrEventRouter;
class ProbeEventRouter;
class ViewpointEventRouter;
class FlowEventRouter;


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
	static DvrParams* getActiveDvrParams(){
		return (getInstance()->getDvrParams(getInstance()->activeViz));}
	static ProbeParams* getActiveProbeParams(){
		return (getInstance()->getProbeParams(getInstance()->activeViz));}
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
	

	//For a renderer, there should always exist a local version.
	DvrParams* getDvrParams(int winNum);
	ProbeParams* getProbeParams(int winNum);
	
	FlowParams* getFlowParams(int winNum);
	AnimationParams* getAnimationParams(int winNum);

	RegionEventRouter* getRegionRouter() {return regionEventRouter;}
	AnimationEventRouter* getAnimationRouter() {return animationEventRouter;}
	DvrEventRouter* getDvrRouter() {return dvrEventRouter;}
	ProbeEventRouter* getProbeRouter() {return probeEventRouter;}
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
	void hookUpProbeTab(ProbeEventRouter*);

	void hookUpAnimationTab(AnimationEventRouter*);
	void hookUpFlowTab(FlowEventRouter*);
	//set/get Data describing window states
	VizWin* getVizWin(int i) {return vizWin[i];}
	//Direct access to actual params object:
	ViewpointParams* getRealVPParams(int i) {return vpParams[i];}
	
	FlowParams* getRealFlowParams(int i) {return flowParams[i];}
	DvrParams* getRealDvrParams(int i) {return dvrParams[i];}
	ProbeParams* getRealProbeParams(int i) {return probeParams[i];}
	RegionParams* getRealRegionParams(int i) {return rgParams[i];}
	AnimationParams* getRealAnimationParams(int i) {return animationParams[i];}

	void setVizWinName(int winNum, QString& qs);
	QString& getVizWinName(int winNum) {return vizName[winNum];}
	bool isMinimized(int winNum) {return isMin[winNum];}
	bool isMaximized(int winNum) {return isMax[winNum];}
	//Setting a params changes the previous params to the
	//specified one, performs needed ref/unref
	
	void setDvrParams(int winNum, DvrParams* p);
	void setProbeParams(int winNum, ProbeParams* p);
	
	void setFlowParams(int winNum, FlowParams* p);
	void setViewpointParams(int winNum, ViewpointParams* p);
	void setRegionParams(int winNum, RegionParams* p);
	void setAnimationParams(int winNum, AnimationParams* p);
		
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
	
	//Force all the renderers that share these DvrParams
	//to set their regions dirty
	//
	void setRegionDirty(DvrParams* dp){
		setVizDirty((Params*)dp, RegionBit, true);
		setVizDirty((Params*)dp, FlowDataBit, true);
	}
	//Similarly for AnimationParams:
	//
	void setAnimationDirty(AnimationParams* ap){setVizDirty(ap, RegionBit, true);}
	//Force rerender of all windows that share a flowParams, or region params..
	//
	void refreshFlow(FlowParams*);
	void refreshRegion(RegionParams* rParams);
	void refreshProbe(ProbeParams* pParams);
	
	//Force dvr renderer to get latest CLUT
	//
	void setClutDirty(DvrParams* p){setVizDirty((Params*)p, DvrClutBit, true);}
	//Force probe renderer to get latest CLUT
	//
	void setClutDirty(ProbeParams* p){setVizDirty(p, ProbeTextureBit, true);}
	//Force dvr renderers to get latest DataRange
	//
	void setDataRangeDirty(DvrParams* p) {setVizDirty((Params*)p, DvrDatarangeBit, true);}

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
	
	//Methods to handle save/restore
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//this tag needs to be visible in session class
	static const string _visualizersTag;

	//General function for all dirty bit setting:
	void setVizDirty(Params* p, DirtyBitType bittype, bool bit, bool refresh = true);
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
	static const string _vizWinTag;
	static const string _vizWinNameAttr;
	static const string _vizBgColorAttr;
	static const string _vizRegionColorAttr;
	static const string _vizSubregionColorAttr;
	static const string _vizColorbarBackgroundColorAttr;
	static const string _vizAxisPositionAttr;
	static const string _vizColorbarLLPositionAttr;
	static const string _vizColorbarURPositionAttr;
	static const string _vizColorbarNumTicsAttr;
	static const string _vizAxesEnabledAttr;
	static const string _vizColorbarEnabledAttr;
	static const string _vizRegionFrameEnabledAttr;
	static const string _vizSubregionFrameEnabledAttr;
	static const string _visualizerNumAttr;
	static VizWinMgr* theVizWinMgr;
	VizWinMgr ();
	
    VizWin* vizWin[MAXVIZWINS];
   
    QString vizName[MAXVIZWINS];
    bool isMax[MAXVIZWINS];
    bool isMin[MAXVIZWINS];
	ViewpointParams* vpParams[MAXVIZWINS];
	RegionParams* rgParams[MAXVIZWINS];
	DvrParams* dvrParams[MAXVIZWINS];
	ProbeParams* probeParams[MAXVIZWINS];
	
	FlowParams* flowParams[MAXVIZWINS];
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
	
	FlowParams* globalFlowParams;
	ProbeParams* globalProbeParams;
	DvrParams* globalDvrParams;
	
	AnimationParams* globalAnimationParams;

	RegionEventRouter* regionEventRouter;
	DvrEventRouter* dvrEventRouter;
	ProbeEventRouter* probeEventRouter;
	ViewpointEventRouter* viewpointEventRouter;
	AnimationEventRouter* animationEventRouter;
	FlowEventRouter* flowEventRouter;
	
    MainForm* myMainWindow;
    TabManager* tabManager;
   
    int activeViz;
	int parsingVizNum;
	
    QWorkspace* myWorkspace;
	
	
	
	FlowTab* myFlowTab;

    int     benchmark;
    QTimer *benchmarkTimer;

protected slots:
	

	//The vizWinMgr has to dispatch signals from gui's to the appropriate parameter panels
	//First, the viztab slots:
	
	void setVpLocalGlobal(int val);
	void setRgLocalGlobal(int val);
	void setDvrLocalGlobal(int val);
	void setProbeLocalGlobal(int val);
	
	void setAnimationLocalGlobal(int val);
	void setFlowLocalGlobal(int val);
};
};
#endif // VIZWINMGR_H

