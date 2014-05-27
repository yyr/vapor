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
//!	It provides various methods relating the active visualizers and the
//! corresponding EventRouter and Params classes.
//! Methods are also provided for setting up the Qt OpenGL context of a QGLWidget and
//! associating the corresponding Visualizer instance.
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

	//! Obtain the active region params.  This is the RegionParams instance that is
	//! applicable in the current active Visualizer.
	//! \retval RegionParams* is the current active RegionParams instance
	static RegionParams* getActiveRegionParams(){
		return (getInstance()->getRegionParams(getInstance()->getActiveViz()));
	}
	//! Obtain the active viewpoint params.  This is the ViewpointParams instance that is
	//! applicable in the current active Visualizer.
	//! \retval ViewpointParams* is the current active ViewpointParams instance
	static ViewpointParams* getActiveVPParams(){
		return (getInstance()->getViewpointParams(getInstance()->getActiveViz()));
	}
	//! Obtain the active animation params.  This is the AnimationParams instance that is
	//! applicable in the current active Visualizer.
	//! \retval AnimationParams* is the current active AnimationParams instance
	static AnimationParams* getActiveAnimationParams(){
		return (getInstance()->getAnimationParams(getInstance()->getActiveViz()));
	}

	//! Method setting up the tabs in their default state.
	//! This is invoked once at the time vaporgui is started.
	//! \note Currently there is no provision for installing extension tabs
	void createAllDefaultTabs();
    
	//! Method that responds to user destruction of a visualizer.
	//! Relevant params, renderers, etc. are removed.
    void vizAboutToDisappear(int i); 
    
    //! Method launches a new visualizer, sets up appropriate
	//! Params etc.  It returns the visualizer index as
	//! returned by ControlExec::NewVisualizer()
	//! \retval visualizer index
    int launchVisualizer();

	//! Method that sets up a particular visualizer when the 
	//! visualizer index and Visualizer index is already known,
	//! as when performing Undo of visualizer destruction.
	//! \param[in] visualizer index
	//! \retval visualizer index
	int addVisualizer(int viznum);

	//! Method to delete a visualizer under program control, e.g. if
	//! the user performs Undo after creating a visualizer
	//! \param[in] viznum Visualizer index.
	void removeVisualizer(int viznum);

	//! method to launch a viz window, when the associated params already exist,
	//! associated with a specified visualizer.
	//! Useful when a new session is opened.
	//! \param[in] useViznum specifies the visualizer number that is to be attached
	//! \retval visualizer index that was attached.
    int attachVisualizer(int useViznum);
    
	//! Determine the total number of visualizers.
	//! \retval number of visualizers.
    int getNumVisualizers(); 

	//! Return the current Tab Manager.
	//! This is useful for querying the front tab
	//! and other aspects of the tab widget.
    TabManager* getTabManager() { return tabManager;}
    
	//! Obtain the index of the current active (i.e. selected) visualizer
	//! \retval index of active visualizer
	int getActiveViz(); 

	//! Obtain the VizWin object associated with the current active visualizer.
	//! \retval VizWin* current active VizWin object
	VizWin* getActiveVisualizer() {
		int activeViz = getActiveViz();
		assert(activeViz >= 0);
		//Use an iterator so we don't insert if the viz is not there
		std::map<int, VizWin*>::iterator it;
		it = VizWindow.find(activeViz);
		if (it == VizWindow.end()) return 0;
		return VizWindow[activeViz];
	}
	//! Set a Visualizer to be the active (selected) Visualizer
	//! \param[in] vizNum index of Visualizer to be activated.
	void setActiveViz(int vizNum); 

	//! Obtain the ViewpointParams that are applicable in a particular Visualizer
	//! \param[in] winNum Visualizer index
	//! \retval ViewpointParams* Params instance that is applicable.
	ViewpointParams* getViewpointParams(int winNum);

	//! Obtain the RegionParams that are applicable in a particular Visualizer
	//! \param[in] winNum Visualizer index
	//! \retval RegionParams* Params instance that is applicable.
	RegionParams* getRegionParams(int winNum);

	//! Obtain the AnimationParams that are applicable in a particular Visualizer
	//! \param[in] winNum Visualizer index
	//! \retval AnimationParams* Params instance that is applicable.
	AnimationParams* getAnimationParams(int winNum);

	//! Obtain the (unique) RegionEventRouter in the GUI
	//! \retval RegionEventRouter* 
	RegionEventRouter* getRegionRouter();

	//! Obtain the (unique) AnimationEventRouter in the GUI
	//! \retval AnimationEventRouter* 
	AnimationEventRouter* getAnimationRouter();

	//! Obtain the (unique) ViewpointEventRouter in the GUI
	//! \retval ViewpointEventRouter* 
	ViewpointEventRouter* getViewpointRouter();

	//! Force all the EventRouters to update based on the
	//! state of the Params for the active window.
	void updateActiveParams();
	
	//! Obtain the VizWin associated with a visualizer index
	//! \param[in] visualizer index
	//! \retval VizWin* associated VizWin object
	VizWin* getVizWin(int i) {return VizWindow[i];}
	
	//! Create all the default params for a specific visualizer
	//! \param[in] winnum Visualizer index
	void createDefaultParams(int winnum);
	
	//! Force all renderers to re-obtain their data, 
	//! for example when a new session is opened.
	void refreshRenderData();

	
	//! Disable all renderers that use a specified set of variables.
	//! Useful for example when a Python script is updated.
	//! \param[in] vars2D vector of 2D variable names
	//! \param[in] vars3D vector of 3D variable names
	void disableRenderers(const vector<string>& vars2D, const vector<string>& vars3D);

	//! Disable all enabled renderers 
	void disableAllRenderers();

	//Following methods notify all params that are associated with a
	//specific window.
	//! Force re-render in all Visualizers that use a specific ViewpointParams instance
	//! \param[in] vParams ViewpointParams instance
	void refreshViewpoint(ViewpointParams* vParams);

	//! Force re-render in all Visualizers that use a specific RegionParams instance
	//! \param[in] rParams RegionParams instance
	void refreshRegion(RegionParams* rParams);
	
	//! Reset the near/far distances for all the windows that
	//! share a viewpoint
	//! \param[in] vp ViewpointParams* 
	void resetViews(ViewpointParams* vp);
	
	~VizWinMgr();

	//! Obtain the (unique) VizWinMgr instance
	//! \retval VizWinMgr*
	static VizWinMgr* getInstance() {
		if (!theVizWinMgr)
			theVizWinMgr = new VizWinMgr();
		return theVizWinMgr;
	}
	

