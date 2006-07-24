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

namespace VAPoR {
class Session;
class MainForm;
//Total number of different widgets that can go into tabs
//
#define MAX_WIDGETS 10
//Extend QTabWidget to help manage the widgets associated with tabs
//
class TabManager : public QTabWidget{

	Q_OBJECT

	public:
		TabManager(QWidget* parent, const char* name, WFlags f = 0);
	
	
		// Insert a widget, it should not already be there.
		// Move to front, if selected is true
		// Associate this with specified viz, if viznum >= 0 
		// Associate with all viz, if viznum = -1
		// Ignore any viz association, if viznum = -2;
		// Bring associated viz to front, if selected=true and viznum>=0
		// Return the position
		//
		int insertWidget(QWidget* wid, Params::ParamType widgetType, bool selected);

		// Swap a tabbed panel out with another (of the same type):
		//
		void replaceTabWidget(Params::ParamType widgetType, QWidget* newWidget);

		//Remove a widget from tab dialog.  Don't delete it 
		//
		QWidget* removeWidget(Params::ParamType widgetType);
		
		//Find the position of the specified widget, or -1 if it isn't there.
		//
		int findWidget(Params::ParamType widgetType);
		
		//Make this the front widget, associate with specified viz
		//return the tab position
		//
		int moveToFront(Params::ParamType widgetType);
		//Mainform must set the session before can handle any history changes.
		//
		//Determine if a widget is the current front tab
		bool isFrontTab(QWidget* wid){
			return (wid == widgets[currentFrontPage]);}
	public slots:
		void newFrontTab(QWidget*);

		
	private:
		//Data structures to store widget info
		//
		QWidget* widgets[MAX_WIDGETS];
		Params::ParamType widgetTypes[MAX_WIDGETS];
		bool haveMultipleViz;
		int currentFrontPage;
};
};



#endif //TABMANAGER_H

