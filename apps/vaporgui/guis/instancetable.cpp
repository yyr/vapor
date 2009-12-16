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
#include <qcheckbox.h>
#include <qcombobox.h>
#include <q3table.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include "instancetable.h"
#include "eventrouter.h"
#include <q3table.h>
#include <qpalette.h>

using namespace VAPoR;
InstanceTable::InstanceTable(QWidget* parent, const char* name) : Q3Table(parent, name) {

// Table size

	const int numRows = 6;
	const int numCols = 1;
	setColumnWidth(0,60);
	//setColumnWidth(1,10);
	setNumRows(numRows);
	setNumCols(numCols);

    
	setFocusPolicy(Qt::ClickFocus);
    Q3Header *header = horizontalHeader();
   
    header->setLabel( 0, QObject::tr( "View" ));
	
    setRowMovingEnabled(FALSE);
	setSelectionMode(Q3Table::SingleRow);

	setMaximumWidth(140);
	setMaximumHeight(100);
	setHScrollBarMode(Q3ScrollView::AlwaysOff);
	setLeftMargin(50);
	numcheckboxes = 0;
	
	
	
	
}
InstanceTable::~InstanceTable(){
	//Possibly not needed.. will the InstanceTable delete its own?
	for (int i = numRows(); i< numcheckboxes; i++) delete checkBoxList[i];
}
//Build the table, based on instances in eventrouter
void InstanceTable::rebuild(EventRouter* myRouter){
	
	
	//All instance info is in VizWinMgr.
	VAPoR::Params::ParamType renderType = myRouter->getParamsType();
	VAPoR::VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	int numInsts = vizMgr->getNumInstances(winnum,renderType);
	assert(MAX_NUM_INSTANCES >= numInsts);
	selectedInstance = vizMgr->getCurrentInstanceIndex(winnum, renderType);
	if (currentRow() != selectedInstance) selectRow(selectedInstance);


	// disable extra rows, before calling setNumRows, 
	//because setNumRows will delete them and they will continue
	//to draw, and cause mouse events.
	for (int r = numInsts; r < numcheckboxes; r++){
		
		checkBoxList[r]->setPaletteForegroundColor(Qt::white);
		checkBoxList[r]->setPaletteBackgroundColor(Qt::white);
		checkBoxList[r]->makeEmit(false);
		checkBoxList[r]->setChecked(false);
		checkBoxList[r]->setEnabled(false);
	}
	setNumRows(numInsts);
	
	if(numInsts < numcheckboxes) numcheckboxes = numInsts;

	Q3Header* vertHead = verticalHeader();
	//put existing checkboxes in proper state, or create new ones:
	
	for (int r = 0; r<numInsts; r++){
		RowCheckBox* checkbox;
		RenderParams* rParams = (RenderParams*)vizMgr->getParams(winnum,renderType,r);
		if( r < numcheckboxes) checkbox = checkBoxList[r];
		else {
			checkbox = new RowCheckBox(r,0, this);
			checkBoxList[r] = checkbox;
			setCellWidget(r,0,checkbox);
			connect(checkbox,SIGNAL(toggleRow(bool, int)),this, SLOT(enableChecked(bool,int)));
		}
		checkbox->setRow(r);
		checkbox->makeEmit(false);
		checkbox->setChecked(rParams->isEnabled());
		checkbox->setEnabled(true);
		
		checkbox->makeEmit(true);

		
		vertHead->setLabel(r,QString("Instance: ")+QString::number(r+1)+ " ");

		if (r == selectedInstance){
			//Invert colors in selected checkbox:
			checkbox->setPaletteForegroundColor(paletteBackgroundColor());
			checkbox->setPaletteBackgroundColor(Qt::darkBlue);
			
		} else {
			//default colors:
			checkbox->setPaletteForegroundColor(paletteForegroundColor());
			checkbox->setPaletteBackgroundColor(paletteBackgroundColor());
		}
	}
	numcheckboxes = numInsts;
	connect(this, SIGNAL(selectionChanged()), this, SLOT(selectInstance()));
	selectRow(selectedInstance);

}


//Slots:
void InstanceTable::selectInstance(){
	int newSelection = currentRow();
	if (newSelection == selectedInstance) return;
	selectedInstance = newSelection;
	emit changeCurrentInstance(selectedInstance);
}

void InstanceTable::enableChecked(bool val, int inst){
	
	emit enableInstance(val, inst);
}
void InstanceTable::checkEnabledBox(bool val, int instance){
	checkBoxList[instance]->setChecked(val);
}
RowCheckBox::RowCheckBox(int row, const QString& text, QWidget* parent) :
		QCheckBox(text, parent) {
	myRow = row;
	doEmit = false;
	connect(this,SIGNAL(toggled(bool)),this,SLOT(toggleme(bool)));
	
}

void RowCheckBox::toggleme(bool value) {
	if (doEmit) emit toggleRow(value, myRow);
}
