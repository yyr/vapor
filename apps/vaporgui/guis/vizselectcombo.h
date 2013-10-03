//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizselectcombo.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Defines the VizSelectCombo class
//		This fits in the main toolbar, enables the user to select
//		the visualizer to activate.  
//
#ifndef VIZSELECTCOMBO_H
#define VIZSELECTCOMBO_H
#include <qcombobox.h>
#include "mainform.h"
#include "vizwinmgr.h"
class QToolBar;

namespace VAPoR {
class VizSelectCombo : public QComboBox{
	
	Q_OBJECT

public:
	VizSelectCombo(MainForm* parent, QToolBar* tBar, VizWinMgr* mgr);
	void makeConsistent(int activeVizNum);
protected:
	//Lookup window num associated with slot
	int winNum[MAXVIZWINS+1];
	VizWinMgr* vizWinMgr;
	int currentActive;
	
	
protected slots:
	// Add a new window to the combobox
	// Use number to maintain order.
	void addWindow(QString& windowName, int windowNum);
	// Remove a window from the combobox
	void removeWindow(int windowNum);
	void setWindowActive(int winNum);
	//When window name is changed, call this:
	void setWindowName(QString& newName, int windowNum);
	//Following slot converts the activated index to the
	//activated window num, to notify the vizWinManager
	void activeWin(int index);

signals:
	//send message that a window number was activated.
	//This results from "activated(index)" signal of parent

	void winActivated(int winNum);
	void newWin();

};
};
#endif //VIZSELECTCOMBO_H

