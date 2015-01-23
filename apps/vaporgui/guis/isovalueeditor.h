#ifndef ISOVALUEEDITOR_H
#define ISOVALUEEDITOR_H

//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		IsovalueEditor.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		December 2005
//
//	Description:	Definition of the IsovalueEditor class
//

#include <QDialog>
class QPushButton;
class QDialogButtonBox;
class QTableWidget;


namespace VAPoR {

class IsolineParams;

class IsovalueEditor: public QDialog
{
    Q_OBJECT
public:
    IsovalueEditor( int numIsovals, IsolineParams*, 
		 QWidget *parent = 0);
    ~IsovalueEditor() {}

public slots:
    void addIso();
    void deleteIso();
    void valueChanged( int row, int col );
	
protected slots:
	void accept();

protected:
    QTableWidget *table;
    QPushButton *addIsoButton;
	QPushButton *deleteButton;
    QPushButton *okButton;
    QPushButton *quitButton;

protected:
    QDialogButtonBox *buttonBox;
	IsolineParams* myIsolineParams;
	bool changed;

};
}; //end namespace VAPoR
#endif
