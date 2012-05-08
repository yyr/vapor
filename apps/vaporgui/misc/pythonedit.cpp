//************************************************************************
//																		*
//		     Copyright (C)  2010										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		pythonedit.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2010
//
//	Description:	Implementation of window for editing python scripts
//					
//

#ifdef WIN32
#pragma warning(disable : 4996)
#endif
#include "pythonedit.h"
#include <vapor/DataMgr.h>
#include <vapor/DataMgrFactory.h>
#include "pythonpipeline.h"
#include "datastatus.h"
#include "command.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "session.h"
#include "messagereporter.h"
#include "string.h"
#include <vector>
#include <string>
#include <QFile>
#include <QListWidgetItem>
#include <QListWidget>
#include <QTextStream>
#include <QAction>
#include <QComboBox>
#include <QPushButton>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextEdit>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDialog>
#include <QBoxLayout>

using namespace VAPoR;
const QString PythonEdit::startupWhatsThisText = 
	"This window is used to edit a Python script that will be \
executed once before any variable-defining scripts are executed.  It can be \
used to define functions, variables or constants that will be later \
used in variable-defining scripts.";
const QString PythonEdit::varDefWhatsThisText = 
	"This window is used to edit a Python script that defines one or more \
derived variables.  Array operations (using NumPy) can be used to define \
new variables as arithmetic expressions applied to existing variables.  \
You must indicate any existing variables that are referenced in the \
script, by checking the variable names in the input 2D or 3D variable check-boxes.  All derived \
(output) variables that are defined by this script must be identified in the \
2D and 3D output variable lists above.";


