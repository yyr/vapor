//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizselectcombo.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Implements the VizSelectCombo class
//		This fits in the main toolbar, enables the user to select
//		the visualizer to activate.  
//
#include "vizselectcombo.h"
#include <qcombobox.h>
#include <qtoolbar.h>
#include <qtooltip.h>
#include "vizwinmgr.h"
#include "assert.h"
using namespace VAPoR;
VizSelectCombo::VizSelectCombo(QToolBar* parent, VizWinMgr* mgr)
	: QComboBox(parent) {
	vizWinMgr = mgr;
	currentActive = -1;
	for (int i = 0; i< MAXVIZWINS; i++){
		winNum[i] = -1;
	}
	//This initially has nothing in it.
	//Hookup signals and slots:
	connect (this, SIGNAL(activated(int)), this, SLOT(activeWin(int)));
	connect (this, SIGNAL(winActivated(int)), vizWinMgr, SLOT(winActivated(int)));
	connect (vizWinMgr, SIGNAL(newViz(QString&, int)), this, SLOT(addWindow(QString&, int)));
	connect (vizWinMgr, SIGNAL(removeViz(int)), this, SLOT(removeWindow(int)));
	connect (vizWinMgr, SIGNAL(activateViz(int)), this, SLOT(setWindowActive(int)));
	connect (vizWinMgr, SIGNAL(changeName(QString&, int)), this, SLOT(setWindowName(QString&, int)));

	setMinimumWidth(150);
	QToolTip::add(this, "Select Active Visualizer");
}

/* 
 * Slots:  connected from vizwinmgr
 */
	/*  Called when a new viz is created:
	 */
void VizSelectCombo:: 
addWindow(QString& windowName, int windowNum){
	//First look to find the right place:
	int i;
	for (i = 0; i<count(); i++){
		if (winNum[i] > windowNum) break;
	}
	//Insert at the specified place:
	insertItem(windowName, i);
	//Move the corresponding numbering up.
	//Note that count has now increased by 1.
	for (int j = i+1; j< count(); j++){
		winNum[j] = winNum[j-1];
	}
	winNum[i] = windowNum;
	currentActive = currentItem();
}
	/* 
	 * Remove specified window from the combobox
	 */

void VizSelectCombo::
removeWindow(int windowNum){
	int i;
	for (i = 0; i< count(); i++){
		if (winNum[i] == windowNum) break;
	}
	assert(i < count());
	removeItem(i);
	for (int j = i; j<count(); j++){
		winNum[j] = winNum[j+1];
	}
	currentActive = currentItem();
}
/* 
 * Select a window when it's been made active
 */
void VizSelectCombo::
setWindowActive(int win){
	if (count() <= 0) return;
	//find which entry this corresponds to:
	int i;
	for (i = 0; i<count(); i++){
		if (winNum[i] == win) break;
	}
	assert (i < count());
	//Avoid generating an event unless there really is an change.
	if (currentItem() != i){
		setCurrentItem(i);
		currentActive = i;
	}
}

	
/*
 * When window name is changed, call this:
 */
void VizSelectCombo::
setWindowName(QString& newName, int windowNum){
	//Find the specified winNum:
	int i;
	for (i = 0; i< count(); i++){
		if (windowNum == winNum[i]) break;
	}
	assert(i<count());
	//Note we can only change the text of the current item
	int currentNum = currentItem();
	if (i != currentNum)
		//Set only the parent version of this:
		QComboBox::setCurrentItem(i);

	setCurrentText(newName);
	if (i != currentNum)
		QComboBox::setCurrentItem(currentNum);
}

/*
 *Convert the active index to the active winNum
 */
void VizSelectCombo::
activeWin(int index){
	int i;
	for (i = 0; i< count(); i++){
		if (winNum[i] == index) break;
	}
	assert(i<count());
	emit (winActivated(i));
}






