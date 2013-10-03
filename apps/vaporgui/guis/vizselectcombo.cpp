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
	for (int i = 0; i< MAXVIZWINS; i++){
		winNum[i] = -1;
	}
	insertItem(0,"Create New Visualizer");
	//Remember this one with -100:
	winNum[0] = -100;
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
		if (winNum[i] > windowNum) break;
	}
	//Insert name at the specified place:
	insertItem(i,windowName);
	//Move the corresponding numbering up.
	//Note that count has now (already) increased by 1.
	for (int j = count()-1; j> i; j--){
		winNum[j] = winNum[j-1];
	}
	winNum[i] = windowNum;
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
		if (winNum[i] == windowNum) break;
	}
	assert(i < count()-1);
	removeItem(i);
	//Now count() has already been reduced by one
	assert(winNum[count()] == -100);
	for (int j = i; j<count(); j++){
		winNum[j] = winNum[j+1];
	}
	//need to reset the active setting
	int activenum = VizWinMgr::getInstance()->getActiveViz();
	if (activenum >= 0){
		for (int k = 0; k<count() -1; k++){
			if (activenum == winNum[k]){
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
		if (winNum[i] == win) break;
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
		if (windowNum == winNum[i]) break;
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
	emit (winActivated(winNum[index]));
	
}
void VizSelectCombo::
makeConsistent(int currentViz){
	int indx = currentIndex();
	if (indx >=0 && indx < count()-1 && indx != currentViz){
		activeWin(indx);
	}
}






