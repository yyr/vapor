/************************************************************************/
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
/************************************************************************/

//
//	File:		mainform.cpp 
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Implementation of MainForm class
//		This QMainWindow class supports all main window functionality
//		including menus, tab dialog, docking, visualizer window,
//		and some of the communication between these classes
//
#define MIN_WINDOW_WIDTH 700
#define MIN_WINDOW_HEIGHT 700

#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "glutil.h"	// Must be included first!!!
#include <QMenuItem>
#include "mainform.h"
#include <QDockWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QWhatsThis>
#include <QFileDialog>
#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qtooltip.h>
#include <qcheckbox.h>
#include <qaction.h>
#include <qmenubar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qdesktopwidget.h>
#include <qmessagebox.h>
#include <QMdiArea>
#include <qcolordialog.h>
#include <qstatusbar.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qtoolbutton.h>
#include <QPaintEvent>
#include <iostream>
#include <fstream>
#include <sstream>
#include "vizwin.h"

#include "vizselectcombo.h"
#include "tabmanager.h"
#include "viewpointeventrouter.h"
#include "regioneventrouter.h"
#include "animationeventrouter.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "animationparams.h"
#include "assert.h"
#include "vizfeatureparams.h"
#include "mousemodeparams.h"
#include "vapor/ControlExecutive.h"

#include <vapor/DataMgrWB.h>
#include <vapor/DataMgrWC.h>
#include <vapor/MetadataVDC.h>
#include <vapor/Version.h>
//Following shortcuts are provided:
// CTRL_N: new session
// CTRL_O: open session
// CTRL_S: Save current session
// CTRL_D: load data into current session
// CTRL_Z: Undo
// CTRL_Y: Redo
// CTRL_R: Region mode
// CTRL_2: 2D mode
// CTRL_I: Image mode
// CTRL_K: Rake mode
// CTRL_P: Play forward
// CTRL_B: Play backward
// CTRL_F: Step forward
// CTRL_E: Stop (End) play
// CTRL_T: Tile windows
// CTRL_V: View all
// CTRL_H: Home viewpoint


//The following are pixmaps that are used in gui:
#include "images/vapor-icon-32.xpm"
#include "images/cascade.xpm"
#include "images/tiles.xpm"
#include "images/wheel.xpm"

//#include "images/planes.xpm"
//#include "images/lightbulb.xpm"
#include "images/home.xpm"
#include "images/sethome.xpm" 
#include "images/eye.xpm"
#include "images/magnify.xpm"
#include "images/playreverse.xpm"
#include "images/playforward.xpm" 
#include "images/pauseimage.xpm"
#include "images/stepfwd.xpm"
#include "images/stepback.xpm"

/*
 *  Constructs a MainForm as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
using namespace VAPoR;
//Initialize the singleton to 0:
MainForm* MainForm::theMainForm = 0;
TabManager* MainForm::tabWidget = 0;
//Only the main program should call the constructor:
//
MainForm::MainForm(QString& fileName, QApplication* app, QWidget* parent, const char*, Qt::WFlags )
    : QMainWindow( parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	theMainForm = this;
	theApp = app;

	interactiveRefinementSpin = 0;
	modeStatusWidget = 0;
	
	setWindowIcon(QPixmap(vapor_icon___));

    //insert my qmdiArea:
    myMDIArea = new QMdiArea;
    myMDIArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    myMDIArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(myMDIArea);

    createActions();
    createMenus();
    

    //Now let's add a docking tabbed window on the left side.
    tabDockWindow = new QDockWidget(this );
    addDockWidget(Qt::LeftDockWidgetArea, tabDockWindow );
	tabDockWindow->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
	//setup the tab widget

	tabWidget = new TabManager(tabDockWindow, "tab manager");
	tabWidget->setMaximumWidth(600);
	tabWidget->setUsesScrollButtons(true);	
	//This is just large enough to show the whole width of flow tab, with a scrollbar
	//on right (on windows, linux, and irix)  Using default settings of 
	//qtconfig (10 pt font)
	tabWidget->setMinimumWidth(100);
    tabWidget->setMinimumHeight(500);
	
	tabDockWindow->setWidget(tabWidget);
	//Create the Control executive before the VizWinMgr.
	ControlExec::getInstance();
	
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	myVizMgr->createAllDefaultTabs();
	
	
	createToolBars();	
	
	addMouseModes();
    (void)statusBar();
    Main_Form->adjustSize();
    languageChange();
	hookupSignals();   

	//Now that the tabmgr and the viz mgr exist, hook up the tabs:
	
	
	//Create one initial visualizer:
	myVizMgr->launchVisualizer();

	sessionIsDefault = true;
	
    show();
	

	
}

/*
 *  Destroys the object and frees any allocated resources
 */
