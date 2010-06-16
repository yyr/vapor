
#include "pythonedit.h"
#include "vapor/DataMgr.h"
#include "vapor/DataMgrFactory.h"
#include "vapor/PythonControl.h"
#include "datastatus.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
#include <vector>
#include <string>
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
PythonEdit::PythonEdit(QWidget *parent, QString varname)
    : QDialog(parent)
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
    	QHBoxLayout* hlayout = new QHBoxLayout();
	QHBoxLayout* buttonLayout1 = new QHBoxLayout();
    
    	setWindowTitle("Python Script Editor");

    	pythonEdit = new QTextEdit(this);
    	pythonEdit->setFocus();
	pythonEdit->setAcceptRichText(false);
	inputVars2 = new QComboBox(this);
	hlayout->addWidget(inputVars2);
	outputVars2 = new QComboBox(this);
	hlayout->addWidget(outputVars2);
	inputVars3 = new QComboBox(this);
	hlayout->addWidget(inputVars3);
	outputVars3 = new QComboBox(this);
	hlayout->addWidget(outputVars3);

	outputVars2->addItem("2D Output Variables");
	outputVars2->addItem("Remove Variable");
	outputVars2->addItem("Add Variable");

	inputVars2->addItem("2D Input Variables");
	inputVars2->addItem("Remove Variable");
	inputVars2->addItem("Add Variable");

	outputVars3->addItem("3D Output Variables");
	outputVars3->addItem("Remove Variable");
	outputVars3->addItem("Add Variable");

	inputVars3->addItem("3D Input Variables");
	inputVars3->addItem("Remove Variable");
	inputVars3->addItem("Add Variable");
	
	connect(inputVars2, SIGNAL(activated(int)), this, SLOT(inputVarsActive2(int)));
	connect(outputVars2, SIGNAL(activated(int)), this, SLOT(outputVarsActive2(int)));
	connect(inputVars3, SIGNAL(activated(int)), this, SLOT(inputVarsActive3(int)));
	connect(outputVars3, SIGNAL(activated(int)), this, SLOT(outputVarsActive3(int)));
	inputVars2->setFocusPolicy(Qt::NoFocus);
	inputVars3->setFocusPolicy(Qt::NoFocus);
	outputVars2->setFocusPolicy(Qt::NoFocus);
	outputVars3->setFocusPolicy(Qt::NoFocus);
	
	QPushButton* settingsButton = new QPushButton("Settings",this);
	buttonLayout1->addWidget(settingsButton);
	QPushButton* saveButton = new QPushButton("Save to File",this);
	buttonLayout1->addWidget(saveButton);
	QPushButton* loadButton = new QPushButton("Load from File",this);
	buttonLayout1->addWidget(loadButton);
	
	QPushButton* testButton = new QPushButton("Test",this);
	buttonLayout1->addWidget(testButton);
	QPushButton* applyButton = new QPushButton("Apply",this);
	buttonLayout1->addWidget(applyButton);
	QPushButton* quitButton = new QPushButton("Cancel",this);
	buttonLayout1->addWidget(quitButton);
	
	connect(testButton, SIGNAL(pressed()), this, SLOT(testScript()));
	connect(applyButton, SIGNAL(pressed()), this, SLOT(applyScript()));
	connect(quitButton, SIGNAL(pressed()), this, SLOT(quit()));

	
	mainLayout->addLayout(hlayout);
	mainLayout->addWidget(pythonEdit);
	mainLayout->addLayout(buttonLayout1);

	inputVars3->insertSeparator(1);
	inputVars2->insertSeparator(1);
	outputVars3->insertSeparator(1);
	outputVars2->insertSeparator(1);
	inputVars3->insertSeparator(2);
	inputVars2->insertSeparator(2);
	outputVars3->insertSeparator(2);
	outputVars2->insertSeparator(2);
	
	if (varname != ""){
		//Set up combos for specified output variable
		DataStatus* ds = DataStatus::getInstance();
		int scriptID = ds->getDerivedScriptId(varname.toStdString());
		assert(scriptID >= 0);
		pythonEdit->setText(ds->getDerivedScript(scriptID).c_str());
		//Set up the input and output variable combos
		vector<string> varnames = ds->getDerived2DInputVars(scriptID);
		for (int i = 0; i< varnames.size(); i++){
			inputVars2->insertItem(2,varnames[i].c_str());
		}
		varnames = ds->getDerived3DInputVars(scriptID);
		for (int i = 0; i< varnames.size(); i++){
			inputVars3->insertItem(2,varnames[i].c_str());
		}
		varnames = ds->getDerived2DOutputVars(scriptID);
		for (int i = 0; i< varnames.size(); i++){
			outputVars2->insertItem(2,varnames[i].c_str());
		}
		varnames = ds->getDerived3DOutputVars(scriptID);
		for (int i = 0; i< varnames.size(); i++){
			outputVars3->insertItem(2,varnames[i].c_str());
		}
	
	}
    show();
}

