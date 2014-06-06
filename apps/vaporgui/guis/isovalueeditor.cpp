#include "isovalueeditor.h"

#include <qcolordialog.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <QTableWidget>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <qlineedit.h>

#include "isolineparams.h"

using namespace VAPoR;

IsovalueEditor::IsovalueEditor( int numVals, IsolineParams* fp,
			  QWidget* parent)
    : QDialog( parent )

{
	myIsolineParams = fp;
	table = new QTableWidget(this);
	table->setColumnCount(2);
	
	table->setRowCount(numVals);

    table->setHorizontalHeaderLabels(QStringList() << tr("Index")
		<< tr("Isovalue"));
    table->verticalHeader()->setVisible(false);
    table->resize(100, 200);
	vector<double> isovals = myIsolineParams->GetIsovalues();
	for (int i = 0; i<numVals; i++){
		QTableWidgetItem *indexItem = new QTableWidgetItem(QString::number(i));
		indexItem->setFlags(Qt::NoItemFlags);  //not editable
		QTableWidgetItem *isovalItem = new QTableWidgetItem(QString::number(isovals[i],'g',9));
	
		table->setItem(i, 0, indexItem);
        table->setItem(i, 1, isovalItem);
		
	}

    table->resizeColumnToContents(0);
    table->horizontalHeader()->setStretchLastSection(true);

	addIsoButton = new QPushButton(tr("Add Isovalue"));
	deleteButton = new QPushButton(tr("Delete Isovalue"));
    quitButton = new QPushButton(tr("Cancel"));
	okButton = new QPushButton(tr("OK"));

    buttonBox = new QDialogButtonBox(this);
    buttonBox->addButton(addIsoButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(deleteButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(okButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    connect(quitButton, SIGNAL(pressed()), this, SLOT(close()));
	connect(addIsoButton, SIGNAL(pressed()), this, SLOT(addIso()));
    connect(deleteButton, SIGNAL(pressed()), this, SLOT(deleteIso()));
    connect(okButton, SIGNAL(pressed()), this, SLOT(accept()));
    connect( table, SIGNAL( cellChanged(int,int) ),this, SLOT( valueChanged(int,int) ) );

    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    mainLayout->addWidget(table);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);


    setWindowTitle(tr("Edit Isovalue List"));
    
    resize( 100, 200 );
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->setSelectionMode(QAbstractItemView::SingleSelection);
  

	changed = false;
}




void IsovalueEditor::valueChanged( int row, int col )
{
	bool ok;
	QTableWidgetItem* item = table->item(row,col);
	double d = item->text().toDouble( &ok );
    if (col != 1) return;
    if (!ok) {
		item->setText("0.0");
	}
	
	changed = true;
}

//Add another row.  Insert it after a selected row, if there is one:
void IsovalueEditor::addIso()
{
	int insertPosn = 0;
	if (table->rowCount()>0) insertPosn = 1+table->currentRow();
	QTableWidgetItem *indexItem = new QTableWidgetItem(QString::number(insertPosn));
	indexItem->setFlags(Qt::NoItemFlags);  //not editable
	QTableWidgetItem *xCoordItem = new QTableWidgetItem(QString::number(0.0));
	
	table->insertRow(insertPosn);
	table->setItem(insertPosn, 0, indexItem);
    table->setItem(insertPosn, 1, xCoordItem);
	
	// renumber all the later rows:
	
	for (int i = insertPosn+1; i<table->rowCount(); i++){
		table->item(i,0)->setText(QString::number(i));
	}
	changed = true;
    
}
//Remove the selected iso
void IsovalueEditor::deleteIso()
{
	if (table->rowCount() < 2) return;
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
void IsovalueEditor::accept()
{
	//Note: we will lose precision by copying values back and forth
	if (changed) {
		vector<double> isovals = myIsolineParams->GetIsovalues();
		if (table->rowCount() < isovals.size()){
			int numToDelete = isovals.size()-table->rowCount();
			for (int i = 0; i<numToDelete; i++) isovals.pop_back();
		}
		for (int i = 0; i<table->rowCount(); i++){
			double isoval = table->item(i,1)->text().toDouble();
			if (i < isovals.size()) isovals[i] = isoval;
			else if (i >= isovals.size()) isovals.push_back(isoval);
		}
		myIsolineParams->SetIsovalues(isovals);
	}
	
	QDialog::accept();
}

