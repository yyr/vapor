//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		instancetable.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Implements the InstanceTable class
//		This fits in the renderer tabs, enables the user to select
//		copy, delete, and enable instances  
//

#include <qpixmap.h>
#include <qstringlist.h>
#include <qlineedit.h>
#include "instancetable.h"
#include "eventrouter.h"
#include <QTableWidget>
#include <QHeaderView>
#include <QString>


using namespace VAPoR;
InstanceTable::InstanceTable(QWidget* parent) : QTableWidget(parent) {

// Table size always 150 wide

	
	setRowCount(0);
	setColumnCount(2);
	setColumnWidth(0,80);
	setColumnWidth(1,50);
    verticalHeader()->hide();
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setFocusPolicy(Qt::ClickFocus);

	QStringList headerList = QStringList() <<"Instance" <<" View ";
    setHorizontalHeaderLabels(headerList);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	setMaximumWidth(150);
	setMaximumHeight(120);
	setMinimumHeight(120);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	
	connect(this,SIGNAL(cellChanged(int,int)), this, SLOT(changeChecked(int,int)));
	connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(selectInstance()));
	
}
InstanceTable::~InstanceTable(){
	for (int i = 0; i<rowCount(); i++){
		if (item(i,0)) delete item(i,0);
		if (item(i,1)) delete item(i,1);
	}

}
//Build the table, based on instances in eventrouter
void InstanceTable::rebuild(EventRouter* myRouter){
	
	int numInsts = 1;
	//All instance info is in VizWinMgr.
	
	VAPoR::Params::ParamsBaseType renderBaseType = myRouter->getParamsBaseType();
	VAPoR::VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();

	if (winnum >= 0) 
		numInsts = vizMgr->getNumInstances(winnum,renderBaseType);
	assert(numInsts > 0);
	
	
	//create items as needed
	//Also insert labels in first column
	if(rowCount() != numInsts) setRowCount(numInsts);
	
	for (int r = 0; r<numInsts; r++){
		
		RenderParams* rParams = (RenderParams*)vizMgr->getParams(winnum,renderBaseType,r);
		bool isEnabled = rParams->isEnabled();
		if (!item(r,0)) {  //need to create new items..

			QString instanceName = QString(" Instance: ") + QString::number(r+1) + " ";
			QTableWidgetItem *item0 = new QTableWidgetItem(instanceName);
			QTableWidgetItem *item1 = new QTableWidgetItem("    ");
			item0->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			item1->setFlags(item1->flags() & ~(Qt::ItemIsEditable));
		
			if (isEnabled)item1->setCheckState(Qt::Checked);
			else item1->setCheckState(Qt::Unchecked);
		
			setItem(r,0, item0);
			setItem(r,1, item1);
		} else {
			if (isEnabled && item(r,1)->checkState() != Qt::Checked)item(r,1)->setCheckState(Qt::Checked);
			else if (!isEnabled && item(r,1)->checkState() != Qt::Unchecked)item(r,1)->setCheckState(Qt::Unchecked);
		}
		
	}
	if(selectedInstance != vizMgr->getCurrentInstanceIndex(winnum, renderBaseType)){
		selectedInstance = vizMgr->getCurrentInstanceIndex(winnum, renderBaseType);
		selectRow(selectedInstance);
	}
	
}

//Slots:
void InstanceTable::selectInstance(){
	QList<QModelIndex> selectList = selectedIndexes();
	int newSelection = selectList[0].row();
	if (newSelection == selectedInstance) return;
	selectedInstance = newSelection;
	emit changeCurrentInstance(selectedInstance);
}
void InstanceTable::changeChecked(int row, int col){
	if (col != 1) return;
	QTableWidgetItem* checkedItem = item(row,col);
	bool val = (checkedItem->checkState() == Qt::Checked);
	emit enableInstance(val, row);
}
void InstanceTable::checkEnabledBox(bool val, int instance){
	item(instance,1)->setCheckState(val ? Qt::Checked : Qt::Unchecked);
}