//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		mainform.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Definition of MainForm class
//		This QT Main Window class supports all main window functionality
//		including menus, tab dialog, docking, visualizer window,
//		and some of the communication between these classes
//
#ifndef MAINFORM_H
#define MAINFORM_H

#include <qvariant.h>
#include <qmainwindow.h>
#include "command.h"
#include "params.h"

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QAction;
class QActionGroup;
class QToolBar;
class QPopupMenu;
class QWidget;
class QDesktopWidget;
class QWorkspace;
class QDockWindow;
class VizTab;
class RegionTab;
class ContourPlaneTab;
class Dvr;
class IsoTab;

namespace VAPoR{

class TabManager;
class VizWindow;
class VizWinMgr;
class VizSelectCombo;
class Vcr;
class Session;

class MainForm : public QMainWindow
{
    Q_OBJECT

public:
    MainForm( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
    ~MainForm();

    
    QWidget* tab;
    QWidget* tab_2;
    QMenuBar *Main_Form;
    QPopupMenu *File;
	QPopupMenu *Edit;
    QPopupMenu *Data;
    QPopupMenu *View;
    
    QPopupMenu *Script;
    QPopupMenu *Rendering;
    QPopupMenu *Animation;
    QPopupMenu *helpMenu;
    QToolBar *modeToolBar;
	QToolBar *vizToolBar;
	QToolBar *vcrToolBar;
   //File menu:
	//
    QAction* fileOpenAction;
    QAction* fileSaveAction;
    QAction* fileSaveAsAction;
    QAction* fileExitAction;
	//Edit menu:
	//
	QAction* editUndoAction;
	QAction* editRedoAction;
	QAction* editSessionParamsAction;
    //Help menu
	//
    QAction* helpContentsAction;
    QAction* helpIndexAction;
    QAction* helpAboutAction;
    //Data menu
    QAction* dataBrowse_DataAction;
	
    QAction* dataConfigure_MetafileAction;
    QAction* dataLoad_MetafileAction;
    QAction* regionAction;
   //View menu
    QAction* viewLaunch_visualizerAction;
    QAction* viewManage_Viz_WindowsAction;
    QAction* viewViewpointAction;
   
    //Script menu
    QAction* scriptJournaling_optionsAction;
    QAction* scriptIDL_scriptAction;
    QAction* scriptMatlab_scriptAction;
    QAction* scriptBatchAction;
    //Renderer menu
    QAction* dataCalculate_IsosurfaceAction;
    QAction* dataGenerate_Flow_VisAction;
    QAction* dataProbeAction;
    QAction* dataContour_PlanesAction;
    QAction* renderDVRAction;
    
    
    //Animation menu
    QAction* animationKeyframingAction;
    QAction* animationVCRAction;
    
	//Toolbars:
	QActionGroup* mouseModeActions;
	QAction* navigationAction;
	QAction* regionSelectAction;
	QAction* probeAction;
	QAction* contourAction;
	QAction* moveLightsAction;
	QAction* tileAction;
	QAction* cascadeAction;
	QAction* homeAction;
	QAction* sethomeAction;

    QDockWindow* tabDockWindow;
	TabManager* getTabManager() {return tabWidget;}

	VizWinMgr* getVizWinMgr() {return myVizMgr;}
	VizTab* getVizTab() { return theVizTab;}
	RegionTab * getRegionTab() {return theRegionTab;}
	Dvr* getDvrTab() {return theDvrTab;}
	ContourPlaneTab* getContourTab() {return theContourTab;}
	IsoTab* getIsoTab() {return theIsoTab;}
	QWorkspace* getWorkspace() {return myWorkspace;}
	Session* getSession() {return currentSession;}
	//Disable the editUndo/Redo action:
	void disableUndoRedo();
	

public slots:
   
    virtual void fileOpen();
    virtual void fileSave();
    virtual void fileSaveAs();
    virtual void filePrint();
    virtual void fileExit();

	virtual void undo();
	virtual void redo();

    virtual void helpIndex();
    virtual void helpContents();
    virtual void helpAbout();
    
    virtual void browseData();
	virtual void loadData();
    virtual void launchVisualizer();
    virtual void manageVizWindows();
	virtual void calcIsosurface();
    virtual void viewpoint();
    virtual void region();
    virtual void renderDVR();
	virtual void contourPlanes();
    virtual void batchSetup();
	virtual void launchVCRPanel();

	virtual void setContourSelect(bool);
	virtual void setRegionSelect(bool);
	virtual void setProbe(bool);
	virtual void setNavigate(bool);
	virtual void setLights(bool);
	virtual void editSessionParams();

	//Whenever the UndoRedo menu is displayed, need to supply the right text:
	//
	virtual void setupUndoRedoText();
	


protected:
	void resetModeButtons();
	
	Params::ParamType currentFrontTab;
	MouseModeCommand::mouseModeType currentMouseMode;
	QWorkspace* myWorkspace;
    TabManager* tabWidget;
    QHBoxLayout* MainFormLayout;
    QHBoxLayout* tabLayout;
    QHBoxLayout* tabLayout_2;
    QDesktopWidget* myDesktopWidget;
    VizWinMgr* myVizMgr;
	VizTab* theVizTab;
	RegionTab* theRegionTab;
	IsoTab* theIsoTab;
	Dvr* theDvrTab;
	ContourPlaneTab* theContourTab;
	VizSelectCombo* windowSelector;
	
	Vcr* vcrPanel;
	Session* currentSession;
	

protected slots:
    virtual void languageChange();

};
};
#endif // MAINFORM_H

