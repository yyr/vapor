//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizmgrdialog.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Defines the VizMgrDialog class
//		This is a QDialog that enables the user to manipulate the
//		size and position of visualizer windows.  To be obsolete
//

#ifndef VIZMGRDLG_H
#define VIZMGRDLG_H

#include <qvariant.h>
#include <qdialog.h>
#include "vizwinmgr.h"


class QVBoxLayout;
class QHBoxLayout;

class QCheckBox;

class QLineEdit;
namespace VAPoR {
class MinMaxCombo;
class VizWinMgr;
class VizMgrDialog : public QDialog
{
    Q_OBJECT

public:
    VizMgrDialog( QWidget* parent, VizWinMgr* vizManager, const char* name = 0, bool modal = TRUE, WFlags fl = 0 );
    ~VizMgrDialog();
	VizWinMgr* getManager() {
		return myVizManager;
	}
    void comboActivated(int winNum, int selection);
	void setCombo(int setting, int viznum);
	
	
	

protected:
	VizWinMgr* myVizManager;
	void minimize(int winNum);
	void maximize(int winNum);
	void normalize(int winNum);
	
	
	
	
	MinMaxCombo* combo[MAXVIZWINS];
	QLineEdit* nameEdit[MAXVIZWINS];
	
	
	
    

protected slots:
    //Methods that arrange the viz windows:
	void cascade();
	void coverRight();

	void fitSpace();

};
};
#endif // VIZMGRDIALOG_H




