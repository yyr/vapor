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
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "floweventrouter.h"
#include "instancetable.h"
#include "qtthumbwheel.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "params.h"
#include "isolinetab.h"
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

using namespace VAPoR;
const float IsolineEventRouter::thumbSpeedFactor = 0.0005f;  //rotates ~45 degrees at full thumbwheel width
const char* IsolineEventRouter::webHelpText[] = 
{
	"Isoline overview",
	"Renderer control",
	"Data accuracy control",
	"Isoline basic settings",
	"Isoline layout",
	"Isoline image settings",
	"Isoline appearance",
	"Isoline orientation",
	
	"<>"
};
const char* IsolineEventRouter::webHelpURL[] =
{

	"http://www.vapor.ucar.edu/docs/vapor-gui-help/isoline-tab-data-Isoline-or-contour-plane",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/renderer-instances",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/refinement-and-lod-control",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/Isoline-tab-data-Isoline-or-contour-plane#BasicIsolineSettings",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/Isoline-tab-data-Isoline-or-contour-plane#IsolineLayout",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/Isoline-tab-data-Isoline-or-contour-plane#IsolineImageSettings",
	"http://www.vapor.ucar.edu/docs/vapor-gui-help/Isoline-tab-data-Isoline-or-contour-plane#IsolineAppearance",
	"http://www.vapor.ucar.edu/docs/vapor-gui-general-guide/orientation-Isoline",
	
};

IsolineEventRouter::IsolineEventRouter(QWidget* parent): QWidget(parent), Ui_IsolineTab(), EventRouter(){
	setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(IsolineParams::_isolineParamsTag);
	myWebHelpActions = makeWebHelpActions(webHelpText,webHelpURL);
	isoSelectionFrame->setOpacityMapping(true);
	isoSelectionFrame->setColorMapping(true);
	isoSelectionFrame->setIsoSlider(false);
	isoSelectionFrame->setIsolineSliders(true);
	fidelityButtons = 0;
	savedCommand = 0;
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
	isoSelectionFrame->setIsolineSliders(true);
	
	for (int i = 0; i<3; i++)maxBoxSize[i] = 1.f;
	MessageReporter::infoMsg("IsolineEventRouter::IsolineEventRouter()");

#if defined(Darwin) && (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	opacityMapShown = false;
	texShown = false;
	isolineImageFrame->hide();
	isoSelectionFrame->hide();
#endif
	
}


IsolineEventRouter::~IsolineEventRouter(){
	if (savedCommand) delete savedCommand;
	
	
}
/**********************************************************
 * Whenever a new Isolinetab is created it must be hooked up here
 ************************************************************/
