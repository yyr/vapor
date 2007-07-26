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
#include "mainform.h"
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
	colorbarLLCoords[0] = vfParams.colorbarLLCoords[0];
	colorbarLLCoords[1] = vfParams.colorbarLLCoords[1];
	colorbarURCoords[0] = vfParams.colorbarURCoords[0];
	colorbarURCoords[1] = vfParams.colorbarURCoords[1];
	numColorbarTics = vfParams.numColorbarTics;
	backgroundColor = vfParams.backgroundColor;
	colorbarBackgroundColor = vfParams.colorbarBackgroundColor;
	regionFrameColor = vfParams.regionFrameColor;
	subregionFrameColor = vfParams.subregionFrameColor;
	surfaceColor = vfParams.surfaceColor;
	showSurface = vfParams.showSurface;
	surfaceRefinement =  vfParams.surfaceRefinement;
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
	connect (vizFeatureDlg->surfaceColorButton,SIGNAL(clicked()), this, SLOT(selectSurfaceColor()));
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
	connect (vizFeatureDlg->refinementCombo, SIGNAL (activated()), this, SLOT(panelChanged()));

	
	
	
	
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
selectSurfaceColor(){
	//Launch colorselector, put result into the button
	QColor sfColor = QColorDialog::getColor(surfaceColor,0,0);
	vizFeatureDlg->surfaceColorButton->setPaletteBackgroundColor(sfColor);
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
	colorbarBackgroundColor = vizWin->getColorbarBackgroundColor();
	vizFeatureDlg->colorbarBackgroundButton->setPaletteBackgroundColor(colorbarBackgroundColor);

	DataStatus* ds = DataStatus::getInstance();
	bool isLayered = (ds->getMetadata() && (StrCmpNoCase(ds->getMetadata()->GetGridType(),"Layered") == 0));
	int numRefs = ds->getNumTransforms();
	if (isLayered){
		vizFeatureDlg->refinementCombo->setMaxCount(numRefs+1);
		vizFeatureDlg->refinementCombo->clear();
		for (i = 0; i<= numRefs; i++){
			vizFeatureDlg->refinementCombo->insertItem(QString::number(i));
		}
	}
	surfaceColor = vizWin->getSurfaceColor();
	surfaceRefinement = vizWin->getSurfaceRefinementLevel();
	vizFeatureDlg->surfaceColorButton->setPaletteBackgroundColor(surfaceColor);
	vizFeatureDlg->refinementCombo->setCurrentItem(surfaceRefinement);
	showSurface = (vizWin->surfaceRenderingEnabled() && isLayered);
	vizFeatureDlg->surfaceCheckbox->setChecked(showSurface);
	
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
	axisCoords[0] = vizFeatureDlg->axisXEdit->text().toFloat();
	axisCoords[1] = vizFeatureDlg->axisYEdit->text().toFloat();
	axisCoords[2] = vizFeatureDlg->axisZEdit->text().toFloat();
	
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
	showAxes = vizFeatureDlg->axisCheckbox->isChecked();
	showRegion = vizFeatureDlg->regionCheckbox->isChecked();
	showSubregion = vizFeatureDlg->subregionCheckbox->isChecked();

	regionFrameColor = vizFeatureDlg->regionFrameColorButton->paletteBackgroundColor();
	subregionFrameColor = vizFeatureDlg->subregionFrameColorButton->paletteBackgroundColor();
	backgroundColor = vizFeatureDlg->backgroundColorButton->paletteBackgroundColor();
	colorbarBackgroundColor = vizFeatureDlg->colorbarBackgroundButton->paletteBackgroundColor();

	surfaceColor = vizFeatureDlg->surfaceColorButton->paletteBackgroundColor();
	showSurface = vizFeatureDlg->surfaceCheckbox->isChecked();
	surfaceRefinement = vizFeatureDlg->refinementCombo->currentItem();
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
	vizWin->setColorbarLLCoord(0,colorbarLLCoords[0]);
	vizWin->setColorbarLLCoord(1,colorbarLLCoords[1]);
	vizWin->setColorbarURCoord(0,colorbarURCoords[0]);
	vizWin->setColorbarURCoord(1,colorbarURCoords[1]);
	vizWin->enableColorbar(showBar);
	vizWin->enableAxes(showAxes);
	vizWin->enableRegionFrame(showRegion);
	vizWin->enableSubregionFrame(showSubregion);
	vizWin->setColorbarNumTics(numColorbarTics);
	vizWin->setBackgroundColor(backgroundColor);
	vizWin->setColorbarBackgroundColor(colorbarBackgroundColor);
	vizWin->setRegionFrameColor(regionFrameColor);
	vizWin->setSubregionFrameColor(subregionFrameColor);
	
	vizWin->setSurfaceColor(surfaceColor);
	vizWin->enableSurfaceRendering(showSurface);
	vizWin->setSurfaceRefinementLevel(surfaceRefinement);
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

