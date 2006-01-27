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


#define MAXVIZWINS 16

class QWorkspace;
class QWidget;
class QDesktopWidget;
class QMainWidget;


#include <qobject.h>
#include "viewpointparams.h"
#include "regionparams.h"
#include "probeparams.h"
#include "contourparams.h"
#include "isosurfaceparams.h"
#include "flowparams.h"
#include "animationparams.h"
#include "vaporinternal/common.h"
#include "params.h"
#include "command.h"

class RegionTab;
class Dvr;
class ProbeTab;
class ContourPlaneTab;
class VizTab;
class IsoTab;
class FlowTab;
class AnimationTab;
class QPopupMenu;



namespace VAPoR{
class AnimationController;
class AnimationParams;
class IsosurfaceParams;

class TabManager;
class MainForm;
class ViewpointParams;
class RegionParams;
class DvrParams;
class ProbeParams;
class ContourParams;
class Trackball;
class VizWin;

class VizWinMgr : public QObject, public ParsedXml
{
	Q_OBJECT
public:
	static VizWinMgr* getInstance() {
		if (!theVizWinMgr)
			theVizWinMgr = new VizWinMgr();
		return theVizWinMgr;
	}
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
	ContourParams* getContourParams(int winNum);
	IsosurfaceParams* getIsoParams(int winNum);
	FlowParams* getFlowParams(int winNum);
	AnimationParams* getAnimationParams(int winNum);
	//Get whatever params apply to a particular tab
	Params* getApplicableParams(Params::ParamType t);
	//Make all the params for the current window update their tabs:
	void updateActiveParams();
	//Establish connections to this from the viztab.
	//This class has the responsibility of rerouting messages from
	//the viz tab to the various viz parameter panels
	void hookUpVizTab(VizTab*);
	void hookUpRegionTab(RegionTab*);
	void hookUpDvrTab(Dvr*);
	void hookUpProbeTab(ProbeTab*);
	void hookUpIsoTab(IsoTab*);
	void hookUpContourTab(ContourPlaneTab*);
	void hookUpAnimationTab(AnimationTab*);
	void hookUpFlowTab(FlowTab*);
	//set/get Data describing window states
	VizWin* getVizWin(int i) {return vizWin[i];}
	//Direct access to actual params object:
	ViewpointParams* getRealVPParams(int i) {return vpParams[i];}
	ContourParams* getRealContourParams(int i) {return contourParams[i];}
	IsosurfaceParams* getRealIsoParams(int i) {return isoParams[i];}
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
	void setContourParams(int winNum, ContourParams* p);
	void setDvrParams(int winNum, DvrParams* p);
	void setProbeParams(int winNum, ProbeParams* p);
	void setIsoParams(int winNum, IsosurfaceParams* p);
	void setFlowParams(int winNum, FlowParams* p);
	void setViewpointParams(int winNum, ViewpointParams* p);
	void setRegionParams(int winNum, RegionParams* p);
	void setAnimationParams(int winNum, AnimationParams* p);
		
	void replaceGlobalParams(Params* p, Params::ParamType typ);
	void createDefaultParams(int winnum);
	
	void setSelectionMode( Command::mouseModeType m);
	Command::mouseModeType getSelectionMode () {return selectionMode;}
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
	
	void setRegionDirty(RegionParams*);
	
	//Force all the renderers that share these DvrParams
	//to set their regions dirty
	//
	void setRegionDirty(DvrParams*);
	//Similarly for AnimationParams:
	//
	void setAnimationDirty(AnimationParams*);
	//Force rerender of all windows that share a flowParams, or region params..
	//
	void refreshFlow(FlowParams*);
	void refreshRegion(RegionParams* rParams);
	void refreshProbe(ProbeParams* pParams);
	
	//Force dvr renderer to get latest CLUT
	//
	void setClutDirty(DvrParams* );
	//Force dvr renderers to get latest DataRange
	//
	void setDataRangeDirty(DvrParams* );
	
	//Tell the animationController that the frame counter has changed 
	//for all the associated windows.
	//
	void animationParamsChanged(AnimationParams* );
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
	Command::mouseModeType selectionMode;

	//Methods to handle save/restore
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	//this tag needs to be visible in session class
	static const string _visualizersTag;


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
	ContourParams* contourParams[MAXVIZWINS];
	IsosurfaceParams* isoParams[MAXVIZWINS];
	FlowParams* flowParams[MAXVIZWINS];
	AnimationParams* animationParams[MAXVIZWINS];

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
	IsosurfaceParams* globalIsoParams;
	FlowParams* globalFlowParams;
	ProbeParams* globalProbeParams;
	DvrParams* globalDvrParams;
	ContourParams* globalContourParams;
	AnimationParams* globalAnimationParams;
	
    MainForm* myMainWindow;
    TabManager* tabManager;
   
    int activeViz;
	int parsingVizNum;
	
    QWorkspace* myWorkspace;
	ContourPlaneTab* myContourPlaneTab;
	Dvr* myDvrTab;
	ProbeTab* myProbeTab;
	FlowTab* myFlowTab;


protected slots:
	

