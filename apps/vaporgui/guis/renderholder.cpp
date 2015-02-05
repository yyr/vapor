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
#include <qpushbutton.h>
#include "vapor/ControlExecutive.h"
#include "qdialog.h"
#include "newRendererDialog.h"
#include <sstream>
#include "assert.h"
using namespace VAPoR;

RenderHolder::RenderHolder(QWidget* parent) : QWidget(parent), Ui_RenderSelector(){
	setupUi(this);
	signalsOn = false;
	
	tableWidget->setColumnCount(3);
	QStringList headerText;
	headerText << "   Name   " << "   Type   " << "Enabled";
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
	
	//Remove any existing widgets:
	for (int i = stackedWidget->count()-1; i>=0; i--){
		QWidget* wid = stackedWidget->widget(i);
		stackedWidget->removeWidget(wid);
		delete wid;
	}
	
	signalsOn = true;
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
	signalsOn = false;
	string tag = renderTags[selection];
	int winnum = VizWinMgr::getInstance()->getActiveViz();
	RenderParams* newP = dynamic_cast<RenderParams*>(ControlExec::GetDefaultParams(tag)->deepCopy());
	
	int rc = ControlExec::AddParams(winnum,tag,newP);
	assert (!rc);
	newP->SetVizNum(winnum);
	newP->SetEnabled(false);
	

	//Create row in table widget:
	int rowCount = tableWidget->rowCount();
	tableWidget->setRowCount(rowCount+1);
	QString name = rDialog.rendererNameEdit->text();
	string name1 = name.toStdString();
	if (name1 == "Renderer Name") name1 = ControlExec::GetShortName(tag);
	name1 = uniqueName(name1);
	name = QString(name1.c_str());

	QTableWidgetItem *item0 = new QTableWidgetItem(name);
	item0->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromStdString(ControlExec::GetShortName(tag)));
	item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
	QTableWidgetItem *item2 = new QTableWidgetItem("      ");
	item2->setCheckState(Qt::Unchecked);
	
	tableWidget->setItem(rowCount, 0, item0);
	tableWidget->setItem(rowCount, 1, item1);
	tableWidget->setItem(rowCount, 2, item2);

	//Need to save stuff associated with this row
	
	instanceName[winnum].push_back(name.toStdString());
	currentRow[winnum]=rowCount;
	int instance = Params::GetNumParamsInstances(tag,winnum)-1;
	instanceIndex[winnum].push_back(instance);
	renParams[winnum].push_back(newP);
	
	newP->SetEnabled(false);
	signalsOn = true;
	tableWidget->selectRow(rowCount);
	

}
void RenderHolder::
deleteRenderer(){
	if (!signalsOn) return;
	//Delete the currently selected renderer.
	int winnum = VizWinMgr::getInstance()->getActiveViz();
	int row = currentRow[winnum];
	RenderParams *rp = renParams[winnum][row];
	string type = rp->GetTagFromType(rp->GetParamsBaseTypeId());
	assert( ControlExec::GetNumParamsInstances(winnum, type) > 0);
	int instance = instanceIndex[winnum][row];
	//disable it first if necessary:
	if (rp->IsEnabled()){
		rp->SetEnabled(false);
		VizWinMgr::getEventRouter(type)->updateRenderer(rp, true, instance, false);
	}
	int rc = ControlExec::RemoveParams(winnum, type, instance);
	assert (!rc);
	//Remove row from table:
	tableWidget->removeRow(row);
	//Remove entries from vectors:
	instanceIndex[winnum].erase(instanceIndex[winnum].begin()+row);
	instanceName[winnum].erase(instanceName[winnum].begin()+row);
	renParams[winnum].erase(renParams[winnum].begin()+row);
	//if this not the first entry, then the previous becomes selected; 
	//otherwise the first one becomes selected.
	int newSelection = row-1;
	if (newSelection < 0) newSelection = 0;
	currentRow[winnum] = newSelection;
	tableWidget->selectRow(newSelection);

	//Also need to renumber instances!
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
	RenderParams *rP = renParams[viznum][row];
	EventRouter* er = VizWinMgr::getInstance()->getEventRouter(rP->GetName());
	bool enable;
	if (checkItem->checkState() == Qt::Checked)
		enable = true;
	 else
		enable = false;
	int instance = instanceIndex[viznum][row];
	er->guiSetEnabled(enable,instance);
	if (currentRow[viznum] != row)
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
	currentRow[viznum] = row;
	RenderParams* rP = renParams[viznum][row];
	string tag = rP->GetName();
	int instance = instanceIndex[viznum][row];
	ControlExec::SetCurrentParamsInstance(viznum,tag, instance);
	VizWinMgr::getEventRouter(tag)->updateTab();
	VizWinMgr::getInstance()->getTabManager()->showRenderWidget(tag);
}
void RenderHolder::
itemTextChange(QTableWidgetItem* item){
	if (! signalsOn) return;
	
	int row = item->row();
	int col = item->column();
	if (col != 0) return;
	int viznum = VizWinMgr::getInstance()->getActiveViz();
	//avoid responding to item creation:
	if (instanceName[viznum].size() <= row) return;
	QString newtext = item->text();
	string stdtext = newtext.toStdString();
	
	if (stdtext == instanceName[viznum][row]) return;
	string stdtext1 = uniqueName(stdtext);
	instanceName[viznum][row] = stdtext1;
	signalsOn = false;
	if (stdtext1 != stdtext) item->setText(QString(stdtext1.c_str()));
	signalsOn = true;
}