using namespace VetsUtil;
using namespace VAPoR;
static int numargs=0;


void PythonEdit::addInputVar2(){
	//Create a QStringList with all the MD 2D variables
	DataStatus* ds = DataStatus::getInstance();
	QStringList qsList;
	for (int i = 0; i<ds->getNumMetadataVariables2D(); i++){
		int m = ds->mapMetadataToSessionVarNum2D(i);
		if (ds->variableIsPresent2D(m))
			qsList << QString(ds->getMetadataVarName2D(i).c_str());	
	}
	if (qsList.isEmpty()) return;
	bool ok;
	QString text = QInputDialog::getItem(this, "Add 2D Input Variable",
		"Specify 2D Input Variable Name:", qsList, 0, false, &ok);
	if (ok){
		//Make sure the selected variable is not already being used
		int location = inputVars2->findText(text);
		if (location >= 0) return;
		inputVars2->insertItem(2,text);
	}
}
void PythonEdit::addOutputVar2(){
	bool ok;
    QString text = QInputDialog::getText(this, "Add 2D Output Variable",
		"Specify 2D Output Variable Name:", QLineEdit::Normal,
                                          "", &ok);
	if (ok && !text.isEmpty()){
		DataStatus* ds;
		ds = DataStatus::getInstance();
		//Make sure it isn't already used in metadata or as output to this script
		for (int i = 2; i<outputVars3->count()-3; i++)
			if(outputVars3->itemText(i) == text) return;
		for (int i = 2; i<outputVars2->count()-3; i++)
			if(outputVars2->itemText(i) == text) return;
		
		for (int i = 0; i< ds->getNumMetadataVariables(); i++)
			if (ds->getMetadataVarName(i) == text.toStdString()) return;
		for (int i = 0; i< ds->getNumMetadataVariables2D(); i++)
			if (ds->getMetadataVarName2D(i) == text.toStdString()) return;
		outputVars2->insertItem(2,text);
	}

}
void PythonEdit::delInputVar2(){
	//Create a QStringList with all the 2D input variables
	
	QStringList qsList;
	for (int i = 2; i<inputVars2->count()-3; i++){
		qsList << inputVars2->itemText(i);
	}
	if (qsList.isEmpty()) return;
	bool ok;
	QString text = QInputDialog::getItem(this, "Remove 2D Input Variable",
		"Select 2D Input Variable to Remove:", qsList, 0, false, &ok);
	if (ok){
		int location = inputVars2->findText(text);
		if (location < 1 ) return;
		inputVars2->removeItem(location);
	}
}
void PythonEdit::delOutputVar2(){
	//Create a QStringList with all the 2D output variables
	QStringList qsList;
	for (int i = 2; i<outputVars2->count()-3; i++){
		qsList << outputVars2->itemText(i);
	}
	if (qsList.isEmpty()) return;
	bool ok;
	QString text = QInputDialog::getItem(this, "Remove 2D Output Variable",
		"Select 2D Output Variable to Remove:", qsList, 0, false, &ok);
	if (ok){
		int location = outputVars2->findText(text);
		if (location < 1 ) return;
		outputVars2->removeItem(location);
	}
	
}
void PythonEdit::inputVarsActive2(int index){
	if (index == inputVars2->count()-2) delInputVar2();
	else if (index == inputVars2->count()-1) addInputVar2();
	inputVars2->setCurrentIndex(0);
}
void PythonEdit::outputVarsActive2(int index){
	if (index == outputVars2->count()-2) delOutputVar2();
	else if (index == outputVars2->count()-1) addOutputVar2();
	outputVars2->setCurrentIndex(0);
}
void PythonEdit::addInputVar3(){
	//Create a QStringList with all the MD 3D variables
	DataStatus* ds = DataStatus::getInstance();
	QStringList qsList;
	for (int i = 0; i<ds->getNumMetadataVariables(); i++){
		int m = ds->mapMetadataToSessionVarNum(i);
		if (ds->variableIsPresent(m))
			qsList << QString(ds->getMetadataVarName(i).c_str());	
	}
	if (qsList.isEmpty()) return;
	bool ok;
	QString text = QInputDialog::getItem(this, "Add 3D Input Variable",
		"Specify 3D Input Variable Name:", qsList, 0, false, &ok);
	if (ok){
		//Make sure the selected variable is not already being used
		int location = inputVars3->findText(text);
		if (location > 0 ) return;
		inputVars3->insertItem(2,text);
	}
}
void PythonEdit::addOutputVar3(){
	bool ok;
	DataStatus* ds;
    QString text = QInputDialog::getText(this, "Add 3D Output Variable",
		"Specify 3D Output Variable Name:", QLineEdit::Normal,
                                          "", &ok);
	if (ok && !text.isEmpty()){
		ds = DataStatus::getInstance();
		//Make sure it isn't already used in metadata or as output to this script
		for (int i = 2; i<outputVars3->count()-3; i++)
			if(outputVars3->itemText(i) == text) return;
		for (int i = 2; i<outputVars2->count()-3; i++)
			if(outputVars2->itemText(i) == text) return;
		
		for (int i = 0; i< ds->getNumMetadataVariables(); i++)
			if (ds->getMetadataVarName(i) == text.toStdString()) return;
		for (int i = 0; i< ds->getNumMetadataVariables2D(); i++)
			if (ds->getMetadataVarName2D(i) == text.toStdString()) return;
		outputVars3->insertItem(2,text);
		
	}
}
void PythonEdit::delInputVar3(){
	//Create a QStringList with all the 3D input variables
	
	QStringList qsList;
	for (int i = 2; i<inputVars3->count()-3; i++){
		qsList << inputVars3->itemText(i);
	}
	if (qsList.isEmpty()) return;
	bool ok;
	QString text = QInputDialog::getItem(this, "Remove 3D Input Variable",
		"Specify 3D Input Variable Name:", qsList, 0, false, &ok);
	if (ok){
		int location = inputVars3->findText(text);
		if (location < 1) return;
		inputVars3->removeItem(location);
	}
}
void PythonEdit::delOutputVar3(){
	//Create a QStringList with all the 2D output variables
	
	QStringList qsList;
	for (int i = 2; i<outputVars3->count()-3; i++){
		qsList << outputVars3->itemText(i);
	}
	if (qsList.isEmpty()) return;
	bool ok;
	QString text = QInputDialog::getItem(this, "Remove 3D Output Variable",
		"Specify 3D Output Variable Name:", qsList, 0, false, &ok);
	if (ok){
		int location = outputVars3->findText(text);
		if (location < 1 ) return;
		outputVars3->removeItem(location);
	}
}
void PythonEdit::inputVarsActive3(int index){
	if (index == inputVars3->count()-2) delInputVar3();
	else if (index == inputVars3->count()-1) addInputVar3();
	inputVars3->setCurrentIndex(0);
}
void PythonEdit::outputVarsActive3(int index){
	if (index == outputVars3->count()-2) delOutputVar3();
	else if (index == outputVars3->count()-1) addOutputVar3();
	outputVars3->setCurrentIndex(0);
}
void PythonEdit::testScript(){
	size_t min_dim[3],max_dim[3],min_bdim[3],max_bdim[3];
	DataStatus* ds = DataStatus::getInstance();
	size_t timeStep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	int reflevel = 0;

	DataMgr* dataMgr = ds->getDataMgr();
	const string script = pythonEdit->toPlainText().toStdString();
	vector<string> inVars2, outVars2, inVars3, outVars3;
	for (int i = 2; i<inputVars3->count()-3; i++) inVars3.push_back(inputVars3->itemText(i).toStdString());
	for (int i = 2; i<outputVars3->count()-3; i++) outVars3.push_back(outputVars3->itemText(i).toStdString());
	for (int i = 2; i<inputVars2->count()-3; i++) inVars2.push_back(inputVars2->itemText(i).toStdString());
	for (int i = 2; i<outputVars2->count()-3; i++) outVars2.push_back(outputVars2->itemText(i).toStdString());
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	
	rParams->getRegionVoxelCoords(reflevel, min_dim, max_dim, min_bdim, max_bdim, timeStep);

	const string& rettxt =  dataMgr->GetPythonControl()->python_test_wrapper(script, inVars2, outVars2, inVars3, outVars3, 
						timeStep,reflevel,min_bdim,max_bdim);
	if (rettxt.size() > 0){
		QMessageBox::information(this,"Python Script Output",rettxt.c_str());
	}

}
void PythonEdit::applyScript(){
	//Setup the variables in the datastatus
	DataStatus* ds = DataStatus::getInstance();
	//Check if this script already exists
	//If there are no output variables, issue an error message;
	string outvar;
	//There need to be at least 6 entries in the pulldown menu for there to be any variables
	//since there are two horizonal bars and 3 predefined text items
	if (outputVars3->count() > 5){
		outvar = outputVars3->itemText(2).toStdString();
	} else if (outputVars2->count() > 5){
		outvar = outputVars2->itemText(2).toStdString();
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
	for (int i = 2; i< inputVars2->count()-3; i++){ 
		if (!prog.contains(inputVars2->itemText(i))) {
			MessageReporter::errorMsg(" Program does not contain variable %s", inputVars2->itemText(i).toAscii().data());
			return;
		}
	}
	for (int i = 2; i< inputVars3->count()-3; i++){ 
		if (!prog.contains(inputVars3->itemText(i))) {
			MessageReporter::errorMsg(" Program does not contain variable %s", inputVars3->itemText(i).toAscii().data());
			return;
		}
	}

	for (int i = 2; i< outputVars2->count()-3; i++){ 
		if (!prog.contains(outputVars2->itemText(i))) {
			MessageReporter::errorMsg(" Program does not contain variable %s", outputVars2->itemText(i).toAscii().data());
			return;
		}
	}
	for (int i = 2; i< outputVars3->count()-3; i++){ 
		if (!prog.contains(outputVars3->itemText(i))) {
			MessageReporter::errorMsg(" Program does not contain variable %s", outputVars3->itemText(i).toAscii().data());
			return;
		}
	}

	//Create string vectors for input and output variables
	vector<string> in2dVars;
	vector<string> out2dVars;
	vector<string> in3dVars;
	vector<string> out3dVars;
	for (int i = 2; i< inputVars2->count()-3; i++) in2dVars.push_back(inputVars2->itemText(i).toStdString());
	for (int i = 2; i< inputVars3->count()-3; i++) in3dVars.push_back(inputVars3->itemText(i).toStdString());
	for (int i = 2; i< outputVars2->count()-3; i++) out2dVars.push_back(outputVars2->itemText(i).toStdString());
	for (int i = 2; i< outputVars3->count()-3; i++) out3dVars.push_back(outputVars3->itemText(i).toStdString());
	
	int scriptID = ds->getDerivedScriptId(outvar);
	//If scriptID is negative, there is no script
	if (scriptID < 0) {

		scriptID = ds->addDerivedScript(in2dVars, out2dVars, in3dVars, out3dVars, pythonProg);
		if (scriptID < 0) {
			MessageReporter::errorMsg(" Invalid script variables ");
			return;
		}
	} 
	else {
	//Update existing script:
		
		int rc = ds->replaceDerivedScript(scriptID, in2dVars, out2dVars, in3dVars, out3dVars, pythonProg);
		if (rc < 0) {
			MessageReporter::errorMsg(" Invalid script variable changes");
			return;
		}
	}
	//Update them in the DataMgr:
	ds->updateDerivedMappings();
	//Update the variables in the gui and the params
	VizWinMgr::getInstance()->reinitializeVariables();
	close();
}
void PythonEdit::quit(){
	close();
}
