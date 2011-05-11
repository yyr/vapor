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
#include "GL/glew.h"
#include <qtabwidget.h>
#include <qwidget.h>
#include <QScrollArea>
#include <QMdiSubWindow>
#include <QMdiArea>
#include "tabmanager.h"
#include "assert.h"
#include "viztab.h"
#include "params.h"
#include "command.h"
#include "session.h"
#include "vizwinmgr.h"
#include "regioneventrouter.h"
#include "floweventrouter.h"
#include "dvreventrouter.h"
#include "isoeventrouter.h"
#include "probeeventrouter.h"
#include "viewpointeventrouter.h"
#include "animationeventrouter.h"
#include "twoDdataeventrouter.h"
#include "twoDimageeventrouter.h"
#include "vizselectcombo.h"
#include "mainform.h"

using namespace VAPoR;
vector<long> TabManager::tabOrdering;
TabManager::TabManager(QWidget* parent, const char* ,  Qt::WFlags )
	: QTabWidget(parent)
{
	myParent = parent;
	//Initialize arrays:
	//
	widgets.clear();
	widgetBaseTypes.clear();
	
	haveMultipleViz = false;
	
	int ok = connect(this, SIGNAL(currentChanged(int)), this, SLOT(newFrontTab(int)));
	if(!ok) assert (ok);
	currentFrontPage = -1;
}
//Insert a new tabbed widget at the end of the tabs
//
int TabManager::insertWidget(QWidget* wid, Params::ParamsBaseType widBaseType, bool selected){
	//qWarning("In insert Widget");
	//Create a QScrollView, put the widget into the scrollview, then
	//Insert the scrollview into the tabs
	//
	
	QScrollArea* myScrollArea = new QScrollArea(this);
	//myScrollview->resizeContents(500, 1000);
	//myScrollview->setResizePolicy(QScrollView::Manual);
	insertTab(-1, myScrollArea, QString::fromStdString(Params::paramName(widBaseType)));
	//connect(myScrollArea, SIGNAL(verticalSliderReleased()), this, SLOT(tabScrolled()));
	//myScrollview->addChild(wid);
	myScrollArea->setWidget(wid);
	
	int posn = count()-1;
	
	if (selected) {
		setCurrentIndex(posn);
	}
	return posn;
}

//Add a tabWidget to saved list of widgets:
//
void TabManager::addWidget(QWidget* wid, Params::ParamsBaseType widBaseType){
	
	widgets.push_back(wid);
	widgetBaseTypes.push_back(widBaseType);
	
}




/*
 * Make the specified named panel move to front tab, using
 * a specific visualizer number.  
 */
		
int TabManager::
moveToFront(Params::ParamsBaseType widgetBaseType){

	int posn = findWidget(widgetBaseType);
	if (posn < 0) return -1;
	int lastCurrentPage = currentIndex();
	currentFrontPage = posn;
	if (lastCurrentPage == posn) return posn; //No change!
	setCurrentIndex(posn);
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
 *  Find the number of the widget that has the specified name
 */
int 
TabManager::findWidget(Params::ParamsBaseType widgetBaseType){
	
	for (int i = 0; i<count(); i++){
		//qWarning("found widget %s in position %d", widgetNames[i]->toAscii(), i);
		if (widgetBaseTypes[i] == widgetBaseType) {
			
			return i;
		}
		
	}
	return -1;
}
//Catch any change in the front page:
//
void TabManager::
newFrontTab(int newFrontPosn) {
	
	//Don't check, sometimes this method can be used to refresh
	//the existing front tab
	if (!Session::getInstance()->isRecording()) return;
	Params::ParamsBaseType prevType = 0;
	if(currentFrontPage >= 0) prevType = widgetBaseTypes[currentFrontPage];
	currentFrontPage = newFrontPosn;
	//Ignore if we haven't set up tabs yet
	if(widgets.size() <= newFrontPosn) return;
	//Refresh this tab 
	
	EventRouter* eRouter = VizWinMgr::getEventRouter(widgetBaseTypes[newFrontPosn]);
	eRouter->updateTab();
	
	//Put into history
	TabChangeCommand* cmd = new TabChangeCommand(prevType, widgetBaseTypes[newFrontPosn]);
	Session::getInstance()->addToHistory(cmd);

} 
/*
 *  When the scroll bar is released, call updateTab on current front window
 *  This improves the display in windows, at a cost of a display delay
*/
void TabManager::tabScrolled(){
#ifdef WIN32
	int frontPage = currentIndex();
	
	if (frontPage < 0) return;
	Params::ParamsBaseType tabType = widgetBaseTypes[frontPage];
	EventRouter* eRouter = VizWinMgr::getEventRouter(tabType);
	
	eRouter->refreshTab();
#endif
}
void TabManager::scrollFrontToTop(){
	//Get the front scrollview:

	QScrollArea *sv = (QScrollArea*)currentWidget();
	sv->ensureVisible(0,0);
}
void TabManager::orderTabs(){
	Session::getInstance()->blockRecording();
	clear();
	
	setEnabled(false);
	//find how many tabs are used:
	int numTabs = 0;
	for (int i = 0; i< tabOrdering.size(); i++) if (tabOrdering[i] > 0) numTabs++;
	//Make sure the tabOrdering is valid.  It needs to have a place for all paramsBaseTypes
	if (tabOrdering.size() < Params::GetNumParamsClasses()){
		for (int i = tabOrdering.size(); i< Params::GetNumParamsClasses(); i++)
			tabOrdering.push_back(++numTabs);
	} else if (tabOrdering.size() > Params::GetNumParamsClasses()){
		//If the ordering is too large, revert to the default
		tabOrdering.clear();
		for (int i = 1; i<= Params::GetNumParamsClasses(); i++)
			tabOrdering.push_back(i);
		numTabs = tabOrdering.size();
	}
	assert(tabOrdering.size() == Params::GetNumParamsClasses());
	//Now construct a list of all the ParamsBaseTypes that are used, in the order they are used:
	vector<Params::ParamsBaseType> usedTypes;
	//Go through the tabOrdering, looking in order for each tab position > 0
	//For each tab position, the corresponding type is the offset where it occurs +1
	int nextIndex = 1;
	while (nextIndex <= numTabs) {
		bool found = false;
		for (int i = 0; i < tabOrdering.size(); i++){
			if (tabOrdering[i] == nextIndex){
				usedTypes.push_back(i+1);
				nextIndex++;
				found = true;
				break;
			}
		}
		if(!found) {//bad ordering.  Revert to default:
			tabOrdering.clear();
			usedTypes.clear();
			for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
				tabOrdering.push_back(i);
				usedTypes.push_back(i);
			}
			numTabs = tabOrdering.size();
			break;
		}
	}
	assert(usedTypes.size() == numTabs);
	//Now add the tabs:
	for (int i = 0; i< usedTypes.size(); i++){
		//Find the appropriate widget:
		bool found = false;
		for (int j = 0; j< widgets.size(); j++){
			if (widgetBaseTypes[j] == usedTypes[i]){
				insertWidget(widgets[j],widgetBaseTypes[j],false);
				found = true;
				break;
			}
		}
		if(!found){
			MessageReporter::warningMsg("Params for tab %d not available in this install\n",i);
		}
	}
	currentFrontPage= numTabs-1;
	setCurrentIndex(currentFrontPage);
	Session::getInstance()->unblockRecording();
	setEnabled(true);
	update();
}
