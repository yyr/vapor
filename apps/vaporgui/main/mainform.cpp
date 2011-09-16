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
#include <QMenuItem>
#include "GL/glew.h"
#include "mainform.h"
#include "pythonedit.h"
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
#include "pythonpipeline.h"

#include "animationparams.h"
#include "probeparams.h"
#include "twoDdataparams.h"
#include "twoDimageparams.h"

#include "assert.h"
#include "command.h"

#include "vizfeatureparams.h"
#include "userpreferences.h"

#include <vapor/DataMgrWB.h>
#include <vapor/LayeredIO.h>
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

    
	int min_width = MIN_WINDOW_WIDTH;
	int min_height = MIN_WINDOW_HEIGHT;
	if (char *s = getenv("VAPOR_WIDTH_HEIGHT")) {
		int h,w;
		int rc = sscanf(s, "%dx%d", &w, &h);
		if (rc==2) {
			min_width = w;
			min_height = h;
		}
		cerr << "VAPOR_WIDTH_HEIGHT = " << s << endl;
	} 
    setMinimumSize( QSize( min_width, min_height ) );
   
   
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
	
	//Setup  the session (hence the viz window manager)
	Session::getInstance();
	MessageReporter::infoMsg("MainForm::MainForm(): setup session");
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	
	myVizMgr->createAllDefaultParams();
	
	createToolBars();	
	myVizMgr->RegisterMouseModes();
    (void)statusBar();
    Main_Form->adjustSize();
    languageChange();
	hookupSignals();   

	//Now that the tabmgr and the viz mgr exist, hook up the tabs:
	
	//These need to be installed to set the tab pointers in the
	//global params.
	Session::getInstance()->blockRecording();
	
	//Create one initial visualizer:
	myVizMgr->launchVisualizer();

	Session::getInstance()->unblockRecording();
	
    	show();
	((DvrEventRouter*)VizWinMgr::getInstance()->getEventRouter(Params::_dvrParamsTag))->initTypes();

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
			vector<string> files;
			files.push_back(fileName.toStdString());
			Session::getInstance()->resetMetadata(files, false, false);
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
   
	delete Session::getInstance();
	PythonPipeLine::terminate();
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
	timestepEdit = new QLineEdit(animationToolBar);
	timestepEdit->setAlignment(Qt::AlignHCenter);
	timestepEdit->setMaximumWidth(40);
	timestepEdit->setToolTip( "Edit/Display current time step");
	timestepEdit->setValidator(v);
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
    
	connect( dataMerge_MetafileAction, SIGNAL( triggered() ), this, SLOT( mergeData() ) );
	connect( dataImportWRF_Action, SIGNAL( triggered() ), this, SLOT( importWRFData() ) );
	connect( dataImportDefaultWRF_Action, SIGNAL( triggered() ), this, SLOT( importDefaultWRFData() ) );
	connect( dataSave_MetafileAction, SIGNAL( triggered() ), this, SLOT( saveMetadata() ) );
	connect( dataLoad_MetafileAction, SIGNAL( triggered() ), this, SLOT( loadData() ) );
	connect( dataLoad_DefaultMetafileAction, SIGNAL( triggered() ), this, SLOT( defaultLoadData() ) );
	connect( dataExportToIDLAction, SIGNAL(triggered()), this, SLOT( exportToIDL()));
	connect(captureMenu, SIGNAL(aboutToShow()), this, SLOT(initCaptureMenu()));
   
	connect(Edit, SIGNAL(aboutToShow()), this, SLOT (setupEditMenu()));
	connect( newPythonAction, SIGNAL(triggered()), this, SLOT(newPythonEditor()));
	connect(editPythonStartupAction,SIGNAL(triggered()), this, SLOT(editPythonStartup()));
	connect(editPythonMenu,SIGNAL(triggered(QAction*)), this, SLOT(launchPythonEditor(QAction*)));
	connect( captureStartJpegCaptureAction, SIGNAL( triggered() ), this, SLOT( startJpegCapture() ) );
	connect( captureEndJpegCaptureAction, SIGNAL( triggered() ), this, SLOT( endJpegCapture() ) );
	connect (captureSingleJpegCaptureAction, SIGNAL(triggered()), this, SLOT (captureSingleJpeg()));
	connect( captureStartFlowCaptureAction, SIGNAL( triggered() ), this, SLOT( startFlowCapture() ) );
	connect( captureEndFlowCaptureAction, SIGNAL( triggered() ), this, SLOT( endFlowCapture() ) );
    

	//Toolbar actions:
	connect (navigationAction, SIGNAL(toggled(bool)), this, SLOT(setNavigate(bool)));
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
	connect (timestepEdit, SIGNAL(editingFinished()),this, SLOT(setTimestep()));
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
	
	editUndoAction->setEnabled(false);
	editRedoAction->setEnabled(false);
    
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
	
	if (!wheelIcon){
		MessageReporter::warningMsg("Unable to obtain image from images/wheel.xpm");
	}
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
	LayeredIO *dataMgrLayered = dynamic_cast<LayeredIO *> (dataMgr);
	MetadataVDC* md = dynamic_cast<MetadataVDC*> (dataMgr);

	//Do nothing if there is no metadata:
	if (!md ||(! dataMgrWB  && !dataMgrLayered)) {
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
			vector<string> files;
			files.push_back(filename.toStdString());
			Session::getInstance()->resetMetadata(files, true, false);
			
		}
		else MessageReporter::errorMsg("Unable to read metadata file \n%s", (const char*)filename.toAscii());
	}
}
//Merge data into current session
//
void MainForm::mergeData()
{

	//This launches a panel that enables the
    //user to choose input data files, then to
	//merge into current metadata 
	DataMgr* dm = DataStatus::getInstance()->getDataMgr();
	MetadataVDC* md = dynamic_cast<MetadataVDC*> (dm);
	if (!md){
		MessageReporter::errorMsg("No current metadata available for merge");
		return;
	}
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
		vector<string> files;
		files.push_back(filename.toStdString());
		if (!Session::getInstance()->resetMetadata(files, false, false, true, offset)){
			MessageReporter::errorMsg("Unsuccessful metadata merge of \n%s",(const char*)filename.toAscii());
		}
	} else MessageReporter::errorMsg("Unable to open \n%s",(const char*)filename.toAscii());
	
}
//import WRF data into current session
//
void MainForm::importWRFData()
{

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
	
	} else MessageReporter::errorMsg("No valid WRF files \n");
	
}
//import WRF data into default session
void MainForm::importDefaultWRFData()
{

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
		vector<string> files;
		Session::getInstance()->resetMetadata(files, false, false);
		files.push_back(filename.toStdString());
		Session::getInstance()->resetMetadata(files, false, false);
	}
	
}
void MainForm::newSession()
{
	vector<string> files;
	Session::getInstance()->resetMetadata(files, false, false);
	//Reload preferences:
	UserPreferences::loadDefault();
	PythonEdit::loadUserStartupScript();
	MessageReporter::getInstance()->resetCounts();
	
}
void MainForm::launchVisualizer()
{
	VizWinMgr::getInstance()->launchVisualizer();
		
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
		int oldMode = GLWindow::getCurrentMouseMode();
		myVizMgr->setSelectionMode(GLWindow::navigateMode);
		currentSession->blockRecording();
		modeCombo->setCurrentIndex(0);
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
}


