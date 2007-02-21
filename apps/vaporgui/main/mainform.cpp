//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
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
#include "mainform.h"
#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qaction.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qtoolbar.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qcombobox.h>
#include <qfiledialog.h>
#include <qlineedit.h>
#include <qscrollview.h>
#include <qdesktopwidget.h>
#include <qmessagebox.h>
#include <qvbox.h>
#include <qworkspace.h>
#include <qcolordialog.h>
#include <qstatusbar.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qtoolbutton.h>


#include <iostream>
#include <fstream>
#include <sstream>
#include "dvreventrouter.h"
#include "setoffset.h"
#include "vizwin.h"
#include "floweventrouter.h"
#include "probeeventrouter.h"
#include "vizselectcombo.h"
#include "tabmanager.h"
#include "viewpointeventrouter.h"
#include "regioneventrouter.h"
#include "vizwinmgr.h"
#include "animationeventrouter.h"
#include "session.h"
#include "messagereporter.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "dvrparams.h"

#include "animationparams.h"
#include "probeparams.h"

#include "assert.h"
#include "command.h"
#include "sessionparameters.h"
#include "vizfeatureparams.h"
#include "sessionparams.h"
#include "vapor/Version.h"

//The following are pixmaps that are used in gui:
#include "images/cascade.xpm"
#include "images/tiles.xpm"
#include "images/probe.xpm"
#include "images/rake.xpm"
#include "images/wheel.xpm"
#include "images/cube.xpm"
//#include "images/planes.xpm"
//#include "images/lightbulb.xpm"
#include "images/home.xpm"
#include "images/sethome.xpm" 
#include "images/eye.xpm"
#include "images/magnify.xpm"

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
MainForm::MainForm( QWidget* parent, const char* name, WFlags )
    : QMainWindow( parent, name, WDestructiveClose )
{
	theMainForm = this;
	theRegionTab = 0;
	theDvrTab = 0;
	theVizTab = 0;
	theAnimationTab = 0;
	theFlowTab = 0;
	theProbeTab = 0;

	modeStatusWidget = 0;
	
	sessionSaveFile.setAscii("VaporSaved.vss");

    (void)statusBar();
    if ( !name )
		setName( "MainForm" );
    setMinimumSize( QSize( 422, 606 ) );
   
    setBackgroundOrigin( QMainWindow::WindowOrigin );
	
    QVBox* vb = new QVBox(this);
    //Now insert my qworkspace:
    myWorkspace = new QWorkspace(vb);
    setCentralWidget(vb);
    myWorkspace->setScrollBarsEnabled(true);
   
    
    
    
    //Now let's add a docking tabbed window on the left side.
    tabDockWindow = new QDockWindow( QDockWindow::InDock, myWorkspace );
    tabDockWindow->setResizeEnabled( TRUE );
    tabDockWindow->setVerticalStretchable( TRUE );
    addDockWindow( tabDockWindow, DockLeft );
    setDockEnabled( tabDockWindow, DockTop, FALSE );
    setDockEnabled( tabDockWindow, DockBottom, FALSE );
    tabDockWindow->setCloseMode( QDockWindow::Never );
	//setup the tab widget, must be done before creating the 

	tabWidget = new TabManager(tabDockWindow, "tab manager");
	//tabWidget->setMinimumWidth(420);
	tabWidget->setMaximumWidth(500);
	
	//This is just large enough to show the whole width of flow tab, with a scrollbar
	//on right (on windows, linux, and irix)  Using default settings of 
	//qtconfig (10 pt font)
	tabWidget->setMinimumWidth(200);
    tabWidget->setMinimumHeight(500);
	
	
	
	tabDockWindow->setWidget(tabWidget);
	
	//Setup  the session (hence the viz window manager)
	Session::getInstance();
	if (getenv("VAPOR_DEBUG")) {
		MessageReporter* msgRpt = MessageReporter::getInstance();
		msgRpt->setMaxLog(MessageReporter::Info, 100);
		msgRpt->setMaxLog(MessageReporter::Warning, 100);
		msgRpt->setMaxLog(MessageReporter::Error, 100);
		msgRpt->reset("stderr");
	}
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	myVizMgr->createGlobalParams();
	
	

    // actions
    
    fileOpenAction = new QAction( this, "fileOpenAction" );
	fileOpenAction->setEnabled(true);
    //fileOpenAction->setIconSet( QIconSet( QPixmap::fromMimeSource( "fileopen" ) ) );
    fileSaveAction = new QAction( this, "fileSaveAction" );
	fileSaveAction->setEnabled(true);
    //fileSaveAction->setIconSet( QIconSet( QPixmap::fromMimeSource( "filesave" ) ) );
    fileSaveAsAction = new QAction( this, "fileSaveAsAction" );
	fileSaveAsAction->setEnabled(true);
    fileExitAction = new QAction( this, "fileExitAction" );

	editUndoAction = new QAction(this, "editUndoAction");
	editRedoAction = new QAction(this, "editRedoAction");
	editSessionParamsAction = new QAction(this, "editSessionParamsAction");
	editVizFeaturesAction = new QAction(this, "editVizFeaturesAction");
	editUndoAction->setEnabled(false);
	editRedoAction->setEnabled(false);
    
	//whatsThisAction = new QAction(QIconSet(*QWhatsThis::whatsThisButton(0)->pixmap()),"What's This?",SHIFT+Key_F1,0);
	//whatsThisAction = new QAction("What's This?",SHIFT+Key_F1,0);
	whatsThisAction = new QAction(this, "whatsthisaction");
	whatsThisAction->setEnabled(true);

	
    //helpContentsAction = new QAction( this, "helpContentsAction" );
	//helpContentsAction->setEnabled(false);
    //helpIndexAction = new QAction( this, "helpIndexAction" );
	//helpIndexAction->setEnabled(false);
    helpAboutAction = new QAction( this, "helpAboutAction" );
	helpAboutAction->setEnabled(true);
    
    dataBrowse_DataAction = new QAction( this, "dataBrowse_DataAction" );
	dataBrowse_DataAction->setEnabled(false);
    dataConfigure_MetafileAction = new QAction( this, "dataConfigure_MetafileAction" );
	dataConfigure_MetafileAction->setEnabled(false);
    dataLoad_MetafileAction = new QAction( this, "dataLoad_MetafileAction" );
	dataMerge_MetafileAction = new QAction( this, "dataMerge_MetafileAction" );
	dataSave_MetafileAction = new QAction( this, "dataSave_MetafileAction" );
	dataLoad_DefaultMetafileAction = new QAction(this, "dataLoad_DefaultMetafileAction");
	fileNew_SessionAction = new QAction( this, "fileNew_SessionAction" );
	dataExportToIDLAction = new QAction(this, "dataExportToIDLAction");
    
    
    viewLaunch_visualizerAction = new QAction( this, "viewLaunch_visualizerAction" );
	viewStartCaptureAction = new QAction( this, "viewStartCaptureAction" );
	viewEndCaptureAction = new QAction( this, "viewEndCaptureAction" );
	viewSingleCaptureAction = new QAction(this, "viewCaptureSingleAction");
	
    scriptIDL_scriptAction = new QAction( this, "scriptIDL_scriptAction" );
	scriptIDL_scriptAction->setEnabled(false);
    scriptMatlab_scriptAction = new QAction( this, "scriptMatlab_scriptAction" );
	scriptMatlab_scriptAction->setEnabled(false);
    scriptBatchAction = new QAction(this, "scriptBatchAction");
	scriptBatchAction->setEnabled(false);
    
    animationKeyframingAction = new QAction( this, "animationKeyframingAction" );
	animationKeyframingAction->setEnabled(false);
	exportAnimationScriptAction = new QAction(this, "exportAnimationScriptAction");
	exportAnimationScriptAction->setEnabled(false);

	//Create an exclusive action group for the mouse mode toolbars:
	mouseModeActions = new QActionGroup(this, "mouse action group", true);
	//Toolbar buttons:
	QPixmap* wheelIcon = new QPixmap(wheel);
	
	if (!wheelIcon){
		MessageReporter::warningMsg("Unable to obtain image from images/wheel.xpm");
	}
	navigationAction = new QAction("Navigation Mode", *wheelIcon,
		"&Navigation", CTRL+Key_N, mouseModeActions);
	navigationAction->setToggleAction(true);
	navigationAction->setOn(true);

	QPixmap* regionIcon = new QPixmap(cube);
	regionSelectAction = new QAction("Region Select Mode", *regionIcon,
		"&Region", CTRL+Key_R, mouseModeActions);
	regionSelectAction->setToggleAction(true);
	regionSelectAction->setOn(false);
	
	QPixmap* probeIcon = new QPixmap(probe);
	probeAction = new QAction("Data Probe Mode", *probeIcon,
		"&Probe", CTRL+Key_P, mouseModeActions);
	probeAction->setToggleAction(true);
	probeAction->setOn(false);

	QPixmap* rakeIcon = new QPixmap(rake);
	rakeAction = new QAction("Rake Mode", *rakeIcon,
		"&Rake", CTRL+Key_R, mouseModeActions);
	rakeAction->setToggleAction(true);
	rakeAction->setOn(false);

	//QPixmap* lightsIcon = new QPixmap(lightbulb);
	//moveLightsAction = new QAction("Move Lights Mode", *lightsIcon,
	//	"&Lights", CTRL+Key_L, mouseModeActions);
	//moveLightsAction->setToggleAction(true);
	//moveLightsAction->setOn(false);

	//Insert toolbar and actions:
	QPixmap* homeIcon = new QPixmap(home);
	homeAction = new QAction("Go to Home Viewpoint", *homeIcon,
		"&Home", CTRL+Key_H, this);
	QPixmap* sethomeIcon = new QPixmap(sethome);
	sethomeAction = new QAction("Set Home Viewpoint", *sethomeIcon,
		"&SetHome", CTRL+Key_S, this);
	QPixmap* eyeIcon = new QPixmap(eye);
	viewAllAction = new QAction("View All", *eyeIcon,
		"&ViewAll", CTRL+Key_V, this);
	QPixmap* magnifyIcon = new QPixmap(magnify);
	viewRegionAction = new QAction("View Region", *magnifyIcon,
		"&ViewRegion", CTRL+Key_R, this);

	QPixmap* tileIcon = new QPixmap(tiles);
	tileAction = new QAction("Tile Windows", *tileIcon,
		"&Tile", CTRL+Key_T, this);

	QPixmap* cascadeIcon = new QPixmap(cascade);
	cascadeAction = new QAction("Cascade Windows", *cascadeIcon,
		"C&ascade", CTRL+Key_A, this);

	
	

    // toolbars for mouse modes and visualizers
    modeToolBar = new QToolBar( QString(""), this, DockTop); 
	vizToolBar = new QToolBar( QString(""), this, DockTop); 
	vizToolBar->setOffset(2000);
	

	//Add a QComboBox to toolbar to select window
	windowSelector = new VizSelectCombo(vizToolBar, myVizMgr);
	//add mode buttons, left to right:
	navigationAction->addTo(modeToolBar);
	regionSelectAction->addTo(modeToolBar);
	rakeAction->addTo(modeToolBar);
	probeAction->addTo(modeToolBar);


	tileAction->addTo(vizToolBar);
	cascadeAction->addTo(vizToolBar);
	homeAction->addTo(vizToolBar);
	sethomeAction->addTo(vizToolBar);
	viewRegionAction->addTo(vizToolBar);
	viewAllAction->addTo(vizToolBar);

	alignViewCombo = new QComboBox(vizToolBar);
	alignViewCombo->insertItem("Align View");
	alignViewCombo->insertItem("Nearest axis");
	alignViewCombo->insertItem("     + X ");
	alignViewCombo->insertItem("     + Y ");
	alignViewCombo->insertItem("     + Z ");
	alignViewCombo->insertItem("     - X ");
	alignViewCombo->insertItem("     - Y ");
	alignViewCombo->insertItem("Default: - Z ");
	QToolTip::add(alignViewCombo, "Rotate view to an axis-aligned viewpoint,\ncentered on current rotation center.");
	
	
   
    //helpContentsAction->addTo( toolBar );
    //helpIndexAction->addTo( toolBar );
    //helpAboutAction->addTo( toolBar );


    // menubar
    Main_Form = new QMenuBar( this, "Main_Form" );


    File = new QPopupMenu( this );
    fileNew_SessionAction->addTo( File );
    fileOpenAction->addTo( File );
    fileSaveAction->addTo( File );
    fileSaveAsAction->addTo( File );
    fileExitAction->addTo( File );
    Main_Form->insertItem( QString(""), File, 1 );

	Edit = new QPopupMenu(this);
	editUndoAction->addTo(Edit);
	editRedoAction->addTo(Edit);
	editSessionParamsAction->addTo(Edit);
	editVizFeaturesAction->addTo(Edit);
	Main_Form->insertItem( QString(""), Edit, 2 );

    Data = new QPopupMenu( this );
    dataBrowse_DataAction->addTo( Data );
    dataConfigure_MetafileAction->addTo( Data );
	
    dataLoad_MetafileAction->addTo( Data );
	dataLoad_DefaultMetafileAction->addTo( Data );
	dataMerge_MetafileAction->addTo( Data );
	dataSave_MetafileAction->addTo( Data );
	
	dataExportToIDLAction->addTo(Data);

    
    
    Main_Form->insertItem( QString(""), Data, 3 );

    viewMenu = new QPopupMenu(this);
	//Note that the ordering of the following 4 is significant, so that image
	//capture actions correctly activate each other.
	viewLaunch_visualizerAction->addTo(viewMenu);
	viewSingleCaptureAction->addTo(viewMenu);
	viewStartCaptureAction->addTo(viewMenu);
	viewEndCaptureAction->addTo(viewMenu); 
	
    Main_Form->insertItem( QString(""), viewMenu, 4 );

    Script = new QPopupMenu( this );
    scriptIDL_scriptAction->addTo( Script );
    scriptMatlab_scriptAction->addTo( Script );
    scriptBatchAction->addTo(Script);
    Main_Form->insertItem( QString(""), Script, 5 );

    

    Animation = new QPopupMenu( this );
    animationKeyframingAction->addTo( Animation );
	exportAnimationScriptAction->addTo(Animation);
    Main_Form->insertItem( QString(""), Animation, 6 );
    
    helpMenu = new QPopupMenu( this );
    //helpContentsAction->addTo( helpMenu );
    //helpIndexAction->addTo( helpMenu );
	whatsThisAction->addTo(helpMenu);
    helpMenu->insertSeparator();
    helpAboutAction->addTo( helpMenu );
    Main_Form->insertItem( QString(""), helpMenu, 7 );
    



    languageChange();
   
    clearWState( WState_Polished );

    // signals and slots connections
    connect( fileNew_SessionAction, SIGNAL( activated() ), this, SLOT( newSession() ) );
    connect( fileOpenAction, SIGNAL( activated() ), this, SLOT( fileOpen() ) );
    connect( fileSaveAction, SIGNAL( activated() ), this, SLOT( fileSave() ) );
    connect( fileSaveAsAction, SIGNAL( activated() ), this, SLOT( fileSaveAs() ) );
    connect( fileExitAction, SIGNAL( activated() ), this, SLOT( fileExit() ) );

	connect(editUndoAction, SIGNAL(activated()), this, SLOT (undo()));
	connect(editRedoAction, SIGNAL(activated()), this, SLOT (redo()));
	connect(editSessionParamsAction, SIGNAL(activated()), this, SLOT(editSessionParams()));
	connect( editVizFeaturesAction, SIGNAL(activated()), this, SLOT(launchVizFeaturesPanel()));
	connect(Edit, SIGNAL(aboutToShow()), this, SLOT (setupUndoRedoText()));
	

    connect (whatsThisAction, SIGNAL(activated()), SLOT(whatsThis()));
    //connect( helpIndexAction, SIGNAL( activated() ), this, SLOT( helpIndex() ) );
    //connect( helpContentsAction, SIGNAL( activated() ), this, SLOT( helpContents() ) );
    connect( helpAboutAction, SIGNAL( activated() ), this, SLOT( helpAbout() ) );
    
    connect( dataBrowse_DataAction, SIGNAL( activated() ), this, SLOT( browseData() ) );
	connect( dataMerge_MetafileAction, SIGNAL( activated() ), this, SLOT( mergeData() ) );
	connect( dataSave_MetafileAction, SIGNAL( activated() ), this, SLOT( saveMetadata() ) );
	connect( dataLoad_MetafileAction, SIGNAL( activated() ), this, SLOT( loadData() ) );
	connect( dataLoad_DefaultMetafileAction, SIGNAL( activated() ), this, SLOT( defaultLoadData() ) );
	
	connect( dataExportToIDLAction, SIGNAL(activated()), this, SLOT( exportToIDL()));
    
	connect(viewMenu, SIGNAL(aboutToShow()), this, SLOT(initViewMenu()));
    connect( viewLaunch_visualizerAction, SIGNAL( activated() ), this, SLOT( launchVisualizer() ) );
	
	connect( viewStartCaptureAction, SIGNAL( activated() ), this, SLOT( startCapture() ) );
	connect( viewEndCaptureAction, SIGNAL( activated() ), this, SLOT( endCapture() ) );
	connect (viewSingleCaptureAction, SIGNAL(activated()), this, SLOT (captureSingle()));
    
	connect( scriptBatchAction, SIGNAL(activated()), this, SLOT(batchSetup()));

	//Toolbar actions:
	connect (navigationAction, SIGNAL(toggled(bool)), this, SLOT(setNavigate(bool)));
	connect (regionSelectAction, SIGNAL(toggled(bool)), this, SLOT(setRegionSelect(bool)));
	
	connect (probeAction, SIGNAL(toggled(bool)), this, SLOT(setProbe(bool)));
	connect (rakeAction, SIGNAL(toggled(bool)), this, SLOT(setRake(bool)));
	//connect (moveLightsAction, SIGNAL(toggled(bool)), this, SLOT(setLights(bool)));
	connect (cascadeAction, SIGNAL(activated()), myVizMgr, SLOT(cascade()));
	connect (tileAction, SIGNAL(activated()), myVizMgr, SLOT(fitSpace()));
	connect (homeAction, SIGNAL(activated()), myVizMgr, SLOT(home()));
	connect (sethomeAction, SIGNAL(activated()), myVizMgr, SLOT(sethome()));
	connect (viewAllAction, SIGNAL(activated()), myVizMgr, SLOT(viewAll()));
	connect (viewRegionAction, SIGNAL(activated()), myVizMgr, SLOT(viewRegion()));

	connect (alignViewCombo, SIGNAL(activated(int)), myVizMgr, SLOT(alignView(int)));
 
 
	//Now that the tabmgr and the viz mgr exist, hook up the tabs:
	
	//These need to be installed to set the tab pointers in the
	//global params.
	Session::getInstance()->blockRecording();
	

	animationParams();
	viewpoint();
	region();
	launchProbeTab();
	
	launchFlowTab();
	//The last one is in front:
	renderDVR();
	
	//Create one initial visualizer:
	myVizMgr->launchVisualizer();

	Session::getInstance()->unblockRecording();
	
    show();
	VizWinMgr::getInstance()->getDvrRouter()->initTypes();

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

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void MainForm::languageChange()
{
	setCaption( tr( "VAPoR:  NCAR Visualization and Analysis Platform for Research" ) );
    QToolTip::add( tabWidget, tr( "Parameter Settings" ) );
    tabWidget->changeTab( tab, tr( "Tab 1" ) );
    tabWidget->changeTab( tab_2, tr( "Tab 2" ) );

    fileNew_SessionAction->setText( tr( "New Session" ) );
    fileNew_SessionAction->setMenuText( tr( "New Session" ) );
	fileNew_SessionAction->setToolTip("Restart the session with default settings");
    
    fileOpenAction->setText( tr( "&Open Session" ) );
    fileOpenAction->setMenuText( tr( "&Open Session" ) );
    fileOpenAction->setAccel( tr( "Ctrl+O" ) );
	fileOpenAction->setToolTip("Launch a file open dialog to reopen a previously saved session file");
    fileSaveAction->setText( tr( "Save" ) );
    fileSaveAction->setMenuText( tr( "&Save Session" ) );
    fileSaveAction->setAccel( tr( "Ctrl+S" ) );
	fileSaveAction->setToolTip("Launch a file-save dialog to save the state of this session in current session file");
    fileSaveAsAction->setText( tr( "Save As" ) );
    fileSaveAsAction->setMenuText( tr( "Save Session &As..." ) );
    fileSaveAsAction->setAccel( QString::null );
	fileSaveAsAction->setToolTip("Launch a file-save dialog to save the state of this session in another session file");
    fileExitAction->setText( tr( "Exit" ) );
    fileExitAction->setMenuText( tr( "E&xit" ) );
    fileExitAction->setAccel( QString::null );

	editUndoAction->setText( tr( "&Foo" ) );
    editUndoAction->setMenuText( tr( "&Undo" ) );
    editUndoAction->setAccel( tr( "Ctrl+U" ) );
	editUndoAction->setToolTip("Undo the most recent session state change");
	editRedoAction->setText( tr( "&Redo" ) );
    editRedoAction->setMenuText( tr( "&Redo" ) );
    editRedoAction->setAccel( tr( "Ctrl+Y" ) );
	editRedoAction->setToolTip("Redo the last undone session state change");
	editSessionParamsAction->setText( tr("Edit Session Parameters"));
    editSessionParamsAction->setMenuText( tr("&Edit Session Parameters"));
    editSessionParamsAction->setToolTip("Launch a panel for setting session parameters"); 
    
   
	
    helpAboutAction->setText( tr( "About VAPOR" ) );
    helpAboutAction->setMenuText( tr( "About VAPOR" ) );
    helpAboutAction->setToolTip( tr( "Information about VAPOR" ) );
    helpAboutAction->setAccel( QString::null );

	whatsThisAction->setText( tr( "Explain This" ) );
    whatsThisAction->setMenuText( tr( "Explain This" ) );
	whatsThisAction->setToolTip(tr(QString("Click here, then click over an object for context-sensitive help;  ")+
		"Or just click Shift-F1 over the object"));
	whatsThisAction->setAccel(QString::null);
	whatsThisAction->setIconSet(QIconSet(*QWhatsThis::whatsThisButton(0)->pixmap()));
	//whatsThisAction = new QAction(QIconSet(*QWhatsThis::whatsThisButton(0)->pixmap()),"What's This?",SHIFT+Key_F1,0);
	//whatsThisAction = new QAction("What's This?",SHIFT+Key_F1,0);

    dataBrowse_DataAction->setText( tr( "Browse Data" ) );
    dataBrowse_DataAction->setMenuText( tr( "&Browse Data" ) );
	dataBrowse_DataAction->setToolTip("Browse filesystem to examine properties of available datasets"); 
	
	dataExportToIDLAction->setText(tr("Export to IDL"));
	dataExportToIDLAction->setMenuText(tr("&Export to IDL"));
	dataExportToIDLAction->setToolTip("Export current data settings to enable IDL access");
   
    dataConfigure_MetafileAction->setText( tr( "Configure Metafile" ) );
    dataConfigure_MetafileAction->setMenuText( tr( "&Configure Metafile" ) );
	dataConfigure_MetafileAction->setToolTip("Launch a tool to construct the metafile associated with a dataset");
    dataLoad_MetafileAction->setText( tr( "Load a Dataset into Current Session" ) );
    dataLoad_MetafileAction->setMenuText( tr( "&Load a Dataset into Current Session" ) );
	dataLoad_MetafileAction->setToolTip("Specify a data set to be loaded into current session");
	dataLoad_DefaultMetafileAction->setText( tr( "Load a Dataset into &Default Session" ) );
    dataLoad_DefaultMetafileAction->setMenuText( tr( "Load a Dataset into &Default Session" ) );
	dataLoad_DefaultMetafileAction->setToolTip("Specify a data set to be loaded into a new session with default settings");
	dataMerge_MetafileAction->setText( tr( "Merge (Import) a Dataset into Current Session" ) );
    dataMerge_MetafileAction->setMenuText( tr( "&Merge (Import) a Dataset into Current Session" ) );
	dataMerge_MetafileAction->setToolTip("Specify a data set to be merged into current session");
	
	dataSave_MetafileAction->setText( tr( "Save the current Metadata to file" ) );
    dataSave_MetafileAction->setMenuText( tr( "&Save the current Metadata to file" ) );
	dataSave_MetafileAction->setToolTip("Specify a file where the current Metadata will be saved");
	

    viewLaunch_visualizerAction->setText( tr( "New Visualizer" ) );
    viewLaunch_visualizerAction->setMenuText( tr( "&New Visualizer" ) );
	viewLaunch_visualizerAction->setToolTip("Launch a new visualization window");

	editVizFeaturesAction->setText(tr("Edit Visualizer Features"));
	editVizFeaturesAction->setMenuText(tr("Edit Visualizer Features"));
	editVizFeaturesAction->setToolTip(tr("View or change various visualizer settings"));

	viewStartCaptureAction->setText( tr( "Begin image capture sequence " ) );
    viewStartCaptureAction->setMenuText( tr( "&Begin image capture sequence " ) );
	viewStartCaptureAction->setToolTip("Begin saving jpeg image files rendered in current active visualizer");
	
	viewEndCaptureAction->setText( tr( "End image capture" ) );
    viewEndCaptureAction->setMenuText( tr( "&End Image Capture" ) );
	viewEndCaptureAction->setToolTip("End capture of image files in current active visualizer");

	viewSingleCaptureAction->setText( tr( "Single image capture" ) );
    viewSingleCaptureAction->setMenuText( tr( "&Single Image Capture" ) );
	viewSingleCaptureAction->setToolTip("Capture one image from current active visualizer");

    scriptIDL_scriptAction->setText( tr( "Execute IDL script" ) );
    scriptIDL_scriptAction->setMenuText( tr( "Execute &IDL script" ) );
	scriptIDL_scriptAction->setToolTip("Launch an IDL script");
    scriptMatlab_scriptAction->setText( tr( "Execute Matlab script" ) );
    scriptMatlab_scriptAction->setMenuText( tr( "Execute &Matlab script" ) );
	scriptMatlab_scriptAction->setToolTip("Launch a Matlab script in a separate window");
    scriptBatchAction->setText( tr( "Batch setup" ) );
    scriptBatchAction->setMenuText( tr( "&Batch setup" ) );
	scriptBatchAction->setToolTip("Launch a panel to script a Batch Rendering session");
    
    animationKeyframingAction->setText( tr( "Keyframing" ) );
    animationKeyframingAction->setMenuText( tr( "Keyframing" ) );
	animationKeyframingAction->setToolTip("Launch a parameter panel that specifies keyframes plus their timing and transitions between them");
	exportAnimationScriptAction->setText( tr( "Export Animation Script" ) );
    exportAnimationScriptAction->setMenuText( tr( "Export Animation Script" ) );
	exportAnimationScriptAction->setToolTip("Export current animation settings as a script");

   
    vizToolBar->setLabel( tr( "VizTools" ) );
	modeToolBar->setLabel( tr( "Mouse Modes" ) );
    if (Main_Form->findItem(1))
        Main_Form->findItem(1)->setText( tr( "&File" ) );
	if (Main_Form->findItem(2))
        Main_Form->findItem(2)->setText( tr( "&Edit" ) );
    if (Main_Form->findItem(3))
        Main_Form->findItem(3)->setText( tr( "&Data" ) );
    if (Main_Form->findItem(4))
        Main_Form->findItem(4)->setText( tr( "&View" ) );
    if (Main_Form->findItem(5))
        Main_Form->findItem(5)->setText( tr( "&Script" ) );
   
    if (Main_Form->findItem(6))
        Main_Form->findItem(6)->setText( tr( "&Animation" ) );
    if (Main_Form->findItem(7))
        Main_Form->findItem(7)->setText( tr( "&Help" ) );
}




void MainForm::fileOpen()
{

	//This launches a panel that enables the
    //user to choose input session save files, then to
	QString filename = QFileDialog::getOpenFileName(sessionSaveFile,
		"Vapor Session Save Files (*.vss)",
		this,
		"Open Session Dialog",
		"Choose the Session File to restore a session");
	if(filename.length() == 0) return;
		
	
	//Force the name to end with .vss
	if (!filename.endsWith(".vss")){
		filename += ".vss";
	}
	
	ifstream is;
	
	is.open(filename.ascii());

	if (!is){//Report error if you can't open the file
		MessageReporter::errorMsg("Unable to open session file: %s", filename.ascii());
		return;
	}
	//Remember file if load is successful:
	if(Session::getInstance()->loadFromFile(is)){
		sessionSaveFile = filename;
	}
}


void MainForm::fileSave()
{
	//This directly saves the session to the current session save file.
    //It does not prompt the user unless there is an error
	if (!Session::getInstance()->getCurrentMetadata()){
		MessageReporter::warningMsg( "There is no current metadata.  Session state cannot be saved");
		return;
	}
	if (!Session::getInstance()->metadataIsSaved())
		MessageReporter::warningMsg( "Note: The current (merged) Metadata has not been saved. \nIt will be easier to restore this session if the Metadata is also saved.");
	
	ofstream fileout;
	fileout.open(sessionSaveFile.ascii());
	if (! fileout) {
		MessageReporter::errorMsg( "Unable to open session file:\n %s", sessionSaveFile.ascii());
		return;
	}
	
	if (!Session::getInstance()->saveToFile(fileout)){//Report error if can't save to file
		MessageReporter::errorMsg("Failed to write session file: \n %s", sessionSaveFile.ascii());
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
	//Do nothing if there is no metadata:
	if (!Session::getInstance()->getCurrentMetadata()) {
			MessageReporter::errorMsg("There is no Metadata to save in current session");
		return;
	}
	//Warn the user that this file is not portable:
	MessageReporter::warningMsg("Note that the Metadata file to be written is non-portable;\n%s",
		"You may not be able to load the associated data from another system");
	//This directly saves the session to the current vdf directory
   QString filename = QFileDialog::getSaveFileName(Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)",
		this,
		"Save Metadata Dialog",
		"Choose the Metadata filename (in this directory) to save the current metadata");
	if(filename != QString::null){
		//Make sure this filename is still in same directory
		//If not, pop up an explanatory error message and quit.
		int posn = filename.findRev("/");
		QString path = filename.left(posn);
		QString metadataFile = QString(Session::getInstance()->getMetadataFile().c_str());
		if (!metadataFile.contains(path)){
			int mposn = metadataFile.findRev("/");
			QString mpath = metadataFile.left(mposn);
			MessageReporter::errorMsg("Specified directory %s is invalid. \n Metadata must be saved to %s .",path.ascii(), mpath.ascii());
			return;
		}
		//If ok, go ahead and try to save using current DataMgr
		QFileInfo finfo(filename);
		if (finfo.exists()){
			int rc = QMessageBox::warning(0, "Metadata File Exists", QString("OK to replace existing metadata file?\n %1 ").arg(filename), QMessageBox::Ok, 
				QMessageBox::No);
			if (rc != QMessageBox::Ok) return;
		}
		Metadata* md = (Metadata *) Session::getInstance()->getCurrentMetadata();
		std::string stdName = std::string(filename.ascii());
		int rc = md->Write(stdName,0);
		if (rc < 0)MessageReporter::errorMsg( "Unable to save metadata file:\n %s", filename.ascii());
		else {
			Session::getInstance()->setMetadataSaved(true);
			//Save the metadata file name
			Session::getInstance()->getMetadataFile() = filename.ascii();
		}
		return;
	}
}


void MainForm::fileSaveAs()
{
	if (!Session::getInstance()->getCurrentMetadata()){
		MessageReporter::warningMsg( "There is no current metadata.  Session state cannot be saved");
		return;
	}
	
	if ( !Session::getInstance()->metadataIsSaved())
		MessageReporter::warningMsg( "Note: The current (merged) Metadata has not been saved. \n It will be easier to restore this session if the Metadata is also saved.");
	//This launches a panel that enables the
    //user to choose output session save files, saves to it
	QString filename = QFileDialog::getSaveFileName(sessionSaveFile,
		"Vapor Session Save Files (*.vss)",
		this,
		"Save Session Dialog",
		"Choose the output Session File to save current session");

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
	fileout.open(filename.ascii());
	if (! fileout) {
		MessageReporter::errorMsg( "Unable to save to file: \n %s", filename.ascii());
		return;
	}
	
	if (!Session::getInstance()->saveToFile(fileout)){//Report error if can't save to file
		MessageReporter::errorMsg("Failed to save session to: \n %s", filename.ascii());
		fileout.close();
		return;
	}
	fileout.close();
	sessionSaveFile = filename;
}


void MainForm::filePrint()
{

}


void MainForm::fileExit()
{
	close(true);
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
	editUndoAction->setMenuText( undoText );
	editRedoAction->setMenuText( redoText );
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
		QString("Version: ")+
		Version::GetVersionString().c_str());

	QMessageBox::information(this, "Information about VAPOR",versionInfo.ascii());

}
void MainForm::batchSetup(){
    //Here we provide panel to setup batch runs
}


