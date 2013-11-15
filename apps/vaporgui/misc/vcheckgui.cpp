#include "vcheckgui.h"

#include <qboxlayout.h>

vCheckGUI::vCheckGUI(char* info, bool* poptout)
{
	optout = poptout;

	infoLabel = new QLabel(info, this);
	wantBox = new QCheckBox("opt out of future notifications", this);
	okButton = new QPushButton("Ok", this);
	connect(okButton, SIGNAL(pressed(void)), this, SLOT(whenOkayIsPressed(void)));

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(infoLabel);
	mainLayout->addWidget(wantBox);
	mainLayout->addWidget(okButton);
	setLayout(mainLayout);

	setWindowTitle("VAPoR: New Version Available!");
}

void vCheckGUI::whenOkayIsPressed()
{
	*optout = (wantBox->checkState() == Qt::Checked);
	this->close();
}
