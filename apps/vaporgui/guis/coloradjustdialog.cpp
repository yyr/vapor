//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		coloradjustdialog.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the coloradjustdialog class.  
//

#include "coloradjustdialog.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qvalidator.h>
#include "tfframe.h"
#include "tfeditor.h"
#include "mapperfunction.h"
#include <qcolordialog.h>
using namespace VAPoR;
/*
 *  Constructs a ColorAdjustDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
ColorAdjustDialog::ColorAdjustDialog( QFrame* parent, MapEditor* ed, int index, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
	//Get values from parent:
	tfe = ed;
	pointIndex = index;
	
	newPoint = tfe->getMapperFunction()->colorCtrlPointPosition(index);
    newRGB = tfe->getMapperFunction()->getControlPointRGB(index);
	newXInt = tfe->getMapperFunction()->mapFloatToColorIndex(newPoint);
	changed = false;
	pointModified = false;
	changingFloat = false;
	if ( !name )
	setName( "ColorAdjustDialog" );
    setSizeGripEnabled( TRUE );
    ColorAdjustDialogLayout = new QHBoxLayout( this, 11, 6, "ColorAdjustDialogLayout"); 

    layout10 = new QVBoxLayout( 0, 0, 6, "layout10"); 

    layout6 = new QHBoxLayout( 0, 0, 6, "layout6"); 

    textLabel1 = new QLabel( this, "textLabel1" );
    QFont textLabel1_font(  textLabel1->font() );
    textLabel1_font.setPointSize( 10 );
    textLabel1->setFont( textLabel1_font ); 
    layout6->addWidget( textLabel1 );

    xValueEdit = new QLineEdit( this, "xValueEdit" );
	xValueEdit->setValidator(new QDoubleValidator(-1.e30,1.e30,8,this));
    layout6->addWidget( xValueEdit );
    textLabel2 = new QLabel( this, "textLabel2" );
    QFont textLabel2_font(  textLabel2->font() );
    textLabel2_font.setPointSize( 10 );
    textLabel2->setFont( textLabel2_font ); 
    layout6->addWidget( textLabel2 );

    mapValueEdit = new QLineEdit( this, "mapValueEdit" );
	mapValueEdit->setValidator(new QIntValidator(0,255,this));
    layout6->addWidget( mapValueEdit );
    layout10->addLayout( layout6 );
    spacer9 = new QSpacerItem( 20, 83, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout10->addItem( spacer9 );

    layout9 = new QHBoxLayout( 0, 0, 6, "layout9"); 

    Color = new QLabel( this, "Color" );
    QFont Color_font(  Color->font() );
    Color_font.setPointSize( 10 );
    Color->setFont( Color_font ); 
    Color->setAlignment( int( QLabel::AlignCenter ) );
    layout9->addWidget( Color );
    spacer11 = new QSpacerItem( 31, 21, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout9->addItem( spacer11 );

    colorButton = new QPushButton( this, "colorButton" );
	colorButton->setDefault( false );
	colorButton->setAutoDefault(false);
	colorButton->setPaletteBackgroundColor(newRGB);
	colorButton->setPaletteForegroundColor( QColor( 255, 170, 0));
    layout9->addWidget(colorButton);
    layout10->addLayout( layout9 );
    spacer10 = new QSpacerItem( 20, 82, QSizePolicy::Minimum, QSizePolicy::Expanding );
    layout10->addItem( spacer10 );

    layout7 = new QHBoxLayout( 0, 0, 6, "layout7"); 

    buttonCancel = new QPushButton( this, "buttonCancel" );
    buttonCancel->setDefault( false );
	buttonCancel->setAutoDefault(false);
    layout7->addWidget( buttonCancel );
    spacer8 = new QSpacerItem( 111, 21, QSizePolicy::Expanding, QSizePolicy::Minimum );
    layout7->addItem( spacer8 );

    buttonOk = new QPushButton( this, "buttonOk" );
    
    buttonOk->setDefault( false );
	buttonOk->setAutoDefault(false);
    layout7->addWidget( buttonOk );
    layout10->addLayout( layout7 );
    ColorAdjustDialogLayout->addLayout( layout10 );
    languageChange();
    resize( QSize(366, 187).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );
	
	xValueEdit->setText(QString::number(newPoint));
	mapValueEdit->setText(QString::number(newXInt));


    // signals and slots connections

	int ok = connect( buttonOk, SIGNAL( clicked() ), this, SLOT( finalize() ) );
    assert (ok);
	connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
	ok = connect(colorButton, SIGNAL(clicked()), this, SLOT(pickColor()));
	
	ok =  connect (mapValueEdit, SIGNAL(textChanged(const QString&)), this, SLOT (mapValueChanged(const QString&)));
	assert(ok);
	
	ok = connect(xValueEdit, SIGNAL(lostFocus()), this, SLOT (pointChanged()));
	assert(ok);
	ok = connect(xValueEdit, SIGNAL(returnPressed()), this, SLOT (pointChanged()));
	assert(ok);
	ok = connect(xValueEdit, SIGNAL(textChanged(const QString&)), this, SLOT(pointDirty(const QString&)));
}

/*
 *  Destroys the object and frees any allocated resources
 */