public slots:
	//! Arrange the Visualizers in a cascading sequence
    void cascade();

	//! Arrange the visualizers to tile the available space.
    void fitSpace();

	//! Change viewpoint to the current home viewpoint
	void home();

	//! Set the current home viewpoint based on current viewpoint
	void sethome();

	//! Respond to user request to activate a window:
	void winActivated(int);

	//! Close the VizWin associated with a Visualizer index
	void killViz(int viznum);
	//Slots that set viewpoint:

	//! Move camera in or out to make entire volume visible
	void viewAll();

	//! Move camera in or out to make current region visible
	void viewRegion();

	//! Align the camera to a specified axis
	//! param[in] axis 1,2, or 3.
	void alignView(int axis);

	//! Set the current active visualizer to use local or global viewpoint
	//! \param[in] val is 0 for global, 1 for local.
	void setVpLocalGlobal(int val);

	//! Set the current active Visualizer to use local or global Region settings
	//! \param[in] val is 0 for global, 1 for local.
	void setRgLocalGlobal(int val);

	//! Set the current active Visualizer to use local or global Animation settings
	//! \param[in] val is 0 for global, 1 for local.
	void setAnimationLocalGlobal(int val);
	
#ifndef DOXYGEN_SKIP_THIS
signals:
	//Turn on/off multiple viz options:
	void enableMultiViz(bool onOff);
	//Respond to user setting the vizselectorcombo:
	void newViz(QString&, int);
	void removeViz(int);
	void activateViz(int);
	void changeName(QString&, int);
	
private:
	
	static Params::ParamsBaseType RegisterEventRouter(const std::string tag, EventRouter* router);
	static std::map<ParamsBase::ParamsBaseType, EventRouter*> eventRouterMap;
	
	static VizWinMgr* theVizWinMgr;
	VizWinMgr();
	
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

