//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vizmgrdialog.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2004
//
//	Description:	Implements the VizMgrDialog class
//		This is a QDialog that enables the user to manipulate the
//		size and position of visualizer windows.  To be obsolete
//

#include "vizmgrdialog.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qimage.h>
#include <qpixmap.h>
#include "vizwinmgr.h"
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include "minmaxcombo.h"
#include "vizwin.h"
#include "viznameedit.h"

/*
 *  Constructs a VizMgrDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
using namespace VAPoR;
VizMgrDialog::VizMgrDialog( QWidget* parent, VizWinMgr* manager, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
		setName( "VizMgrDialog" );
	
	myVizManager = manager;

	// Create a VH layout for the whole dialog
	QVBoxLayout* mainLayout = new QVBoxLayout(this, 11, 6, "mainLayout");
	
	//Default positioning of this dialog box
    setGeometry( QRect( 24, 48, 535, 222 ) );
	
	
	
	
	//Go through the visualizers, one row of dialog for each:
	for (int i = 0; i<MAXVIZWINS; i++){
		if (myVizManager->getVizWin(i) == 0) continue;
		//Create an HBoxLayout:
		QHBoxLayout* rowLayout = new QHBoxLayout(0,0,6,"rowlayout");
		//Create text specifying the window number
		QString* text = new QString("Window No. ");
		(*text) += QString::number(i);
		QLabel* textLabel = new QLabel(*text, this);
		textLabel->setMinimumSize( QSize( 50, 0 ) );
		rowLayout ->addWidget(textLabel);
		
		//Create line edit for viz name:
		
		VizNameEdit* vizNameEdit = new VizNameEdit(*myVizManager->getVizWinName(i), this, i);
		rowLayout->addWidget(vizNameEdit);
		nameEdit[i] = vizNameEdit;
		
		QPushButton* frontButton = new QPushButton("Front", this);
                connect(frontButton, SIGNAL(clicked()), myVizManager->getVizWin(i), SLOT(setFocus()));
                
		rowLayout->addWidget(frontButton);
		
		
		MinMaxCombo* minMaxCombo = new MinMaxCombo(this, i);
		combo[i] = minMaxCombo;
		rowLayout->addWidget(minMaxCombo);
		
		//QCheckBox* checkHidden = new QCheckBox("Hidden", this);
		//checkHidden->setChecked(myVizManager->isHidden[i]);
		//hiddenCheck[i] = checkHidden;
		//rowLayout->addWidget(checkHidden);
		
		mainLayout->addLayout(rowLayout);
	}
	//Now add a line of buttons:
	
	QHBoxLayout* rowLayout = new QHBoxLayout(0,0,6, "buttonLayout");
	
	QPushButton* cascadeButton = new QPushButton(*(new QString("Cascade")), this);
	rowLayout->addWidget(cascadeButton);
	connect(cascadeButton, SIGNAL(clicked()), this, SLOT(cascade()));
	
	QPushButton* fitButton = new QPushButton(*(new QString("Fit Space")), this);
	rowLayout->addWidget(fitButton);
	connect(fitButton, SIGNAL(clicked()), this, SLOT(fitSpace()));
	
	
	QPushButton* fullButton = new QPushButton(*(new QString("Cover All")), this);
	rowLayout->addWidget(fullButton);
	connect(fullButton, SIGNAL(clicked()), this, SLOT(coverRight()));
	
	QPushButton* quitButton = new QPushButton(*(new QString("Quit")), this);
	rowLayout->addWidget(quitButton);
	connect(quitButton, SIGNAL(clicked()), this, SLOT(close())); 
	
	mainLayout->addLayout(rowLayout);
	
	
	resize( QSize(600, 290).expandedTo(minimumSizeHint()) );//?
    clearWState( WState_Polished );//?
		
  
	myVizManager->setDialog(this);
}

/*
 *  Destroys the object and frees any allocated resources
 */
VizMgrDialog::~VizMgrDialog()
{
    //qWarning("start vizmgrdialog destructor");
    // no need to delete child widgets, Qt does it all for us
	myVizManager->setDialog(0);
    
}

//Response to dialog events
void
VizMgrDialog::minimize(int winNum){
	//Set the state in the manager
	myVizManager->minimize(winNum);
	
	//minimize the window
	((QWidget*)myVizManager->getVizWin(winNum))->showMinimized();
}
void
VizMgrDialog::maximize(int winNum){
	//Set the state in the manager
	myVizManager->maximize(winNum);
	
	//First normalize, then maximize!  This seems to work on KDE:
	((QWidget*)myVizManager->getVizWin(winNum))->showNormal();
	//maximize the window
	((QWidget*)myVizManager->getVizWin(winNum))->showMaximized();
	
	
}
void
VizMgrDialog::normalize(int winNum){
	//Set the state in the manager
	myVizManager->normalize(winNum);
	
	((QWidget*)myVizManager->getVizWin(winNum))->showNormal();
	
	
}
//This occurs when someone changes the dialog selection box
void 
VizMgrDialog::comboActivated(int winNum, int selection)
{
	if (selection == 0) { //minimize
		minimize(winNum);
	} else if (selection == 1) { //maximize
		maximize(winNum);
	} else { //setNormalSize
		normalize(winNum);
	}
}
void 
VizMgrDialog::setCombo(int setting, int viznum){
	//qWarning("setting combo %d to %d", viznum, setting);
	combo[viznum]->setCurrentItem(setting);
	combo[viznum]->update();
	//repaint();
}



//Methods that arrange the viz windows:
void 
VizMgrDialog::cascade(){
	myVizManager->cascade();
}
void 
VizMgrDialog::coverRight(){
	myVizManager->coverRight();
}

void 
VizMgrDialog::fitSpace(){
	myVizManager->fitSpace();
}

	

	
