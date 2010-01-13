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
#include <QTableWidget>

#define MAX_NUM_INSTANCES 100
namespace VAPoR {
class EventRouter;
}
using namespace VAPoR;
class RowCheckBox;
class InstanceTable : public QTableWidget {
	
	Q_OBJECT

public:
	InstanceTable(QWidget* parent);
	~InstanceTable();
	void rebuild(EventRouter* myRouter);
	void checkEnabledBox(bool val, int instance);

protected:
	
	int selectedInstance;

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
	void changeChecked(int r, int c);
	

signals:
	
	void enableInstance(bool onOff, int instance);
	void changeCurrentInstance(int newCurrent);

};

	

#endif //INSTANCETABLE_H

