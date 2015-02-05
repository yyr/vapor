//************************************************************************
//																		*
//		     Copyright (C)  2014										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolineeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the IsolineEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the isoline tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 4996 )
#endif
#include "glutil.h"	// Must be included first!!!
#include <algorithm>
#include <QScrollBar>
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <QFileDialog>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qtooltip.h>
#include "isolinerenderer.h"
#include "isovalueeditor.h"
#include "regionparams.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "qtthumbwheel.h"
#include "histo.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "params.h"
#include "contourTab.h"
#include <vapor/jpegapi.h>
#include <vapor/XmlNode.h>
#include "vapor/GetAppPath.h"
#include "vapor/errorcodes.h"
#include "tabmanager.h"
#include "isolineparams.h"
#include "isolinerenderer.h"
#include "isolineeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "vapor/ColorMapBase.h"
#include "vapor/ControlExecutive.h"
#include "manip.h"
#include "vizwinparams.h"
#include "isolineAppearance.h"
#include "isolineImage.h"
#include "isolineIsovals.h"
#include "isolineLayout.h"
#include "isolineBasics.h"

using namespace VAPoR;
const float IsolineEventRouter::thumbSpeedFactor = 0.0005f;  //rotates ~45 degrees at full thumbwheel width
const char* IsolineEventRouter::webHelpText[] = 
{
	"Contour Line (Isoline) overview",
	"Renderer control",
	"Data accuracy control",
	"Contour line basic settings",
	"Isovalue selection",
	"Contour line layout",
	"Contour line image settings",
	"Contour line appearance",
	"<>"
};
const char* IsolineEventRouter::webHelpURL[] =
{
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isolines",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/renderer-instances",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/refinement-and-lod-control",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isolines#Basic",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isolines#IsoSelection",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isolines#Layout",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isolines#Image",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/isolines#Appearance"
};

IsolineEventRouter::IsolineEventRouter(QWidget* parent): QWidget(parent), Ui_ContourTab(), EventRouter(){
	setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(IsolineParams::_isolineParamsTag);
	myWebHelpActions = makeWebHelpActions(webHelpText,webHelpURL);

	showLayout = false;
	
	QTabWidget* myTabWidget = new QTabWidget(this);
	myTabWidget->setTabPosition(QTabWidget::West);
	myBasics = new IsolineBasics(myTabWidget);
	myTabWidget->addTab(myBasics, "Basics");
	myAppearance = new IsolineAppearance(myTabWidget);
	myTabWidget->addTab(myAppearance, "Appearance");
	myLayout = new IsolineLayout(myTabWidget);
	myTabWidget->addTab(myLayout,"Layout");
	myIsovals = new IsolineIsovals(myTabWidget);
	myTabWidget->addTab(myIsovals,"Isovalues");
	myImage = new IsolineImage(myTabWidget);
	myTabWidget->addTab(myImage,"Image");
	tabHolderLayout->addWidget(myTabWidget);

	myIsovals->isoSelectionFrame->setOpacityMapping(true);
	myIsovals->isoSelectionFrame->setColorMapping(true);
	myIsovals->isoSelectionFrame->setIsoSlider(false);
	myIsovals->isoSelectionFrame->setIsolineSliders(true);
	fidelityButtons = 0;
	
	ignoreComboChanges = false;

	seedAttached = false;
	showAppearance = true;
	showLayout = false;
	showImage = true;
	editMode = true;
	lastXSizeSlider = 256;
	lastYSizeSlider = 256;
	lastXCenterSlider = 128;
	lastYCenterSlider = 128;
	lastZCenterSlider = 128;
	myIsovals->isoSelectionFrame->setIsolineSliders(true);
	
	for (int i = 0; i<3; i++)maxBoxSize[i] = 1.f;
	

#if defined(Darwin) && (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	_opacityMapShown = false;
	_texShown = false;
	myImage->isolineImageFrame->hide();
	myIsovals->isoSelectionFrame->hide();
#endif
	
}

IsolineEventRouter::~IsolineEventRouter(){

}
/**********************************************************
 * Whenever a new Isolinetab is created it must be hooked up here
 ************************************************************/
