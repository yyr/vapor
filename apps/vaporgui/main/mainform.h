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
class QApplication;
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
class QLabel;
class QComboBox;
class QSpinBox;


namespace VAPoR{

class TabManager;
class VizWindow;
class VizWinMgr;
class VizSelectCombo;
class Vcr;
class Session;
class RegionEventRouter;
class AnimationEventRouter;
class DvrEventRouter;
class IsoEventRouter;
class ViewpointEventRouter;
class ProbeEventRouter;
class TwoDEventRouter;
class FlowEventRouter;

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
    MainForm(QString& fileName, QApplication* app, QWidget* parent = 0, const char* name = 0, WFlags fl = WType_TopLevel );
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
	QAction* loadPreferencesAction;
	QAction* savePreferencesAction;

	//Edit menu:
	//
	QAction* editUndoAction;
	QAction* editRedoAction;
	QAction* editVizFeaturesAction;
	QAction* editPreferencesAction;
    
    //Help menu
	//
    QAction* helpContentsAction;
    QAction* helpIndexAction;
    QAction* helpAboutAction;
	QAction* whatsThisAction;
    //Data menu
    QAction* dataBrowse_DataAction;
	QAction* dataExportToIDLAction;
    QAction* dataConfigure_MetafileAction;
	QAction* dataMerge_MetafileAction;
    QAction* dataLoad_MetafileAction;
	QAction* dataLoad_DefaultMetafileAction;
	QAction* dataSave_MetafileAction;
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
	QAction* twoDAction;
	QAction* rakeAction;
	
	QAction* moveLightsAction;
	QAction* tileAction;
	QAction* cascadeAction;
	QAction* homeAction;
	QAction* sethomeAction;
	QAction* viewAllAction;
	QAction* viewRegionAction;

	QComboBox* alignViewCombo;
	QSpinBox* interactiveRefinementSpin;

    QDockWindow* tabDockWindow;
	TabManager* getTabManager() {return tabWidget;}

	ViewpointEventRouter* getVizTab() { return theVizTab;}
	RegionEventRouter* getRegionTab() {return theRegionTab;}
	DvrEventRouter* getDvrTab() {return theDvrTab;}
	ProbeEventRouter* getProbeTab() {return theProbeTab;}
	TwoDEventRouter* getTwoDTab() {return theTwoDTab;}
	AnimationEventRouter* getAnimationTab() {return theAnimationTab;}
	
	
	FlowEventRouter* getFlowTab() {return theFlowTab;}
	
	QWorkspace* getWorkspace() {return myWorkspace;}
	//Disable the editUndo/Redo action:
	void disableUndoRedo();
	QApplication* getApp() {return theApp;}
	
	void setInteractiveRefinementSpin(int);

	
	

public slots:
   
    virtual void fileOpen();
    virtual void fileSave();
    virtual void fileSaveAs();
    virtual void filePrint();
    virtual void fileExit();
	virtual void loadPrefs();
	virtual void savePrefs();

	virtual void undo();
	virtual void redo();

    virtual void helpIndex();
    virtual void helpContents();
    virtual void helpAbout();
    
    virtual void browseData();
	virtual void loadData();
	virtual void defaultLoadData();
	virtual void mergeData();
	virtual void saveMetadata();
	virtual void newSession();
	virtual void exportToIDL();
    virtual void launchVisualizer();
	virtual void launchVizFeaturesPanel();
	virtual void launchPreferencesPanel();
	virtual void startCapture();
	virtual void endCapture();
	virtual void captureSingle();
  
    virtual void viewpoint();
    virtual void region();
    virtual void renderDVR();
	virtual void animationParams();
	virtual void launchFlowTab();
	virtual void launchIsoTab();
	virtual void launchProbeTab();
	virtual void launchTwoDTab();
    virtual void batchSetup();

	//Set various manipulator modes:
	virtual void setRegionSelect(bool);
	virtual void setProbe(bool);
	virtual void setTwoD(bool);
	virtual void setRake(bool);
	virtual void setNavigate(bool);
	virtual void setLights(bool);

	//Whenever the UndoRedo menu is displayed, need to supply the right text:
	//
	virtual void setupUndoRedoText();

protected:
	static MainForm* theMainForm;
	void resetModeButtons();
	
	Params::ParamType currentFrontTab;
	
	QWorkspace* myWorkspace;
    TabManager* tabWidget;
    QHBoxLayout* MainFormLayout;
    QHBoxLayout* tabLayout;
    QHBoxLayout* tabLayout_2;
    QDesktopWidget* myDesktopWidget;
	ViewpointEventRouter* theVizTab;
	RegionEventRouter* theRegionTab;
	DvrEventRouter* theDvrTab;
	IsoEventRouter* theIsoTab;
	FlowEventRouter* theFlowTab;
	ProbeEventRouter* theProbeTab;
	TwoDEventRouter* theTwoDTab;
	AnimationEventRouter* theAnimationTab;
	QApplication* theApp;
	
	VizSelectCombo* windowSelector;

	QLabel* modeStatusWidget;
	
	
	Vcr* vcrPanel;
	

protected slots:
    virtual void languageChange();
	void initViewMenu();
	void setInteractiveRefLevel(int);

};
};
#endif // MAINFORM_H

