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
    
	
    int getNumVisualizers(); 
    TabManager* getTabManager() { return tabManager;}
    
	//activeViz is -1 if none is active, otherwise is no. of active viz win.
	int getActiveViz(); 
	VizWin* getActiveVisualizer() {
		int activeViz = getActiveViz();
		assert(activeViz >= 0);
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
	VizWin* getVizWin(int i) {return VizWindow[i];}
	int maxVizWins(){return VizWindow.size();}
	
	void setVizWinName(int winNum, QString& qs);
	QString& getVizWinName(int winNum) {return VizName[winNum];}
	
	void replaceGlobalParams(Params* p, ParamsBase::ParamsBaseType t);
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
	//Set the dirty bits in all the visualizers that use a
	//region parameter setting
	//
	
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
	

    MainForm* myMainWindow;
    static TabManager* tabManager;
   
	
    QMdiArea* myMDIArea;
	
	int numVizWins;
	bool spinAnimate;

#endif //DOXYGEN_SKIP_THIS
	

	
};
};
#endif // VIZWINMGR_H

