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
#include "glwindow.h"


using namespace VAPoR;

//Create a new vizfeatureparams
VizFeatureParams::VizFeatureParams(){
	dialogChanged = false;
	vizFeatureDlg = 0;
	featureHolder = 0;
	currentComboIndex = -1;
	
	
	Session* currentSession = Session::getInstance();
	
	for (int i = 0; i< 3; i++) stretch[i] = currentSession->getStretch(i);

	DataStatus *ds;
	ds = DataStatus::getInstance();
	
	
}
//Clone a new vizfeatureparams
VizFeatureParams::VizFeatureParams(const VizFeatureParams& vfParams){
	dialogChanged = false;
	vizFeatureDlg = 0;
	featureHolder = 0;
	for (int i = 0; i< 3; i++) stretch[i] = vfParams.stretch[i];
	currentComboIndex = vfParams.currentComboIndex;
	vizName = vfParams.vizName;
	
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
	
	colorbarFontsize = vfParams.colorbarFontsize;
	colorBarLLX = vfParams.colorBarLLX;
	colorBarLLY = vfParams.colorBarLLY;
	colorBarEnabled = vfParams.colorBarEnabled;
	colorbarTitles = vfParams.colorbarTitles;
	colorbarSize[0] = vfParams.colorbarSize[0];
	colorbarSize[1] = vfParams.colorbarSize[1];
	numColorbarTics = vfParams.numColorbarTics;
	
	colorbarBackgroundColor = vfParams.colorbarBackgroundColor;
	colorbarDigits = vfParams.colorbarDigits;
	
	colorbarFontsize = vfParams.colorbarFontsize;
	
	useLatLon = vfParams.useLatLon;
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
	if (vizNum < 0 || 
		(!DataStatus::getInstance()->dataIsPresent3D() &&
		!DataStatus::getInstance()->dataIsPresent2D())){
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
	
	//Set up the renderer type lookup:
	rendererTypeLookup.clear();
	
	for (int i = 1; i<= Params::GetNumParamsClasses(); i++){
		RenderParams* p = dynamic_cast<RenderParams*>(Params::GetDefaultParams(i));
		if (!p) continue;
		if (!p->UsesMapperFunction()) continue;
		rendererTypeLookup.push_back(p->GetParamsBaseTypeId());
	}

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
	//latlon annotation is not supported with no map projection, or with polar stereo projection:
	if ((DataStatus::getInstance()->getProjectionString() == "") || (DataStatus::getInstance()->getProjectionString().find("+proj=stere") != string::npos))
	{
		vizFeatureDlg->latLonCheckbox->setEnabled(false);
		vizFeatureDlg->latLonCheckbox->setChecked(false);
	} 
	else {
		vizFeatureDlg->latLonCheckbox->setEnabled(true);
	}
	
	//Do connections.  
	connect(vizFeatureDlg->colorbarTable, SIGNAL(cellChanged(int, int)), this, SLOT(colorbarTableChanged(int, int)));
	connect(vizFeatureDlg->currentNameCombo, SIGNAL(activated(int)), this, SLOT(visualizerSelected(int)));
	
	connect (vizFeatureDlg->vizNameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisXEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisYEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->axisZEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarNumDigitsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarFontsizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarXSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarYSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->numTicsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->colorbarBackgroundButton, SIGNAL(clicked()), this, SLOT(selectColorbarBackgroundColor()));
	connect (vizFeatureDlg->axisCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	connect (vizFeatureDlg->spinAnimationCheckbox, SIGNAL(clicked()), this, SLOT(panelChanged()));
	
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
	connect (vizFeatureDlg->latLonCheckbox, SIGNAL(toggled(bool)), this, SLOT(toggleLatLon(bool)));
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

	int i;
	vizFeatureDlg->stretch0Edit->setText(QString::number(stretch[0]));
	vizFeatureDlg->stretch1Edit->setText(QString::number(stretch[1]));
	vizFeatureDlg->stretch2Edit->setText(QString::number(stretch[2]));

	Session* currentSession = Session::getInstance();
	bool enableStretch = !currentSession->sphericalTransform();
    vizFeatureDlg->stretch0Edit->setEnabled(enableStretch);
    vizFeatureDlg->stretch1Edit->setEnabled(enableStretch);
    vizFeatureDlg->stretch2Edit->setEnabled(enableStretch);
	
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
	useLatLon = vizWin->useLatLonAnnotation();
	vizFeatureDlg->latLonCheckbox->setChecked(useLatLon);
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
	// Insert a row in the table for each renderer type that has a TF
	vizFeatureDlg->colorbarTable->setRowCount(rendererTypeLookup.size());
	vizFeatureDlg->colorbarTable->setColumnCount(5);
	
    vizFeatureDlg->colorbarTable->verticalHeader()->hide();
	//setSelectionMode(QAbstractItemView::SingleSelection);
	//setSelectionBehavior(QAbstractItemView::SelectRows);
	//setFocusPolicy(Qt::ClickFocus);

	QStringList headerList = QStringList() <<"Renderer" <<"Enabled" <<"Lower-left X" << "Lower-left Y" << "Title";
    vizFeatureDlg->colorbarTable->setHorizontalHeaderLabels(headerList);
	vizFeatureDlg->colorbarTable->horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
	colorbarTitles = vizWin->getColorbarTitles();
	colorBarEnabled = vizWin->getColorbarEnabled();
	colorBarLLX = vizWin->getColorbarLLX();
	colorBarLLY = vizWin->getColorbarLLY();
	
	//Populate the table:
	for (int i = 0; i< rendererTypeLookup.size(); i++){
		RenderParams* p = dynamic_cast<RenderParams*>(Params::GetDefaultParams(rendererTypeLookup[i]));
		const string& s = p->getShortName();
		QString rendererName = QString(s.c_str());
		QTableWidgetItem* it0 = new QTableWidgetItem(rendererName);
		it0->setTextAlignment(Qt::AlignCenter);
		vizFeatureDlg->colorbarTable->setItem(i,0,it0);
		QTableWidgetItem* it1 = new QTableWidgetItem("");
		it1->setTextAlignment(Qt::AlignCenter);
		if (colorBarEnabled[i])it1->setCheckState(Qt::Checked);
			else it1->setCheckState(Qt::Unchecked);
		vizFeatureDlg->colorbarTable->setItem(i,1,it1);
		QTableWidgetItem* it2 = new QTableWidgetItem(QString::number(colorBarLLX[i]));
		it2->setTextAlignment(Qt::AlignCenter);
		vizFeatureDlg->colorbarTable->setItem(i,2,it2);
		QTableWidgetItem* it3 = new QTableWidgetItem(QString::number(colorBarLLY[i]));
		it3->setTextAlignment(Qt::AlignCenter);
		vizFeatureDlg->colorbarTable->setItem(i,3,it3);
		QTableWidgetItem* it4 = new QTableWidgetItem(colorbarTitles[i].c_str());
		it4->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
		it4->setSizeHint(QSize(130,0));
		vizFeatureDlg->colorbarTable->setItem(i,4,it4);
	}
	vizFeatureDlg->colorbarTable->resizeColumnsToContents();
	//setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	
	colorbarDigits = vizWin->getColorbarDigits();
	
	colorBarLLX = vizWin->getColorbarLLX();
	colorBarLLY = vizWin->getColorbarLLY();

	
	colorbarFontsize = vizWin->getColorbarFontsize();
	vizFeatureDlg->colorbarFontsizeEdit->setText(QString::number(colorbarFontsize));
	vizFeatureDlg->colorbarNumDigitsEdit->setText(QString::number(colorbarDigits));
	
	
	colorbarSize[0] = vizWin->getColorbarSize(0);
	colorbarSize[1] = vizWin->getColorbarSize(1);
	
	vizFeatureDlg->colorbarXSizeEdit->setText(QString::number(colorbarSize[0]));
	vizFeatureDlg->colorbarYSizeEdit->setText(QString::number(colorbarSize[1]));

	
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
	
	showAxisArrows = vizWin->axisArrowsAreEnabled();

	vizFeatureDlg->axisCheckbox->setChecked(showAxisArrows);

	enableSpin = GLWindow::spinAnimationEnabled();
	vizFeatureDlg->spinAnimationCheckbox->setChecked(enableSpin);
	
	colorbarBackgroundColor = vizWin->getColorbarBackgroundColor();
	tempColorbarBackgroundColor = colorbarBackgroundColor;
	QPalette pal2(vizFeatureDlg->colorbarBackgroundEdit->palette());
	pal2.setColor(QPalette::Base, colorbarBackgroundColor);
	vizFeatureDlg->colorbarBackgroundEdit->setPalette(pal2);


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

	stretch[0] = vizFeatureDlg->stretch0Edit->text().toFloat();
	stretch[1] = vizFeatureDlg->stretch1Edit->text().toFloat();
	stretch[2] = vizFeatureDlg->stretch2Edit->text().toFloat();
	
	if (vizFeatureDlg->vizNameEdit->text() != vizName){
		vizName = vizFeatureDlg->vizNameEdit->text();
	}
	axisArrowCoords[0] = vizFeatureDlg->axisXEdit->text().toFloat();
	axisArrowCoords[1] = vizFeatureDlg->axisYEdit->text().toFloat();
	axisArrowCoords[2] = vizFeatureDlg->axisZEdit->text().toFloat();
	axisOriginCoords[0] = vizFeatureDlg->axisOriginXEdit->text().toDouble();
	axisOriginCoords[1] = vizFeatureDlg->axisOriginYEdit->text().toDouble();
	axisOriginCoords[2] = vizFeatureDlg->axisOriginZEdit->text().toDouble();
	minTic[0] = vizFeatureDlg->xMinTicEdit->text().toDouble();
	minTic[1] = vizFeatureDlg->yMinTicEdit->text().toDouble();
	minTic[2] = vizFeatureDlg->zMinTicEdit->text().toDouble();
	maxTic[0] = vizFeatureDlg->xMaxTicEdit->text().toDouble();
	maxTic[1] = vizFeatureDlg->yMaxTicEdit->text().toDouble();
	maxTic[2] = vizFeatureDlg->zMaxTicEdit->text().toDouble();
	numTics[0] = vizFeatureDlg->xNumTicsEdit->text().toInt();
	numTics[1] = vizFeatureDlg->yNumTicsEdit->text().toInt();
	numTics[2] = vizFeatureDlg->zNumTicsEdit->text().toInt();
	ticLength[0] = vizFeatureDlg->xTicSizeEdit->text().toDouble();
	ticLength[1] = vizFeatureDlg->yTicSizeEdit->text().toDouble();
	ticLength[2] = vizFeatureDlg->zTicSizeEdit->text().toDouble();
	ticDir[0] = vizFeatureDlg->xTicOrientCombo->currentIndex()+1;
	ticDir[1] = vizFeatureDlg->yTicOrientCombo->currentIndex()*2;
	ticDir[2] = vizFeatureDlg->zTicOrientCombo->currentIndex();
	labelHeight = vizFeatureDlg->labelHeightEdit->text().toInt();
	labelDigits = vizFeatureDlg->labelDigitsEdit->text().toInt();
	ticWidth = vizFeatureDlg->ticWidthEdit->text().toFloat();
	useLatLon = vizFeatureDlg->latLonCheckbox->isChecked();

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
	
	
	colorbarDigits = vizFeatureDlg->colorbarNumDigitsEdit->text().toInt();
	colorbarFontsize = vizFeatureDlg->colorbarFontsizeEdit->text().toInt();
	
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
	colorbarSize[0] = wid;
	colorbarSize[1] = ht;
	
	numColorbarTics = vizFeatureDlg->numTicsEdit->text().toInt();
	if (numColorbarTics <2) numColorbarTics = 0;
	if (numColorbarTics > 50) numColorbarTics = 50;
	
	showAxisArrows = vizFeatureDlg->axisCheckbox->isChecked();

	enableSpin = vizFeatureDlg->spinAnimationCheckbox->isChecked();

	colorbarBackgroundColor = tempColorbarBackgroundColor;

	
	applyToViz(vizNum);
	
	//Save the new visualizer state in the history
	VizFeatureCommand::captureEnd(cmd, this);
	
	
}
//Apply these settings to specified visualizer, and to global state:
void VizFeatureParams::
applyToViz(int vizNum){
	
	int i;
	DataStatus* ds = DataStatus::getInstance();
	
	
	
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
		if (minStretch != 1.f) stretch[i] /= minStretch;
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
		int timestep = vizMgr->getActiveAnimationParams()->getCurrentTimestep();
		//Set the region dirty bit in every window:
		bool firstSharedVp = false;
		bool firstSharedAp = true;
		for (int j = 0; j< MAXVIZWINS; j++) {
			VizWin* win = vizMgr->getVizWin(j);
			if (!win) continue;
			
			//Only do each viewpoint params once
			ViewpointParams* vpp = vizMgr->getViewpointParams(j);
			AnimationParams* aParams = vizMgr->getAnimationParams(j);
			//Rescale the keyframes of animation params,
			//Just rescale keyframes of the first shared animation params
			if (!aParams->isLocal()){
				if (firstSharedAp){
					aParams->rescaleKeyframes(ratio);
					firstSharedAp = false;
				}
			} else aParams->rescaleKeyframes(ratio);

			if (!vpp->isLocal()) {
				if(firstSharedVp) continue;
				else firstSharedVp = true;
			}
			vpp->rescale(ratio, timestep);
			
			vpp->setCoordTrans();
			win->setValuesFromGui(vpp);
			vizMgr->resetViews(vpp);
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
		vizWin->setLatLonAnnotation(useLatLon);
	}
	vizWin->setAxisColor(axisAnnotationColor);
	vizWin->setTicWidth(ticWidth);
	vizWin->setLabelHeight(labelHeight);
	vizWin->setLabelDigits(labelDigits);
	
	vizWin->setColorbarDigits(colorbarDigits);
	vizWin->setColorbarTitles(colorbarTitles);
	vizWin->setColorbarEnabled(colorBarEnabled);
	vizWin->setColorbarLLX(colorBarLLX);
	vizWin->setColorbarLLY(colorBarLLY);
	
	vizWin->setColorbarFontsize(colorbarFontsize);
	
	vizWin->setColorbarSize(0,colorbarSize[0]);
	vizWin->setColorbarSize(1,colorbarSize[1]);
	
	
	vizWin->enableAxisArrows(showAxisArrows);
	vizWin->enableAxisAnnotation(showAxisAnnotation);

	GLWindow::setSpinAnimation(enableSpin);
	
	vizWin->setColorbarNumTics(numColorbarTics);
	vizWin->setColorbarBackgroundColor(colorbarBackgroundColor);

	vizWin->setTimeAnnotCoords(timeAnnotCoords);
	vizWin->setTimeAnnotColor(timeAnnotColor);
	vizWin->setTimeAnnotTextSize(timeAnnotTextSize);
	vizWin->setTimeAnnotType(timeAnnotType);
	vizWin->setTimeAnnotDirty();
	vizWin->setAxisLabelsDirty();
	
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
void VizFeatureParams::checkSurface(bool on){
	if (!on) return;
	MessageReporter::warningMsg("Note: Improved terrain mapping capabilities are available\n%s",
		"in the Image panel and in the 2D Data panel.");
}
void VizFeatureParams::
reloadShaders(){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	bool rc = vizMgr->reloadShaders();
	if (!rc) MessageReporter::errorMsg("Error reloading shaders\n%s",
		"Note that all applicable renderers must be disabled when reloading shaders.");

}
//When the latLon is toggled, change coords to/from lat-lon
void VizFeatureParams::
toggleLatLon(bool on){
	if (useLatLon == on) return; // no change
	if (on) { //Issue warning if using rotated lat lon
		if(DataStatus::getInstance()->getProjectionString().find("+proj=ob_tran") != string::npos) {
			MessageReporter::warningMsg("Note that Rotated Lat/Lon will not display axis annotation correctly when rotation is nonzero");
		}
	}
	GLWindow::ConvertAxes(on,ticDir,minTic,maxTic,axisOriginCoords,ticLength,minTic,maxTic,axisOriginCoords,ticLength);
	
	//Set new values into dialog
	vizFeatureDlg->axisOriginXEdit->setText(QString::number(axisOriginCoords[0]));
	vizFeatureDlg->axisOriginYEdit->setText(QString::number(axisOriginCoords[1]));
	vizFeatureDlg->axisOriginZEdit->setText(QString::number(axisOriginCoords[2]));
	vizFeatureDlg->xMinTicEdit->setText(QString::number(minTic[0]));
	vizFeatureDlg->yMinTicEdit->setText(QString::number(minTic[1]));
	vizFeatureDlg->zMinTicEdit->setText(QString::number(minTic[2]));
	vizFeatureDlg->xMaxTicEdit->setText(QString::number(maxTic[0]));
	vizFeatureDlg->yMaxTicEdit->setText(QString::number(maxTic[1]));
	vizFeatureDlg->zMaxTicEdit->setText(QString::number(maxTic[2]));
	vizFeatureDlg->xTicSizeEdit->setText(QString::number(ticLength[0]));
	vizFeatureDlg->yTicSizeEdit->setText(QString::number(ticLength[1]));
	vizFeatureDlg->zTicSizeEdit->setText(QString::number(ticLength[2]));
	useLatLon = on;
	dialogChanged = true;
}
void VizFeatureParams::colorbarTableChanged(int row, int col){
	assert(col > 0 && col < 5);
	QTableWidgetItem *thisItem = vizFeatureDlg->colorbarTable->item(row,col);
	dialogChanged = true;
	if (col == 1) { //enabled checkbox
		bool enabled = (thisItem->checkState() == Qt::Checked);
		colorBarEnabled[row] = enabled;
		return;
	}
	if (col == 2) { //LLX
		float val = thisItem->text().toFloat();
		colorBarLLX[row] = val;
		return;
	}
	if (col == 3) { //LLY
		float val = thisItem->text().toFloat();
		colorBarLLY[row] = val;
		return;
	}
	if (col == 4) { //Title
		string txt = thisItem->text().toStdString();
		colorbarTitles[row] = txt;
		return;
	}

}