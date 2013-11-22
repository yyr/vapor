#ifndef VCHECKGUI_H
#define VCHECKGUI_H

#include <QDialog>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>

class vCheckGUI : public QDialog
{
	Q_OBJECT
public:
	vCheckGUI(char* info, bool* optout);

private:
	QLabel* infoLabel;
	QCheckBox* wantBox;
	QPushButton* okButton;
	bool* optout;

private slots:
	void whenOkayIsPressed();
};

#endif