PythonEdit::PythonEdit(QWidget *parent, QString varname)
    : QDialog(parent)
{
	variableName = varname;
	if(varname == "startup script") startUp = true; 
	else startUp = false;
	
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* hlayout = new QHBoxLayout();
	QHBoxLayout* buttonLayout1 = new QHBoxLayout();
    
    if (startUp) setWindowTitle("Python Startup Script Editor");
	else setWindowTitle("Python Derived Variable Script Editor");
	
    pythonEdit = new QTextEdit(this);
    pythonEdit->setFocus();
	pythonEdit->setAcceptRichText(false);
	
	//pythonEdit->setFontPointSize(12);
	
	pythonEdit->setCurrentFont(QFont("Courier", 12));
	pythonEdit->setTabStopWidth(30);
	
	if (!startUp){
		pythonEdit->setWhatsThis(varDefWhatsThisText);
		QVBoxLayout* vlayout = new QVBoxLayout();
		QLabel* colLabel = new QLabel("Input 2D Variables");
		colLabel->setAlignment(Qt::AlignHCenter);
		vlayout->addWidget(colLabel);
		vlayout->addStretch();
		inputVars2 = new QListWidget(this);
		inputVars2->setMaximumHeight(120);
		
		vlayout->addWidget(inputVars2);
		hlayout->addLayout(vlayout);
		
		vlayout = new QVBoxLayout();
		colLabel = new QLabel("Output 2D Variables");
		colLabel->setAlignment(Qt::AlignHCenter);
		vlayout->addWidget(colLabel);
		vlayout->addStretch();
		outputVars2 = new QListWidget(this);
		outputVars2->setMaximumHeight(50);
		vlayout->addWidget(outputVars2);
		add2DVar = new QPushButton("Add 2D Variable");
		vlayout->addWidget(add2DVar);
		rem2DVar = new QPushButton("Remove Selected Variable");
		vlayout->addWidget(rem2DVar);
		hlayout->addLayout(vlayout);

		vlayout = new QVBoxLayout();
		colLabel = new QLabel("Input 3D Variables");
		colLabel->setAlignment(Qt::AlignHCenter);
		vlayout->addWidget(colLabel);
		vlayout->addStretch();
		inputVars3 = new QListWidget(this);
		inputVars3->setMaximumHeight(120);
		vlayout->addWidget(inputVars3);
		hlayout->addLayout(vlayout);

		vlayout = new QVBoxLayout();
		colLabel = new QLabel("Output 3D Variables");
		colLabel->setAlignment(Qt::AlignHCenter);
		vlayout->addWidget(colLabel);
		vlayout->addStretch();
		outputVars3 = new QListWidget(this);
		outputVars3->setMaximumHeight(50);
		vlayout->addWidget(outputVars3);
		add3DVar = new QPushButton("Add 3D Variable");
		vlayout->addWidget(add3DVar);
		rem3DVar = new QPushButton("Remove Selected Variable");
		vlayout->addWidget(rem3DVar);
		hlayout->addLayout(vlayout);
		

	

		
		connect(inputVars2, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(inputVarsActive2(QListWidgetItem*)));
		connect(inputVars3, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(inputVarsActive3(QListWidgetItem*)));
		connect(add3DVar, SIGNAL(clicked()), this, SLOT(addOutputVar3()));
		connect(add2DVar, SIGNAL(clicked()), this, SLOT(addOutputVar2()));
		connect(rem3DVar, SIGNAL(clicked()), this, SLOT(delOutputVar3()));
		connect(rem2DVar, SIGNAL(clicked()), this, SLOT(delOutputVar2()));
		
		inputVars2->setFocusPolicy(Qt::NoFocus);
		inputVars3->setFocusPolicy(Qt::NoFocus);
		outputVars2->setFocusPolicy(Qt::NoFocus);
		outputVars3->setFocusPolicy(Qt::NoFocus);

		inputVars2->setToolTip("Check the names of 2D variables that are used as input to this python script");
		inputVars3->setToolTip("Check the names of 3D variables that are used as input to this python script");
		outputVars2->setToolTip("Specify the names of 2D variables that are output by this python script");
		outputVars3->setToolTip("Specify the names of 3D variables that are output by this python script");
		add2DVar->setToolTip("Click to specify another 2D output variable");
		add3DVar->setToolTip("Click to specify another 3D output variable");
		rem3DVar->setToolTip("Click to remove the selected 3D output variable");
		rem2DVar->setToolTip("Click to remove the selected 2D output variable");
	} else {
		pythonEdit->setWhatsThis(startupWhatsThisText);
	}
	
	QPushButton* saveButton = new QPushButton("Save to File",this);
	buttonLayout1->addWidget(saveButton);
	QPushButton* loadButton = new QPushButton("Load (Append) from File",this);
	buttonLayout1->addWidget(loadButton);
	QPushButton* testButton = new QPushButton("Test",this);
	buttonLayout1->addWidget(testButton);
	QPushButton* applyButton = new QPushButton("Apply",this);
	buttonLayout1->addWidget(applyButton);

	saveButton->setToolTip("Click to save this python script to a file");
	loadButton->setToolTip("Click to append the python code in a file to this script");
	if (startUp) 
		testButton->setToolTip("Click to execute this startup script, displaying output text");
	else testButton->setToolTip("Click to execute this script, displaying output text but without defining new variables");
	if (startUp)
		applyButton->setToolTip("Click to install this startup script; it will then execute before any other python scripts");
	else applyButton->setToolTip("Click to use this script as the definition of variables in this session");

	
	if (!DataStatus::getInstance()->getDataMgr() && !startUp){
		testButton->setEnabled(false);
		applyButton->setEnabled(false);
	}
	
	if (!startUp && varname != ""){
		QPushButton* deleteButton = new QPushButton("Delete",this);
		buttonLayout1->addWidget(deleteButton);
		connect(deleteButton, SIGNAL(pressed()), this, SLOT(deleteScript()));
		deleteButton->setToolTip("Click to remove this script, and the variables it defines, from the session");
	}
	QPushButton* quitButton = new QPushButton("Cancel",this);
	quitButton->setToolTip("Click to stop editing this python script, ignoring all changes");
	buttonLayout1->addWidget(quitButton);
	
	connect(testButton, SIGNAL(pressed()), this, SLOT(testScript()));
	connect(applyButton, SIGNAL(pressed()), this, SLOT(applyScript()));
	connect(quitButton, SIGNAL(pressed()), this, SLOT(quit()));
	connect(pythonEdit, SIGNAL(textChanged()), this, SLOT(textChanged()) );
	connect(saveButton, SIGNAL(pressed()), this, SLOT(saveScript()));
	connect(loadButton, SIGNAL(pressed()), this, SLOT(loadScript()));

	
	mainLayout->addLayout(hlayout);
	mainLayout->addWidget(pythonEdit);
	mainLayout->addLayout(buttonLayout1);

	
	int scriptID = -1;
	if (!startUp && varname != "") scriptID = DataStatus::getDerivedScriptId(varname.toStdString());
	
	if (!startUp) {  //Build the variable list
		for (int i = 0; i< DataStatus::getNumMetadataVariables2D(); i++){
			QListWidgetItem *currItem = new QListWidgetItem(DataStatus::getMetadataVarName2D(i).c_str()) ;
			inputVars2->addItem(currItem);
			currItem->setCheckState(Qt::Unchecked);
			if (varname != "") {
				//see if it's in the input list
				for (int i = 0; i< DataStatus::getDerived2DInputVars(scriptID).size(); i++){
					if (DataStatus::getDerived2DInputVars(scriptID)[i] == currItem->text().toStdString())
						currItem->setCheckState(Qt::Checked);
				}


			}

		}
		for (int i = 0; i< DataStatus::getNumMetadataVariables(); i++){
			QListWidgetItem *currItem = new QListWidgetItem(DataStatus::getMetadataVarName(i).c_str()) ;
			inputVars3->addItem(currItem);
			currItem->setCheckState(Qt::Unchecked);
			if (varname != "") {
				//see if it's in the input list
				for (int i = 0; i< DataStatus::getDerived3DInputVars(scriptID).size(); i++){
					if (DataStatus::getDerived3DInputVars(scriptID)[i] == currItem->text().toStdString())
						currItem->setCheckState(Qt::Checked);
				}


			}

		}
	}
	if (!startUp && varname != ""){
		//Set up lists output variables
		
		scriptID = DataStatus::getDerivedScriptId(varname.toStdString());
		assert(scriptID > 0);
		pythonEdit->setText(DataStatus::getDerivedScript(scriptID).c_str());
		//Set up the input and output variable combos
		vector<string> varnames = DataStatus::getDerived2DInputVars(scriptID);
		
		
		varnames = DataStatus::getDerived2DOutputVars(scriptID);
		for (int i = 0; i< varnames.size(); i++){
			QListWidgetItem *currItem = new QListWidgetItem(varnames[i].c_str()) ;
			outputVars2->addItem(currItem);
		}
		varnames = DataStatus::getDerived3DOutputVars(scriptID);
		for (int i = 0; i< varnames.size(); i++){
			QListWidgetItem *currItem = new QListWidgetItem(varnames[i].c_str()) ;
			outputVars3->addItem(currItem);
		}
	
	} 
	if (startUp){
		pythonEdit->setText(PythonPipeLine::getStartupScript().c_str());
	}
	//Three cases for command:  startup, new, or edit, result in scriptID = -1, 0, or actual value
	if (startUp) {
		currentCommand = ScriptCommand::captureStart(-1, "Edit Python Startup script");
		currentScriptId = -1;
	}

	else if (varname == ""){
		currentCommand = ScriptCommand::captureStart(0, "Create new Python-derived variable"); 
		currentScriptId = 0;
	}

	else {
		currentCommand = ScriptCommand::captureStart(scriptID, "Edit existing Python-derived variable");
		currentScriptId = scriptID;
	}

	changeFlag = false;
	setWindowModality(Qt::ApplicationModal);
    show();
}

