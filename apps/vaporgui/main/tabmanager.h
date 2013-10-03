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
//Total number of different widgets that can go into tabs
//

//Extend QTabWidget to help manage the widgets associated with tabs
//
class TabManager : public QTabWidget{

	Q_OBJECT

	public:
		TabManager(QWidget* parent, const char* name, Qt::WFlags f = 0);
		
	
		// Insert a widget, it should not already be there.
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

		//Find the position of the specified widget, or -1 if it isn't there.
		//
		int findWidget(Params::ParamsBaseType widgetBaseType);
		
		//Make this the front widget, associate with specified viz
		//return the tab position
		//
		int moveToFront(Params::ParamsBaseType widgetType);
		//Mainform must set the session before can handle any history changes.
		//
		//Determine if a widget is the current front tab
		bool isFrontTab(QWidget* wid){
			int widtype = usedTypes[currentFrontPage];
			return (wid == widgets[widtype-1]);
		}

		EventRouter* getFrontEventRouter() { 
			return VizWinMgr::getInstance()->getEventRouter(usedTypes[currentFrontPage]);
		}
		QPoint tabPos() {
			return (myParent->pos());
		}
		//Make the visible tab scroll to top.
		void scrollFrontToTop();
		virtual QSize sizeHint() const { return QSize(460, 800);}
		virtual void keyPressEvent(QKeyEvent* e){
			e->accept();// This prevents a "beep" from occuring when you press enter on the Mac.
		}
		static const vector<long>& getTabOrdering() {return tabOrdering;}
		static void setTabOrdering(const vector<long>& ordering){
			tabOrdering.clear();
			tabOrdering = ordering;
		}
		//Refresh the tab ordering based on the current values in tabOrdering
		void orderTabs();
		//switch tabs, to force front tab to resize properly
		void toggleFrontTabs(Params::ParamsBaseType currentType);
		Params::ParamsBaseType getTypeInPosition(int posn);
	public slots:
		void newFrontTab(int);
		void tabScrolled();
		
	private:
		//Data structures to store widget info
		//
		//Each value in tabOrdering indicates the position the corresponding widgetBaseType is placed.  Zero indicates that widgetbaseType is not used.
		//If tabOrdering is longer than number of base types, extra values are ignored.  If tabOrdering is shorter than number of base types, the
		//unspecified base types are appended at the end.
		static vector<long> tabOrdering;
		//Following is to lookup a widget or a widgetType based on a tab index:
		vector<ParamsBase::ParamsBaseType> usedTypes;
		vector<QWidget*> widgets;
		vector<ParamsBase::ParamsBaseType> widgetBaseTypes;
		bool haveMultipleViz;
		int currentFrontPage;
		QWidget* myParent;
};
};



#endif //TABMANAGER_H

