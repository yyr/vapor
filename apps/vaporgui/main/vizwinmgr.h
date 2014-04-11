//************************************************************************
//																		*tab
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


#ifndef VIZWINMGR_H
#define VIZWINMGR_H

class QMdiArea;
class QMdiSubWindow;
class QWidget;
class QDesktopWidget;
class QMainWidget;
class QTimer;

#include <qobject.h>

#include "viewpointparams.h"
#include "regionparams.h"
#include "animationparams.h"
#include <vapor/common.h>
#include "params.h"


namespace VAPoR{

class AnimationParams;
class EventRouter;
class TabManager;
class MainForm;
class ViewpointParams;
class RegionParams;

class VizWin;
class AnimationEventRouter;
class RegionEventRouter;
class ViewpointEventRouter;
typedef EventRouter* (EventRouterCreateFcn)();

//! \class VizWinMgr
//! \brief A class for managing all visualizers
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!	This class manages the VAPOR visualizers.
//!	Its main function is to keep track of what visualizers are active
//!	and what EventRouter and Params instances are associated with each visualizer.
//
class VizWinMgr : public QObject
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
	
	//! Static method that identifies the currently active params instance of a given type.
	//! This is the currently selected instance in the current active visualizer.
	//! \param[in] const std::string& tag  The XML tag associated with a Params class
	//! \retval Params* Pointer to Params instance that is currently active
	static Params* getActiveParams(const std::string& tag){
		return (Params::GetParamsInstance(tag, getInstance()->activeViz));
	}
	//! If the params indicated is not RenderParams, all visualizer windows are refreshed.  If it's a RenderParams,
	//! requests a re-rendering for the renderer associated with that particular RenderParams instance.
	//! The renderer must be enabled or this will have no effect unless the optional 'always' parameter is true.
	//! \param[in] RenderParams* pointer to RenderParams instance that is associated with the rendering requested.
	//! \param[in] bool always indicates that rerender will occur even if the params is disabled.
	void forceRender(const Params* p, bool always=false);
	
	
	//! Static method obtains the EventRouter instance associated with a particular Params type.  There is a unique EventRouter
	//! subclass associated with each tab, and a unique instance of that subclass.
	//! \param[in] Params::ParamsBaseType TypeId of the Params type
	//! \retval EventRouter* Pointer to the associated EventRouter instance
	static EventRouter* getEventRouter(Params::ParamsBaseType typeId);

	//! Static method that creates an eventRouter, and installs it as one of the tabs.
	//! All extension EventRouter classes must call this during the InstallExtensions() method.
	//! \param[in] const std::string tag : XML tag identifying the Params class.
	//! \param[in] EventRouterCreateFcn : the required method that creates the EventRouter.
	static void InstallTab(const std::string tag, EventRouterCreateFcn fcn);

	//! Static method obtains the EventRouter instance associated with a particular Params tag
	//! \param[in] const std::string& Tag of the Params 
	//! \retval EventRouter* pointer to the associated EventRouter instance
	static EventRouter* getEventRouter(const std::string& tag){
		return getEventRouter(ParamsBase::GetTypeFromTag(tag));
	}

	//! Static method that returns the global Params of a given type.
	//! If the params is a RenderParams type, returns a default instance that
	//! is not actually used in rendering
	//! \param[in] Params::ParamsBaseType TypeId of the Params type
	//! \retval Params* pointer to the associated Params instance
	static Params* getGlobalParams(Params::ParamsBaseType ptype);

	
	//! Method that identifies the params instance associated with a visualizer, type, and instance index. 
	//! \param[in] int winNum : Visualizer number
	//! \param[in] ParamsBase::ParamsBaseType TypeId associated with this Params
	//! \param[in] int instance : The index of the associated instance, or -1 for the active instance.
	//! \retval Params*  Pointer to the Params instance
	Params* getParams(int winNum, Params::ParamsBaseType pType, int instance = -1);

	//! Static method that identifies the number of instances of a Params of a particular type, in a particular visualizer.
	//! \param[in] int winnum Visualizer number
	//! \param[in] ParamsBase::ParamsBaseType TypeId associated with this Params
	//! \retval int Number of instances
	static int getNumInstances(int winnum, Params::ParamsBaseType pType);

	//! Static method that identifies the active instance index of a Params in a particular visualizer.
	//! \param[in] int winnum Visualizer number
	//! \param[in] ParamsBase::ParamsBaseType TypeId associated with this Params
	//! \retval int Current active instance index
	static int getCurrentInstanceIndex(int winnum, Params::ParamsBaseType t);

