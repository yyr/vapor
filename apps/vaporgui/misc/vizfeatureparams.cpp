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
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qscrollview.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qwhatsthis.h>


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
	currentComboIndex = vfParams.currentComboIndex;
	vizName = vfParams.vizName;
	showBar = vfParams.showBar;
	showAxisArrows = vfParams.showAxisArrows;
	showRegion = vfParams.showRegion;
	showSubregion = vfParams.showSubregion;
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
	labelHeight = vfParams.labelHeight;
	labelDigits = vfParams.labelDigits;
	ticWidth = vfParams.ticWidth;

	colorbarLLCoords[0] = vfParams.colorbarLLCoords[0];
	colorbarLLCoords[1] = vfParams.colorbarLLCoords[1];
	colorbarURCoords[0] = vfParams.colorbarURCoords[0];
	colorbarURCoords[1] = vfParams.colorbarURCoords[1];
	numColorbarTics = vfParams.numColorbarTics;
	backgroundColor = vfParams.backgroundColor;
	colorbarBackgroundColor = vfParams.colorbarBackgroundColor;
	regionFrameColor = vfParams.regionFrameColor;
	subregionFrameColor = vfParams.subregionFrameColor;
	elevGridColor = vfParams.elevGridColor;
	showElevGrid = vfParams.showElevGrid;
	elevGridRefinement =  vfParams.elevGridRefinement;
}
//Set up the dialog with current parameters from current active visualizer
void VizFeatureParams::launch(){
	//Determine current combo entry
	int vizNum = VizWinMgr::getInstance()->getActiveViz();
	if (vizNum < 0) {
		QMessageBox::warning(vizFeatureDlg,"No visualizers",QString("No visualizers exist to be modified"),
			QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}
	//Determine corresponding combo index for currentVizNum:
	currentComboIndex = getComboIndex(vizNum);
	
	featureHolder = new ScrollContainer((QWidget*)MainForm::getInstance());
	
	QScrollView* sv = new QScrollView(featureHolder);
	sv->setHScrollBarMode(QScrollView::AlwaysOff);
	sv->setVScrollBarMode(QScrollView::AlwaysOn);
	featureHolder->setScroller(sv);
	
	vizFeatureDlg = new VizFeatures(featureHolder);
	
	sv->addChild(vizFeatureDlg);
	
	//Do connections.  
	connect(vizFeatureDlg->currentNameCombo, SIGNAL(activated(int)), this, SLOT(visualizerSelected(int)));
	connect (vizFeatureDlg->backgroundColorButton, SIGNAL(clicked()), this, SLOT(selectBackgroundColor()));
	connect (vizFeatureDlg->regionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectRegionFrameColor()));
	connect (vizFeatureDlg->subregionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectSubregionFrameColor()));
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
	connect (vizFeatureDlg->regionCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->subregionCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->surfaceCheckbox,SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->applyButton, SIGNAL(clicked()), this, SLOT(applySettings()));
	connect (vizFeatureDlg->refinementCombo, SIGNAL (activated(int)), this, SLOT(panelChanged()));

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
	connect (vizFeatureDlg->xTicOrientCombo, SIGNAL(activated(int)), this, SLOT(xTicOrientationChanged(int)));
	connect (vizFeatureDlg->yTicOrientCombo, SIGNAL(activated(int)), this, SLOT(yTicOrientationChanged(int)));
	connect (vizFeatureDlg->zTicOrientCombo, SIGNAL(activated(int)), this, SLOT(zTicOrientationChanged(int)));
	connect (vizFeatureDlg->labelHeightEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->labelDigitsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->ticWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisColorButton,SIGNAL(clicked()), this, SLOT(selectAxisColor()));
	connect (vizFeatureDlg->buttonOk, SIGNAL(clicked()), this, SLOT(okClicked()));
	connect (vizFeatureDlg->buttonCancel,SIGNAL(clicked()), featureHolder, SLOT(reject()));
	connect (this, SIGNAL(doneWithIt()), featureHolder, SLOT(reject()));
	connect(vizFeatureDlg->buttonHelp, SIGNAL(released()), this, SLOT(doHelp()));

	//Copy values into dialog, using current comboIndex:
	setDialog();
	dialogChanged = false;
	if (vizFeatureDlg->axisAnnotationCheckbox->isChecked()){
		vizFeatureDlg->axisAnnotationFrame->show();
	} else {
		vizFeatureDlg->axisAnnotationFrame->hide();
	}
	
	int h = 690;
	int w = 415;
	if (h > MainForm::getInstance()->height()){
		h = MainForm::getInstance()->height();
	}
	vizFeatureDlg->setGeometry(0, 0, w, h);
	int swidth = sv->verticalScrollBar()->width();
	featureHolder->setGeometry(50, 50, w+swidth,h);
	
	sv->resizeContents(w,h);
	
	featureHolder->exec();
	
}
//Slots to identify that a change has occurred
void VizFeatureParams::
panelChanged(){
	dialogChanged = true;
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
			vizFeatureDlg->currentNameCombo->setCurrentItem(currentComboIndex);
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
//Slots to respond to color changes:
void VizFeatureParams::
selectRegionFrameColor(){
	QColor frameColor = QColorDialog::getColor(regionFrameColor,0,0);
	vizFeatureDlg->regionFrameColorButton->setPaletteBackgroundColor(frameColor);
	dialogChanged = true;
}
void VizFeatureParams::
selectSubregionFrameColor(){
	QColor frameColor = QColorDialog::getColor(subregionFrameColor,0,0);
	vizFeatureDlg->subregionFrameColorButton->setPaletteBackgroundColor(frameColor);
	dialogChanged = true;
}
void VizFeatureParams::
selectBackgroundColor(){
	//Launch colorselector, put result into the button
	QColor bgColor = QColorDialog::getColor(backgroundColor,0,0);
	vizFeatureDlg->backgroundColorButton->setPaletteBackgroundColor(bgColor);
	dialogChanged = true;
}
void VizFeatureParams::
selectColorbarBackgroundColor(){
	//Launch colorselector, put result into the button
	QColor bgColor = QColorDialog::getColor(colorbarBackgroundColor,0,0);
	vizFeatureDlg->colorbarBackgroundButton->setPaletteBackgroundColor(bgColor);
	dialogChanged = true;
}
void VizFeatureParams::
selectElevGridColor(){
	//Launch colorselector, put result into the button
	QColor sfColor = QColorDialog::getColor(elevGridColor,0,0);
	vizFeatureDlg->surfaceColorButton->setPaletteBackgroundColor(sfColor);
	dialogChanged = true;
}
	
void VizFeatureParams::
selectAxisColor(){
	QColor axColor = QColorDialog::getColor(axisAnnotationColor,0,0);
	vizFeatureDlg->axisColorButton->setPaletteBackgroundColor(axColor);
	dialogChanged = true;
}
//Copy values into 'this' and into the dialog, using current comboIndex
//Also set up the visualizer combo
//
void VizFeatureParams::
setDialog(){
	int i;
	int vizNum = getVizNum(currentComboIndex);
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	VizWin* vizWin = vizWinMgr->getVizWin(vizNum);

	//Put all the visualizer names into the combo box:
	vizFeatureDlg->currentNameCombo->clear();
	for (i = 0; i<MAXVIZWINS; i++){
		if (vizWinMgr->getVizWin(i))
			vizFeatureDlg->currentNameCombo->insertItem(vizWinMgr->getVizWinName(i));
	}
	vizFeatureDlg->currentNameCombo->setCurrentItem(currentComboIndex);
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
	vizFeatureDlg->xTicOrientCombo->setCurrentItem(ticDir[0]-1);
	vizFeatureDlg->yTicOrientCombo->setCurrentItem(ticDir[1]/2);
	vizFeatureDlg->zTicOrientCombo->setCurrentItem(ticDir[2]);
	labelHeight = vizWin->getLabelHeight();
	labelDigits = vizWin->getLabelDigits();
	ticWidth = vizWin->getTicWidth();
	vizFeatureDlg->labelHeightEdit->setText(QString::number(labelHeight));
	vizFeatureDlg->labelDigitsEdit->setText(QString::number(labelDigits));
	vizFeatureDlg->ticWidthEdit->setText(QString::number(ticWidth));
	axisAnnotationColor = vizWin->getAxisColor();
	vizFeatureDlg->axisColorButton->setPaletteBackgroundColor(axisAnnotationColor);
	
	
	
	colorbarLLCoords[0] = vizWin->getColorbarLLCoord(0);
	colorbarLLCoords[1] = vizWin->getColorbarLLCoord(1);
	vizFeatureDlg->colorbarLLXEdit->setText(QString::number(colorbarLLCoords[0]));
	vizFeatureDlg->colorbarLLYEdit->setText(QString::number(colorbarLLCoords[1]));
	colorbarURCoords[0] = vizWin->getColorbarURCoord(0);
	colorbarURCoords[1] = vizWin->getColorbarURCoord(1);
	vizFeatureDlg->colorbarXSizeEdit->setText(QString::number(colorbarURCoords[0]-colorbarLLCoords[0]));
	vizFeatureDlg->colorbarYSizeEdit->setText(QString::number(colorbarURCoords[1]-colorbarLLCoords[1]));

	numColorbarTics = vizWin->getColorbarNumTics();
	vizFeatureDlg->numTicsEdit->setText(QString::number(numColorbarTics));
	showBar = vizWin->colorbarIsEnabled();
	vizFeatureDlg->colorbarCheckbox->setChecked(showBar);
	showAxisArrows = vizWin->axisArrowsAreEnabled();
	vizFeatureDlg->axisCheckbox->setChecked(showAxisArrows);
	showRegion = vizWin->regionFrameIsEnabled();
	vizFeatureDlg->regionCheckbox->setChecked(showRegion);
	showSubregion = vizWin->subregionFrameIsEnabled();
	vizFeatureDlg->subregionCheckbox->setChecked(showSubregion);
	
	backgroundColor = vizWin->getBackgroundColor();
	vizFeatureDlg->backgroundColorButton->setPaletteBackgroundColor(backgroundColor);
	regionFrameColor = vizWin->getRegionFrameColor();
	vizFeatureDlg->regionFrameColorButton->setPaletteBackgroundColor(regionFrameColor);
	subregionFrameColor = vizWin->getSubregionFrameColor();
	vizFeatureDlg->subregionFrameColorButton->setPaletteBackgroundColor(subregionFrameColor);
	colorbarBackgroundColor = vizWin->getColorbarBackgroundColor();
	vizFeatureDlg->colorbarBackgroundButton->setPaletteBackgroundColor(colorbarBackgroundColor);

	DataStatus* ds = DataStatus::getInstance();
	bool isLayered = ds->dataIsLayered();
	int numRefs = ds->getNumTransforms();
	if (isLayered){
		vizFeatureDlg->refinementCombo->setMaxCount(numRefs+1);
		vizFeatureDlg->refinementCombo->clear();
		for (i = 0; i<= numRefs; i++){
			vizFeatureDlg->refinementCombo->insertItem(QString::number(i));
		}
	}
	elevGridColor = vizWin->getElevGridColor();
	elevGridRefinement = vizWin->getElevGridRefinementLevel();
	vizFeatureDlg->surfaceColorButton->setPaletteBackgroundColor(elevGridColor);
	vizFeatureDlg->refinementCombo->setCurrentItem(elevGridRefinement);
	showElevGrid = (vizWin->elevGridRenderingEnabled() && isLayered);
	vizFeatureDlg->surfaceCheckbox->setChecked(showElevGrid);
	
	vizFeatureDlg->refinementCombo->setEnabled(isLayered);
	vizFeatureDlg->surfaceCheckbox->setEnabled(isLayered);
	vizFeatureDlg->surfaceColorButton->setEnabled(isLayered);

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
	ticDir[0] = vizFeatureDlg->xTicOrientCombo->currentItem()+1;
	ticDir[1] = vizFeatureDlg->yTicOrientCombo->currentItem()*2;
	ticDir[2] = vizFeatureDlg->zTicOrientCombo->currentItem();
	labelHeight = vizFeatureDlg->labelHeightEdit->text().toInt();
	labelDigits = vizFeatureDlg->labelDigitsEdit->text().toInt();
	ticWidth = vizFeatureDlg->ticWidthEdit->text().toFloat();
	axisAnnotationColor = vizFeatureDlg->axisColorButton->paletteBackgroundColor();
	showAxisAnnotation = vizFeatureDlg->axisAnnotationCheckbox->isChecked();
	
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
	showRegion = vizFeatureDlg->regionCheckbox->isChecked();
	showSubregion = vizFeatureDlg->subregionCheckbox->isChecked();

	regionFrameColor = vizFeatureDlg->regionFrameColorButton->paletteBackgroundColor();
	subregionFrameColor = vizFeatureDlg->subregionFrameColorButton->paletteBackgroundColor();
	backgroundColor = vizFeatureDlg->backgroundColorButton->paletteBackgroundColor();
	colorbarBackgroundColor = vizFeatureDlg->colorbarBackgroundButton->paletteBackgroundColor();

	elevGridColor = vizFeatureDlg->surfaceColorButton->paletteBackgroundColor();
	showElevGrid = vizFeatureDlg->surfaceCheckbox->isChecked();
	elevGridRefinement = vizFeatureDlg->refinementCombo->currentItem();
	applyToViz(vizNum);

	//Save the new visualizer state in the history
	VizFeatureCommand::captureEnd(cmd, this);
}
//Apply these settings to specified visualizer
void VizFeatureParams::
applyToViz(int vizNum){
	int i;
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
	vizWin->enableRegionFrame(showRegion);
	vizWin->enableSubregionFrame(showSubregion);
	vizWin->setColorbarNumTics(numColorbarTics);
	vizWin->setBackgroundColor(backgroundColor);
	vizWin->setColorbarBackgroundColor(colorbarBackgroundColor);
	vizWin->setRegionFrameColor(regionFrameColor);
	vizWin->setSubregionFrameColor(subregionFrameColor);
	
	vizWin->setElevGridColor(elevGridColor);
	vizWin->enableElevGridRendering(showElevGrid);
	vizWin->setElevGridRefinementLevel(elevGridRefinement);
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
void VizFeatureParams::xTicOrientationChanged(int val){
	ticDir[0] = val+1;
	dialogChanged = true;
}
void VizFeatureParams::yTicOrientationChanged(int val){
	ticDir[1] = 2*val;
	dialogChanged = true;
}
void VizFeatureParams::zTicOrientationChanged(int val){
	ticDir[2] = val;
	dialogChanged = true;
}
void VizFeatureParams::okClicked(){

	//If user clicks OK, need to store values back to visualizer, plus
	//save changes in history
	if (dialogChanged) {
		copyFromDialog();
	} 
	
	emit doneWithIt();
	
}
void VizFeatureParams::
doHelp(){
	QWhatsThis::enterWhatsThisMode();
}