void
IsolineEventRouter::hookUpTab()
{
	//Nudge sliders by clicking on slider bar:
	connect (myLayout->xSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXSize(int)));
	connect (myLayout->xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXCenter(int)));
	connect (myLayout->ySizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYSize(int)));
	connect (myLayout->yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYCenter(int)));
	connect (myLayout->zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeZCenter(int)));
	connect (myLayout->xCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->yCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->zCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->thetaEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->phiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->psiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->xSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myLayout->ySizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myIsovals->minIsoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myIsovals->isoSpaceEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myIsovals->countIsoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myAppearance->isolineWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myImage->panelLineWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myAppearance->textSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myAppearance->densityEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myIsovals->histoScaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myIsovals->leftHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myIsovals->rightHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myAppearance->numDigitsEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (myBasics->fidelityDefaultButton, SIGNAL(clicked()), this, SLOT(guiSetFidelityDefault()));
	connect (myIsovals->fitDataButton, SIGNAL(clicked()), this, SLOT(guiFitToData()));
	connect (myIsovals->uniformButton, SIGNAL(clicked()),this,SLOT(guiSpaceIsovalues()));
	connect (myAppearance->singleColorButton, SIGNAL(clicked()),this, SLOT(guiSetSingleColor()));

	connect (myLayout->xCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->yCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->zCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->xSizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->ySizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->thetaEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->phiEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myLayout->psiEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myIsovals->minIsoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myIsovals->isoSpaceEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myIsovals->countIsoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myAppearance->isolineWidthEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myImage->panelLineWidthEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myAppearance->textSizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myAppearance->densityEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myIsovals->leftHistoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myIsovals->rightHistoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myIsovals->histoScaleEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (myAppearance->numDigitsEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));

	connect (myIsovals->loadButton, SIGNAL(clicked()), this, SLOT(isolineLoadTF()));
	connect (myIsovals->loadInstalledButton, SIGNAL(clicked()), this, SLOT(isolineLoadInstalledTF()));
	connect (myIsovals->saveButton, SIGNAL(clicked()), this, SLOT(isolineSaveTF()));
	connect (myImage->regionCenterButton, SIGNAL(clicked()), this, SLOT(isolineCenterRegion()));
	connect (myImage->viewCenterButton, SIGNAL(clicked()), this, SLOT(isolineCenterView()));
	connect (myImage->rakeCenterButton, SIGNAL(clicked()), this, SLOT(isolineCenterRake()));
	connect (myImage->isolineCenterButton, SIGNAL(clicked()), this, SLOT(guiCenterIsolines()));
	connect (myImage->addSeedButton, SIGNAL(clicked()), this, SLOT(isolineAddSeed()));
	connect (myLayout->axisAlignCombo, SIGNAL(activated(int)), this, SLOT(guiAxisAlign(int)));
	connect (myLayout->fitRegionButton, SIGNAL(clicked()), this, SLOT(guiFitRegion()));
	connect (myLayout->fitDomainButton, SIGNAL(clicked()), this, SLOT(guiFitDomain()));
	connect (myLayout->cropRegionButton, SIGNAL(clicked()), this, SLOT(guiCropToRegion()));
	connect (myLayout->cropDomainButton, SIGNAL(clicked()), this, SLOT(guiCropToDomain()));

	connect (myLayout->xThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateXWheel(int)));
	connect (myLayout->yThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateYWheel(int)));
	connect (myLayout->zThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateZWheel(int)));
	connect (myLayout->xThumbWheel, SIGNAL(wheelReleased(int)), this, SLOT(guiReleaseXWheel(int)));
	connect (myLayout->yThumbWheel, SIGNAL(wheelReleased(int)), this, SLOT(guiReleaseYWheel(int)));
	connect (myLayout->zThumbWheel, SIGNAL(wheelReleased(int)), this, SLOT(guiReleaseZWheel(int)));
	connect (myLayout->xThumbWheel, SIGNAL(wheelPressed()), this, SLOT(pressXWheel()));
	connect (myLayout->yThumbWheel, SIGNAL(wheelPressed()), this, SLOT(pressYWheel()));
	connect (myLayout->zThumbWheel, SIGNAL(wheelPressed()), this, SLOT(pressZWheel()));
	
	connect (myImage->attachSeedCheckbox,SIGNAL(toggled(bool)),this, SLOT(isolineAttachSeed(bool)));
	connect (myAppearance->singleColorCheckbox, SIGNAL(toggled(bool)),this, SLOT(guiSetUseSingleColor(bool)));
	connect (myAppearance->textCheckbox, SIGNAL(toggled(bool)), this, SLOT(guiEnableText(bool)));
	connect (myBasics->dimensionCombo,SIGNAL(activated(int)), this, SLOT(guiSetDimension(int)));
	connect (myBasics->refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (myBasics->lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (myLayout->rotate90Combo,SIGNAL(activated(int)), this, SLOT(guiRotate90(int)));
	connect (myBasics->variableCombo,SIGNAL(activated(int)), this, SLOT(guiChangeVariable(int)));
	connect (myLayout->xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineXCenter()));
	connect (myLayout->yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineYCenter()));
	connect (myLayout->zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineZCenter()));
	connect (myLayout->xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineXSize()));
	connect (myLayout->ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineYSize()));
	connect (myAppearance->densitySlider, SIGNAL(sliderReleased()), this, SLOT (guiSetIsolineDensity()));
	connect (myIsovals->copyToProbeButton, SIGNAL(clicked()), this, SLOT(copyToProbeOr2D()));

	
	connect (myImage->isolinePanelBackgroundColorButton, SIGNAL(clicked()), this, SLOT(guiSetPanelBackgroundColor()));
	connect(myIsovals->newHistoButton, SIGNAL(clicked()), this, SLOT(refreshHisto()));
	connect(myIsovals->editButton, SIGNAL(toggled(bool)), this, SLOT(setIsolineEditMode(bool)));
	connect(myIsovals->navigateButton, SIGNAL(toggled(bool)), this, SLOT(setIsolineNavigateMode(bool)));
	connect(myIsovals->editIsovaluesButton, SIGNAL(clicked()), this, SLOT(guiEditIsovalues()));

	// isoSelectionFrame controls:
	connect(myIsovals->editButton, SIGNAL(toggled(bool)), 
            myIsovals->isoSelectionFrame, SLOT(setEditMode(bool)));
	connect(myIsovals->alignButton, SIGNAL(clicked()), this, SLOT(guiSetAligned()));
	connect(myIsovals->alignButton, SIGNAL(clicked()),
            myIsovals->isoSelectionFrame, SLOT(fitToView()));
    connect(myIsovals->isoSelectionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeIsoSelection(QString)));
    connect(myIsovals->isoSelectionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeIsoSelection()));
}
//Insert values from params into tab panel
//
void IsolineEventRouter::updateTab(){
	//if(!MainForm::getTabManager()->isFrontTab(this)) return;
	MainForm::getInstance()->buildWebTabHelpMenu(myWebHelpActions);

	guiSetTextChanged(false);
	setIgnoreBoxSliderEvents(true);  //don't generate nudge events

	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr || dataMgr->GetVariableNames().size() == 0) return;
	
	IsolineParams* isolineParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	if (!isolineParams) return;
	Command::blockCapture();
	myIsovals->isoSelectionFrame->setIsolineSliders(isolineParams->GetIsovalues());
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	size_t timestep = (size_t)vizMgr->getActiveAnimationParams()->getCurrentTimestep();
	int winnum = vizMgr->getActiveViz();
	if (ds->getDataMgr() && fidelityDefaultChanged){
		setupFidelity(3, myBasics->fidelityLayout,myBasics->fidelityBox, isolineParams, false);
		connect(fidelityButtons,SIGNAL(buttonClicked(int)),this, SLOT(guiSetFidelity(int)));
		fidelityDefaultChanged = false;
	}
	if (ds->getDataMgr()) updateFidelity(myBasics->fidelityBox, isolineParams,myBasics->lodCombo,myBasics->refinementCombo);
	
	guiSetTextChanged(false);

	myAppearance->singleColorCheckbox->setChecked(isolineParams->UseSingleColor());
	QPalette pal(myAppearance->singleColorEdit->palette());
	vector<double> clr = isolineParams->GetSingleColor();
	QColor newColor = QColor((int)(clr[0]*255),(int)(clr[1]*255),(int)(clr[2]*255));
	pal.setColor(QPalette::Base, newColor);
	myAppearance->singleColorEdit->setPalette(pal);

	myAppearance->textCheckbox->setChecked(isolineParams->textEnabled());
	
	vector<long> viznums = VizWinParams::GetVisualizerNums();
	
	if (viznums.size() > 1) {
		copyCount.clear();
		//The first two entries should not be referenced
		copyCount.push_back(-1);
		copyCount.push_back(-1);
		for (int i = 0; i< viznums.size(); i++){
			int vizwin = viznums[i];
			if (winnum != vizwin){
				//Remember the viznum corresponding to a combo item:
				copyCount.push_back(vizwin);
			}
		}
	}

	
	QString strn;

	guiSetTextChanged(false);
	
	//Setup render window size:
	resetImageSize(isolineParams);
	//setup the size sliders 
	adjustBoxSize(isolineParams);
	if (!DataStatus::getInstance()->getDataMgr()) {
		Command::unblockCapture();
		return;
	}
	const vector<double>&userExts = ds->getDataMgr()->GetExtents(timestep);
	//And the center sliders/textboxes:
	double locExts[6],boxCenter[3];
	const double* fullSizes = ds->getFullSizes();
	isolineParams->GetBox()->GetLocalExtents(locExts);
	for (int i = 0; i<3; i++) boxCenter[i] = (locExts[i]+locExts[3+i])*0.5f;
	myLayout->xCenterSlider->setValue((int)(256.f*boxCenter[0]/fullSizes[0]));
	myLayout->yCenterSlider->setValue((int)(256.f*boxCenter[1]/fullSizes[1]));
	myLayout->zCenterSlider->setValue((int)(256.f*boxCenter[2]/fullSizes[2]));
	myLayout->xCenterEdit->setText(QString::number(userExts[0]+boxCenter[0]));
	myLayout->yCenterEdit->setText(QString::number(userExts[1]+boxCenter[1]));
	myLayout->zCenterEdit->setText(QString::number(userExts[2]+boxCenter[2]));

	//Calculate extents of the containing box
	double corners[8][3];
	isolineParams->GetBox()->calcLocalBoxCorners(corners, 0.f, -1);
	double dboxmin[3], dboxmax[3];
	size_t gridExts[6];
	for (int i = 0; i< 3; i++){
		float mincrd = corners[0][i];
		float maxcrd = mincrd;
		for (int j = 0; j<8;j++){
			if (mincrd > corners[j][i]) mincrd = corners[j][i];
			if (maxcrd < corners[j][i]) maxcrd = corners[j][i];
		}
		if (mincrd < 0.) mincrd = 0.;
		if (maxcrd > fullSizes[i]) maxcrd = fullSizes[i];
		dboxmin[i] = mincrd;
		dboxmax[i] = maxcrd;
	}
	//Now convert to user coordinates

	myLayout->minUserXLabel->setText(QString::number(userExts[0]+dboxmin[0]));
	myLayout->minUserYLabel->setText(QString::number(userExts[1]+dboxmin[1]));
	myLayout->minUserZLabel->setText(QString::number(userExts[2]+dboxmin[2]));
	myLayout->maxUserXLabel->setText(QString::number(userExts[0]+dboxmax[0]));
	myLayout->maxUserYLabel->setText(QString::number(userExts[1]+dboxmax[1]));
	myLayout->maxUserZLabel->setText(QString::number(userExts[2]+dboxmax[2]));

	//And convert these to grid coordinates:
	
	if (dataMgr && showLayout){
		DataStatus::getInstance()->mapBoxToVox(isolineParams->GetBox(),isolineParams->GetRefinementLevel(),isolineParams->GetCompressionLevel(),timestep,gridExts);
		
		myLayout->minGridXLabel->setText(QString::number(gridExts[0]));
		myLayout->minGridYLabel->setText(QString::number(gridExts[1]));
		myLayout->minGridZLabel->setText(QString::number(gridExts[2]));
		myLayout->maxGridXLabel->setText(QString::number(gridExts[3]));
		myLayout->maxGridYLabel->setText(QString::number(gridExts[4]));
		myLayout->maxGridZLabel->setText(QString::number(gridExts[5]));
	}
	vector<double>ivalues = isolineParams->GetIsovalues();
	//find min and max
	double isoMax = -1.e30, isoMin = 1.e30;
	for (int i = 0; i<ivalues.size(); i++){
		if (isoMax<ivalues[i]) isoMax = ivalues[i];
		if (isoMin>ivalues[i]) isoMin = ivalues[i];
	}
	double isoSpace = 0.;
	if (ivalues.size() > 1) 
		isoSpace = (isoMax - isoMin)/(double)(ivalues.size()-1);
	myAppearance->numDigitsEdit->setText(QString::number(isolineParams->GetNumDigits()));
	myIsovals->minIsoEdit->setText(QString::number(isoMin));
	myIsovals->isoSpaceEdit->setText(QString::number(isoSpace));
	myIsovals->countIsoEdit->setText(QString::number(ivalues.size()));
	float histoBounds[2];
	isolineParams->GetHistoBounds(histoBounds);
	myIsovals->leftHistoEdit->setText(QString::number(histoBounds[0]));
	myIsovals->rightHistoEdit->setText(QString::number(histoBounds[1]));
	myIsovals->histoScaleEdit->setText(QString::number(isolineParams->GetHistoStretch()));
	

	myAppearance->isolineWidthEdit->setText(QString::number(isolineParams->GetLineThickness()));
	myImage->panelLineWidthEdit->setText(QString::number(isolineParams->GetPanelLineThickness()));
	myAppearance->textSizeEdit->setText(QString::number(isolineParams->GetTextSize()));
	double den = isolineParams->GetTextDensity();
	myAppearance->densityEdit->setText(QString::number(isolineParams->GetTextDensity()));
	int sliderVal = den*256.;
	int sval = myAppearance->densitySlider->value();
	if (sval != sliderVal) myAppearance->densitySlider->setValue(sliderVal);

	//set color buttons
	QPalette pal2;
	const vector<double>&bColor = isolineParams->GetPanelBackgroundColor();
	QColor clr2 = QColor((int)(255*bColor[0]),(int)(255*bColor[1]),(int)(255*bColor[2]));
	pal2.setColor(QPalette::Base, clr2);
	myImage->isolinePanelBackgroundColorEdit->setPalette(pal2);
	
	//Provide latlon box extents if available:
	if (dataMgr->GetMapProjection().size() == 0){
		myLayout->minMaxLonLatFrame->hide();
	} else {
		double boxLatLon[4];
		
		boxLatLon[0] = locExts[0];
		boxLatLon[1] = locExts[1];
		boxLatLon[2] = locExts[3];
		boxLatLon[3] = locExts[4];
		
		if (DataStatus::convertLocalToLonLat((int)timestep, boxLatLon,2)){
			myLayout->minLonLabel->setText(QString::number(boxLatLon[0]));
			myLayout->minLatLabel->setText(QString::number(boxLatLon[1]));
			myLayout->maxLonLabel->setText(QString::number(boxLatLon[2]));
			myLayout->maxLatLabel->setText(QString::number(boxLatLon[3]));
			myLayout->minMaxLonLatFrame->show();
		} else {
			myLayout->minMaxLonLatFrame->hide();
		}
	}
	float angles[3];
    isolineParams->GetBox()->GetAngles(angles);
	myLayout->thetaEdit->setText(QString::number(angles[0],'f',1));
	myLayout->phiEdit->setText(QString::number(angles[1],'f',1));
	myLayout->psiEdit->setText(QString::number(angles[2],'f',1));
	mapCursor();
	
	const double* selectedPoint = isolineParams->getSelectedPointLocal();
	double selectedUserCoords[3];
	//selectedPoint is in local coordinates.  convert to user coords:
	for (int i = 0; i<3; i++)selectedUserCoords[i] = selectedPoint[i]+userExts[i];
	myImage->selectedXLabel->setText(QString::number(selectedUserCoords[0]));
	myImage->selectedYLabel->setText(QString::number(selectedUserCoords[1]));
	myImage->selectedZLabel->setText(QString::number(selectedUserCoords[2]));

	//Provide latlon coords if available:
	if (dataMgr->GetMapProjection().size() == 0){
		myImage->latLonFrame->hide();
	} else {
		double selectedLatLon[2];
		selectedLatLon[0] = selectedUserCoords[0];
		selectedLatLon[1] = selectedUserCoords[1];
		if (DataStatus::convertToLonLat(selectedLatLon,1)){
			myImage->selectedLonLabel->setText(QString::number(selectedLatLon[0]));
			myImage->selectedLatLabel->setText(QString::number(selectedLatLon[1]));
			myImage->latLonFrame->show();
		} else {
			myImage->latLonFrame->hide();
		}
	}
	myImage->attachSeedCheckbox->setChecked(seedAttached);
	int activeVarNum;
	float range[2];
	dataMgr->GetDataRange(timestep, isolineParams->GetVariableName().c_str(),range);
	myIsovals->minDataBound->setText(QString::number(range[0]));
	myIsovals->maxDataBound->setText(QString::number(range[1]));
	if (isolineParams->VariablesAre3D()){
		myIsovals->copyToProbeButton->setText("Copy to Probe");
		myIsovals->copyToProbeButton->setToolTip("Click to make the current active Probe display these contour lines as a color contour plot");
		activeVarNum = ds->getActiveVarNum3D(isolineParams->GetVariableName());
	}
	else {
		myIsovals->copyToProbeButton->setText("Copy to 2D");
		myIsovals->copyToProbeButton->setToolTip("Click to make the current active 2D Data display these contour lines as a color contour plot");
		activeVarNum = ds->getActiveVarNum2D(isolineParams->GetVariableName());
	}
	
	float val = 0.;
	if (isolineParams->IsEnabled())
		val = calcCurrentValue(isolineParams,selectedUserCoords);
	
	if (val == OUT_OF_BOUNDS)
		myImage->valueMagLabel->setText(QString(" "));
	else myImage->valueMagLabel->setText(QString::number(val));
	guiSetTextChanged(false);
	//Set the selection in the variable combo
	//Turn off combo message-listening
	ignoreComboChanges = true;
	myBasics->variableCombo->setCurrentIndex(activeVarNum);
	int dim = isolineParams->VariablesAre3D() ? 3 : 2;
	myBasics->dimensionCombo->setCurrentIndex(dim-2);
	ignoreComboChanges = false;
	
	if(isolineParams->GetIsoControl()){
		isolineParams->GetIsoControl()->setParams(isolineParams);
		myIsovals->isoSelectionFrame->setMapperFunction(isolineParams->GetIsoControl());
	}
	
    myIsovals->isoSelectionFrame->setVariableName(isolineParams->GetVariableName());
	updateHistoBounds(isolineParams);
	
	myIsovals->isoSelectionFrame->updateParams();
	
	myImage->isolineImageFrame->setParams(isolineParams);
	myImage->isolineImageFrame->update();
	
	
	
	adjustSize();
	
	vizMgr->getTabManager()->update();
	
	guiSetTextChanged(false);
	Command::unblockCapture();

	setIgnoreBoxSliderEvents(false);
	
}


void IsolineEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	IsolineParams* isolineParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	QString strn;
	if (isolineParams->VariablesAre3D()){
		float thetaVal = myLayout->thetaEdit->text().toFloat();
		while (thetaVal > 180.f) thetaVal -= 360.f;
		while (thetaVal < -180.f) thetaVal += 360.f;
		myLayout->thetaEdit->setText(QString::number(thetaVal,'f',1));
		float phiVal = myLayout->phiEdit->text().toFloat();
		while (phiVal > 180.f) phiVal -= 180.f;
		while (phiVal < 0.f) phiVal += 180.f;
		myLayout->phiEdit->setText(QString::number(phiVal,'f',1));
		float psiVal = myLayout->psiEdit->text().toFloat();
		while (psiVal > 180.f) psiVal -= 360.f;
		while (psiVal < -180.f) psiVal += 360.f;
		myLayout->psiEdit->setText(QString::number(psiVal,'f',1));
		vector<double>angles;
		angles.push_back(thetaVal);
		angles.push_back(phiVal);
		angles.push_back(psiVal);
		isolineParams->GetBox()->SetAngles(angles,isolineParams);
		
	}

	vector<double>ivalues;
	int numDigits = myAppearance->numDigitsEdit->text().toInt();
	if (numDigits < 2) numDigits = 2;
	if (numDigits > 12) numDigits = 12;
	if (numDigits != isolineParams->GetNumDigits()) isolineParams->SetNumDigits(numDigits);
	int numIsos = myIsovals->countIsoEdit->text().toInt();
	if (numIsos < 1) numIsos = 1;
	
	double isoSpace = (double)myIsovals->isoSpaceEdit->text().toDouble();
	if (isoSpace <0.) isoSpace = 0.;
	double minIso = (double)myIsovals->minIsoEdit->text().toDouble();
	double maxIso = minIso + isoSpace*(numIsos-1);
	if (maxIso < minIso) {
		maxIso = minIso;
		numIsos = 1;
	}
	double prevMinIso = 1.e30, prevMaxIso = -1.e30;
	const vector<double>& prevIsoVals = isolineParams->GetIsovalues(); 
	//Determine previous min/max of isovalues and previous spacing
	for (int i = 0; i<prevIsoVals.size(); i++){
		if (prevIsoVals[i]<prevMinIso) prevMinIso = prevIsoVals[i];
		if (prevIsoVals[i]>prevMaxIso) prevMaxIso = prevIsoVals[i];
	}
	double prevSpacing = prevIsoVals[prevIsoVals.size()-1] - prevIsoVals[0];
	if (prevIsoVals.size()>1) prevSpacing = prevSpacing/(double)(prevIsoVals.size() -1);
	float prevBnds[2];
	isolineParams->GetHistoBounds(prevBnds);
	//Determine if either the minIso, numIsos or isoSpace has changed.  
	bool isoIntervalChanged = false;
	if (numIsos != isolineParams->getNumIsovalues()) isoIntervalChanged = true;
	if (abs(prevMinIso - minIso) > 1.e-7) isoIntervalChanged = true;
	if (abs(isoSpace - prevSpacing) > 1.e-7) isoIntervalChanged = true;
	if (numIsos != isolineParams->getNumIsovalues()){
		//If the number was previously one, then also need to make sure the spacing is nonzero
		if (isolineParams->getNumIsovalues() == 1){
			if (isoSpace <= 0.) isoSpace = 0.1*(prevBnds[1]-prevBnds[0]);
		}
		isoIntervalChanged = true;
	}

	//If isoIntervalChanged is true, then just generate the isovalues based on the interval:
	if (isoIntervalChanged){

		ivalues.clear();
		for (int i = 0; i<numIsos; i++){
			ivalues.push_back(minIso + (double)i*isoSpace);
		}
		isolineParams->SetIsovalues(ivalues);
	} 

	//Determine if the histo interval has changed
	//If the isovalue interval changed and the histo interval did not change, then make the histo interval as large as
	//the isovalue interval.  If the histo interval is invalid, make it include the iso interval
	float bnds[2];
	bnds[0] = myIsovals->leftHistoEdit->text().toFloat();
	bnds[1] = myIsovals->rightHistoEdit->text().toFloat();

	if (bnds[0] >= bnds[1] ){ //fix invalid settings
		bnds[0] = minIso - 0.1*(maxIso-minIso);
		bnds[1] = maxIso + 0.1*(maxIso-minIso);
	}
	//make the histo interval include all the isovalues:
	if (isoIntervalChanged){
		if (bnds[0] > minIso) bnds[0] = minIso - 0.1*(maxIso-minIso);
		if (bnds[1] < maxIso) bnds[1] = maxIso + 0.1*(maxIso-minIso);
	}
	bool histoBoundsChanged = false;
	if (abs(prevBnds[0]-bnds[0]) >0.0005*(prevBnds[1]-prevBnds[0])) histoBoundsChanged = true;
	if (abs(prevBnds[1]-bnds[1]) >0.0005*(prevBnds[1]-prevBnds[0])) histoBoundsChanged = true;

	//Rescale the isovalues if just the histo bounds changed
	if(histoBoundsChanged) {
		isolineParams->SetHistoBounds(bnds);
		if (!isoIntervalChanged && (bnds[0] > minIso || bnds[1] < maxIso)){
			fitIsovalsToHisto(isolineParams);
		}
	}

	isolineParams->SetHistoStretch(myIsovals->histoScaleEdit->text().toDouble());
	
	double thickness = myAppearance->isolineWidthEdit->text().toDouble();
	if (thickness <= 0. || thickness > 100.) thickness = 1.0;
	isolineParams->SetLineThickness(thickness);
	thickness = myImage->panelLineWidthEdit->text().toDouble();
	if (thickness <= 0. || thickness > 100.) thickness = 1.0;
	isolineParams->SetPanelLineThickness(thickness);
	double textsize = myAppearance->textSizeEdit->text().toDouble();
	if (textsize <= 0. || textsize > 100.) textsize = 10.0;
	isolineParams->SetTextSize(textsize);
	double textDensity = myAppearance->densityEdit->text().toDouble();
	if (textDensity <= 0. || textDensity > 1.) textDensity = 0.0;
	
	isolineParams->SetTextDensity(textDensity);
	int sliderVal = textDensity*256.;
	int sval = myAppearance->densitySlider->value();
	if (sval != sliderVal) myAppearance->densitySlider->setValue(sliderVal);

	if (!DataStatus::getInstance()->getDataMgr()) return;

	size_t timestep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& userExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Set the isoline size based on current text box settings:
	float boxSize[3], boxexts[6],  boxCenter[3];
	boxSize[0] = myLayout->xSizeEdit->text().toFloat();
	boxSize[1] = myLayout->ySizeEdit->text().toFloat();
	boxSize[2] =0.;
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
	}
	//Convert text to local extents:
	boxCenter[0] = myLayout->xCenterEdit->text().toFloat()- userExts[0];
	boxCenter[1] = myLayout->yCenterEdit->text().toFloat()- userExts[1];
	boxCenter[2] = myLayout->zCenterEdit->text().toFloat()- userExts[2];
	const double* fullSizes = DataStatus::getInstance()->getFullSizes();
	for (int i = 0; i<3;i++){
		//Don't constrain the box to have center in the domain:
		boxexts[i] = boxCenter[i] - 0.5f*boxSize[i];
		boxexts[i+3] = boxCenter[i] + 0.5f*boxSize[i];
	}
	//Set the local box extents:
	isolineParams->GetBox()->SetLocalExtents(boxexts,isolineParams, -1);
	adjustBoxSize(isolineParams);
	//set the center sliders:
	myLayout->xCenterSlider->setValue((int)(256.f*boxCenter[0]/fullSizes[0]));
	myLayout->yCenterSlider->setValue((int)(256.f*boxCenter[1]/fullSizes[1]));
	myLayout->zCenterSlider->setValue((int)(256.f*boxCenter[2]/fullSizes[2]));
	resetImageSize(isolineParams);
	
	setIsolineDirty(isolineParams);
	
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(isolineParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	
	updateTab();
}


/*********************************************************************************
 * Slots associated with IsolineTab:
 *********************************************************************************/
void IsolineEventRouter::pressXWheel(){
	//Figure out the starting direction of z axis
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	renormalizedRotate = false;
	myLayout->xThumbWheel->setValue(0);
	if (stretch[1] == stretch[2]) return;
	double rotMatrix[9];
	double angles[3];
	pParams->GetBox()->GetAngles(angles);
	getRotationMatrix(angles[0]*M_PI/180., angles[1]*M_PI/180., angles[2]*M_PI/180., rotMatrix);
	//The starting rotation is obtained by projecting the z-axis of isoline
	//to the Y-Z plane.  The z-axis of isoline is just the last column of the
	//rotation matrix.
	float startRotateVector[3];
	startRotateVector[0] = 0;
	startRotateVector[1] = rotMatrix[5];
	startRotateVector[2] = rotMatrix[8];
	if (startRotateVector[2] == 0.f) return;
	
	//Calculate initial angle in viewer (stretched) space
	startRotateViewAngle = atan((startRotateVector[1]/startRotateVector[2])*(stretch[2]/stretch[1]));
	//To determine whether to use the principal value of the arctangent, need to know
	//whether the z axis of the isoline points in the positive or negative z direction
	if (rotMatrix[8] < 0.f) startRotateViewAngle += M_PI;
	startRotateActualAngle = atan(startRotateVector[1]/startRotateVector[2]);
	if (rotMatrix[8] < 0.f) startRotateActualAngle += M_PI;
	
	renormalizedRotate = true;
	
}
void IsolineEventRouter::pressYWheel(){
	//Figure out the starting direction of z axis
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	renormalizedRotate = false;
	myLayout->yThumbWheel->setValue(0);
	if (stretch[0] == stretch[2]) return;
	double rotMatrix[9];
	double angles[3];
	pParams->GetBox()->GetAngles(angles);
	getRotationMatrix(angles[0]*M_PI/180., angles[1]*M_PI/180., angles[2]*M_PI/180., rotMatrix);
	//The starting rotation is obtained by projecting the z-axis of isoline
	//to the X-Z plane.  The z-axis of isoline is just the last column of the
	//rotation matrix.
	float startRotateVector[3];
	startRotateVector[0] = rotMatrix[2];
	startRotateVector[1] = 0.f;
	startRotateVector[2] = rotMatrix[8];
	if (startRotateVector[2] == 0.f) return;
	
	//Calculate initial angle in viewer (stretched) space
	startRotateViewAngle = atan((startRotateVector[0]/startRotateVector[2])*(stretch[2]/stretch[0]));
	//To determine whether to use the principal value of the arctangent, need to know
	//whether the z axis of the isoline points in the positive or negative z direction
	if (rotMatrix[8] < 0.f) startRotateViewAngle += M_PI;
	
	startRotateActualAngle = atan(startRotateVector[0]/startRotateVector[2]);
	if (rotMatrix[8] < 0.f) startRotateActualAngle += M_PI;
	
	renormalizedRotate = true;

}
void IsolineEventRouter::pressZWheel(){
	//Figure out the starting direction of z axis
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	renormalizedRotate = false;
	myLayout->zThumbWheel->setValue(0);
	if (stretch[1] == stretch[0]) return;
	double rotMatrix[9];
	double angles[3];
	pParams->GetBox()->GetAngles(angles);
	getRotationMatrix(angles[0]*M_PI/180., angles[1]*M_PI/180., angles[2]*M_PI/180., rotMatrix);
	//The starting rotation is obtained by projecting the z-axis of isoline
	//to the X-Y plane.  The z-axis of isoline is just the last column of the
	//rotation matrix.
	float startRotateVector[3];
	startRotateVector[0] = rotMatrix[2];
	startRotateVector[1] = rotMatrix[5];
	startRotateVector[2] = 0.f;
	if (startRotateVector[1] == 0.f) return;
	
	//Calculate initial angle in viewer (stretched) space
	startRotateViewAngle = atan((startRotateVector[0]/startRotateVector[1])*(stretch[1]/stretch[0]));
	//To determine whether to use the principal value of the arctangent, need to know
	//whether the z axis of the isoline points in the positive or negative z direction
	if (rotMatrix[5] < 0.f) startRotateViewAngle += M_PI;
	startRotateActualAngle = atan(startRotateVector[0]/startRotateVector[1]);
	if (rotMatrix[5] < 0.f) startRotateActualAngle += M_PI;
	
	renormalizedRotate = true;

}
void IsolineEventRouter::
rotateXWheel(int val){
	//Check if we are in Isoline mode, if not do nothing:

	if(MouseModeParams::isolineMode != MouseModeParams::GetCurrentMouseMode()) return;

	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getVisualizer()->getManip(IsolineParams::_isolineParamsTag);
	
	if(!renormalizedRotate) manip->setTempRotation((float)val*thumbSpeedFactor, 0);
	else {
		
		double newrot = convertRotStretchedToActual(0,-startRotateViewAngle*180./M_PI+thumbSpeedFactor*(float)val);
		double angleChangeDg = newrot + startRotateActualAngle*180./M_PI;
		manip->setTempRotation((float)angleChangeDg, 0);
	}
	viz->updateGL();
	
	assert(!myLayout->xThumbWheel->isSliderDown());
	
}
void IsolineEventRouter::
rotateYWheel(int val){
	
	if(MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode) return;

	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getVisualizer()->getManip(IsolineParams::_isolineParamsTag);
	
	if(!renormalizedRotate) manip->setTempRotation(-(float)val*thumbSpeedFactor, 1);
	else {
		
		double newrot = convertRotStretchedToActual(1,-startRotateViewAngle*180./M_PI-thumbSpeedFactor*(float)val);
		double angleChangeDg = newrot + startRotateActualAngle*180./M_PI;
		manip->setTempRotation((float)angleChangeDg, 1);
	}
	viz->updateGL();
	
}
void IsolineEventRouter::
rotateZWheel(int val){
	
	if(MouseModeParams::isolineMode != MouseModeParams::GetCurrentMouseMode()) return;

	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getVisualizer()->getManip(IsolineParams::_isolineParamsTag);
	
	if(!renormalizedRotate) manip->setTempRotation((float)val*thumbSpeedFactor, 2);
	else {
		
		double newrot = convertRotStretchedToActual(2,-startRotateViewAngle*180./M_PI+thumbSpeedFactor*(float)val);
		double angleChangeDg = newrot + startRotateActualAngle*180./M_PI;
		manip->setTempRotation((float)angleChangeDg, 2);
	}
	viz->updateGL();
	
}
void IsolineEventRouter::
guiReleaseXWheel(int val){
	confirmText(false);
	
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	double finalRotate = (double)val*thumbSpeedFactor;
	if(renormalizedRotate) {
	
		float angleChangeDegView = (float)val*thumbSpeedFactor;
		double newrot2 = convertRotStretchedToActual(0,-startRotateViewAngle*180./M_PI+angleChangeDegView);
		double angleChangeDg = newrot2 + startRotateActualAngle*180./M_PI;
		finalRotate = angleChangeDg;
	}
	//apply rotation:
	pParams->GetBox()->rotateAndRenormalize(0, finalRotate,pParams);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getVisualizer()->getManip(IsolineParams::_isolineParamsTag);
	manip->setTempRotation(0.f, 0);
	//reset the thumbwheel

	updateTab();
	setIsolineDirty(pParams);

	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams, MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}
