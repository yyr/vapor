//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		minmaxcombo.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:  Implementation of MinMaxCombo class
//
#include "minmaxcombo.h"
#include <qcombobox.h>
#include "vizwinmgr.h"
#include "vizmgrdialog.h"
using namespace VAPoR;

MinMaxCombo::MinMaxCombo(VizMgrDialog* parent, int winNum) 
	: QComboBox(parent, "minmaxcombo")
{
	myWinNum = winNum;
	vizWinMgr = VizWinMgr::getInstance();
	dialog = parent;
	QString* label1 = new QString("Minimize");
	QString* label2 = new QString("Maximize");
	QString* label3 = new QString("Normal");
	insertItem(*label1, -1);
	insertItem(*label2, -1);
	insertItem(*label3, -1);
	if (vizWinMgr->isMinimized(winNum)) setCurrentItem(0);
		else if (vizWinMgr->isMaximized(winNum)) setCurrentItem(1);
		else setCurrentItem(2);
		
	connect (this, SIGNAL(activated(int)), this, SLOT(activated(int)));
}
//This slot gets the activated signal from parent class,
//Tell the manager what happened
//
void
MinMaxCombo::activated(int newSetting)
{
	dialog->comboActivated(myWinNum, newSetting);
}