void
IsolineEventRouter::hookUpTab()
{
	//Nudge sliders by clicking on slider bar:
	connect (xSizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXSize(int)));
	connect (xCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeXCenter(int)));
	connect (ySizeSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYSize(int)));
	connect (yCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeYCenter(int)));
	connect (zCenterSlider, SIGNAL(valueChanged(int)), this, SLOT(guiNudgeZCenter(int)));
	connect (xCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (yCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (zCenterEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (thetaEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (phiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (psiEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (xSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (ySizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (minIsoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (maxIsoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (countIsoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (isolineWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (panelLineWidthEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (textSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (panelTextSizeEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (histoScaleEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (leftHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (rightHistoEdit, SIGNAL(textChanged(const QString&)), this, SLOT(setIsolineTabTextChanged(const QString&)));
	connect (fidelityDefaultButton, SIGNAL(clicked()), this, SLOT(guiSetFidelityDefault()));
	connect (fitDataButton, SIGNAL(clicked()), this, SLOT(guiFitToData()));
	connect (fitIsovalsButton, SIGNAL(clicked()), this, SLOT(guiFitIsovalsToHisto()));

	connect (xCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (yCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (zCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (xSizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (ySizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (thetaEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (phiEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (psiEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (minIsoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (maxIsoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (countIsoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (isolineWidthEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (panelLineWidthEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (textSizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (panelTextSizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (leftHistoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (rightHistoEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (histoScaleEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));

	connect (loadButton, SIGNAL(clicked()), this, SLOT(isolineLoadTF()));
	connect (loadInstalledButton, SIGNAL(clicked()), this, SLOT(isolineLoadInstalledTF()));
	connect (saveButton, SIGNAL(clicked()), this, SLOT(isolineSaveTF()));
	connect (regionCenterButton, SIGNAL(clicked()), this, SLOT(isolineCenterRegion()));
	connect (viewCenterButton, SIGNAL(clicked()), this, SLOT(isolineCenterView()));
	connect (rakeCenterButton, SIGNAL(clicked()), this, SLOT(isolineCenterRake()));
	connect (probeCenterButton, SIGNAL(clicked()), this, SLOT(guiCenterProbe()));
	connect (addSeedButton, SIGNAL(clicked()), this, SLOT(isolineAddSeed()));
	connect (axisAlignCombo, SIGNAL(activated(int)), this, SLOT(guiAxisAlign(int)));
	connect (fitRegionButton, SIGNAL(clicked()), this, SLOT(guiFitRegion()));
	connect (fitDomainButton, SIGNAL(clicked()), this, SLOT(guiFitDomain()));
	connect (cropRegionButton, SIGNAL(clicked()), this, SLOT(guiCropToRegion()));
	connect (cropDomainButton, SIGNAL(clicked()), this, SLOT(guiCropToDomain()));

	connect (xThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateXWheel(int)));
	connect (yThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateYWheel(int)));
	connect (zThumbWheel, SIGNAL(valueChanged(int)), this, SLOT(rotateZWheel(int)));
	connect (xThumbWheel, SIGNAL(wheelReleased(int)), this, SLOT(guiReleaseXWheel(int)));
	connect (yThumbWheel, SIGNAL(wheelReleased(int)), this, SLOT(guiReleaseYWheel(int)));
	connect (zThumbWheel, SIGNAL(wheelReleased(int)), this, SLOT(guiReleaseZWheel(int)));
	connect (xThumbWheel, SIGNAL(wheelPressed()), this, SLOT(pressXWheel()));
	connect (yThumbWheel, SIGNAL(wheelPressed()), this, SLOT(pressYWheel()));
	connect (zThumbWheel, SIGNAL(wheelPressed()), this, SLOT(pressZWheel()));
	
	connect (attachSeedCheckbox,SIGNAL(toggled(bool)),this, SLOT(isolineAttachSeed(bool)));
	connect (dimensionCombo,SIGNAL(activated(int)), this, SLOT(guiSetDimension(int)));
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (rotate90Combo,SIGNAL(activated(int)), this, SLOT(guiRotate90(int)));
	connect (variableCombo,SIGNAL(activated(int)), this, SLOT(guiChangeVariable(int)));
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineZCenter()));
	connect (xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineXSize()));
	connect (ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineYSize()));

	connect (copyToProbeButton, SIGNAL(clicked()), this, SLOT(copyToProbeOr2D()));

	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setIsolineEnabled(bool,int)));
	
	connect (showHideLayoutButton, SIGNAL(pressed()), this, SLOT(showHideLayout()));
	connect (showHideImageButton, SIGNAL(pressed()), this, SLOT(showHideImage()));
	connect (showHideAppearanceButton, SIGNAL(pressed()), this, SLOT(showHideAppearance()));
	connect (isolinePanelBackgroundColorButton, SIGNAL(clicked()), this, SLOT(guiSetPanelBackgroundColor()));
	connect(newHistoButton, SIGNAL(clicked()), this, SLOT(refreshHisto()));
	connect(editButton, SIGNAL(toggled(bool)), this, SLOT(setIsolineEditMode(bool)));
	connect(navigateButton, SIGNAL(toggled(bool)), this, SLOT(setIsolineNavigateMode(bool)));
	connect(editIsovaluesButton, SIGNAL(clicked()), this, SLOT(guiEditIsovalues()));

	// isoSelectionFrame controls:
	connect(editButton, SIGNAL(toggled(bool)), 
            isoSelectionFrame, SLOT(setEditMode(bool)));
	connect(alignButton, SIGNAL(clicked()), this, SLOT(guiSetAligned()));
	connect(alignButton, SIGNAL(clicked()),
            isoSelectionFrame, SLOT(fitToView()));
    connect(isoSelectionFrame, SIGNAL(startChange(QString)), 
            this, SLOT(guiStartChangeIsoSelection(QString)));
    connect(isoSelectionFrame, SIGNAL(endChange()),
            this, SLOT(guiEndChangeIsoSelection()));
}
//Insert values from params into tab panel
//
void IsolineEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	MainForm::getInstance()->buildWebTabHelpMenu(myWebHelpActions);
	if (GLWindow::isRendering())return;
	guiSetTextChanged(false);
	setIgnoreBoxSliderEvents(true);  //don't generate nudge events

	DataStatus* ds = DataStatus::getInstance();

	if (ds->getDataMgr() && (ds->dataIsPresent2D() ||ds->dataIsPresent3D())) instanceTable->setEnabled(true);
	else return;
	instanceTable->rebuild(this);
	
	IsolineParams* isolineParams = VizWinMgr::getActiveIsolineParams();
	isoSelectionFrame->setIsolineSliders(isolineParams->GetIsovalues());
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	size_t timestep = (size_t)vizMgr->getActiveAnimationParams()->getCurrentTimestep();
	int winnum = vizMgr->getActiveViz();
	if (ds->getDataMgr() && fidelityDefaultChanged){
		setupFidelity(3, fidelityLayout,fidelityBox, isolineParams, false);
		connect(fidelityButtons,SIGNAL(buttonClicked(int)),this, SLOT(guiSetFidelity(int)));
		fidelityDefaultChanged = false;
	}
	if (ds->getDataMgr()) updateFidelity(fidelityBox, isolineParams,lodCombo,refinementCombo);
	
	guiSetTextChanged(false);

	
	deleteInstanceButton->setEnabled(Params::GetNumParamsInstances(IsolineParams::_isolineParamsTag, winnum) > 1);
	

	int numViz = vizMgr->getNumVisualizers();

	copyCombo->clear();
	copyCombo->addItem("Duplicate In:");
	copyCombo->addItem("This visualizer");
	if (numViz > 1) {
		int copyNum = 2;
		for (int i = 0; i<MAXVIZWINS; i++){
			if (vizMgr->getVizWin(i) && winnum != i){
				copyCombo->addItem(vizMgr->getVizWinName(i));
				//Remember the viznum corresponding to a combo item:
				copyCount[copyNum++] = i;
			}
		}
	}
	
	
	QString strn;
	Session* ses = Session::getInstance();
	ses->blockRecording();

	guiSetTextChanged(false);
	
	//Setup render window size:
	resetImageSize(isolineParams);
	//setup the size sliders 
	adjustBoxSize(isolineParams);
	if (!DataStatus::getInstance()->getDataMgr()) {
		ses->unblockRecording();
		return;
	}
	const vector<double>&userExts = ds->getDataMgr()->GetExtents(timestep);
	//And the center sliders/textboxes:
	double locExts[6],boxCenter[3];
	const float* fullSizes = ds->getFullSizes();
	isolineParams->GetBox()->GetLocalExtents(locExts);
	for (int i = 0; i<3; i++) boxCenter[i] = (locExts[i]+locExts[3+i])*0.5f;
	xCenterSlider->setValue((int)(256.f*boxCenter[0]/fullSizes[0]));
	yCenterSlider->setValue((int)(256.f*boxCenter[1]/fullSizes[1]));
	zCenterSlider->setValue((int)(256.f*boxCenter[2]/fullSizes[2]));
	xCenterEdit->setText(QString::number(userExts[0]+boxCenter[0]));
	yCenterEdit->setText(QString::number(userExts[1]+boxCenter[1]));
	zCenterEdit->setText(QString::number(userExts[2]+boxCenter[2]));

	//Calculate extents of the containing box
	float corners[8][3];
	isolineParams->calcLocalBoxCorners(corners, 0.f, -1);
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

	minUserXLabel->setText(QString::number(userExts[0]+dboxmin[0]));
	minUserYLabel->setText(QString::number(userExts[1]+dboxmin[1]));
	minUserZLabel->setText(QString::number(userExts[2]+dboxmin[2]));
	maxUserXLabel->setText(QString::number(userExts[0]+dboxmax[0]));
	maxUserYLabel->setText(QString::number(userExts[1]+dboxmax[1]));
	maxUserZLabel->setText(QString::number(userExts[2]+dboxmax[2]));

	//And convert these to grid coordinates:
	
	DataMgr* dataMgr = ds->getDataMgr();
	if (dataMgr && showLayout){
		isolineParams->mapBoxToVox(dataMgr,isolineParams->GetRefinementLevel(),isolineParams->GetCompressionLevel(),timestep,gridExts);
		
		minGridXLabel->setText(QString::number(gridExts[0]));
		minGridYLabel->setText(QString::number(gridExts[1]));
		minGridZLabel->setText(QString::number(gridExts[2]));
		maxGridXLabel->setText(QString::number(gridExts[3]));
		maxGridYLabel->setText(QString::number(gridExts[4]));
		maxGridZLabel->setText(QString::number(gridExts[5]));
	}
	vector<double>ivalues = isolineParams->GetIsovalues();
	//find min and max
	double isoMax = -1.e30, isoMin = 1.e30;
	for (int i = 0; i<ivalues.size(); i++){
		if (isoMax<ivalues[i]) isoMax = ivalues[i];
		if (isoMin>ivalues[i]) isoMin = ivalues[i];
	}
	minIsoEdit->setText(QString::number(isoMin));
	maxIsoEdit->setText(QString::number(isoMax));
	countIsoEdit->setText(QString::number(ivalues.size()));
	float histoBounds[2];
	isolineParams->GetHistoBounds(histoBounds);
	leftHistoEdit->setText(QString::number(histoBounds[0]));
	rightHistoEdit->setText(QString::number(histoBounds[1]));
	histoScaleEdit->setText(QString::number(isolineParams->GetHistoStretch()));
	

	isolineWidthEdit->setText(QString::number(isolineParams->GetLineThickness()));
	panelLineWidthEdit->setText(QString::number(isolineParams->GetPanelLineThickness()));
	textSizeEdit->setText(QString::number(isolineParams->GetTextSize()));
	panelTextSizeEdit->setText(QString::number(isolineParams->GetPanelTextSize()));
	//set color buttons
	QPalette pal;
	const vector<double>&bColor = isolineParams->GetPanelBackgroundColor();
	QColor clr = QColor((int)(255*bColor[0]),(int)(255*bColor[1]),(int)(255*bColor[2]));
	pal.setColor(QPalette::Base, clr);
	isolinePanelBackgroundColorEdit->setPalette(pal);
	
	//Provide latlon box extents if available:
	if (DataStatus::getProjectionString().size() == 0){
		minMaxLonLatFrame->hide();
	} else {
		double boxLatLon[4];
		
		boxLatLon[0] = locExts[0];
		boxLatLon[1] = locExts[1];
		boxLatLon[2] = locExts[3];
		boxLatLon[3] = locExts[4];
		
		if (DataStatus::convertLocalToLonLat((int)timestep, boxLatLon,2)){
			minLonLabel->setText(QString::number(boxLatLon[0]));
			minLatLabel->setText(QString::number(boxLatLon[1]));
			maxLonLabel->setText(QString::number(boxLatLon[2]));
			maxLatLabel->setText(QString::number(boxLatLon[3]));
			minMaxLonLatFrame->show();
		} else {
			minMaxLonLatFrame->hide();
		}
	}
    
	thetaEdit->setText(QString::number(isolineParams->getTheta(),'f',1));
	phiEdit->setText(QString::number(isolineParams->getPhi(),'f',1));
	psiEdit->setText(QString::number(isolineParams->getPsi(),'f',1));
	mapCursor();
	
	const float* selectedPoint = isolineParams->getSelectedPointLocal();
	float selectedUserCoords[3];
	//selectedPoint is in local coordinates.  convert to user coords:
	for (int i = 0; i<3; i++)selectedUserCoords[i] = selectedPoint[i]+userExts[i];
	selectedXLabel->setText(QString::number(selectedUserCoords[0]));
	selectedYLabel->setText(QString::number(selectedUserCoords[1]));
	selectedZLabel->setText(QString::number(selectedUserCoords[2]));

	//Provide latlon coords if available:
	if (DataStatus::getProjectionString().size() == 0){
		latLonFrame->hide();
	} else {
		double selectedLatLon[2];
		selectedLatLon[0] = selectedUserCoords[0];
		selectedLatLon[1] = selectedUserCoords[1];
		if (DataStatus::convertToLonLat(selectedLatLon,1)){
			selectedLonLabel->setText(QString::number(selectedLatLon[0]));
			selectedLatLabel->setText(QString::number(selectedLatLon[1]));
			latLonFrame->show();
		} else {
			latLonFrame->hide();
		}
	}
	attachSeedCheckbox->setChecked(seedAttached);
	int sesVarNum;
	if (isolineParams->VariablesAre3D()){
		sesVarNum = ds->getSessionVariableNum3D(isolineParams->GetVariableName());
		minDataBound->setText(QString::number(ds->getDataMin3D(sesVarNum,timestep)));
		maxDataBound->setText(QString::number(ds->getDataMax3D(sesVarNum,timestep)));
		copyToProbeButton->setText("Copy to Probe");
		copyToProbeButton->setToolTip("Click to make the current active Probe display these isolines as a color contour plot");
	}
	else {
		sesVarNum = ds->getSessionVariableNum2D(isolineParams->GetVariableName());
		minDataBound->setText(QString::number(ds->getDataMin2D(sesVarNum,timestep)));
		maxDataBound->setText(QString::number(ds->getDataMax2D(sesVarNum,timestep)));
		copyToProbeButton->setText("Copy to 2D");
		copyToProbeButton->setToolTip("Click to make the current active 2D Data display these isolines as a color contour plot");
	}
	
	float val = 0.;
	if (isolineParams->isEnabled())
		val = calcCurrentValue(isolineParams,selectedUserCoords,&sesVarNum, 1);
	
	if (val == OUT_OF_BOUNDS)
		valueMagLabel->setText(QString(" "));
	else valueMagLabel->setText(QString::number(val));
	guiSetTextChanged(false);
	//Set the selection in the variable combo
	//Turn off combo message-listening
	ignoreComboChanges = true;
	variableCombo->setCurrentIndex(sesVarNum);
	int dim = isolineParams->VariablesAre3D() ? 3 : 2;
	dimensionCombo->setCurrentIndex(dim-2);
	ignoreComboChanges = false;
	
	if(isolineParams->GetIsoControl()){
		isolineParams->GetIsoControl()->setParams(isolineParams);
		isoSelectionFrame->setMapperFunction(isolineParams->GetIsoControl());
	}
	
    isoSelectionFrame->setVariableName(isolineParams->GetVariableName());
	updateHistoBounds(isolineParams);
	
	isoSelectionFrame->updateParams();
	
	isolineImageFrame->setParams(isolineParams);
	isolineImageFrame->update();
	if (showLayout) {
		layoutFrame->show();
		if (dim == 3) orientationFrame->show();
		else orientationFrame->hide();
	}
	else layoutFrame->hide();
	if (showAppearance) appearanceFrame->show();
	else appearanceFrame->hide();
	if (showImage) imageFrame->show();
	else imageFrame->hide();
	adjustSize();
	
	vizMgr->getTabManager()->update();
	
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();

	setIgnoreBoxSliderEvents(false);
	
}
//Fix for clean Windows scrolling:
void IsolineEventRouter::refreshTab(){
	isolineFrameHolder->hide();
	isolineFrameHolder->show();
	appearanceFrame->hide();
	appearanceFrame->show();
}

void IsolineEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	IsolineParams* isolineParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(isolineParams, "edit Isoline text");
	QString strn;
	if (isolineParams->VariablesAre3D()){
		float thetaVal = thetaEdit->text().toFloat();
		while (thetaVal > 180.f) thetaVal -= 360.f;
		while (thetaVal < -180.f) thetaVal += 360.f;
		thetaEdit->setText(QString::number(thetaVal,'f',1));
		float phiVal = phiEdit->text().toFloat();
		while (phiVal > 180.f) phiVal -= 180.f;
		while (phiVal < 0.f) phiVal += 180.f;
		phiEdit->setText(QString::number(phiVal,'f',1));
		float psiVal = psiEdit->text().toFloat();
		while (psiVal > 180.f) psiVal -= 360.f;
		while (psiVal < -180.f) psiVal += 360.f;
		psiEdit->setText(QString::number(psiVal,'f',1));

		isolineParams->setTheta(thetaVal);
		isolineParams->setPhi(phiVal);
		isolineParams->setPsi(psiVal);
	}

	vector<double>ivalues;
	
	double maxIso = (double)maxIsoEdit->text().toDouble();
	double minIso = (double)minIsoEdit->text().toDouble();
	double prevMinIso = 1.e30, prevMaxIso = -1.e30;
	const vector<double>& prevIsoVals = isolineParams->GetIsovalues(); 
	//Determine previous min/max of isovalues
	for (int i = 0; i<prevIsoVals.size(); i++){
		if (prevIsoVals[i]<prevMinIso) prevMinIso = prevIsoVals[i];
		if (prevIsoVals[i]>prevMaxIso) prevMaxIso = prevIsoVals[i];
	}
	
	int numIsos = countIsoEdit->text().toInt();
	if (numIsos < 1) numIsos = 1;
	if (maxIso < minIso) {
		maxIso = minIso;
		numIsos = 1;
	}
	if (maxIso > minIso && numIsos == 1){
		maxIso = minIso = 0.5*(maxIso+minIso);
	}
	float bnds[2];
	bnds[0] = leftHistoEdit->text().toFloat();
	bnds[1] = rightHistoEdit->text().toFloat();
	if (bnds[0] >= bnds[1]){
		bnds[0] = minIso - 0.1*(maxIso-minIso);
		bnds[1] = maxIso + 0.1*(maxIso-minIso);
	}
	isolineParams->SetHistoBounds(bnds);
	//If the number of isovalues is changing from 1 to a value >1, and if the
	//min and max are the same, then expand the min/max interval by .5 times the 
	//histo interval.
	if (maxIso == minIso && numIsos > 1) {
		minIso = maxIso - 0.25*(bnds[1]-bnds[0]);
		maxIso = maxIso + 0.25*(bnds[1]-bnds[0]);
	}
	ivalues.push_back(minIso);
	//Did numIso's change?  If so set intermediate values based on end points.
	if (numIsos != isolineParams->getNumIsovalues()){
		//Set intermediate isovalues based on end points
		
		for (int i = 1; i<numIsos; i++){
			ivalues.push_back(minIso + (maxIso - minIso)*(float)i/(float)(numIsos-1));
		}
		isolineParams->SetIsovalues(ivalues);
	// rescale isovalues to use new interval
	} else if ((minIso != prevMinIso || maxIso != prevMaxIso) && numIsos > 1){
		ivalues.clear();
		for (int i = 0; i< numIsos; i++){
			double frac = (prevIsoVals[i]-prevMinIso)/(prevMaxIso-prevMinIso);
			ivalues.push_back(minIso+frac*(maxIso-minIso));
		}
		isolineParams->SetIsovalues(ivalues);
	}
	minIsoEdit->setText(QString::number(minIso));
	maxIsoEdit->setText(QString::number(maxIso));
	countIsoEdit->setText(QString::number(ivalues.size()));

	isolineParams->SetHistoStretch(histoScaleEdit->text().toDouble());
	
	double thickness = isolineWidthEdit->text().toDouble();
	if (thickness <= 0. || thickness > 100.) thickness = 1.0;
	isolineParams->SetLineThickness(thickness);
	thickness = panelLineWidthEdit->text().toDouble();
	if (thickness <= 0. || thickness > 100.) thickness = 1.0;
	isolineParams->SetPanelLineThickness(thickness);
	double textsize = textSizeEdit->text().toDouble();
	if (textsize <= 0. || textsize > 100.) textsize = 10.0;
	isolineParams->SetTextSize(textsize);
	textsize = panelTextSizeEdit->text().toDouble();
	if (textsize <= 0. || textsize > 100.) textsize = 10.0;
	isolineParams->SetPanelTextSize(textsize);

	if (!DataStatus::getInstance()->getDataMgr()) return;

	size_t timestep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& userExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Set the isoline size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[0] = xSizeEdit->text().toFloat();
	boxSize[1] = ySizeEdit->text().toFloat();
	boxSize[2] =0.;
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
	}
	//Convert text to local extents:
	boxCenter[0] = xCenterEdit->text().toFloat()- userExts[0];
	boxCenter[1] = yCenterEdit->text().toFloat()- userExts[1];
	boxCenter[2] = zCenterEdit->text().toFloat()- userExts[2];
	const float* fullSizes = DataStatus::getInstance()->getFullSizes();
	for (int i = 0; i<3;i++){
		//Don't constrain the box to have center in the domain:
		boxmin[i] = boxCenter[i] - 0.5f*boxSize[i];
		boxmax[i] = boxCenter[i] + 0.5f*boxSize[i];
	}
	//Set the local box extents:
	isolineParams->setLocalBox(boxmin,boxmax, -1);
	adjustBoxSize(isolineParams);
	//set the center sliders:
	xCenterSlider->setValue((int)(256.f*boxCenter[0]/fullSizes[0]));
	yCenterSlider->setValue((int)(256.f*boxCenter[1]/fullSizes[1]));
	zCenterSlider->setValue((int)(256.f*boxCenter[2]/fullSizes[2]));
	resetImageSize(isolineParams);
	
	setIsolineDirty(isolineParams);
	
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(isolineParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, isolineParams);
	updateTab();
}


/*********************************************************************************
 * Slots associated with IsolineTab:
 *********************************************************************************/
void IsolineEventRouter::pressXWheel(){
	//Figure out the starting direction of z axis
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	renormalizedRotate = false;
	xThumbWheel->setValue(0);
	if (stretch[1] == stretch[2]) return;
	float rotMatrix[9];
	getRotationMatrix(pParams->getTheta()*M_PI/180., pParams->getPhi()*M_PI/180., pParams->getPsi()*M_PI/180., rotMatrix);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	renormalizedRotate = false;
	yThumbWheel->setValue(0);
	if (stretch[0] == stretch[2]) return;
	float rotMatrix[9];
	getRotationMatrix(pParams->getTheta()*M_PI/180., pParams->getPhi()*M_PI/180., pParams->getPsi()*M_PI/180., rotMatrix);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	renormalizedRotate = false;
	zThumbWheel->setValue(0);
	if (stretch[1] == stretch[0]) return;
	float rotMatrix[9];
	getRotationMatrix(pParams->getTheta()*M_PI/180., pParams->getPhi()*M_PI/180., pParams->getPsi()*M_PI/180., rotMatrix);
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

	ParamsBase::ParamsBaseType t = Params::GetTypeFromTag(IsolineParams::_isolineParamsTag);
	if(GLWindow::getModeFromParams(t) != GLWindow::getCurrentMouseMode()) return;

	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getGLWindow()->getManip(IsolineParams::_isolineParamsTag);
	
	if(!renormalizedRotate) manip->setTempRotation((float)val*thumbSpeedFactor, 0);
	else {
		
		double newrot = convertRotStretchedToActual(0,-startRotateViewAngle*180./M_PI+thumbSpeedFactor*(float)val);
		double angleChangeDg = newrot + startRotateActualAngle*180./M_PI;
		manip->setTempRotation((float)angleChangeDg, 0);
	}
	viz->updateGL();
	
	assert(!xThumbWheel->isSliderDown());
	
}
void IsolineEventRouter::
rotateYWheel(int val){
	ParamsBase::ParamsBaseType t = Params::GetTypeFromTag(IsolineParams::_isolineParamsTag);
	if(GLWindow::getModeFromParams(t) != GLWindow::getCurrentMouseMode()) return;

	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getGLWindow()->getManip(IsolineParams::_isolineParamsTag);
	
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
	ParamsBase::ParamsBaseType t = Params::GetTypeFromTag(IsolineParams::_isolineParamsTag);
	if(GLWindow::getModeFromParams(t) != GLWindow::getCurrentMouseMode()) return;

	//Find the current manip in the active visualizer
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getGLWindow()->getManip(IsolineParams::_isolineParamsTag);
	
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
	
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "rotate isoline");
	float finalRotate = (float)val*thumbSpeedFactor;
	if(renormalizedRotate) {
	
		float angleChangeDegView = (float)val*thumbSpeedFactor;
		double newrot2 = convertRotStretchedToActual(0,-startRotateViewAngle*180./M_PI+angleChangeDegView);
		double angleChangeDg = newrot2 + startRotateActualAngle*180./M_PI;
		finalRotate = angleChangeDg;
	}
	//apply rotation:
	pParams->rotateAndRenormalizeBox(0, finalRotate);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getGLWindow()->getManip(IsolineParams::_isolineParamsTag);
	manip->setTempRotation(0.f, 0);
	//reset the thumbwheel

	updateTab();
	setIsolineDirty(pParams);
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams, GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
}
void IsolineEventRouter::
guiReleaseYWheel(int val){
	confirmText(false);
	
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "rotate isoline");
	float finalRotate = -(float)val*thumbSpeedFactor;
	if(renormalizedRotate) {
	
		float angleChangeDegView = -(float)val*thumbSpeedFactor;
		double newrot2 = convertRotStretchedToActual(1,-startRotateViewAngle*180./M_PI+angleChangeDegView);
		double angleChangeDg = newrot2 + startRotateActualAngle*180./M_PI;
		finalRotate = angleChangeDg;
	}
	//apply rotation:
	pParams->rotateAndRenormalizeBox(1, finalRotate);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getGLWindow()->getManip(IsolineParams::_isolineParamsTag);
	manip->setTempRotation(0.f, 1);

	updateTab();
	setIsolineDirty(pParams);
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
}
void IsolineEventRouter::
guiReleaseZWheel(int val){
	confirmText(false);
	
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "rotate isoline");
	float finalRotate = (float)val*thumbSpeedFactor;
	if(renormalizedRotate) {
	
		float angleChangeDegView = (float)val*thumbSpeedFactor;
		double newrot2 = convertRotStretchedToActual(2,-startRotateViewAngle*180./M_PI+angleChangeDegView);
		double angleChangeDg = newrot2 + startRotateActualAngle*180./M_PI;
		finalRotate = angleChangeDg;
	}
	//apply rotation:
	pParams->rotateAndRenormalizeBox(2, finalRotate);

	//Reset the manip:
	VizWin* viz = VizWinMgr::getInstance()->getActiveVisualizer();
	TranslateRotateManip* manip = (TranslateRotateManip*)viz->getGLWindow()->getManip(IsolineParams::_isolineParamsTag);
	manip->setTempRotation(0.f, 2);

	updateTab();
	setIsolineDirty(pParams);
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
}

void IsolineEventRouter::guiRotate90(int selection){
	if (selection == 0) return;
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "90 deg isoline rotation");
	int axis = (selection < 4) ? selection - 1 : selection -4;
	float angle = (selection < 4) ? 90.f : -90.f;
	//Renormalize and apply rotation:
	pParams->rotateAndRenormalizeBox(axis, angle);
	rotate90Combo->setCurrentIndex(0);
	updateTab();
	setIsolineDirty(pParams);
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);

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

void IsolineEventRouter::guiCopyInstanceTo(int toViz){
	if (toViz == 0) return; 
	if (toViz == 1){performGuiCopyInstance(); return;}
	int viznum = copyCount[toViz];
	copyCombo->setCurrentIndex(0);
	performGuiCopyInstanceToViz(viznum);
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

	
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int activeViz = vizMgr->getActiveViz();
	
	IsolineParams* pParams = vizMgr->getIsolineParams(activeViz,instance);
	//Make sure this is a change:
	if (pParams->isEnabled() == val ) return;
	
	//If we are enabling, also make this the current instance:
	if (val) {
		performGuiChangeInstance(instance);
	}
	guiSetEnabled(val, instance);
	
}



void IsolineEventRouter::
isolineCenterRegion(){
	IsolineParams* pParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	VizWinMgr::getInstance()->getRegionRouter()->guiSetCenter(pParams->getSelectedPointLocal());
}
void IsolineEventRouter::
isolineCenterView(){
	IsolineParams* pParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	VizWinMgr::getInstance()->getViewpointRouter()->guiSetCenter(pParams->getSelectedPointLocal());
}
void IsolineEventRouter::
isolineCenterRake(){
	IsolineParams* pParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	FlowEventRouter* fRouter = VizWinMgr::getInstance()->getFlowRouter();
	fRouter->guiCenterRake(pParams->getSelectedPointLocal());
}

void IsolineEventRouter::
isolineAddSeed(){
	if (!DataStatus::getInstance()->getDataMgr()) return;
	Point4 pt;
	IsolineParams* pParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
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
}	

void IsolineEventRouter::
guiAxisAlign(int choice){
	if (choice == 0) return;
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "axis-align isoline");
	switch (choice) {
		case (1) : //nearest axis
			{ //align to nearest axis
				float theta = pParams->getTheta();
				//convert to closest number of quarter-turns
				int angleInt = (int)(((theta+180.)/90.)+0.5) - 2;
				theta = angleInt*90.;
				float psi = pParams->getPsi();
				angleInt = (int)(((psi+180.)/90.)+0.5) - 2;
				psi = angleInt*90.;
				float phi = pParams->getPhi();
				angleInt = (int)(((phi+180)/90.)+0.5) - 2;
				phi = angleInt*90.;
				pParams->setPhi(phi);
				pParams->setPsi(psi);
				pParams->setTheta(theta);
			}
			break;
		case (2) : //+x
			pParams->setPsi(0.f);
			pParams->setTheta(0.f);
			pParams->setPhi(-90.f);
			break;
		case (3) : //+y
			pParams->setPsi(180.f);
			pParams->setTheta(90.f);
			pParams->setPhi(-90.f);
			break;
		case (4) : //+z
			pParams->setPsi(0.f);
			pParams->setTheta(0.f);
			pParams->setPhi(180.f);
			break;
		case (5) : //-x
			pParams->setPsi(0.f);
			pParams->setTheta(0.f);
			pParams->setPhi(90.f);
			break;
		case (6) : //-y
			pParams->setPsi(180.f);
			pParams->setTheta(90.f);
			pParams->setPhi(90.f);
			break;
		case (7) : //-z
			pParams->setPsi(0.f);
			pParams->setTheta(0.f);
			pParams->setPhi(0.f);
			break;
		default:
			assert(0);
	}
	axisAlignCombo->setCurrentIndex(0);
	//Force a redraw, update tab
	updateTab();
	setIsolineDirty(pParams);
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);

}	
void IsolineEventRouter::
isolineAttachSeed(bool attach){
	if (attach) isolineAddSeed();
	FlowParams* fParams = (FlowParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_flowParamsTag);
	
	guiAttachSeed(attach, fParams);
}


void IsolineEventRouter::
setIsolineXCenter(){
	guiSetXCenter(
		xCenterSlider->value());
}
void IsolineEventRouter::
setIsolineYCenter(){
	guiSetYCenter(
		yCenterSlider->value());
}
void IsolineEventRouter::
setIsolineZCenter(){
	guiSetZCenter(
		zCenterSlider->value());
}
void IsolineEventRouter::
setIsolineXSize(){
	guiSetXSize(
		xSizeSlider->value());
}
void IsolineEventRouter::
setIsolineYSize(){
	guiSetYSize(
		ySizeSlider->value());
}




//Fit to domain extents
void IsolineEventRouter::
guiFitDomain(){
	confirmText(false);
	//int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "fit isoline to domain");

	double locextents[6];
	const float* sizes = DataStatus::getInstance()->getFullSizes();
	for (int i = 0; i<3; i++){
		locextents[i] = 0.;
		locextents[i+3] = sizes[i];
	}

	setIsolineToExtents(locextents,pParams);
	
	updateTab();
	setIsolineDirty(pParams);
	
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
}




//Reinitialize Isoline tab settings, session has changed.
//Note that this is called after the globalIsolineParams are set up, but before
//any of the localIsolineParams are setup.
void IsolineEventRouter::
reinitTab(bool doOverride){
	Session* ses = Session::getInstance();
	setEnabled(true);
	setIgnoreBoxSliderEvents(false);
	xThumbWheel->setRange(-100000,100000);
	yThumbWheel->setRange(-100000,100000);
	zThumbWheel->setRange(-100000,100000);
	xThumbWheel->setValue(0);
	yThumbWheel->setValue(0);
	zThumbWheel->setValue(0);
	
	//Set the names in the variable listbox
	ignoreComboChanges = true;
	variableCombo->clear();

	ignoreComboChanges = false;

	seedAttached = false;

	//Set up the refinement combo:
	const DataMgr* dataMgr = ses->getDataMgr();
	
	int numRefinements = dataMgr->GetNumTransforms();
	refinementCombo->setMaxCount(numRefinements+1);
	refinementCombo->clear();
	for (int i = 0; i<= numRefinements; i++){
		refinementCombo->addItem(QString::number(i));
	}
	if (dataMgr){
		vector<size_t> cRatios = dataMgr->GetCRatios();
		lodCombo->clear();
		lodCombo->setMaxCount(cRatios.size());
		for (int i = 0; i<cRatios.size(); i++){
			QString s = QString::number(cRatios[i]);
			s += ":1";
			lodCombo->addItem(s);
		}
	}
	if (histogramList) {
		for (int i = 0; i<numHistograms; i++){
			if (histogramList[i]) delete histogramList[i];
		}
		delete [] histogramList;
		histogramList = 0;
		numHistograms = 0;
	}
	
	IsolineParams* dParams = (IsolineParams*)VizWinMgr::getActiveParams(IsolineParams::_isolineParamsTag);
	//Set the names in the variable listbox
	ignoreComboChanges = true;
	variableCombo->clear();
	if (dParams->VariablesAre3D()){
		for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables3D(); i++){
			const std::string& s = DataStatus::getInstance()->getActiveVarName3D(i);
			const QString& text = QString(s.c_str());
		
			variableCombo->insertItem(i,text);
		}
	} else {
		for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables2D(); i++){
			const std::string& s = DataStatus::getInstance()->getActiveVarName2D(i);
			const QString& text = QString(s.c_str());
			variableCombo->insertItem(i,text);
		}
	}
	ignoreComboChanges = false;
	setupFidelity(3, fidelityLayout,fidelityBox, dParams, doOverride);
	connect(fidelityButtons,SIGNAL(buttonClicked(int)),this, SLOT(guiSetFidelity(int)));
	updateTab();
}



/* Handle the change of status associated with change of enablement and change
 * of local/global.  If we are enabling global, a renderer must be created in every
 * global window, including active one.  If we are enabling local, only active one is created.
 * If we change from local to global, (no change in enablement) then new renderers are
 * created for every additional global window.  Similar for disable.
 * It can occur that both enablement and local/global change, if the local and global enablement
 * are different, during a local/global change
 * This assumes that the VizWinMgr already is set with the current (new) local/global
 * Isoline settings.  
 * If the window is new, (i.e. we are just creating a new window, use: 
 * prevEnabled = false, wasLocal = isLocal = true,
 * even if the renderer is really global, since we don't want to affect other global renderers.
 */
void IsolineEventRouter::
updateRenderer(RenderParams* rParams, bool prevEnabled,   bool newWindow){

	IsolineParams* pParams = (IsolineParams*)rParams;
	VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
	
	if (newWindow) {
		prevEnabled = false;
	}
	
	//The actual enabled state of "this" :
	bool nowEnabled = pParams->isEnabled();
	
	if (prevEnabled == nowEnabled) return;
	
	VizWin* viz = 0;
	if(pParams->getVizNum() >= 0){//Find the viz that this applies to:
		//Note that this is only for the cases below where one particular
		//visualizer is needed
		viz = vizWinMgr->getVizWin(pParams->getVizNum());
	} 
	
	//5.  Change of enable->disable with unchanged global , disable all global renderers, provided the
	//   VizWinMgr already has the current local/global renderer settings
	//6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
	//  change of enable->disable with global->local.  (Must disable the local renderer)
	//  change of enable->disable with local->global (Must disable the local renderer)
	
	pParams->setEnabled(nowEnabled);

	
	if (nowEnabled && !prevEnabled ){//For case 2:  create a renderer in the active window:


		IsolineRenderer* myRenderer = new IsolineRenderer (viz->getGLWindow(), pParams);
		viz->getGLWindow()->insertSortedRenderer(pParams,myRenderer);
		VizWinMgr::getInstance()->setClutDirty(pParams);
		setIsolineDirty(pParams);
		return;
	}
	
	
	
	//case 6, disable 
	assert(prevEnabled && !nowEnabled); 
	viz->getGLWindow()->removeRenderer(pParams);
	return;
}


void IsolineEventRouter::
guiSetEnabled(bool value, int instance, bool undoredo){
	VizWinMgr* vizMgr = VizWinMgr::getInstance();
	int winnum = vizMgr->getActiveViz();
	IsolineParams* pParams = VizWinMgr::getInstance()->getIsolineParams(winnum, instance);    
	confirmText(false);
	if (value == pParams->isEnabled()) return;
	
	PanelCommand* cmd;
	if(undoredo) cmd = PanelCommand::captureStart(pParams, "toggle isoline enabled",instance);
	pParams->setEnabled(value);
	if(undoredo) PanelCommand::captureEnd(cmd, pParams);

	//Make the change in enablement occur in the rendering window, 
	// Local/Global is not changing.
	updateRenderer(pParams,!value, false);
	
	//isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
	//and refresh the gui
	updateTab();
	update();
}


//Make the probe center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void IsolineEventRouter::guiCenterProbe(){
	//confirmText(false);
	//ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	//PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center Probe at Selected Point");
	/*const float* selectedPoint = pParams->getSelectedPointLocal();
	float isolineMin[3],isolineMax[3];
	pParams->getLocalBox(isolineMin,isolineMax,-1);
	for (int i = 0; i<3; i++)
		textToSlider(pParams,i,selectedPoint[i], isolineMax[i]-isolineMin[i]);
	PanelCommand::captureEnd(cmd, pParams);
	updateTab();
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);*/

}
//Following method sets up (or releases) a connection to the Flow 
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
//Respond to an change of variable.  set the appropriate bits
void IsolineEventRouter::
guiChangeVariable(int varnum){
	//Don't react if the combo is being reset programmatically:
	if (ignoreComboChanges) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	DataStatus* ds = DataStatus::getInstance();
	int ts = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentTimestep();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "change isoline-selected variable");
	
	int activeVar = variableCombo->currentIndex();
	float minval, maxval;
	if (pParams->VariablesAre3D()){
		const string& varname = DataStatus::getActiveVarName3D(activeVar);
		minval = ds->getDataMin3D(activeVar,ts);
		maxval = ds->getDataMax3D(activeVar,ts);
		pParams->SetVariableName(varname);
	}
	else {
		const string& varname = DataStatus::getActiveVarName2D(activeVar);
		minval = ds->getDataMin2D(activeVar,ts);
		maxval = ds->getDataMax2D(activeVar,ts);
		pParams->SetVariableName(varname);
	}
	pParams->spaceIsovals(minval+0.05*(maxval - minval), minval+0.95*(maxval - minval));
	updateHistoBounds(pParams);
	PanelCommand::captureEnd(cmd, pParams);
	//Need to update the selected point for the new variables
	updateTab();
	setIsolineDirty(pParams);	
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
}
void IsolineEventRouter::
guiSetXCenter(int sliderval){
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide isoline X center");
	setXCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	setIsolineDirty(pParams);	
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
}
void IsolineEventRouter::
guiSetYCenter(int sliderval){
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide isoline Y center");
	setYCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
}
void IsolineEventRouter::
guiSetZCenter(int sliderval){
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide isoline Z center");
	setZCenter(pParams,sliderval);
	PanelCommand::captureEnd(cmd, pParams);
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);

}
void IsolineEventRouter::
guiSetXSize(int sliderval){
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide isoline X size");
	setXSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	
	resetImageSize(pParams);
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);

}
void IsolineEventRouter::
guiSetYSize(int sliderval){
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "slide isoline Y size");
	setYSize(pParams,sliderval);
	
	PanelCommand::captureEnd(cmd, pParams);
	resetImageSize(pParams);
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);

}

void IsolineEventRouter::
guiSetCompRatio(int num){
	confirmText(false);
	//make sure we are changing it
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	if (num == pParams->GetCompressionLevel()) return;
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "set compression level");
	
	pParams->SetCompressionLevel(num);
	lodCombo->setCurrentIndex(num);
	pParams->SetIgnoreFidelity(true);
	QPalette pal = QPalette(fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::gray);
	fidelityBox->setPalette(pal);
	PanelCommand::captureEnd(cmd, pParams);
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);	
}
void IsolineEventRouter::
guiSetNumRefinements(int n){
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	confirmText(false);
	int maxNumRefinements = 0;
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "set number Refinements for isoline");
	if (DataStatus::getInstance()) {
		maxNumRefinements = DataStatus::getInstance()->getNumTransforms();
		if (n > maxNumRefinements) {
			MessageReporter::warningMsg("%s","Invalid number of Refinements \nfor current data");
			n = maxNumRefinements;
			refinementCombo->setCurrentIndex(n);
		}
	} else if (n > maxNumRefinements) maxNumRefinements = n;
	pParams->SetRefinementLevel(n);
	pParams->SetIgnoreFidelity(true);
	QPalette pal = QPalette(fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::gray);
	fidelityBox->setPalette(pal);
	PanelCommand::captureEnd(cmd, pParams);
	setIsolineDirty(pParams);
	isolineImageFrame->update();
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
	
	const float* sizes = DataStatus::getInstance()->getFullSizes();
	float newCenter = ((float)slideCenter)*(sizes[coord])/256.f;
	float newSize = maxBoxSize[coord]*(float)slideSize/256.f;
	pParams->setLocalBoxMin(coord, newCenter-0.5f*newSize);
	pParams->setLocalBoxMax(coord, newCenter+0.5f*newSize);
	adjustBoxSize(pParams);
	//Set the text in the edit boxes
	mapCursor();
	const float* selectedPoint = pParams->getSelectedPointLocal();
	//Map to user coordinates
	if (!DataStatus::getInstance()->getDataMgr()) return;
	size_t timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>&userExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	newCenter += userExts[coord];
	float selectCoord = selectedPoint[coord] + userExts[coord];
	switch(coord) {
		case 0:
			xSizeEdit->setText(QString::number(newSize,'g',7));
			xCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedXLabel->setText(QString::number(selectCoord));
			break;
		case 1:
			ySizeEdit->setText(QString::number(newSize,'g',7));
			yCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedYLabel->setText(QString::number(selectCoord));
			break;
		case 2:
			zCenterEdit->setText(QString::number(newCenter,'g',7));
			selectedZLabel->setText(QString::number(selectCoord));
			break;
		default:
			assert(0);
	}
	guiSetTextChanged(false);
	resetImageSize(pParams);
	update();
	//force a new render with new Isoline data
	setIsolineDirty(pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	return;
	
}	


//Save undo/redo state when user grabs a isoline handle, or maybe a isoline face (later)
//
void IsolineEventRouter::
captureMouseDown(int){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	savedCommand = PanelCommand::captureStart(pParams,  "slide isoline handle");
	
	//Force a rerender, so we will see the selected face:
	VizWinMgr::getInstance()->refreshIsoline(pParams);
}
//The Manip class will have already changed the box?..
void IsolineEventRouter::
captureMouseUp(){
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	//float boxMin[3],boxMax[3];
	//pParams->getLocalBox(boxMin,boxMax);
	//isolineImageFrame->setTextureSize(boxMax[0]-boxMin[0],boxMax[1]-boxMin[1]);
	resetImageSize(pParams);
	setIsolineDirty(pParams);
	//Update the tab if it's in front:
	if(MainForm::getTabManager()->isFrontTab(this)) {
		VizWinMgr* vwm = VizWinMgr::getInstance();
		int viznum = vwm->getActiveViz();
		if (viznum >= 0 && (pParams == vwm->getIsolineParams(viznum)))
			updateTab();
	}
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,true);
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	
}
//When the center slider moves, set the IsolineMin and IsolineMax
void IsolineEventRouter::
setXCenter(IsolineParams* pParams,int sliderval){
	//new min and max are center -+ size/2.  
	//center is min + (slider/256)*(max-min)
	sliderToText(pParams,0, sliderval, xSizeSlider->value());
	setIsolineDirty(pParams);
}
void IsolineEventRouter::
setYCenter(IsolineParams* pParams,int sliderval){
	sliderToText(pParams,1, sliderval, ySizeSlider->value());
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
	sliderToText(pParams,0, xCenterSlider->value(),sliderval);
	setIsolineDirty(pParams);
}
void IsolineEventRouter::
setYSize(IsolineParams* pParams,int sliderval){
	sliderToText(pParams,1, yCenterSlider->value(),sliderval);
	setIsolineDirty(pParams);
}

//Save undo/redo state when user clicks cursor
//
void IsolineEventRouter::
guiStartCursorMove(){
	confirmText(false);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	savedCommand = PanelCommand::captureStart(VizWinMgr::getActiveIsolineParams(),  "move isoline cursor");
}
void IsolineEventRouter::
guiEndCursorMove(){
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	//Update the selected point:
	//If we are connected to a seed, move it:
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
	
	//Update the tab, it's in front:
	updateTab();
	if (!savedCommand) return;
	PanelCommand::captureEnd(savedCommand, pParams);
	savedCommand = 0;
	VizWinMgr::getInstance()->forceRender(pParams);
	
}
//calculate the variable, or rms of the variables, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not in isoline
//


float IsolineEventRouter::
calcCurrentValue(IsolineParams* pParams, const float point[3], int* , int ){
	double regMin[3],regMax[3];
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds ) return 0.f; 
	DataMgr* dataMgr =	ds->getDataMgr();
	if (!dataMgr) return 0.f;
	if (!pParams->isEnabled()) return 0.f;
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	if (pParams->doBypass(timeStep)) return OUT_OF_BOUNDS;
	

	for (int i = 0; i<3; i++){
		regMin[i] = point[i];
		regMax[i] = point[i];
	}
	
	//Get the data dimensions (at current resolution):
	
	int numRefinements = pParams->GetRefinementLevel();
	int lod = pParams->GetCompressionLevel();
	int sesVarNum;
	if (pParams->VariablesAre3D()){
		sesVarNum= DataStatus::getSessionVariableNum3D(pParams->GetVariableName());
		if (ds->useLowerAccuracy()){
			lod = Min(ds->maxLODPresent3D(sesVarNum, timeStep), lod);
		}
	}
	else {
		sesVarNum= DataStatus::getSessionVariableNum2D(pParams->GetVariableName());
		if (ds->useLowerAccuracy()){
			lod = Min(ds->maxLODPresent2D(sesVarNum, timeStep), lod);
		}
	}

	
	if (lod < 0) return OUT_OF_BOUNDS;
	//Find the region that contains the point
	size_t coordMin[3], coordMax[3];
	
	vector<string> varNames;
	varNames.push_back(pParams->GetVariableName());
	int actualRefLevel = RegionParams::PrepareCoordsForRetrieval(numRefinements, lod, timeStep, varNames, regMin, regMax, coordMin, coordMax);
	if (actualRefLevel < 0) return OUT_OF_BOUNDS;
	
	
	
	RegularGrid* valGrid = dataMgr->GetGrid(timeStep, varNames[0].c_str(), actualRefLevel, lod, coordMin,coordMax,0);
		
	if (!valGrid) return OUT_OF_BOUNDS;
		
	float varVal = valGrid->GetValue(point[0],point[1],point[2]);
	delete valGrid;
	return varVal;
}



	
//Method called when undo/redo changes params.  It does the following:
//  puts the new params into the vizwinmgr, deletes the old one
//  Updates the tab if it's the current instance
//  Calls updateRenderer to rebuild renderer 
//	Makes the vizwin update.
void IsolineEventRouter::
makeCurrent(Params* prevParams, Params* nextParams, bool newWin, int instance,bool) {

	
	assert(instance >= 0);
	IsolineParams* pParams = (IsolineParams*)(nextParams->deepCopy());
	int vizNum = pParams->getVizNum();
	//If we are creating one, it should be the first missing instance:
	if (!prevParams) assert(Params::GetNumParamsInstances(IsolineParams::_isolineParamsTag,vizNum) == instance);
	VizWinMgr::getInstance()->setParams(vizNum, pParams, Params::GetTypeFromTag(IsolineParams::_isolineParamsTag), instance);
	
	updateTab();
	IsolineParams* formerParams = (IsolineParams*)prevParams;
	bool wasEnabled = false;
	if (formerParams) wasEnabled = formerParams->isEnabled();
	//Check if the enabled  changed:
	if (newWin || (formerParams->isEnabled() != pParams->isEnabled())){
		updateRenderer(pParams, wasEnabled,  newWin);
	}
	VizWin* viz = VizWinMgr::getInstance()->getVizWin(vizNum);
	viz->setColorbarDirty(true);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "nudge isoline X size");
	
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
		xSizeSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge isoline X center");
	
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
		xCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge isoline Y center");
	
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
		yCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge isoline Z center");
	
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
		zCenterSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge isoline Y size");
	
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
		ySizeSlider->setValue(newSliderPos);
	}
	updateTab();
	PanelCommand::captureEnd(cmd,pParams);
	setIsolineDirty(pParams);
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
}

//The following adjusts the sliders associated with box size.
//Each slider range is the maximum of 
//(1) the domain size in the current direction
//(2) the current value of domain size.
void IsolineEventRouter::
adjustBoxSize(IsolineParams* pParams){
	
	float boxmin[3], boxmax[3];
	//Don't do anything if we haven't read the data yet:
	if (!Session::getInstance()->getDataMgr()) return;
	pParams->getLocalBox(boxmin, boxmax, -1);
	float rotMatrix[9];
	getRotationMatrix(pParams->getTheta()*M_PI/180.,pParams->getPhi()*M_PI/180., pParams->getPsi()*M_PI/180., rotMatrix);
	//Apply rotation matrix inverted to full domain size
	
	const float* extentSize = DataStatus::getInstance()->getFullSizes();
	
	//Determine the size of the domain in the direction associated with each
	//axis of the isoline.  To do this, find a unit vector in that direction.
	//The domain size in that direction is the dot product of that vector
	//with the vector that has components the domain sizes in each dimension.


	float domSize[3];
	
	for (int i = 0; i<3; i++){
		//create unit vec along axis:
		float axis[3];
		for (int j = 0; j<3; j++) axis[j] = 0.f;
		axis[i] = 1.f;
		float boxDir[3];
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
	xSizeEdit->setText(QString::number(boxmax[0]-boxmin[0]));
	ySizeEdit->setText(QString::number(boxmax[1]-boxmin[1]));
	
	//Cancel any response to text events generated in this method, to prevent
	//the sliders from triggering text change
	//
	guiSetTextChanged(false);
	xSizeSlider->setValue((int)(256.f*(boxmax[0]-boxmin[0])/(maxBoxSize[0])));
	ySizeSlider->setValue((int)(256.f*(boxmax[1]-boxmin[1])/(maxBoxSize[1])));
	
}

// Map the cursor coords into local user coord space,
// refreshing the selected point.  CursorCoords go from -1 to 1
//
void IsolineEventRouter::mapCursor(){
	//Get the transform matrix:
	float transformMatrix[12];
	float isolineCoord[3];
	float selectPoint[3];
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	pParams->buildLocalCoordTransform(transformMatrix, 0.f, -1);
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
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "crop isoline to region");
	double extents[6];
	rParams->getLocalBox(extents,extents+3,timestep);

	if (pParams->cropToBox(extents)){
		updateTab();
		setIsolineDirty(pParams);
		PanelCommand::captureEnd(cmd,pParams);
		isolineImageFrame->update();
		VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	} else {
		MessageReporter::warningMsg(" Isoline cannot be cropped to region, insufficient overlap");
		delete cmd;
	}
}
void IsolineEventRouter::
guiCropToDomain(){
	confirmText(false);
	
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "crop isoline to domain");
	const float *sizes = DataStatus::getInstance()->getFullSizes();
	double extents[6];
	for (int i = 0; i<3; i++){
		extents[i] = 0.;
		extents[i+3]=sizes[i];
	}
	if (pParams->cropToBox(extents)){
		updateTab();
		setIsolineDirty(pParams);
		PanelCommand::captureEnd(cmd,pParams);
		isolineImageFrame->update();
		VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	} else {
		MessageReporter::warningMsg(" Isoline cannot be cropped to domain, insufficient overlap");
		delete cmd;
	}
}
//Fill to domain extents
void IsolineEventRouter::
guiFitRegion(){
	confirmText(false);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	RegionParams* rParams = VizWinMgr::getActiveRegionParams();
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "fit isoline to region");
	double extents[6];
	rParams->getLocalBox(extents,extents+3,timestep);
	setIsolineToExtents(extents,pParams);
	updateTab();
	setIsolineDirty(pParams);
	PanelCommand::captureEnd(cmd,pParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
}
void IsolineEventRouter::
setIsolineToExtents(const double extents[6], IsolineParams* pParams){
	
	//First try to fit to extents.  If we fail, then move isoline to fit 
	bool success = pParams->fitToBox(extents);
	if (success) return;

	//Move the isoline so that it is centered in the extents:
	double pExts[6];
	pParams->GetBox()->GetLocalExtents(pExts);
	
	for (int i = 0; i<3; i++){
		double psize = pExts[i+3]-pExts[i];
		pExts[i] = 0.5*(extents[i]+extents[i+3] - psize);
		pExts[i+3] = 0.5*(extents[i]+extents[i+3] + psize);
	}
	pParams->GetBox()->SetLocalExtents(pExts);
	success = pParams->fitToBox(extents);
	assert(success);
	
	return;
}
//Angle conversions (in degrees)
double IsolineEventRouter::convertRotStretchedToActual(int axis, double angle){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
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

void IsolineEventRouter::
showHideLayout(){
	if (showLayout) {
		showLayout = false;
		showHideLayoutButton->setText("Show Isoline Layout Options");
	} else {
		showLayout = true;
		showHideLayoutButton->setText("Hide Isoline Layout Options");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(IsolineParams::_isolineParamsTag));
	updateTab();
}
void IsolineEventRouter::
showHideAppearance(){
	if (showAppearance) {
		showAppearance = false;
		showHideAppearanceButton->setText("Show Isoline Appearance Options");
	} else {
		showAppearance = true;
		showHideAppearanceButton->setText("Hide Isoline Appearance Options");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(IsolineParams::_isolineParamsTag));
	updateTab();
}
void IsolineEventRouter::
showHideImage(){
	if (showImage) {
		showImage = false;
		showHideImageButton->setText("Show Isoline Image Settings");
	} else {
		showImage = true;
		showHideImageButton->setText("Hide Isoline Image Settings");
	}
	//Following HACK is needed to convince Qt to remove the extra space in the tab:
	updateTab();
	VizWinMgr::getInstance()->getTabManager()->toggleFrontTabs(Params::GetTypeFromTag(IsolineParams::_isolineParamsTag));
	updateTab();
}
//Workaround for Qt/Cocoa bug: postpone showing of OpenGL widgets 

#ifdef Darwin
void IsolineEventRouter::paintEvent(QPaintEvent* ev){
	
		
		//First show the texture frame, 
		//Other order doesn't work.
		if(!texShown ){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
			QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
			sArea->ensureWidgetVisible(isolineFrameHolder);
			texShown = true;
#endif
			isolineImageFrame->show();
			isolineImageFrame->updateGeometry();
			QWidget::paintEvent(ev);
			return;
		} 
		QWidget::paintEvent(ev);
		return;
}

#endif
QSize IsolineEventRouter::sizeHint() const {
	IsolineParams* pParams = (IsolineParams*) VizWinMgr::getActiveIsolineParams();
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
	IsolineParams* dParams = (IsolineParams*)VizWinMgr::getActiveParams(IsolineParams::_isolineParamsTag);
	int newLOD = fidelityLODs[buttonID];
	int newRef = fidelityRefinements[buttonID];
	int lodSet = dParams->GetCompressionLevel();
	int refSet = dParams->GetRefinementLevel();
	if (lodSet == newLOD && refSet == newRef) return;
	int fidelity = buttonID;
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Set Data Fidelity");
	dParams->SetCompressionLevel(newLOD);
	dParams->SetRefinementLevel(newRef);
	dParams->SetFidelityLevel(fidelity);
	dParams->SetIgnoreFidelity(false);
	QPalette pal = QPalette(fidelityBox->palette());
	pal.setColor(QPalette::WindowText, Qt::black);
	fidelityBox->setPalette(pal);
	//change values of LOD and refinement combos using setCurrentIndex().
	lodCombo->setCurrentIndex(newLOD);
	refinementCombo->setCurrentIndex(newRef);
	PanelCommand::captureEnd(cmd, dParams);
	setIsolineDirty(dParams);
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(dParams);	
}

//User clicks on SetDefault button, need to make current fidelity settings the default.
void IsolineEventRouter::guiSetFidelityDefault(){
	//Check current values of LOD and refinement and their combos.
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if (!dataMgr) return;
	confirmText(false);
	IsolineParams* dParams = (IsolineParams*)VizWinMgr::getActiveParams(IsolineParams::_isolineParamsTag);
	UserPreferences *prePrefs = UserPreferences::getInstance();
	PreferencesCommand* pcommand = PreferencesCommand::captureStart(prePrefs, "Set Fidelity Default Preference");

	setFidelityDefault(3,dParams);
	
	UserPreferences *postPrefs = UserPreferences::getInstance();
	PreferencesCommand::captureEnd(pcommand,postPrefs);
	delete prePrefs;
	delete postPrefs;
	updateTab();
}
void IsolineEventRouter::resetImageSize(IsolineParams* iParams){
	//setup the texture:
	float voxDims[2];
	iParams->getRotatedVoxelExtents(voxDims);
	isolineImageFrame->setImageSize(voxDims[0],voxDims[1]);
}
/*
 * Respond to user clicking a color button
 */


void IsolineEventRouter::guiSetPanelBackgroundColor(){
	QPalette pal(isolinePanelBackgroundColorEdit->palette());
	QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);
	if (!newColor.isValid()) return;
	pal.setColor(QPalette::Base, newColor);
	isolinePanelBackgroundColorEdit->setPalette(pal);
	//Set parameter value of the appropriate parameter set:
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getActiveParams(IsolineParams::_isolineParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "set panel background color");
	float clr[3];
	clr[0]=(newColor.red()/256.);
	clr[1]=(newColor.green()/256.);
	clr[2]=(newColor.blue()/256.);
	iParams->SetPanelBackgroundColor(clr);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
}