void IsolineEventRouter::
guiReleaseYWheel(int val){
	confirmText(false);
	
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	double finalRotate = -(double)val*thumbSpeedFactor;
	if(renormalizedRotate) {
	
		float angleChangeDegView = -(float)val*thumbSpeedFactor;
		double newrot2 = convertRotStretchedToActual(1,-startRotateViewAngle*180./M_PI+angleChangeDegView);
		double angleChangeDg = newrot2 + startRotateActualAngle*180./M_PI;
		finalRotate = angleChangeDg;
	}
	//apply rotation:
	pParams->GetBox()->rotateAndRenormalize(1, finalRotate, pParams);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getVisualizer()->getManip(IsolineParams::_isolineParamsTag);
	manip->setTempRotation(0.f, 1);

	updateTab();
	setIsolineDirty(pParams);

	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	
}
void IsolineEventRouter::
guiReleaseZWheel(int val){
	confirmText(false);
	
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	double finalRotate = (double)val*thumbSpeedFactor;
	if(renormalizedRotate) {
	
		float angleChangeDegView = (float)val*thumbSpeedFactor;
		double newrot2 = convertRotStretchedToActual(2,-startRotateViewAngle*180./M_PI+angleChangeDegView);
		double angleChangeDg = newrot2 + startRotateActualAngle*180./M_PI;
		finalRotate = angleChangeDg;
	}
	//apply rotation:
	pParams->GetBox()->rotateAndRenormalize(2, finalRotate, pParams);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getVisualizer()->getManip(IsolineParams::_isolineParamsTag);
	manip->setTempRotation(0.f, 2);

	updateTab();
	setIsolineDirty(pParams);

	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}

void IsolineEventRouter::guiRotate90(int selection){
	if (selection == 0) return;
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	int axis = (selection < 4) ? selection - 1 : selection -4;
	float angle = (selection < 4) ? 90.f : -90.f;
	//Renormalize and apply rotation:
	pParams->GetBox()->rotateAndRenormalize(axis, angle, pParams);
	myLayout->rotate90Combo->setCurrentIndex(0);
	updateTab();
	setIsolineDirty(pParams);
	
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);

}
void IsolineEventRouter::guiChangeInstance(int inst){
	performGuiChangeInstance(inst);
}
void IsolineEventRouter::guiNewInstance(){
	performGuiNewInstance();
}
void IsolineEventRouter::guiDeleteInstance(){
	performGuiDeleteInstance();
}


void IsolineEventRouter::
setIsolineTabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
void IsolineEventRouter::
isolineReturnPressed(void){
	//Find the appropriate parameter panel, make it update the visualization window
	confirmText(true);
}

void IsolineEventRouter::
setIsolineEnabled(bool val, int instance){


	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	//Make sure this is a change:
	if (pParams->IsEnabled() == val ) return;
	
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val, instance);
	
}

void IsolineEventRouter::
isolineCenterRegion(){
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	VizWinMgr::getInstance()->getRegionRouter()->setCenter(pParams->getSelectedPointLocal());
}
void IsolineEventRouter::
isolineCenterView(){
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPointLocal());
}
void IsolineEventRouter::
isolineCenterRake(){
	/*
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPointLocal());
	*/
}

void IsolineEventRouter::
isolineAddSeed(){
	/*
	if (!DataStatus::getInstance()->getDataMgr()) return;
	Point4 pt;
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	mapCursor();
	pt.set3Val(pParams->getSelectedPointLocal());
	AnimationParams* ap = (AnimationParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_animationParamsTag);
	int ts = ap->getCurrentTimestep();
	pt.set1Val(3,(float)ts);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	//Check that it's OK:
	FlowParams* fParams = VizWinMgr::getActiveFlowParams();
	if (!fParams->isEnabled()){
		MessageReporter::warningMsg("Seed is being added to a disabled flow");
	}
	if (fParams->rakeEnabled()){
		MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the target flow is using \nrake instead of seed list");
	}
	//Check that the point is in the current Region:
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	float boxMin[3], boxMax[3];
	rParams->getLocalBox(boxMin, boxMax,ts);
	if (pt.getVal(0) < boxMin[0] || pt.getVal(1) < boxMin[1] || pt.getVal(2) < boxMin[2] ||
		pt.getVal(0) > boxMax[0] || pt.getVal(1) > boxMax[1] || pt.getVal(2) > boxMax[2]) {
			MessageReporter::warningMsg("Seed will not result in a flow line because\n%s",
			"the seed point is outside the current region");
	}
	const vector<double>& usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)ts);
	for (int i = 0; i<3; i++) pt.set1Val(i, (float)( pt.getVal(i)+usrExts[i]));
	fRouter->guiAddSeed(pt);
	*/
}	

void IsolineEventRouter::
guiAxisAlign(int choice){
	if (choice == 0) return;
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	double angles[3];
	switch (choice) {
		case (1) : //nearest axis
			{ //align to nearest axis
				
				pParams->GetBox()->GetAngles(angles);
				
				//convert to closest number of quarter-turns
				int angleInt = (int)(((angles[0]+180.)/90.)+0.5) - 2;
				angles[0] = angleInt*90.;
				
				angleInt = (int)(((angles[2]+180.)/90.)+0.5) - 2;
				angles[2] = angleInt*90.;
				
				angleInt = (int)(((angles[1]+180)/90.)+0.5) - 2;
				angles[1] = angleInt*90.;
				pParams->GetBox()->SetAngles(angles,pParams);
				
			}
			break;
		case (2) : //+x
			angles[0] = 0.;
			angles[1]=-90.;
			angles[2] = 0.;
			pParams->GetBox()->SetAngles(angles, pParams);

			break;
		case (3) : //+y
			angles[0] = 90.;
			angles[1]=-90.;
			angles[2] = 180.;
			pParams->GetBox()->SetAngles(angles, pParams);
			break;
		case (4) : //+z
			angles[0] = 0.;
			angles[1]=180.;
			angles[2] = 0.;
			
			pParams->GetBox()->SetAngles(angles, pParams);
			break;
		case (5) : //-x
			angles[0] = 0.;
			angles[1]=90.;
			angles[2] = 0.;
			
			pParams->GetBox()->SetAngles(angles, pParams);
			break;
		case (6) : //-y
			angles[0] = 90.;
			angles[1]= 90.;
			angles[2] = 180.;
			
			pParams->GetBox()->SetAngles(angles, pParams);
			break;
		case (7) : //-z
			angles[0] = 0.;
			angles[1]=0.;
			angles[2] = 0.;
			
			pParams->GetBox()->SetAngles(angles, pParams);
			break;
		default:
			assert(0);
	}
	myLayout->axisAlignCombo->setCurrentIndex(0);
	//Force a redraw, update tab
	updateTab();
	setIsolineDirty(pParams);

	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);

}	
void IsolineEventRouter::
isolineAttachSeed(bool attach){
/*	if (attach) isolineAddSeed();
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_flowParamsTag);
	
	guiAttachSeed(attach, fParams);
	*/
}


void IsolineEventRouter::
setIsolineXCenter(){
	guiSetXCenter(
		myLayout->xCenterSlider->value());
}
void IsolineEventRouter::
setIsolineYCenter(){
	guiSetYCenter(
		myLayout->yCenterSlider->value());
}
void IsolineEventRouter::
setIsolineZCenter(){
	guiSetZCenter(
		myLayout->zCenterSlider->value());
}
void IsolineEventRouter::
setIsolineXSize(){
	guiSetXSize(
		myLayout->xSizeSlider->value());
}
void IsolineEventRouter::
setIsolineYSize(){
	guiSetYSize(
		myLayout->ySizeSlider->value());
}




//Fit to domain extents
void IsolineEventRouter::
guiFitDomain(){
	confirmText(false);
	//int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	double locextents[6];
	const double* sizes = DataStatus::getInstance()->getFullSizes();
	for (int i = 0; i<3; i++){
		locextents[i] = 0.;
		locextents[i+3] = sizes[i];
	}

	setIsolineToExtents(locextents,pParams);
	
	updateTab();
	setIsolineDirty(pParams);
	
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	
}




