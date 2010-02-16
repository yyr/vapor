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
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include <QMenuItem>
#include "GL/glew.h"
#include "mainform.h"
#include <QDockWidget>
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
#include "dvreventrouter.h"
#include "isoeventrouter.h"
#include "setoffset.h"
#include "vizwin.h"
#include "floweventrouter.h"
#include "probeeventrouter.h"
#include "twoDdataeventrouter.h"
#include "twoDimageeventrouter.h"
#include "vizselectcombo.h"
#include "tabmanager.h"
#include "viewpointeventrouter.h"
#include "regioneventrouter.h"
#include "vizwinmgr.h"
#include "flowparams.h"
#include "animationeventrouter.h"
#include "session.h"
#include "messagereporter.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "dvrparams.h"
#include "ParamsIso.h"

#include "animationparams.h"
#include "probeparams.h"
#include "twoDdataparams.h"
#include "twoDimageparams.h"

#include "assert.h"
#include "command.h"

#include "vizfeatureparams.h"
#include "userpreferences.h"

#include <vapor/DataMgrWB.h>
#include <vapor/DataMgrLayered.h>
#include <vapor/MetadataVDC.h>
#include "vapor/Version.h"
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
#include "images/probe.xpm"
#include "images/twoDData.xpm"
#include "images/twoDImage.xpm"
#include "images/rake.xpm"
#include "images/wheel.xpm"
#include "images/cube.xpm"
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

//Only the main program should call the constructor:
//
MainForm::MainForm(QString& fileName, QApplication* app, QWidget* parent, const char*, Qt::WFlags )
    : QMainWindow( parent)
{
	setAttribute(Qt::WA_DeleteOnClose);
	theMainForm = this;
	theRegionTab = 0;
	theDvrTab = 0;
	theIsoTab = 0;
	theVizTab = 0;
	theAnimationTab = 0;
	theFlowTab = 0;
	theProbeTab = 0;
	theTwoDDataTab = 0;
	theTwoDImageTab = 0;
	theApp = app;

	interactiveRefinementSpin = 0;
	modeStatusWidget = 0;
	
	setWindowIcon(QPixmap(vapor_icon___));

    //insert my qmdiArea:
    myMDIArea = new QMdiArea;
    myMDIArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    myMDIArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(myMDIArea);

    
    setMinimumSize( QSize( 422, 606 ) );
   
   
    createActions();
    createMenus();
    

    //Now let's add a docking tabbed window on the left side.
    tabDockWindow = new QDockWidget(this );
    addDockWidget(Qt::LeftDockWidgetArea, tabDockWindow );
	tabDockWindow->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
	//setup the tab widget

	tabWidget = new TabManager(tabDockWindow, "tab manager");
	tabWidget->setMaximumWidth(700);
	
	//This is just large enough to show the whole width of flow tab, with a scrollbar
	//on right (on windows, linux, and irix)  Using default settings of 
	//qtconfig (10 pt font)
	tabWidget->setMinimumWidth(100);
    tabWidget->setMinimumHeight(500);
	
	
	
	tabDockWindow->setWidget(tabWidget);
	
	//Setup  the session (hence the viz window manager)
	Session::getInstance();
	MessageReporter::infoMsg("MainForm::MainForm(): setup session");
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	
	myVizMgr->createGlobalParams();
	
	createToolBars();	
	
    (void)statusBar();
    	Main_Form->adjustSize();
    	languageChange();
	hookupSignals();   

	//Now that the tabmgr and the viz mgr exist, hook up the tabs:
	
	//These need to be installed to set the tab pointers in the
	//global params.
	Session::getInstance()->blockRecording();
	

	//the following determines the tab ordering from left to right:
	viewpoint();
	animationParams();
	region();
	launchTwoDDataTab();
	launchTwoDImageTab();
	launchProbeTab();
	launchIsoTab();
	launchFlowTab();
	//The last one is in front:
	renderDVR();

	
	//Create one initial visualizer:
	myVizMgr->launchVisualizer();

	Session::getInstance()->unblockRecording();
	
    	show();
	VizWinMgr::getInstance()->getDvrRouter()->initTypes();

	if(fileName != ""){
		if (fileName.endsWith(".vss")){

			ifstream is;
			is.open((const char*)fileName.toAscii());
			if (!is){//Report error if you can't open the file
				MessageReporter::errorMsg("Unable to open session file: \n%s", (const char*)fileName.toAscii());
				return;
			}
			//Remember file if load is successful:
			if(Session::getInstance()->loadFromFile(is)){
				QString sessionSaveFile = fileName;
				QFileInfo fi(fileName);
				Session::getInstance()->setSessionFilepath((const char*)sessionSaveFile.toAscii());
			}
		} else if (fileName.endsWith(".vdf")){
#ifdef WIN32
			//Windows:  Need to reverse the backwards slashes, so that
			//DataMgr can deal with this path
			fileName.replace('\\','/');
#endif
			Session::getInstance()->resetMetadata((const char*)fileName.toAscii(), false);
		}
	}
	MessageReporter::infoMsg("MainForm::MainForm() end");
}

/*
 *  Destroys the object and frees any allocated resources
 */
MainForm::~MainForm()
{
	if (modeStatusWidget) delete modeStatusWidget;
    //qWarning("mainform destructor start");
	delete Session::getInstance();
    // no need to delete child widgets, Qt does it all for us?? (see closeEvent)
}

