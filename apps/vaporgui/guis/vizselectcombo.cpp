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
#include <QToolBar>
#include <qtooltip.h>

#include "mainform.h"
#include "assert.h"
using namespace VAPoR;
VizSelectCombo::VizSelectCombo(MainForm* mainWin, QToolBar* tBar, VizWinMgr* mgr)
	: QComboBox(mainWin) {
	tBar->addWidget(this);
	vizWinMgr = mgr;
	currentActive = -1;
	winNums.clear();
	insertItem(0,"Create New Visualizer");
	//Remember this one with -100:
	winNums.push_back(-100);
	//This initially has just the "New viz" entry.
	//Hookup signals and slots:
	connect (this, SIGNAL(activated(int)), this, SLOT(activeWin(int)));
	connect (this, SIGNAL(winActivated(int)), vizWinMgr, SLOT(winActivated(int)));
	connect (vizWinMgr, SIGNAL(newViz(QString&, int)), this, SLOT(addWindow(QString&, int)));
	connect (vizWinMgr, SIGNAL(removeViz(int)), this, SLOT(removeWindow(int)));
	connect (vizWinMgr, SIGNAL(activateViz(int)), this, SLOT(setWindowActive(int)));
	connect (vizWinMgr, SIGNAL(changeName(QString&, int)), this, SLOT(setWindowName(QString&, int)));
	connect (this, SIGNAL(newWin()), mainWin, SLOT(launchVisualizer()));

	setMinimumWidth(150);
	setToolTip("Select Active Visualizer or create new one");
}

/* 
 * Slots:  connected from vizwinmgr
 */
	/*  Called when a new viz is created:
	 */
void VizSelectCombo:: 
addWindow(QString& windowName, int windowNum){
	//First look to find the right place; insert it in a gap if necessary.
	int i;
	for (i = 0; i<count()-1; i++){
		if (winNums[i] > windowNum) break;
	}
	//Insert name at the specified place:
	insertItem(i,windowName);
	//Move the corresponding numbering up.
	//Note that count has now (already) increased by 1.
	winNums.push_back(-100);
	for (int j = count()-1; j> i; j--){
		winNums[j] = winNums[j-1];
	}
	winNums[i] = windowNum;
	currentActive = currentIndex();
	if (currentActive < 0) setWindowActive(windowNum);
}
	/* 
	 * Remove specified window from the combobox
	 */

void VizSelectCombo::
removeWindow(int windowNum){
	int i;
	//It should be found in the bottom part of the combo
	for (i = 0; i< count()-1; i++){
		if (winNums[i] == windowNum) break;
	}
	assert(i < count()-1);
	
	removeItem(i);

	//Now count() has been reduced by one
	assert(winNums.size() == count()+1);
	for (int j = i; j<count(); j++){
		winNums[j] = winNums[j+1];
	}
	winNums.pop_back();
	//need to reset the active setting
	int activenum = VizWinMgr::getInstance()->getActiveViz();
	if (activenum >= 0){
		for (int k = 0; k<count() -1; k++){
			if (activenum == winNums[k]){
				setCurrentIndex(k);
				currentActive = k;
				break;
			}
			assert(k < count()-1);
		}
	}
	
	
}
/* 
 * Select a window when it's been made active
 */
void VizSelectCombo::
setWindowActive(int win){
	if (count() <= 0) return;
	//find which entry this corresponds to:
	int i;
	for (i = 0; i<count()-1; i++){
		if (winNums[i] == win) break;
	}
	if (i >= count() - 1){
		assert (i < count()-1);
	}
	//Avoid generating an event unless there really is an change.
	if (currentIndex() != i){
		setCurrentIndex(i);
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
		if (windowNum == winNums[i]) break;
	}
	assert(i<count());
	//Note we can only change the text of the current item
	int currentNum = currentIndex();
	if (i != currentNum)
		//Set only the parent version of this:
		QComboBox::setCurrentIndex(i);

	setItemText(currentNum, newName);
	if (i != currentNum)
		QComboBox::setCurrentIndex(currentNum);
}

/*
 *Convert the active index to the active winNum,
 * or launch a visualizer
 */
void VizSelectCombo::
activeWin(int index){
	
	//If they clicked the end, just create a new visualizer:
	if (index == count()-1) {
		emit (newWin());
		return;
	}
	//Activate the window that is in position index in the list:
	emit (winActivated(winNums[index]));
	
}
void VizSelectCombo::
makeConsistent(int currentViz){
	int indx = currentIndex();
	if (indx >=0 && indx < count()-1 && indx != currentViz){
		activeWin(indx);
	}
}