//Reinitialize Isoline tab settings, session has changed.
//Note that this is called after the globalIsolineParams are set up, but before
//any of the localIsolineParams are setup.
void IsolineEventRouter::
reinitTab(bool doOverride){
	DataStatus* ds = DataStatus::getInstance();
	setEnabled(true);
	setIgnoreBoxSliderEvents(false);
	myLayout->xThumbWheel->setRange(-100000,100000);
	myLayout->yThumbWheel->setRange(-100000,100000);
	myLayout->zThumbWheel->setRange(-100000,100000);
	myLayout->xThumbWheel->setValue(0);
	myLayout->yThumbWheel->setValue(0);
	myLayout->zThumbWheel->setValue(0);
	
	//Set the names in the variable listbox
	ignoreComboChanges = true;
	myBasics->variableCombo->clear();

	ignoreComboChanges = false;

	seedAttached = false;

	//Set up the refinement combo:
	const DataMgr* dataMgr = ds->getDataMgr();
	
	int numRefinements = dataMgr->GetNumTransforms();
	myBasics->refinementCombo->setMaxCount(numRefinements+1);
	myBasics->refinementCombo->clear();
	for (int i = 0; i<= numRefinements; i++){
		myBasics->refinementCombo->addItem(QString::number(i));
	}
	if (dataMgr){
		vector<size_t> cRatios = dataMgr->GetCRatios();
		myBasics->lodCombo->clear();
		myBasics->lodCombo->setMaxCount(cRatios.size());
		for (int i = 0; i<cRatios.size(); i++){
			QString s = QString::number(cRatios[i]);
			s += ":1";
			myBasics->lodCombo->addItem(s);
		}
	}
	if (currentHistogram) {
		delete currentHistogram;
		currentHistogram = 0;
	}
	//Do not record changes in the dParams...
	Command::blockCapture();
	IsolineParams* dParams = (IsolineParams*)ControlExec::GetDefaultParams(IsolineParams::_isolineParamsTag);
	//Set the names in the variable listbox
	ignoreComboChanges = true;
	myBasics->variableCombo->clear();
	vector<string>varnames;
	if (dParams->VariablesAre3D())
		varnames = dataMgr->GetVariables3D();
	else 
		varnames = dataMgr->GetVariables2DXY();

	for (int i = 0; i<varnames.size(); i++)
		myBasics->variableCombo->insertItem(i,varnames[i].c_str());
		
	ignoreComboChanges = false;
	setupFidelity(3, myBasics->fidelityLayout,myBasics->fidelityBox, dParams, doOverride);
	connect(fidelityButtons,SIGNAL(buttonClicked(int)),this, SLOT(guiSetFidelity(int)));
	
	updateTab();
	Command::unblockCapture();
}



void IsolineEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetParams(winnum,IsolineParams::_isolineParamsTag,instance);
	confirmText(false);
	if (value == pParams->IsEnabled()) return;
	string commandText;
	if (value) commandText = "enable rendering";
	else commandText = "disable rendering";
	Command* cmd = Command::CaptureStart(pParams, commandText.c_str(), Renderer::UndoRedo);
	pParams->SetEnabled(value);
	Command::CaptureEnd(cmd,pParams);
	
	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(pParams,!value, instance, false);
	
	//isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
	//and refresh the gui
	updateTab();
}


//Make the isolines center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void IsolineEventRouter::guiCenterIsolines(){
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	const double* selectedPoint = pParams->getSelectedPointLocal();
	
	double isoExts[6];
	pParams->GetBox()->GetLocalExtents(isoExts);
	for (int i = 0; i<3; i++)
		textToSlider(pParams,i,selectedPoint[i], isoExts[i+3]-isoExts[i]);
	
	updateTab();
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}
//Following method sets up (or releases) a connection to the Flow 
/*
void IsolineEventRouter::
guiAttachSeed(bool attach, FlowParams* fParams){
	confirmText(false);
	//Don't capture the attach/detach event.
	//This cannot be easily undone/redone because it requires maintaining the
	//state of both the flowparams and the isolineparams.
	//But we will capture the successive seed moves that occur while
	//the seed is attached.
	if (attach){ 
		attachedFlow = fParams;
		seedAttached = true;
		
	} else {
		seedAttached = false;
		attachedFlow = 0;
	} 
}
*/
//Respond to an change of variable.  set the appropriate bits
void IsolineEventRouter::
guiChangeVariable(int varnum){
	//Don't react if the combo is being reset programmatically:
	if (ignoreComboChanges) return;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	int activeVar = myBasics->variableCombo->currentIndex();
	
	if (pParams->VariablesAre3D()){
		const string varname = dataMgr->GetVariables3D()[activeVar];
		pParams->SetVariableName(varname);
	}
	else {
		const string varname = dataMgr->GetVariables2DXY()[activeVar];
		pParams->SetVariableName(varname);
	}
	
	updateHistoBounds(pParams);
	setEditorDirty(pParams);
	myIsovals->isoSelectionFrame->fitToView();
	
	//Need to update the selected point for the new variables
	updateTab();
	setIsolineDirty(pParams);	
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
}
void IsolineEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	setXCenter(pParams,sliderval);
	
	setIsolineDirty(pParams);	
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	
}
void IsolineEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	setYCenter(pParams,sliderval);
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	
}
void IsolineEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	setZCenter(pParams,sliderval);
	
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);

}
void IsolineEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	setXSize(pParams,sliderval);
	
	resetImageSize(pParams);
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);

}
void IsolineEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	setYSize(pParams,sliderval);

	resetImageSize(pParams);
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);

}

void IsolineEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	if (num == pParams->GetCompressionLevel()) return;
	
	pParams->SetCompressionLevel(num);
	myBasics->lodCombo->setCurrentIndex(num);
	pParams->SetIgnoreFidelity(true);
	QPalette pal = QPalette(myBasics->fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::gray);
	myBasics->fidelityBox->setPalette(pal);
	
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);	
}
void IsolineEventRouter::
guiSetNumRefinements(int n){
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	confirmText(false);
	int maxNumRefinements = 0;
	
	if (DataStatus::getInstance()) {
		maxNumRefinements = DataStatus::getInstance()->getNumTransforms();
		if (n > maxNumRefinements) {
			//MessageReporter::warningMsg("%s","Invalid number of Refinements \nfor current data");
			n = maxNumRefinements;
			myBasics->refinementCombo->setCurrentIndex(n);
		}
	} else if (n > maxNumRefinements) maxNumRefinements = n;
	pParams->SetRefinementLevel(n);
	pParams->SetIgnoreFidelity(true);
	QPalette pal = QPalette(myBasics->fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::gray);
	myBasics->fidelityBox->setPalette(pal);
	
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
}
	
//Set slider position, based on text change. 
//Requirement is that center is inside full domain.
//Should not change values in params unless the text is invalid.
//
void IsolineEventRouter::
textToSlider(IsolineParams* pParams, int coord, float newCenter, float newSize){
	setIgnoreBoxSliderEvents(true);
	pParams->setLocalBoxMin(coord, newCenter-0.5f*newSize);
	pParams->setLocalBoxMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	return;
	
}
//Set text when a slider changes.
//
void IsolineEventRouter::
sliderToText(IsolineParams* pParams, int coord, int slideCenter, int slideSize){
	
	const double* sizes = DataStatus::getInstance()->getFullSizes();
	float newCenter = ((float)slideCenter)*(sizes[coord])/256.f;
	float newSize = maxBoxSize[coord]*(float)slideSize/256.f;
	pParams->setLocalBoxMin(coord, newCenter-0.5f*newSize);
	pParams->setLocalBoxMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	//Set the text in the edit boxes
	mapCursor();
	const double* selectedPoint = pParams->getSelectedPointLocal();
	//Map to user coordinates
	if (!DataStatus::getInstance()->getDataMgr()) return;
	size_t timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>&userExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	newCenter += userExts[coord];
	float selectCoord = selectedPoint[coord] + userExts[coord];
	switch(coord) {
		case 0:
			myLayout->xSizeEdit->setText(QString::number(newSize,'g',7));
			myLayout->xCenterEdit->setText(QString::number(newCenter,'g',7));
			myImage->selectedXLabel->setText(QString::number(selectCoord));
			break;
		case 1:
			myLayout->ySizeEdit->setText(QString::number(newSize,'g',7));
			myLayout->yCenterEdit->setText(QString::number(newCenter,'g',7));
			myImage->selectedYLabel->setText(QString::number(selectCoord));
			break;
		case 2:
			myLayout->zCenterEdit->setText(QString::number(newCenter,'g',7));
			myImage->selectedZLabel->setText(QString::number(selectCoord));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	resetImageSize(pParams);
	update();
	//force a new render with new Isoline data
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	return;
	
}	


//Save undo/redo state when user grabs a isoline handle, or maybe a isoline face (later)
//
void IsolineEventRouter::
captureMouseDown(int){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}
//The Manip class will have already changed the box?..
void IsolineEventRouter::
captureMouseUp(){
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	resetImageSize(pParams);
	setIsolineDirty(pParams);
	//Update the tab if it's in front:
	if(MainForm::getTabManager()->isFrontTab(this)) {
		updateTab();
	}
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,true);
}
//When the center slider moves, set the IsolineMin and IsolineMax
void IsolineEventRouter::
setXCenter(IsolineParams* pParams,int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(pParams,0, sliderval, myLayout->xSizeSlider->value());
	setIsolineDirty(pParams);
}
void IsolineEventRouter::
setYCenter(IsolineParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval, myLayout->ySizeSlider->value());
	setIsolineDirty(pParams);
}
void IsolineEventRouter::
setZCenter(IsolineParams* pParams,int sliderval){
	sliderToText(pParams,2, sliderval, 0);
	setIsolineDirty(pParams);
}
//Min and Max are center -+ size/2
//size is regionsize*sliderval/256
void IsolineEventRouter::
setXSize(IsolineParams* pParams,int sliderval){
	sliderToText(pParams,0, myLayout->xCenterSlider->value(),sliderval);
	setIsolineDirty(pParams);
}
void IsolineEventRouter::
setYSize(IsolineParams* pParams,int sliderval){
	sliderToText(pParams,1, myLayout->yCenterSlider->value(),sliderval);
	setIsolineDirty(pParams);
}

//Save undo/redo state when user clicks cursor
//
void IsolineEventRouter::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	
	
}
void IsolineEventRouter::
guiEndCursorMove(){
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	//Update the selected point:
	//If we are connected to a seed, move it:
	/*
	if (seedIsAttached() && attachedFlow){
		mapCursor();
		float pt[3];
		for (int i = 0; i<3; i++) pt[i] = pParams->getSelectedPointLocal()[i];
		AnimationParams* ap = (AnimationParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_animationParamsTag);
		int ts = ap->getCurrentTimestep();
		const vector<double>& usrExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)ts);
		for (int i = 0; i<3; i++) pt[i]+=usrExts[i];
		VizWinMgr::getInstance()->getFlowRouter()->guiMoveLastSeed(pt);
	}
	*/
	
	//Update the tab, it's in front:
	updateTab();
	
	VizWinMgr::getInstance()->forceRender(pParams);
	
}
//calculate the variable at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not in isoline
//


float IsolineEventRouter::
calcCurrentValue(IsolineParams* pParams, const double point[3] ){
	double exts[6];
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds ) return 0.f; 
	DataMgr* dataMgr =	ds->getDataMgr();
	if (!dataMgr) return 0.f;
	if (!pParams->IsEnabled()) return 0.f;
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	if (pParams->doBypass(timeStep)) return OUT_OF_BOUNDS;
	
	for (int i = 0; i<6; i++){
		exts[i%3] = point[i];
	}
	vector<string> varname;
	varname.push_back(pParams->GetVariableName());
	RegularGrid* grid;
	//Get the data dimensions (at current resolution):
	
	int numRefinements = pParams->GetRefinementLevel();
	int lod = pParams->GetCompressionLevel();
	int rc = Renderer::getGrids(timeStep, varname, exts, &numRefinements, &lod, &grid);
	
	if (!rc) return OUT_OF_BOUNDS;
	float varVal = (grid)->GetValue(point[0],point[1],point[2]);
	
	delete grid;
	return varVal;
}