using namespace VetsUtil;
using namespace VAPoR;


void PythonEdit::addOutputVar2(){
	bool ok;
    QString text = QInputDialog::getText(this, "Add 2D Output Variable",
		"Specify 2D Output Variable Name:", QLineEdit::Normal,
                                          "", &ok);
	if (ok && !text.isEmpty()){
		DataStatus* ds;
		ds = DataStatus::getInstance();
		//Make sure it isn't already used in metadata or as output to this script
		for (int i = 0; i<outputVars3->count(); i++)
			if(outputVars3->item(i)->text() == text) {
				QMessageBox::information(this,"Output variable name invalid",
						"This variable name is used already, it cannot be redefined.");
				return;
			}
		for (int i = 0; i<outputVars2->count(); i++)
			if(outputVars2->item(i)->text() == text) {
				QMessageBox::information(this,"Output variable name invalid",
						"This variable name is used already, it cannot be redefined.");
				return;
			}
		
		for (int i = 0; i< ds->getNumMetadataVariables(); i++)
			if (ds->getMetadataVarName(i) == text.toStdString()) {
				QMessageBox::information(this,"Output variable name invalid",
										 "This variable name is used already, it cannot be redefined.");
				return;
			}
		for (int i = 0; i< ds->getNumMetadataVariables2D(); i++)
			if (ds->getMetadataVarName2D(i) == text.toStdString()) {
				QMessageBox::information(this,"Output variable name invalid",
										 "This variable name is used already, it cannot be redefined.");
				return;
			}
		outputVars2->addItem(new QListWidgetItem(text));
		changeFlag = true;
	}

}

