//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viznameedit.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Defines the VizNameEdit class
//		This fits in the vizmgrdialog, enables the user to edit the
//		names of visualizers.. To be obsoleted?
//
#ifndef VIZNAMEEDIT_H
#define VIZNAMEEDIT_H

#include <qlineedit.h>

namespace VAPoR {
class VizMgrDialog;
class VizWinMgr;


class VizNameEdit : public QLineEdit{
	
	Q_OBJECT
public:
	VizNameEdit(QString& str, VizMgrDialog* parent, int winNum);

protected:
	int myWinNum;
	VizWinMgr* vizWinMgr;
	VizMgrDialog* dialog;
	
protected slots:
	//This is connected from the parent class, results in manager being notified
	void returnPressed();

};
};
#endif //VIZNAMEEDIT_H