void MainForm::createToolBars(){
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
    // mouse mode toolbar:
    	modeToolBar = addToolBar(""); 
//	modeToolBar->setAllowedAreas(Qt::TopToolBarArea);
	QString qws = QString("The mode buttons are used to enable various manipulation tools ")+
		"that can be used to control the location and position of objects in "+
		"the 3D scene, by dragging with the mouse in the scene.  These include "+
		"navigation, region, rake, probe, 2D, and Image tools.";
	modeToolBar->setWhatsThis(qws);
	//add mode buttons, left to right:
	modeToolBar->addAction(navigationAction);
	modeToolBar->addAction(regionSelectAction);
	modeToolBar->addAction(rakeAction);
	modeToolBar->addAction(probeAction);
	modeToolBar->addAction(twoDDataAction);
	modeToolBar->addAction(twoDImageAction);
	

// Animation Toolbar:
	animationToolBar = addToolBar("animation control");
	timestepEdit = new QLineEdit(animationToolBar);
	timestepEdit->setAlignment(Qt::AlignHCenter);
	timestepEdit->setMaximumWidth(40);
	timestepEdit->setToolTip( "Edit/Display current time step");
	animationToolBar->addWidget(timestepEdit);
	
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
//	vizToolBar->setAllowedAreas(Qt::TopToolBarArea);
	

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
    VizWinMgr* myVizMgr = VizWinMgr::getInstance();
    // signals and slots connections
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
	connect(Edit, SIGNAL(aboutToShow()), this, SLOT (setupUndoRedoText()));
    connect( helpAboutAction, SIGNAL( triggered() ), this, SLOT( helpAbout() ) );
    connect( dataBrowse_DataAction, SIGNAL( triggered() ), this, SLOT( browseData() ) );
	connect( dataMerge_MetafileAction, SIGNAL( triggered() ), this, SLOT( mergeData() ) );
	connect( dataSave_MetafileAction, SIGNAL( triggered() ), this, SLOT( saveMetadata() ) );
	connect( dataLoad_MetafileAction, SIGNAL( triggered() ), this, SLOT( loadData() ) );
	connect( dataLoad_DefaultMetafileAction, SIGNAL( triggered() ), this, SLOT( defaultLoadData() ) );
	connect( dataExportToIDLAction, SIGNAL(triggered()), this, SLOT( exportToIDL()));
	connect(captureMenu, SIGNAL(aboutToShow()), this, SLOT(initCaptureMenu()));
    	connect( viewLaunch_visualizerAction, SIGNAL( triggered() ), this, SLOT( launchVisualizer() ) );
	
	connect( captureStartJpegCaptureAction, SIGNAL( triggered() ), this, SLOT( startJpegCapture() ) );
	connect( captureEndJpegCaptureAction, SIGNAL( triggered() ), this, SLOT( endJpegCapture() ) );
	connect (captureSingleJpegCaptureAction, SIGNAL(triggered()), this, SLOT (captureSingleJpeg()));
	connect( captureStartFlowCaptureAction, SIGNAL( triggered() ), this, SLOT( startFlowCapture() ) );
	connect( captureEndFlowCaptureAction, SIGNAL( triggered() ), this, SLOT( endFlowCapture() ) );
    

	//Toolbar actions:
	connect (navigationAction, SIGNAL(toggled(bool)), this, SLOT(setNavigate(bool)));
	connect (regionSelectAction, SIGNAL(toggled(bool)), this, SLOT(setRegionSelect(bool)));
	connect (probeAction, SIGNAL(toggled(bool)), this, SLOT(setProbe(bool)));
	connect (twoDDataAction, SIGNAL(toggled(bool)), this, SLOT(setTwoDData(bool)));
	connect (twoDImageAction, SIGNAL(toggled(bool)), this, SLOT(setTwoDImage(bool)));
	connect (rakeAction, SIGNAL(toggled(bool)), this, SLOT(setRake(bool)));
	connect (cascadeAction, SIGNAL(triggered()), myVizMgr, SLOT(cascade()));
	connect (tileAction, SIGNAL(triggered()), myVizMgr, SLOT(fitSpace()));
	connect (homeAction, SIGNAL(triggered()), myVizMgr, SLOT(home()));
	connect (sethomeAction, SIGNAL(triggered()), myVizMgr, SLOT(sethome()));
	connect (viewAllAction, SIGNAL(triggered()), myVizMgr, SLOT(viewAll()));
	connect (viewRegionAction, SIGNAL(triggered()), myVizMgr, SLOT(viewRegion()));
	connect (alignViewCombo, SIGNAL(activated(int)), myVizMgr, SLOT(alignView(int)));
	connect (interactiveRefinementSpin, SIGNAL(valueChanged(int)), this, SLOT(setInteractiveRefLevel(int)));
	connect (playForwardAction, SIGNAL(triggered()), this, SLOT(playForward()));
	connect (playBackwardAction, SIGNAL(triggered()), this, SLOT(playBackward()));
	connect (pauseAction, SIGNAL(triggered()), this, SLOT(pauseClick()));
	connect (stepForwardAction, SIGNAL(triggered()), this, SLOT(stepForward()));
	connect (stepBackAction, SIGNAL(triggered()), this, SLOT(stepBack()));
	connect (timestepEdit, SIGNAL(returnPressed()),this, SLOT(setTimestep()));
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

    Data = menuBar()->addMenu(tr("Data"));
    Data->addAction(dataBrowse_DataAction );
    Data->addAction(dataConfigure_MetafileAction );
	
    Data->addAction(dataLoad_MetafileAction );
	Data->addAction(dataLoad_DefaultMetafileAction );
	Data->addAction(dataMerge_MetafileAction);
	Data->addAction(dataSave_MetafileAction);
	
	Data->addAction(dataExportToIDLAction);
    
	Main_Form->addMenu(Data);

    viewMenu = menuBar()->addMenu(tr("View"));
	
	viewMenu->addAction(viewLaunch_visualizerAction);

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
	editUndoAction->setEnabled(false);
	editRedoAction->setEnabled(false);
    
	whatsThisAction = QWhatsThis::createAction(this);

    helpAboutAction = new QAction( this );
	helpAboutAction->setEnabled(true);
    
    dataBrowse_DataAction = new QAction( this );
	dataBrowse_DataAction->setEnabled(false);
    dataConfigure_MetafileAction = new QAction( this );
	dataConfigure_MetafileAction->setEnabled(false);
    dataLoad_MetafileAction = new QAction( this);
	dataMerge_MetafileAction = new QAction( this );
	dataSave_MetafileAction = new QAction( this );
	dataLoad_DefaultMetafileAction = new QAction(this);
	fileNew_SessionAction = new QAction( this );
	dataExportToIDLAction = new QAction(this);
    
    
    viewLaunch_visualizerAction = new QAction( this );
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
	
	if (!wheelIcon){
		MessageReporter::warningMsg("Unable to obtain image from images/wheel.xpm");
	}
	navigationAction = new QAction(*wheelIcon,"Navigation Mode",mouseModeActions);
	navigationAction->setCheckable(true);
	navigationAction->setChecked(true);

	QPixmap* regionIcon = new QPixmap(cube);
	regionSelectAction = new QAction(*regionIcon, "Region Select Mode", mouseModeActions);
	regionSelectAction->setShortcut(Qt::CTRL+Qt::Key_R);
	regionSelectAction->setCheckable(true);
	regionSelectAction->setChecked(false);
	
	QPixmap* probeIcon = new QPixmap(probe);
	probeAction = new QAction(*probeIcon, "Data Probe and Contour Plane Mode", mouseModeActions);
	probeAction->setCheckable(true);
	probeAction->setChecked(false);

	
	QPixmap* twoDDataIcon = new QPixmap(twoDData);
	twoDDataAction = new QAction(*twoDDataIcon, "2D Data Mode", mouseModeActions);
	twoDDataAction->setShortcut(Qt::CTRL+Qt::Key_2);
	twoDDataAction->setCheckable(true);
	twoDDataAction->setChecked(false);

	QPixmap* twoDImageIcon = new QPixmap(twoDImage);
	twoDImageAction = new QAction(*twoDImageIcon, "Image Mode",mouseModeActions);
	twoDImageAction->setShortcut(Qt::CTRL+Qt::Key_I);
	twoDImageAction->setCheckable(true);
	twoDImageAction->setChecked(false);

	QPixmap* rakeIcon = new QPixmap(rake);
	rakeAction = new QAction(*rakeIcon, "Rake Mode", mouseModeActions);
	rakeAction->setShortcut(Qt::CTRL+Qt::Key_K);
	rakeAction->setCheckable(true);
	rakeAction->setChecked(false);

	//Actions for the viztoolbar:
	QPixmap* homeIcon = new QPixmap(home);
	homeAction = new QAction(*homeIcon, "Go to Home Viewpoint", this);
	homeAction->setShortcut(Qt::CTRL+Qt::Key_H);
	QPixmap* sethomeIcon = new QPixmap(sethome);
	sethomeAction = new QAction(*sethomeIcon, "Set Home Viewpoint", this);
	QPixmap* eyeIcon = new QPixmap(eye);
	viewAllAction = new QAction(*eyeIcon, "View All", this);
	viewAllAction->setShortcut(Qt::CTRL+Qt::Key_V);
	QPixmap* magnifyIcon = new QPixmap(magnify);
	viewRegionAction = new QAction(*magnifyIcon, "View Region", this);

	QPixmap* tileIcon = new QPixmap(tiles);
	tileAction = new QAction(*tileIcon,"Tile Windows",this);
	tileAction->setShortcut(Qt::CTRL+Qt::Key_T);

	QPixmap* cascadeIcon = new QPixmap(cascade);
	cascadeAction = new QAction(*cascadeIcon, "Cascade Windows", this);


	//Create actions for each animation control button:
	QPixmap* playForwardIcon = new QPixmap(playforward);
	playForwardAction = new QAction(*playForwardIcon, "Play Forward", this);
	playForwardAction->setShortcut(Qt::CTRL+Qt::Key_P);
	QPixmap* playBackwardIcon = new QPixmap(playreverse);
	playBackwardAction = new QAction(*playBackwardIcon, "Play Backward",this);
	playBackwardAction->setShortcut(Qt::CTRL+Qt::Key_B);
	QPixmap* pauseIcon = new QPixmap(pauseimage);
	pauseAction = new QAction(*pauseIcon, "Stop animation", this);
	pauseAction->setShortcut(Qt::CTRL+Qt::Key_E);
	QPixmap* stepForwardIcon = new QPixmap(stepfwd);
	stepForwardAction = new QAction(*stepForwardIcon, "Step forward", this);
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

    dataBrowse_DataAction->setText( tr( "Browse Data" ) );
    
	dataBrowse_DataAction->setToolTip("Browse filesystem to examine properties of available datasets"); 
	
	dataExportToIDLAction->setText(tr("Export to IDL"));
	
	dataExportToIDLAction->setToolTip("Export current data settings to enable IDL access");
   
    dataConfigure_MetafileAction->setText( tr( "Configure Metafile" ) );
    
	dataConfigure_MetafileAction->setToolTip("Launch a tool to construct the metafile associated with a dataset");
    dataLoad_MetafileAction->setText( tr( "Load a Dataset into Current Session" ) );
   
	dataLoad_MetafileAction->setToolTip("Specify a data set to be loaded into current session");
	dataLoad_MetafileAction->setShortcut(tr("Ctrl+D"));
	dataLoad_DefaultMetafileAction->setText( tr( "Load a Dataset into &Default Session" ) );
  
	dataLoad_DefaultMetafileAction->setToolTip("Specify a data set to be loaded into a new session with default settings");
	
	dataMerge_MetafileAction->setText( tr( "Merge (Import) a Dataset into Current Session" ) );
    
	dataMerge_MetafileAction->setToolTip("Specify a data set to be merged into current session");
	
	dataSave_MetafileAction->setText( tr( "Save the current Metadata to file" ) );
    
	dataSave_MetafileAction->setToolTip("Specify a file where the current Metadata will be saved");
	

    viewLaunch_visualizerAction->setText( tr( "New Visualizer" ) );
    
	viewLaunch_visualizerAction->setToolTip("Launch a new visualization window");

	editVizFeaturesAction->setText(tr("Edit Visualizer Features"));
	
	editVizFeaturesAction->setToolTip(tr("View or change various visualizer settings"));

	editPreferencesAction->setText(tr("Edit User Preferences "));
	
	editPreferencesAction->setToolTip(tr("View or change various user preference settings"));

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




void MainForm::fileOpen()
{

	//This launches a panel that enables the
    //user to choose input session save files, then to
	//load that session
	Session* ses = Session::getInstance();
	string s;
	ses->makeSessionFilepath(s);
	QString fn = s.c_str();//start with filename in session
	QString filename = QFileDialog::getOpenFileName(this, 
		"Choose a VAPOR session file to restore a session",
		fn,
		"Vapor Session Save Files (*.vss)");
	if(filename.length() == 0) return;
		
	
	//Force the name to end with .vss
	if (!filename.endsWith(".vss")){
		filename += ".vss";
	}
	ifstream is;
	is.open((const char*)filename.toAscii());
	if (!is){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open session file: \n%s", (const char*)filename.toAscii());
		return;
	}
	//Remember file if load is successful:
	if(Session::getInstance()->loadFromFile(is)){
		Session::getInstance()->setSessionFilepath((const char*)filename.toAscii());
	}
	MessageReporter::infoMsg("Loaded session file: \n%s", (const char*)filename.toAscii());
}


void MainForm::fileSave()
{
	DataMgr *dataMgr = Session::getInstance()->getDataMgr();

	//This directly saves the session to the current session save file.
    	//It does not prompt the user unless there is an error
	if (! dataMgr) {
		MessageReporter::warningMsg( "There is no current metadata.  \nSession state cannot be saved");
		return;
	}
	if (!Session::getInstance()->metadataIsSaved())
		MessageReporter::warningMsg( "Note: The current (merged) Metadata \nhas not been saved. \nIt will be easier to restore this session \nif the Metadata is also saved.");
	
	ofstream fileout;
	string s;
	Session::getInstance()->makeSessionFilepath(s);
	
	fileout.open(s.c_str());
	if (! fileout) {
		MessageReporter::errorMsg( "Unable to open session file:\n%s", s.c_str());
		return;
	}
	
	if (!Session::getInstance()->saveToFile(fileout)){//Report error if can't save to file
		MessageReporter::errorMsg("Failed to write session file: \n%s", s.c_str());
		fileout.close();
		return;
	}
	fileout.close();
}
//Save the metadata in a file in the current vdf directory.
//The metadata currently won't work if it is saved to a different directory
//
void MainForm::saveMetadata()
{
	DataMgr *dataMgr = Session::getInstance()->getDataMgr();
	DataMgrWB *dataMgrWB = dynamic_cast<DataMgrWB *> (dataMgr);
	DataMgrLayered *dataMgrLayered = dynamic_cast<DataMgrLayered *> (dataMgr);

	//Do nothing if there is no metadata:
	if (! dataMgrWB  && !dataMgrLayered) {
			MessageReporter::errorMsg("There is no Metadata \nto save in current session");
		return;
	}
	//Warn the user that this file is not portable:
	MessageReporter::warningMsg("Note that the Metadata file\nto be written is non-portable;\n%s",
		"You may not be able to load \nthe associated data from another system");
	//This directly saves the session to the current vdf directory
   	QString filename = QFileDialog::getSaveFileName(this,
		"Choose the Metadata filename (in this directory) to save the current metadata",
		Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)");

	if(filename != QString::null){
		//Make sure this filename is still in same directory
		//If not, pop up an explanatory error message and quit.
		int posn = filename.lastIndexOf("/");
		QString path = filename.left(posn);
		QString metadataFile = QString(Session::getInstance()->getMetadataFile().c_str());
		if (!metadataFile.contains(path)){
			int mposn = metadataFile.lastIndexOf("/");
			QString mpath = metadataFile.left(mposn);
			MessageReporter::errorMsg("Specified directory %s is invalid. \nMetadata must be saved to \n%s .",(const char*)path.toAscii(), (const char*)mpath.toAscii());
			return;
		}
		//If ok, go ahead and try to save using current DataMgr
		QFileInfo finfo(filename);
		if (finfo.exists()){
			int rc = QMessageBox::warning(0, "Metadata File Exists", QString("OK to replace existing metadata file?\n %1 ").arg(filename), QMessageBox::Ok, 
				QMessageBox::No);
			if (rc != QMessageBox::Ok) return;
		}
		std::string stdName = filename.toStdString();
		int rc;
		MetadataVDC* md;
		if (dataMgrWB) md = (MetadataVDC*) dataMgrWB;
		else md = (MetadataVDC*) dataMgrLayered;
		rc = md->Write(stdName,0);

		if (rc < 0)MessageReporter::errorMsg( "Unable to save metadata file:\n%s", (const char*)filename.toAscii());
		else {
			Session::getInstance()->setMetadataSaved(true);
			//Save the metadata file name
			Session::getInstance()->getMetadataFile() = filename.toStdString();
		}
		return;
	}
}


void MainForm::fileSaveAs()
{
	DataMgr *dataMgr = Session::getInstance()->getDataMgr();

	if (! dataMgr) {
		MessageReporter::warningMsg( "There is no current metadata.  \nSession state cannot be saved");
		return;
	}
	
	if ( !Session::getInstance()->metadataIsSaved())
		MessageReporter::warningMsg( "Note: The current (merged) Metadata \nhas not been saved. \n It will be easier to restore this session \nif the Metadata is also saved.");
	//This launches a panel that enables the
    //user to choose output session save files, saves to it
	string s;
	Session::getInstance()->makeSessionFilepath(s);
	
	QString filename = QFileDialog::getSaveFileName(this,
		"Choose the output Session File to save current session",
		s.c_str(),
		"Vapor Session Save Files (*.vss)");

	if(filename.length() == 0) return;
		
	
	//Force the name to end with .vss
	if (!filename.endsWith(".vss")){
		filename += ".vss";
	}
	QFileInfo finfo(filename);
	if (finfo.exists()){
		int rc = QMessageBox::warning(0, "Session File Exists", QString("OK to replace session file?\n %1 ").arg(filename), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	ofstream fileout;
	fileout.open(filename.toAscii());
	if (! fileout) {
		MessageReporter::errorMsg( "Unable to save to file: \n%s", (const char*)filename.toAscii());
		return;
	}
	
	if (!Session::getInstance()->saveToFile(fileout)){//Report error if can't save to file
		MessageReporter::errorMsg("Failed to save session to: \n%s", (const char*)filename.toAscii());
		fileout.close();
		return;
	}
	fileout.close();
	Session::getInstance()->setSessionFilepath((const char*)filename.toAscii());
}


void MainForm::filePrint()
{

}


void MainForm::fileExit()
{
	close();
}

void MainForm::undo(){
	Session::getInstance()->backupQueue();
}

void MainForm::redo(){
	Session::getInstance()->advanceQueue();
}
void MainForm::setupUndoRedoText(){
	Session* currentSession = Session::getInstance();
	QString undoText("Undo ");
	QString redoText("Redo ");
	//If it's not active, just set default text:
	if (editUndoAction->isEnabled()) {
		undoText += currentSession->currentUndoCommand()->getDescription();
	}
	if (editRedoAction->isEnabled()) {
		redoText += currentSession->currentRedoCommand()->getDescription();
	}
	editUndoAction->setText( undoText );
	editRedoAction->setText( redoText );
}
// Disable the undo/redo actions, for when the 
// command queue is reinitialized
//
void MainForm::disableUndoRedo(){
	editUndoAction->setEnabled(false);
	editRedoAction->setEnabled(false);
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


//int showWBInfo(const char* wbfile);

void MainForm::browseData()
{
	static QString MDFile("");
	//This launches a panel that enables the
    	//user to peruse the available data files
    	//or metafiles.  Initially it will just select (and open) a vdf file.
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose a Metadata File",
		MDFile,
		"Metadata Files (*.vdf)");
	if(filename!= QString::null) {
		//Need to update this to browse vdf files
		//showWBInfo(filename.toAscii());
		MDFile = filename;
	}
	
}
void MainForm::loadPrefs(){
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose the Preferences File to load into current session",
		Session::getInstance()->getPreferencesFile().c_str(),
		"Vapor Preferences Files (*.vapor_prefs)");
	if(filename != QString::null){
		QFileInfo fInfo(filename);
		if (fInfo.isReadable() && fInfo.isFile())
			UserPreferences::loadPreferences((const char*)filename.toAscii());
		else MessageReporter::errorMsg("Unable to read preferences file: \n%s", (const char*)filename.toAscii());
	}
	
}
void MainForm::savePrefs(){
	QString filename = QFileDialog::getSaveFileName(this,
		"Choose the file name to save current preferences",
		Session::getInstance()->getPreferencesFile().c_str(),
		"Vapor Preferences Files (*.vapor_prefs)");
	if(filename != QString::null){
		QFileInfo fInfo(filename);
		
		if(!UserPreferences::savePreferences((const char*)filename.toAscii()))
			MessageReporter::errorMsg("Unable to save preferences file \n%s", (const char*)filename.toAscii());
	}
}
//Load data into current session
//
void MainForm::loadData()
{
	//This launches a panel that enables the
    	//user to choose input data files, then to
	//create a datamanager using those files
    	//or metafiles.  
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose the Metadata File to load into current session",
		Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)");
	if(filename != QString::null){
		QFileInfo fInfo(filename);
		if (fInfo.isReadable() && fInfo.isFile()){
			Session::getInstance()->resetMetadata((const char*)filename.toAscii(), true);
			
		}
		else MessageReporter::errorMsg("Unable to read metadata file \n%s", (const char*)filename.toAscii());
	}
}
//Merge/Import data into current session
//
void MainForm::mergeData()
{

	//This launches a panel that enables the
    //user to choose input data files, then to
	//merge into current metadata 
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose the Metadata File to merge into current session",
		Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)");

	if(filename != QString::null){
		//Check on the timestep offset:
		int defaultOffset = 0;
		QDialog sDialog(this);
		Ui_SetOffsetDialog uiSetter;
		uiSetter.setupUi(&sDialog);
		int activeWinNum = VizWinMgr::getInstance()->getActiveViz();
		if (activeWinNum >= 0){
			defaultOffset = VizWinMgr::getInstance()->getAnimationParams(activeWinNum)->getCurrentFrameNumber();
		}
		uiSetter.timestepOffsetSpin->setValue(defaultOffset);
		if (sDialog.exec() != QDialog::Accepted) return;
		int offset = uiSetter.timestepOffsetSpin->value();
		if (!Session::getInstance()->resetMetadata((const char*)filename.toAscii(), false, true, offset)){
			MessageReporter::errorMsg("Unsuccessful metadata merge of \n%s",(const char*)filename.toAscii());
		}
	} else MessageReporter::errorMsg("Unable to open \n%s",(const char*)filename.toAscii());
	
}
//Load data into default session
//
void MainForm::defaultLoadData()
{

	//This launches a panel that enables the
    //user to choose input data files, then to
	//create a datamanager using those files
    //or metafiles.  
	QString filename = QFileDialog::getOpenFileName(this,
		"Choose the Metadata File to load into new session",
		Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)");
	if(filename != QString::null){
		Session::getInstance()->resetMetadata(0, false);
		Session::getInstance()->resetMetadata((const char*)filename.toAscii(), false);
	}
	
}
void MainForm::newSession()
{

	Session::getInstance()->resetMetadata(0, false);
	//Reload preferences:
	UserPreferences::loadDefault();
	MessageReporter::getInstance()->resetCounts();
	
}
void MainForm::launchVisualizer()
{
	VizWinMgr::getInstance()->launchVisualizer();
		
}


