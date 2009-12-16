//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		loadtfdialog.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Feb 2005
//
//	Description:	Defines a dialog for transfer function loading
//

#ifndef LOADTFDIALOG_H
#define LOADTFDIALOG_H

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
class QComboBox;
class QLabel;

namespace VAPoR {
class EventRouter;
class LoadTFDialog : public QDialog
{
    Q_OBJECT

public:
    LoadTFDialog(EventRouter*, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );
    ~LoadTFDialog();

    QPushButton* fileLoadButton;
    QPushButton* sessionLoadButton;
    QPushButton* cancelButton;
	QComboBox* savedTFCombo;
	QLabel* nameLabel;

public slots:
    virtual void fileLoad();
    virtual void sessionLoad();

protected:
    Q3HBoxLayout* LoadTFDialogLayout;
    Q3VBoxLayout* layout23;
    QSpacerItem* spacer26;
    QSpacerItem* spacer27;
	QString* loadName;
	EventRouter* myRouter;

protected slots:
    virtual void languageChange();
	virtual void setTFName(const QString&);

};
};
#endif // LOADTFDIALOG_H
