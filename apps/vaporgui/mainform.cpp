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

#include "mainform.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlayout.h>
#include <qtooltip.h>
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
#include <qmessagebox.h>
#include <qdesktopwidget.h>
#include <qvbox.h>
#include <qworkspace.h>
#include <qmessagebox.h>

#include "vizwin.h"
#include "isotab.h"
#include "vizselectcombo.h"
#include "tabmanager.h"
#include "viztab.h"
#include "regiontab.h"
#include "vizwinmgr.h"
#include "dvr.h"
#include "isotab.h"
#include "contourplanetab.h"
#include "animationtab.h"
#include "session.h"
#include "vizmgrdialog.h"
#include "viewpointparams.h"
#include "regionparams.h"
#include "dvrparams.h"
#include "contourparams.h"
#include "isosurfaceparams.h"
#include "animationparams.h"
#include "vcr.h"
#include "assert.h"
#include "command.h"
#include "sessionparameters.h"

//The following are pixmaps that are used in gui:
#include "images/cascade.xpm"
#include "images/tiles.xpm"
#include "images/probe.xpm"
#include "images/wheel.xpm"
#include "images/cube.xpm"
#include "images/planes.xpm"
#include "images/lightbulb.xpm"
#include "images/home.xpm"
#include "images/sethome.xpm"

/*
 *  Constructs a MainForm as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 */
using namespace VAPoR;

