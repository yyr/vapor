//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		opacadjustdialog.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Defines the opacadjustdialog class.  This
//		is enabled in the transfer function editing window by
//      right-mouse menu, when the user request to adjust opacity
//


#ifndef OPACADJUSTDIALOG_H
#define OPACADJUSTDIALOG_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QLabel;
class QLineEdit;
class QPushButton;
class QFrame;

namespace VAPoR {
class MapEditor;
class OpacAdjustDialog : public QDialog
{
    Q_OBJECT

public:
    OpacAdjustDialog( QFrame* parent, MapEditor* ed, int index, const char* name = 0, bool modal = TRUE, WFlags fl = 0 );
    ~OpacAdjustDialog();
    QLabel* textLabel1;
    QLineEdit* xValueEdit;
    QLabel* textLabel2;
    QLineEdit* mapValueEdit;
    QLabel* Color;
    QLineEdit* opacEdit;
    QPushButton* buttonCancel;
    QPushButton* buttonOk;

protected:
    QHBoxLayout* OpacAdjustDialogLayout;
    QVBoxLayout* layout10;
    QSpacerItem* spacer9;
    QSpacerItem* spacer10;
    QHBoxLayout* layout6;
    QHBoxLayout* layout9;
    QSpacerItem* spacer11;
    QHBoxLayout* layout7;
    QSpacerItem* spacer8;
	MapEditor* tfe;
	bool changed;
	bool pointModified;
	bool changingFloat;
	float newOpac;
	float newPoint;
	int newXInt;
	int pointIndex;

protected slots:
    virtual void languageChange();
	void pointChanged();
	void pointDirty(const QString&);
	void opacChanged(const QString&);
	void mapValueChanged(const QString&);
	void finalize();

};
};

#endif // OPACADJUSTDIALOG_H