	//! Static method that identifies the instance index of a Params instance in a particular renderer tab.
	//! Each of the renderer instances indicated in the tab are associated with a unique Params instance, whether or
	//! not the instance has been enabled.  All of the instances are associated with a particular visualizer.
	//! \param[in] int winnum Visualizer number
	//! \param[in] Params* Pointer to Params instance
	//! \param[in] ParamsBase::ParamsBaseType TypeId associated with this Params
	//! \retval int Instance index associated with this params or -1 if not found.
	static int findInstanceIndex(int winnum, Params* params, Params::ParamsBaseType t);

	//! Method that identifies the active index in the active visualizer
	//! \param[in] ParamsBase::ParamsBaseType TypeId associated with this Params
	//! \retval int Current active instance index
	int getActiveInstanceIndex(Params::ParamsBaseType t){
		return getCurrentInstanceIndex(activeViz,t);
	}

	//! Identify the params instance that is current of a particular type (i.e. in a particular tab).
	//! \param[in] ParamsBase::ParamsBaseType TypeId associated with this Params
	//! \retval Params* Params instance that is current
	Params* getApplicableParams(Params::ParamsBaseType t);

	//! Identify the params instance that is currently applied of a particular tag
	//! \param[in] const std::string& XML tag associated with this Params
	//! \retval Params* Params instance that is current
	Params* getApplicableParams(const std::string& tag){
		return getApplicableParams(ParamsBase::GetTypeFromTag(tag));
	}


#ifndef DOXYGEN_SKIP_THIS
	//Following methods are not usually needed for extensibility:
	~VizWinMgr();
	static VizWinMgr* getInstance() {
		if (!theVizWinMgr)
			theVizWinMgr = new VizWinMgr();
		return theVizWinMgr;
	}
	
	static RegionParams* getActiveRegionParams(){
		return (getInstance()->getRegionParams(getInstance()->activeViz));
	}
	static ViewpointParams* getActiveVPParams(){
		return (getInstance()->getViewpointParams(getInstance()->activeViz));
	}
	static AnimationParams* getActiveAnimationParams(){
		return (getInstance()->getAnimationParams(getInstance()->activeViz));
	}
	void createAllDefaultTabs();
	void InstallExtensionTabs();
	
    
    //Respond to end:
    void closeEvent();
    void vizAboutToDisappear(int i); 

    
    //method to launch a viz window, returns vizNum

    int launchVisualizer();
    
	
    int getNumVisualizers(); 
    TabManager* getTabManager() { return tabManager;}
    
	//activeViz is -1 if none is active, otherwise is no. of active viz win.
	int getActiveViz() {return activeViz;}
	VizWin* getActiveVisualizer() {
		if(activeViz>=0)return VizWindow[activeViz];
		else return 0;
	}
	//Method that is called when a window is made active:
	void setActiveViz(int vizNum); 
	//Obtain the parameters that currently apply in specified window:
	ViewpointParams* getViewpointParams(int winNum){
		return (ViewpointParams*)Params::GetCurrentParamsInstance(Params::GetTypeFromTag(Params::_viewpointParamsTag),winNum);
	}
	RegionParams* getRegionParams(int winNum){
		return (RegionParams*)Params::GetCurrentParamsInstance(Params::GetTypeFromTag(Params::_regionParamsTag),winNum);
	}
	AnimationParams* getAnimationParams(int winNum){
		return (AnimationParams*)Params::GetCurrentParamsInstance(Params::GetTypeFromTag(Params::_animationParamsTag),winNum);
	}
	//For a renderer, there will only exist a local version.
	// If instance is negative, return the current instance.
	
	void setCurrentInstanceIndex(int winnum, int inst, Params::ParamsBaseType t);
	
	void appendInstance(int winnum, Params* p);
	
	void insertInstance(int winnum, int posn, Params* p);
	
	
	void removeInstance(int winnum, int instance, Params::ParamsBaseType t);
	
	RegionEventRouter* getRegionRouter();
	AnimationEventRouter* getAnimationRouter(); 
	ViewpointEventRouter* getViewpointRouter();

	//Make all the params for the current window update their tabs:
	void updateActiveParams();
	
	//set/get Data describing window states
	VizWin* getVizWin(int i) {return VizWindow[i];}
	int maxVizWins(){return VizWindow.size();}
	
	void setVizWinName(int winNum, QString& qs);
	QString& getVizWinName(int winNum) {return VizName[winNum];}
	
	void replaceGlobalParams(Params* p, ParamsBase::ParamsBaseType t);
	void createDefaultParams(int winnum);
	Params* getCorrespondingLocalParams(Params* p) {
		return getLocalParams(p->GetParamsBaseTypeId());
	}
	
	//Direct access to actual params object:
	ViewpointParams* getRealVPParams(int win) {
		if (!VizWindow[win]) return 0;
		Params* p = Params::GetParamsInstance(Params::_viewpointParamsTag,win,-1);
		if (p->IsLocal()) return (ViewpointParams*)p;
		return (ViewpointParams*)Params::GetDefaultParams(Params::_viewpointParamsTag);
	}
	//Trackball* getGlobalTrackball() {return globalTrackball;}
	