void IsolineEventRouter::guiNudgeXSize(int val) {
	
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	
	//ignore if change is not 1 
	if(abs(val - lastXSizeSlider) != 1) {
		lastXSizeSlider = val;
		return;
	}
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 0);
	float pmin = pParams->getLocalBoxMin(0);
	float pmax = pParams->getLocalBoxMax(0);
	float maxExtent = ds->getFullSizes()[0];
	float minExtent = 0.;
	float newSize = pmax - pmin;
	if (val > lastXSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastXSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setLocalBoxMin(0, pmin-voxelSize);
			pParams->setLocalBoxMax(0, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastXSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setLocalBoxMin(0, pmin+voxelSize);
			pParams->setLocalBoxMax(0, pmax-voxelSize);
			newSize = newSize - 2.*voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*newSize/(maxExtent-minExtent) +0.5f);
	if(lastXSizeSlider != newSliderPos){
		lastXSizeSlider = newSliderPos;
		myLayout->xSizeSlider->setValue(newSliderPos);
	}
	updateTab();
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}
void IsolineEventRouter::guiNudgeXCenter(int val) {

	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastXCenterSlider) != 1) {
		lastXCenterSlider = val;
		return;
	}
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 0);
	float pmin = pParams->getLocalBoxMin(0);
	float pmax = pParams->getLocalBoxMax(0);
	float maxExtent = ds->getFullSizes()[0];
	float minExtent = 0.;
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastXCenterSlider){//move by 1 voxel, but don't move past end
		lastXCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setLocalBoxMin(0, pmin+voxelSize);
			pParams->setLocalBoxMax(0, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastXCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setLocalBoxMin(0, pmin-voxelSize);
			pParams->setLocalBoxMax(0, pmax-voxelSize);
			newCenter = (pmin+pmax)*0.5f - voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*(newCenter - minExtent)/(maxExtent-minExtent) +0.5f);
	if(lastXCenterSlider != newSliderPos){
		lastXCenterSlider = newSliderPos;
		myLayout->xCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}
void IsolineEventRouter::guiNudgeYCenter(int val) {
	
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYCenterSlider) != 1) {
		lastYCenterSlider = val;
		return;
	}
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 1);
	float pmin = pParams->getLocalBoxMin(1);
	float pmax = pParams->getLocalBoxMax(1);
	float maxExtent = ds->getFullSizes()[1];
	float minExtent = 0.;
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastYCenterSlider){//move by 1 voxel, but don't move past end
		lastYCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setLocalBoxMin(1, pmin+voxelSize);
			pParams->setLocalBoxMax(1, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastYCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setLocalBoxMin(1, pmin-voxelSize);
			pParams->setLocalBoxMax(1, pmax-voxelSize);
			newCenter = (pmin+pmax)*0.5f - voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*(newCenter - minExtent)/(maxExtent-minExtent) +0.5f);
	if(lastYCenterSlider != newSliderPos){
		lastYCenterSlider = newSliderPos;
		myLayout->yCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,true);
}
void IsolineEventRouter::guiNudgeZCenter(int val) {
	
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastZCenterSlider) != 1) {
		lastZCenterSlider = val;
		return;
	}
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 2);
	float pmin = pParams->getLocalBoxMin(2);
	float pmax = pParams->getLocalBoxMax(2);
	float maxExtent = ds->getFullSizes()[2];
	float minExtent = 0.;
	float newCenter = (pmin+pmax)*0.5f;
	if (val > lastZCenterSlider){//move by 1 voxel, but don't move past end
		lastZCenterSlider++;
		if (pmax+voxelSize <= maxExtent){ 
			pParams->setLocalBoxMin(2, pmin+voxelSize);
			pParams->setLocalBoxMax(2, pmax+voxelSize);
			newCenter = (pmin+pmax)*0.5f + voxelSize;
		}
	} else {
		lastZCenterSlider--;
		if (pmin-voxelSize >= minExtent) {//slide 1 voxel down:
			pParams->setLocalBoxMin(2, pmin-voxelSize);
			pParams->setLocalBoxMax(2, pmax-voxelSize);
			newCenter = (pmin+pmax)*0.5f - voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*(newCenter - minExtent)/(maxExtent-minExtent) +0.5f);
	if(lastZCenterSlider != newSliderPos){
		lastZCenterSlider = newSliderPos;
		myLayout->zCenterSlider->setValue(newSliderPos);
	}
	updateTab();

	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}

void IsolineEventRouter::guiNudgeYSize(int val) {
	
	if (ignoreBoxSliderEvents) return;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	//ignore if change is not 1 
	if(abs(val - lastYSizeSlider) != 1) {
		lastYSizeSlider = val;
		return;
	}
	confirmText(false);
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	
	//See if the change was an increase or decrease:
	float voxelSize = ds->getVoxelSize(pParams->GetRefinementLevel(), 1);
	float pmin = pParams->getLocalBoxMin(1);
	float pmax = pParams->getLocalBoxMax(1);
	float maxExtent = ds->getFullSizes()[1];
	float minExtent = 0.;
	float newSize = pmax - pmin;
	if (val > lastYSizeSlider){//increase size by 1 voxel on each end, but no bigger than region:
		lastYSizeSlider++;
		if (pmax-pmin+2.f*voxelSize <= (maxExtent - minExtent)){ 
			pParams->setLocalBoxMin(1, pmin-voxelSize);
			pParams->setLocalBoxMax(1, pmax+voxelSize);
			newSize = newSize + 2.*voxelSize;
		}
	} else {
		lastYSizeSlider--;
		if ((pmax - pmin) >= 2.f*voxelSize) {//shrink by 1 voxel on each side:
			pParams->setLocalBoxMin(1, pmin+voxelSize);
			pParams->setLocalBoxMax(1, pmax-voxelSize);
			newSize = newSize - 2.*voxelSize;
		}
	}
	//Determine where the slider really should be:
	int newSliderPos = (int)(256.*newSize/(maxExtent-minExtent) +0.5f);
	if(lastYSizeSlider != newSliderPos){
		lastYSizeSlider = newSliderPos;
		myLayout->ySizeSlider->setValue(newSliderPos);
	}
	updateTab();
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}

//The following adjusts the sliders associated with box size.
//Each slider range is the maximum of 
//(1) the domain size in the current direction
//(2) the current value of domain size.
void IsolineEventRouter::
adjustBoxSize(IsolineParams* pParams){
	
	double extents[6];
	//Don't do anything if we haven't read the data yet:
	if (!DataStatus::getInstance()->getDataMgr()) return;
	pParams->GetBox()->GetLocalExtents(extents, -1);
	double rotMatrix[9];
	double angles[3];
	pParams->GetBox()->GetAngles(angles);
	getRotationMatrix(angles[0]*M_PI/180., angles[1]*M_PI/180., angles[2]*M_PI/180., rotMatrix);
	//Apply rotation matrix inverted to full domain size
	
	const double* extentSize = DataStatus::getInstance()->getFullSizes();
	
	//Determine the size of the domain in the direction associated with each
	//axis of the isoline.  To do this, find a unit vector in that direction.
	//The domain size in that direction is the dot product of that vector
	//with the vector that has components the domain sizes in each dimension.


	float domSize[3];
	
	for (int i = 0; i<3; i++){
		//create unit vec along axis:
		double axis[3];
		for (int j = 0; j<3; j++) axis[j] = 0.;
		axis[i] = 1.f;
		double boxDir[3];
		vtransform3(axis, rotMatrix, boxDir);
		//Make each component positive...
		for (int j = 0; j< 3; j++) { boxDir[j] = abs(boxDir[j]);}
		//normalize this direction vector:
		float vlen = vlength(boxDir);
		if (vlen > 0.f) {
			vnormal(boxDir);
			domSize[i] = vdot(boxDir,extentSize);
		} else {
			domSize[i] = 0.f;
		}
		maxBoxSize[i] = domSize[i];
		if (maxBoxSize[i] < (pParams->getLocalBoxMax(i)-pParams->getLocalBoxMin(i))){
			maxBoxSize[i] = (pParams->getLocalBoxMax(i)-pParams->getLocalBoxMin(i));
		}
		if (maxBoxSize[i] <= 0.f) maxBoxSize[i] = 1.f;
	}
	
	//Set the size sliders appropriately:
	myLayout->xSizeEdit->setText(QString::number(extents[3]-extents[0]));
	myLayout->ySizeEdit->setText(QString::number(extents[4]-extents[1]));
	
	//Cancel any response to text events generated in this method, to prevent
	//the sliders from triggering text change
	//
	guiSetTextChanged(false);
	myLayout->xSizeSlider->setValue((int)(256.f*(extents[3]-extents[0])/(maxBoxSize[0])));
	myLayout->ySizeSlider->setValue((int)(256.f*(extents[4]-extents[1])/(maxBoxSize[1])));
	
}

// Map the cursor coords into local user coord space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void IsolineEventRouter::mapCursor(){
	//Get the transform matrix:
	double transformMatrix[12];
	double isolineCoord[3];
	double selectPoint[3];
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	pParams->GetBox()->buildLocalCoordTransform(transformMatrix, 0.f, -1);
	//The cursor sits in the z=0 plane of the isoline box coord system.
	//x is reversed because we are looking from the opposite direction (?)
	const vector<double>& cursorCoords = pParams->GetCursorCoords();
	
	isolineCoord[0] = -cursorCoords[0];
	isolineCoord[1] = cursorCoords[1];
	isolineCoord[2] = 0.f;
	
	vtransform(isolineCoord, transformMatrix, selectPoint);
	pParams->setSelectedPointLocal(selectPoint);
}

void IsolineEventRouter::
guiCropToRegion(){
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	double extents[6];
	rParams->GetBox()->GetLocalExtents(extents,timestep);

	if (pParams->GetBox()->cropToBox(extents,pParams)){
		updateTab();
		setIsolineDirty(pParams);
		
		myImage->isolineImageFrame->update();
		VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	} else {
		//MessageReporter::warningMsg(" Contour line extents cannot be cropped to region, insufficient overlap");
		
	}
}
void IsolineEventRouter::
guiCropToDomain(){
	confirmText(false);
	
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	const double *sizes = DataStatus::getInstance()->getFullSizes();
	double extents[6];
	for (int i = 0; i<3; i++){
		extents[i] = 0.;
		extents[i+3]=sizes[i];
	}
	if (pParams->GetBox()->cropToBox(extents,pParams)){
		updateTab();
		setIsolineDirty(pParams);
		myImage->isolineImageFrame->update();
		VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	} else {
	//	MessageReporter::warningMsg(" Contour line extents cannot be cropped to domain, insufficient overlap");
		
	}
}
//Fill to domain extents
void IsolineEventRouter::
guiFitRegion(){
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);

	double extents[6];
	rParams->GetBox()->GetLocalExtents(extents,timestep);
	setIsolineToExtents(extents,pParams);
	updateTab();
	setIsolineDirty(pParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	
}
void IsolineEventRouter::
setIsolineToExtents(const double extents[6], IsolineParams* pParams){
	
	//First try to fit to extents.  If we fail, then move isoline to fit 
	bool success = pParams->GetBox()->fitToBox(extents,pParams);
	if (success) return;

	//Move the isoline so that it is centered in the extents:
	double pExts[6];
	pParams->GetBox()->GetLocalExtents(pExts);
	
	for (int i = 0; i<3; i++){
		double psize = pExts[i+3]-pExts[i];
		pExts[i] = 0.5*(extents[i]+extents[i+3] - psize);
		pExts[i+3] = 0.5*(extents[i]+extents[i+3] + psize);
	}
	pParams->GetBox()->SetLocalExtents(pExts,pParams, -1);
	success = pParams->GetBox()->fitToBox(extents,pParams);
	assert(success);
	
	return;
}
//Angle conversions (in degrees)
double IsolineEventRouter::convertRotStretchedToActual(int axis, double angle){
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	double retval = 0.0;
	switch (axis){
		case 0:
			retval =  (180./M_PI)*atan((stretch[1]/stretch[2])*tan(angle*M_PI/180.f));
			break;
		case 1:
			retval = (180./M_PI)*atan((stretch[0]/stretch[2])*tan(angle*M_PI/180.f));
			break;
		case 2:
			retval =  (180./M_PI)*atan((stretch[0]/stretch[1])*tan(angle*M_PI/180.f));
			break;
	}
	if (abs(angle) > 90.) retval += 180.;
	return retval;
}


//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widgets 

