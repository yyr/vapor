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
#include "dvrparams.h"
#include "contourparams.h"
#include "isosurfaceparams.h"
#include "animationparams.h"
#include "vaporinternal/common.h"
#include "params.h"
#include "command.h"
class RegionTab;
class Dvr;
class ContourPlaneTab;
class VizTab;
class IsoTab;
class AnimationTab;



namespace VAPoR{
class AnimationParams;
class IsosurfaceParams;
class VizMgrDialog;
class TabManager;
class MainForm;
class ViewpointParams;
class RegionParams;
class DvrParams;
class ContourParams;
class Trackball;
class VizWin;
class VizWinMgr : public QObject
{
	Q_OBJECT
public:
    VizWinMgr (MainForm* mainWindow);
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
    //Temp method to launch a viz window:
    void launchVisualizer(int num = -1, const char* name = "");
    
    void setDialog (VizMgrDialog* dlg) {
        //if (dlg) qWarning("setting dialog to valid no.");
        currentVizMgrDialog = dlg;
    }
    //When the user changes window (max, min, normal), we may need to update the gui:
    void comboChanged(int newSetting, int vizNum);

	void nameChanged(QString& name, int num);
    
    
    int getNumVisualizers(); 
    
    TabManager* getTabManager() { return tabManager;}
    MainForm* getMainWindow() {return myMainWindow;}
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
	ContourParams* getContourParams(int winNum);
	IsosurfaceParams* getIsoParams(int winNum);
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
	void hookUpIsoTab(IsoTab*);
	void hookUpContourTab(ContourPlaneTab*);
	void hookUpAnimationTab(AnimationTab*);
	//set/get Data describing window states
	VizWin* getVizWin(int i) {return vizWin[i];}
	//Direct access to actual params object:
	ViewpointParams* getRealVPParams(int i) {return vpParams[i];}
	ContourParams* getRealContourParams(int i) {return contourParams[i];}
	IsosurfaceParams* getRealIsoParams(int i) {return isoParams[i];}
	DvrParams* getRealDvrParams(int i) {return dvrParams[i];}
	RegionParams* getRealRegionParams(int i) {return rgParams[i];}
	AnimationParams* getRealAnimationParams(int i) {return animationParams[i];}

	void setVizWinName(int winNum, QString qs);
	QString* getVizWinName(int winNum) {return vizName[winNum];}
	bool isMinimized(int winNum) {return isMin[winNum];}
	bool isMaximized(int winNum) {return isMax[winNum];}
	//Setting a params changes the previous params to the
	//specified one, performs needed ref/unref
	void setContourParams(int winNum, ContourParams* p);
	void setDvrParams(int winNum, DvrParams* p);
	void setIsoParams(int winNum, IsosurfaceParams* p);
	void setViewpointParams(int winNum, ViewpointParams* p);
	void setRegionParams(int winNum, RegionParams* p);
	void setAnimationParams(int winNum, AnimationParams* p);
		
	void createDefaultRendererPanels(int winnum);
	
	void setSelectionMode( Command::mouseModeType m) {selectionMode = m;}
	Command::mouseModeType getSelectionMode () {return selectionMode;}
	Trackball* getGlobalTrackball() {return globalTrackball;}

	//Set the dirty bits in all the visualizers that use a
	//region parameter setting
	void setRegionDirty(RegionParams*);
	//Force updates of all renderers using a dvr
	//
	void setDvrDirty(DvrParams* );
	//Tell all parameter panels to reinitialize (based on change of 
	//Metadata)
	void reinitializeParams();
	Command::mouseModeType selectionMode;

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

signals:
	//Turn on/off multiple viz options:

	void enableMultiViz(bool onOff);
	//Respond to user setting the vizselectorcombo:
	void newViz(QString&, int);
	void removeViz(int);
	void activateViz(int);
	void changeName(QString&, int);
	
	

protected:
	//Made public only to allow gui to access
    VizWin* vizWin[MAXVIZWINS];
    QRect* vizRect[MAXVIZWINS];
    QString* vizName[MAXVIZWINS];
    bool isMax[MAXVIZWINS];
    bool isMin[MAXVIZWINS];
	ViewpointParams* vpParams[MAXVIZWINS];
	RegionParams* rgParams[MAXVIZWINS];
	DvrParams* dvrParams[MAXVIZWINS];
	ContourParams* contourParams[MAXVIZWINS];
	IsosurfaceParams* isoParams[MAXVIZWINS];
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
	DvrParams* globalDvrParams;
	ContourParams* globalContourParams;
	AnimationParams* globalAnimationParams;
	
    VizMgrDialog* currentVizMgrDialog;
    MainForm* myMainWindow;
    TabManager* tabManager;
   
    int activeViz;
	
	
    QWorkspace* myWorkspace;
	ContourPlaneTab* myContourPlaneTab;
	Dvr* myDvrTab;


protected slots:
	//The vizWinMgr has to dispatch signals from gui's to the appropriate parameter panels
	//First, the viztab slots:
	//void setLights(const QString& qs);
	void setVpLocalGlobal(int val);
	void setRgLocalGlobal(int val);
	void setDvrLocalGlobal(int val);
	void setIsoLocalGlobal(int val);
	void setContourLocalGlobal(int val);
	void setAnimationLocalGlobal(int val);
	void setVPPerspective(int isOn);

	void setVtabTextChanged(const QString& qs);
	void setRegionTabTextChanged(const QString& qs);
	void setIsoTabTextChanged(const QString& qs);
	void setDvrTabTextChanged(const QString& qs);
	void setContourTabTextChanged(const QString& qs);
	void setAtabTextChanged(const QString& qs);
	
	void viewpointReturnPressed();
	void regionReturnPressed();
	void dvrReturnPressed();
	void isoReturnPressed();
	void contourReturnPressed();
	void animationReturnPressed();

	//Animation slots:
	void animationSetFrameStep();
	void animationSetPosition();
	void animationPauseClick(bool);
	void animationPlayReverseClick(bool);
	void animationPlayForwardClick(bool);
	void animationReplayClick();
	void animationToBeginClick();
	void animationToEndClick();
	void animationStepForwardClick();
	void animationStepReverseClick();


	//Then the regionTab slots:
	void setRegionNumTrans(int);
	void setRegionMaxSize();
	//Sliders set these:
	void setRegionXCenter();
	void setRegionYCenter();
	void setRegionZCenter();
	void setRegionXSize();
	void setRegionYSize();
	void setRegionZSize();
	void setRegionTimestep();

	//Slots for dvr panel:
	void setDvrEnabled(int on);
	void setDvrVariableNum(int);
	void setDvrLighting(bool);
	void setDvrNumBits(int);
	void setDvrMouseMode(int);
	void dvrHistoStretch();
	void dvrColorBind();
	void dvrOpacBind();
	
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

	
	

};
};
#endif // VIZWINMGR_H

