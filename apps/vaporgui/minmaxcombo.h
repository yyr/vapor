//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		minmaxcombo.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:  Implementation of MinMaxCombo class
//		Supports a QComboBox in VizMgrDialog.
//		Probably to be removed eventually
//
#ifndef MINMAXCOMBO_H
#define MINMAXCOMBO_H

#include <qcombobox.h>

namespace VAPoR {
class VizMgrDialog;
class VizWinMgr;


class MinMaxCombo : public QComboBox{
	
	Q_OBJECT
public:
	MinMaxCombo(VizMgrDialog* parent, int winNum);
	



protected:
	int myWinNum;
	VizWinMgr* vizWinMgr;
	VizMgrDialog* dialog;
	
protected slots:
	//This is connected from the parent class, results in manager being notified
	void activated(int selection);

};
};
#endif //MINMAXCOMBO_H