void PythonEdit::delOutputVar2(){
	//See if a variable is selected
	
	for (int i = 0; i< outputVars2->count(); i++){
		QListWidgetItem* item = outputVars2->item(i);
		if (item->isSelected()){
			outputVars2->removeItemWidget(item);
			delete item;
			changeFlag = true;
			return;
		}
	}
}
void PythonEdit::inputVarsActive2(QListWidgetItem* item){
	if (item->checkState() == Qt::Unchecked)
		item->setCheckState(Qt::Checked);
	else item->setCheckState(Qt::Unchecked);
	changeFlag = true;
}
void PythonEdit::inputVarsActive3(QListWidgetItem* item){
	if (item->checkState() == Qt::Unchecked)
		item->setCheckState(Qt::Checked);
	else item->setCheckState(Qt::Unchecked);
	changeFlag = true;
}

void PythonEdit::addOutputVar3(){
	bool ok;
    QString text = QInputDialog::getText(this, "Add 3D Output Variable",
		"Specify 3D Output Variable Name:", QLineEdit::Normal,
                                          "", &ok);
	if (ok && !text.isEmpty()){
		DataStatus* ds;
		ds = DataStatus::getInstance();
		//Make sure it isn't already used in metadata or as output to this script
		for (int i = 0; i<outputVars3->count(); i++)
			if(outputVars3->item(i)->text() == text) {
				QMessageBox::information(this,"Output variable name invalid",
						"This variable name is used already, it cannot be redefined.");
				return;
			}
		for (int i = 0; i<outputVars2->count(); i++)
			if(outputVars2->item(i)->text() == text) {
				QMessageBox::information(this,"Output variable name invalid",
						"This variable name is used already, it cannot be redefined.");
				return;
			}
		
		for (int i = 0; i< ds->getNumMetadataVariables(); i++)
			if (ds->getMetadataVarName(i) == text.toStdString()) {
				QMessageBox::information(this,"Output variable name invalid",
										 "This variable name is used already, it cannot be redefined.");
				return;
			}
		for (int i = 0; i< ds->getNumMetadataVariables2D(); i++)
			if (ds->getMetadataVarName2D(i) == text.toStdString()) {
				QMessageBox::information(this,"Output variable name invalid",
										 "This variable name is used already, it cannot be redefined.");
				return;
			}
		outputVars3->addItem(new QListWidgetItem(text));
		changeFlag = true;
	}

}
void PythonEdit::delOutputVar3(){
	//See if a variable is selected
	
	for (int i = 0; i< outputVars3->count(); i++){
		QListWidgetItem* item = outputVars3->item(i);
		if (item->isSelected()){
			outputVars3->removeItemWidget(item);
			delete item;
			changeFlag = true;
			return;
		}
	}
}


