#include "seedlisteditor.h"

#include <qcolordialog.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <qlineedit.h>

#include "flowparams.h"

using namespace VAPoR;

SeedListEditor::SeedListEditor( int numSeeds, FlowParams* fp,
			  QWidget* parent)
    : QDialog( parent )

{
	myFlowParams = fp;
	table = new QTableWidget(this);
	table->setColumnCount(5);
	
	table->setRowCount(numSeeds);
	

    table->setHorizontalHeaderLabels(QStringList() << tr("Index")
		<< tr("X coord")<< tr("Y coord")<< tr("Z coord")
                                                   << tr("time step"));
    table->verticalHeader()->setVisible(false);
    table->resize(150, 50);
	for (int i = 0; i<numSeeds; i++){
		QTableWidgetItem *indexItem = new QTableWidgetItem(QString::number(i));
		indexItem->setFlags(Qt::NoItemFlags);  //not editable
		QTableWidgetItem *xCoordItem = new QTableWidgetItem(QString::number(myFlowParams->getListSeedPoint(i,0),'g',7));
		QTableWidgetItem *yCoordItem = new QTableWidgetItem(QString::number(myFlowParams->getListSeedPoint(i,1),'g',7));
		QTableWidgetItem *zCoordItem = new QTableWidgetItem(QString::number(myFlowParams->getListSeedPoint(i,2),'g',7));
		
		float tstep = myFlowParams->getListSeedPoint(i,3);
		QTableWidgetItem *timestepItem;
		if (tstep < 0.f)
			timestepItem = new QTableWidgetItem(QString(" * "));
		else 
			timestepItem = new QTableWidgetItem(QString::number(tstep,'g',7));
		table->setItem(i, 0, indexItem);
        table->setItem(i, 1, xCoordItem);
		table->setItem(i, 2, yCoordItem);
		table->setItem(i, 3, zCoordItem);
		table->setItem(i, 4, timestepItem);
	}

    
    table->resizeColumnToContents(0);
    table->horizontalHeader()->setStretchLastSection(true);

	addSeedButton = new QPushButton(tr("Add Seed"));
	deleteButton = new QPushButton(tr("Delete Seed"));
	clearButton = new QPushButton(tr("Clear Table"));
    quitButton = new QPushButton(tr("Cancel"));
	okButton = new QPushButton(tr("OK"));

    buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(addSeedButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(deleteButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(clearButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(okButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    connect(quitButton, SIGNAL(pressed()), this, SLOT(close()));
    connect(clearButton, SIGNAL(pressed()), this, SLOT(clearTable()));
	connect(addSeedButton, SIGNAL(pressed()), this, SLOT(addSeed()));
    connect(deleteButton, SIGNAL(pressed()), this, SLOT(deleteSeed()));
    connect(okButton, SIGNAL(pressed()), this, SLOT(accept()));
    connect( table, SIGNAL( cellChanged(int,int) ),this, SLOT( valueChanged(int,int) ) );

    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    mainLayout->addWidget(table);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);


    setWindowTitle(tr("Edit Seed Point List"));
    
    resize( 180, 220 );
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
  

	changed = false;
}




void SeedListEditor::valueChanged( int row, int col )
{
	bool ok;
	QTableWidgetItem* item = table->item(row,col);
	double d = item->text().toDouble( &ok );
    if ( col <  4 && col > 0) {
		
		if (!ok) item->setText("0.0");
	}
	else { // col is 4; If not a valid timestep, make it "*"
		if ( ok && d >= 0.0) item->setText(QString( "%1" ).arg(d, 0, 'g', 4 ) );
		else item->setText(QString(" * "));
	}
	changed = true;
}

//Add another row.  Insert it after a selected row, if there is one:
void SeedListEditor::addSeed()
{
	int insertPosn = 0;
	if (table->rowCount()>0) insertPosn = 1+table->currentRow();
	QTableWidgetItem *indexItem = new QTableWidgetItem(QString::number(insertPosn));
	indexItem->setFlags(Qt::NoItemFlags);  //not editable
	QTableWidgetItem *xCoordItem = new QTableWidgetItem(QString::number(0.0));
	QTableWidgetItem *yCoordItem = new QTableWidgetItem(QString::number(0.0));
	QTableWidgetItem *zCoordItem = new QTableWidgetItem(QString::number(0.0));
	QTableWidgetItem *timestepItem;
	float tstep = -1.f;
	if(insertPosn > 0 && insertPosn < myFlowParams->getNumListSeedPoints()-1) 
		tstep = myFlowParams->getListSeedPoint(insertPosn-1,3);
	if (tstep < 0.f)
		timestepItem = new QTableWidgetItem(QString(" * "));
	else 
		timestepItem = new QTableWidgetItem(QString::number(tstep,'g',7));
	
	table->insertRow(insertPosn);
	table->setItem(insertPosn, 0, indexItem);
    table->setItem(insertPosn, 1, xCoordItem);
	table->setItem(insertPosn, 2, yCoordItem);
	table->setItem(insertPosn, 3, zCoordItem);
	table->setItem(insertPosn, 4, timestepItem);
	// renumber all the later rows:
	
	for (int i = insertPosn+1; i<table->rowCount(); i++){
		table->item(i,0)->setText(QString::number(i));
	}
	changed = true;
    
}
//Remove the selected seed
void SeedListEditor::deleteSeed()
{
	int curRow = table->currentRow();
	if (curRow<0) return;
	table->removeRow(curRow);
	//Renumber
	for (int i = curRow; i< table->rowCount(); i++){
		table->item(i,0)->setText(QString::number(i));
	}
	changed = true;
}

//Copy the new values back:
void SeedListEditor::accept()
{
	//Note: we will lose precision by copying values back and forth
	if (changed) {
		std::vector<Point4>& seedList = myFlowParams->getSeedPointList();
		
		seedList.clear();
		for (int i = 0; i<table->rowCount(); i++){
			
			Point4 pt4;
			pt4.set1Val(0, table->item(i,1)->text().toFloat());
			pt4.set1Val(1, table->item(i,2)->text().toFloat());
			pt4.set1Val(2, table->item(i,3)->text().toFloat());
			bool ok;
			float tstep = table->item(i,4)->text().toFloat(&ok);
			if (!ok) tstep = -1.f;
			pt4.set1Val(3,tstep);
			seedList.push_back(pt4);
		}
	}
	QDialog::accept();
}

void SeedListEditor::clearTable() {
	table->clearContents();
	table->setRowCount(0);
	changed = true;
}
