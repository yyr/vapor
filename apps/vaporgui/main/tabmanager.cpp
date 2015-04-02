//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tabmanager.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Implements the TabManager class.  This is the QTabWidget
//		that contains a tab for each of the Params classes
//
#include "glutil.h"	// Must be included first
#include <qtabwidget.h>
#include <qwidget.h>
#include <QScrollArea>
#include <QMdiSubWindow>
#include "qstackedwidget.h"
#include <QMdiArea>
#include "tabmanager.h"
#include "assert.h"
#include "viztab.h"
#include "params.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "animationeventrouter.h"
#include "vizselectcombo.h"
#include "mainform.h"
#include "vapor/ControlExecutive.h"
#include "renderholder.h"
#include "arrowparams.h"
#include "isolineparams.h"

using namespace VAPoR;

TabManager::TabManager(QWidget* parent, const char* )
	: QTabWidget(parent)
{
	myParent = parent;
	//Initialize arrays:
	//
	for (int i = 0; i<3; i++){
		widgets[i].clear();
		widgetBaseTypes[i].clear();
		currentFrontPage[i] = -1;
		topWidgets[i] = 0;
		
	}
	
	currentTopTab = -1;
	
	setElideMode(Qt::ElideNone);
	
	topName[0]= "Renderers";
	topName[1]= "Navigation";
	topName[2]= "Settings";

	show();
}
//Insert a new tabbed widget at the end of the tabs, as either a 0=rendererTab, 1=navTab, 2=settingsTab
//
int TabManager::insertWidget(QWidget* wid, Params::ParamsBaseType widBaseType, bool selected){
	//qWarning("In insert Widget");
	//Create a QScrollView, put the widget into the scrollview, then
	//Insert the scrollview into the tabs
	//
	int widType = getTabType(widBaseType);
	//the top-level widget should exist
	assert(topWidgets[widType]);
	
	QScrollArea* myScrollArea = new QScrollArea(topWidgets[widType]);
	//myScrollview->resizeContents(500, 1000);
	//myScrollview->setResizePolicy(QScrollView::Manual);
	
	string tag = ControlExec::GetTagFromType(widBaseType);

	myScrollArea->setWidget(wid);
	assert(widType >0);
	QTabWidget* qtw = (QTabWidget*) topWidgets[widType];
	int posn = qtw->count()-1;
	
	if (selected) {
		qtw->setCurrentIndex(posn);
	}
	
	return posn;
}

//Add a tabWidget to appropriate saved list of widgets:
//
void TabManager::addWidget(QWidget* wid, Params::ParamsBaseType widBaseType){
	int ttype = getTabType(widBaseType);
	widgets[ttype].push_back(wid);
	widgetBaseTypes[ttype].push_back(widBaseType);
}

void TabManager::showRenderWidget(string tag){
	Params::ParamsBaseType typ = ParamsBase::GetTypeFromTag(tag);
	moveToFront(typ);
	for (int i = 0; i<widgets[0].size(); i++){
		if (widgetBaseTypes[0][i] != typ) widgets[0][i]->hide();
		else widgets[0][i]->show();
	}
}
void TabManager::hideRenderWidgets(){
	for (int i = 0; i<widgets[0].size(); i++){
		widgets[0][i]->hide();
	}
}




/*
 * Make the specified named panel move to front tab, using
 * a specific visualizer number.  
 */
		
int TabManager::
moveToFront(Params::ParamsBaseType widgetBaseType){

	int posn = findWidget(widgetBaseType);
	if (posn < 0) return -1;
	int lastCurrentTopPage = currentIndex(); 
	int tabType = getTabType(widgetBaseType);
	if (lastCurrentTopPage == tabType && posn == currentFrontPage[tabType]) return posn;  //No change
	currentFrontPage[tabType] = posn;
	
	setCurrentIndex(tabType);
	if(tabType>0){
		QTabWidget* qtw = (QTabWidget*)topWidgets[tabType];
		qtw->setCurrentIndex(posn);
	} else {
		renderHolder->setCurrentIndex(posn);
	}

	
	//The following code is inserted as a workaround for bug 3138674.
	//This is apparently a QT4.6 bug
	//The problem is that sometimes setCurrentIndex() triggers a QMdiArea event,
	//bringing the wrong visualizer to the front.  The following code sets
	//the current visualizer to be consistent with the window selector.
	MainForm* mainForm = MainForm::getInstance();
	QMdiSubWindow* subWin = mainForm->getMDIArea()->activeSubWindow();
	if (subWin){
		VizWin* viz = (VizWin*)subWin->widget();
		if (viz){
			int activeNum = viz->getWindowNum();
			if (activeNum >=0)
				MainForm::getInstance()->getWindowSelector()->makeConsistent(activeNum);
		}
	}
	return posn;
}
/*
 *  Find the index of the widget in its subTabWidget that has the specified type
 */