#ifdef Darwin
void IsolineEventRouter::paintEvent(QPaintEvent* ev){
	
		
		//First show the texture frame, 
		//Other order doesn't work.
		if(!_texShown ){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
			QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
			sArea->ensureWidgetVisible(isolineFrameHolder);
			_texShown = true;
#endif
			myImage->isolineImageFrame->show();
			myImage->isolineImageFrame->updateGeometry();
			QWidget::paintEvent(ev);
			return;
		} 
		QWidget::paintEvent(ev);
		return;
}

#endif
QSize IsolineEventRouter::sizeHint() const {
	IsolineParams* pParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	if (!pParams) return QSize(460,1500);
	int vertsize = 720;//basic panel plus instance panel 
	//add showAppearance button, showLayout button, showLayout button, frames
	vertsize += 150;
	if (showLayout) {
		vertsize += 467;
		//no lat long
	}
	if (showImage){
		vertsize += 567;
	}
	if (showAppearance) vertsize += 200;  //Add in appearance panel 
	//Mac and Linux have gui elements fatter than windows by about 10%
#ifndef WIN32
	vertsize = (int)(1.1*vertsize);
#endif

	return QSize(460,vertsize);
}
//Occurs when user clicks a fidelity radio button
void IsolineEventRouter::guiSetFidelity(int buttonID){
	// Recalculate LOD and refinement based on current slider value and/or current text value
	//.  If they don't change, then don't 
	// generate an event.
	confirmText(false);
	IsolineParams* dParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	int newLOD = fidelityLODs[buttonID];
	int newRef = fidelityRefinements[buttonID];
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	if (lodSet == newLOD && refSet == newRef) return;
	int fidelity = buttonID;
	

	dParams->SetCompressionLevel(newLOD);
	dParams->SetRefinementLevel(newRef);
	dParams->SetFidelityLevel(fidelity);
	dParams->SetIgnoreFidelity(false);
	QPalette pal = QPalette(myBasics->fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::black);
	myBasics->fidelityBox->setPalette(pal);
	//change values of LOD and refinement combos using setCurrentIndex().
	myBasics->lodCombo->setCurrentIndex(newLOD);
	myBasics->refinementCombo->setCurrentIndex(newRef);

	setIsolineDirty(dParams);
	myImage->isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(dParams);	
}

//User clicks on SetDefault button, need to make current fidelity settings the default.
void IsolineEventRouter::guiSetFidelityDefault(){
	/*
	//Check current values of LOD and refinement and their combos.
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	confirmText(false);
	IsolineParams* dParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	UserPreferences *prePrefs = UserPreferences::getInstance();
	PreferencesCommand* pcommand = PreferencesCommand::captureStart(prePrefs, "Set Fidelity Default Preference");

	setFidelityDefault(3,dParams);
	
	UserPreferences *postPrefs = UserPreferences::getInstance();
	PreferencesCommand::captureEnd(pcommand,postPrefs);
	delete prePrefs;
	delete postPrefs;
	updateTab();
	*/
}
void IsolineEventRouter::resetImageSize(IsolineParams* iParams){
	//setup the texture:
	float voxDims[2];
	iParams->GetBox()->getRotatedVoxelExtents(voxDims,iParams->GetRefinementLevel());
	myImage->isolineImageFrame->setImageSize(voxDims[0],voxDims[1]);
}
/*
 * Respond to user clicking a color button
 */