MainForm::~MainForm()
{
	if (modeStatusWidget) delete modeStatusWidget;
   
	
    // no need to delete child widgets, Qt does it all for us?? (see closeEvent)
}

void MainForm::createToolBars(){
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
    // mouse mode toolbar:
    modeToolBar = addToolBar("Mouse Modes"); 
	modeToolBar->addWidget(new QLabel(" Modes: "));
	QString qws = QString("The mouse modes are used to enable various manipulation tools ")+
		"that can be used to control the location and position of objects in "+
		"the 3D scene, by dragging box-handles in the scene. "+
		"Select the desired mode from the pull-down menu,"+
		"or revert to the default (Navigation) by clicking the button";
	modeToolBar->setWhatsThis(qws);
	//add mode buttons, left to right:
	modeToolBar->addAction(navigationAction);
	
	modeCombo = new QComboBox(this);
	modeCombo->setToolTip("Select the mouse mode to use in the visualizer");
	
	modeToolBar->addWidget(modeCombo);
	

	
// Animation Toolbar:
	animationToolBar = addToolBar("animation control");
	QIntValidator *v = new QIntValidator(0,99999,animationToolBar);
	timeStepEdit = new QLineEdit(animationToolBar);
	timeStepEdit->setAlignment(Qt::AlignHCenter);
	timeStepEdit->setMaximumWidth(40);
	timeStepEdit->setToolTip( "Edit/Display current time step");
	timeStepEdit->setValidator(v);
	animationToolBar->addWidget(timeStepEdit);
	
	animationToolBar->addAction(playBackwardAction);
	animationToolBar->addAction(stepBackAction);
	animationToolBar->addAction(pauseAction);
	animationToolBar->addAction(stepForwardAction);
	animationToolBar->addAction(playForwardAction);
	
	QString qat = QString("The animation toolbar enables control of the time steps ")+
		"in the current active visualizer.  Additional controls are available in"+
		"the animation tab ";
	animationToolBar->setWhatsThis(qat);
	
// Viz tool bar:
	vizToolBar = addToolBar("");

	//Add a QComboBox to toolbar to select window
	windowSelector = new VizSelectCombo(this, vizToolBar, myVizMgr);

	vizToolBar->addAction(tileAction);
	vizToolBar->addAction(cascadeAction);
	vizToolBar->addAction(homeAction);
	vizToolBar->addAction(sethomeAction);
	vizToolBar->addAction(viewRegionAction);
	vizToolBar->addAction(viewAllAction);


	alignViewCombo = new QComboBox(vizToolBar);
	alignViewCombo->addItem("Align View");
	alignViewCombo->addItem("Nearest axis");
	alignViewCombo->addItem("     + X ");
	alignViewCombo->addItem("     + Y ");
	alignViewCombo->addItem("     + Z ");
	alignViewCombo->addItem("     - X ");
	alignViewCombo->addItem("     - Y ");
	alignViewCombo->addItem("Default: - Z ");
	alignViewCombo->setToolTip( "Rotate view to an axis-aligned viewpoint,\ncentered on current rotation center.");
	
	vizToolBar->addWidget(alignViewCombo);
	interactiveRefinementSpin = new QSpinBox(vizToolBar);
	interactiveRefinementSpin->setPrefix(" Interactive Refinement: ");
	interactiveRefinementSpin->setMinimumWidth(230);
	
	interactiveRefinementSpin->setToolTip(
		"Specify minimum refinement level during mouse motion,\nused in DVR and Iso rendering");
	interactiveRefinementSpin->setWhatsThis(
		QString("While the viewpoint is changing due to mouse-dragging ")+
		"in a visualizer, the refinement level used by the DVR "+
		"and Iso renderers is reduced to this interactive refinement level, "+
		"if it is less than the current refinement level of the renderer.");
	interactiveRefinementSpin->setMinimum(0);
	interactiveRefinementSpin->setMaximum(10);
	
	vizToolBar->addWidget(interactiveRefinementSpin);
}
void MainForm::hookupSignals() {
   
	connect(modeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT( modeChange(int)));
	connect( fileNew_SessionAction, SIGNAL( triggered() ), this, SLOT( newSession() ) );
	connect( fileOpenAction, SIGNAL( triggered() ), this, SLOT( fileOpen() ) );
	connect( fileSaveAction, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
	connect( fileSaveAsAction, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );
	connect( fileExitAction, SIGNAL( triggered() ), this, SLOT( fileExit() ) );
	connect( loadPreferencesAction, SIGNAL( triggered() ), this, SLOT( loadPrefs() ) );
	connect( savePreferencesAction, SIGNAL( triggered() ), this, SLOT( savePrefs() ) );
	connect(editUndoAction, SIGNAL(triggered()), this, SLOT (undo()));
	connect(editRedoAction, SIGNAL(triggered()), this, SLOT (redo()));
	connect( editVizFeaturesAction, SIGNAL(triggered()), this, SLOT(launchVizFeaturesPanel()));
	connect( editPreferencesAction, SIGNAL(triggered()), this, SLOT(launchPreferencesPanel()));
	
    connect( helpAboutAction, SIGNAL( triggered() ), this, SLOT( helpAbout() ) );
    
	
	connect( dataLoad_MetafileAction, SIGNAL( triggered() ), this, SLOT( loadData() ) );
	connect( dataLoad_DefaultMetafileAction, SIGNAL( triggered() ), this, SLOT( defaultLoadData() ) );
	connect(captureMenu, SIGNAL(aboutToShow()), this, SLOT(initCaptureMenu()));
   
	connect(Edit, SIGNAL(aboutToShow()), this, SLOT (setupEditMenu()));
	
	connect( captureStartJpegCaptureAction, SIGNAL( triggered() ), this, SLOT( startJpegCapture() ) );
	connect( captureEndJpegCaptureAction, SIGNAL( triggered() ), this, SLOT( endJpegCapture() ) );
	connect (captureSingleJpegCaptureAction, SIGNAL(triggered()), this, SLOT (captureSingleJpeg()));
	connect( captureStartFlowCaptureAction, SIGNAL( triggered() ), this, SLOT( startFlowCapture() ) );
	connect( captureEndFlowCaptureAction, SIGNAL( triggered() ), this, SLOT( endFlowCapture() ) );
    

	//Toolbar actions:
	connect (navigationAction, SIGNAL(toggled(bool)), this, SLOT(setNavigate(bool)));
	
	connect (interactiveRefinementSpin, SIGNAL(valueChanged(int)), this, SLOT(setInteractiveRefLevel(int)));
	connect (playForwardAction, SIGNAL(triggered()), this, SLOT(playForward()));
	connect (playBackwardAction, SIGNAL(triggered()), this, SLOT(playBackward()));
	connect (pauseAction, SIGNAL(triggered()), this, SLOT(pauseClick()));
	connect (stepForwardAction, SIGNAL(triggered()), this, SLOT(stepForward()));
	connect (stepBackAction, SIGNAL(triggered()), this, SLOT(stepBack()));
	connect (timeStepEdit, SIGNAL(editingFinished()),this, SLOT(setTimestep()));
}

void MainForm::createMenus(){
    // menubar
    Main_Form = menuBar();
    File = menuBar()->addMenu(tr("File"));
    File->addAction(fileNew_SessionAction);
    File->addAction(fileOpenAction);
    File->addAction(fileSaveAction);
    File->addAction(fileSaveAsAction);
	File->addAction(loadPreferencesAction);
	File->addAction(savePreferencesAction);
    File->addAction(fileExitAction);
    Edit = menuBar()->addMenu(tr("Edit"));

	Edit->addAction(editUndoAction);
	Edit->addAction(editRedoAction);
	Edit->addAction(editVizFeaturesAction);
	Edit->addAction(editPreferencesAction);
	Edit->addSeparator();

	editPythonMenu = new QMenu("Edit script defining variable");
	Edit->addAction(newPythonAction);
	Edit->addMenu(editPythonMenu);
	Edit->addAction(editPythonStartupAction);
	
	

    Data = menuBar()->addMenu(tr("Data"));
    
	
    Data->addAction(dataLoad_MetafileAction );
    Data->addAction(dataLoad_DefaultMetafileAction );
    Data->addAction(dataImportWRF_Action);
    Data->addAction(dataImportDefaultWRF_Action);
	Data->addAction(dataMerge_MetafileAction);
	Data->addAction(dataSave_MetafileAction);
	
	Data->addAction(dataExportToIDLAction);
    
	Main_Form->addMenu(Data);

    
	

	//Note that the ordering of the following 4 is significant, so that image
	//capture actions correctly activate each other.
    captureMenu = menuBar()->addMenu(tr("Capture"));
	
    captureMenu->addAction(captureSingleJpegCaptureAction);
	captureMenu->addAction(captureStartJpegCaptureAction);
	captureMenu->addAction(captureEndJpegCaptureAction);
	captureMenu->addAction(captureStartFlowCaptureAction);
	captureMenu->addAction(captureEndFlowCaptureAction);

    
    Main_Form->addSeparator();

    helpMenu = menuBar()->addMenu(tr("Help"));

    helpMenu->addAction(whatsThisAction);
    helpMenu->addSeparator();
    helpMenu->addAction(helpAboutAction );
}

void MainForm::createActions(){
    // first do actions for menu bar:
    
    fileOpenAction = new QAction( this);
	fileOpenAction->setEnabled(true);
    fileSaveAction = new QAction( this );
	fileSaveAction->setEnabled(true);
    fileSaveAsAction = new QAction( this );
	fileSaveAsAction->setEnabled(true);
    fileExitAction = new QAction( this);
	loadPreferencesAction = new QAction(this);
	loadPreferencesAction->setEnabled(true);
	savePreferencesAction = new QAction(this);
	savePreferencesAction->setEnabled(true);

	editUndoAction = new QAction(this);
	editRedoAction = new QAction(this);
	
	editVizFeaturesAction = new QAction(this);
	editPreferencesAction = new QAction(this);

	newPythonAction = new QAction(this);
	editPythonStartupAction = new QAction(this);
	
	editUndoAction->setEnabled(true);
	editRedoAction->setEnabled(true);
    
	whatsThisAction = QWhatsThis::createAction(this);

    helpAboutAction = new QAction( this );
	helpAboutAction->setEnabled(true);
    
   
    dataConfigure_MetafileAction = new QAction( this );
	dataConfigure_MetafileAction->setEnabled(false);
    dataLoad_MetafileAction = new QAction( this);
	dataMerge_MetafileAction = new QAction( this );
	dataImportWRF_Action = new QAction( this );
	dataImportDefaultWRF_Action = new QAction( this );
	dataSave_MetafileAction = new QAction( this );
	dataLoad_DefaultMetafileAction = new QAction(this);
	fileNew_SessionAction = new QAction( this );
	dataExportToIDLAction = new QAction(this);
    
    
    
	newPythonAction = new QAction(this);
	

	captureStartJpegCaptureAction = new QAction( this );
	captureEndJpegCaptureAction = new QAction( this);
	captureSingleJpegCaptureAction = new QAction(this);
	captureStartJpegCaptureAction = new QAction( this );
	captureEndJpegCaptureAction = new QAction( this );
	captureSingleJpegCaptureAction = new QAction(this);
	captureStartFlowCaptureAction = new QAction( this );
	captureEndFlowCaptureAction = new QAction( this);

	//Then do the actions for the toolbars:
	//Create an exclusive action group for the mouse mode toolbar:
	mouseModeActions = new QActionGroup(this);
	//Toolbar buttons:
	QPixmap* wheelIcon = new QPixmap(wheel);
	
	
	navigationAction = new QAction(*wheelIcon,"Navigation Mode",mouseModeActions);
	navigationAction->setCheckable(true);
	navigationAction->setChecked(true);


	//Actions for the viztoolbar:
	QPixmap* homeIcon = new QPixmap(home);
	homeAction = new QAction(*homeIcon,QString(tr("Go to Home Viewpoint  "))+QString(QKeySequence(tr("Ctrl+H"))), this);
	homeAction->setShortcut(Qt::CTRL+Qt::Key_H);
	QPixmap* sethomeIcon = new QPixmap(sethome);
	sethomeAction = new QAction(*sethomeIcon, "Set Home Viewpoint", this);
	QPixmap* eyeIcon = new QPixmap(eye);
	viewAllAction = new QAction(*eyeIcon,QString(tr("View All  "))+QString(QKeySequence(tr("Ctrl+V"))), this);
	viewAllAction->setShortcut(Qt::CTRL+Qt::Key_V);
	QPixmap* magnifyIcon = new QPixmap(magnify);
	viewRegionAction = new QAction(*magnifyIcon, "View Region", this);

	QPixmap* tileIcon = new QPixmap(tiles);
	tileAction = new QAction(*tileIcon,QString(tr("Tile Windows  "))+QString(QKeySequence(tr("Ctrl+T"))),this);
	tileAction->setShortcut(Qt::CTRL+Qt::Key_T);

	QPixmap* cascadeIcon = new QPixmap(cascade);
	cascadeAction = new QAction(*cascadeIcon, "Cascade Windows", this);


	//Create actions for each animation control button:
	QPixmap* playForwardIcon = new QPixmap(playforward);
	playForwardAction = new QAction(*playForwardIcon,QString(tr("Play Forward  "))+QString(QKeySequence(tr("Ctrl+P"))) , this);
	playForwardAction->setShortcut(Qt::CTRL+Qt::Key_P);
	playForwardAction->setCheckable(true);
	QPixmap* playBackwardIcon = new QPixmap(playreverse);
	playBackwardAction = new QAction(*playBackwardIcon,QString(tr("Play Backward  "))+QString(QKeySequence(tr("Ctrl+B"))),this);
	playBackwardAction->setShortcut(Qt::CTRL+Qt::Key_B);
	playBackwardAction->setCheckable(true);
	QPixmap* pauseIcon = new QPixmap(pauseimage);
	pauseAction = new QAction(*pauseIcon,QString(tr("End animation and unsteady flow integration  "))+QString(QKeySequence(tr("Ctrl+E"))), this);
	pauseAction->setShortcut(Qt::CTRL+Qt::Key_E);
	pauseAction->setCheckable(true);
	QPixmap* stepForwardIcon = new QPixmap(stepfwd);
	stepForwardAction = new QAction(*stepForwardIcon,QString(tr("Step forward  "))+QString(QKeySequence(tr("Ctrl+F"))), this);
	stepForwardAction->setShortcut(Qt::CTRL+Qt::Key_F);
	QPixmap* stepBackIcon = new QPixmap(stepback);
	stepBackAction = new QAction(*stepBackIcon,"Step back",this);
}
/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void MainForm::languageChange()
{
	setWindowTitle( tr( "VAPoR:  NCAR Visualization and Analysis Platform for Research" ) );

    fileNew_SessionAction->setText( tr( "New Session" ) );
    
	fileNew_SessionAction->setToolTip("Restart the session with default settings");
	fileNew_SessionAction->setShortcut( Qt::CTRL + Qt::Key_N );
    
    fileOpenAction->setText( tr( "&Open Session" ) );
   
    fileOpenAction->setShortcut( tr( "Ctrl+O" ) );
	fileOpenAction->setToolTip("Launch a file open dialog to reopen a previously saved session file");
    
    fileSaveAction->setText( tr( "&Save Session" ) );
    fileSaveAction->setShortcut( tr( "Ctrl+S" ) );
	fileSaveAction->setToolTip("Launch a file-save dialog to save the state of this session in current session file");
    fileSaveAsAction->setText( tr( "Save Session As" ) );
    
	fileSaveAsAction->setToolTip("Launch a file-save dialog to save the state of this session in another session file");
    
	loadPreferencesAction->setText( tr( "Load Preferences" ) );
    
	loadPreferencesAction->setToolTip("Load the user preferences from a specified file");
    
    savePreferencesAction->setText( tr( "Save Preferences" ) );
    
	savePreferencesAction->setToolTip("Save the user preferences to a specified file");
    
    fileExitAction->setText( tr( "E&xit" ) );

	editUndoAction->setText( tr( "&Undo" ) );
    
    editUndoAction->setShortcut( tr( "Ctrl+Z" ) );
	editUndoAction->setToolTip("Undo the most recent session state change");
	editRedoAction->setText( tr( "&Redo" ) );
    
    editRedoAction->setShortcut( tr( "Ctrl+Y" ) );
	editRedoAction->setToolTip("Redo the last undone session state change");
    
	
    helpAboutAction->setText( tr( "About VAPOR" ) );
    
    helpAboutAction->setToolTip( tr( "Information about VAPOR" ) );

	whatsThisAction->setText( tr( "Explain This" ) );
    
	whatsThisAction->setToolTip(tr("Click here, then click over an object for context-sensitive help. "));

	
	dataExportToIDLAction->setText(tr("Export to IDL"));
	
	dataExportToIDLAction->setToolTip("Export current data settings to enable IDL access");
   
    dataConfigure_MetafileAction->setText( tr( "Configure Metafile" ) );
    
	dataConfigure_MetafileAction->setToolTip("Launch a tool to construct the metafile associated with a dataset");
    dataLoad_MetafileAction->setText( tr( "Load a Dataset into Current Session" ) );
   
	dataLoad_MetafileAction->setToolTip("Specify a data set to be loaded into current session");
	dataLoad_MetafileAction->setShortcut(tr("Ctrl+D"));
	dataLoad_DefaultMetafileAction->setText( tr( "Load a Dataset into &Default Session" ) );
  
	dataLoad_DefaultMetafileAction->setToolTip("Specify a data set to be loaded into a new session with default settings");
	
	
	dataImportDefaultWRF_Action->setText(tr("Import WRF-ARW output files into default session"));
	dataImportDefaultWRF_Action->setToolTip("Specify one or more WRF-ARW output files to import into a new session");
	dataImportWRF_Action->setText(tr("Import WRF-ARW output files into current session"));
	dataImportWRF_Action->setToolTip("Specify one or more WRF-ARW output files to import into the current session");
	dataMerge_MetafileAction->setText( tr( "Merge a VDC Dataset into Current Session" ) );
	
    
	dataMerge_MetafileAction->setToolTip("Specify a data set to be merged into current session");
	
	dataSave_MetafileAction->setText( tr( "Save the current Metadata to file" ) );
    
	dataSave_MetafileAction->setToolTip("Specify a file where the current Metadata will be saved");


	editVizFeaturesAction->setText(tr("Edit Visualizer Features"));
	
	editVizFeaturesAction->setToolTip(tr("View or change various visualizer settings"));

	editPreferencesAction->setText(tr("Edit User Preferences "));
	
	editPreferencesAction->setToolTip(tr("View or change various user preference settings"));
	editPythonMenu->setTitle(tr("Edit Python Program defining existing variable"));
	editPythonMenu->setToolTip(tr("Edit the Python program that defines an existing variable"));
	newPythonAction->setText(tr("Edit Python program defining a new variable"));
	newPythonAction->setToolTip(tr("Edit Python program defining a new variable"));
	editPythonStartupAction->setText(tr("Edit Python startup script"));
	editPythonStartupAction->setToolTip(tr("Edit Python script that will be executed at Python startup"));

	captureStartJpegCaptureAction->setText( tr( "Begin image capture sequence " ) );
    
	captureStartJpegCaptureAction->setToolTip("Begin saving jpeg image files rendered in current active visualizer");
	
	captureEndJpegCaptureAction->setText( tr( "End image capture" ) );
   
	captureEndJpegCaptureAction->setToolTip("End capture of image files in current active visualizer");

	captureSingleJpegCaptureAction->setText( tr( "Single image capture" ) );
    
	captureSingleJpegCaptureAction->setToolTip("Capture one image from current active visualizer");

	captureStartFlowCaptureAction->setText( tr( "Begin flow capture sequence " ) );
    
	captureStartFlowCaptureAction->setToolTip("Begin saving flow lines in current active visualizer");
	
	captureEndFlowCaptureAction->setText( tr( "End flow capture" ) );
   
	captureEndFlowCaptureAction->setToolTip("End capture of flow lines in current active visualizer");

	
   
    vizToolBar->setWindowTitle( tr( "VizTools" ) );
	modeToolBar->setWindowTitle( tr( "Mouse Modes" ) );
   
}

//Open session file
void MainForm::fileOpen()

{

	//This launches a panel that enables the
    //user to choose input session save files, then to
	//load that session
	
	QString fn = "VaporSaved.vss";
	QString qfilename = QFileDialog::getOpenFileName(this, 
		"Choose a VAPOR session file to restore a session",
		fn,
		"Vapor Session Save Files (*.vss)");
	if(qfilename.length() == 0) return;
		
	//Force the name to end with .vss
	if (!qfilename.endsWith(".vss")){
		qfilename += ".vss";
	}
	string filename = qfilename.toStdString();
	ControlExec::getInstance()->RestoreSession(filename);
	sessionIsDefault = false;
	//Now need to set up visualizer(s)
}


void MainForm::fileSave()
{
	DataMgr *dataMgr = DataStatus::getInstance()->getDataMgr();

	//This directly saves the session to the current session save file.
    	//It does not prompt the user unless there is an error
	if (! dataMgr) {
		//MessageReporter::warningMsg( "There is no current metadata.  \nSession state cannot be saved");
		return;
	}
	
   	QString filename = QFileDialog::getSaveFileName(this,
		"Choose the filename to save the current session",
		".",
		"Vapor Session Files (*.vss)");
	
	string s = filename.toStdString();
	
	
	
	if (!ControlExec::getInstance()->SaveSession(s)){//Report error if can't save to file
		//MessageReporter::errorMsg("Failed to write session file: \n%s", s.c_str());
		return;
	}
}


void MainForm::fileSaveAs()
{
	const DataMgr *dataMgr = ControlExec::GetDataMgr();

	if (! dataMgr) {
		//MessageReporter::warningMsg( "There is no current metadata.  \nSession state cannot be saved");
		return;
	}
	
   	QString filename = QFileDialog::getSaveFileName(this,
		"Choose the filename to save the current session",
		".",
		"Vapor Session Files (*.vss)");
	
	string s = filename.toStdString();
	
	
	
	if (!ControlExec::getInstance()->SaveSession(s)){//Report error if can't save to file
		//MessageReporter::errorMsg("Failed to write session file: \n%s", s.c_str());
		return;
	}
}



void MainForm::fileExit()
{
	delete ControlExec::getInstance();
	close();
}

void MainForm::undo(){
	ControlExec* ce = ControlExec::getInstance();
	//If it's not active, just set default text:
	if (!ce->CommandExists(0)) return; 
	const Params* p = ce->Undo();
	tabWidget->getFrontEventRouter()->updateTab();
	VizWinMgr::getInstance()->forceRender(p,true);
}

void MainForm::redo(){
	ControlExec* ce = ControlExec::getInstance();
	//If it's not active, just set default text:
	if (!ce->CommandExists(-1)) return; 
	const Params* p = ce->Redo();
	tabWidget->getFrontEventRouter()->updateTab();
	VizWinMgr::getInstance()->forceRender(p,true);
}



void MainForm::helpIndex()
{

}


void MainForm::helpContents()
{

}


void MainForm::helpAbout()
{
	QString versionInfo(QString("Visualization and Analysis Platform for atmospheric, Oceanic and solar Research\n") + 
		QString("Developed at the National Center for Atmospheric Research (NCAR), Boulder, Colorado 80305, U.S.A.\nWeb site: http://www.vapor.ucar.edu\n")+
		QString("Contact: vapor@ucar.edu\n")+
		QString("Version: ")+
		Version::GetVersionString().c_str());

	QMessageBox::information(this, "Information about VAPOR",(const char*)versionInfo.toAscii());

}
void MainForm::batchSetup(){
    //Here we provide panel to setup batch runs
}



void MainForm::loadPrefs(){
	
	
}
void MainForm::savePrefs(){
	
}
//Load data into current session
//If current session is at default then same as loadDefaultData
//
void MainForm::loadData()
{
	//This launches a panel that enables the
    	//user to choose input data files, then to
	//create a datamanager using those files
    	//or metafiles.  
	
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose the Metadata File to load into current session",
		"",
		"Vapor Metadata Files (*.vdf)");
	
	if(filename != QString::null){
		QFileInfo fInfo(filename);
		const DataMgr* dmgr=0;
		if (fInfo.isReadable() && fInfo.isFile()){
			vector<string> files;
			files.push_back(filename.toStdString());
			dmgr = ControlExec::getInstance()->LoadData(files,sessionIsDefault);
			// Reinitialize all tabs
			//
			if (dmgr){
				sessionIsDefault=false;
				int numParamsTabs = ControlExec::GetNumTabParamsClasses();
				for (int pType = 1; pType <= numParamsTabs; pType++){
					EventRouter* eRouter = VizWinMgr::getInstance()->getEventRouter(pType);
					eRouter->reinitTab(false);
				}

			}
		}
		else {
			QMessageBox::information(this,"Load Data Error","Unable to read metadata file ");
			return;
		}
		if (dmgr) return;
	}
	QMessageBox::information(this,"Load Data Error","Invalid VDC");

}

