#include "seedlisteditor.h"

#include <qcolordialog.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qtable.h>
#include <qlineedit.h>

#include "flowparams.h"

using namespace VAPoR;

SeedListEditor::SeedListEditor( int numSeeds, FlowParams* fp,
			  QWidget* parent,  const char* name,
			  bool modal, WFlags f )
    : QDialog( parent, name, modal, f )

{

    setCaption( "Edit Seed Point List" );
    resize( 180, 220 );
	myFlowParams = fp;
    tableButtonBox = new QVBoxLayout( this, 11, 6, "seed editor layout" );

    table = new QTable( this, "seed table" );
    table->setNumCols( 4 );
    table->setNumRows( numSeeds );
    
    table->setColumnWidth( 0, 60 );
    table->setColumnWidth( 1, 60 ); 
    table->setColumnWidth( 2, 60 );
    table->setColumnWidth( 3, 60 );
	table->setColumnStretchable(0,true);
	table->setColumnStretchable(1,true);
	table->setColumnStretchable(2,true);
	table->setColumnStretchable(3,true);
   
    QHeader *th = table->horizontalHeader();
    th->setLabel( 0, "X Coord" );
    th->setLabel( 1, "Y Coord" );
    th->setLabel( 2, "Z Coord" );
    th->setLabel( 3, "Time Step" );
	
    tableButtonBox->addWidget( table );

    buttonBox = new QHBoxLayout( 0, 0, 6, "button box layout" );

    addPushButton = new QPushButton( this, "add button" );
    addPushButton->setText( "&Add Seed" );
    addPushButton->setEnabled( true );
    buttonBox->addWidget( addPushButton );

    QSpacerItem *spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding,
						 QSizePolicy::Minimum );
    buttonBox->addItem( spacer );

	deletePushButton = new QPushButton( this, "delete button" );
    deletePushButton->setText( "&Delete Seed" );
    deletePushButton->setEnabled( false );
    buttonBox->addWidget( deletePushButton );

    spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding,
						 QSizePolicy::Minimum );
    buttonBox->addItem( spacer );
    okPushButton = new QPushButton( this, "ok button" );
    okPushButton->setText( "OK" );
    okPushButton->setDefault( true );
    buttonBox->addWidget( okPushButton );

    cancelPushButton = new QPushButton( this, "cancel button" );
    cancelPushButton->setText( "Cancel" );
    cancelPushButton->setAccel( Key_Escape );
    buttonBox->addWidget( cancelPushButton );

    tableButtonBox->addLayout( buttonBox );
   
    connect( table, SIGNAL( currentChanged(int,int) ),this, SLOT( currentChanged(int,int) ) );
	connect( table, SIGNAL( selectionChanged() ),this, SLOT( selectionChanged() ) );
    connect( table, SIGNAL( valueChanged(int,int) ),this, SLOT( valueChanged(int,int) ) );
    connect( deletePushButton, SIGNAL( clicked() ), this, SLOT( deleteSeed() ) );
	connect( addPushButton, SIGNAL( clicked() ), this, SLOT( addSeed() ) );
    connect( okPushButton, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( cancelPushButton, SIGNAL( clicked() ), this, SLOT( reject() ) );

    
	//Insert the existing seeds
	for (int i = 0; i<numSeeds; i++){
		for (int j = 0; j<3; j++){
			table->setText( i, j, QString::number(myFlowParams->getListSeedPoint(i,j),'g',7));
			//Hmmm can't do this.((QLineEdit*)(table->item(i,j)))->setAlignment(AlignHCenter);
		}
		float tstep = myFlowParams->getListSeedPoint(i,3);
		if (tstep < 0.f)
			table->setText( i, 3, QString(" * "));
		else 
			table->setText( i, 3, QString::number(tstep));
	}

	changed = false;
}


void SeedListEditor::currentChanged( int row, int  )
{
	checkPushButton(row);
}
void SeedListEditor::selectionChanged( )
{
	for (int i = 0; i< table->numRows(); i++){
		checkPushButton(i);
	}
}


void SeedListEditor::valueChanged( int row, int col )
{
	bool ok;
	double d = table->text( row, col ).toDouble( &ok );
    if ( col <  3) {
		
		if (ok) table->setText(row, col, QString( "%1" ).arg(
					d, 0, 'g', 7 ) );
		else table->setText(row,col,"0.0");
	}
	else { // col is 3; If not a valid timestep, make it "*"
		if ( ok && d >= 0.0) table->setText(row, col, QString( "%1" ).arg(
					d, 0, 'g', 4 ) );
		else table->setText(row, col, QString(" * "));

	}
	changed = true;
}

//Add another slot.  Insert it after a selected row, if there is one:
void SeedListEditor::addSeed()
{
	int insertRow = 1+table->currentRow();
	if (!table->isRowSelected(insertRow-1,true))
		insertRow = table->numRows();
	table->insertRows(insertRow);
	table->setText(insertRow,0, QString("0.0"));
	table->setText(insertRow,1, QString("0.0"));
	table->setText(insertRow,2, QString("0.0"));
	table->setText(insertRow,3, QString(" * "));
	checkPushButton(insertRow);
	changed = true;
    
}
//Remove the selected seed
void SeedListEditor::deleteSeed()
{
	int curRow = table->currentRow();
	if (table->isRowSelected(curRow,true))
		
		table->removeRow(curRow);

	checkPushButton(curRow);
	changed = true;
}

//Copy the new values back:
void SeedListEditor::accept()
{
	//Note: we will lose precision by copying values back and forth
	if (changed) {
		std::vector<Point4>& seedList = myFlowParams->getSeedPointList();
		
		seedList.clear();
		for (int i = 0; i<table->numRows(); i++){
			Point4 pt4;
			pt4.set1Val(0, table->text(i,0).toFloat());
			pt4.set1Val(1, table->text(i,1).toFloat());
			pt4.set1Val(2, table->text(i,2).toFloat());
			bool ok;
			float tstep = table->text(i,3).toFloat(&ok);
			if (!ok) tstep = -1.f;
			pt4.set1Val(3,tstep);
			seedList.push_back(pt4);
		}
	}
	QDialog::accept();
}
void SeedListEditor::checkPushButton( int row)
{
	if (table->isRowSelected(row,true))
		deletePushButton->setEnabled( true );
	else 
		deletePushButton->setEnabled( false );
}