void RenderHolder::copyInstanceTo(int toViz){
	if (toViz == 0) return; 
	dupCombo->setCurrentIndex(0);
	if (!signalsOn) return;
	signalsOn = false;
	int fromViz = VizWinMgr::getInstance()->getActiveViz();
	int row = currentRow[fromViz];
	RenderParams* rP = renParams[fromViz][row];
	string tag = rP->GetName();
	EventRouter* er = VizWinMgr::getEventRouter(tag);
	if (toViz == 1) {
		//another instance in the same viz:
		RenderParams* newP = (RenderParams*)rP->deepCopy();
		ControlExec::AddParams(fromViz,tag,newP);
		newP->SetVizNum(fromViz);
		newP->SetEnabled(false);
		//Create row in table widget:
		int rowCount = tableWidget->rowCount();
		tableWidget->setRowCount(rowCount+1);
		string name = uniqueName(instanceName[fromViz][row]);

		QTableWidgetItem *item0 = new QTableWidgetItem(name.c_str());
		item0->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		QTableWidgetItem *item1 = new QTableWidgetItem(QString::fromStdString(ControlExec::GetShortName(tag)));
		item1->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		QTableWidgetItem *item2 = new QTableWidgetItem("      ");
		item2->setCheckState(Qt::Unchecked);
	
		tableWidget->setItem(rowCount, 0, item0);
		tableWidget->setItem(rowCount, 1, item1);
		tableWidget->setItem(rowCount, 2, item2);
	
		//Need to save stuff associated with this row
	
		instanceName[fromViz].push_back(name);
		currentRow[fromViz]=rowCount;
		int instance = Params::GetNumParamsInstances(tag,fromViz)-1;
		instanceIndex[fromViz].push_back(instance);
		renParams[fromViz].push_back(newP);					
		newP->SetEnabled(false);
		tableWidget->selectRow(rowCount);
		signalsOn = true;
		er->updateTab();
		return;
	}
	assert(0);  //no copy to other viz...
	int viznum = toViz - 1;
	er->performGuiCopyInstanceToViz(viznum);
	return;
}
std::string RenderHolder::uniqueName(std::string name){
	string newname = name;
	int numViz = VizWinMgr::getInstance()->getNumVisualizers();
	
	while(1){
		bool match = false;
		for (int viz = 0; viz<numViz; viz++){ 
			for (int i = 0; i< instanceName[viz].size(); i++){
				if (newname != instanceName[viz][i]) continue;
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
				break;
			}

		}
		if (!match) break;
	}
	return newname;
}


