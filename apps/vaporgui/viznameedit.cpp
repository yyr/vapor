//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viznameedit.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Implements the VizNameEdit class
//		This fits in the vizmgrdialog, enables the user to edit the
//		names of visualizers.. To be obsoleted?
//
#include "viznameedit.h"
#include <qlineedit.h>
#include "vizwinmgr.h"
#include "vizmgrdialog.h"
#include "vizwin.h"
#include <qwidget.h>
using namespace VAPoR;
VizNameEdit::VizNameEdit(QString& text, VizMgrDialog* parent, int winNum) 
	: QLineEdit(text, parent, "frontcheck")
{
	myWinNum = winNum;
	vizWinMgr = parent->getManager();
	dialog = parent;
	
		
	connect (this, SIGNAL(returnPressed()), this, SLOT(returnPressed()));
}
//This slot gets the activated signal from parent class,
//Just change the window name, and tell the manager:
void
VizNameEdit::returnPressed()
{
	vizWinMgr->setVizWinName(myWinNum, text());

	//*(vizWinMgr->vizName[myWinNum]) = text();
	//(vizWinMgr->getVizWin(myWinNum])->setCaption(text());
	//qWarning(" viznameedit: new caption for window %d", myWinNum);
	//vizWinMgr->nameChanged(*(vizWinMgr->vizName[myWinNum]), myWinNum);
	
}