//import WRF data into current session
//
void MainForm::importWRFData()
{
/*
	//This launches a panel that enables the
    //user to choose input WRF output files, then to
	//use them to create a new data
	
	QStringList filenames = QFileDialog::getOpenFileNames(this,
		"Select WRF-ARW Output Files to import into current session",
		Session::getInstance()->getMetadataFile().c_str(),"");

	if (filenames.length() > 0){
		//Create a string vector from the QStringList
		vector<string> files;
		QStringList list = filenames;
		QStringList::Iterator it = list.begin();
		while(it != list.end()) {
			files.push_back((*it).toStdString());
			++it;
		}
		Session::getInstance()->resetMetadata(files, true, true);
		DataStatus::setPre22Session(false);
	
	} else MessageReporter::errorMsg("No valid WRF files \n");
	*/
}
//import WRF data into default session
void MainForm::importDefaultWRFData()
{
/*
	//This launches a panel that enables the
    //user to choose input WRF output files, then to
	//use them to create a new data
	QStringList filenames = QFileDialog::getOpenFileNames(this,
		"Select WRF-ARW Output Files to import into default session",
		Session::getInstance()->getMetadataFile().c_str(),"");
	
	if (filenames.length() > 0){
		//Create a string vector from the QStringList
		vector<string> files;
		//reset to default session:
		Session::getInstance()->resetMetadata(files, false, false);
		QStringList list = filenames;
		QStringList::Iterator it = list.begin();
		while(it != list.end()) {
			files.push_back((*it).toStdString());
			++it;
		}
		Session::getInstance()->resetMetadata(files, false, true);
	
	} else MessageReporter::errorMsg("No valid WRF files \n");
	*/
}
//Load data into default session
//
void MainForm::defaultLoadData()
{
	//This launches a panel that enables the
    //user to choose input data files, then to
	//create a datamanager using those files
    //with default settings
	
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose the Metadata File to load into default session",
		"",
		"Vapor Metadata Files (*.vdf)");
	
	if(filename != QString::null){
		QFileInfo fInfo(filename);
		const DataMgr* dmgr=0;
		if (fInfo.isReadable() && fInfo.isFile()){
			vector<string> files;
			files.push_back(filename.toStdString());
			dmgr = ControlExec::getInstance()->LoadData(files,true);
			// Reinitialize all tabs
			//
			if (dmgr){
				sessionIsDefault = false;
				int numParamsTabs = ControlExec::GetNumTabParamsClasses();
				for (int pType = 1; pType <= numParamsTabs; pType++){
					EventRouter* eRouter = VizWinMgr::getInstance()->getEventRouter(pType);
					eRouter->reinitTab(true);
				}
			}
		}
		else {
			QMessageBox::information(this,"Load Data Error","Unable to read metadata file ");
			return;
		}
		if (dmgr) return;
	}
	QMessageBox::information(this,"Load Data Error","Invalid VDC");
}
void MainForm::newSession()
{
	
	
}
void MainForm::launchVisualizer()
{
	Command::blockCapture();
	VizWinMgr::getInstance()->launchVisualizer();
	Command::unblockCapture();
		
}