void MainForm::setupEditMenu(){
	//Setup undo/redo text
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
	//set up the menus based on what derived variables are available
	
	editPythonMenu->clear();
	newPythonAction->setEnabled(true);
	bool haveVars = false;
	
	map<int,vector<string> > pyMap = DataStatus::getDerived3DOutputMap();
	//Iterate through the maps, getting all 3D variables
	map <int,vector<string> > :: const_iterator var_Iter = pyMap.begin();
	while (var_Iter != pyMap.end()){
		const vector<string> varnames = var_Iter->second;
		for (int i = 0; i<varnames.size(); i++){
			const QString varname = QString(varnames[i].c_str());
			editPythonMenu->addAction(varname);
			haveVars = true;
		}
		var_Iter++;
	}
	pyMap = DataStatus::getDerived2DOutputMap();
	//Iterate through the maps, getting all 2D variables
	var_Iter = pyMap.begin();
	while (var_Iter != pyMap.end()){
		const vector<string> varnames = var_Iter->second;
		for (int i = 0; i<varnames.size(); i++){
			const QString varname = QString(varnames[i].c_str());
			editPythonMenu->addAction(varname);
			haveVars = true;
		}
		var_Iter++;
	}
	editPythonMenu->setEnabled(haveVars);

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
	GLWindow* glWin=0;
	if(viz) glWin= viz->getGLWindow();
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

//Make all the current region/animation settings available to IDL
void MainForm::exportToIDL(){
	Session::getInstance()->exportData();
}
	
//Begin capturing images.
//Launch a file save dialog to specify the names
//Then start file saving mode.
void MainForm::startJpegCapture() {
	showCitationReminder();
	QFileDialog fileDialog(this,
		"Specify first file name for image capture sequence",
		Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg or Tiff images (*.jpg *.tif)");
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
	//See if it ends with "tif":
	bool isTif = false;
	if (fileInfo->suffix() == "tif") isTif = true;
	//Determine the active window:
	//Turn on "image capture mode" in the current active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	if (viz) {
		viz->getGLWindow()->startImageCapture(filePath,startFileNum, isTif);
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
	showCitationReminder();
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
void MainForm::editPythonStartup(){
	PythonEdit* pythonEditor = new PythonEdit(this, "startup script");
	pythonEditor->show();
}
void MainForm::newPythonEditor(){
	PythonEdit* pythonEditor = new PythonEdit(this);
	pythonEditor->show();
}
void MainForm::launchPythonEditor(QAction* act){
	QString str = act->text();
	PythonEdit* pythonEditor = new PythonEdit(this,str);
	pythonEditor->show();
}
void MainForm::setInteractiveRefLevel(int val){
	VizWinMgr::getInstance()->setInteractiveNavigating(val);
}
void MainForm::setInteractiveRefinementSpin(int val){
	if(interactiveRefinementSpin)interactiveRefinementSpin->setValue(val);
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
	AnimationEventRouter* aRouter = (AnimationEventRouter*)VizWinMgr::getEventRouter(Params::_animationParamsTag);
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
void MainForm::showTab(const std::string& tag){
	ParamsBase::ParamsBaseType t = Params::GetTypeFromTag(tag);
	tabWidget->moveToFront(t);
	EventRouter* eRouter = VizWinMgr::getEventRouter(tag);
	eRouter->updateTab();
}
void MainForm::modeChange(int newmode){
	if (newmode == 0) {
		navigationAction->setChecked(true);
		return;
	}
	Session* currentSession = Session::getInstance();
	int oldMode = GLWindow::getCurrentMouseMode();
	VizWinMgr::getInstance()->setSelectionMode(newmode);
	//bring up the tab, but don't put into history:
	currentSession->blockRecording();
	navigationAction->setChecked(false);
	showTab(Params::GetTagFromType(GLWindow::getModeParamType(newmode)));
	currentSession->unblockRecording();
	currentSession->addToHistory(new MouseModeCommand(oldMode,  newmode));
	GLWindow::setCurrentMouseMode(newmode);
	VizWinMgr::getInstance()->updateActiveParams();
	if(modeStatusWidget) {
		statusBar()->removeWidget(modeStatusWidget);
		delete modeStatusWidget;
	}

	modeStatusWidget = new QLabel(QString::fromStdString(GLWindow::getModeName(newmode))+" Mode: To modify box in scene, grab handle with left mouse to translate, right mouse to stretch",this); 
	statusBar()->addWidget(modeStatusWidget,2);
	
}
void MainForm::showCitationReminder(){
	//First check if reminder is turned off:
	if (!Session::getInstance()->getCitationRemind()) return;
	//Provide a customized message box
	QMessageBox msgBox;
	QString reminder("VAPOR is developed as an Open Source application by the National Center for Atmospheric Research ");
	reminder.append("under the sponsorship of the National Science Foundation.  ");
	reminder.append("Continued support from VAPOR is dependent on demonstrable evidence of the software's value to the scientific community.  ");
	reminder.append("You are free to use VAPOR as permitted under the terms and conditions of the licence.\n\n ");
	reminder.append("We kindly request that you cite VAPOR in your publications and presentations. ");
	reminder.append("Citation details can be found on the VAPOR website at: \n\n  http://www.vapor.ucar.edu/index.php?id=citation");
	msgBox.setText(reminder);
	msgBox.setInformativeText("This reminder can be silenced from the User Preferences panel");
		
	msgBox.setStandardButtons(QMessageBox::Ok);
	msgBox.setDefaultButton(QMessageBox::Ok);
		
	msgBox.exec();
	Session::getInstance()->setCitationRemind(false);

}
