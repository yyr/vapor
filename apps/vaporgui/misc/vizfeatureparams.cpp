//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		vizfeatureparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the VizFeatureParams class.
//
#include "vizfeatureparams.h"
#include "command.h"
#include "vizwin.h"
#include "vizfeatures.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
#include "mainform.h"
#include "session.h"
#include "datastatus.h"
#include <qlineedit.h>
#include <QFileDialog>
#include <QScrollBar>
#include <QWhatsThis>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qlayout.h>
#include <vector>
#include <vapor/LayeredIO.h>

using namespace VAPoR;
int VizFeatureParams::sessionVariableNum = 0;
//Create a new vizfeatureparams
VizFeatureParams::VizFeatureParams(){
	dialogChanged = false;
	vizFeatureDlg = 0;
	featureHolder = 0;
	currentComboIndex = -1;
	textureSurface = false;
	
	Session* currentSession = Session::getInstance();
	
	for (int i = 0; i< 3; i++) stretch[i] = currentSession->getStretch(i);

	
	if (sessionVariableNum >= DataStatus::getInstance()->getNumSessionVariables())
		sessionVariableNum = 0;

	lowValues.clear();
	highValues.clear();
	extendDown.clear();
	extendUp.clear();
	tempLowValues.clear();
	tempHighValues.clear();
	tempExtendDown.clear();
	tempExtendUp.clear();

	DataStatus *ds;
	ds = DataStatus::getInstance();
	for (int i = 0; i< ds->getNumSessionVariables(); i++){
		lowValues.push_back(ds->getBelowValue(i));
		highValues.push_back(ds->getAboveValue(i));
		tempLowValues.push_back(ds->getBelowValue(i));
		tempHighValues.push_back(ds->getAboveValue(i));
		extendUp.push_back(ds->isExtendedUp(i));
		tempExtendUp.push_back(ds->isExtendedUp(i));
		extendDown.push_back(ds->isExtendedDown(i));
		tempExtendDown.push_back(ds->isExtendedDown(i));
	}
}
//Clone a new vizfeatureparams
VizFeatureParams::VizFeatureParams(const VizFeatureParams& vfParams){
	dialogChanged = false;
	vizFeatureDlg = 0;
	featureHolder = 0;
	for (int i = 0; i< 3; i++) stretch[i] = vfParams.stretch[i];
	currentComboIndex = vfParams.currentComboIndex;
	vizName = vfParams.vizName;
	showBar = vfParams.showBar;
	showAxisArrows = vfParams.showAxisArrows;
	
	surfaceImageFilename = vfParams.surfaceImageFilename;
	surfaceRotation = vfParams.surfaceRotation;
	surfaceUpsideDown = vfParams.surfaceUpsideDown;
	textureSurface = vfParams.textureSurface;
	axisArrowCoords[0] = vfParams.axisArrowCoords[0];
	axisArrowCoords[1] = vfParams.axisArrowCoords[1];
	axisArrowCoords[2] = vfParams.axisArrowCoords[2];
	
	for (int i = 0; i< 3; i++){
		minTic[i] = vfParams.minTic[i];
		maxTic[i] = vfParams.maxTic[i];
		numTics[i] = vfParams.numTics[i];
		axisOriginCoords[i] = vfParams.axisOriginCoords[i];
		ticLength[i] = vfParams.ticLength[i];
		ticDir[i] = vfParams.ticDir[i];
	}
	displacement = vfParams.displacement;
	showAxisAnnotation = vfParams.showAxisAnnotation;
	axisAnnotationColor = vfParams.axisAnnotationColor;
	tempAxisAnnotationColor = vfParams.tempAxisAnnotationColor;
	labelHeight = vfParams.labelHeight;
	labelDigits = vfParams.labelDigits;
	ticWidth = vfParams.ticWidth;

	colorbarLLCoords[0] = vfParams.colorbarLLCoords[0];
	colorbarLLCoords[1] = vfParams.colorbarLLCoords[1];
	colorbarURCoords[0] = vfParams.colorbarURCoords[0];
	colorbarURCoords[1] = vfParams.colorbarURCoords[1];
	numColorbarTics = vfParams.numColorbarTics;
	
	colorbarBackgroundColor = vfParams.colorbarBackgroundColor;
	
	elevGridColor = vfParams.elevGridColor;
	showElevGrid = vfParams.showElevGrid;
	elevGridRefinement =  vfParams.elevGridRefinement;

	timeAnnotCoords[0] = vfParams.timeAnnotCoords[0];
	timeAnnotCoords[1] = vfParams.timeAnnotCoords[1];
	timeAnnotColor = vfParams.timeAnnotColor;
	timeAnnotType = vfParams.timeAnnotType;
	timeAnnotTextSize = vfParams.timeAnnotTextSize;
	lowValues.clear();
	highValues.clear();
	tempLowValues.clear();
	tempHighValues.clear();
	extendDown.clear();
	extendUp.clear();
	tempExtendDown.clear();
	tempExtendUp.clear();
	int numvars = vfParams.lowValues.size();
	for (int i = 0; i<numvars; i++){
		lowValues.push_back(vfParams.lowValues[i]);
		highValues.push_back(vfParams.highValues[i]);
		extendUp.push_back(vfParams.extendUp[i]);
		extendDown.push_back(vfParams.extendDown[i]);
	}
	newOutVals = false;
}
//Set up the dialog with current parameters from current active visualizer
void VizFeatureParams::launch(){
	//Determine current combo entry
	int vizNum = VizWinMgr::getInstance()->getActiveViz();
	if (vizNum < 0 || DataStatus::getInstance()->getNumSessionVariables() <=0){
		QMessageBox::warning(vizFeatureDlg,"No visualizers or variables",
			QString("No visualizers or variables exist to be modified"),
			QMessageBox::Ok, Qt::NoButton);
		return;
	}
	
	//Determine corresponding combo index for currentVizNum:
	currentComboIndex = getComboIndex(vizNum);
	
	featureHolder = new ScrollContainer((QWidget*)MainForm::getInstance(), "Visualizer Feature Selection");
	
	QScrollArea* sv = new QScrollArea(featureHolder);
	featureHolder->setScroller(sv);
	vizFeatureDlg = new VizFeatureDialog(featureHolder);
	sv->setWidget(vizFeatureDlg);
	
	//Copy values into dialog, using current comboIndex:
	setDialog();
	dialogChanged = false;
	vizFeatureDlg->displacementEdit->setText(QString::number(displacement));
	vizFeatureDlg->variableCombo->setCurrentIndex(sessionVariableNum);
	vizFeatureDlg->lowValEdit->setText(QString::number(lowValues[sessionVariableNum]));
	vizFeatureDlg->highValEdit->setText(QString::number(highValues[sessionVariableNum]));
	vizFeatureDlg->extendDownCheckbox->setChecked(extendDown[sessionVariableNum]);
	vizFeatureDlg->lowValEdit->setEnabled(!extendDown[sessionVariableNum]);
	vizFeatureDlg->extendUpCheckbox->setChecked(extendUp[sessionVariableNum]);
	vizFeatureDlg->highValEdit->setEnabled(!extendUp[sessionVariableNum]);
	newOutVals = false;
	if (vizFeatureDlg->axisAnnotationCheckbox->isChecked()){
		vizFeatureDlg->axisAnnotationFrame->show();
	} else {
		vizFeatureDlg->axisAnnotationFrame->hide();
	}
	
	int h = MainForm::getInstance()->height();
	if ( h > 750) h = 750;
	int w = 390;
	
	vizFeatureDlg->setGeometry(0, 0, w, h);
	int swidth = sv->verticalScrollBar()->width();
	featureHolder->setGeometry(50, 50, w+swidth,h);
	
//	sv->resizeContents(w,h);
	sv->resize(w,h);
	
	//Check if we have time stamps in data:
	int numTimeTypes = 1;
	const DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	int firstTime = DataStatus::getInstance()->getMinTimestep();
	if (dataMgr){
		//If the first time stamp has more than 10 characters, it's
		//probably a real time stamp
		string timeStamp;
		dataMgr->GetTSUserTimeStamp(firstTime, timeStamp);
			if (timeStamp.length() > 10) numTimeTypes = 2;
	}
	vizFeatureDlg->timeCombo->setMaxCount(numTimeTypes+1);
	
	//Do connections.  
	connect(vizFeatureDlg->currentNameCombo, SIGNAL(activated(int)), this, SLOT(visualizerSelected(int)));
	
	connect (vizFeatureDlg->surfaceColorButton,SIGNAL(clicked()), this, SLOT(selectElevGridColor()));
	connect (vizFeatureDlg->vizNameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisZEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarLLXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarLLYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarXSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarYSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->numTicsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarBackgroundButton, SIGNAL(clicked()), this, SLOT(selectColorbarBackgroundColor()));
	connect (vizFeatureDlg->axisCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->surfaceCheckbox,SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->surfaceCheckbox,SIGNAL(toggled(bool)), this, SLOT(checkSurface(bool)));
	connect (vizFeatureDlg->applyButton, SIGNAL(clicked()), this, SLOT(applySettings()));
	connect (vizFeatureDlg->applyButton2, SIGNAL(clicked()), this, SLOT(applySettings()));
	connect (vizFeatureDlg->refinementCombo, SIGNAL (activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisAnnotationCheckbox, SIGNAL(clicked()), this, SLOT(annotationChanged()));
	connect (vizFeatureDlg->displacementEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->xMinTicEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->yMinTicEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->zMinTicEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->xMaxTicEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->yMaxTicEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->zMaxTicEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->xNumTicsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->yNumTicsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->zNumTicsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->xTicSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->yTicSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->zTicSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisOriginYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisOriginZEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisOriginXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->xTicOrientCombo, SIGNAL(activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->yTicOrientCombo, SIGNAL(activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->zTicOrientCombo, SIGNAL(activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->labelHeightEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->labelDigitsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->ticWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisColorButton,SIGNAL(clicked()), this, SLOT(selectAxisColor()));
	connect (vizFeatureDlg->buttonOk, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect (vizFeatureDlg->buttonCancel,SIGNAL(clicked()), featureHolder, SLOT(reject()));
	connect (vizFeatureDlg->buttonOk2, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect (vizFeatureDlg->buttonCancel2,SIGNAL(clicked()), featureHolder, SLOT(reject()));
	connect (this, SIGNAL(doneWithIt()), featureHolder, SLOT(reject()));
	connect (vizFeatureDlg->buttonHelp, SIGNAL(released()), this, SLOT(doHelp()));
	connect (vizFeatureDlg->buttonHelp2, SIGNAL(released()), this, SLOT(doHelp()));
	connect (vizFeatureDlg->imageCheckbox, SIGNAL(toggled(bool)), this, SLOT(imageToggled(bool)));
	connect (vizFeatureDlg->imageRotationCombo, SIGNAL(activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->imageUpDownCombo, SIGNAL(activated(int)), this, SLOT(panelChanged()));

	connect (vizFeatureDlg->timeLLXEdit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeLLYEdit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeCombo,SIGNAL(activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeColorButton,SIGNAL(clicked()), this, SLOT(selectTimeTextColor()));
	connect(vizFeatureDlg->variableCombo, SIGNAL(activated(int)), this, SLOT(setVariableNum(int)));
	connect(vizFeatureDlg->displacementEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect(vizFeatureDlg->lowValEdit, SIGNAL(returnPressed()), this, SLOT(setOutsideVal()));
	connect(vizFeatureDlg->highValEdit, SIGNAL(returnPressed()), this, SLOT(setOutsideVal()));
	connect(vizFeatureDlg->extendDownCheckbox, SIGNAL(clicked()), this, SLOT(setOutsideVal()));
	connect(vizFeatureDlg->extendUpCheckbox, SIGNAL(clicked()), this, SLOT(setOutsideVal()));
	connect(vizFeatureDlg->highValEdit,SIGNAL(textChanged(const QString&)), this, SLOT(changeOutsideVal(const QString&)));
	connect(vizFeatureDlg->lowValEdit,SIGNAL(textChanged(const QString&)), this, SLOT(changeOutsideVal(const QString&)));
	connect(vizFeatureDlg->stretch0Edit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect(vizFeatureDlg->stretch1Edit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect(vizFeatureDlg->stretch2Edit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	featureHolder->exec();
	
}
void VizFeatureParams::
setVariableNum(int varNum){
	//Save values from previous setting:
	if(newOutVals){
		setOutsideVal();
	}
	sessionVariableNum = varNum;
	vizFeatureDlg->lowValEdit->setText(QString::number(tempLowValues[sessionVariableNum]));
	vizFeatureDlg->highValEdit->setText(QString::number(tempHighValues[sessionVariableNum]));
	vizFeatureDlg->extendDownCheckbox->setChecked(tempExtendDown[sessionVariableNum]);
	vizFeatureDlg->extendUpCheckbox->setChecked(tempExtendUp[sessionVariableNum]);
	vizFeatureDlg->highValEdit->setEnabled(!vizFeatureDlg->extendUpCheckbox->isChecked());
	vizFeatureDlg->lowValEdit->setEnabled(!vizFeatureDlg->extendDownCheckbox->isChecked());
	dialogChanged = true;
}
void VizFeatureParams::
setOutsideVal(){
	float aboveVal = vizFeatureDlg->highValEdit->text().toFloat();
	float belowVal = vizFeatureDlg->lowValEdit->text().toFloat();
	tempLowValues[sessionVariableNum] = belowVal;
	tempHighValues[sessionVariableNum] = aboveVal;
	tempExtendUp[sessionVariableNum] = vizFeatureDlg->extendUpCheckbox->isChecked();
	tempExtendDown[sessionVariableNum] = vizFeatureDlg->extendDownCheckbox->isChecked();
	vizFeatureDlg->highValEdit->setEnabled(!vizFeatureDlg->extendUpCheckbox->isChecked());
	vizFeatureDlg->lowValEdit->setEnabled(!vizFeatureDlg->extendDownCheckbox->isChecked());
	newOutVals = true;
	dialogChanged = true;
	
}
void VizFeatureParams::
changeOutsideVal(const QString&){
	newOutVals = true;
	dialogChanged = true;
}

//Slots to identify that a change has occurred
void VizFeatureParams::
panelChanged(){
	dialogChanged = true;
	vizFeatureDlg->update();
}
//Respond to clicking "apply" button
//
void VizFeatureParams::
applySettings(){
	if (dialogChanged){
		copyFromDialog();
		dialogChanged = false;
	}
}
//Slot to respond to user changing selected visualizer in combo box
void VizFeatureParams::
visualizerSelected(int comboIndex){
	if (comboIndex == currentComboIndex) return;
	if(dialogChanged){
		int rc = QMessageBox::warning(vizFeatureDlg,"Save changes?",QString("Save changes to ") + vizName + "?",
			QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
		 
		if (rc == QMessageBox::Yes){ //put changes into previous visualizer
			copyFromDialog();
		}
		else if (rc == QMessageBox::Cancel) { //go back to previous setting
			vizFeatureDlg->currentNameCombo->setCurrentIndex(currentComboIndex);
			return;
		}
		dialogChanged = false;
	}
	currentComboIndex = comboIndex;
	//Copy values into dialog, using current comboIndex:
	setDialog();
	//Show new values:
	vizFeatureDlg->update();
}

void VizFeatureParams::
selectTimeTextColor(){
	QPalette pal(vizFeatureDlg->timeColorEdit->palette());
	tempTimeAnnotColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!tempTimeAnnotColor.isValid()) return;
	pal.setColor(QPalette::Base,tempTimeAnnotColor);
	vizFeatureDlg->timeColorEdit->setPalette(pal);
	dialogChanged = true;
}


void VizFeatureParams::
selectColorbarBackgroundColor(){
	//Launch colorselector, put result into the button
	QPalette pal(vizFeatureDlg->colorbarBackgroundEdit->palette());
	tempColorbarBackgroundColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!tempColorbarBackgroundColor.isValid()) return;
	pal.setColor(QPalette::Base,tempColorbarBackgroundColor);
	vizFeatureDlg->colorbarBackgroundEdit->setPalette(pal);
	dialogChanged = true;
}
void VizFeatureParams::
selectElevGridColor(){
	//Launch colorselector, put result into the button
	QPalette pal(vizFeatureDlg->surfaceColorEdit->palette());
	tempElevGridColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!tempElevGridColor.isValid()) return;
	pal.setColor(QPalette::Base,tempElevGridColor);
	vizFeatureDlg->surfaceColorEdit->setPalette(pal);
	dialogChanged = true;
	
}
	
void VizFeatureParams::
selectAxisColor(){
	QPalette pal(vizFeatureDlg->axisColorEdit->palette());
	tempAxisAnnotationColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!tempAxisAnnotationColor.isValid()) return;
	pal.setColor(QPalette::Base,tempAxisAnnotationColor);
	vizFeatureDlg->axisColorEdit->setPalette(pal);
	dialogChanged = true;

}
//Copy values into 'this' and into the dialog, using current comboIndex
//Also set up the visualizer combo
//
void VizFeatureParams::
setDialog(){

	int i;
	vizFeatureDlg->stretch0Edit->setText(QString::number(stretch[0]));
	vizFeatureDlg->stretch1Edit->setText(QString::number(stretch[1]));
	vizFeatureDlg->stretch2Edit->setText(QString::number(stretch[2]));

	Session* currentSession = Session::getInstance();
	bool enableStretch = !currentSession->sphericalTransform();
    vizFeatureDlg->stretch0Edit->setEnabled(enableStretch);
    vizFeatureDlg->stretch1Edit->setEnabled(enableStretch);
    vizFeatureDlg->stretch2Edit->setEnabled(enableStretch);

	DataStatus* ds = DataStatus::getInstance();
	bool isLayered = ds->dataIsLayered();
	vizFeatureDlg->outsideValFrame->setEnabled(isLayered);
	vizFeatureDlg->variableCombo->setCurrentIndex(sessionVariableNum);
	if (isLayered) vizFeatureDlg->buttonOk->setDefault(false);
	if (ds->getNumSessionVariables()>0){
		vizFeatureDlg->lowValEdit->setText(QString::number(ds->getBelowValue(sessionVariableNum)));
		vizFeatureDlg->highValEdit->setText(QString::number(ds->getAboveValue(sessionVariableNum)));
		vizFeatureDlg->variableCombo->clear();
		for (int i = 0; i<ds->getNumSessionVariables(); i++){
			vizFeatureDlg->variableCombo->addItem(ds->getVariableName(i).c_str());
		}
	}
	int vizNum = getVizNum(currentComboIndex);
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* vizWin = vizWinMgr->getVizWin(vizNum);

	//Put all the visualizer names into the combo box:
	vizFeatureDlg->currentNameCombo->clear();
	for (i = 0; i<MAXVIZWINS; i++){
		if (vizWinMgr->getVizWin(i))
			vizFeatureDlg->currentNameCombo->addItem(vizWinMgr->getVizWinName(i));
	}
	vizFeatureDlg->currentNameCombo->setCurrentIndex(currentComboIndex);
	//Save the current one:
	vizName = vizWinMgr->getVizWinName(vizNum);
	vizFeatureDlg->vizNameEdit->setText(vizName);
	for (i = 0; i<3; i++){
		axisArrowCoords[i] = vizWin->getAxisArrowCoord(i);
	}
	vizFeatureDlg->axisXEdit->setText(QString::number(axisArrowCoords[0]));
	vizFeatureDlg->axisYEdit->setText(QString::number(axisArrowCoords[1]));
	vizFeatureDlg->axisZEdit->setText(QString::number(axisArrowCoords[2]));

	//Handle axis annotation parameters.
	showAxisAnnotation = vizWin->axisAnnotationIsEnabled();

	vizFeatureDlg->axisAnnotationCheckbox->setChecked(showAxisAnnotation);
	//axis annotation setup:
	for (i = 0; i<3; i++) axisOriginCoords[i] = vizWin->getAxisOriginCoord(i);
	vizFeatureDlg->axisOriginXEdit->setText(QString::number(axisOriginCoords[0]));
	vizFeatureDlg->axisOriginYEdit->setText(QString::number(axisOriginCoords[1]));
	vizFeatureDlg->axisOriginZEdit->setText(QString::number(axisOriginCoords[2]));
	for (i = 0; i< 3; i++) minTic[i] = vizWin->getMinTic(i);
	for (i = 0; i< 3; i++) maxTic[i] = vizWin->getMaxTic(i);
	for (i = 0; i< 3; i++) numTics[i] = vizWin->getNumTics(i);
	for (i = 0; i< 3; i++) ticLength[i] = vizWin->getTicLength(i);
	for (i = 0; i< 3; i++) ticDir[i] = vizWin->getTicDir(i);
	vizFeatureDlg->xMinTicEdit->setText(QString::number(minTic[0]));
	vizFeatureDlg->yMinTicEdit->setText(QString::number(minTic[1]));
	vizFeatureDlg->zMinTicEdit->setText(QString::number(minTic[2]));
	vizFeatureDlg->xMaxTicEdit->setText(QString::number(maxTic[0]));
	vizFeatureDlg->yMaxTicEdit->setText(QString::number(maxTic[1]));
	vizFeatureDlg->zMaxTicEdit->setText(QString::number(maxTic[2]));
	vizFeatureDlg->xNumTicsEdit->setText(QString::number(numTics[0]));
	vizFeatureDlg->yNumTicsEdit->setText(QString::number(numTics[1]));
	vizFeatureDlg->zNumTicsEdit->setText(QString::number(numTics[2]));
	vizFeatureDlg->xTicSizeEdit->setText(QString::number(ticLength[0]));
	vizFeatureDlg->yTicSizeEdit->setText(QString::number(ticLength[1]));
	vizFeatureDlg->zTicSizeEdit->setText(QString::number(ticLength[2]));
	vizFeatureDlg->xTicOrientCombo->setCurrentIndex(ticDir[0]-1);
	vizFeatureDlg->yTicOrientCombo->setCurrentIndex(ticDir[1]/2);
	vizFeatureDlg->zTicOrientCombo->setCurrentIndex(ticDir[2]);
	labelHeight = vizWin->getLabelHeight();
	labelDigits = vizWin->getLabelDigits();
	ticWidth = vizWin->getTicWidth();
	vizFeatureDlg->labelHeightEdit->setText(QString::number(labelHeight));
	vizFeatureDlg->labelDigitsEdit->setText(QString::number(labelDigits));
	vizFeatureDlg->ticWidthEdit->setText(QString::number(ticWidth));
	axisAnnotationColor = vizWin->getAxisColor();
	tempAxisAnnotationColor = axisAnnotationColor;
	QPalette pal0(vizFeatureDlg->axisColorEdit->palette());
	pal0.setColor(QPalette::Base,axisAnnotationColor);
	vizFeatureDlg->axisColorEdit->setPalette(pal0);
	
	
	
	colorbarLLCoords[0] = vizWin->getColorbarLLCoord(0);
	colorbarLLCoords[1] = vizWin->getColorbarLLCoord(1);
	vizFeatureDlg->colorbarLLXEdit->setText(QString::number(colorbarLLCoords[0]));
	vizFeatureDlg->colorbarLLYEdit->setText(QString::number(colorbarLLCoords[1]));
	colorbarURCoords[0] = vizWin->getColorbarURCoord(0);
	colorbarURCoords[1] = vizWin->getColorbarURCoord(1);
	vizFeatureDlg->colorbarXSizeEdit->setText(QString::number(colorbarURCoords[0]-colorbarLLCoords[0]));
	vizFeatureDlg->colorbarYSizeEdit->setText(QString::number(colorbarURCoords[1]-colorbarLLCoords[1]));

	timeAnnotCoords[0] = vizWin->getTimeAnnotCoord(0);
	timeAnnotCoords[1] = vizWin->getTimeAnnotCoord(1);
	timeAnnotType = vizWin->getTimeAnnotType();
	timeAnnotColor = vizWin->getTimeAnnotColor();
	tempTimeAnnotColor=timeAnnotColor;
	timeAnnotTextSize = vizWin->getTimeAnnotTextSize();
	vizFeatureDlg->timeCombo->setCurrentIndex(timeAnnotType);
	vizFeatureDlg->timeLLXEdit->setText(QString::number(timeAnnotCoords[0]));
	vizFeatureDlg->timeLLYEdit->setText(QString::number(timeAnnotCoords[1]));
	vizFeatureDlg->timeSizeEdit->setText(QString::number(timeAnnotTextSize));
	QPalette pal1(vizFeatureDlg->timeColorEdit->palette());
	pal1.setColor(QPalette::Base, timeAnnotColor);
	vizFeatureDlg->timeColorEdit->setPalette(pal1);

	numColorbarTics = vizWin->getColorbarNumTics();
	vizFeatureDlg->numTicsEdit->setText(QString::number(numColorbarTics));
	showBar = vizWin->colorbarIsEnabled();
	vizFeatureDlg->colorbarCheckbox->setChecked(showBar);
	showAxisArrows = vizWin->axisArrowsAreEnabled();
	vizFeatureDlg->axisCheckbox->setChecked(showAxisArrows);
	
	colorbarBackgroundColor = vizWin->getColorbarBackgroundColor();
	tempColorbarBackgroundColor = colorbarBackgroundColor;
	QPalette pal2(vizFeatureDlg->colorbarBackgroundEdit->palette());
	pal2.setColor(QPalette::Base, colorbarBackgroundColor);
	vizFeatureDlg->colorbarBackgroundEdit->setPalette(pal2);

	int numRefs = ds->getNumTransforms();
	//if (isLayered){
		vizFeatureDlg->refinementCombo->setMaxCount(numRefs+1);
		vizFeatureDlg->refinementCombo->clear();
		for (i = 0; i<= numRefs; i++){
			vizFeatureDlg->refinementCombo->addItem(QString::number(i));
		}
	//}
	displacement = vizWin->getDisplacement();
	vizFeatureDlg->displacementEdit->setText(QString::number(displacement));
	elevGridColor = vizWin->getElevGridColor();
	tempElevGridColor = elevGridColor;
	elevGridRefinement = vizWin->getElevGridRefinementLevel();
	QPalette pal3(vizFeatureDlg->surfaceColorEdit->palette());
	pal3.setColor(QPalette::Base, elevGridColor);
	vizFeatureDlg->surfaceColorEdit->setPalette(pal3);

	vizFeatureDlg->refinementCombo->setCurrentIndex(elevGridRefinement);
	showElevGrid = vizWin->elevGridRenderingEnabled();
	vizFeatureDlg->surfaceCheckbox->setChecked(showElevGrid);
	textureSurface = vizWin->elevGridTextureEnabled();
	vizFeatureDlg->imageCheckbox->setChecked(textureSurface);
	surfaceUpsideDown = vizWin->textureInverted();
	vizFeatureDlg->imageUpDownCombo->setCurrentIndex(surfaceUpsideDown ? 1 : 0);
	surfaceRotation = vizWin->getTextureRotation();
	vizFeatureDlg->imageRotationCombo->setCurrentIndex(surfaceRotation/90);
	surfaceImageFilename = vizWin->getTextureFile();
	vizFeatureDlg->imageFilenameEdit->setText(surfaceImageFilename);
	

}
//Copy values from the dialog into 'this', and also to the visualizer state specified
//by the currentComboIndex (not the actual combo index).  This event gets captured in the
//the undo/redo history
//
void VizFeatureParams::
copyFromDialog(){
	
	
	int vizNum = getVizNum(currentComboIndex);
	//Make copy for history.  Note that the "currentComboIndex" is not part
	//Of the state that will modify visualizer
	VizFeatureCommand* cmd = VizFeatureCommand::captureStart(this, "Feature edit", vizNum);

	if (newOutVals){
		float aboveVal = vizFeatureDlg->highValEdit->text().toFloat();
		float belowVal = vizFeatureDlg->lowValEdit->text().toFloat();
		tempLowValues[sessionVariableNum] = belowVal;
		tempHighValues[sessionVariableNum] = aboveVal;
		tempExtendUp[sessionVariableNum] = vizFeatureDlg->extendUpCheckbox->isChecked();
		tempExtendDown[sessionVariableNum] = vizFeatureDlg->extendDownCheckbox->isChecked();
		//Now copy all temp values to perm values:
		for (int i = 0; i<lowValues.size(); i++){
			lowValues[i] = tempLowValues[i];
			highValues[i] = tempHighValues[i];
			extendDown[i] = tempExtendDown[i];
			extendUp[i] = tempExtendUp[i];
		}
		newOutVals = false;
	}
	

	stretch[0] = vizFeatureDlg->stretch0Edit->text().toFloat();
	stretch[1] = vizFeatureDlg->stretch1Edit->text().toFloat();
	stretch[2] = vizFeatureDlg->stretch2Edit->text().toFloat();
	
	if (vizFeatureDlg->vizNameEdit->text() != vizName){
		vizName = vizFeatureDlg->vizNameEdit->text();
	}
	axisArrowCoords[0] = vizFeatureDlg->axisXEdit->text().toFloat();
	axisArrowCoords[1] = vizFeatureDlg->axisYEdit->text().toFloat();
	axisArrowCoords[2] = vizFeatureDlg->axisZEdit->text().toFloat();
	axisOriginCoords[0] = vizFeatureDlg->axisOriginXEdit->text().toFloat();
	axisOriginCoords[1] = vizFeatureDlg->axisOriginYEdit->text().toFloat();
	axisOriginCoords[2] = vizFeatureDlg->axisOriginZEdit->text().toFloat();
	minTic[0] = vizFeatureDlg->xMinTicEdit->text().toFloat();
	minTic[1] = vizFeatureDlg->yMinTicEdit->text().toFloat();
	minTic[2] = vizFeatureDlg->zMinTicEdit->text().toFloat();
	maxTic[0] = vizFeatureDlg->xMaxTicEdit->text().toFloat();
	maxTic[1] = vizFeatureDlg->yMaxTicEdit->text().toFloat();
	maxTic[2] = vizFeatureDlg->zMaxTicEdit->text().toFloat();
	numTics[0] = vizFeatureDlg->xNumTicsEdit->text().toInt();
	numTics[1] = vizFeatureDlg->yNumTicsEdit->text().toInt();
	numTics[2] = vizFeatureDlg->zNumTicsEdit->text().toInt();
	ticLength[0] = vizFeatureDlg->xTicSizeEdit->text().toFloat();
	ticLength[1] = vizFeatureDlg->yTicSizeEdit->text().toFloat();
	ticLength[2] = vizFeatureDlg->zTicSizeEdit->text().toFloat();
	ticDir[0] = vizFeatureDlg->xTicOrientCombo->currentIndex()+1;
	ticDir[1] = vizFeatureDlg->yTicOrientCombo->currentIndex()*2;
	ticDir[2] = vizFeatureDlg->zTicOrientCombo->currentIndex();
	labelHeight = vizFeatureDlg->labelHeightEdit->text().toInt();
	labelDigits = vizFeatureDlg->labelDigitsEdit->text().toInt();
	ticWidth = vizFeatureDlg->ticWidthEdit->text().toFloat();

	axisAnnotationColor = tempAxisAnnotationColor;

	showAxisAnnotation = vizFeatureDlg->axisAnnotationCheckbox->isChecked();
	
	timeAnnotCoords[0] = vizFeatureDlg->timeLLXEdit->text().toFloat();
	timeAnnotCoords[1] = vizFeatureDlg->timeLLYEdit->text().toFloat();
	//validate:
	if (timeAnnotCoords[0] < 0.f) timeAnnotCoords[0] = 0.f;
	if (timeAnnotCoords[0] > 1.f) timeAnnotCoords[0] = 1.f;
	if (timeAnnotCoords[1] < 0.f) timeAnnotCoords[0] = 0.f;
	if (timeAnnotCoords[1] > 1.f) timeAnnotCoords[0] = 1.f;

	timeAnnotType = vizFeatureDlg->timeCombo->currentIndex();
	timeAnnotTextSize = vizFeatureDlg->timeSizeEdit->text().toInt();

	timeAnnotColor = tempTimeAnnotColor;

	colorbarLLCoords[0] = vizFeatureDlg->colorbarLLXEdit->text().toFloat();
	colorbarLLCoords[1] = vizFeatureDlg->colorbarLLYEdit->text().toFloat();
	float wid = vizFeatureDlg->colorbarXSizeEdit->text().toFloat();
	float ht = vizFeatureDlg->colorbarYSizeEdit->text().toFloat();

	//Make sure they're valid.  It's OK to place off-screen, but can't have negative dimensions
	if (wid < 0.01){
		wid = 0.1f;
		vizFeatureDlg->colorbarXSizeEdit->setText(QString::number(wid));
	}
	if (ht < 0.01){
		ht = 0.1f;
		vizFeatureDlg->colorbarYSizeEdit->setText(QString::number(ht));
	}
	colorbarURCoords[0] = colorbarLLCoords[0]+wid;
	colorbarURCoords[1] = colorbarLLCoords[1]+ht;
	
	numColorbarTics = vizFeatureDlg->numTicsEdit->text().toInt();
	if (numColorbarTics <2) numColorbarTics = 0;
	if (numColorbarTics > 50) numColorbarTics = 50;
	showBar = vizFeatureDlg->colorbarCheckbox->isChecked();
	showAxisArrows = vizFeatureDlg->axisCheckbox->isChecked();

	colorbarBackgroundColor = tempColorbarBackgroundColor;

	displacement = vizFeatureDlg->displacementEdit->text().toFloat();

	elevGridColor = tempElevGridColor;

	showElevGrid = vizFeatureDlg->surfaceCheckbox->isChecked();
	elevGridRefinement = vizFeatureDlg->refinementCombo->currentIndex();
	textureSurface = vizFeatureDlg->imageCheckbox->isChecked();
	surfaceRotation = vizFeatureDlg->imageRotationCombo->currentIndex()*90;
	surfaceUpsideDown = (vizFeatureDlg->imageUpDownCombo->currentIndex() == 1);
	surfaceImageFilename = vizFeatureDlg->imageFilenameEdit->text();

	applyToViz(vizNum);
	
	
	
	//Save the new visualizer state in the history
	VizFeatureCommand::captureEnd(cmd, this);
	
	
}
//Apply these settings to specified visualizer, and to global state:
void VizFeatureParams::
applyToViz(int vizNum){
	
	int i;
	DataStatus* ds = DataStatus::getInstance();
	
	bool changedOutVals = false;
	for (i = 0; i<ds->getNumSessionVariables(); i++){
	
		if(ds->getBelowValue(i) != lowValues[i] || ds->getAboveValue(i) != highValues[i]
			|| ds->isExtendedDown(i) != extendDown[i] || ds->isExtendedUp(i) != extendUp[i])
		{
			ds->setOutsideValues(i, lowValues[i],highValues[i],extendDown[i],extendUp[i]);
			changedOutVals = true;
		}
	}
	if (changedOutVals){
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		
		for (int j = 0; j< MAXVIZWINS; j++) {
			VizWin* win = vizMgr->getVizWin(j);
			if (!win) continue;
			//do each dvr
			
			DvrParams* dp = (DvrParams*)vizMgr->getDvrParams(j);
			vizMgr->setDatarangeDirty(dp);
			win->setDirtyBit(RegionBit, true);
		}
		if (ds->dataIsLayered()){	
			DataMgr *dataMgr = ds->getDataMgr();
			LayeredIO   *dataMgrLayered = dynamic_cast<LayeredIO *>(dataMgr);

			//construct a list of the non extended variables
			std::vector<string> vNames;
			std::vector<float> vals;
			for (int i = 0; i< ds->getNumSessionVariables(); i++){
				if (!ds->isExtendedDown(i)){
					vNames.push_back(ds->getVariableName(i));
					vals.push_back(ds->getBelowValue(i));
				}
			}
			dataMgrLayered->SetLowVals(vNames, vals);
			vNames.clear();
			for (int i = 0; i< ds->getNumSessionVariables(); i++){
				if (!ds->isExtendedUp(i)){
					vNames.push_back(ds->getVariableName(i));
					vals.push_back(ds->getAboveValue(i));
				}
			}
			dataMgrLayered->SetHighVals(vNames, vals);
		}
		//Must purge the cache when the outside values change:
		ds->getDataMgr()->Clear();
	}
	
	bool stretchChanged = false;
	float oldStretch[3];
	float ratio[3] = { 1.f, 1.f, 1.f };
	for (i = 0; i<3; i++) oldStretch[i] = ds->getStretchFactors()[i];
	
	float minStretch = 1.e30f;
	for (i= 0; i<3; i++){
		if (stretch[i] <= 0.f) stretch[i] = 1.f;
		if (stretch[i] < minStretch) minStretch = stretch[i];
	}
	//Normalize so minimum stretch is 1
	for ( i= 0; i<3; i++){
		if (stretch[i] != 1.f) stretch[i] /= minStretch;
		if (stretch[i] != oldStretch[i]){
			ratio[i] = stretch[i]/oldStretch[i];
			stretchChanged = true;
			Session::getInstance()->setStretch(i, stretch[i]);
		}
	}
	if (stretchChanged) {
		
		DataStatus* ds = DataStatus::getInstance();
		ds->stretchExtents(stretch);
		
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		vizMgr->setAllTwoDElevDirty();
		int timestep = vizMgr->getActiveAnimationParams()->getCurrentFrameNumber();
		//Set the region dirty bit in every window:
		bool firstShared = false;
		for (int j = 0; j< MAXVIZWINS; j++) {
			VizWin* win = vizMgr->getVizWin(j);
			if (!win) continue;
			
			//Only do each viewpoint params once
			ViewpointParams* vpp = vizMgr->getViewpointParams(j);
			
			if (!vpp->isLocal()) {
				if(firstShared) continue;
				else firstShared = true;
			}
			vpp->rescale(ratio, timestep);
			vpp->setCoordTrans();
			win->setValuesFromGui(vpp);
			vizMgr->resetViews(vizMgr->getRegionParams(j),vpp);
			vizMgr->setViewerCoordsChanged(vpp);
			win->setDirtyBit(RegionBit, true);
		}
		
	}
	
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* vizWin = vizWinMgr->getVizWin(vizNum);
	vizWinMgr->setVizWinName(vizNum, vizName);
	for (i = 0; i<3; i++){
		vizWin->setAxisArrowCoord(i, axisArrowCoords[i]);
		vizWin->setAxisOriginCoord(i, axisOriginCoords[i]);
		vizWin->setMinTic(i, minTic[i]);
		vizWin->setMaxTic(i, maxTic[i]);
		vizWin->setNumTics(i, numTics[i]);
		vizWin->setTicLength(i, ticLength[i]);
		vizWin->setTicDir(i, ticDir[i]);
	}
	vizWin->setAxisColor(axisAnnotationColor);
	vizWin->setTicWidth(ticWidth);
	vizWin->setLabelHeight(labelHeight);
	vizWin->setLabelDigits(labelDigits);
	vizWin->setColorbarLLCoord(0,colorbarLLCoords[0]);
	vizWin->setColorbarLLCoord(1,colorbarLLCoords[1]);
	vizWin->setColorbarURCoord(0,colorbarURCoords[0]);
	vizWin->setColorbarURCoord(1,colorbarURCoords[1]);
	
	vizWin->enableColorbar(showBar);
	vizWin->enableAxisArrows(showAxisArrows);
	vizWin->enableAxisAnnotation(showAxisAnnotation);
	
	vizWin->setColorbarNumTics(numColorbarTics);
	vizWin->setColorbarBackgroundColor(colorbarBackgroundColor);

	vizWin->setTimeAnnotCoords(timeAnnotCoords);
	vizWin->setTimeAnnotColor(timeAnnotColor);
	vizWin->setTimeAnnotTextSize(timeAnnotTextSize);
	vizWin->setTimeAnnotType(timeAnnotType);
	
	vizWin->setDisplacement(displacement);
	vizWin->setElevGridColor(elevGridColor);
	vizWin->enableElevGridRendering(showElevGrid);
	vizWin->setElevGridRefinementLevel(elevGridRefinement);
	vizWin->enableElevGridTexture(textureSurface); 
	vizWin->rotateTexture(surfaceRotation);
	vizWin->invertTexture(surfaceUpsideDown);
	vizWin->setTextureFile(surfaceImageFilename);

	vizWin->getGLWindow()->invalidateElevGrid();
	vizWin->setColorbarDirty(true);
	vizWin->updateGL();
	
}
int VizFeatureParams::
getComboIndex(int vizNum){
	//Determine the combo index that corresponds to a particular visualizer
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	//Count how many visualizers precede this one:
	int numPrecede = 0;
	for (int i = 0; i<vizNum; i++){
		if (vizMgr->getVizWin(i)) numPrecede++;
	}
	return numPrecede;
}
int VizFeatureParams::
getVizNum(int comboIndex){
	//Determine the visualizer that corresponds to a particular index
	//These are always in the same order as stored in the VizWinMgr
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int vizCount = -1;
	for (int i = 0; i<MAXVIZWINS; i++){
		if (vizMgr->getVizWin(i)) vizCount++;
		if (vizCount == comboIndex) return i;
	}
	assert(0);
	return -1;
}
// Response to a click on axisAnnotation checkbox:
void VizFeatureParams::annotationChanged(){
	if (vizFeatureDlg->axisAnnotationCheckbox->isChecked()){
		vizFeatureDlg->axisAnnotationFrame->show();
	} else {
		vizFeatureDlg->axisAnnotationFrame->hide();
	}
	
	vizFeatureDlg->update();
	dialogChanged = true;
}

void VizFeatureParams::okClicked(){

	//If user clicks OK, need to store values back to visualizer, plus
	//save changes in history
	if (dialogChanged) {
		copyFromDialog();
		dialogChanged = false;
	} 
	emit doneWithIt();
	
}
void VizFeatureParams::
doHelp(){
	QWhatsThis::enterWhatsThisMode();
}
void VizFeatureParams::
imageToggled(bool onOff){
	if (!onOff) {
		
		dialogChanged = true;
	}
	else { 
		//select a filename, if succeed turn on textureSurface.
		QString filename = QFileDialog::getOpenFileName(vizFeatureDlg,
			"Choose the Image File to map to terrain",
			surfaceImageFilename,
			"Image files (*.jpg)");
		if(filename.length() == 0) return;
		
		vizFeatureDlg->imageFilenameEdit->setText(filename);
		dialogChanged = true;
	}

}
void VizFeatureParams::checkSurface(bool on){
	if (!on) return;
	MessageReporter::warningMsg("Note: Improved terrain mapping capabilities are available\n%s",
		"in the Image panel and in the 2D Data panel.");
}