/*
 * Respond to toolbar clicks:
 * navigate mode.  Don't change tab menu
 */
void MainForm::setNavigate(bool on)
{
if (!on) return;
	
	//Only do something if this is an actual change of mode
	if (MouseModeParams::GetCurrentMouseMode() != MouseModeParams::navigateMode){
		
		MouseModeParams::SetCurrentMouseMode(MouseModeParams::navigateMode);
		modeCombo->setCurrentIndex(0);
		
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("Navigation Mode:  Use left mouse to rotate or spin-animate, right to zoom, middle to translate",this);
		statusBar()->addWidget(modeStatusWidget,2);
	}	
}


void MainForm::setupEditMenu(){
	
	QString undoText("Undo ");
	QString redoText("Redo ");
	ControlExec* ce = ControlExec::getInstance();
	//If it's not active, just set default text:
	if (editUndoAction->isEnabled() && ce->CommandExists(0)) {
		undoText += ce->GetCommandText(0).c_str();
	}
	if (editRedoAction->isEnabled() && ce->CommandExists(-1)) {
		redoText +=  ce->GetCommandText(-1).c_str();
	}
	editUndoAction->setText( undoText );
	editRedoAction->setText( redoText );
}
//Enable or disable the View menu options:
void MainForm::initCaptureMenu(){
	
}

