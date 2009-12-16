//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		savetfdialog.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Feb 2005
//
//	Description:	Defines a dialog for transfer function saving
//
  
#ifndef SAVETFDIALOG_H
#define SAVETFDIALOG_H

#include <qvariant.h>
#include <qdialog.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <QLabel>

class Q3VBoxLayout;
class Q3HBoxLayout;
class Q3GridLayout;
class QSpacerItem;
class QPushButton;
class QLineEdit;
class QComboBox;
class QLabel;

namespace VAPoR {
class RenderParams;

class SaveTFDialog : public QDialog
{
    Q_OBJECT

public:
	SaveTFDialog( RenderParams*, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    ~SaveTFDialog();

    QPushButton* fileSaveButton;
    QPushButton* sessionSaveButton;
    QPushButton* cancelButton;
	QLineEdit* tfNameEdit;
	QComboBox* savedTFCombo;
	QString* newName;
	QLabel* textLabel2;
	QLabel* nameEditLabel;

public slots:
    virtual void fileSave();
    virtual void sessionSave();
	

protected:
    Q3HBoxLayout* SaveTFDialogLayout;
    Q3VBoxLayout* layout24;
    QSpacerItem* spacer28;
    QSpacerItem* spacer29;

	VAPoR::RenderParams* myParams;

protected slots:
    virtual void languageChange();
	virtual void setTFName(const QString&);

};
};
#endif // SAVETFDIALOG_H

