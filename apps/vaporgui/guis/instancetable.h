//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		instancetable.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Defines the InstanceTable class
//		This fits in the renderer tabs, enables the user to select
//		copy, delete, and enable instances  
//
#ifndef INSTANCETABLE_H
#define INSTANCETABLE_H
#include <q3table.h>
#include <qcheckbox.h>
#include "params.h"
#define MAX_NUM_INSTANCES 100
namespace VAPoR {
class EventRouter;
}
using namespace VAPoR;
class RowCheckBox;
class InstanceTable : public Q3Table {
	
	Q_OBJECT

public:
	InstanceTable(QWidget* parent, const char* name = 0);
	~InstanceTable();
	void rebuild(EventRouter* myRouter);
	void checkEnabledBox(bool val, int instance);

protected:
	
	int selectedInstance;
	RowCheckBox* checkBoxList[MAX_NUM_INSTANCES];
	int numcheckboxes;
	//Special checkbox class knows its row
	


protected slots:
	//Respond to events in table:
	//Users can: 
	//select (click in) a row, making that the current instance
	//click the delete button, deleting that instance
	//check/uncheck the enable checkbox
	//Type in a new name
	//Select entry in a copy-to combo
	//

	void selectInstance();
	void enableChecked(bool val, int inst);
	

signals:
	
	void enableInstance(bool onOff, int instance);
	void changeCurrentInstance(int newCurrent);

};

class RowCheckBox : public QCheckBox {
	Q_OBJECT
	public:
		RowCheckBox(int row, const QString& text, QWidget* parent) ;
		void setRow(int row) {myRow = row;}
		void makeEmit(bool val) { doEmit = val;}
	public slots:
		void toggleme(bool value);
	signals:
		void toggleRow(bool value, int row);

	protected:
		int myRow;
		bool doEmit;
};


	

#endif //VIZSELECTCOMBO_H

