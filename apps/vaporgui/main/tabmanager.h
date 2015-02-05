//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tabmanager.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Defines the TabManager class.  This is the QTabWidget
//		that contains a tab for each of the Params classes
//
#ifndef TABMANAGER_H
#define TABMANAGER_H
#include <qtabwidget.h>
#include <qwidget.h>
#include "params.h"
#include "eventrouter.h"

namespace VAPoR {

class MainForm;
class ControlExec;
class RenderHolder;
//Total number of different widgets that can go into tabs
//

//Extend QTabWidget to help manage the widgets associated with tabs
//
class TabManager : public QTabWidget{

	Q_OBJECT

	public:
		TabManager(QWidget* parent, const char* name);
		
	
		// Insert a widget into a sub-tab, it should not already be there.
		// Move to front, if selected is true
		// Associate this with specified viz, if viznum >= 0 
		// Associate with all viz, if viznum = -1
		// Ignore any viz association, if viznum = -2;
		// Bring associated viz to front, if selected=true and viznum>=0
		// Return the position
		//
		int insertWidget(QWidget* wid, Params::ParamsBaseType widgetBaseType, bool selected);

		// Add a widget to the list of widgets
		// Don't actually make it show.
		//
		void addWidget(QWidget* wid, Params::ParamsBaseType widgetBaseType);

		// Show the render widget corresponding to a particular tag:
		void showRenderWidget(string tag);

		//Find the position of the specified widget in subTab, or -1 if it isn't there.
		//
		int findWidget(Params::ParamsBaseType widgetBaseType);
		
		//Make this the front widget, of associated subTab, associate with specified viz
		//return the tab position in subTab
		//
		int moveToFront(Params::ParamsBaseType widgetType);
		//Mainform must set the session before can handle any history changes.
		//
		//Determine if a widget is the current front tab in its subtab
		bool isFrontTab(QWidget* wid);
		
		EventRouter* getFrontEventRouter() { 
			return VizWinMgr::getInstance()->getEventRouter(widgetBaseTypes[currentTopTab][currentFrontPage[currentTopTab]]);
		}
		
		//Make the visible tab scroll to top.
		void scrollFrontToTop();
		virtual QSize sizeHint() const { return QSize(460, 800);}
		virtual void keyPressEvent(QKeyEvent* e){
			e->accept();// This prevents a "beep" from occuring when you press enter on the Mac.
		}
		
		//During initialization, put all the widgets in the appropriate tabs.
		void installWidgets();
		//switch tabs, to force front tab to resize properly
		void toggleFrontTabs(Params::ParamsBaseType currentType);
		
		QWidget* getSubTabWidget(int widType){
			return topWidgets[widType];
		}
		int getTabType(ParamsBase::ParamsBaseType);
		void newFrontTab(int topType, int subPosn);
	public slots:
		void newTopTab(int);
		void newSubTab(int);
		
		void tabScrolled();
		
	private:
		//Data structures to store widget info
		vector<QWidget*> widgets[3];
		vector<ParamsBase::ParamsBaseType> widgetBaseTypes[3];
		QWidget* topWidgets[3];
		QString topName[3];
		int currentFrontPage[3];
		int currentTopTab;
		QWidget* myParent;
		RenderHolder* renderHolder;
		
};
};



#endif //TABMANAGER_H

