//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		sessionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the SessionParams class.
//
#include "vizfeatureparams.h"
#include "command.h"
#include "vizwin.h"
#include "vizfeatures.h"
#include "vizwinmgr.h"
#include "messagereporter.h"
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qcolordialog.h>


using namespace VAPoR;
//Create a new vizfeatureparams
VizFeatureParams::VizFeatureParams(){
	dialogChanged = false;
	vizFeatureDlg = 0;
	currentComboIndex = -1;
}
//Clone a new vizfeatureparams
VizFeatureParams::VizFeatureParams(const VizFeatureParams& vfParams){
	dialogChanged = false;
	vizFeatureDlg = 0;
	currentComboIndex = vfParams.currentComboIndex;
	vizName = vfParams.vizName;
	showBar = vfParams.showBar;
	showAxes = vfParams.showAxes;
	showRegion = vfParams.showRegion;
	showSubregion = vfParams.showSubregion;
	axisCoords[0] = vfParams.axisCoords[0];
	axisCoords[1] = vfParams.axisCoords[1];
	axisCoords[2] = vfParams.axisCoords[2];
	colorbarCoords[0] = vfParams.colorbarCoords[0];
	colorbarCoords[1] = vfParams.colorbarCoords[1];
	backgroundColor = vfParams.backgroundColor;
	regionFrameColor = vfParams.regionFrameColor;
	subregionFrameColor = vfParams.subregionFrameColor;
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
	vizFeatureDlg = new VizFeatures((QWidget*)MainForm::getInstance());
	//Do connections.  
	int ok = connect(vizFeatureDlg->currentNameCombo, SIGNAL(activated(int)), this, SLOT(visualizerSelected(int)));
	assert(ok);
	connect (vizFeatureDlg->backgroundColorButton, SIGNAL(clicked()), this, SLOT(selectBackgroundColor()));
	connect (vizFeatureDlg->regionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectRegionFrameColor()));
	connect (vizFeatureDlg->subregionFrameColorButton, SIGNAL(clicked()), this, SLOT(selectSubregionFrameColor()));
	connect (vizFeatureDlg->vizNameEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisXEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisYEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisZEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarXEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarYEdit, SIGNAL(returnPressed()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->regionCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->subregionCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	
	
	
	//Copy values into dialog, using current comboIndex:
	setDialog();
	dialogChanged = false;
	
	
	int rc = vizFeatureDlg->exec();
	if (rc){
		//If user clicks OK, need to store values back to visualizer, plus
		//save changes in history
		if (dialogChanged) {
			copyFromDialog();
			
		} 
	} 
	delete vizFeatureDlg;
	vizFeatureDlg = 0;
}
//Slot to respond to user pressing "enter" in textbox, or making other change
void VizFeatureParams::
panelChanged(){
	dialogChanged = true;
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
		axisCoords[i] = vizWin->getAxisCoord(i);
	}
	vizFeatureDlg->axisXEdit->setText(QString::number(axisCoords[0]));
	vizFeatureDlg->axisYEdit->setText(QString::number(axisCoords[1]));
	vizFeatureDlg->axisZEdit->setText(QString::number(axisCoords[2]));
	
	colorbarCoords[0] = vizWin->getColorbarCoord(0);
	colorbarCoords[1] = vizWin->getColorbarCoord(1);
	vizFeatureDlg->colorbarXEdit->setText(QString::number(colorbarCoords[0]));
	vizFeatureDlg->colorbarYEdit->setText(QString::number(colorbarCoords[1]));

	
	showBar = vizWin->colorbarIsEnabled();
	vizFeatureDlg->colorbarCheckbox->setChecked(showBar);
	showAxes = vizWin->axesAreEnabled();
	vizFeatureDlg->axisCheckbox->setChecked(showAxes);
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
	axisCoords[0] = vizFeatureDlg->axisXEdit->text().toFloat();
	axisCoords[1] = vizFeatureDlg->axisYEdit->text().toFloat();
	axisCoords[2] = vizFeatureDlg->axisZEdit->text().toFloat();
	
	colorbarCoords[0] = vizFeatureDlg->colorbarXEdit->text().toInt();
	colorbarCoords[1] = vizFeatureDlg->colorbarYEdit->text().toInt();
	
	showBar = vizFeatureDlg->colorbarCheckbox->isChecked();
	showAxes = vizFeatureDlg->axisCheckbox->isChecked();
	showRegion = vizFeatureDlg->regionCheckbox->isChecked();
	showSubregion = vizFeatureDlg->subregionCheckbox->isChecked();

	regionFrameColor = vizFeatureDlg->regionFrameColorButton->paletteBackgroundColor();
	subregionFrameColor = vizFeatureDlg->subregionFrameColorButton->paletteBackgroundColor();
	backgroundColor = vizFeatureDlg->backgroundColorButton->paletteBackgroundColor();

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
		vizWin->setAxisCoord(i, axisCoords[i]);
	}
	vizWin->setColorbarCoord(0,colorbarCoords[0]);
	vizWin->setColorbarCoord(1,colorbarCoords[1]);
	vizWin->enableColorbar(showBar);
	vizWin->enableAxes(showAxes);
	vizWin->enableRegionFrame(showRegion);
	vizWin->enableSubregionFrame(showSubregion);
	
	vizWin->setBackgroundColor(backgroundColor);
	vizWin->setRegionFrameColor(regionFrameColor);
	vizWin->setSubregionFrameColor(subregionFrameColor);

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