void IsolineEventRouter::guiSetDimension(int dim){
	
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getActiveParams(IsolineParams::_isolineParamsTag);
	int curdim = iParams->VariablesAre3D() ? 1 : 0 ;
	if (curdim == dim) return;
	//Don't allow setting of dimension of there aren't any variables in the proposed dimension:
	if (dim == 1 && DataStatus::getInstance()->getNumActiveVariables3D()==0){
		dimensionCombo->setCurrentIndex(0);
		return;
	}
	if (dim == 0 && DataStatus::getInstance()->getNumActiveVariables2D()==0){
		dimensionCombo->setCurrentIndex(1);
		return;
	}
	PanelCommand* cmd = PanelCommand::captureStart(iParams,  "set isoline variable dimension");
	
	//the combo is either 0 or 1 for dimension 2 or 3.
	iParams->SetVariables3D(dim == 1);
	//Put the appropriate variable names into the combo
	//Set the names in the variable listbox.
	//Set the current variable name to be the first variable
	ignoreComboChanges = true;
	variableCombo->clear();
	if (dim == 1){ //dim = 1 for 3D vars
		for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables3D(); i++){
			const std::string& s = DataStatus::getInstance()->getActiveVarName3D(i);
			if (i == 0) iParams->SetVariableName(s);
			const QString& text = QString(s.c_str());
			variableCombo->insertItem(i,text);
		}
		copyToProbeButton->setText("Copy to Probe");
		copyToProbeButton->setToolTip("Click to make the current active Probe display these isolines as a color contour plot");
	} else {
		for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables2D(); i++){
			const std::string& s = DataStatus::getInstance()->getActiveVarName2D(i);
			if (i == 0) iParams->SetVariableName(s);
			const QString& text = QString(s.c_str());
			variableCombo->insertItem(i,text);
		}
		copyToProbeButton->setText("Copy to 2D");
		copyToProbeButton->setToolTip("Click to make the current active 2D Data display these isolines as a color contour plot");
	}
	if (showLayout) {
		if (dim == 1) orientationFrame->show();
		else orientationFrame->hide();
	}
	ignoreComboChanges = false;
	PanelCommand::captureEnd(cmd, iParams);
	setIsolineDirty(iParams);

}
void IsolineEventRouter::invalidateRenderer(IsolineParams* iParams)
{
	IsolineRenderer* iRender = (IsolineRenderer*)VizWinMgr::getInstance()->getActiveVisualizer()->getGLWindow()->getRenderer(iParams);
	if (iRender) iRender->invalidateLineCache();
}

