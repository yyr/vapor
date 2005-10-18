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
//		and some of the communication between these classes.
//		There is only one of these, it is created by the main program.
//		Other classes can use getInstance() to obtain it
//
#ifndef MAINFORM_H
#define MAINFORM_H

#include <qvariant.h>
#include <qmainwindow.h>
#include <qstring.h>

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
class FlowTab;
class AnimationTab;

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
	//Note this is a singleton class.  Only the main program will create it.
	static MainForm* getInstance(){
		if (!theMainForm)
			assert(0);
		return theMainForm;
	}
    MainForm( QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
    ~MainForm();

    
    QWidget* tab;
    QWidget* tab_2;
    QMenuBar *Main_Form;
    QPopupMenu *File;
	QPopupMenu *Edit;
    QPopupMenu *Data;
    QPopupMenu *viewMenu;
    
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
	QAction* editVizFeaturesAction;
    //Help menu
	//
    QAction* helpContentsAction;
    QAction* helpIndexAction;
    QAction* helpAboutAction;
    //Data menu
    QAction* dataBrowse_DataAction;
	QAction* dataExportToIDLAction;
    QAction* dataConfigure_MetafileAction;
    QAction* dataLoad_MetafileAction;
	QAction* dataLoad_DefaultMetafileAction;
	QAction* fileNew_SessionAction;
    
   //View menu
    QAction* viewLaunch_visualizerAction;
	QAction* viewStartCaptureAction;
	QAction* viewEndCaptureAction;
	QAction* viewSingleCaptureAction;
	
    //Script menu
   
    QAction* scriptIDL_scriptAction;
    QAction* scriptMatlab_scriptAction;
    QAction* scriptBatchAction;
    
    //Animation menu
    QAction* animationKeyframingAction;
	QAction* exportAnimationScriptAction;
    
    
	//Toolbars:
	QActionGroup* mouseModeActions;
	QAction* navigationAction;
	QAction* regionSelectAction;
	QAction* probeAction;
	QAction* rakeAction;
	QAction* contourAction;
	QAction* moveLightsAction;
	QAction* tileAction;
	QAction* cascadeAction;
	QAction* homeAction;
	QAction* sethomeAction;

    QDockWindow* tabDockWindow;
	TabManager* getTabManager() {return tabWidget;}

	VizTab* getVizTab() { return theVizTab;}
	RegionTab * getRegionTab() {return theRegionTab;}
	Dvr* getDvrTab() {return theDvrTab;}
	AnimationTab* getAnimationTab() {return theAnimationTab;}
	ContourPlaneTab* getContourTab() {return theContourTab;}
	IsoTab* getIsoTab() {return theIsoTab;}
	FlowTab* getFlowTab() {return theFlowTab;}
	QWorkspace* getWorkspace() {return myWorkspace;}
	//Disable the editUndo/Redo action:
	void disableUndoRedo();
	MouseModeCommand::mouseModeType getCurrentMouseMode() {return currentMouseMode;}
	
	

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
	virtual void defaultLoadData();
	virtual void newSession();
	virtual void exportToIDL();
    virtual void launchVisualizer();
	virtual void launchVizFeaturesPanel();
	virtual void startCapture();
	virtual void endCapture();
	virtual void captureSingle();
  
	virtual void calcIsosurface();
    virtual void viewpoint();
    virtual void region();
    virtual void renderDVR();
	virtual void animationParams();
	virtual void launchFlowTab();
	virtual void contourPlanes();
    virtual void batchSetup();

	virtual void setContourSelect(bool);
	virtual void setRegionSelect(bool);
	virtual void setProbe(bool);
	virtual void setRake(bool);
	virtual void setNavigate(bool);
	virtual void setLights(bool);
	virtual void editSessionParams();

	//Whenever the UndoRedo menu is displayed, need to supply the right text:
	//
	virtual void setupUndoRedoText();
	
protected:
	static MainForm* theMainForm;
	void resetModeButtons();
	
	Params::ParamType currentFrontTab;
	MouseModeCommand::mouseModeType currentMouseMode;
	QWorkspace* myWorkspace;
    TabManager* tabWidget;
    QHBoxLayout* MainFormLayout;
    QHBoxLayout* tabLayout;
    QHBoxLayout* tabLayout_2;
    QDesktopWidget* myDesktopWidget;
	VizTab* theVizTab;
	RegionTab* theRegionTab;
	IsoTab* theIsoTab;
	Dvr* theDvrTab;
	FlowTab* theFlowTab;
	AnimationTab* theAnimationTab;
	ContourPlaneTab* theContourTab;
	VizSelectCombo* windowSelector;
	
	QString sessionSaveFile;
	
	
	Vcr* vcrPanel;
	

protected slots:
    virtual void languageChange();
	void initViewMenu();

};
};
#endif // MAINFORM_H

