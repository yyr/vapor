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
#include <QPaintEvent>
#include <QActionGroup>
#include <QLabel>
#include "command.h"
#include "params.h"
class QApplication;
class QSpacerItem;
class QAction;
class QMenu;
class QActionGroup;
class QToolBar;
class QWidget;
class QDesktopWidget;
class QMdiArea;
class QDockWindow;
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
    	MainForm(QString& fileName, QApplication* app, QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window );
    	~MainForm();
	TabManager* getTabManager() {return tabWidget;}

	ViewpointEventRouter* getVizTab() { return theVizTab;}
	RegionEventRouter* getRegionTab() {return theRegionTab;}
	DvrEventRouter* getDvrTab() {return theDvrTab;}
	ProbeEventRouter* getProbeTab() {return theProbeTab;}
	TwoDImageEventRouter* getTwoDImageTab() {return theTwoDImageTab;}
	TwoDDataEventRouter* getTwoDDataTab() {return theTwoDDataTab;}
	AnimationEventRouter* getAnimationTab() {return theAnimationTab;}
	
	
	FlowEventRouter* getFlowTab() {return theFlowTab;}
	
	QMdiArea* getMDIArea() {return myMDIArea;}
	//Disable the editUndo/Redo action:
	void disableUndoRedo();
	QApplication* getApp() {return theApp;}
	
	void setInteractiveRefinementSpin(int);
	void setCurrentTimestep(int tstep);
	VizSelectCombo* getWindowSelector(){return windowSelector;}
	//following are accessed during undo/redo
	QAction* navigationAction;
	QAction* regionSelectAction;
	QAction* probeAction;
	QAction* twoDImageAction;
	QAction* twoDDataAction;
	QAction* rakeAction;
	QAction* moveLightsAction;
	QAction* editUndoAction;
	QAction* editRedoAction;
	QLineEdit* timestepEdit;
	QComboBox* alignViewCombo;
	//Following are public so accessible from animation tab:
	QAction* playForwardAction;
	QAction* playBackwardAction;
	QAction* pauseAction;
	
private:
    void createActions(); 
    void createMenus();
    void hookupSignals();
    void createToolBars();

    QWidget* tab;
    QWidget* tab_2;
    QMenuBar *Main_Form;
    QMenu *File;
    QMenu *Edit;
    QMenu *Data;
    QMenu *pythonMenu;
    QMenu *captureMenu;
    
    QMenu *helpMenu;
    QToolBar *modeToolBar;
    QToolBar *vizToolBar;
    QToolBar *animationToolBar;
   //File menu:
    QAction* fileOpenAction;
    QAction* fileSaveAction;
    QAction* fileSaveAsAction;
    QAction* fileExitAction;
    QAction* loadPreferencesAction;
    QAction* savePreferencesAction;

	//Edit menu:
	//
    QAction* editVizFeaturesAction;
    QAction* editPreferencesAction;
	QAction* editPythonStartupAction;
	QMenu* editPythonMenu;
	QAction* newPythonAction;
    
    //Help menu
    QAction* helpContentsAction;
    QAction* helpIndexAction;
    QAction* helpAboutAction;
    QAction* whatsThisAction;
    //Data menu
   
    QAction* dataExportToIDLAction;
    QAction* dataConfigure_MetafileAction;
    QAction* dataMerge_MetafileAction;
    QAction* dataImportWRF_Action;
    QAction* dataImportDefaultWRF_Action;
    QAction* dataLoad_MetafileAction;
    QAction* dataLoad_DefaultMetafileAction;
    QAction* dataSave_MetafileAction;
    QAction* fileNew_SessionAction;
	

   // Capture menu
	QAction* captureStartJpegCaptureAction;
	QAction* captureEndJpegCaptureAction;
	QAction* captureSingleJpegCaptureAction;
	QAction* captureStartFlowCaptureAction;
	QAction* captureEndFlowCaptureAction;

    
	//Toolbars:
	QActionGroup* mouseModeActions;
	
	QAction* tileAction;
	QAction* cascadeAction;
	QAction* homeAction;
	QAction* sethomeAction;
	QAction* viewAllAction;
	QAction* viewRegionAction;

	QAction* stepForwardAction;
	QAction* stepBackAction;
	
	QSpinBox* interactiveRefinementSpin;

    QDockWidget* tabDockWindow;

	
	

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
	virtual void importWRFData();
	virtual void importDefaultWRFData();
	virtual void saveMetadata();
	virtual void newSession();
	virtual void exportToIDL();
    virtual void launchVisualizer();
	virtual void launchVizFeaturesPanel();
	virtual void launchPythonEditor(QAction* act);
	virtual void newPythonEditor();
	virtual void editPythonStartup();
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


protected:
	virtual void paintEvent(QPaintEvent* e);
	static MainForm* theMainForm;
	void resetModeButtons();
	
	
	QMdiArea* myMDIArea;
    	TabManager* tabWidget;
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
	void setupEditMenu();
	void setInteractiveRefLevel(int);

};
};
#endif // MAINFORM_H