int 
TabManager::findWidget(Params::ParamsBaseType widgetBaseType){
	int tabType = getTabType(widgetBaseType);
	for (int i = 0; i< widgets[tabType].size(); i++){
		if (widgetBaseTypes[tabType][i] == widgetBaseType) return i;
	}
	return -1;
}
void TabManager::toggleFrontTabs(Params::ParamsBaseType currentType){
	//Let's see if this is really needed.
	return;
}
//Catch any change in the top tab, update the eventRouter of the sub tab
void TabManager::
newTopTab(int newFrontPosn) {
	if (newFrontPosn < 0) return;
	int subTabIndex = currentFrontPage[newFrontPosn];
	if (subTabIndex < 0) return;
	currentTopTab = newFrontPosn;
	if (newFrontPosn==2) return;
	
	ParamsBase::ParamsBaseType t = widgetBaseTypes[newFrontPosn][subTabIndex];
	
	
	EventRouter* eRouter = VizWinMgr::getEventRouter(t);
	QWidget *wid = dynamic_cast<QWidget*>(eRouter);
	if(wid && wid->isVisible())eRouter->updateTab();
	
	//Put into history!
	

} 
void TabManager::
newFrontTab(int topTab, int newSubPosn) {
	currentTopTab = topTab;
	currentFrontPage[topTab] = newSubPosn;
	EventRouter* eRouter = VizWinMgr::getEventRouter(widgetBaseTypes[topTab][newSubPosn]);
	eRouter->updateTab();
	
} 
void TabManager::newSubTab(int posn){
	int topTab = currentIndex();
	if (topTab < 0) return;
	newFrontTab(topTab,posn);
}



/*
 *  When the scroll bar is released, call updateTab on current front window
 *  This improves the display in windows, at a cost of a display delay
*/
void TabManager::tabScrolled(){
#ifdef WIN32
	int topPage = currentIndex();
	if (topPage < 0) return;
	ParamsBase::ParamsBaseType t = widgetBaseTypes[topPage][currentFrontPage[topPage]];
	EventRouter* eRouter = VizWinMgr::getEventRouter(t);
	
	eRouter->refreshTab();
#endif
}
void TabManager::scrollFrontToTop(){
	//Get the front scrollview:
	/*
	QScrollArea *sv = (QScrollArea*)currentWidget();
	sv->ensureVisible(0,0);
	*/
}

void TabManager::installWidgets(){
	
	clear();
	//Create top widgets.  Tab widgets exist but need to be inserted as tabs, based on their type
	//Type 0 is for renderers, 1 for nav and 2 for settings. 
	//Create top Tab Widgets to hold the nav and settings
	for (int i = 1; i<3; i++) topWidgets[i] = new QTabWidget(this);
	//The renderer tabs are put into a stackedWidget, managed by RenderHolder.
	renderHolder = new RenderHolder(this);
	//Insert the renderer 'tabs' but don't show them yet.
	int rc = renderHolder->addWidget(widgets[0][0], "Barbs_1", ArrowParams::_arrowParamsTag);
	rc = renderHolder->addWidget(widgets[0][1], "Contours_1", IsolineParams::_isolineParamsTag);
	widgets[0][0]->hide();
	widgets[0][1]->hide();
	//Add the renderer tab to the top level TabWidget.
	rc = addTab(renderHolder, topName[0]);
	topWidgets[0] = renderHolder;
	//Add the bottom widgets (eventrouter-based) to the nav and setting tabs:
	for (int topTab = 1; topTab<3; topTab++){
		for (int j = 0; j< widgets[topTab].size(); j++){
			QScrollArea* myScrollArea = new QScrollArea(topWidgets[topTab]);
			ParamsBase::ParamsBaseType t = widgetBaseTypes[topTab][j];
			string tag = ControlExec::GetTagFromType(t);
			QTabWidget* qtw = (QTabWidget*)topWidgets[topTab];
			qtw->addTab(myScrollArea, QString::fromStdString(ControlExec::GetShortName(tag)));
			myScrollArea->setWidget(widgets[topTab][j]);
		}
	}
	//Add all 3 top tabs to this
	for (int widType = 1; widType<3; widType++){ 
		addTab(topWidgets[widType],topName[widType]);
		QTabWidget* qtw = (QTabWidget*)topWidgets[widType];
		qtw->setCurrentIndex(0);
	}
	
	//Start them with the renderer tab showing.
	
	currentTopTab = 0;
	setCurrentIndex(0);
	
	for (int i = 0; i<3; i++) currentFrontPage[i] = 0;

	//Hook up signals
	for (int topTab = 1; topTab<3; topTab++){
		connect(topWidgets[topTab], SIGNAL(currentChanged(int)), this, SLOT(newSubTab(int)));
	}
	connect(this, SIGNAL(currentChanged(int)), this, SLOT(newTopTab(int)));
	return;
	
}

bool TabManager::isFrontTab(QWidget* wid){
	int topTab = currentTopTab;
	
	return (wid == widgets[topTab][currentFrontPage[topTab]]);
}
int TabManager::getTabType(ParamsBase::ParamsBaseType t){
	Params* p = Params::GetDefaultParams(t);
	if (p->isRenderParams()) return 0;
	if (dynamic_cast<RegionParams*>(p)) return 1;
	if (dynamic_cast<AnimationParams*>(p)) return 1;
	if (dynamic_cast<ViewpointParams*>(p)) return 1;
	return 2;
}
void TabManager::refresh(){
	if (currentIndex() == 0) { //Renderer tab
		renderHolder->updateTableWidget();
	} 
	getFrontEventRouter()->updateTab();

}