//int showWBInfo(const char* wbfile);

void MainForm::browseData()
{
	//Remember the wb filename
	static QString MDFile("");
//This launches a panel that enables the
    //user to peruse the available data files
    //or metafiles.  Initially it will just select (and open) a vdf file.
	QString filename = QFileDialog::getOpenFileName(MDFile,
		"Metadata Files (*.vdf)",
		this,
		"Browse Data Dialog",
		"Choose a Metadata File");
	if(filename!= QString::null) {
		//Need to update this to browse vdf files
		//showWBInfo(filename.ascii());
		MDFile = filename;
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
	QString filename = QFileDialog::getOpenFileName(Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)",
		this,
		"Load Metadata Dialog",
		"Choose the Metadata File to load into current session");
	if(filename != QString::null){
		QFileInfo fInfo(filename);
		if (fInfo.isReadable() && fInfo.isFile())
			Session::getInstance()->resetMetadata(filename.ascii(), true);
		else MessageReporter::errorMsg("Unable to read metadata file %s", filename.ascii());
	}
	
}
//Merge/Import data into current session
//
void MainForm::mergeData()
{

	//This launches a panel that enables the
    //user to choose input data files, then to
	//merge into current metadata 
	QString filename = QFileDialog::getOpenFileName(Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)",
		this,
		"Merge Metadata Data Dialog",
		"Choose the Metadata File to merge into current session");
	if(filename != QString::null){
		//Check on the timestep offset:
		int defaultOffset = 0;
		SetOffsetDialog* sDialog = new SetOffsetDialog(this, 0, true);
		int activeWinNum = VizWinMgr::getInstance()->getActiveViz();
		if (activeWinNum >= 0){
			defaultOffset = VizWinMgr::getInstance()->getAnimationParams(activeWinNum)->getCurrentFrameNumber();
		}
		sDialog->timestepOffsetSpin->setValue(defaultOffset);
		if (sDialog->exec() != QDialog::Accepted) return;
		int offset = sDialog->timestepOffsetSpin->value();
		if (!Session::getInstance()->resetMetadata(filename.ascii(), false, true, offset)){
			MessageReporter::errorMsg("Unsuccessful metadata merge of %s",filename.ascii());
		}
	} else MessageReporter::errorMsg("Unable to open %s",filename.ascii());
	
}
//Load data into default session
//
void MainForm::defaultLoadData()
{

	//This launches a panel that enables the
    //user to choose input data files, then to
	//create a datamanager using those files
    //or metafiles.  
	QString filename = QFileDialog::getOpenFileName(Session::getInstance()->getMetadataFile().c_str(),
		"Vapor Metadata Files (*.vdf)",
		this,
		"Load Volume Data Dialog",
		"Choose the Metadata File to load into new session");
	if(filename != QString::null){
		Session::getInstance()->resetMetadata(0, false);
		Session::getInstance()->resetMetadata(filename.ascii(), false);
	}
	
}
void MainForm::newSession()
{

	Session::getInstance()->resetMetadata(0, false);
	MessageReporter::getInstance()->resetCounts();
	
}
void MainForm::launchVisualizer()
{
	VizWinMgr::getInstance()->launchVisualizer();
		
}


