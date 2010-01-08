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

#include <QDialog>
class QPushButton;
class QDialogButtonBox;
class QTableWidget;


namespace VAPoR {

class FlowParams;

class SeedListEditor: public QDialog
{
    Q_OBJECT
public:
    SeedListEditor( int numSeeds, FlowParams*, 
		 QWidget *parent = 0);
    ~SeedListEditor() {}

public slots:
    void addSeed();
    void deleteSeed();
    void valueChanged( int row, int col );
	
protected slots:
	void accept();
	void clearTable();

protected:
    QTableWidget *table;
    QPushButton *addSeedButton;
	QPushButton *deleteButton;
    QPushButton *okButton;
    QPushButton *quitButton;
	QPushButton *clearButton;

protected:
    QDialogButtonBox *buttonBox;
	FlowParams* myFlowParams;
	bool changed;

};
}; //end namespace VAPoR
#endif
