//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		renderholder.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2014
//
//	Description:	Implements the RenderHolder class

#include "renderholder.h"
#include "arrowparams.h"
#include "isolineparams.h"
#include <qcombobox.h>
#include <QStringList>
#include <QTableWidget>
#include "mainform.h"
#include "vizselectcombo.h"
#include <qpushbutton.h>
#include "vapor/ControlExecutive.h"
#include "instanceparams.h"
#include "qdialog.h"
#include "newRendererDialog.h"
#include <sstream>
#include "assert.h"
using namespace VAPoR;

QStackedWidget* RenderHolder::stackedWidget = 0;
RenderHolder* RenderHolder::theRenderHolder = 0;
std::vector<int> RenderHolder::vizDupNums;

RenderHolder::RenderHolder(QWidget* parent) : QWidget(parent), Ui_RenderSelector(){
	setupUi(this);
	signalsOn = false;
	
	tableWidget->setColumnCount(3);
	QStringList headerText;
	headerText << "    Name    " << "  Type  " << "Enabled";
	tableWidget->setHorizontalHeaderLabels(headerText);
	tableWidget->verticalHeader()->hide();
	tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableWidget->setFocusPolicy(Qt::ClickFocus);
	
	tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
	tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	connect(newButton, SIGNAL(clicked()), this, SLOT(newRenderer()));
	connect(deleteButton, SIGNAL(clicked()),this,SLOT(deleteRenderer()));
	connect(dupCombo, SIGNAL(activated(int)), this, SLOT(copyInstanceTo(int)));
	connect(tableWidget,SIGNAL(cellChanged(int,int)), this, SLOT(changeChecked(int,int)));
	connect(tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectInstance()));
	connect(tableWidget, SIGNAL(itemChanged(QTableWidgetItem*)),this, SLOT(itemTextChange(QTableWidgetItem*)));
	stackedWidget = new QStackedWidget(this);
	scrollArea->setWidget(stackedWidget);
	//Remove any existing widgets:
	for (int i = stackedWidget->count()-1; i>=0; i--){
		QWidget* wid = stackedWidget->widget(i);
		stackedWidget->removeWidget(wid);
		delete wid;
	}
	
	signalsOn = true;
	theRenderHolder = this;
	vizDupNums.clear();
	vizDupNums.push_back(0);
}
int RenderHolder::addWidget(QWidget* wid, const char* name, string tag){
	//int viznum = VizWinMgr::getInstance()->getActiveViz();
	// rc indicates position in the stacked widget.  It will be needed to change "active" renderer
	int rc = stackedWidget->addWidget(wid);
	stackedWidget->setCurrentIndex(rc);

	return rc;
}

/* 
 * Slots: 
 */