//Make all the current region/animation settings available to IDL
void MainForm::exportToIDL(){
	
}
	
//Begin capturing images.
//Launch a file save dialog to specify the names
//Then start file saving mode.
void MainForm::startJpegCapture() {
	
}
//Begin capturing flow.
//Launch a file save dialog to specify the names
//Then start file saving mode.
void MainForm::startFlowCapture() {
	
}

//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void MainForm::captureSingleJpeg() {
	
}
void MainForm::endJpegCapture(){
	
}
void MainForm::endFlowCapture(){
	
}
void MainForm::launchVizFeaturesPanel(){
	VizFeatureParams vFP;
	vFP.launch();
}
void MainForm::launchPreferencesPanel(){
	
}
void MainForm::editPythonStartup(){
	
}
void MainForm::newPythonEditor(){
	
}
void MainForm::launchPythonEditor(QAction* act){
	
}
void MainForm::setInteractiveRefLevel(int val){
	
}
void MainForm::setInteractiveRefinementSpin(int val){
	
}
	
void MainForm::pauseClick(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::_animationParamsTag);
	aRouter->animationPauseClick();
}
void MainForm::playForward(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::_animationParamsTag);
	aRouter->animationPlayForwardClick();
}
void MainForm::playBackward(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::_animationParamsTag);
	aRouter->animationPlayReverseClick();
}
void MainForm::stepBack(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::_animationParamsTag);
	aRouter->animationStepReverseClick();
}	
void MainForm::stepForward(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::_animationParamsTag);
	aRouter->animationStepForwardClick();
}	
//Respond to a change in the text in the animation toolbar
void MainForm::setTimestep(){
	
}
//Set the timestep in the animation toolbar:
void MainForm::setCurrentTimestep(int tstep){
	timeStepEdit->setText(QString::number(tstep));
	update();
}
void MainForm::enableKeyframing(bool ison){
	QPalette pal(timeStepEdit->palette());
	timeStepEdit->setEnabled(!ison);
}
void MainForm::paintEvent(QPaintEvent* e){
	
}
void MainForm::showTab(const std::string& tag){
	ParamsBase::ParamsBaseType t = ControlExec::GetTypeFromTag(tag);
	tabWidget->moveToFront(t);
	EventRouter* eRouter = VizWinMgr::getEventRouter(tag);
	eRouter->updateTab();
}
void MainForm::modeChange(int newmode){
	if (newmode == 0) {
		navigationAction->setChecked(true);
		return;
	}
	
	navigationAction->setChecked(false);
	showTab(ControlExec::GetTagFromType(MouseModeParams::getModeParamType(newmode)));

	MouseModeParams::SetCurrentMouseMode((MouseModeParams::mouseModeType)newmode);
	
	if(modeStatusWidget) {
		statusBar()->removeWidget(modeStatusWidget);
		delete modeStatusWidget;
	}

	modeStatusWidget = new QLabel(QString::fromStdString(MouseModeParams::getModeName(newmode))+" Mode: To modify box in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
	statusBar()->addWidget(modeStatusWidget,2);
	
}
void MainForm::showCitationReminder(){
	

}
void MainForm::addMouseModes(){
	for (int i = 0; i<MouseModeParams::getNumMouseModes(); i++){
		QString text = QString::fromStdString(MouseModeParams::getModeName(i));
		const char* const * xpmIcon = MouseModeParams::GetIcon(i);
		QPixmap qp = QPixmap(xpmIcon);
		QIcon icon = QIcon(qp);
		addMode(text,icon);
	}
}