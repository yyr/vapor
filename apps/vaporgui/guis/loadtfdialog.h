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

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
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
    LoadTFDialog(EventRouter*, QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
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
    QHBoxLayout* LoadTFDialogLayout;
    QVBoxLayout* layout23;
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
