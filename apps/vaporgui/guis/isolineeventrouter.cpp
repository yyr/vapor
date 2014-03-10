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
//#include "isolinerenderer.h"
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
#include "tabmanager.h"
#include "isolineparams.h"
#include "isolinerenderer.h"
#include "isolineeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"

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
	fidelityButtons = 0;
	savedCommand = 0;
	ignoreComboChanges = false;

	seedAttached = false;
	showAppearance = true;
	showLayout = false;
	showImage = true;
	lastXSizeSlider = 256;
	lastYSizeSlider = 256;
	lastXCenterSlider = 128;
	lastYCenterSlider = 128;
	lastZCenterSlider = 128;

	
	for (int i = 0; i<3; i++)maxBoxSize[i] = 1.f;
	MessageReporter::infoMsg("IsolineEventRouter::IsolineEventRouter()");

#if defined(Darwin) && (QT_VERSION < QT_VERSION_CHECK(4,8,0))
	opacityMapShown = false;
	texShown = false;
	isolineImageFrame->hide();
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
	
	connect (fidelityDefaultButton, SIGNAL(clicked()), this, SLOT(guiSetFidelityDefault()));

	connect (xCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (yCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (zCenterEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (xSizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (ySizeEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (thetaEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (phiEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	connect (psiEdit, SIGNAL(returnPressed()), this, SLOT(isolineReturnPressed()));
	
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
	connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
	connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
	connect (rotate90Combo,SIGNAL(activated(int)), this, SLOT(guiRotate90(int)));
	connect (variableCombo,SIGNAL(activated(int)), this, SLOT(guiChangeVariable(int)));
	connect (xCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineXCenter()));
	connect (yCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineYCenter()));
	connect (zCenterSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineZCenter()));
	connect (xSizeSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineXSize()));
	connect (ySizeSlider, SIGNAL(sliderReleased()), this, SLOT (setIsolineYSize()));

	connect (captureButton, SIGNAL(clicked()), this, SLOT(captureImage()));
	

	connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
	connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
	connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
	connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
	connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setIsolineEnabled(bool,int)));
	
	connect (showHideLayoutButton, SIGNAL(pressed()), this, SLOT(showHideLayout()));
	connect (showHideImageButton, SIGNAL(pressed()), this, SLOT(showHideImage()));
	connect (showHideAppearanceButton, SIGNAL(pressed()), this, SLOT(showHideAppearance()));
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

	if (ds->getDataMgr() && ds->dataIsPresent3D() ) instanceTable->setEnabled(true);
	else return;
	instanceTable->rebuild(this);
	
	IsolineParams* isolineParams = VizWinMgr::getActiveIsolineParams();
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
	int sesVarNum = ds->getSessionVariableNum3D(isolineParams->GetVariableName());
	
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
	ignoreComboChanges = false;


	isolineImageFrame->setParams(isolineParams);
	isolineImageFrame->update();
	if (showLayout) layoutFrame->show();
	else layoutFrame->hide();
	if (showAppearance) appearanceFrame->show();
	else appearanceFrame->hide();
	if (showImage) imageFrame->show();
	else imageFrame->hide();
	adjustSize();
	update();
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

	if (!DataStatus::getInstance()->getDataMgr()) return;

	size_t timestep = (size_t)VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	const vector<double>& userExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Set the isoline size based on current text box settings:
	float boxSize[3], boxmin[3], boxmax[3], boxCenter[3];
	boxSize[0] = xSizeEdit->text().toFloat();
	boxSize[1] = ySizeEdit->text().toFloat();
	boxSize[2] = zSizeEdit->text().toFloat();
	for (int i = 0; i<3; i++){
		if (boxSize[i] < 0.f) boxSize[i] = 0.f;
	}
	//Convert text to local extents:
	boxCenter[0] = xCenterEdit->text().toFloat()- userExts[0];
	boxCenter[1] = yCenterEdit->text().toFloat()- userExts[1];
	boxCenter[2] = zCenterEdit->text().toFloat()- userExts[2];
	const float* fullSizes = DataStatus::getInstance()->getFullSizes();
	for (int i = 0; i<3;i++){
		//No longer constrain the box to have center in the domain:
		
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
	//isolineImageFrame->setTextureSize(voxDims[0],voxDims[1]);
	setIsolineDirty(isolineParams);
	
	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(isolineParams,GLWindow::getCurrentMouseMode() == GLWindow::isolineMode);
	
	//Cancel any response to events generated in this method:
	//
	guiSetTextChanged(false);
	PanelCommand::captureEnd(cmd, isolineParams);
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
	for (int i = 0; i< DataStatus::getInstance()->getNumActiveVariables3D(); i++){
		const std::string& s = DataStatus::getInstance()->getActiveVarName3D(i);
		const QString& text = QString(s.c_str());
		
		variableCombo->insertItem(i,text);
	}
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
	setDatarangeDirty(pParams);
	

	isolineImageFrame->update();
	VizWinMgr::getInstance()->forceRender(pParams);
	//and refresh the gui
	updateTab();
	update();
}


//Make the probe center at selectedPoint.  Shrink size if necessary.
//Reset sliders and text as appropriate.  Equivalent to typing in the values
void IsolineEventRouter::guiCenterProbe(){
	confirmText(false);
	ProbeParams* pParams = VizWinMgr::getActiveProbeParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "Center Probe at Selected Point");
	const float* selectedPoint = pParams->getSelectedPointLocal();
	float isolineMin[3],isolineMax[3];
	/*pParams->getLocalBox(isolineMin,isolineMax,-1);
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
	if (!DataStatus::getInstance()->dataIsPresent3D()) return;
	confirmText(false);
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	PanelCommand* cmd = PanelCommand::captureStart(pParams, "change isoline-selected variable");
	
	int activeVar = variableCombo->currentIndex();
	const string& varname = DataStatus::getActiveVarName3D(activeVar);
	pParams->SetVariableName(varname);
	
	PanelCommand::captureEnd(cmd, pParams);
	//Need to update the selected point for the new variables
	updateTab();
	
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
	//setup the texture:
	resetImageSize(pParams);
	
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
			zSizeEdit->setText(QString::number(newSize,'g',7));
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
	sliderToText(pParams,2, sliderval, zSizeSlider->value());
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
	int sesVarNum = DataStatus::getSessionVariableNum3D(pParams->GetVariableName());
	
	if (ds->useLowerAccuracy()){
		lod = Min(ds->maxLODPresent3D(sesVarNum, timeStep), lod);
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

//Capture just one image
//Launch a file save dialog to specify the names
//Then put jpeg in it.
//
void IsolineEventRouter::captureImage() {
	MainForm::getInstance()->showCitationReminder();
	QString filename = QFileDialog::getSaveFileName(this,
		"Specify image capture Jpeg file name",
		Session::getInstance()->getJpegDirectory().c_str(),
		"Jpeg Images (*.jpg)");
	if(filename == QString("")) return;
	
	//Extract the path, and the root name, from the returned string.
	QFileInfo* fileInfo = new QFileInfo(filename);
	
	if (fileInfo->exists()){
		int rc = QMessageBox::warning(0, "File Exists", QString("OK to replace existing jpeg file?\n %1 ").arg(filename), QMessageBox::Ok, 
			QMessageBox::No);
		if (rc != QMessageBox::Ok) return;
	}
	//Save the path for future captures
	Session::getInstance()->setJpegDirectory((const char*)fileInfo->absolutePath().toAscii());
	if (!filename.endsWith(".jpg")) filename += ".jpg";
	//
	
	IsolineParams* pParams = VizWinMgr::getActiveIsolineParams();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	int imgSize[2];
	float voxDims[3];
	pParams->getRotatedVoxelExtents(voxDims);
	
	int wid = (int) voxDims[0];
	int ht = (int)voxDims[1];
	unsigned char* isolineTex;
	unsigned char* buf;
	 
		//Determine the image size.  Start with the texture dimensions in 
		// the scene, then increase x or y to make the aspect ratio match the
		// aspect ratio in the scene
		float aspRatio = pParams->getRealImageHeight()/pParams->getRealImageWidth();
	
		//Make sure the image is at least 256x256
		if (wid < 256) wid = 256;
		if (ht < 256) ht = 256;
		float imAspect = (float)ht/(float)wid;
		if (imAspect > aspRatio){
			//want ht/wid = asp, but ht/wid > asp; make wid larger:
			wid = (int)(0.5f+ (imAspect/aspRatio)*(float)wid);
		} else { //Make ht larger:
			ht = (int) (0.5f + (aspRatio/imAspect)*(float)ht);
		}
		//Construct the isoline image of the desired dimensions:
		//buf = pParams->calcIsolineImage(timestep,wid,ht);

	//Construct an RGB image from this.  Ignore alpha.
	//invert top and bottom while removing alpha component
	isolineTex = new unsigned char[3*wid*ht];
	for (int j = 0; j<ht; j++){
		for (int i = 0; i< wid; i++){
			for (int k = 0; k<3; k++)
				isolineTex[k+3*(i+wid*j)] = buf[k+4*(i+wid*(ht-j-1))];
		}
	}
		

	
	//Now open the jpeg file:
	FILE* jpegFile = fopen((const char*)filename.toAscii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: Error opening \noutput Jpeg file: \n%s",(const char*)filename.toAscii());
		return;
	}
	//Now call the Jpeg library to compress and write the file
	//
	int quality = GLWindow::getJpegQuality();
	int rc = write_JPEG_file(jpegFile, wid, ht, isolineTex, quality);
	delete [] isolineTex;
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; \nError writing jpeg file \n%s",
			(const char*)filename.toAscii());
		delete [] buf;
		return;
	}
	//Provide a message stating the capture in effect.
	MessageReporter::infoMsg("Image is captured to %s",
			(const char*)filename.toAscii());
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
	
	PanelCommand* cmd = PanelCommand::captureStart(pParams,  "nudge isoline X size");
	
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
	zSizeEdit->setText(QString::number(boxmax[2]-boxmin[2]));
	//Cancel any response to text events generated in this method, to prevent
	//the sliders from triggering text change
	//
	guiSetTextChanged(false);
	xSizeSlider->setValue((int)(256.f*(boxmax[0]-boxmin[0])/(maxBoxSize[0])));
	ySizeSlider->setValue((int)(256.f*(boxmax[1]-boxmin[1])/(maxBoxSize[1])));
	zSizeSlider->setValue((int)(256.f*(boxmax[2]-boxmin[2])/(maxBoxSize[2])));
	
	
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
	
		
		//First show the texture frame, next time through, show the tf frame
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
		if (!opacityMapShown){
#if (QT_VERSION < QT_VERSION_CHECK(4,8,0))
			QScrollArea* sArea = (QScrollArea*)MainForm::getTabManager()->currentWidget();
			sArea->ensureWidgetVisible(tfFrame);
			opacityMapShown = true;
#endif
			transferFunctionFrame->show();
		} 
		QWidget::paintEvent(ev);
		return;
}

#endif
QSize IsolineEventRouter::sizeHint() const {
	IsolineParams* pParams = (IsolineParams*) VizWinMgr::getActiveIsolineParams();
	if (!pParams) return QSize(460,1500);
	int vertsize = 230;//basic panel plus instance panel 
	//add showAppearance button, showLayout button, showLayout button, frames
	vertsize += 150;
	if (showLayout) {
		vertsize += 544;
		if(DataStatus::getProjectionString().size() == 0) vertsize -= 56;  //no lat long
	}
	if (showImage){
		vertsize += 687;
	}
	if (showAppearance) vertsize += 445;  //Add in appearance panel 
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
	isolineImageFrame->setTextureSize(voxDims[0],voxDims[1]);
}