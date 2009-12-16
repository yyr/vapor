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
#include <q3mainwindow.h>
#include <qstring.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QPaintEvent>
#include <Q3GridLayout>
#include <QLabel>
#include <Q3PopupMenu>
#include <Q3VBoxLayout>
#include <Q3ActionGroup>


#include "command.h"
#include "params.h"
class QApplication;
class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QSpacerItem;
class QAction;
class Q3ActionGroup;
class Q3ToolBar;
class Q3PopupMenu;
class QWidget;
class QDesktopWidget;
class QWorkspace;
class Q3DockWindow;
class QLabel;
class QComboBox;
class QSpinBox;
class QLineEdit;


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
class TwoDDataEventRouter;
class TwoDImageEventRouter;
class FlowEventRouter;

class MainForm : public Q3MainWindow
{
    Q_OBJECT

public:
	//Note this is a singleton class.  Only the main program will create it.
	static MainForm* getInstance(){
		if (!theMainForm)
			assert(0);
		return theMainForm;
	}
    MainForm(QString& fileName, QApplication* app, QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::WType_TopLevel );
    ~MainForm();

    
    QWidget* tab;
    QWidget* tab_2;
    QMenuBar *Main_Form;
    Q3PopupMenu *File;
	Q3PopupMenu *Edit;
    Q3PopupMenu *Data;
    Q3PopupMenu *viewMenu;
	Q3PopupMenu *captureMenu;
    
    //QPopupMenu *Script;
    Q3PopupMenu *Rendering;
    //QPopupMenu *Animation;
    Q3PopupMenu *helpMenu;
    Q3ToolBar *modeToolBar;
	Q3ToolBar *vizToolBar;
	Q3ToolBar *animationToolbar;
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

   // Capture menu
	QAction* captureStartJpegCaptureAction;
	QAction* captureEndJpegCaptureAction;
	QAction* captureSingleJpegCaptureAction;
	QAction* captureStartFlowCaptureAction;
	QAction* captureEndFlowCaptureAction;

    //Script menu
   /*
    QAction* scriptIDL_scriptAction;
    QAction* scriptMatlab_scriptAction;
    QAction* scriptBatchAction;
    
    //Animation menu
    QAction* animationKeyframingAction;
	QAction* exportAnimationScriptAction;
    */
    
	//Toolbars:
	Q3ActionGroup* mouseModeActions;
	QAction* navigationAction;
	QAction* regionSelectAction;
	QAction* probeAction;
	QAction* twoDImageAction;
	QAction* twoDDataAction;
	QAction* rakeAction;
	
	QAction* moveLightsAction;
	QAction* tileAction;
	QAction* cascadeAction;
	QAction* homeAction;
	QAction* sethomeAction;
	QAction* viewAllAction;
	QAction* viewRegionAction;

	QAction* playForwardAction;
	QAction* playBackwardAction;
	QAction* stepForwardAction;
	QAction* stepBackAction;
	QAction* pauseAction;
	QLineEdit* timestepEdit;

	QComboBox* alignViewCombo;
	QSpinBox* interactiveRefinementSpin;

    Q3DockWindow* tabDockWindow;
	TabManager* getTabManager() {return tabWidget;}

	ViewpointEventRouter* getVizTab() { return theVizTab;}
	RegionEventRouter* getRegionTab() {return theRegionTab;}
	DvrEventRouter* getDvrTab() {return theDvrTab;}
	ProbeEventRouter* getProbeTab() {return theProbeTab;}
	TwoDImageEventRouter* getTwoDImageTab() {return theTwoDImageTab;}
	TwoDDataEventRouter* getTwoDDataTab() {return theTwoDDataTab;}
	AnimationEventRouter* getAnimationTab() {return theAnimationTab;}
	
	
	FlowEventRouter* getFlowTab() {return theFlowTab;}
	
	QWorkspace* getWorkspace() {return myWorkspace;}
	//Disable the editUndo/Redo action:
	void disableUndoRedo();
	QApplication* getApp() {return theApp;}
	
	void setInteractiveRefinementSpin(int);
	void setCurrentTimestep(int tstep);

	
	

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
	virtual void startJpegCapture();
	virtual void endJpegCapture();
	virtual void captureSingleJpeg();
	virtual void startFlowCapture();
	virtual void endFlowCapture();
  
    virtual void viewpoint();
    virtual void region();
    virtual void renderDVR();
	virtual void animationParams();
	virtual void launchFlowTab();
	virtual void launchIsoTab();
	virtual void launchProbeTab();
	virtual void launchTwoDImageTab();
	virtual void launchTwoDDataTab();
    virtual void batchSetup();

	//Set various manipulator modes:
	virtual void setRegionSelect(bool);
	virtual void setProbe(bool);
	virtual void setTwoDImage(bool);
	virtual void setTwoDData(bool);
	virtual void setRake(bool);
	virtual void setNavigate(bool);
	virtual void setLights(bool);

	//animation toolbar:
	virtual void playForward();
	virtual void playBackward();
	virtual void pauseClick();
	virtual void stepForward();
	virtual void stepBack();
	virtual void setTimestep();

	//Whenever the UndoRedo menu is displayed, need to supply the right text:
	//
	virtual void setupUndoRedoText();

protected:
	virtual void paintEvent(QPaintEvent* e);
	static MainForm* theMainForm;
	void resetModeButtons();
	
	Params::ParamType currentFrontTab;
	
	QWorkspace* myWorkspace;
    TabManager* tabWidget;
    Q3HBoxLayout* MainFormLayout;
    Q3HBoxLayout* tabLayout;
    Q3HBoxLayout* tabLayout_2;
    QDesktopWidget* myDesktopWidget;
	ViewpointEventRouter* theVizTab;
	RegionEventRouter* theRegionTab;
	DvrEventRouter* theDvrTab;
	IsoEventRouter* theIsoTab;
	FlowEventRouter* theFlowTab;
	ProbeEventRouter* theProbeTab;
	TwoDImageEventRouter* theTwoDImageTab;
	TwoDDataEventRouter* theTwoDDataTab;
	AnimationEventRouter* theAnimationTab;
	QApplication* theApp;
	
	VizSelectCombo* windowSelector;

	QLabel* modeStatusWidget;
	
	
	Vcr* vcrPanel;
	

protected slots:
    virtual void languageChange();
	void initCaptureMenu();
	void setInteractiveRefLevel(int);

};
};
#endif // MAINFORM_H