/*
 * Method to launch the viewpoint/lights tab into the tabbed dialog
 */
void MainForm::viewpoint()
{
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theVizTab){
		theVizTab = new ViewpointEventRouter(tabWidget, "viztab");
		//Set the global vp tab to this:
		
		myVizMgr->hookUpViewpointTab(theVizTab);
	}
    
	
	
	ViewpointEventRouter* myViewpointRouter = myVizMgr->getViewpointRouter();
	
	int posn = tabWidget->findWidget(Params::ViewpointParamsType);
         
	//Create a new parameter class to work with the widget
		
    
	if (posn < 0){
		tabWidget->insertWidget(theVizTab, Params::ViewpointParamsType, true );
	} else {
		tabWidget->moveToFront(Params::ViewpointParamsType);
	}
	myViewpointRouter->updateTab();
}
/*
 * Method that launches the region tab into the tabbed dialog
 */
void MainForm::
region()
{
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theRegionTab){
		theRegionTab = new RegionEventRouter(tabWidget, "regiontab");
		//Set the global region tab to this:
		//myVizMgr->getRegionParams(-1)->setTab(theRegionTab);
		myVizMgr->hookUpRegionTab(theRegionTab);
	}
	
	
	RegionEventRouter* myRegionRouter = myVizMgr->getRegionRouter();
	
	int posn = tabWidget->findWidget(Params::RegionParamsType);
         
	//Create a new parameter class to work with the widget
		
    
	if (posn < 0){
		tabWidget->insertWidget(theRegionTab, Params::RegionParamsType, true );
	} else {
		tabWidget->moveToFront(Params::RegionParamsType);
	}
	myRegionRouter->updateTab();
	
}
/*
 * Launch the DVR panel
 */
