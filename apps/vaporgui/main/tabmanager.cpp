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
#include <qtabwidget.h>
#include <qwidget.h>
#include <qscrollview.h>
#include "tabmanager.h"
#include "assert.h"
#include "viztab.h"
#include "params.h"
#include "command.h"
#include "session.h"
#include "vizwinmgr.h"
using namespace VAPoR;
TabManager::TabManager(QWidget* parent, const char* name,  WFlags f)
	: QTabWidget(parent, name, f)
{
	//Initialize arrays:
	//
	for (int i = 0; i< MAX_WIDGETS; i++){
		widgets[i] = 0;
		widgetTypes[i] = Params::UnknownParamsType;
	}
	haveMultipleViz = false;
	
	int ok = connect(this, SIGNAL(currentChanged(QWidget*)), this, SLOT(newFrontTab(QWidget*)));
	assert (ok);
	currentFrontPage = -1;
}
//Insert a new tabbed widget at the end of the tabs
//
int TabManager::insertWidget(QWidget* wid, Params::ParamType widType, bool selected){
	//qWarning("In insert Widget");
	//Create a QScrollView, put the widget into the scrollview, then
	//Insert the scrollview into the tabs
	//
	QScrollView* myScrollview = new QScrollView(this, "Scrollview");
	//myScrollview->resizeContents(500, 1000);
	//myScrollview->setResizePolicy(QScrollView::Manual);
	insertTab(myScrollview, Params::paramName(widType));
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
	QScrollView* myScrollview = new QScrollView(this, "Scrollview");
	myScrollview->setResizePolicy(QScrollView::AutoOne);
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
	if (newFrontPosn == currentFrontPage) return;
	Params::ParamType prevType = Params::UnknownParamsType;
	if(currentFrontPage >= 0) prevType = widgetTypes[currentFrontPage];
	currentFrontPage = newFrontPosn;
	//Refresh this tab from the corresponding params:
	Params* p = VizWinMgr::getInstance()->getApplicableParams(widgetTypes[newFrontPosn]);
	p->updateDialog();
	//Put into history
	TabChangeCommand* cmd = new TabChangeCommand(prevType, widgetTypes[newFrontPosn]);
	Session::getInstance()->addToHistory(cmd);

} 
	