void PythonEdit::testScript(){
	size_t min_dim[3],max_dim[3];
	DataStatus* ds = DataStatus::getInstance();
	size_t timeStep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int reflevel = 0;
	int compression = 0;

	const string script = pythonEdit->toPlainText().toStdString();
	
	if (script.length()==0){
		MessageReporter::errorMsg(" No python script specified");
		return;
	}
	//check that all input and output variables appear in script:
	vector<string>  inVars3D, inVars2D;
	vector<pair<string, DataMgr::VarType_T> > outputs;
	if(!startUp){
		QString prog = pythonEdit->toPlainText();
		
		for (int i = 0; i<inputVars2->count(); i++){
			QListWidgetItem* item = inputVars2->item(i);
			if (item->checkState() == Qt::Checked){
				if (!prog.contains(item->text())) {
					MessageReporter::errorMsg(" Program does not contain input variable %s", item->text().toAscii().data());
					return;
				}
				inVars2D.push_back(item->text().toStdString());
			}
			
		}
		for (int i = 0; i<inputVars3->count(); i++){
			QListWidgetItem* item = inputVars3->item(i);
			if (item->checkState() == Qt::Checked){
				if (!prog.contains(item->text())) {
					MessageReporter::errorMsg(" Program does not contain input variable %s", item->text().toAscii().data());
					return;
				}
				inVars3D.push_back(item->text().toStdString());
			}
			
		}

		for (int i = 0; i< outputVars2->count(); i++){ 
			if (!prog.contains(outputVars2->item(i)->text())) {
				MessageReporter::errorMsg(" Program does not contain output variable %s", outputVars2->item(i)->text().toAscii().data());
				return;
			}
			outputs.push_back( make_pair(outputVars2->item(i)->text().toStdString(),DataMgr::VAR2D_XY));
		}
		for (int i = 0; i< outputVars3->count(); i++){ 
			if (!prog.contains(outputVars3->item(i)->text())) {
				MessageReporter::errorMsg(" Program does not contain output variable %s", outputVars3->item(i)->text().toAscii().data());
				return;
			}
			outputs.push_back(make_pair(outputVars3->item(i)->text().toStdString(),DataMgr::VAR3D));
		}
		
	}
		
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	
	rParams->getRegionVoxelCoords(reflevel, min_dim, max_dim, timeStep);
	DataMgr* dmgr = ds->getDataMgr();
	//If there is an output 2D variable and no output 3d variable, then make the extents the full VDC extents
	//at refinement level 0
	if (!startUp && outputVars2->count() > 0 && outputVars3->count() == 0 && inputVars3->count() > 0){
		size_t regExtents[3];
		dmgr->GetDim(regExtents,0);
		min_dim[2] = 0;
		max_dim[2] = regExtents[2];
	}
	vector<string> testIn;
	vector<pair<string, DataMgr::VarType_T> > testOut;
	PythonPipeLine* pipe = new PythonPipeLine(string("TEST"), testIn, testOut, dmgr);
	
	const string& rettxt = pipe->python_test_wrapper(script, inVars2D, inVars3D, outputs, timeStep, reflevel, compression, min_dim, max_dim);
	
	if (rettxt.size() > 0){
		if (rettxt != "TEST_SCRIPT_ERROR")
			QMessageBox::information(this,"Output of script",rettxt.c_str());
	} else {
		QMessageBox::information(this,"Completion","Successful test");
	}
	delete pipe;

}
void PythonEdit::applyScript(){
	if (startUp){
		string pythonProg = pythonEdit->toPlainText().toStdString();
		PythonPipeLine::setStartupScript(pythonProg);
		DataStatus::purgeAllCachedDerivedVariables();
		VizWinMgr::getInstance()->refreshRenderData();
		close();
		ScriptCommand::captureEnd(currentCommand, -1);
		return;
	}
	//Setup the variables in the datastatus
	DataStatus* ds = DataStatus::getInstance();
	//Check if this script already exists
	//If there are no output variables, issue an error message;
	string outvar;
	
	if (outputVars3->count() > 0){
		outvar = outputVars3->item(0)->text().toStdString();
	} else if (outputVars2->count() > 0){
		outvar = outputVars2->item(0)->text().toStdString();
	} else {
		MessageReporter::errorMsg(" No output variables are specified");
		return;
	}
	//Get the program:
	string pythonProg = pythonEdit->toPlainText().toStdString();
	if (pythonProg.length()==0){
		MessageReporter::errorMsg(" No python script specified");
		return;
	}
	//check that all variables appear in script:
	QString prog = pythonEdit->toPlainText();
	vector<string> in2dVars, in3dVars;
	for (int i = 0; i<inputVars2->count(); i++){
		QListWidgetItem* item = inputVars2->item(i);
		if (item->checkState() == Qt::Checked){
			if (!prog.contains(item->text())) {
				MessageReporter::errorMsg(" Program does not contain input variable %s", item->text().toAscii().data());
				return;
			}
			in2dVars.push_back(item->text().toStdString());
		}
	}
	for (int i = 0; i<inputVars3->count(); i++){
		QListWidgetItem* item = inputVars3->item(i);
		if (item->checkState() == Qt::Checked){
			if (!prog.contains(item->text())) {
				MessageReporter::errorMsg(" Program does not contain input variable %s", item->text().toAscii().data());
				return;
			}
			in3dVars.push_back(item->text().toStdString());
		}
	}

	for (int i = 0; i< outputVars2->count(); i++){ 
		if (!prog.contains(outputVars2->item(i)->text())) {
			MessageReporter::errorMsg(" Program does not contain 2D variable %s", outputVars2->item(i)->text().toAscii().data());
			return;
		}
	}
	for (int i = 0; i< outputVars3->count(); i++){ 
		if (!prog.contains(outputVars3->item(i)->text())) {
			MessageReporter::errorMsg(" Program does not contain 3D variable %s", outputVars3->item(i)->text().toAscii().data());
			return;
		}
	}

	

	//Create string vectors for input and output variables
	
	vector<string> out2dVars;
	vector<string> out3dVars;
	
	for (int i = 0; i< outputVars3->count(); i++) out3dVars.push_back(outputVars3->item(i)->text().toStdString());
	for (int i = 0; i< outputVars2->count(); i++) out2dVars.push_back(outputVars2->item(i)->text().toStdString());
	
	//Check for use of 2D outputs with 3D inputs.  Issue a warning in that case
	if (out2dVars.size() > 0 && in3dVars.size()>0 && out3dVars.size()==0){
		MessageReporter::warningMsg("Note: When deriving a 2D output variable from 3D input variables,\n%s",
			"the vertical extents of the 3D input variable arrays will be the full vertical extents of the VDC.");
	}


	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//If scriptID is 0, there is no script
	int scriptID = currentScriptId;
	if (currentScriptId == 0) {

		scriptID = ds->addDerivedScript(in2dVars, out2dVars, in3dVars, out3dVars, pythonProg);
		if (scriptID < 0) {
			MessageReporter::errorMsg(" Invalid script variable(s). Is a variable name already in use?");
			return;
		}
	} 
	else {
		//Turn off renderers that use previous outputs:
		
		vizMgr->disableRenderers(ds->getDerived2DOutputVars(scriptID),ds->getDerived3DOutputVars(scriptID));
		//Update existing script:
		scriptID = ds->replaceDerivedScript(currentScriptId, in2dVars, out2dVars, in3dVars, out3dVars, pythonProg);
		if (scriptID < 0) {
			MessageReporter::errorMsg(" Invalid script variable changes");
			return;
		}
		
	}
	
		
	//Update the variables in the gui and the params
	vizMgr->reinitializeVariables();
	
	ScriptCommand::captureEnd(currentCommand, scriptID);
	close();
}
void PythonEdit::saveScript(){
	string path; 
	if (startUp) path = Session::getInstance()->getPythonDirectory() + "/pythonUserStartup.py";
	else path = Session::getInstance()->getPythonDirectory();

	QString filename = QFileDialog::getSaveFileName(this,
		"Choose a file name to save this Python program",
		path.c_str(),
		"Python Files (*.py)");

	if(filename.length() == 0) return;
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future python I/O
	Session::getInstance()->setPythonDirectory((const char*)fileInfo->absolutePath().toAscii());
	
	FILE* pythonFile = fopen((const char*)filename.toAscii(), "w");
	if (!pythonFile) {
		MessageReporter::errorMsg("File Save Error: Error opening \noutput Python file: \n%s",(const char*)filename.toAscii());
		return;
	}
	string prog = pythonEdit->toPlainText().toStdString();

	if (0 == fprintf(pythonFile, "%s", prog.c_str())){
		MessageReporter::errorMsg("File Save Error: Error writing \noutput Python file: \n%s",(const char*)filename.toAscii());
	}
	fclose(pythonFile);
	changeFlag = false;
}
void PythonEdit::loadScript(){
	string path; 
	if (startUp) path = Session::getInstance()->getPythonDirectory() + "/pythonUserStartup.py";
	else path = Session::getInstance()->getPythonDirectory();

	QString filename = QFileDialog::getOpenFileName(this,
		"Specify a Python file name to append to current Python program",
		path.c_str(),
		"Python Files (*.py)");

	if(filename.length() == 0) return;

	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	//Save the path for future python I/O
	Session::getInstance()->setPythonDirectory((const char*)fileInfo->absolutePath().toAscii());

	QFile file(filename);
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
		MessageReporter::errorMsg("File Open Error: Error opening \ninput Python file: \n%s",(const char*)filename.toAscii());
		return;
	}
	QTextStream in(&file);
	int numlines = 0;
	while (!in.atEnd()){
		QString line = in.readLine();
		pythonEdit->append(line);
		numlines++;
	}
	file.close();
	if(numlines == 0){
		MessageReporter::errorMsg("File Read Error: Error reading \ninput Python file: \n%s",(const char*)filename.toAscii());
		return;
	}
	textChanged();
	return;
}
bool PythonEdit::loadUserStartupScript(){
	string filename = Session::getInstance()->getPythonDirectory();
	string startupScript;
	filename += "/pythonUserStartup.py";
	PythonPipeLine::getStartupScript().clear();
	QFile file(filename.c_str());
	if(!file.open(QIODevice::ReadOnly | QIODevice::Text)){
		return false;
	}
	QTextStream in(&file);
	int numlines = 0;
	while (!in.atEnd()){
		QString line = in.readLine();
		line += "\n";
		startupScript.append(line.toStdString());
		numlines++;
	}
	file.close();
	if(numlines == 0){
		return false;
	}
	PythonPipeLine::setStartupScript(startupScript);
	return true;

}
void PythonEdit::quit(){
	if(changeFlag){
		QMessageBox msgBox;
		msgBox.setText("The script has been changed.");
		msgBox.setInformativeText("Do you want to discard your changes?");
		if (DataStatus::getInstance()->getDataMgr()){
			msgBox.setStandardButtons(QMessageBox::Apply | QMessageBox::Discard | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Discard);
		} else {
			msgBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
			msgBox.setDefaultButton(QMessageBox::Discard);
		}
		int ret = msgBox.exec();
		if(ret == QMessageBox::Cancel) return;
		if(ret == QMessageBox::Apply) {
			applyScript();
			return;
		}
	}
	delete currentCommand;
	currentCommand = 0;
	close();
}
void PythonEdit::textChanged(){
	changeFlag = true;
}
void PythonEdit::deleteScript(){
	
	QMessageBox msgBox;
	msgBox.setText("Discarding Python script");
	msgBox.setInformativeText("Do you want to discard this script, removing all the variables that it defines?");
	msgBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Discard);
	int ret = msgBox.exec();
	if(ret == QMessageBox::Cancel) return;
	//Remove the script and its variables..
	
	DataStatus* ds = DataStatus::getInstance();
	int id = DataStatus::getDerivedScriptId(variableName.toStdString());
	VizWinMgr* vizMgr = VizWinMgr::getInstance();

	if (id > 0) {
		vizMgr->disableRenderers(ds->getDerived2DOutputVars(id),ds->getDerived3DOutputVars(id));
		ds->removeDerivedScript(id);
	}
	if (ds->getDataMgr()) vizMgr->reinitializeVariables();
	currentCommand->setDescription(*(new QString("Delete Python-derived variable")));
	ScriptCommand::captureEnd(currentCommand, -2);
	close();
}