void IsolineEventRouter::
setIsolineEditMode(bool mode){
	navigateButton->setChecked(!mode);
	editMode = mode;
}
void IsolineEventRouter::
setIsolineNavigateMode(bool mode){
	editButton->setChecked(!mode);
	editMode = !mode;
}
void IsolineEventRouter::
refreshHisto(){
	VizWin* vizWin = VizWinMgr::getInstance()->getActiveVisualizer();
	if (!vizWin) return;
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	if (iParams->doBypass(VizWinMgr::getActiveAnimationParams()->getCurrentTimestep())){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE,"Unable to refresh histogram");
		return;
	}
	
	DataMgr* dataManager = Session::getInstance()->getDataMgr();
	if (dataManager) {
		float bnds[2];
		iParams->GetHistoBounds(bnds);
		refreshHistogram(iParams,iParams->getSessionVarNum(),bnds);
	}
	setEditorDirty(iParams);
}
void IsolineEventRouter::guiSetAligned(){
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "fit iso selection window to view");
	setEditorDirty(iParams);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
}
void IsolineEventRouter::
guiStartChangeIsoSelection(QString qstr){
	//If text has changed, and enter not pressed, will ignore it-- don't call confirmText()!
	guiSetTextChanged(false);
	//If another command is in process, don't disturb it:
	if (savedCommand) return;
	//Save the previous isovalue max and min
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	const vector<double>& ivalues = iParams->GetIsovalues();
	prevIsoMin = 1.e30, prevIsoMax = -1.e30;
	for (int i = 0; i<ivalues.size(); i++){
		if (prevIsoMin > ivalues[i]) prevIsoMin = ivalues[i];
		if (prevIsoMax < ivalues[i]) prevIsoMax = ivalues[i]; 
	}
    savedCommand = PanelCommand::captureStart(iParams, qstr.toLatin1());
}
//This will set dirty bits and undo/redo changes to histo bounds and eventually iso value
void IsolineEventRouter::
guiEndChangeIsoSelection(){
	if (!savedCommand) return;
	
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
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
		//If the new values are not inside the histo bounds, respecify the bounds
		float newHistoBounds[2];
		if (minIso <= bnds[0] || maxIso >= bnds[1]){
			newHistoBounds[0]=maxIso - 1.1*(maxIso-minIso);
			newHistoBounds[1]=minIso + 1.1*(maxIso-minIso);
			iParams->SetHistoBounds(newHistoBounds);
		}
	}
	
	
	PanelCommand::captureEnd(savedCommand,iParams);
	savedCommand = 0;
	updateTab();
	//force redraw with changed isovalues
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
}
//Set isoControl editor  dirty.
void IsolineEventRouter::
setEditorDirty(RenderParams* p){
	IsolineParams* ip = (IsolineParams*)p;
	if(!ip) ip = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
    isoSelectionFrame->setMapperFunction(ip->GetIsoControl());
	isoSelectionFrame->setVariableName(ip->GetVariableName());
	isoSelectionFrame->setIsoValue(ip->GetIsovalues()[0]);
    isoSelectionFrame->updateParams();
	
    Session *session = Session::getInstance();

	if (session->getNumSessionVariables()){
		
		const std::string& varname = ip->GetVariableName();
		isoSelectionFrame->setVariableName(varname);
		
	} else {
		isoSelectionFrame->setVariableName("N/A");
	}
	if(ip) {
		
	}
	isoSelectionFrame->update();
	
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
	int viznum = iParams->getVizNum();
	if (viznum < 0) return;
	int currentTimeStep = VizWinMgr::getInstance()->getAnimationParams(viznum)->getCurrentTimestep();
	DataStatus* ds = DataStatus::getInstance();
	int varnum;
	float minval, maxval;
	if (iParams->VariablesAre3D()){
		varnum = ds->getActiveVarNum3D(iParams->GetVariableName());
		if (iParams->isEnabled()){
			minval = ds->getDataMin3D(varnum, currentTimeStep);
			maxval = ds->getDataMax3D(varnum, currentTimeStep);
		} else {
			minval = ds->getDefaultDataMin3D(varnum);
			maxval = ds->getDefaultDataMax3D(varnum);
		}

	} else {
		varnum = ds->getActiveVarNum2D(iParams->GetVariableName());
		if (iParams->isEnabled()){
			minval = ds->getDataMin2D(varnum, currentTimeStep);
			maxval = ds->getDataMax2D(varnum, currentTimeStep);
		} else {
			minval = ds->getDefaultDataMin2D(varnum);
			maxval = ds->getDefaultDataMax2D(varnum);
		}
	}
	minDataBound->setText(strn.setNum(minval));
	maxDataBound->setText(strn.setNum(maxval));
	
}