void RenderHolder:: 
newRenderer(){
	//Launch a dialog to select a renderer type, visualizer, name
	//Then insert a horizontal line with text and checkbox.
	//The new line becomes selected.
	
	QDialog nDialog(this);
	Ui_NewRendererDialog rDialog;
	rDialog.setupUi(&nDialog);
	rDialog.rendererNameEdit->setText("Renderer Name");
	if (nDialog.exec() != QDialog::Accepted) return;
	//Create a new default RenderParams of specified type in current active visualizer
	//The type is determined by the combo index (currently only barbs and contours supported)
	int selection = rDialog.rendererCombo->currentIndex();
	//eventually we will need to have a mapping between combo entries and params tags
	vector<string> renderTags;
	renderTags.push_back(ArrowParams::_arrowParamsTag);
	renderTags.push_back(IsolineParams::_isolineParamsTag);
	
	string tag = renderTags[selection];
	int winnum = VizWinMgr::getInstance()->getActiveViz();
	RenderParams* newP = dynamic_cast<RenderParams*>(ControlExec::GetDefaultParams(tag)->deepCopy());

	//figure out the name
	QString name = rDialog.rendererNameEdit->text();
	string name1 = name.toStdString();
	if (name1 == "Renderer Name") name1 = ControlExec::GetShortName(tag);
	name1 = uniqueName(name1);
	name = QString(name1.c_str());

	InstanceParams::AddInstance(name1, winnum, newP);
	updateTableWidget();
	/*
	//Create row in table widget:
	int rowCount = tableWidget->rowCount();
	tableWidget->setRowCount(rowCount+1);
	

	QTableWidgetItem *item0 = new QTableWidgetItem(name);
	item0->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromStdString(ControlExec::GetShortName(tag)));
	item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	QTableWidgetItem *item2 = new QTableWidgetItem("      ");
	item2->setCheckState(Qt::Unchecked);
	
	tableWidget->setItem(rowCount, 0, item0);
	tableWidget->setItem(rowCount, 1, item1);
	tableWidget->setItem(rowCount, 2, item2);

	tableWidget->selectRow(rowCount);
	*/
	VizWinMgr::getEventRouter(tag)->updateTab();
	VizWinMgr::getInstance()->getTabManager()->showRenderWidget(tag);
	
}
void RenderHolder::
deleteRenderer(){
	if (!signalsOn) return;
	//Delete the currently selected renderer.
	int winnum = VizWinMgr::getInstance()->getActiveViz();
	//Check if there is anything to delete:
	if (tableWidget->rowCount() == 0) return;
	int row = InstanceParams::GetSelectedIndex(winnum);
	RenderParams* rp = InstanceParams::GetSelectedRenderParams(winnum);
	//Disable it if necessary;
	if (rp->IsEnabled()){
		VizWinMgr::getEventRouter(rp->GetName())->guiSetEnabled(false, rp->GetInstanceIndex());
	}
	InstanceParams::RemoveSelectedInstance(winnum);

	
	//Remove row from table:
	signalsOn = false;
	tableWidget->removeRow(row);
	int newSelection = InstanceParams::GetSelectedIndex(winnum);
	tableWidget->selectRow(newSelection);

	signalsOn = true;
	if (tableWidget->rowCount() == 0){
		TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
		tMgr->hideRenderWidgets();
	} else { //update the info for the newly selected renderer instance
		string tag = InstanceParams::GetSelectedRenderParams(winnum)->GetName();
		VizWinMgr::getInstance()->getEventRouter(tag)->updateTab();
	}
}
//Respond to check/uncheck enabled checkbox
void RenderHolder::
changeChecked(int row, int col){

	if (!signalsOn) return;
	if ( col != 2) return;
	//Determine if it was checked or unchecked
	QTableWidgetItem* checkItem = tableWidget->item(row,col);
	//Determine the EventRouter associated with this item
	int viznum = VizWinMgr::getInstance()->getActiveViz();
	if (viznum < 0) return;
	
	RenderParams *rP = InstanceParams::GetRenderParamsInstance(viznum, row);
	EventRouter* er = VizWinMgr::getInstance()->getEventRouter(rP->GetName());
	bool enable;
	if (checkItem->checkState() == Qt::Checked)
		enable = true;
	 else
		enable = false;
	
	int instance = rP->GetInstanceIndex();
	er->guiSetEnabled(enable,instance);
	if (!checkItem->isSelected())
		tableWidget->selectRow(row);
	
}
void RenderHolder::
selectInstance(){
	
	if (!signalsOn) return;
	QList<QTableWidgetItem*> selectList = tableWidget->selectedItems();
	if (selectList.count() < 1) return;
	QTableWidgetItem* item = selectList[0];
	int row = item->row();
	int viznum = VizWinMgr::getInstance()->getActiveViz();

	RenderParams *rP = InstanceParams::GetRenderParamsInstance(viznum, row);
	string tag = rP->GetName();

	InstanceParams::SetSelectedIndex(viznum, row);
	VizWinMgr::getInstance()->getTabManager()->showRenderWidget(tag);
	VizWinMgr::getEventRouter(tag)->updateTab();
}
void RenderHolder::
itemTextChange(QTableWidgetItem* item){
	
	if (! signalsOn) return;
	
	int row = item->row();
	int col = item->column();
	if (col != 0) return;
	int viznum = VizWinMgr::getInstance()->getActiveViz();
	//avoid responding to item creation:
	if (InstanceParams::GetNumInstances(viznum) <= row) return;
	
	QString newtext = item->text();
	string stdtext = newtext.toStdString();
	RenderParams* rP = InstanceParams::GetRenderParamsInstance(viznum, row);
	if (stdtext == rP->GetRendererName()) return;
	string stdtext1 = uniqueName(stdtext);

	rP->SetRendererName(stdtext1);
	
	signalsOn = false;
	if (stdtext1 != stdtext) item->setText(QString(stdtext1.c_str()));
	signalsOn = true;
}