void MainForm::
renderDVR(){
    //Do the DVR panel here
	//Determine which is the current active window:
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theDvrTab){
		theDvrTab = new DvrEventRouter(tabWidget, "DVR");
		//myVizMgr->getDvrParams(-1)->setTab(theDvrTab);
		myVizMgr->hookUpDvrTab(theDvrTab);
	}
	int posn = tabWidget->findWidget(Params::DvrParamsType);
	//Create a new parameter class to work with the widget
	if (posn < 0){
		tabWidget->insertWidget(theDvrTab, Params::DvrParamsType, true );
	} else {
		tabWidget->moveToFront(Params::DvrParamsType);
	}
	theDvrTab->updateTab();
	
}
/*
 * Launch the Isosurface panel
 */
void MainForm::
launchIsoTab(){
    //Do the Iso panel here
	//Determine which is the current active window:
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theIsoTab){
		theIsoTab = new IsoEventRouter(tabWidget, "Iso");
		//myVizMgr->getDvrParams(-1)->setTab(theDvrTab);
		myVizMgr->hookUpIsoTab(theIsoTab);
	}
	
	
	int posn = tabWidget->findWidget(Params::IsoParamsType);
         
	//Create a new parameter class to work with the widget
		
    
	if (posn < 0){
		tabWidget->insertWidget(theIsoTab, Params::IsoParamsType, true );
	} else {
		tabWidget->moveToFront(Params::IsoParamsType);
	}
	theIsoTab->updateTab();
	
}
/*
 * Launch the animation panel
 */
