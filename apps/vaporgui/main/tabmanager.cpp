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
#include <q3scrollview.h>
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

using namespace VAPoR;
TabManager::TabManager(QWidget* parent, const char* name,  Qt::WFlags f)
	: QTabWidget(parent, name, f)
{
	myParent = parent;
	//Initialize arrays:
	//
	for (int i = 0; i< MAX_WIDGETS; i++){
		widgets[i] = 0;
		widgetTypes[i] = Params::UnknownParamsType;
	}
	haveMultipleViz = false;
	
	int ok = connect(this, SIGNAL(currentChanged(QWidget*)), this, SLOT(newFrontTab(QWidget*)));
	if(!ok) assert (ok);
	currentFrontPage = -1;
}
//Insert a new tabbed widget at the end of the tabs
//
int TabManager::insertWidget(QWidget* wid, Params::ParamType widType, bool selected){
	//qWarning("In insert Widget");
	//Create a QScrollView, put the widget into the scrollview, then
	//Insert the scrollview into the tabs
	//
	Q3ScrollView* myScrollview = new Q3ScrollView(this, "Scrollview");
	//myScrollview->resizeContents(500, 1000);
	//myScrollview->setResizePolicy(QScrollView::Manual);
	insertTab(myScrollview, Params::paramName(widType));
	connect(myScrollview, SIGNAL(verticalSliderReleased()), this, SLOT(tabScrolled()));
	myScrollview->addChild(wid);
	
	int posn = count()-1;
	widgets[posn] = wid;
	//qWarning("inserted widget %s in position %d", name.ascii(), posn);
	widgetTypes[posn] = widType;
	if (selected) {
		setCurrentPage(posn);
	}
	return posn;
}

QWidget*
TabManager::removeWidget(Params::ParamType widgetType){
	
	int posn = findWidget(widgetType);
	if (posn<0) return 0;
	QWidget* foundScroller = page(posn);
	QWidget* foundWidget = widgets[posn];
	for (int j=posn; j<count()-1; j++){
		widgetTypes[j] = widgetTypes[j+1];
		widgets[j] = widgets[j+1];
	}
	removePage(foundScroller);
	return foundWidget;
}


/*
 *Replaces a tabbed widget with another of the same type.
 *Retains front status if needed
 */
void TabManager::
replaceTabWidget(Params::ParamType widgetType, QWidget* newWidget){
	int posn = findWidget(widgetType);
	assert(posn >= 0);
	bool front = (posn == currentPageIndex());
	//Create a QScrollView, put the widget into the scrollview, then
	//Insert the scrollview into the tabs
	//
	Q3ScrollView* myScrollview = new Q3ScrollView(this, "Scrollview");
	myScrollview->setResizePolicy(Q3ScrollView::AutoOne);
	insertTab(myScrollview, Params::paramName(widgetType), posn);
	myScrollview->addChild(newWidget);
	widgets[posn] = newWidget;
	//qWarning("replaced widget %s in position %d", name.ascii(), posn);
	if(front) {
		setCurrentPage(posn);
		show();
	}
	
}

/*
 * Make the specified named panel move to front tab, using
 * a specific visualizer number.  
 */
		
int TabManager::
moveToFront(Params::ParamType widgetType){
	int posn = findWidget(widgetType);
	if (posn < 0) return -1;
	int lastCurrentPage = currentPageIndex();
	if (lastCurrentPage == posn) return posn; //No change!
	setCurrentPage(posn);
	return posn;
}
/*
 *  Find the number of the widget that has the specified name
 */
int 
TabManager::findWidget(Params::ParamType widgetType){
	for (int i = 0; i<count(); i++){
		//qWarning("found widget %s in position %d", widgetNames[i]->ascii(), i);
		if (widgetTypes[i] == widgetType) {
			return i;
		}
	}
	return -1;
}
//Catch any change in the front page:
//
void TabManager::
newFrontTab(QWidget*) {
	int newFrontPosn = currentPageIndex();
	//Don't check, sometimes this method can be used to refresh
	//the existing front tab
	//if (newFrontPosn == currentFrontPage) return;
	Params::ParamType prevType = Params::UnknownParamsType;
	if(currentFrontPage >= 0) prevType = widgetTypes[currentFrontPage];
	currentFrontPage = newFrontPosn;
	//Ignore if we haven't set up tabs yet
	if( widgetTypes[newFrontPosn]== 0) return;
	//Refresh this tab from the corresponding params:
	Params* p = VizWinMgr::getInstance()->getApplicableParams(widgetTypes[newFrontPosn]);
	Params::ParamType newType = p->getParamType();
	if (newType == Params::RegionParamsType){
		RegionEventRouter* rer = VizWinMgr::getInstance()->getRegionRouter();
		rer->updateTab();
	} else if (newType == Params::DvrParamsType){
		DvrEventRouter* der = VizWinMgr::getInstance()->getDvrRouter();
		der->updateTab();
	} else if (newType == Params::IsoParamsType){
		IsoEventRouter* ier = VizWinMgr::getInstance()->getIsoRouter();
		ier->updateTab();
	} else if (newType == Params::ViewpointParamsType){
		ViewpointEventRouter* ver = VizWinMgr::getInstance()->getViewpointRouter();
		ver->updateTab();
	} else if (newType == Params::ProbeParamsType){
		ProbeEventRouter* per = VizWinMgr::getInstance()->getProbeRouter();
		per->updateTab();
	} else if (newType == Params::TwoDDataParamsType){
		TwoDDataEventRouter* per = VizWinMgr::getInstance()->getTwoDDataRouter();
		per->updateTab();
	} else if (newType == Params::TwoDImageParamsType){
		TwoDImageEventRouter* per = VizWinMgr::getInstance()->getTwoDImageRouter();
		per->updateTab();
	} else if (newType == Params::AnimationParamsType){
		AnimationEventRouter* aer = VizWinMgr::getInstance()->getAnimationRouter();
		aer->updateTab();
	} else if (newType == Params::FlowParamsType){
		FlowEventRouter* fer = VizWinMgr::getInstance()->getFlowRouter();
		fer->updateTab();
	}
	else {
		assert(0);
	}
	//Put into history
	TabChangeCommand* cmd = new TabChangeCommand(prevType, widgetTypes[newFrontPosn]);
	Session::getInstance()->addToHistory(cmd);

} 
/*
 *  When the scroll bar is released, call updateTab on current front window
 *  This improves the display in windows, at a cost of a display delay
*/
void TabManager::tabScrolled(){
#ifdef WIN32
	int frontPage = currentPageIndex();
	
	if (frontPage < 0) return;
	Params::ParamType tabType = widgetTypes[frontPage];
	
	EventRouter* eRouter = VizWinMgr::getEventRouter(tabType);
	
	eRouter->refreshTab();
#endif
}
void TabManager::scrollFrontToTop(){
	//Get the front scrollview:
	Q3ScrollView* sv = (Q3ScrollView*)currentPage();
	sv->ensureVisible(0,0);
}
	