void IsolineEventRouter::guiSetPanelBackgroundColor(){
	QPalette pal(myImage->isolinePanelBackgroundColorEdit->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	pal.setColor(QPalette::Base, newColor);
	myImage->isolinePanelBackgroundColorEdit->setPalette(pal);
	//Set parameter value of the appropriate parameter set:
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	float clr[3];
	clr[0]=(newColor.red()/256.);
	clr[1]=(newColor.green()/256.);
	clr[2]=(newColor.blue()/256.);
	iParams->SetPanelBackgroundColor(clr);

	updateTab();
}

void IsolineEventRouter::guiSetDimension(int dim){
	
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	int curdim = iParams->VariablesAre3D() ? 1 : 0 ;
	if (curdim == dim) return;
	//Don't allow setting of dimension of there aren't any variables in the proposed dimension:
	if (dim == 1 && DataStatus::getInstance()->getNumActiveVariables3D()==0){
		myBasics->dimensionCombo->setCurrentIndex(0);
		return;
	}
	if (dim == 0 && DataStatus::getInstance()->getNumActiveVariables2D()==0){
		myBasics->dimensionCombo->setCurrentIndex(1);
		return;
	}
	
	//the combo is either 0 or 1 for dimension 2 or 3.
	iParams->SetVariables3D(dim == 1);
	//Put the appropriate variable names into the combo
	//Set the names in the variable listbox.
	//Set the current variable name to be the first variable
	ignoreComboChanges = true;
	myBasics->variableCombo->clear();
	vector<string> varnames;
	if (dim == 1){ //dim = 1 for 3D vars
		varnames = DataStatus::getInstance()->getDataMgr()->GetVariables3D();
		for (int i = 0; i< varnames.size(); i++){
			string s = varnames[i];
			if (i == 0) iParams->SetVariableName(s);
			const QString& text = QString(s.c_str());
			myBasics->variableCombo->insertItem(i,text);
		}
		myIsovals->copyToProbeButton->setText("Copy to Probe");
		myIsovals->copyToProbeButton->setToolTip("Click to make the current active Probe display these contours as a color contour plot");
	} else {
		varnames = DataStatus::getInstance()->getDataMgr()->GetVariables2DXY();
		for (int i = 0; i< varnames.size(); i++){
			string s = varnames[i];
			if (i == 0) iParams->SetVariableName(s);
			const QString& text = QString(s.c_str());
			myBasics->variableCombo->insertItem(i,text);
		}
		
		myIsovals->copyToProbeButton->setText("Copy to 2D");
		myIsovals->copyToProbeButton->setToolTip("Click to make the current active 2D Data display these contours as a color contour plot");
	}
	if (showLayout) {
		if (dim == 1) myLayout->orientationFrame->show();
		else myLayout->orientationFrame->hide();
	}
	ignoreComboChanges = false;

	setIsolineDirty(iParams);

}
void IsolineEventRouter::invalidateRenderer(IsolineParams* iParams)
{
	IsolineRenderer* iRender = (IsolineRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getVisualizer()->getRenderer(iParams);
	if (iRender) iRender->invalidateLineCache();
}

void IsolineEventRouter::
setIsolineEditMode(bool mode){
	myIsovals->navigateButton->setChecked(!mode);
	editMode = mode;
}
void IsolineEventRouter::
setIsolineNavigateMode(bool mode){
	myIsovals->editButton->setChecked(!mode);
	editMode = !mode;
}
void IsolineEventRouter::
refreshHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	if (iParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	DataMgr* dataManager = DataStatus::getInstance()->getDataMgr();
	if (dataManager) {
		refreshHistogram(iParams);
	}
	setEditorDirty(iParams);
}
void IsolineEventRouter::guiSetAligned(){
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	confirmText(false);

	setEditorDirty(iParams);

	updateTab();
}
void IsolineEventRouter::
guiStartChangeIsoSelection(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	
	//Save the previous isovalue max and min
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	const vector<double>& ivalues = iParams->GetIsovalues();
	prevIsoMin = 1.e30, prevIsoMax = -1.e30;
	for (int i = 0; i<ivalues.size(); i++){
		if (prevIsoMin > ivalues[i]) prevIsoMin = ivalues[i];
		if (prevIsoMax < ivalues[i]) prevIsoMax = ivalues[i]; 
	}
   
}
//This will set dirty bits and undo/redo changes to histo bounds and eventually iso value
void IsolineEventRouter::
guiEndChangeIsoSelection(){
	
	
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	float bnds[2];
	iParams->GetHistoBounds(bnds);
	//see if it's necessary to Change min/max isovalue settings and hange histo bounds.
	const vector<double>& ivalues = iParams->GetIsovalues();
	double minIso = 1.e30, maxIso = -1.e30;
	
	for (int i = 0; i<ivalues.size(); i++){
		if (minIso > ivalues[i]) minIso = ivalues[i];
		if (maxIso < ivalues[i]) maxIso = ivalues[i]; 
	}

	if (minIso != prevIsoMin || maxIso != prevIsoMax){
		//If the isovalues are new and the new values are not inside the histo bounds, respecify the bounds
		float newHistoBounds[2];
		if (minIso <= bnds[0] || maxIso >= bnds[1]){
			newHistoBounds[0]=maxIso - 1.1*(maxIso-minIso);
			newHistoBounds[1]=minIso + 1.1*(maxIso-minIso);
			iParams->SetHistoBounds(newHistoBounds);
		}
	} else {
		//If the isovalues are not inside the histo bounds, move the isovalues
		if (minIso < bnds[0] || maxIso > bnds[1]){
			fitIsovalsToHisto(iParams);
		}
	}
	
	updateTab();
	//force redraw with changed isovalues
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
}
//Set isoControl editor  dirty.
void IsolineEventRouter::
setEditorDirty(RenderParams* p){
	IsolineParams* ip = dynamic_cast<IsolineParams*>(p);
	if(!ip) ip = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	if(ip->GetIsoControl())ip->GetIsoControl()->setParams(ip);
    myIsovals->isoSelectionFrame->setMapperFunction(ip->GetIsoControl());
	myIsovals->isoSelectionFrame->setVariableName(ip->GetVariableName());
	myIsovals->isoSelectionFrame->setIsoValue(ip->GetIsovalues()[0]);
    myIsovals->isoSelectionFrame->updateParams();
	
	if (DataStatus::getInstance()->getNumActiveVariables()){
		
		const std::string& varname = ip->GetVariableName();
		myIsovals->isoSelectionFrame->setVariableName(varname);
		
	} else {
		myIsovals->isoSelectionFrame->setVariableName("N/A");
	}
	
	myIsovals->isoSelectionFrame->update();
	
}
/*
 * Method to be invoked after the user has changed a variable
 * Make the labels consistent with the new left/right data limits, but
 * don't trigger a new undo/redo event
 */
void IsolineEventRouter::
updateHistoBounds(RenderParams* params){
	QString strn;
	IsolineParams* iParams = (IsolineParams*)params;
	//Find out what timestep is current:
	int viznum = iParams->GetVizNum();
	if (viznum < 0) return;
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentTimestep();
	DataStatus* ds = DataStatus::getInstance();
	
	float minval, maxval;
	string varname = iParams->GetVariableName();
	
	if (iParams->IsEnabled()){
		float range[2];
		ds->getDataMgr()->GetDataRange(currentTimeStep, varname.c_str(),range);
		minval = range[0];
		maxval = range[1];
	} else {
		minval = ds->getDefaultDataMin(varname);
		maxval = ds->getDefaultDataMax(varname);
	}

	myIsovals->minDataBound->setText(strn.setNum(minval));
	myIsovals->maxDataBound->setText(strn.setNum(maxval));
	
}

//Obtain a new histogram for the current selected variable.
//Save it at the position associated with firstVarNum
void IsolineEventRouter::
refreshHistogram(RenderParams* p){
	
	IsolineParams* pParams = (IsolineParams*)p;
	bool is3D = pParams->VariablesAre3D();
	if (!is3D) {
		refresh2DHistogram(pParams);
		return;
	}
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();

	if(pParams->doBypass(timeStep)) return;
	float minRange, maxRange;
	minRange = pParams->GetMapperFunc()->getMinMapValue();
	maxRange = pParams->GetMapperFunc()->getMaxMapValue();
	if (!currentHistogram){
		currentHistogram = new Histo(256,minRange, maxRange);
	}
	

	currentHistogram->reset(256,minRange,maxRange);
	calcSliceHistogram(pParams, timeStep, currentHistogram);
}
void IsolineEventRouter::guiFitToData(){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	//Get bounds from DataStatus:
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	 
	float range[2];
	ds->getDataMgr()->GetDataRange(ts,iParams->GetVariableName().c_str(),range);
	
	if (range[1]<range[0]){ //no data
		range[1] = 1.f;
		range[0] = 0.f;
	}
	
	iParams->GetIsoControl()->setMinHistoValue(range[0]);
	iParams->GetIsoControl()->setMaxHistoValue(range[1]);
	updateTab();
	
}

void IsolineEventRouter::copyToProbeOr2D(){
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	if (iParams->VariablesAre3D()) guiCopyToProbe();
	else guiCopyTo2D();
}
void IsolineEventRouter::guiCopyToProbe(){
	/*
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_probeParamsTag);
	
	//Copy the Box
	const vector<double>& exts = iParams->GetBox()->GetLocalExtents();
	const vector<double>& angles = iParams->GetBox()->GetAngles();
	pParams->GetBox()->SetAngles(angles);
	pParams->GetBox()->SetLocalExtents(exts);
	
	//set the variable:
	const std::string& varname = iParams->GetVariableName();
	int sesvarnum = DataStatus::getInstance()->getSessionVariableNum3D(varname);
	pParams->setVariableSelected(sesvarnum,true);
	pParams->SetCompressionLevel(iParams->GetCompressionLevel());
	pParams->SetRefinementLevel(iParams->GetRefinementLevel());
	//Modify the TransferFunction
	TransferFunction* tf = pParams->GetTransFunc();
	
	convertIsovalsToColors(tf);
	pParams->setProbeDirty();
	*/
	
}
void IsolineEventRouter::guiCopyTo2D(){
	/*
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_twoDDataParamsTag);
	
	//Copy the Box
	const vector<double>& exts = iParams->GetBox()->GetLocalExtents();
	pParams->GetBox()->SetLocalExtents(exts);
	const std::string& varname = iParams->GetVariableName();
	int sesvarnum = DataStatus::getInstance()->getSessionVariableNum2D(varname);
	pParams->setVariableSelected(sesvarnum,true);
	pParams->SetCompressionLevel(iParams->GetCompressionLevel());
	pParams->SetRefinementLevel(iParams->GetRefinementLevel());
	//Modify the TransferFunction
	TransferFunction* tf = pParams->GetTransFunc();
	convertIsovalsToColors(tf);
	pParams->setTwoDDirty();
	*/
}
void IsolineEventRouter::convertIsovalsToColors(TransferFunction* tf){
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	vector<double> isovals = iParams->GetIsovalues();//copy the isovalues from the params
	//If the colormap is discrete, make it (temporarily) linear
	bool wasDiscrete = (tf->getColorInterpType() == TFInterpolator::discrete);
	if (wasDiscrete) tf->setColorInterpType(TFInterpolator::linear);
	ColorMapBase* cmap = tf->GetColorMap();
	std::sort(isovals.begin(),isovals.end());

	//Determine the minimum and maximum values for the color map
	//initially start with an interval about the first isovalue in case there is only one isovalue
	float bnds[2];
	iParams->GetHistoBounds(bnds);
	float minMapValue = isovals[0]-(bnds[1]-bnds[0]);
	float maxMapValue = isovals[0]+(bnds[1]-bnds[0]);
	if (isovals.size() > 1) {
		minMapValue = isovals[0] - 0.5*(isovals[1]-isovals[0]);
		maxMapValue = isovals[isovals.size()-1] + 0.5*(isovals[isovals.size()-1]-isovals[isovals.size()-2]);
	}
	tf->setMinMapValue(minMapValue);
	tf->setMaxMapValue(maxMapValue);
	
	//For each intermediate isovalue, determine 1/2 the minimum distance to left and right neighbor
	vector<float> leftMidPosn;
	leftMidPosn.push_back(minMapValue);  //first is simply minMapValue
	for (int i = 1; i<isovals.size()-1; i++){
		float mindist = 0.5*Min(isovals[i]-isovals[i-1], isovals[i+1]-isovals[i]);
		leftMidPosn.push_back(isovals[i]-mindist);
	}
	if(isovals.size()>1) leftMidPosn.push_back(isovals[isovals.size()-1]-(maxMapValue - isovals[isovals.size()-1]));
	leftMidPosn.push_back(maxMapValue);//last is maxMapValue, to right of all the positions.
	//Save the colors at the midpoints
	vector<ColorMapBase::Color*> leftMidColors;
	leftMidColors.push_back(new ColorMapBase::Color(cmap->controlPointColor(0)));
	for (int i = 1; i<isovals.size(); i++){
		float midPoint = 0.5*(isovals[i-1]+(isovals[i-1]-leftMidPosn[i-1])+leftMidPosn[i]);
		ColorMapBase::Color* mdColor = new ColorMapBase::Color(cmap->color(midPoint));
		leftMidColors.push_back(mdColor);
	}
	int ncolors = cmap->numControlPoints();
	leftMidColors.push_back(new ColorMapBase::Color(cmap->controlPointColor(ncolors-1)));
	//Now delete all the color control points:
	cmap->clear();
	//Insert a color control point at min
	cmap->addControlPointAt(minMapValue,*leftMidColors[0]);
	//Insert a color control point at each leftmidpoint
	for (int i = 1; i<isovals.size(); i++){
		cmap->addControlPointAt(leftMidPosn[i],*leftMidColors[i]);
		//Insert another one of same color if the two intermediate points between isovals[i-1] and isovals[i] are not close
		if ( (leftMidPosn[i] - (isovals[i-1]+isovals[i-1]-leftMidPosn[i-1])) > (maxMapValue - minMapValue)/512.){
			float otherPoint = (isovals[i-1]+isovals[i-1]-leftMidPosn[i-1]);
			cmap->addControlPointAt(otherPoint,*leftMidColors[i]);
		}
	}
	//Insert the last color
	cmap->addControlPointAt(maxMapValue, *leftMidColors[leftMidColors.size()-1]);
	//
	//Delete all the colors:
	for (int i = 0; i<leftMidColors.size(); i++) delete leftMidColors[i];
	//Make the color map discrete
	tf->setColorInterpType(TFInterpolator::discrete);
}
/*
 * Method to be invoked after the user has moved the right or left bounds
 * (e.g. From the MapEditor. ) 
 * Make the textboxes consistent with the new left/right bounds, but
 * don't trigger a new undo/redo event
 */
void IsolineEventRouter::
updateMapBounds(RenderParams* params){
	IsolineParams* iParams = (IsolineParams*)params;
	QString strn;
	IsoControl* mpFunc = (IsoControl*)iParams->GetIsoControl();
	if (mpFunc){
		myIsovals->leftHistoEdit->setText(strn.setNum(mpFunc->getMinMapValue(),'g',4));
		myIsovals->rightHistoEdit->setText(strn.setNum(mpFunc->getMaxMapValue(),'g',4));
	} 
	setEditorDirty(iParams);

}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void IsolineEventRouter::
isolineSaveTF(void){
	//IsolineParams* dParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	//saveTF(dParams);
}

void IsolineEventRouter::
isolineLoadInstalledTF(){
	IsolineParams* dParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	IsoControl* tf = dParams->GetIsoControl();
	if (!tf) return;
	float minb = tf->getMinMapValue();
	float maxb = tf->getMaxMapValue();
	if (minb >= maxb){ minb = 0.0; maxb = 1.0;}
	//loadInstalledTF(dParams,0);
	tf = dParams->GetIsoControl();
	tf->setMinHistoValue(minb);
	tf->setMaxHistoValue(maxb);
	setEditorDirty(dParams);
	
}
void IsolineEventRouter::
isolineLoadTF(void){
	//If there are no TF's currently in Session, just launch file load dialog.
	//IsolineParams* dParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	//loadTF(dParams,dParams->GetVariableName());
	
}
//Respond to user request to load/save TF
//Assumes name is valid
//
void IsolineEventRouter::
sessionLoadTF(QString* name){/*
	IsolineParams* dParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	confirmText(false);
	
	
	//Get the transfer function from the session:
	
	std::string s(name->toStdString());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getSessionVarNum();
	dParams->hookupTF(tf, varNum);

	setEditorDirty(dParams);
	
	VizWinMgr::getInstance()->setClutDirty(dParams);
	*/
}
//Launch an editor on the isovalues
void IsolineEventRouter::guiEditIsovalues(){
	IsolineParams* iParams =(IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	confirmText(false);

	IsovalueEditor sle(iParams->getNumIsovalues(), iParams);
	if (!sle.exec()){
		return;
	}
	vector<double>isovals = iParams->GetIsovalues();
	std::sort(isovals.begin(),isovals.end());
	iParams->SetIsovalues(isovals);
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams,MouseModeParams::GetCurrentMouseMode() == MouseModeParams::isolineMode);
	updateTab();
}
//Respond to densitySlider release
void IsolineEventRouter::guiSetIsolineDensity(){
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	confirmText(false);

	int sliderVal = myAppearance->densitySlider->value();
	double den = sliderVal/256.;
	iParams->SetTextDensity(den);
	myAppearance->densityEdit->setText(QString::number(den));

	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams);
	guiSetTextChanged(false);
}
void IsolineEventRouter::guiSpaceIsovalues(){
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	const vector<double>& isoVals = iParams->GetIsovalues();
	int numIsos = isoVals.size();
	if (numIsos <= 1) return;
	confirmText(false);


	double minIso = (double)myIsovals->minIsoEdit->text().toDouble();
	double interval = (double)myIsovals->isoSpaceEdit->text().toDouble();
	iParams->spaceIsovals(minIso,interval);

	setIsolineDirty(iParams);
	updateTab();
	VizWinMgr::getInstance()->forceRender(iParams);
	guiSetTextChanged(false);
}
void IsolineEventRouter::guiEnableText(bool val){
	IsolineParams* iParams =(IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	confirmText(false);
	

	iParams->SetTextEnabled(val);
	
	
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams);
	guiSetTextChanged(false);
}
/*
 * Respond to user clicking the color button
 */
void IsolineEventRouter::
guiSetSingleColor(){
	QPalette pal(myAppearance->singleColorEdit->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	pal.setColor(QPalette::Base, newColor);
	myAppearance->singleColorEdit->setPalette(pal);
	double rgb[3];
	rgb[0] = (double)newColor.red()/256.;
	rgb[1] = (double)newColor.green()/256.;
	rgb[2] = (double)newColor.blue()/256.;
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	iParams->SetSingleColor(rgb);
	
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams);
}
/*
 * Respond to user checking or unchecking the single color checkbox
 */
void IsolineEventRouter::
guiSetUseSingleColor(bool val){
	
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)ControlExec::GetActiveParams(IsolineParams::_isolineParamsTag);
	
	iParams->SetUseSingleColor(val);
	
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams);
}
//Rescale isovalues to fit in histo
void IsolineEventRouter::fitIsovalsToHisto(IsolineParams* iParams){
	float bounds[2];
	iParams->GetHistoBounds(bounds);
	const vector<double>& isovals = iParams->GetIsovalues();
	vector<double> newIsovals;
	if (isovals.size()==1){
		double newval = (bounds[0]+bounds[1])*0.5;
		newIsovals.push_back(newval);
	} else {
		//find min and max
		double isoMax = -1.e30, isoMin = 1.e30;
		for (int i = 0; i<isovals.size(); i++){
			if (isoMax<isovals[i]) isoMax = isovals[i];
			if (isoMin>isovals[i]) isoMin = isovals[i];
		}
		//Now rearrange proportionately to fit in bounds
		float newmin = bounds[0];
		float newmax = bounds[1];
		//Don't change bounds if isos are inside
		if (isoMin >= newmin && isoMax <= newmax && isoMin <= newmax && isoMax >= newmin) return;
		//If there's only one, put it in the middle:
		if (isovals.size() <= 0 || isoMax <= isoMin){
			newmin = 0.5*(bounds[1]+bounds[0]);
			isoMax = isoMin+1.; //prevent divide by zero in upcoming rescale
		} 
		//Arrange them proportionately between newmin and newmax
		for (int i = 0; i<isovals.size(); i++){
			double newIsoval = newmin + (newmax-newmin)*(isovals[i]-isoMin)/(isoMax-isoMin);
			newIsovals.push_back(newIsoval);
		}
	}
	iParams->SetIsovalues(newIsovals);
}