void RenderHolder::copyInstanceTo(int comboNum){
	dupCombo->setCurrentIndex(0);  //reset for next click on combo..
	if (comboNum == 0) return;  //"duplicate-in" entry
	if (tableWidget->rowCount() == 0) return; //Nothing to copy.
	if (!signalsOn) return;
	signalsOn = false;
	int fromViz = VizWinMgr::getInstance()->getActiveViz();

	RenderParams* rP = InstanceParams::GetSelectedRenderParams(fromViz);
	string tag = rP->GetName();
	EventRouter* er = VizWinMgr::getEventRouter(tag);
	if (comboNum == 1) {
		//another instance in the same viz, need to adjoin it to tableWidget:
		RenderParams* newP = (RenderParams*)rP->deepCopy();
		string rendererName = uniqueName(rP->GetRendererName());
		InstanceParams::AddInstance(rendererName, fromViz, newP);
	
		//Create row in table widget:
		int rowCount = tableWidget->rowCount();
		tableWidget->setRowCount(rowCount+1);
		//string name = uniqueName(instanceName[fromViz][row]);

		QTableWidgetItem *item0 = new QTableWidgetItem(rendererName.c_str());
		item0->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromStdString(ControlExec::GetShortName(tag)));
		item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		QTableWidgetItem *item2 = new QTableWidgetItem("      ");
		item2->setCheckState(Qt::Unchecked);
	
		tableWidget->setItem(rowCount, 0, item0);
		tableWidget->setItem(rowCount, 1, item1);
		tableWidget->setItem(rowCount, 2, item2);
	
		//Need to save stuff associated with this row
	
		tableWidget->selectRow(rowCount);
		signalsOn = true;
		er->updateTab();
		return;
	}
	//Otherwise copy to another viz..
	int vizNum = vizDupNums[comboNum-2];
	RenderParams* newP = (RenderParams*)rP->deepCopy();
	string rendererName = uniqueName((rP->GetRendererName())+"Copy");
	InstanceParams::AddInstance(rendererName, vizNum, newP);
	
	signalsOn = true;
	return;
}
std::string RenderHolder::uniqueName(std::string name){
	string newname = name;
	int numViz = VizWinMgr::getInstance()->getNumVisualizers();
	
	while(1){
		bool match = false;
		for (int viz = 0; viz<numViz; viz++){ 
			for (int i = 0; i< InstanceParams::GetNumInstances(viz); i++){
				RenderParams* rp = InstanceParams::GetRenderParamsInstance(viz,i);
				if (newname != rp->GetRendererName()) continue;
				match = true;
				//found a match.  Modify newname
				//If newname ends with a number, increase the number.  Otherwise just append _1
				size_t lastnonint = newname.find_last_not_of("0123456789");
				if (lastnonint < newname.length()-1){
					//remove terminating int
					string endchars = newname.substr(lastnonint+1);
					//Convert to int:
					int termInt = atoi(endchars.c_str());
					//int termInt = std::stoi(endchars);
					termInt++;
					//convert termInt to a string
					std::stringstream ss;
					ss << termInt;
				 	endchars = ss.str();	
					//endchars = std::to_string((unsigned long long)termInt);
					newname.replace(lastnonint+1,string::npos, endchars);
				} else {
					newname = newname + "_1";
				}
			}

		}
		if (!match) break;
	}
	return newname;
}


void RenderHolder::rebuildCombo(){
	VizSelectCombo* vCombo = MainForm::getInstance()->getWindowSelector();
	int activeViz = VizWinMgr::getInstance()->getActiveViz();
	RenderHolder* rh = getInstance();
	QComboBox* dCombo = rh->dupCombo;
	//Delete everything except the first two entries:
	for (int i = dCombo->count()-1; i>1; i--){
		dCombo->removeItem(i);
	}
	vizDupNums.clear();
	//Insert every visualizer that is not activeViz
	for (int i = 0; i < vCombo->getNumWindows(); i++){
		if (vCombo->getWinNum(i) == activeViz) 
			continue;
		dCombo->addItem(vCombo->getWinName(i));
		vizDupNums.push_back(vCombo->getWinNum(i));
	}

}
void RenderHolder::updateTableWidget(){
	//Rebuild the table widget, based on current active visualizer state
	int activeViz = VizWinMgr::getInstance()->getActiveViz();
	if (activeViz < 0) return;
	signalsOn = false;
	
	tableWidget->clearContents();

	//Add one row in tableWidget for each RenderParams that is associated with activeViz:
	int numRows = InstanceParams::GetNumInstances(activeViz);
	
	tableWidget->setRowCount(numRows);
	for (int row = 0; row < numRows; row++){
		
		RenderParams* rP = InstanceParams::GetRenderParamsInstance(activeViz,row);
		string name = rP->GetRendererName();
		QString qName = QString(name.c_str());
		
		QTableWidgetItem *item0 = new QTableWidgetItem(qName);
		item0->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		string tag = rP->GetName();
		QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromStdString(ControlExec::GetShortName(tag)));
		item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		QTableWidgetItem *item2 = new QTableWidgetItem("      ");
		if (rP->IsEnabled())
			item2->setCheckState(Qt::Checked);
		else 
			item2->setCheckState(Qt::Unchecked);
		tableWidget->setItem(row, 0, item0);
		tableWidget->setItem(row, 1, item1);
		tableWidget->setItem(row, 2, item2);

	}
	TabManager* tm = VizWinMgr::getInstance()->getTabManager();
	if (numRows > 0){
		int selectedRow = InstanceParams::GetSelectedIndex(activeViz);
		tableWidget->selectRow(selectedRow);
		RenderParams* rP = InstanceParams::GetSelectedRenderParams(activeViz);
		tm->showRenderWidget(rP->GetName());
	}
	else {
		tableWidget->selectRow(-1);
		tm->hideRenderWidgets();
	}
	
	signalsOn = true;
	rebuildCombo();
}