//This creates the popup to edit session parameters
void MainForm::editSessionParams()
{
	SessionParams sP;
	sP.launch();
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
		QToolTip::add(theProbeTab->attachSeedCheckbox, "Enable continuous updating of the flow using selected point as seed.\nNote: Flow must be enabled to use seed list, and Region must contain the seed");
		QToolTip::add(theProbeTab->addSeedButton,"Click to add the selected point to the seeds for the applicable flow panel.\nNote: Flow must be enabled to use seed list, and Region must contain the seed");
	}
	
	int posn = tabWidget->findWidget(Params::ProbeParamsType);
         
	//Create a new parameter class to work with the widget
		 
	if (posn < 0){
		tabWidget->insertWidget(theProbeTab, Params::ProbeParamsType, true );
	} else {
		tabWidget->moveToFront(Params::ProbeParamsType);
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
		modeStatusWidget = new QLabel("Navigation Mode:  Use left mouse to rotate, right to zoom, middle to translate",this);
		statusBar()->addWidget(modeStatusWidget,2);
		//statusBar()->message("Use right mouse to rotate, left to zoom, middle to translate",10000);
	}
	//resetModeButtons();
}
void MainForm::setLights(bool  on)
{
// Until we implement this, does nothing:
	Session* currentSession = Session::getInstance();
	if (!on && moveLightsAction->isOn()){navigationAction->toggle(); return;}
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
	//Probe mode right now does nothing
	if (!on && probeAction->isOn()){navigationAction->toggle(); return;}
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
void MainForm::setRake(bool on)
{
	
	if (!on && rakeAction->isOn()){navigationAction->toggle(); return;}
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
	if (!on && regionSelectAction->isOn()){navigationAction->toggle(); return;}
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
void MainForm::initViewMenu(){
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* viz = vizWinMgr->getActiveVisualizer();
	int winNum = vizWinMgr->getActiveViz();
	QString vizName = "";
	if(winNum >= 0) vizName = vizWinMgr->getVizWinName(winNum);
	//Disable the start capture if no viz, or if active viz already capturing
	//pos2 is start sequence capture,
	//pos3 is end sequence
	//pos1 is single capture
	int pos1 = viewMenu->idAt(1);
	int pos2 = viewMenu->idAt(2);
	int pos3 = viewMenu->idAt(3);
	if (!viz || viz->getGLWindow()->isCapturing()) {
		viewStartCaptureAction->setMenuText( "&Begin image capture sequence"  );
		viewMenu->setItemEnabled(pos2, false);
		viewSingleCaptureAction->setMenuText("Capture single image");
		viewMenu->setItemEnabled(pos1, false);
	}
	else {// there is a visualizer, but it's not capturing
		viewStartCaptureAction->setMenuText( "&Begin image capture sequence in "+(vizName) );
		viewMenu->setItemEnabled(pos2,true);
		viewSingleCaptureAction->setMenuText("Capture single image of "+(vizName) );
		viewMenu->setItemEnabled(pos1,true);
	}
	
	//disable the end capture if no viz, or if active viz is not capturing
	GLWindow* glWin = viz->getGLWindow();
	if (!viz || !glWin->isCapturing()){
		viewEndCaptureAction->setMenuText( "End capture sequence" );
		viewMenu->setItemEnabled(pos3, false);
	}
	else {
		viewEndCaptureAction->setMenuText("End capture sequence in " +(vizName) );
		viewMenu->setItemEnabled(pos3, true);
	}
	
}

/*
 * Set all the mode buttons off, except navigation
 */
void MainForm::resetModeButtons(){
	navigationAction->setOn(true);
	probeAction->setOn(false);
	regionSelectAction->setOn(false);
	moveLightsAction->setOn(false);
	rakeAction->setOn(false);
}
//Make all the current region/animation settings available to IDL
void MainForm::exportToIDL(){
	Session::getInstance()->exportData();
}
	
//Begin capturing images.
//Launch a file save dialog to specify the names
//Then start file saving mode.
void MainForm::startCapture() {
	QFileDialog fileDialog(Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg Images (*.jpg)",
		this,
		"Start image capture dialog",
		true);  //modal
	fileDialog.move(pos());
	fileDialog.setMode(QFileDialog::AnyFile);
	fileDialog.setCaption("Specify first file name for image capture sequence");
	
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QString s = fileDialog.selectedFile();
	QFileInfo* fileInfo = new QFileInfo(s);
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory(fileInfo->dirPath(true).ascii());
	QString fileBaseName = fileInfo->baseName(true);
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
	QString filePath = fileInfo->dirPath(true) + "/" + fileBaseName;
	//Determine the active window:
	//Turn on "image capture mode" in the current active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	if (viz) {
		viz->getGLWindow()->startCapture(filePath,startFileNum);
		//Provide a popup stating the capture parameters in effect.
		MessageReporter::infoMsg("Image Capture Activated \n Image is being captured to %s",
			filePath.ascii());
		
	} else {
		MessageReporter::errorMsg("Image Capture Error;\nNo active visualizer for capturing images");
	}
	delete fileInfo;
}

//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void MainForm::captureSingle() {
	QFileDialog fileDialog(Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg Images (*.jpg)",
		this,
		"Start image capture dialog",
		true);  //modal
	fileDialog.move(pos());
	fileDialog.setMode(QFileDialog::AnyFile);
	fileDialog.setCaption("Specify single image capture file name");
	fileDialog.resize(450,450);
	if (fileDialog.exec() != QDialog::Accepted) return;
	
	//Extract the path, and the root name, from the returned string.
	QString filename = fileDialog.selectedFile();
    
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory(fileInfo->dirPath(true).ascii());
	
	//Determine the active window:
	//Turn on "image capture mode" in the current active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	if (viz) {
		viz->getGLWindow()->singleCapture(filename);
		//Provide a message stating the capture in effect.
		MessageReporter::infoMsg("Single Image is captured to %s",
			filename.ascii());
		
	} else {
		MessageReporter::errorMsg("Image Capture Error;\nNo active visualizer for capturing image");
	}
}
void MainForm::endCapture(){
	//Turn off capture mode for the current active visualizer (if it is on!)
	//Otherwise indicate that the visualizer is not capturing.
	GLWindow* glWin = VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow();

	if (glWin && glWin->isCapturing()) glWin->stopCapture();
	else {
		MessageReporter::warningMsg("Image Capture Warning;\nCurrent active visualizer is not capturing images");
	}
}
void MainForm::launchVizFeaturesPanel(){
	VizFeatureParams vFP;
	vFP.launch();
}
