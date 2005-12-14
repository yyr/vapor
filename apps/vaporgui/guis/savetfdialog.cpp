//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		savetfdialog.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Feb 2005
//
//	Description:	Implements a dialog for transfer function saving
//

#include "savetfdialog.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qmessagebox.h>
#include "session.h"
#include "params.h"
using namespace VAPoR;
/*
 *  Constructs a SaveTFDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
SaveTFDialog::SaveTFDialog(Params* params, QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "SaveTFDialog" );
	//setFocusPolicy(QWidget::TabFocus);
    SaveTFDialogLayout = new QHBoxLayout( this, 11, 6, "SaveTFDialogLayout"); 
	myParams = params;

    layout24 = new QVBoxLayout( 0, 0, 6, "layout24"); 

    fileSaveButton = new QPushButton( this, "fileSaveButton" );
    QFont fileSaveButton_font(  fileSaveButton->font() );
    fileSaveButton_font.setPointSize( 8 );
    fileSaveButton->setFont( fileSaveButton_font ); 
    layout24->addWidget( fileSaveButton );
	fileSaveButton->setAutoDefault(false);
	QToolTip::add(fileSaveButton, "Click to save current transfer function to file");
    
	spacer28 = new QSpacerItem( 20, 41, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout24->addItem( spacer28 );

	//Add a label for combo box
		
	QLabel* comboLabel = new QLabel(this, "combolabel");
	comboLabel->setText("Currently saved Transfer Functions in Session:");
	comboLabel->setAlignment(Qt::AlignHCenter);
	layout24->addWidget( comboLabel);
	
	Session* ses = Session::getInstance();
	
	//Create editable combo box with all tf names.
	savedTFCombo = new QComboBox(true, this);
	
	layout24->addWidget(savedTFCombo);
	
	//Insert existing names
	for (int i = 0; i<ses->getNumTFs(); i++){
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& st = QString(ses->getTFName(i)->c_str());
		savedTFCombo->insertItem(st);
	}
	QToolTip::add(savedTFCombo, "Enter name for transfer function, or\n select an existing name to replace");
	//Add a label to show current name
	nameEditLabel = new QLabel(this, "nameEditLabel");
	nameEditLabel->setText("No Current TF Name");
	layout24->addWidget( nameEditLabel);
	
	savedTFCombo->setInsertionPolicy(QComboBox::NoInsertion);
	//supply a "new name" edit area
	savedTFCombo->setEditText(QString("<New Transfer Function Name>"));

	
	//Follow this with a button, and a spacer

	sessionSaveButton = new QPushButton( this, "sessionSaveButton" );
	QFont sessionSaveButton_font(  sessionSaveButton->font() );
	sessionSaveButton_font.setPointSize( 8 );
	sessionSaveButton->setFont( sessionSaveButton_font ); 
	//Not active until name is specified:
	sessionSaveButton->setEnabled(false);
	QToolTip::add(sessionSaveButton, "click to save transfer function using specified name");
	
	layout24->addWidget( sessionSaveButton );
	
	sessionSaveButton->setAutoDefault(false);
	spacer29 = new QSpacerItem( 21, 41, QSizePolicy::Minimum, QSizePolicy::Expanding );
	layout24->addItem( spacer29 );

	//Finally, the cancel button
    cancelButton = new QPushButton( this, "cancelButton" );
    QFont cancelButton_font(  cancelButton->font() );
    cancelButton_font.setPointSize( 8 );
    cancelButton->setFont( cancelButton_font ); 
    layout24->addWidget( cancelButton );
	
	cancelButton->setAutoDefault(false);

	//Add this to to big layout:
    SaveTFDialogLayout->addLayout( layout24 );
    languageChange();
    resize( QSize(389, 166).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections.
	// If combo is selected, put the text into the line-edit
	
	//When a slot is selected, enable the save button
	
	connect (savedTFCombo, SIGNAL(textChanged(const QString&)),
		this, SLOT(setTFName(const QString&)));
	//FileSave will ignore any other input, launch file save dialog
    connect( fileSaveButton, SIGNAL( clicked() ), this, SLOT( fileSave() ) );


	//When user clicks sessionSave, a warning dialog appears if the
	//existing name matches previously used names
    connect( sessionSaveButton, SIGNAL( clicked() ), this, SLOT( sessionSave()));
	connect( cancelButton, SIGNAL( clicked() ), this, SLOT(reject()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
SaveTFDialog::~SaveTFDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void SaveTFDialog::languageChange()
{
    setCaption( tr( "Save Transfer Function" ) );
    fileSaveButton->setText( tr( "Save Transfer Function to File" ) );
    sessionSaveButton->setText( tr( "Save Transfer Function in this Session" ) );
    cancelButton->setText( tr( "Cancel" ) );
}

void SaveTFDialog::fileSave()
{
    done(1);
}

void SaveTFDialog::sessionSave()
{
	//Popup a warning if the name already exists:
	Session* ses = Session::getInstance();
	std::string stdName(newName->ascii());
	if (ses->isValidTFName(&stdName)){
		int ok = QMessageBox::warning(this, "Replacing Existing Transfer Function",
			QString("OK to Replace existing Transfer Function:\n %1 ?").arg(*newName),
			QMessageBox::Ok, QMessageBox::Cancel);
		if (ok != QMessageBox::Ok) done(2);
	}
	ses->addTF(newName->ascii(), myParams);
    done(2);
}
void SaveTFDialog::setTFName(const QString& s){
	newName = new QString(s.stripWhiteSpace());
	if (newName->length() == 0) {
		nameEditLabel->setText( QString( "No Current Transfer Function Name" ));
		sessionSaveButton->setEnabled(false);
		sessionSaveButton->setDefault(false);
		delete newName;
	}
	else{
		nameEditLabel->setText( QString( "Current TF Name: %1" ).arg( s ) );
		sessionSaveButton->setEnabled(true);
		sessionSaveButton->setDefault(true);
	}
}


