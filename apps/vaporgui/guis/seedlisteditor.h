#ifndef SEEDLISTEDITOR_H
#define SEEDLISTEDITOR_H

//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		seedlisteditor.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		December 2005
//
//	Description:	Definition of the SeedListEditor class
//

#include <qdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3HBoxLayout>

class Q3HBoxLayout;
class QPushButton;
class Q3Table;
class Q3VBoxLayout;


namespace VAPoR {

class FlowParams;

class SeedListEditor: public QDialog
{
    Q_OBJECT
public:
    SeedListEditor( int numSeeds, FlowParams*, 
		 QWidget *parent = 0, const char *name = "set data form",
		 bool modal = TRUE, Qt::WFlags f = 0 );
    ~SeedListEditor() {}

public slots:
    void addSeed();
    void deleteSeed();
    void valueChanged( int row, int col );
	
protected slots:
	void currentChanged( int row, int col );
	void accept();

protected:
    Q3Table *table;
    QPushButton *addPushButton;
	QPushButton *deletePushButton;
    QPushButton *okPushButton;
    QPushButton *cancelPushButton;
	void checkPushButton();

protected:
    Q3VBoxLayout *tableButtonBox;
    Q3HBoxLayout *buttonBox;
	FlowParams* myFlowParams;
	bool changed;

};
}; //end namespace VAPoR
#endif