	//The vizWinMgr has to dispatch signals from gui's to the appropriate parameter panels
	//First, the viztab slots:
	//void setLights(const QString& qs);
	void setVpLocalGlobal(int val);
	void setRgLocalGlobal(int val);
	void setDvrLocalGlobal(int val);
	void setProbeLocalGlobal(int val);
	void setIsoLocalGlobal(int val);
	void setContourLocalGlobal(int val);
	void setAnimationLocalGlobal(int val);
	void setFlowLocalGlobal(int val);

	void setVPPerspective(int isOn);

	void setVtabTextChanged(const QString& qs);
	void setRegionTabTextChanged(const QString& qs);
	void setIsoTabTextChanged(const QString& qs);
	//three kinds of text changed for flow tab
	void setFlowTabFlowTextChanged(const QString&);
	void setFlowTabGraphicsTextChanged(const QString&);
	void setFlowTabRangeTextChanged(const QString&);
	
	void setDvrTabTextChanged(const QString& qs);
	void setProbeTabTextChanged(const QString& qs);
	void setContourTabTextChanged(const QString& qs);
	void setAtabTextChanged(const QString& qs);
	
	void viewpointReturnPressed();
	void regionReturnPressed();
	void dvrReturnPressed();
	void probeReturnPressed();
	void isoReturnPressed();
	void contourReturnPressed();
	void animationReturnPressed();
	void flowTabReturnPressed();

	//Animation slots:
	void animationSetFrameStep();
	void animationSetPosition();
	void animationPauseClick();
	void animationPlayReverseClick();
	void animationPlayForwardClick();
	void animationReplayClick();
	void animationToBeginClick();
	void animationToEndClick();
	void animationStepForwardClick();
	void animationStepReverseClick();


	//Then the regionTab slots:
	
	void setRegionMaxSize();
	//Sliders set these:
	void setRegionXCenter();
	void setRegionYCenter();
	void setRegionZCenter();
	void setRegionXSize();
	void setRegionYSize();
	void setRegionZSize();
	void setRegionRefinement(int);
	void setRegionVarNum(int);
	void setRegionTimestep(int);
	void copyRegionToRake();
	void copyRakeToRegion();
	void copyRegionToProbe();
	void copyProbeToRegion();


	//Slots for dvr panel:
	void setDvrType(int type);
	void setDvrNumRefinements(int num);
	void setDvrEnabled(int on);
	void setDvrVariableNum(int);
	void setDvrLighting(bool);
	void setDvrNumBits(int);
	void setDvrEditMode(bool);
	void setDvrNavigateMode(bool);
	void setDvrAligned();
	void dvrOpacityScale();
	void dvrColorBind();
	void dvrOpacBind();
	void dvrLoadTF();
	void dvrSaveTF();
	void refreshHisto();
	
	//Slots for Probe panel:
	void setProbeEnabled(int on);

	void setProbeEditMode(bool);
	void setProbeNavigateMode(bool);
	void setProbeAligned();
	void probeOpacityScale();
	
	void probeColorBind();
	void probeOpacBind();
	void probeLoadTF();
	void probeSaveTF();
	void refreshProbeHisto();
	
	//Slots for contour panel:
	
	void setContourEnabled(int on);
	void setContourVariableNum(int);
	void setContourLighting(bool);
	
	//slots for iso panel:
	void setIsoEnabled(int on);
	void setIsoVariable1Num(int);
	void setIsoValue1();
	void setIsoOpac1();
	void setIsoColor1();

	//slots for flow tab:
	void rebuildFlow();
	void setFlowEnabled(int);
	
	void setRakeOnRegion();
	void setFlowType(int);
	void setFlowNumRefinements(int);
	void setFlowXVar(int);
	void setFlowYVar(int);
	void setFlowZVar(int);
	void checkFlowRandom(bool isRandom);
	void flowAutoToggled(bool isOn);
	void setFlowXCenter();
	void setFlowYCenter();
	void setFlowZCenter();
	void setFlowXSize();
	void setFlowYSize();
	void setFlowZSize();
	void setFlowGeomSamples();
	void setFlowConstantColor();
	void setFlowGeometry(int);
	void setFlowColorMapEntity(int);
	void setFlowOpacMapEntity(int);
	void setFlowEditMode(bool);
	void setFlowNavigateMode(bool);
	void setFlowAligned();
	void doFlowRake(bool);
	void doFlowSeedList(bool);
	void loadFlowSeedList();
	void saveFlow();
	void editFlowSeedList();
	void flowOpacityScale();

	//Probe slots:
	
	void probeCenterRegion();
	void probeCenterView();
	void probeCenterRake();
	void probeCenterProbe();
	void probeAddSeed();
	void probeAttachSeed(bool attach);
	void setProbeNumRefinements(int numtrans);
	void probeSelectionChanged();
	void setProbeXCenter();
	void setProbeYCenter();
	void setProbeZCenter();
	void setProbeXSize();
	void setProbeYSize();
	void setProbeZSize();

	

};
};
#endif // VIZWINMGR_H

