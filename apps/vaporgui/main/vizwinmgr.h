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

public:
	
	
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

	//! Reset the GUI to its default state, either due to New Session, or in preparation for loading a session file.
	void SetToDefaults();

	//! Static helper method invoked during Undo and Redo visualizer creation and destruction, as well
	//! setting the current viz window.
	//! This function must be passed in Command::CaptureStart()
	//! \sa UndoRedoHelpCB_T
	//! \param[in] isUndo indicates whether an Undo or Redo is being performed
	//! \param[in] instance is not used here
	//! \param[in] beforeP is a copy of the VizWinParams* at the start of the Command
	//! \param[in] afterP is a copy of the VizWinParams* at the end of the Command 
	static void UndoRedo(bool isUndo, int /*instance*/, Params* beforeP, Params* afterP);

#ifndef DOXYGEN_SKIP_THIS
	//Following methods are not usually needed for extensibility:
	~VizWinMgr();
	static VizWinMgr* getInstance() {
		if (!theVizWinMgr)
			theVizWinMgr = new VizWinMgr();
		return theVizWinMgr;
	}
	
	static RegionParams* getActiveRegionParams(){
		return (getInstance()->getRegionParams(getInstance()->getActiveViz()));
	}
	static ViewpointParams* getActiveVPParams(){
		return (getInstance()->getViewpointParams(getInstance()->getActiveViz()));
	}
	static AnimationParams* getActiveAnimationParams(){
		return (getInstance()->getAnimationParams(getInstance()->getActiveViz()));
	}
	void createAllDefaultTabs();
	void InstallExtensionTabs();
    
    //Respond to end:
    void closeEvent();
    void vizAboutToDisappear(int i); 
    
    //method to launch a viz window, returns vizNum
    int launchVisualizer();
	//method to add a specific viz window, associated with a specific visualizer:
	int addVisualizer(int viznum);
	void removeVisualizer(int viznum);
	//! method to launch a viz window, params already exist.
	//! Associated with a specified visualizer
	//! \param[in] useViznum specifies the visualizer number that is to be attached
    int attachVisualizer(int useViznum);
    
    int getNumVisualizers(); 
    TabManager* getTabManager() { return tabManager;}
    
	//activeViz is -1 if none is active, otherwise is no. of active viz win.
	int getActiveViz(); 

	VizWin* getActiveVisualizer() {
		int activeViz = getActiveViz();
		assert(activeViz >= 0);
		//Use an iterator so we don't insert if the viz is not there
		std::map<int, VizWin*>::iterator it;
		it = VizWindow.find(activeViz);
		if (it == VizWindow.end()) return 0;
		return VizWindow[activeViz];
	}

	//Method that is called when a window is made active:
	void setActiveViz(int vizNum); 
	//Obtain the parameters that currently apply in specified window:
	ViewpointParams* getViewpointParams(int winNum);
	RegionParams* getRegionParams(int winNum);
	AnimationParams* getAnimationParams(int winNum);
	RegionEventRouter* getRegionRouter();
	AnimationEventRouter* getAnimationRouter(); 
	ViewpointEventRouter* getViewpointRouter();

	//Make all the params for the current window update their tabs:
	void updateActiveParams();
	
	//set/get Data describing window states
	//Obtain the vizWin associated with a visualizer
	VizWin* getVizWin(int i) {return VizWindow[i];}
	
	void createDefaultParams(int winnum);
	
	//Force all renderers to re-obtain render data
	void refreshRenderData();
	//
	//Disable all renderers that use specified variables
	void disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D);
	//Disable all renderers 
	void disableAllRenderers();
	//Following methods notify all params that are associated with a
	//specific window.
	
	void refreshViewpoint(ViewpointParams* vParams);
	void refreshRegion(RegionParams* rParams);
	
	//Reset the near/far distances for all the windows that
	//share a viewpoint, based on region in specified regionparams
	//
	void resetViews(ViewpointParams* vp);
	
	//Make each window use its viewpoint params
	void initViews();
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
	
	ViewpointParams* getGlobalVPParams();
	RegionParams* getGlobalRegionParams();
	AnimationParams* getGlobalAnimationParams();

	static Params::ParamsBaseType RegisterEventRouter(const std::string tag, EventRouter* router);
	static std::map<ParamsBase::ParamsBaseType, EventRouter*> eventRouterMap;
	
	static VizWinMgr* theVizWinMgr;
	VizWinMgr ();
	
	//Map visualizer id to vizwin
	std::map<int, VizWin*> VizWindow;
	std::map<int,QMdiSubWindow*> VizMdiWin;
	//Activation order is a history of all the active visualizer indices.
	//It is a mapping from visualizer indices to the activation order.
	//When a visualizer is closed, the most recently active visualizer is re-activated.
	std::map<int,int> ActivationOrder;
	
	//Remember the activation order:
	int activationCount;
	//Find the previous active visualizer, by looking at the last one
	//that corresponds to a valid visualizer
	int getLastActive(){
		int lastActivation = -1; int prevActiveVis = -1;
		//Use an iterator to loop over activation orders
		std::map<int, int>::iterator it;
		std::map<int, VizWin*>::iterator it2;
		for (it = ActivationOrder.begin(); it != ActivationOrder.end(); it++){
			int order = it->second;
			int vis = it->first;
			//Check if vis is a valid visualizer
			it2 = VizWindow.find(vis);
			if (it2 == VizWindow.end()) continue;
			
			if (order >lastActivation){ 
				lastActivation = order;
				prevActiveVis = vis;
			}
		}
		return prevActiveVis;
	}
	

    MainForm* myMainWindow;
    static TabManager* tabManager;
   
    QMdiArea* myMDIArea;
	
	bool spinAnimate;

#endif //DOXYGEN_SKIP_THIS
	

	
};
};
#endif // VIZWINMGR_H