//Obtain a new histogram for the current selected variables.
//Save it at the position associated with firstVarNum
void IsolineEventRouter::
refreshHistogram(RenderParams* p, int sesvarnum, const float drange[2]){
	IsolineParams* pParams = (IsolineParams*)p;
	bool is3D = pParams->VariablesAre3D();
	if (!is3D) {
		refresh2DHistogram(p,sesvarnum,drange);
		return;
	}
	int firstVarNum = pParams->getSessionVarNum();
	const float* currentDatarange = pParams->getCurrentDatarange();
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	int numVariables;
	if (is3D) numVariables = ds->getNumSessionVariables();
	else numVariables = ds->getNumSessionVariables2D();

	if(pParams->doBypass(timeStep)) return;
	if (!histogramList){
		histogramList = new Histo*[numVariables];
		numHistograms = numVariables;
		for (int i = 0; i<numVariables; i++)
			histogramList[i] = 0;
	}
	if (!histogramList[firstVarNum]){
		histogramList[firstVarNum] = new Histo(256,currentDatarange[0],currentDatarange[1]);
	}
	Histo* histo = histogramList[firstVarNum];
	histo->reset(256,currentDatarange[0],currentDatarange[1]);
	pParams->calcSliceHistogram(timeStep, histo);
}
void IsolineEventRouter::guiFitToData(){
	
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return;
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "fit histo bounds to data");
	//Get bounds from DataStatus:
	int ts = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	bool is3D = iParams->VariablesAre3D();

	int sesvarnum = iParams->getSessionVarNum(); 

	 
	float minval, maxval;
	if (is3D){
		minval = ds->getDataMin3D(sesvarnum, ts);
		maxval = ds->getDataMax3D(sesvarnum, ts);
	} else {
		minval = ds->getDataMin2D(sesvarnum, ts);
		maxval = ds->getDataMax2D(sesvarnum, ts);
	}
	
	if (minval > maxval){ //no data
		maxval = 1.f;
		minval = 0.f;
	}
	
	iParams->GetIsoControl()->setMinHistoValue(minval);
	iParams->GetIsoControl()->setMaxHistoValue(maxval);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
	
}
//Fit the isovals into current histo window.
void IsolineEventRouter::guiFitIsovalsToHisto(){
	confirmText(false);
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "fit isovalues to histo bounds");
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
		//Now rearrange proportionately to fit in center 90% of bounds
		float min90 = bounds[0]+0.05*(bounds[1]-bounds[0]);
		float max90 = bounds[0]+0.95*(bounds[1]-bounds[0]);
		for (int i = 0; i<isovals.size(); i++){
			double newIsoval = min90 + (max90-min90)*(isovals[i]-isoMin)/(isoMax-isoMin);
			newIsovals.push_back(newIsoval);
		}
	}
	iParams->SetIsovalues(newIsovals);
	PanelCommand::captureEnd(cmd, iParams);
	updateTab();
}
void IsolineEventRouter::copyToProbeOr2D(){
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	if (iParams->VariablesAre3D()) guiCopyToProbe();
	else guiCopyTo2D();
}
void IsolineEventRouter::guiCopyToProbe(){
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	ProbeParams* pParams = (ProbeParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_probeParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Copy isoline setup to Probe");
	//Copy the Box
	const vector<double>& exts = iParams->GetBox()->GetLocalExtents();
	const vector<double>& angles = iParams->GetBox()->GetAngles();
	pParams->GetBox()->SetAngles(angles);
	pParams->GetBox()->SetLocalExtents(exts);
	//set the variable:
	const std::string& varname = iParams->GetVariableName();
	int sesvarnum = DataStatus::getInstance()->getSessionVariableNum3D(varname);
	pParams->setVariableSelected(sesvarnum,true);
	//Modify the TransferFunction
	TransferFunction* tf = pParams->GetTransFunc();
	
	convertIsovalsToColors(tf);
	pParams->setProbeDirty();
	PanelCommand::captureEnd(cmd,pParams);
}
void IsolineEventRouter::guiCopyTo2D(){
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	TwoDDataParams* pParams = (TwoDDataParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_twoDDataParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Copy isoline setup to 2D Data");
	//Copy the Box
	const vector<double>& exts = iParams->GetBox()->GetLocalExtents();
	pParams->GetBox()->SetLocalExtents(exts);
	const std::string& varname = iParams->GetVariableName();
	int sesvarnum = DataStatus::getInstance()->getSessionVariableNum2D(varname);
	pParams->setVariableSelected(sesvarnum,true);
	//Modify the TransferFunction
	TransferFunction* tf = pParams->GetTransFunc();
	convertIsovalsToColors(tf);
	pParams->setTwoDDirty();
	PanelCommand::captureEnd(cmd,pParams);
}
void IsolineEventRouter::convertIsovalsToColors(TransferFunction* tf){
	IsolineParams* iParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	vector<double> isovals = iParams->GetIsovalues();//copy the isovalues from the params
	ColorMapBase* cmap = tf->getColormap();
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
		leftHistoEdit->setText(strn.setNum(mpFunc->getMinColorMapValue(),'g',4));
		rightHistoEdit->setText(strn.setNum(mpFunc->getMaxColorMapValue(),'g',4));
	} 
	setEditorDirty(iParams);

}
//Respond to user click on save/load TF.  This launches the intermediate
//dialog, then sends the result to the DVR params
void IsolineEventRouter::
isolineSaveTF(void){
	IsolineParams* dParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	saveTF(dParams);
}

void IsolineEventRouter::
isolineLoadInstalledTF(){
	IsolineParams* dParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	IsoControl* tf = dParams->GetIsoControl();
	if (!tf) return;
	float minb = tf->getMinColorMapValue();
	float maxb = tf->getMaxColorMapValue();
	if (minb >= maxb){ minb = 0.0; maxb = 1.0;}
	loadInstalledTF(dParams,0);
	tf = dParams->GetIsoControl();
	tf->setMinHistoValue(minb);
	tf->setMaxHistoValue(maxb);
	setEditorDirty(dParams);
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
void IsolineEventRouter::
isolineLoadTF(void){
	//If there are no TF's currently in Session, just launch file load dialog.
	IsolineParams* dParams = (IsolineParams*)VizWinMgr::getInstance()->getApplicableParams(IsolineParams::_isolineParamsTag);
	loadTF(dParams,dParams->getSessionVarNum());
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
//Respond to user request to load/save TF
//Assumes name is valid
//
void IsolineEventRouter::
sessionLoadTF(QString* name){
	IsolineParams* dParams = VizWinMgr::getActiveIsolineParams();
	
	confirmText(false);
	
	PanelCommand* cmd = PanelCommand::captureStart(dParams, "Load Transfer Function from Session");
	
	//Get the transfer function from the session:
	
	std::string s(name->toStdString());
	TransferFunction* tf = Session::getInstance()->getTF(&s);
	assert(tf);
	int varNum = dParams->getSessionVarNum();
	dParams->hookupTF(tf, varNum);
	PanelCommand::captureEnd(cmd, dParams);
	setEditorDirty(dParams);
	
	VizWinMgr::getInstance()->setClutDirty(dParams);
}
//Launch an editor on the isovalues
void IsolineEventRouter::guiEditIsovalues(){
	IsolineParams* iParams = VizWinMgr::getActiveIsolineParams();
	confirmText(false);
	PanelCommand* cmd = PanelCommand::captureStart(iParams, "Edit Isovalues");
	
	IsovalueEditor sle(iParams->getNumIsovalues(), iParams);
	if (!sle.exec()){
		delete cmd;
		return;
	}
	vector<double>isovals = iParams->GetIsovalues();
	std::sort(isovals.begin(),isovals.end());
	iParams->SetIsovalues(isovals);
	PanelCommand::captureEnd(cmd, iParams);
	setIsolineDirty(iParams);
	VizWinMgr::getInstance()->forceRender(iParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	updateTab();
}