ColorAdjustDialog::~ColorAdjustDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void ColorAdjustDialog::languageChange()
{
    setCaption( tr( "Adjust color control point" ) );
    textLabel1->setText( tr( "Variable value" ) );
    textLabel2->setText( tr( "Mapping value" ) );
	Color->setText( tr( "Color chooser:" ) );
    buttonCancel->setText( tr( "&Cancel" ) );
    buttonCancel->setAccel( QKeySequence( QString::null ) );
    buttonOk->setText( tr( "&OK" ) );
    buttonOk->setAccel( QKeySequence( QString::null ) );
}
/* 
 * make final adjustments before quitting
 */
void ColorAdjustDialog::
finalize(){
	if (!changed ) accept();
	if (pointModified) pointChanged();
	tfe->getMapperFunction()->moveColorControlPoint(pointIndex, newPoint);
	tfe->getMapperFunction()->setControlPointRGB(pointIndex,newRGB);
	tfe->setDirty();
	accept();
}
//When user stops editing, need to validate
void ColorAdjustDialog::
pointChanged(){
	bool fixed = false;
	float pt = xValueEdit->text().toFloat();
	float minVal = tfe->getMapperFunction()->getMinColorMapValue();
	if(pt < minVal) {
		fixed = true;
		pt = minVal;
	}
	float maxVal = tfe->getMapperFunction()->getMaxColorMapValue();
	if(pt >maxVal) {
		fixed = true;
		pt = maxVal;
	}
	if (fixed) xValueEdit->setText(QString::number(pt));
	newPoint = pt;
	int tempInt = tfe->getMapperFunction()->mapFloatToColorIndex(newPoint);
	changingFloat = true;
	mapValueEdit->setText(QString::number(tempInt));
	changingFloat = false;
	changed = true;
}

void ColorAdjustDialog::
mapValueChanged(const QString& qs){
	int tempInt = qs.toInt();
	if (tempInt != newXInt){
		newXInt = tempInt;
		//Don't propagate a change that came
		//from the valueEdit
		if (!changingFloat){
			newPoint = tfe->getMapperFunction()->mapColorIndexToFloat(newXInt);
			xValueEdit->setText(QString::number(newPoint));
		}
		changed = true;
	}
}
/*
 * when the value is changed, just keep track of it.
 */
void ColorAdjustDialog::
pointDirty(const QString& qs){
	newPoint =  qs.toFloat();
	pointModified = true;
	changed = true;
}
void ColorAdjustDialog::
pickColor(){
	QColor newColor = QColorDialog::getColor(newRGB, this);
	if (!newColor.isValid()) return;
	if (newColor.rgb() == newRGB) return;
	newRGB = newColor.rgb();
	colorButton->setPaletteBackgroundColor(newRGB);
	changed = true;
}