MainForm::MainForm( QWidget* parent, const char* name, WFlags )
    : QMainWindow( parent, name, WDestructiveClose )
{
	
	theRegionTab = 0;
	theDvrTab = 0;
	theVizTab = 0;
	theIsoTab = 0;
	theContourTab = 0;
	theAnimationTab = 0;
	myVizMgr = 0;
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
	tabWidget->setMaximumWidth(600);
    tabWidget->setMinimumHeight(500);
	
	
	
	tabDockWindow->setWidget(tabWidget);
	
	//Setup  the session (hence the viz window manager)
	currentSession = new Session(this);
    myVizMgr = currentSession->getWinMgr();

	
	tabWidget->setSession(currentSession);
	//Initial state is set in vizmgr
	currentMouseMode = myVizMgr->getSelectionMode();

    // actions
    
    fileOpenAction = new QAction( this, "fileOpenAction" );
    //fileOpenAction->setIconSet( QIconSet( QPixmap::fromMimeSource( "fileopen" ) ) );
    fileSaveAction = new QAction( this, "fileSaveAction" );
    //fileSaveAction->setIconSet( QIconSet( QPixmap::fromMimeSource( "filesave" ) ) );
    fileSaveAsAction = new QAction( this, "fileSaveAsAction" );
    fileExitAction = new QAction( this, "fileExitAction" );

	editUndoAction = new QAction(this, "editUndoAction");
	editRedoAction = new QAction(this, "editRedoAction");
	editSessionParamsAction = new QAction(this, "editSessionParamsAction");
	editUndoAction->setEnabled(false);
	editRedoAction->setEnabled(false);
    
    helpContentsAction = new QAction( this, "helpContentsAction" );
    helpIndexAction = new QAction( this, "helpIndexAction" );
    helpAboutAction = new QAction( this, "helpAboutAction" );
    
    dataBrowse_DataAction = new QAction( this, "dataBrowse_DataAction" );
    dataConfigure_MetafileAction = new QAction( this, "dataConfigure_MetafileAction" );
    dataLoad_MetafileAction = new QAction( this, "dataLoad_MetafileAction" );
    regionAction = new QAction( this, "regionAction" );
    
    dataProbeAction = new QAction( this, "dataProbeAction" );
    dataContour_PlanesAction = new QAction( this, "dataContour_PlanesAction" );
    dataGenerate_Flow_VisAction = new QAction( this, "dataGenerate_Flow_VisAction" );
    dataCalculate_IsosurfaceAction = new QAction( this, "dataCalculate_IsosurfaceAction" );
    renderDVRAction = new QAction(this, "renderDVRAction");
    
    viewLaunch_visualizerAction = new QAction( this, "viewLaunch_visualizerAction" );
    viewManage_Viz_WindowsAction = new QAction(this, "viewManage_Viz_WindowsAction");
	viewViewpointAction = new QAction( this, "viewViewpointAction" );
    
    scriptJournaling_optionsAction = new QAction( this, "scriptJournaling_optionsAction" );
    scriptIDL_scriptAction = new QAction( this, "scriptIDL_scriptAction" );
    scriptMatlab_scriptAction = new QAction( this, "scriptMatlab_scriptAction" );
    scriptBatchAction = new QAction(this, "scriptBatchAction");
    
    animationKeyframingAction = new QAction( this, "animationKeyframingAction" );
    animationControlAction = new QAction( this, "animationControlAction" );

	//Create an exclusive action group for the mouse mode toolbars:
	mouseModeActions = new QActionGroup(this, "mouse action group", true);
	//Toolbar buttons:
	QPixmap* wheelIcon = new QPixmap(wheel);
	
	if (!wheelIcon){ 
		QMessageBox::warning(this, "vaporgui", 
			"Unable to obtain image from images/wheel.xpm");
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

	QPixmap* planesIcon = new QPixmap(planes);
	contourAction = new QAction("Contour Select Mode", *planesIcon,
		"&Contours", CTRL+Key_C, mouseModeActions);
	contourAction->setToggleAction(true);
	contourAction->setOn(false);

	QPixmap* probeIcon = new QPixmap(probe);
	probeAction = new QAction("Data Probe Mode", *probeIcon,
		"&Probe", CTRL+Key_P, mouseModeActions);
	probeAction->setToggleAction(true);
	probeAction->setOn(false);

	QPixmap* lightsIcon = new QPixmap(lightbulb);
	moveLightsAction = new QAction("Move Lights Mode", *lightsIcon,
		"&Lights", CTRL+Key_L, mouseModeActions);
	moveLightsAction->setToggleAction(true);
	moveLightsAction->setOn(false);

	//Insert toolbar and actions:
	QPixmap* homeIcon = new QPixmap(home);
	homeAction = new QAction("Home", *homeIcon,
		"&Home", CTRL+Key_H, this);
	QPixmap* sethomeIcon = new QPixmap(sethome);
	sethomeAction = new QAction("Set Home", *sethomeIcon,
		"&SetHome", CTRL+Key_S, this);
	QPixmap* tileIcon = new QPixmap(tiles);
	tileAction = new QAction("Tile Windows", *tileIcon,
		"&Tile", CTRL+Key_T, this);

	QPixmap* cascadeIcon = new QPixmap(cascade);
	cascadeAction = new QAction("Cascade Windows", *cascadeIcon,
		"C&ascade", CTRL+Key_A, this);
	

    // toolbars for mouse modes and visualizers
    modeToolBar = new QToolBar( QString(""), this, DockTop); 
	vizToolBar = new QToolBar( QString(""), this, DockTop); 
	

	//Add a QComboBox to toolbar to select window
	windowSelector = new VizSelectCombo(vizToolBar, myVizMgr);
	//add navigation button
	navigationAction->addTo(modeToolBar);
	regionSelectAction->addTo(modeToolBar);
	contourAction->addTo(modeToolBar);
	probeAction->addTo(modeToolBar);
	moveLightsAction->addTo(modeToolBar);
	tileAction->addTo(vizToolBar);
	cascadeAction->addTo(vizToolBar);
	homeAction->addTo(vizToolBar);
	sethomeAction->addTo(vizToolBar);
	
	
   
    //helpContentsAction->addTo( toolBar );
    //helpIndexAction->addTo( toolBar );
    //helpAboutAction->addTo( toolBar );


    // menubar
    Main_Form = new QMenuBar( this, "Main_Form" );


    File = new QPopupMenu( this );
    
    fileOpenAction->addTo( File );
    fileSaveAction->addTo( File );
    fileSaveAsAction->addTo( File );
    fileExitAction->addTo( File );
    Main_Form->insertItem( QString(""), File, 1 );

	Edit = new QPopupMenu(this);
	editUndoAction->addTo(Edit);
	editRedoAction->addTo(Edit);
	editSessionParamsAction->addTo(Edit);
	Main_Form->insertItem( QString(""), Edit, 2 );

    Data = new QPopupMenu( this );
    dataBrowse_DataAction->addTo( Data );
    dataConfigure_MetafileAction->addTo( Data );
    dataLoad_MetafileAction->addTo( Data );
    regionAction->addTo( Data );
    
    
    
    Main_Form->insertItem( QString(""), Data, 3 );

    View = new QPopupMenu(this);
    viewLaunch_visualizerAction->addTo( View );
    viewManage_Viz_WindowsAction->addTo(View );
	viewViewpointAction->addTo( View );
    Main_Form->insertItem( QString(""), View, 4 );

    Script = new QPopupMenu( this );
    scriptJournaling_optionsAction->addTo( Script );
    scriptIDL_scriptAction->addTo( Script );
    scriptMatlab_scriptAction->addTo( Script );
    scriptBatchAction->addTo(Script);
    Main_Form->insertItem( QString(""), Script, 5 );

    Rendering = new QPopupMenu( this );
	renderDVRAction->addTo(Rendering);
    dataProbeAction->addTo( Rendering );
    dataContour_PlanesAction->addTo( Rendering );
    dataGenerate_Flow_VisAction->addTo( Rendering );
    dataCalculate_IsosurfaceAction->addTo( Rendering );
    Main_Form->insertItem( QString(""), Rendering, 6 );

    Animation = new QPopupMenu( this );
    animationKeyframingAction->addTo( Animation );
    animationControlAction->addTo( Animation );
    Main_Form->insertItem( QString(""), Animation, 7 );
    
    helpMenu = new QPopupMenu( this );
    helpContentsAction->addTo( helpMenu );
    helpIndexAction->addTo( helpMenu );
    helpMenu->insertSeparator();
    helpAboutAction->addTo( helpMenu );
    Main_Form->insertItem( QString(""), helpMenu, 8 );
    



    languageChange();
   
    clearWState( WState_Polished );

    // signals and slots connections
   
    connect( fileOpenAction, SIGNAL( activated() ), this, SLOT( fileOpen() ) );
    connect( fileSaveAction, SIGNAL( activated() ), this, SLOT( fileSave() ) );
    connect( fileSaveAsAction, SIGNAL( activated() ), this, SLOT( fileSaveAs() ) );
    connect( fileExitAction, SIGNAL( activated() ), this, SLOT( fileExit() ) );

	connect(editUndoAction, SIGNAL(activated()), this, SLOT (undo()));
	connect(editRedoAction, SIGNAL(activated()), this, SLOT (redo()));
	connect(editSessionParamsAction, SIGNAL(activated()), this, SLOT(editSessionParams()));
	connect(Edit, SIGNAL(aboutToShow()), this, SLOT (setupUndoRedoText()));
	

    
    connect( helpIndexAction, SIGNAL( activated() ), this, SLOT( helpIndex() ) );
    connect( helpContentsAction, SIGNAL( activated() ), this, SLOT( helpContents() ) );
    connect( helpAboutAction, SIGNAL( activated() ), this, SLOT( helpAbout() ) );
    
    connect( dataBrowse_DataAction, SIGNAL( activated() ), this, SLOT( browseData() ) );
	connect( dataLoad_MetafileAction, SIGNAL( activated() ), this, SLOT( loadData() ) );
	
    connect( regionAction, SIGNAL(activated()), this, SLOT(region()));
    
    connect( dataCalculate_IsosurfaceAction, SIGNAL( activated() ), this, SLOT( calcIsosurface() ) );
    connect( renderDVRAction, SIGNAL(activated()), this, SLOT( renderDVR()));
	connect( dataContour_PlanesAction, SIGNAL(activated()), this, SLOT (contourPlanes()));
    
    
    connect( viewLaunch_visualizerAction, SIGNAL( activated() ), this, SLOT( launchVisualizer() ) );
    connect( viewManage_Viz_WindowsAction, SIGNAL(activated()), this, SLOT(manageVizWindows()));
    connect( viewViewpointAction, SIGNAL(activated()), this, SLOT(viewpoint()));

    connect( scriptBatchAction, SIGNAL(activated()), this, SLOT(batchSetup()));

	connect( animationControlAction, SIGNAL(activated()), this, SLOT(animationParams()));
	//Toolbar actions:
	connect (navigationAction, SIGNAL(toggled(bool)), this, SLOT(setNavigate(bool)));
	connect (regionSelectAction, SIGNAL(toggled(bool)), this, SLOT(setRegionSelect(bool)));
	connect (contourAction, SIGNAL(toggled(bool)), this, SLOT(setContourSelect(bool)));
	connect (probeAction, SIGNAL(toggled(bool)), this, SLOT(setProbe(bool)));
	connect (moveLightsAction, SIGNAL(toggled(bool)), this, SLOT(setLights(bool)));
	connect (cascadeAction, SIGNAL(activated()), myVizMgr, SLOT(cascade()));
	connect (tileAction, SIGNAL(activated()), myVizMgr, SLOT(fitSpace()));
	connect (homeAction, SIGNAL(activated()), myVizMgr, SLOT(home()));
	connect (sethomeAction, SIGNAL(activated()), myVizMgr, SLOT(sethome()));
 
	//Now that the tabmgr and the viz mgr exist, hook up the tabs:
	
	//These need to be installed to set the tab pointers in the
	//global params.
	currentSession->blockRecording();
	region();
	animationParams();
	calcIsosurface();
	renderDVR();
	contourPlanes();
	viewpoint();
	
	currentSession->unblockRecording();
	

    show();
}

/*
 *  Destroys the object and frees any allocated resources
 */
MainForm::~MainForm()
{
    //qWarning("mainform destructor start");
	delete currentSession;
    // no need to delete child widgets, Qt does it all for us?? (see closeEvent)
	
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void MainForm::languageChange()
{
    setCaption( tr( "VAPoR Gui Prototype" ) );
    QToolTip::add( tabWidget, tr( "Parameter Settings" ) );
    tabWidget->changeTab( tab, tr( "Tab 1" ) );
    tabWidget->changeTab( tab_2, tr( "Tab 2" ) );
   
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
    
    helpContentsAction->setText( tr( "Contents" ) );
    helpContentsAction->setMenuText( tr( "&Contents..." ) );
    helpContentsAction->setToolTip( tr( "Help Contents" ) );
    helpContentsAction->setAccel( QString::null );
    helpIndexAction->setText( tr( "Index" ) );
    helpIndexAction->setMenuText( tr( "&Index..." ) );
    helpIndexAction->setToolTip( tr( "Help Index" ) );
    helpIndexAction->setAccel( QString::null );
    helpAboutAction->setText( tr( "About" ) );
    helpAboutAction->setMenuText( tr( "&About" ) );
    helpAboutAction->setToolTip( tr( "Help About" ) );
    helpAboutAction->setAccel( QString::null );
    
    dataBrowse_DataAction->setText( tr( "Browse Data" ) );
    dataBrowse_DataAction->setMenuText( tr( "&Browse Data" ) );
	dataBrowse_DataAction->setToolTip("Browse filesystem to examine properties of available datasets"); 
	
   
    dataConfigure_MetafileAction->setText( tr( "Configure Metafile" ) );
    dataConfigure_MetafileAction->setMenuText( tr( "&Configure Metafile" ) );
	dataConfigure_MetafileAction->setToolTip("Launch a tool to construct the metafile associated with a dataset");
    dataLoad_MetafileAction->setText( tr( "Load Metafile" ) );
    dataLoad_MetafileAction->setMenuText( tr( "&Load Metafile" ) );
	dataLoad_MetafileAction->setToolTip("Specify the data set to be loaded via its metafile");
    

	regionAction->setText( tr( "Region Selection" ) );
    regionAction->setMenuText( tr( "Region Selection" ) );
	regionAction->setToolTip("Launch a parameter panel specifying the extent of the region to be visualized");
    
    
    viewLaunch_visualizerAction->setText( tr( "Launch Visualizer" ) );
    viewLaunch_visualizerAction->setMenuText( tr( "L&aunch Visualizer" ) );
	viewLaunch_visualizerAction->setToolTip("Launch a new visualization window");

    viewManage_Viz_WindowsAction->setText( tr("Manage Visualizers"));
    viewManage_Viz_WindowsAction->setMenuText( tr("&Manage Visualizers"));
    viewManage_Viz_WindowsAction->setToolTip("Launch a panel for setting properties of the existing visualizer windows"); 
    dataCalculate_IsosurfaceAction->setText( tr( "Visualize Isosurface" ) );
    dataCalculate_IsosurfaceAction->setMenuText( tr( "Visualize &Isosurface" ) );
	dataCalculate_IsosurfaceAction->setToolTip("Launch a parameter panel specifying properties of an isosurface rendering");
    dataGenerate_Flow_VisAction->setText( tr( "Visualize Flow" ) );
    dataGenerate_Flow_VisAction->setMenuText( tr( "Visualize &Flow" ) );
	dataGenerate_Flow_VisAction->setToolTip("Launch a parameter panel specifying properties of a flow visualization");

    dataProbeAction->setText( tr( "Probe" ) );
    dataProbeAction->setMenuText( tr( "Probe" ) );
	dataProbeAction->setToolTip("Launch a parameter panel specifying the positioning of a data probe");
    dataContour_PlanesAction->setText( tr( "Contour Planes" ) );
    dataContour_PlanesAction->setMenuText( tr( "Contour Planes" ) );
	dataContour_PlanesAction->setToolTip("Launch a parameter panel specifying the placement and coloring of contour planes");
    renderDVRAction->setMenuText( tr( "Volume Rendering"));
    renderDVRAction->setText(tr("Volume Rendering"));
	renderDVRAction->setToolTip("Launch a parameter panel specifying the attributes of a volume renderer"); 
    
    scriptJournaling_optionsAction->setText( tr( "Journaling options" ) );
    scriptJournaling_optionsAction->setMenuText( tr( "&Journaling options" ) );
	scriptJournaling_optionsAction->setToolTip("Turn Journaling on or off, or replay a saved journal");
    scriptIDL_scriptAction->setText( tr( "IDL script" ) );
    scriptIDL_scriptAction->setMenuText( tr( "&IDL script" ) );
	scriptIDL_scriptAction->setToolTip("Launch an IDL session in a separate window");
    scriptMatlab_scriptAction->setText( tr( "Matlab script" ) );
    scriptMatlab_scriptAction->setMenuText( tr( "&Matlab script" ) );
	scriptMatlab_scriptAction->setToolTip("Launch a Matlab session in a separate window");
    scriptBatchAction->setText( tr( "Batch setup" ) );
    scriptBatchAction->setMenuText( tr( "&Batch setup" ) );
	scriptBatchAction->setToolTip("Launch a panel to script a Batch Rendering session");
    
   
    viewViewpointAction->setText( tr( "Viewpoint and Lights" ) );
    viewViewpointAction->setMenuText( tr( "Viewpoint and Lights" ) );
	viewViewpointAction->setToolTip("Launch a parameter panel setting the viewpoint and light positions");
    
    
    animationKeyframingAction->setText( tr( "Keyframing" ) );
    animationKeyframingAction->setMenuText( tr( "Keyframing" ) );
	animationKeyframingAction->setToolTip("Launch a parameter panel that specifies keyframes plus their timing and transitions between them");

    animationControlAction->setText( tr( "Animation Control" ) );
    animationControlAction->setMenuText( tr( "Animation Control" ) );
	animationControlAction->setToolTip("Launch animation control panel"); 
    
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
        Main_Form->findItem(6)->setText( tr( "&Rendering" ) );
    if (Main_Form->findItem(7))
        Main_Form->findItem(7)->setText( tr( "&Animation" ) );
    if (Main_Form->findItem(8))
        Main_Form->findItem(8)->setText( tr( "&Help" ) );
}




void MainForm::fileOpen()
{

}


void MainForm::fileSave()
{

}


void MainForm::fileSaveAs()
{

}


void MainForm::filePrint()
{

}


void MainForm::fileExit()
{
	close(true);
}

void MainForm::undo(){
	currentSession->backupQueue();
}

void MainForm::redo(){
	currentSession->advanceQueue();
}
void MainForm::setupUndoRedoText(){
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

void MainForm::loadData()
{
	static QString MDFile("/cxfs/w4/clyne/wavelet");
	//This launches a panel that enables the
    //user to choose input data files, then to
	//create a datamanager using those files
    //or metafiles.  Initially it will just load a wb file.
	QString filename = QFileDialog::getOpenFileName(MDFile,
		"Vapor Metadata Files (*.vdf)",
		this,
		"Load Data Dialog",
		"Choose the Metadata File");
	if(filename != QString::null){
		
		currentSession->resetMetadata(filename.ascii());

		MDFile = filename;
	}
	
}

void MainForm::launchVisualizer()
{
	myVizMgr->launchVisualizer();
		
}

//This creates the popup to allow viz window management
void MainForm::manageVizWindows()
{
	if (myVizMgr->getNumVisualizers() == 0){
             QMessageBox::information(this, "VAPoR Information", 
                     "There are no visualizer windows", 
                     QMessageBox::Default);
             return;
             }
	VizMgrDialog* vizMgrDlg = new VizMgrDialog((QWidget*)this, myVizMgr);
	vizMgrDlg->exec();
	delete vizMgrDlg;
	
}
//This creates the popup to edit session parameters
void MainForm::editSessionParams()
{
	
	SessionParameters* sessionParamsDlg = new SessionParameters((QWidget*)this);
	QString str;
	sessionParamsDlg->cacheSizeEdit->setText(str.setNum(currentSession->getCacheMB()));
	int rc = sessionParamsDlg->exec();
	if (rc){
		//see if the memory size changed:
		int newVal = sessionParamsDlg->cacheSizeEdit->text().toInt();
		if (newVal > 100 && newVal != currentSession->getCacheMB()){
			currentSession->setCacheMB(newVal);
			QMessageBox::information(this,
				"Information", 
				"Cache size will change at next metadata loading", 
				QMessageBox::Ok);
		}
	}
	delete sessionParamsDlg;
}
/*
 * Method to launch the viewpoint/lights tab into the tabbed dialog
 */
void MainForm::viewpoint()
{
	//Determine which is the current active window:
	int activeViz = myVizMgr->getActiveViz();
	//Get the viz parameter set (local or global) that applies:
	ViewpointParams* myVizParams = myVizMgr->getViewpointParams(activeViz);
	if (!theVizTab){
		theVizTab = new VizTab(tabWidget, "viztab");
		myVizMgr->getViewpointParams(-1)->setTab(theVizTab);
		myVizMgr->hookUpVizTab(theVizTab);
	}
    
	int posn = tabWidget->findWidget(Params::ViewpointParamsType);
		//Create a new parameter class to work with the widget
        myVizParams->updateDialog();
	if (posn < 0){
		tabWidget->insertWidget(theVizTab, Params::ViewpointParamsType,  true );
	} else {
		tabWidget->moveToFront(Params::ViewpointParamsType);
	}
	
}
/*
 * Method that launches the region tab into the tabbed dialog
 */
void MainForm::
region()
{
    //Determine which is the current active window:
	int activeViz = myVizMgr->getActiveViz();
	//Get the region parameter set (local or global) that applies:
	RegionParams* myRegionParams = myVizMgr->getRegionParams(activeViz);
	if (!theRegionTab){
		theRegionTab = new RegionTab(tabWidget, "regiontab");
		//Set the global region tab to this:
		myVizMgr->getRegionParams(-1)->setTab(theRegionTab);
		myVizMgr->hookUpRegionTab(theRegionTab);
	}
	int posn = tabWidget->findWidget(Params::RegionParamsType);
         
	//Create a new parameter class to work with the widget
		
    myRegionParams->updateDialog();
	if (posn < 0){
		tabWidget->insertWidget(theRegionTab, Params::RegionParamsType, true );
	} else {
		tabWidget->moveToFront(Params::RegionParamsType);
	}
	
}
/*
 * Launch the DVR panel
 */
void MainForm::
renderDVR(){
    //Do the DVR panel here
	//Determine which is the current active window:
	int activeViz = myVizMgr->getActiveViz();
	//Get the region parameter set (local or global) that applies:
	DvrParams* myDvrParams = myVizMgr->getDvrParams(activeViz);
	if (!theDvrTab){
		theDvrTab = new Dvr(tabWidget, "DVR");
		myVizMgr->getDvrParams(-1)->setTab(theDvrTab);
		myVizMgr->hookUpDvrTab(theDvrTab);
	}
	int posn = tabWidget->findWidget(Params::DvrParamsType);
         
	//Create a new parameter class to work with the widget
		
    myDvrParams->updateDialog();
	if (posn < 0){
		tabWidget->insertWidget(theDvrTab, Params::DvrParamsType, true );
	} else {
		tabWidget->moveToFront(Params::DvrParamsType);
	}
	
}
/*
 * Launch the animation panel
 */
void MainForm::
animationParams(){
    
	//Determine which is the current active window:
	int activeViz = myVizMgr->getActiveViz();
	//Get the region parameter set (local or global) that applies:
	AnimationParams* myAnimationParams = myVizMgr->getAnimationParams(activeViz);
	if (!theAnimationTab){
		theAnimationTab = new AnimationTab(tabWidget, "Animation");
		myVizMgr->getAnimationParams(-1)->setTab(theAnimationTab);
		myVizMgr->hookUpAnimationTab(theAnimationTab);
	}
	int posn = tabWidget->findWidget(Params::AnimationParamsType);
         
	//Create a new parameter class to work with the widget
		
    myAnimationParams->updateDialog();
	if (posn < 0){
		tabWidget->insertWidget(theAnimationTab, Params::AnimationParamsType, true );
	} else {
		tabWidget->moveToFront(Params::AnimationParamsType);
	}
	
}
/*
 * Launch the Contour Planes panel
 */
void MainForm::
contourPlanes(){
    //Do the contour planes panel here
	//Determine which is the current active window:
	int activeViz = myVizMgr->getActiveViz();
	//Get the region parameter set (local or global) that applies:
	ContourParams* myContourParams = myVizMgr->getContourParams(activeViz);
	if (!theContourTab){
		theContourTab = new ContourPlaneTab(tabWidget, "Contours");
		myVizMgr->getContourParams(-1)->setTab(theContourTab);
		myVizMgr->hookUpContourTab(theContourTab);
	}
	int posn = tabWidget->findWidget(Params::ContourParamsType);
         
	//Create a new parameter class to work with the widget
		
    myContourParams->updateDialog();
	if (posn < 0){
		tabWidget->insertWidget(theContourTab, Params::ContourParamsType, true );
	} else {
		tabWidget->moveToFront(Params::ContourParamsType);
	}
}
/*
 * Launch the Isosurface rendering tab
 */
void MainForm::calcIsosurface()
{
	
	//Determine which is the current active window:
	int activeViz = myVizMgr->getActiveViz();
	//Get the region parameter set (local or global) that applies:
	IsosurfaceParams* myIsoParams = myVizMgr->getIsoParams(activeViz);
	if (!theIsoTab){
		theIsoTab = new IsoTab(tabWidget, "isotab");
		myVizMgr->getIsoParams(-1)->setTab(theIsoTab);
		myVizMgr->hookUpIsoTab(theIsoTab);
	}
	myIsoParams->updateDialog();
	
	int posn = tabWidget->findWidget(Params::IsoParamsType);
	if (posn < 0){
		tabWidget->insertWidget( theIsoTab, Params::IsoParamsType, true );
	} else {
		tabWidget->moveToFront(Params::IsoParamsType);
	}
	
}

/*
 * Respond to toolbar clicks:
 * navigate mode.  Don't change tab menu
 */
void MainForm::setNavigate(bool on)
{
	if (!on) return;
	
	//Only do something if this is an actual change of mode
	if (currentMouseMode != Command::navigateMode){
		myVizMgr->setSelectionMode(Command::navigateMode);
		currentSession->blockRecording();
		viewpoint();
		currentSession->unblockRecording();
		currentSession->addToHistory(new MouseModeCommand(currentMouseMode,  Command::navigateMode, currentSession));
		currentMouseMode = Command::navigateMode;
	}
}
void MainForm::setLights(bool on)
{
	//Test minimizing a dock window
	//setDockEnabled(tabDockWindow, Qt::DockMinimized, true);
	//moveDockWindow(tabDockWindow, Qt::Minimized,true, 0);
	if (!on) return;
	if (currentMouseMode != Command::lightMode){
		myVizMgr->setSelectionMode(Command::lightMode);
		currentSession->addToHistory(new MouseModeCommand(currentMouseMode,  Command::lightMode, currentSession));
		currentMouseMode = Command::lightMode;
	}
	
}
void MainForm::setProbe(bool on)
{
	if (!on) return;
	if (currentMouseMode != Command::probeMode){
		myVizMgr->setSelectionMode(Command::probeMode);
	
		currentSession->addToHistory(new MouseModeCommand(currentMouseMode,  Command::probeMode, currentSession));
		currentMouseMode = Command::probeMode;
	}
}
void MainForm::setRegionSelect(bool on)
{
	//Only respond to toggling on:
	if (!on) return;
	if (currentMouseMode != Command::regionMode){
		myVizMgr->setSelectionMode(Command::regionMode);
		//bring up the tab, but don't put into history:
		currentSession->blockRecording();
		region();
		currentSession->unblockRecording();
		currentSession->addToHistory(new MouseModeCommand(currentMouseMode,  Command::regionMode, currentSession));
		currentMouseMode = Command::regionMode;
	}
}
void MainForm::setContourSelect(bool on)
{
	if (!on) return;
	if (currentMouseMode != Command::contourMode){
		myVizMgr->setSelectionMode(Command::contourMode);
	
		//bring up the tab, but don't put into history:
		currentSession->blockRecording();
		contourPlanes();
		currentSession->unblockRecording();
		currentSession->addToHistory(new MouseModeCommand(currentMouseMode,  Command::contourMode, currentSession));
		currentMouseMode = Command::contourMode;
	}
}
/*
 * Set all the mode buttons off
 */
void MainForm::resetModeButtons(){
	navigationAction->setOn(false);
	probeAction->setOn(false);
	regionSelectAction->setOn(false);
	contourAction->setOn(false);
	moveLightsAction->setOn(false);
}

