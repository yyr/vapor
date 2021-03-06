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
#include <QComboBox>
#include <QIcon>
#include "command.h"
#include "params.h"
#include "bannergui.h"
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
class ViewpointEventRouter;

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
	static TabManager* getTabManager() {return tabWidget;}

	static void showTab(const std::string& tag);
	QMdiArea* getMDIArea() {return myMDIArea;}
	//Disable the editUndo/Redo action:
	void disableUndoRedo();
	QApplication* getApp() {return theApp;}
	
	void setInteractiveRefinementSpin(int);
	void setCurrentTimestep(int tstep);
	void enableKeyframing(bool onoff);
	VizSelectCombo* getWindowSelector(){return windowSelector;}
	//following are accessed during undo/redo
	QAction* navigationAction;
	
	QAction* editUndoAction;
	QAction* editRedoAction;
	QLineEdit* timeStepEdit;
	QComboBox* alignViewCombo;
	QComboBox* modeCombo;
	//Following are public so accessible from animation tab:
	QAction* playForwardAction;
	QAction* playBackwardAction;
	QAction* pauseAction;
	//Set the animation buttons in pause state,
	//don't trigger an event:
	void setPause(){
		playForwardAction->setChecked(false);
		playBackwardAction->setChecked(false);
		pauseAction->setChecked(true);
	}
	int addMode(QString& text, QIcon& icon){
		modeCombo->addItem(icon, text);
		return (modeCombo->count()-1);
	}
	void setMouseMode(int newMode) {modeCombo->setCurrentIndex(newMode);}
	void showCitationReminder();
	void buildWebTabHelpMenu(vector<QAction*>* actions);
	void buildWebHelpMenus();
	
private:
    void createActions(); 
    void createMenus();
    void hookupSignals();
    void createToolBars();
	void importData(const string& modelType, bool useDefault);

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
	QMenu *webTabHelpMenu;
	QMenu *webBasicHelpMenu;
	QMenu *webPreferencesHelpMenu;
	QMenu *webPythonHelpMenu;
	QMenu *webVisualizationHelpMenu;
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
	QMenu* importDataMenu;
	QMenu* importDefaultDataMenu;
    QAction* dataImportWRF_Action;
    QAction* dataImportDefaultWRF_Action;
	QAction* dataImportMOM_Action;
    QAction* dataImportDefaultMOM_Action;
	QAction* dataImportROMS_Action;
    QAction* dataImportDefaultROMS_Action;
	QAction* dataImportCAM_Action;
	QAction* dataImportDefaultCAM_Action;
	QAction* dataImportGRIB_Action;
	QAction* dataImportDefaultGRIB_Action;
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
    
    BannerGUI* banner;
	
	
	

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
	virtual void importMOMData();
	virtual void importDefaultMOMData();
	virtual void importROMSData();
	virtual void importDefaultROMSData();
	virtual void importCAMData();
	virtual void importDefaultCAMData();
	virtual void importGRIBData();
	virtual void importDefaultGRIBData();
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
  
    virtual void batchSetup();

	//Set navigate mode
	virtual void setNavigate(bool);
	
	//animation toolbar:
	virtual void playForward();
	virtual void playBackward();
	virtual void pauseClick();
	virtual void stepForward();
	virtual void stepBack();
	virtual void setTimestep();
	void launchWebHelp(QAction*);


protected:
	virtual void paintEvent(QPaintEvent* e);
	static MainForm* theMainForm;
	
	
	QMdiArea* myMDIArea;
    static TabManager* tabWidget;
    QDesktopWidget* myDesktopWidget;
	
	QApplication* theApp;
	
	VizSelectCombo* windowSelector;

	QLabel* modeStatusWidget;
	
	
	Vcr* vcrPanel;
	

protected slots:
	void modeChange(int);
    virtual void languageChange();
	void initCaptureMenu();
	void setupEditMenu();
	void setInteractiveRefLevel(int);

};
};
#endif // MAINFORM_H

