//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		loadtfdialog.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Feb 2005
//
//	Description:	Implements a dialog for transfer function loading 
//


#include "loadtfdialog.h" 
#include "session.h"
#include <qvariant.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qlabel.h>
#include <qcombobox.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3VBoxLayout>

#include "eventrouter.h"
using namespace VAPoR;

/*
 *  Constructs a LoadTFDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
LoadTFDialog::LoadTFDialog(EventRouter* router,  QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "LoadTFDialog" );
	myRouter = router;
    LoadTFDialogLayout = new Q3HBoxLayout( this, 11, 6, "LoadTFDialogLayout"); 

    layout23 = new Q3VBoxLayout( 0, 0, 6, "layout23"); 

    fileLoadButton = new QPushButton( this, "fileLoadButton" );
    QFont fileLoadButton_font(  fileLoadButton->font() );
    fileLoadButton_font.setPointSize( 8 );
    fileLoadButton->setFont( fileLoadButton_font ); 
	fileLoadButton->setAutoDefault(false);
	QToolTip::add(fileLoadButton,"Click to load a transfer function from a file");
    layout23->addWidget( fileLoadButton );
    spacer26 = new QSpacerItem( 20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout23->addItem( spacer26 );

	//Create a combo box containing the existing transfer function names
	//There should be at least one.
	Session* ses = Session::getInstance();
	assert(ses->getNumTFs() > 0);
	
	//Add a label for combo box
		
	QLabel* comboLabel = new QLabel(this, "combolabel");
	comboLabel->setText(" Existing transfer functions saved in session:");
	comboLabel->setAlignment(Qt::AlignHCenter);
	layout23->addWidget( comboLabel);

	//Create combo box with all tf names.
	savedTFCombo = new QComboBox(false, this);
	layout23->addWidget(savedTFCombo);
	
	//Insert names
	for (int i = 0; i<ses->getNumTFs(); i++){
		//Direct conversion of std::string& to QString doesn't seem to work
		//Maybe std was not enabled when QT was built?
		const QString& st = QString(ses->getTFName(i)->c_str());
		savedTFCombo->insertItem(st, i);
	}
	QToolTip::add(savedTFCombo,"Select from transfer functions currently saved in session");
	//Add a label to show current name
	nameLabel = new QLabel(this, "nameLabel");
	layout23->addWidget(nameLabel);

    sessionLoadButton = new QPushButton( this, "sessionLoadButton" );
    QFont sessionLoadButton_font(  sessionLoadButton->font() );
    sessionLoadButton_font.setPointSize( 8 );
    sessionLoadButton->setFont( sessionLoadButton_font ); 
	layout23->addWidget( sessionLoadButton );
	sessionLoadButton->setAutoDefault(false);
	QToolTip::add(sessionLoadButton, "Click to load selected transfer function currently saved in this session");

	const QString& s = QString(ses->getTFName(0)->c_str());
	//label the first (current) name:
	setTFName(s);
	savedTFCombo->setCurrentItem(0);

    
    spacer27 = new QSpacerItem( 20, 31, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout23->addItem( spacer27 );

    cancelButton = new QPushButton( this, "cancelButton" );
    QFont cancelButton_font(  cancelButton->font() );
    cancelButton_font.setPointSize( 8 );
    cancelButton->setFont( cancelButton_font ); 
    layout23->addWidget( cancelButton );
    LoadTFDialogLayout->addLayout( layout23 );
    languageChange();
    resize( QSize(389, 167).expandedTo(minimumSizeHint()) );
    //clearWState( WState_Polished );

	// signals and slots connections
	// If a name is chosen, the sessionSave button is activated,
	// and the latest text is saved:
	connect(savedTFCombo, SIGNAL(activated(const QString&)),
			this, SLOT(setTFName(const QString&)));

    int rc = connect( fileLoadButton, SIGNAL( clicked() ), this, SLOT( fileLoad()));
    rc = connect( sessionLoadButton, SIGNAL( clicked() ), this, SLOT( sessionLoad()));
    rc = connect( cancelButton, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
LoadTFDialog::~LoadTFDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void LoadTFDialog::languageChange()
{
    setCaption( tr( "Load Transfer Function" ) );
    fileLoadButton->setText( tr( "Load Transfer Function from File" ) );
    sessionLoadButton->setText( tr( "Load Selected Transfer Function in this Session" ) );
    cancelButton->setText( tr( "Cancel" ) );
}
void LoadTFDialog::fileLoad()
{
    done(1);
}

void LoadTFDialog::sessionLoad()
{
	myRouter->sessionLoadTF(loadName);
    done(2);
}
void LoadTFDialog::setTFName(const QString& s){
	loadName = new QString(s.stripWhiteSpace());
	if (loadName->length() == 0) {
		nameLabel->setText( QString( "No Current Transfer Function Name" ));
		sessionLoadButton->setEnabled(false);
		sessionLoadButton->setDefault(false);
		delete loadName;
	}
	else
		nameLabel->setText( QString( "Current TF Name: %1" ).arg( s ) );
		sessionLoadButton->setEnabled(true);
		sessionLoadButton->setDefault(true);
}