	//Force all renderers to re-obtain render data
	void refreshRenderData();
	//
	//Disable all renderers that use specified variables
	void disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D);
	//Disable all renderers 
	void disableAllRenderers();
	//Following methods notify all params that are associated with a
	//specific window.
	//Set the dirty bits in all the visualizers that use a
	//region parameter setting
	//
	
	void refreshViewpoint(ViewpointParams* vParams);
	void refreshRegion(RegionParams* rParams);
	
	
	//Reset the near/far distances for all the windows that
	//share a viewpoint, based on region in specified regionparams
	//
	void resetViews(ViewpointParams* vp);
	
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
	
	
	//this tag needs to be visible in session class
	static const string _visualizersTag;

	
	
	void setInteractiveNavigating(int level);
	
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
	//The vizWinMgr has to dispatch signals from gui's to the appropriate parameter panels
	
	void setVpLocalGlobal(int val);
	void setRgLocalGlobal(int val);
	void setAnimationLocalGlobal(int val);
	
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
	static const string _vizColorbarDigitsAttr;
	static const string _vizColorbarFontsizeAttr;
	static const string _vizColorbarLLPositionAttr;
	static const string _vizColorbarURPositionAttr;
	static const string _vizColorbarNumTicsAttr;
	static const string _vizAxisArrowsEnabledAttr;
	static const string _vizColorbarEnabledAttr;
	static const string _vizColorbarParamsNameAttr;
	static const string _vizRegionFrameEnabledAttr;
	static const string _vizSubregionFrameEnabledAttr;
	static const string _visualizerNumAttr;
	static const string _vizElevGridDisplacementAttr;
	static const string _vizElevGridInvertedAttr;
	static const string _vizElevGridRotationAttr;
	static const string _vizElevGridTexturedAttr;
	static const string _vizElevGridTextureNameAttr;

	
	RegionParams* getRealRegionParams(int win) {
		if (!VizWindow[win]) return 0;
		Params* p = Params::GetParamsInstance(Params::_regionParamsTag,win,-1);
		if (p->IsLocal()) return (RegionParams*)p;
		return (RegionParams*)Params::GetDefaultParams(Params::_regionParamsTag);
	}
	AnimationParams* getRealAnimationParams(int win) {
		if (!VizWindow[win]) return 0;
		Params* p = Params::GetParamsInstance(Params::_animationParamsTag,win,-1);
		if (p->IsLocal()) return (AnimationParams*)p;
		return (AnimationParams*)Params::GetDefaultParams(Params::_animationParamsTag);
	}
	ViewpointParams* getGlobalVPParams(){return (ViewpointParams*)(Params::GetDefaultParams(Params::_viewpointParamsTag));}
	RegionParams* getGlobalRegionParams(){
		return (RegionParams*)(Params::GetDefaultParams(Params::_regionParamsTag));
	}
	AnimationParams* getGlobalAnimationParams(){return (AnimationParams*)(Params::GetDefaultParams(Params::_animationParamsTag));}

	static Params::ParamsBaseType RegisterEventRouter(const std::string tag, EventRouter* router);
	static std::map<ParamsBase::ParamsBaseType, EventRouter*> eventRouterMap;
	
	// Method that returns the current params instance that applies in the current
	// active visualizer if it is local.  
	Params* getLocalParams(Params::ParamsBaseType t);
	
	static void setGlobalParams(Params* p, ParamsBase::ParamsBaseType t);

	static VizWinMgr* theVizWinMgr;
	VizWinMgr ();
	
   
	std::map<int, VizWin*> VizWindow;
	std::map<int,QMdiSubWindow*> VizMdiWin;
	std::map<int,QString> VizName;
	std::map<int,int> ActivationOrder;
	
	//Remember the activation order:
	int activationCount;
	int getLastActive(){
		int mx = -1; int mj = -1;
		for (int j = 0; j< VizWindow.size(); j++){
			if (VizWindow[j] && ActivationOrder[j]>mx){ 
				mx = ActivationOrder[j];
				mj = j;
			}
		}
		return mj;
	}
	//Trackball* globalTrackball;

    MainForm* myMainWindow;
    static TabManager* tabManager;
   
    int activeViz;
	vector<int> parsingInstance;
	int parsingVizNum, parsingDvrInstance, parsingIsoInstance,parsingFlowInstance, parsingProbeInstance,parsingTwoDDataInstance,parsingTwoDImageInstance;
	
    QMdiArea* myMDIArea;
	
	int numVizWins;
	bool spinAnimate;

#endif //DOXYGEN_SKIP_THIS
	

	
};
};
#endif // VIZWINMGR_H

