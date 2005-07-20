//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		coloradjustdialog.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the coloradjustdialog class.  This
//		is enabled in the transfer function editing window by
//      right-mouse menu, when the user request to adjust colors.
//

#ifndef COLORADJUSTDIALOG_H
#define COLORADJUSTDIALOG_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QSpacerItem;
class QLabel;
class QLineEdit;
class QPushButton;
class TFFrame;
namespace VAPoR {
class TFEditor;

class ColorAdjustDialog : public QDialog
{
    Q_OBJECT

public:
    ColorAdjustDialog( TFFrame* parent, int index, const char* name = 0, bool modal = TRUE, WFlags fl = 0 );
    ~ColorAdjustDialog();
    QLabel* textLabel1;
    QLineEdit* xValueEdit;
    QLabel* textLabel2;
    QLineEdit* mapValueEdit;
    QLabel* Color;
    QPushButton* colorButton;
    QPushButton* buttonCancel;
    QPushButton* buttonOk;

protected:
    QHBoxLayout* ColorAdjustDialogLayout;
    QVBoxLayout* layout10;
    QSpacerItem* spacer9;
    QSpacerItem* spacer10;
    QHBoxLayout* layout6;
    QHBoxLayout* layout9;
    QSpacerItem* spacer11;
    QHBoxLayout* layout7;
    QSpacerItem* spacer8;
	TFEditor* tfe;
	bool changed;
	bool pointModified;
	bool changingFloat;
	QRgb newRGB;
	float newPoint;
	int newXInt;
	int pointIndex;

protected slots:
    virtual void languageChange();
	void pointChanged();
	void pointDirty(const QString&);
	void mapValueChanged(const QString&);
	void finalize();
	void pickColor();

};
};

#endif // COLORADJUSTDIALOG_H