void MainForm::
animationParams(){
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theAnimationTab){
		theAnimationTab = new AnimationEventRouter(tabWidget, "animationtab");
		//Set the global region tab to this:
		//myVizMgr->getAnimationParams(-1)->setTab(theAnimationTab);
		myVizMgr->hookUpAnimationTab(theAnimationTab);
	}
   
	AnimationEventRouter* myAnimationRouter = myVizMgr->getAnimationRouter();
	
	int posn = tabWidget->findWidget(Params::AnimationParamsType);
         
	//Create a new parameter class to work with the widget
    
	if (posn < 0){
		tabWidget->insertWidget(theAnimationTab, Params::AnimationParamsType, true );
	} else {
		tabWidget->moveToFront(Params::AnimationParamsType);
	}
	myAnimationRouter->updateTab();
	
	
}

/*
 * Launch the Flow rendering tab
 */
void MainForm::launchFlowTab()
{
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theFlowTab){
		theFlowTab = new FlowEventRouter(tabWidget, "Flowtab");
		//Set the global Flow tab to this:
		//myVizMgr->getFlowParams(-1)->setTab(theFlowTab);
		myVizMgr->hookUpFlowTab(theFlowTab);
	}
   
	
	FlowEventRouter* myFlowRouter = myVizMgr->getFlowRouter();
	
	int posn = tabWidget->findWidget(Params::FlowParamsType);
         
	//Create a new parameter class to work with the widget
		
    
	if (posn < 0){
		tabWidget->insertWidget(theFlowTab, Params::FlowParamsType, true );
	} else {
		tabWidget->moveToFront(Params::FlowParamsType);
	}
	myFlowRouter->updateTab();
	
}
void MainForm::launchProbeTab()
{
	//Do the probe panel here
	//Determine which is the current active window:
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theProbeTab){
		theProbeTab = new ProbeEventRouter(tabWidget, "ProbeTab");
		myVizMgr->hookUpProbeTab(theProbeTab);
		theProbeTab->attachSeedCheckbox->setToolTip("Enable continuous updating of the flow using selected point as seed.\nNote: Flow must be enabled to use seed list, and Region must contain the seed");
		theProbeTab->addSeedButton->setToolTip("Click to add the selected point to the seeds for the applicable flow panel.\nNote: Flow must be enabled to use seed list, and Region must contain the seed");

	}
	
	int posn = tabWidget->findWidget(Params::ProbeParamsType);
         
	//Create a new parameter class to work with the widget
		 
	if (posn < 0){
		tabWidget->insertWidget(theProbeTab, Params::ProbeParamsType, true );
	} else {
		tabWidget->moveToFront(Params::ProbeParamsType);
	}
}
void MainForm::launchTwoDDataTab()
{
	//Do the probe panel here
	//Determine which is the current active window:
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theTwoDDataTab){
		theTwoDDataTab = new TwoDDataEventRouter(tabWidget, "TwoDDataTab");
		myVizMgr->hookUpTwoDDataTab(theTwoDDataTab);
		theTwoDDataTab->attachSeedCheckbox->setToolTip("Enable continuous updating of the flow using selected point as seed.\nNote: Flow must be enabled to use seed list, and Region must contain the seed");
		theTwoDDataTab->addSeedButton->setToolTip("Click to add the selected point to the seeds for the applicable flow panel.\nNote: Flow must be enabled to use seed list, and Region must contain the seed");
	}
	
	int posn = tabWidget->findWidget(Params::TwoDDataParamsType);
         
	//Create a new parameter class to work with the widget
		 
	if (posn < 0){
		tabWidget->insertWidget(theTwoDDataTab, Params::TwoDDataParamsType, true );
	} else {
		tabWidget->moveToFront(Params::TwoDDataParamsType);
	}
}
void MainForm::launchTwoDImageTab()
{
	//Do the probe panel here
	//Determine which is the current active window:
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	if (!theTwoDImageTab){
		theTwoDImageTab = new TwoDImageEventRouter(tabWidget, "TwoDImageTab");
		myVizMgr->hookUpTwoDImageTab(theTwoDImageTab);
//QT4.5		QToolTip::add(theTwoDImageTab->attachSeedCheckbox, QString("Enable continuous updating of the flow using selected point as seed.\nNote: Flow must be enabled to use seed list, and Region must contain the seed"));
//QT4.5		QToolTip::add(theTwoDImageTab->addSeedButton,QString("Click to add the selected point to the seeds for the applicable flow panel.\nNote: Flow must be enabled to use seed list, and Region must contain the seed"));
	}
	
	int posn = tabWidget->findWidget(Params::TwoDImageParamsType);
         
	//Create a new parameter class to work with the widget
		 
	if (posn < 0){
		tabWidget->insertWidget(theTwoDImageTab, Params::TwoDImageParamsType, true );
	} else {
		tabWidget->moveToFront(Params::TwoDImageParamsType);
	}
}
/*
 * Respond to toolbar clicks:
 * navigate mode.  Don't change tab menu
 */
