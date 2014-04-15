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
#include "vizwin.h"
#include "vizfeatures.h"
#include "mainform.h"
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
#include "visualizer.h"


using namespace VAPoR;

//Create a new vizfeatureparams
VizFeatureParams::VizFeatureParams(){
	dialogChanged = false;
	vizFeatureDlg = 0;
	featureHolder = 0;
	currentComboIndex = -1;
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
	enableSpin = vfParams.enableSpin;
	
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
	
	showAxisAnnotation = vfParams.showAxisAnnotation;
	axisAnnotationColor = vfParams.axisAnnotationColor;
	tempAxisAnnotationColor = vfParams.tempAxisAnnotationColor;
	labelHeight = vfParams.labelHeight;
	labelDigits = vfParams.labelDigits;
	ticWidth = vfParams.ticWidth;

	colorbarDigits = vfParams.colorbarDigits;
	colorbarRendererTypeId = vfParams.colorbarRendererTypeId;
	colorbarFontsize = vfParams.colorbarFontsize;
	colorbarLLCoords[0] = vfParams.colorbarLLCoords[0];
	colorbarLLCoords[1] = vfParams.colorbarLLCoords[1];
	colorbarURCoords[0] = vfParams.colorbarURCoords[0];
	colorbarURCoords[1] = vfParams.colorbarURCoords[1];
	numColorbarTics = vfParams.numColorbarTics;
	
	colorbarBackgroundColor = vfParams.colorbarBackgroundColor;
	colorbarDigits = vfParams.colorbarDigits;
	colorbarRendererTypeId = vfParams.colorbarRendererTypeId;
	colorbarFontsize = vfParams.colorbarFontsize;
	

	timeAnnotCoords[0] = vfParams.timeAnnotCoords[0];
	timeAnnotCoords[1] = vfParams.timeAnnotCoords[1];
	timeAnnotColor = vfParams.timeAnnotColor;
	timeAnnotType = vfParams.timeAnnotType;
	timeAnnotTextSize = vfParams.timeAnnotTextSize;
	
	
}
//Set up the dialog with current parameters from current active visualizer
void VizFeatureParams::launch(){
	//Determine current combo entry
	int vizNum = VizWinMgr::getInstance()->getActiveViz();
	if (vizNum < 0 || !DataStatus::getInstance()->getDataMgr()){
		
		return;
	}
	
	
	
	featureHolder = new ScrollContainer((QWidget*)MainForm::getInstance(), "Visualizer Feature Selection");
	
	QScrollArea* sv = new QScrollArea(featureHolder);
	featureHolder->setScroller(sv);
	vizFeatureDlg = new VizFeatureDialog(featureHolder);
	sv->setWidget(vizFeatureDlg);
	
	//Copy values into dialog, using current comboIndex:
	setDialog();
	dialogChanged = false;
	
	
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
	vizFeatureDlg->adjustSize();
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
	
	connect (vizFeatureDlg->vizNameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisZEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarNumDigitsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarFontsizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarLLXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarLLYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarXSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarYSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->numTicsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarBackgroundButton, SIGNAL(clicked()), this, SLOT(selectColorbarBackgroundColor()));
	connect (vizFeatureDlg->axisCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->spinAnimationCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->rendererCombo, SIGNAL(activated(int)), this, SLOT(rendererChanged(int)));
	
	connect (vizFeatureDlg->applyButton, SIGNAL(clicked()), this, SLOT(applySettings()));
	connect (vizFeatureDlg->applyButton2, SIGNAL(clicked()), this, SLOT(applySettings()));
	
	connect (vizFeatureDlg->axisAnnotationCheckbox, SIGNAL(clicked()), this, SLOT(annotationChanged()));
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

	connect (vizFeatureDlg->timeLLXEdit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeLLYEdit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeSizeEdit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeCombo,SIGNAL(activated(int)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->timeColorButton,SIGNAL(clicked()), this, SLOT(selectTimeTextColor()));
	
	connect(vizFeatureDlg->stretch0Edit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect(vizFeatureDlg->stretch1Edit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect(vizFeatureDlg->stretch2Edit,SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect(vizFeatureDlg->reloadShadersButton,SIGNAL(clicked()), this, SLOT(reloadShaders()));
	featureHolder->exec();
	
}
void VizFeatureParams::
rendererChanged(int renIndex){
	colorbarRendererTypeId = rendererTypeLookup[renIndex];
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
selectAxisColor(){
	QPalette pal(vizFeatureDlg->axisColorEdit->palette());
	tempAxisAnnotationColor = QColorDialog::getColor(pal.color(QPalette::Base));
	if (!tempAxisAnnotationColor.isValid()) return;
	pal.setColor(QPalette::Base,tempAxisAnnotationColor);
	vizFeatureDlg->axisColorEdit->setPalette(pal);
	dialogChanged = true;

}
//Copy values into 'this' and into the dialog, using current comboIndex
//Also set up the visualizer combo and the rendererCombo
//
void VizFeatureParams::
setDialog(){

	
	vizFeatureDlg->stretch0Edit->setText(QString::number(stretch[0]));
	vizFeatureDlg->stretch1Edit->setText(QString::number(stretch[1]));
	vizFeatureDlg->stretch2Edit->setText(QString::number(stretch[2]));

	vizFeatureDlg->axisAnnotationCheckbox->setChecked(showAxisAnnotation);
	
	
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
	
	vizFeatureDlg->labelHeightEdit->setText(QString::number(labelHeight));
	vizFeatureDlg->labelDigitsEdit->setText(QString::number(labelDigits));
	vizFeatureDlg->ticWidthEdit->setText(QString::number(ticWidth));
	
	tempAxisAnnotationColor = axisAnnotationColor;
	QPalette pal0(vizFeatureDlg->axisColorEdit->palette());
	pal0.setColor(QPalette::Base,axisAnnotationColor);
	vizFeatureDlg->axisColorEdit->setPalette(pal0);
	
	
	
	int indx = 0;
	for (int i = 0; i< rendererTypeLookup.size(); i++){
		if(rendererTypeLookup[i] == colorbarRendererTypeId) indx = i;
	}
	vizFeatureDlg->rendererCombo->setCurrentIndex(indx);
	
	vizFeatureDlg->colorbarFontsizeEdit->setText(QString::number(colorbarFontsize));
	vizFeatureDlg->colorbarNumDigitsEdit->setText(QString::number(colorbarDigits));

	//Set up the renderer combo.
	vizFeatureDlg->rendererCombo->clear();
	rendererTypeLookup.clear();
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
	colorbarRendererTypeId = rendererTypeLookup[vizFeatureDlg->rendererCombo->currentIndex()];
	colorbarDigits = vizFeatureDlg->colorbarNumDigitsEdit->text().toInt();
	colorbarFontsize = vizFeatureDlg->colorbarFontsizeEdit->text().toInt();
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

	enableSpin = vizFeatureDlg->spinAnimationCheckbox->isChecked();

	colorbarBackgroundColor = tempColorbarBackgroundColor;

	
	applyToViz(vizNum);
	
	
	
	
}
//Apply these settings to specified visualizer, and to global state:
void VizFeatureParams::
applyToViz(int vizNum){
	
	int i;
	DataStatus* ds = DataStatus::getInstance();
	
	
	
	bool stretchChanged = false;
	double oldStretch[3];
	double ratio[3] = { 1.f, 1.f, 1.f };
	for (i = 0; i<3; i++) oldStretch[i] = ds->getStretchFactors()[i];
	
	double minStretch = 1.e30;
	for (i= 0; i<3; i++){
		if (stretch[i] <= 0.) stretch[i] = 1.;
		if (stretch[i] < minStretch) minStretch = stretch[i];
	}
	//Normalize so minimum stretch is 1
	for ( i= 0; i<3; i++){
		if (minStretch != 1.f) stretch[i] /= minStretch;
		if (stretch[i] != oldStretch[i]){
			ratio[i] = stretch[i]/oldStretch[i];
			stretchChanged = true;
		}
	}
	if (stretchChanged) {
		
		DataStatus* ds = DataStatus::getInstance();
		ds->stretchExtents(stretch);
		
		VizWinMgr* vizMgr = VizWinMgr::getInstance();
		
		int timestep = vizMgr->getActiveAnimationParams()->getCurrentTimestep();
		//Set the region dirty bit in every window:
		bool firstSharedVp = false;
		
		for (int j = 0; j< vizMgr->getNumVisualizers(); j++) {
			VizWin* win = vizMgr->getVizWin(j);
			if (!win) continue;
			
			//Only do each viewpoint params once
			ViewpointParams* vpp = vizMgr->getViewpointParams(j);
		
			//Rescale the keyframes of animation params,
			
			if (!vpp->IsLocal()) {
				if(firstSharedVp) continue;
				else firstSharedVp = true;
			}
			vpp->rescale(ratio, timestep);
			
			win->setValuesFromGui(vpp);
			vizMgr->resetViews(vpp);
			
		}
		
	}
	
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* vizWin = vizWinMgr->getVizWin(vizNum);
	vizWinMgr->setVizWinName(vizNum, vizName);
	
	
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
	for (int i = 0; i<vizMgr->getNumVisualizers(); i++){
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
void VizFeatureParams::checkSurface(bool on){
	if (!on) return;
}

