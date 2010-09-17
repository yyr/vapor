
#include "pythonedit.h"
#include "vapor/DataMgr.h"
#include "vapor/DataMgrFactory.h"
#include "pythonpipeline.h"
#include "datastatus.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "session.h"
#include "messagereporter.h"
#include "string.h"
#include <vector>
#include <string>
#include <QFile>
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
PythonEdit::PythonEdit(QWidget *parent, QString varname)
    : QDialog(parent)
{
	variableName = varname;
	if(varname == "startup script") startUp = true; 
	else startUp = false;
	changeFlag = false;
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* hlayout = new QHBoxLayout();
	QHBoxLayout* buttonLayout1 = new QHBoxLayout();
    
    if (startUp) setWindowTitle("Python Startup Script Editor");
	else setWindowTitle("Python Derived Variable Script Editor");

    pythonEdit = new QTextEdit(this);
    pythonEdit->setFocus();
	pythonEdit->setAcceptRichText(false);
	if (!startUp){
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
	}
	
	QPushButton* saveButton = new QPushButton("Save to File",this);
	buttonLayout1->addWidget(saveButton);
	QPushButton* loadButton = new QPushButton("Load from File",this);
	buttonLayout1->addWidget(loadButton);
	QPushButton* testButton = new QPushButton("Test",this);
	buttonLayout1->addWidget(testButton);
	QPushButton* applyButton = new QPushButton("Apply",this);
	buttonLayout1->addWidget(applyButton);
	
	if (!startUp && varname != ""){
		QPushButton* deleteButton = new QPushButton("Delete",this);
		buttonLayout1->addWidget(deleteButton);
		connect(deleteButton, SIGNAL(pressed()), this, SLOT(deleteScript()));
	}
	QPushButton* quitButton = new QPushButton("Cancel",this);
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

	if (!startUp){
		inputVars3->insertSeparator(1);
		inputVars2->insertSeparator(1);
		outputVars3->insertSeparator(1);
		outputVars2->insertSeparator(1);
		inputVars3->insertSeparator(2);
		inputVars2->insertSeparator(2);
		outputVars3->insertSeparator(2);
		outputVars2->insertSeparator(2);
	}
	
	if (!startUp && varname != ""){
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
	if (startUp){
		pythonEdit->setText(PythonPipeLine::getStartupScript().c_str());
	}
    show();
}

using namespace VetsUtil;
using namespace VAPoR;

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
		changeFlag = true;
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
		changeFlag = true;
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
		changeFlag = true;
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
		changeFlag = true;
		
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
		changeFlag = true;
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
		changeFlag = true;
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

	const string script = pythonEdit->toPlainText().toStdString();
	
	if (script.length()==0){
		MessageReporter::errorMsg(" No python script specified");
		return;
	}
	//check that all input and output variables appear in script:
	vector<string>  inVars3D, inVars2D;
	if(!startUp){
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
		
	
		for (int i = 2; i<inputVars3->count()-3; i++) inVars3D.push_back(inputVars3->itemText(i).toStdString());
		for (int i = 2; i<inputVars2->count()-3; i++) inVars2D.push_back(inputVars2->itemText(i).toStdString());
	}
		
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	
	rParams->getRegionVoxelCoords(reflevel, min_dim, max_dim, min_bdim, max_bdim, timeStep);
	DataMgr* dmgr = ds->getDataMgr();
	vector<string> testIn;
	vector<pair<string, Metadata::VarType_T> > testOut;
	PythonPipeLine* pipe = new PythonPipeLine(string("TEST"), testIn, testOut, dmgr);
	
	const string& rettxt = pipe->python_test_wrapper(script, inVars2D, inVars3D, timeStep, reflevel, min_bdim, max_bdim);
	
	if (rettxt.size() > 0){
		QMessageBox::information(this,"Python Script Output",rettxt.c_str());
	}
	delete pipe;

}
void PythonEdit::applyScript(){
	if (startUp){
		string pythonProg = pythonEdit->toPlainText().toStdString();
		PythonPipeLine::setStartupScript(pythonProg);
		close();
		return;
	}
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
		//Need to remove old pipeline, too.
		string name;
		if (ds->getDerived3DOutputVars(scriptID).size()>0)
			name = ds->getDerived3DOutputVars(scriptID)[0];
		else 
			name = ds->getDerived2DOutputVars(scriptID)[0];
		ds->getDataMgr()->RemovePipeline(name);
		
		int rc = ds->replaceDerivedScript(scriptID, in2dVars, out2dVars, in3dVars, out3dVars, pythonProg);
		if (rc < 0) {
			MessageReporter::errorMsg(" Invalid script variable changes");
			return;
		}
		//refresh the visualizers:
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		for (int i = 0; i< MAXVIZWINS; i++){
			VizWin* win = vizMgr->getVizWin(i);
			if(win) {
				win->setRegionDirty(true);
				win->updateGL();
			}
		}
	}
	
	//Create a new PythonPipeline:
	vector<string>inputs;
	for (int i = 0; i< in2dVars.size(); i++) inputs.push_back(in2dVars[i]);
	for (int i = 0; i< in3dVars.size(); i++) inputs.push_back(in3dVars[i]);
	vector<pair<string, Metadata::VarType_T> > outpairs;
	for (int i = 0; i < out3dVars.size(); i++){
		outpairs.push_back( make_pair(out3dVars[i],Metadata::VAR3D));
	}
	for (int i = 0; i < out2dVars.size(); i++){
		outpairs.push_back( make_pair(out2dVars[i],Metadata::VAR2D_XY));
	}
	string pname;
	if (out3dVars.size() > 0) pname = out3dVars[0]; else pname = out2dVars[0];
	
	DataMgr* dmgr = ds->getDataMgr();
	PythonPipeLine* pipe = new PythonPipeLine(pname, inputs, outpairs, dmgr);
	dmgr->NewPipeline(pipe);
	
	//Update the variables in the gui and the params
	VizWinMgr::getInstance()->reinitializeVariables();
	close();
}
void PythonEdit::saveScript(){
	
	QString filename = QFileDialog::getSaveFileName(this,
		"Choose a file name to save this Python program",
		Session::getInstance()->getSessionDirectory().c_str(),
		"Python Files (*.py)");

	if(filename.length() == 0) return;
		
	
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
	QString filename = QFileDialog::getOpenFileName(this,
		"Specify a Python file name to append to current Python program",
		Session::getInstance()->getSessionDirectory().c_str(),
		"Python Files (*.py)");

	if(filename.length() == 0) return;
		
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
void PythonEdit::quit(){
	if(changeFlag){
		QMessageBox msgBox;
		msgBox.setText("The script has been changed.");
		msgBox.setInformativeText("Do you want to discard your changes?");
		msgBox.setStandardButtons(QMessageBox::Apply | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Discard);
		int ret = msgBox.exec();
		if(ret == QMessageBox::Cancel) return;
		if(ret == QMessageBox::Apply) {
			applyScript();
			return;
		}
	}
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
	
	int id = ds->getDerivedScriptId(variableName.toStdString());
	if (id >=0) ds->removeDerivedScript(id);
	close();
}