void MainForm::setNavigate(bool on)
{
	if (!on) return;
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	Session* currentSession = Session::getInstance();
	//Only do something if this is an actual change of mode
	if (GLWindow::getCurrentMouseMode() != GLWindow::navigateMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		myVizMgr->setSelectionMode(GLWindow::navigateMode);
		currentSession->blockRecording();
		//viewpoint();
		currentSession->unblockRecording();
		currentSession->addToHistory(new MouseModeCommand(oldMode,  GLWindow::navigateMode));
		
		GLWindow::setCurrentMouseMode(GLWindow::navigateMode);
		VizWinMgr::getInstance()->updateActiveParams();
		
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("Navigation Mode:  Use left mouse to rotate or spin-animate, right to zoom, middle to translate",this);
		statusBar()->addWidget(modeStatusWidget,2);
	}
	//resetModeButtons();
}
void MainForm::setLights(bool  on)
{
// Until we implement this, does nothing:
	Session* currentSession = Session::getInstance();
	if (!on && moveLightsAction->isChecked()){navigationAction->toggle(); return;}
	if (!on) return;
	if (GLWindow::getCurrentMouseMode() != GLWindow::lightMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		VizWinMgr::getInstance()->setSelectionMode(GLWindow::lightMode);
		currentSession->addToHistory(new MouseModeCommand(oldMode,  GLWindow::lightMode));
		GLWindow::setCurrentMouseMode(GLWindow::lightMode);
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
			modeStatusWidget = 0;
		}
	}
	
}
void MainForm::setProbe(bool on)
{
	
	if (!on && probeAction->isChecked()){navigationAction->toggle(); return;}
	if (!on) return;
	Session* currentSession = Session::getInstance();
	if (GLWindow::getCurrentMouseMode() != GLWindow::probeMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		VizWinMgr::getInstance()->setSelectionMode(GLWindow::probeMode);
		
		currentSession->blockRecording();
		launchProbeTab();
		currentSession->unblockRecording();
		Session::getInstance()->addToHistory(new MouseModeCommand(oldMode,  GLWindow::probeMode));
		GLWindow::setCurrentMouseMode(GLWindow::probeMode);
		VizWinMgr::getInstance()->updateActiveParams();
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("Probe Mode:  To modify probe in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
		statusBar()->addWidget(modeStatusWidget,2);
		
	}
	
}
void MainForm::setTwoDData(bool on)
{
	//2d mode is like rake mode
	if (!on && twoDDataAction->isChecked()){navigationAction->toggle(); return;}
	if (!on) return;
	Session* currentSession = Session::getInstance();
	if (GLWindow::getCurrentMouseMode() != GLWindow::twoDDataMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		VizWinMgr::getInstance()->setSelectionMode(GLWindow::twoDDataMode);
		
		currentSession->blockRecording();
		launchTwoDDataTab();
		currentSession->unblockRecording();
		Session::getInstance()->addToHistory(new MouseModeCommand(oldMode,  GLWindow::twoDDataMode));
		GLWindow::setCurrentMouseMode(GLWindow::twoDDataMode);
		VizWinMgr::getInstance()->updateActiveParams();
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("TwoDData Mode:  To modify 2D plane in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
		statusBar()->addWidget(modeStatusWidget,2);
		
	}
	
}
void MainForm::setTwoDImage(bool on)
{
	//2d image mode is like rake mode
	if (!on && twoDImageAction->isChecked()){navigationAction->toggle(); return;}
	if (!on) return;
	Session* currentSession = Session::getInstance();
	if (GLWindow::getCurrentMouseMode() != GLWindow::twoDImageMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		VizWinMgr::getInstance()->setSelectionMode(GLWindow::twoDImageMode);
		
		currentSession->blockRecording();
		launchTwoDImageTab();
		currentSession->unblockRecording();
		Session::getInstance()->addToHistory(new MouseModeCommand(oldMode,  GLWindow::twoDImageMode));
		GLWindow::setCurrentMouseMode(GLWindow::twoDImageMode);
		VizWinMgr::getInstance()->updateActiveParams();
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("TwoDImage Mode:  To modify 2D plane in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
		statusBar()->addWidget(modeStatusWidget,2);
		
	}
	
}
void MainForm::setRake(bool on)
{
	
	if (!on && rakeAction->isChecked()){navigationAction->toggle(); return;}
	if (!on) return;
	Session* currentSession = Session::getInstance();
	if (GLWindow::getCurrentMouseMode() != GLWindow::rakeMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		VizWinMgr::getInstance()->setSelectionMode(GLWindow::rakeMode);
		//bring up the flowtab, but don't put into history:
		currentSession->blockRecording();
		launchFlowTab();
		currentSession->unblockRecording();
		Session::getInstance()->addToHistory(new MouseModeCommand(oldMode,  GLWindow::rakeMode));
		GLWindow::setCurrentMouseMode(GLWindow::rakeMode);
		VizWinMgr::getInstance()->updateActiveParams();
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("Rake Mode: To modify rake in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
		statusBar()->addWidget(modeStatusWidget,2);
	}
	
}
void MainForm::setRegionSelect(bool on)
{
	Session* currentSession = Session::getInstance();
	//Only respond to toggling on:
	if (!on && regionSelectAction->isChecked()){navigationAction->toggle(); return;}
	if (!on) return;
	if (GLWindow::getCurrentMouseMode() != GLWindow::regionMode){
		GLWindow::mouseModeType oldMode = GLWindow::getCurrentMouseMode();
		VizWinMgr::getInstance()->setSelectionMode(GLWindow::regionMode);
		//bring up the tab, but don't put into history:
		currentSession->blockRecording();
		region();
		currentSession->unblockRecording();
		currentSession->addToHistory(new MouseModeCommand(oldMode,  GLWindow::regionMode));
		GLWindow::setCurrentMouseMode(GLWindow::regionMode);
		VizWinMgr::getInstance()->updateActiveParams();
		if(modeStatusWidget) {
			statusBar()->removeWidget(modeStatusWidget);
			delete modeStatusWidget;
		}
		modeStatusWidget = new QLabel("Region Mode: To modify region in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
		statusBar()->addWidget(modeStatusWidget,2);
	}
}

//Enable or disable the View menu options:
void MainForm::initCaptureMenu(){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* viz = vizWinMgr->getActiveVisualizer();
	int winNum = vizWinMgr->getActiveViz();
	QString vizName = "";
	if(winNum >= 0) vizName = vizWinMgr->getVizWinName(winNum);
	//Disable the start capture if no viz, or if active viz already capturing
	
	
	if (!viz || viz->getGLWindow()->isCapturingImage()) {
		captureStartJpegCaptureAction->setText( "&Begin image capture sequence"  );
		captureStartJpegCaptureAction->setEnabled(false);
		captureSingleJpegCaptureAction->setText("Capture single image");
		captureSingleJpegCaptureAction->setEnabled(false);
		
	} else {// there is a visualizer, but it's not capturing images
		captureStartJpegCaptureAction->setText( "&Begin image capture sequence in "+(vizName) );
		captureStartJpegCaptureAction->setEnabled(true);
		
		captureSingleJpegCaptureAction->setText("Capture single image of "+(vizName) );
		captureSingleJpegCaptureAction->setEnabled(true);
	}
	//Likewise for flow:
	if (!viz || viz->getGLWindow()->isCapturingFlow()) {
		captureStartFlowCaptureAction->setText( "&Begin flow capture sequence"  );
		captureStartFlowCaptureAction->setEnabled( false);
	} else {// there is a visualizer, but it's not capturing flow
		captureStartFlowCaptureAction->setText( "&Begin flow capture sequence in "+(vizName) );
		captureStartFlowCaptureAction->setEnabled(true);
	}
	
	//disable the end capture if no viz, or if active viz is not capturing
	GLWindow* glWin = viz->getGLWindow();
	if (!viz || !glWin->isCapturingImage()){
		captureEndJpegCaptureAction->setText( "End image capture sequence" );
		captureEndJpegCaptureAction->setEnabled( false);
	} else {
		captureEndJpegCaptureAction->setText("End image capture sequence in " +(vizName) );
		captureEndJpegCaptureAction->setEnabled( true);
	}
	if (!viz || !glWin->isCapturingFlow()){
		captureEndFlowCaptureAction->setText( "End flow capture sequence" );
		captureEndFlowCaptureAction->setEnabled(false);
	} else {
		captureEndFlowCaptureAction->setText("End flow capture sequence in " +(vizName) );
		captureEndFlowCaptureAction->setEnabled(true);
	}
	
}

/*
 * Set all the mode buttons off, except navigation
 */
void MainForm::resetModeButtons(){
	navigationAction->setChecked(true);
	probeAction->setChecked(false);
	twoDDataAction->setChecked(false);
	twoDImageAction->setChecked(false);
	regionSelectAction->setChecked(false);
	moveLightsAction->setChecked(false);
	rakeAction->setChecked(false);
}
//Make all the current region/animation settings available to IDL
void MainForm::exportToIDL(){
	Session::getInstance()->exportData();
}
	
//Begin capturing images.
//Launch a file save dialog to specify the names
//Then start file saving mode.
void MainForm::startJpegCapture() {
	QFileDialog fileDialog(this,
		"Specify first file name for image capture sequence",
		Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg Images (*.jpg)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.move(pos());
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QStringList qsl = fileDialog.selectedFiles();
	if (qsl.isEmpty()) return;
	QString s = qsl[0];
	QFileInfo* fileInfo = new QFileInfo(s);
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory((const char*)fileInfo->absolutePath().toAscii());
	QString fileBaseName = fileInfo->baseName();
	//See if it ends with digits
	int posn;
	for (posn = fileBaseName.length()-1; posn >=0; posn--){
		if (!fileBaseName.at(posn).isDigit()) break;
	}
	int startFileNum = 0;
	unsigned int lastDigitPos = posn+1;
	if (lastDigitPos < fileBaseName.length()) {
		startFileNum = fileBaseName.right(fileBaseName.length()-lastDigitPos).toInt();
		fileBaseName.truncate(lastDigitPos);
	}
	QString filePath = fileInfo->absolutePath() + "/" + fileBaseName;
	//Determine the active window:
	//Turn on "image capture mode" in the current active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	if (viz) {
		viz->getGLWindow()->startImageCapture(filePath,startFileNum);
		//Provide a popup stating the capture parameters in effect.
		MessageReporter::infoMsg("Image Capture Activated \n Image is being captured to %s",
			(const char*)filePath.toAscii());
		
	} else {
		MessageReporter::errorMsg("Image Capture Error;\nNo active visualizer for capturing images");
	}
	delete fileInfo;
}
//Begin capturing flow.
//Launch a file save dialog to specify the names
//Then start file saving mode.
void MainForm::startFlowCapture() {
	//Check first that we are not capturing unsteady flow:
	FlowParams* fparams = VizWinMgr::getActiveFlowParams();
	if (fparams->getFlowType() == 1){
		MessageReporter::errorMsg(" Unsteady flow lines may only be captured from flow panel");
		return;
	}
	QFileDialog fileDialog(this,
		"Specify first file name for flow capture sequence",
		Session::getInstance()->getFlowDirectory().c_str(),
		"text files (*.txt)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.move(pos());
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QStringList qs = fileDialog.selectedFiles();
	if (qs.isEmpty()) return;
	QString s = qs[0];
	QFileInfo* fileInfo = new QFileInfo(s);
	//Save the path for future captures
	Session::getInstance()->setFlowDirectory((const char*)fileInfo->absolutePath().toAscii());
	QString fileBaseName = fileInfo->baseName();
	//See if it ends with digits
	int posn;
	for (posn = fileBaseName.length()-1; posn >=0; posn--){
		if (!fileBaseName.at(posn).isDigit()) break;
	}
	unsigned int lastDigitPos = posn+1;
	if (lastDigitPos < fileBaseName.length()) {
		fileBaseName.truncate(lastDigitPos);
	}
	
	QString filePath = fileInfo->absolutePath() + "/" + fileBaseName;
	//Determine the active window:
	//Turn on "flow capture mode" in the current active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	if (viz) {
		viz->getGLWindow()->startFlowCapture(filePath);
		//Provide a popup stating the capture parameters in effect.
		MessageReporter::infoMsg("Flow Capture Activated \n Flow is being captured to %s",
			(const char*)filePath.toAscii());
		
	} else {
		MessageReporter::errorMsg("Flow Capture Error;\nNo active visualizer for capturing images");
	}
	delete fileInfo;
}

//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void MainForm::captureSingleJpeg() {
	QFileDialog fileDialog(this,
		"Specify single image capture file name",
		Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg or Tiff images (*.jpg *.tif)");
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	fileDialog.move(pos());
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QStringList files = fileDialog.selectedFiles();
	if (files.isEmpty()) return;
	QString filename = files[0];
    
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory((const char*)fileInfo->absolutePath().toAscii());
	
	//Determine the active window:
	//Turn on "image capture mode" in the current active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	if (viz) {
		viz->getGLWindow()->singleCaptureImage(filename);
		//Provide a message stating the capture in effect.
		MessageReporter::infoMsg("Single Image is captured to %s",
			(const char*)filename.toAscii());
		
	} else {
		MessageReporter::errorMsg("Image Capture Error;\nNo active visualizer for capturing image");
	}
}
void MainForm::endJpegCapture(){
	//Turn off capture mode for the current active visualizer (if it is on!)
	//Otherwise indicate that the visualizer is not capturing.
	GLWindow* glWin = VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow();

	if (glWin && glWin->isCapturingImage()) glWin->stopImageCapture();
	else {
		MessageReporter::warningMsg("Image Capture Warning;\nCurrent active visualizer is not capturing images");
	}
}
void MainForm::endFlowCapture(){
	//Turn off capture mode for the current active visualizer (if it is on!)
	//Otherwise indicate that the visualizer is not capturing.
	GLWindow* glWin = VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow();

	if (glWin && glWin->isCapturingFlow()) glWin->stopFlowCapture();
	else {
		MessageReporter::warningMsg("Image Capture Warning;\nCurrent active visualizer is not capturing images");
	}
}
void MainForm::launchVizFeaturesPanel(){
	VizFeatureParams vFP;
	vFP.launch();
}
void MainForm::launchPreferencesPanel(){
	UserPreferences uPref;
	uPref.launch();
}
void MainForm::setInteractiveRefLevel(int val){
	VizWinMgr::getInstance()->setInteractiveNavigating(val);
}
void MainForm::setInteractiveRefinementSpin(int val){
	if(interactiveRefinementSpin)interactiveRefinementSpin->setValue(val);
}
	
void MainForm::pauseClick(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::AnimationParamsType);
	aRouter->animationPauseClick();
}
void MainForm::playForward(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::AnimationParamsType);
	aRouter->animationPlayForwardClick();
}
void MainForm::playBackward(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::AnimationParamsType);
	aRouter->animationPlayReverseClick();
}
void MainForm::stepBack(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::AnimationParamsType);
	aRouter->animationStepReverseClick();
}	
void MainForm::stepForward(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::AnimationParamsType);
	aRouter->animationStepForwardClick();
}	
//Respond to a change in the text in the animation toolbar
void MainForm::setTimestep(){
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::AnimationParamsType);
	int tstep = timestepEdit->text().toInt();
	AnimationParams* aParams = VizWinMgr::getActiveAnimationParams();

	if (tstep < aParams->getStartFrameNumber()) tstep = aParams->getStartFrameNumber();
	if (tstep > aParams->getEndFrameNumber()) tstep = aParams->getEndFrameNumber();
	aRouter->guiSetTimestep(tstep);
}
//Set the timestep in the animation toolbar:
void MainForm::setCurrentTimestep(int tstep){
	timestepEdit->setText(QString::number(tstep));
	update();
}
void MainForm::paintEvent(QPaintEvent* e){
	MessageReporter::infoMsg("MainForm::paintEvent");
	QMainWindow::paintEvent(e);
}
