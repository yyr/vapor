//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		viewpointeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2006
//
//	Description:	Implements the ViewpointsEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the viewpoint tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qlineedit.h>
#include <qcheckbox.h>

#include <qcombobox.h>
#include <qlabel.h>
#include <qfiledialog.h>
#include <qmessagebox.h>
#include "viewpointeventrouter.h"
#include "viewpointparams.h"
#include "viztab.h"
#include "tabmanager.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include "params.h"
using namespace VAPoR;

ViewpointEventRouter::ViewpointEventRouter(QWidget* parent ): QWidget(parent), Ui_VizTab(), EventRouter(){
	setupUi(this);
	myParamsBaseType = Params::GetTypeFromTag(Params::_viewpointParamsTag);
	savedCommand = 0;
	panChanged = false;
	for (int i = 0; i<3; i++)lastCamPos[i] = 0.f;

#ifdef TEST_KEYFRAMING
	viewpointOutputFile = 0;
	keyframeSpeed = 1.0;

	ViewpointParams::clearLoadedViewpoints();
#endif

	MessageReporter::infoMsg("ViewpointEventRouter::ViewpointEventRouter()");
}


ViewpointEventRouter::~ViewpointEventRouter(){
	if (savedCommand) delete savedCommand;
#ifdef TEST_KEYFRAMING
	if (viewpointOutputFile)fclose(viewpointOutputFile);
#endif
}
/**********************************************************
 * Whenever a new viztab is created it must be hooked up here
 ************************************************************/
void
ViewpointEventRouter::hookUpTab()
{
	
	connect (stereoCombo, SIGNAL (activated(int)), this, SLOT (guiSetStereoMode(int)));
	connect (latLonCheckbox, SIGNAL (toggled(bool)), this, SLOT(guiToggleLatLon(bool)));
	connect (camPosLat, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPosLon, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenterLat, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenterLon, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (numLights, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos00, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos01, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos02, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos10, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos11, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos12, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos20, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos21, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightPos22, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPos0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPos1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (camPos2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (viewDir0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (viewDir1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (viewDir2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (upVec0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (upVec1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (upVec2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenter0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenter1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (rotCenter2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightDiff0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightDiff1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightDiff2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightSpec0, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightSpec1, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (lightSpec2, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (shininessEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (ambientEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	connect (stereoSeparationEdit, SIGNAL( textChanged(const QString&) ), this, SLOT( setVtabTextChanged(const QString&)));
	
	//Connect all the returnPressed signals, these will update the visualizer.
	connect (camPosLat, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (camPosLon, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (rotCenterLat, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (rotCenterLon, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos00, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos01, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos02, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos10, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos11, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos12, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos20, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos21, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightPos22, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightDiff0, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightDiff1, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightDiff2, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightSpec0, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightSpec1, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (lightSpec2, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (shininessEdit, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	connect (ambientEdit, SIGNAL( returnPressed()), this, SLOT(viewpointReturnPressed()));
	
	connect (camPos0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (camPos1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (camPos2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (viewDir0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (viewDir1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (viewDir2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (upVec0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (upVec1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (upVec2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (rotCenter0, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (rotCenter1, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (rotCenter2, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (numLights, SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed()));
	connect (stereoSeparationEdit,SIGNAL( returnPressed()) , this, SLOT(viewpointReturnPressed())); 

	connect (LocalGlobal, SIGNAL (activated (int)), VizWinMgr::getInstance(), SLOT (setVpLocalGlobal(int)));
	connect (VizWinMgr::getInstance(), SIGNAL(enableMultiViz(bool)), LocalGlobal, SLOT(setEnabled(bool)));

#ifdef TEST_KEYFRAMING
	int rc = connect (writeViewpointButton, SIGNAL (clicked()), this, SLOT(writeKeyframe()));
	rc = connect (speedEdit, SIGNAL (returnPressed()), this, SLOT(changeKeyframeSpeed()));
	rc = connect (readViewpointsButton, SIGNAL (clicked()), this, SLOT(readKeyframes()));
#endif

}

/*********************************************************************************
 * Slots associated with ViewpointTab:
 *********************************************************************************/

void ViewpointEventRouter::
setVtabTextChanged(const QString& ){
	guiSetTextChanged(true);
}
//Put all text changes into the params
void ViewpointEventRouter::confirmText(bool /*render*/){
	if (!textChangedFlag) return;
	if (!DataStatus::getInstance()->getDataMgr()) return;
	ViewpointParams* vParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(vParams, "edit Viewpoint text");
	bool usingLatLon = vParams->isLatLon();
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	Viewpoint* currentViewpoint = vParams->getCurrentViewpoint();
	const vector<double>& tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	//In lat-lon mode convert latlon values to campos
	if (usingLatLon) {
		//Set the latlon values from the gui to the params
		vParams->setCamPosLatLon(camPosLat->text().toFloat(),camPosLon->text().toFloat());
		vParams->setRotCenterLatLon(rotCenterLat->text().toFloat(),rotCenterLon->text().toFloat());
		//Convert the latlon coordinates for this time step:
		vParams->convertLocalFromLonLat(timestep);
		//Update the gui text to display user coordinates:

		camPos0->setText(QString::number(vParams->getCameraPosLocal(0)+tvExts[0]));
		camPos1->setText(QString::number(vParams->getCameraPosLocal(1)+tvExts[1]));
		rotCenter0->setText(QString::number(vParams->getRotationCenterLocal(0)+tvExts[0]));
		rotCenter1->setText(QString::number(vParams->getRotationCenterLocal(1)+tvExts[1]));
	} else {
		//Do the opposite
		
		currentViewpoint->setCameraPosLocal(0, camPos0->text().toFloat()-tvExts[0]);
		currentViewpoint->setCameraPosLocal(1, camPos1->text().toFloat()-tvExts[1]);
		currentViewpoint->setRotationCenterLocal(0,rotCenter0->text().toFloat()-tvExts[0]);
		currentViewpoint->setRotationCenterLocal(1,rotCenter1->text().toFloat()-tvExts[1]);
		//Convert to latlon if there is georeferencing
		bool doconvert = false;
		if (DataStatus::getProjectionString().size() != 0){
			doconvert = vParams->convertLocalToLonLat(timestep);
		}
		if(doconvert){
			camPosLat->setText(QString::number(vParams->getCamPosLatLon()[0]));
			camPosLon->setText(QString::number(vParams->getCamPosLatLon()[1]));
			rotCenterLat->setText(QString::number(vParams->getRotCenterLatLon()[0]));
			rotCenterLon->setText(QString::number(vParams->getRotCenterLatLon()[1]));
		}
	}
	//Did numLights change?
	int nLights = numLights->text().toInt();
	
	//make sure it's a valid number:
	bool changed = false;
	if (nLights < 0 ){
		nLights = 0; changed = true;
	}
	if (nLights > 3){
		nLights = 3; changed = true;
	}
	if (changed) {
		numLights->setText(QString::number(nLights));
	}
	if (nLights != vParams->getNumLights()){
		changed = true;
		vParams->setNumLights(nLights);
		bool lightOn;
		lightOn = (nLights > 0);
		lightPos00->setEnabled(lightOn);
		lightPos01->setEnabled(lightOn);
		lightPos02->setEnabled(lightOn);
		lightSpec0->setEnabled(lightOn);
		lightDiff0->setEnabled(lightOn);
		shininessEdit->setEnabled(lightOn);
		ambientEdit->setEnabled(lightOn);
		lightOn = (nLights > 1);
		lightPos10->setEnabled(lightOn);
		lightPos11->setEnabled(lightOn);
		lightPos12->setEnabled(lightOn);
		lightSpec1->setEnabled(lightOn);
		lightDiff1->setEnabled(lightOn);
		lightOn = (nLights > 2);
		lightPos20->setEnabled(lightOn);
		lightPos21->setEnabled(lightOn);
		lightPos22->setEnabled(lightOn);
		lightSpec2->setEnabled(lightOn);
		lightDiff2->setEnabled(lightOn);
	}
	//Get the light directions from the gui:
	vParams->setLightDirection(0,0,lightPos00->text().toFloat());
	vParams->setLightDirection(0,1,lightPos01->text().toFloat());
	vParams->setLightDirection(0,2,lightPos02->text().toFloat());
	vParams->setLightDirection(1,0,lightPos10->text().toFloat());
	vParams->setLightDirection(1,1,lightPos11->text().toFloat());
	vParams->setLightDirection(1,2,lightPos12->text().toFloat());
	vParams->setLightDirection(2,0,lightPos20->text().toFloat());
	vParams->setLightDirection(2,1,lightPos21->text().toFloat());
	vParams->setLightDirection(2,2,lightPos22->text().toFloat());
	//final component is 0 (for gl directional light)
	vParams->setLightDirection(0,3,0.f);
	vParams->setLightDirection(1,3,0.f);
	vParams->setLightDirection(2,3,0.f);

	//get the lighting coefficients from the gui:
	vParams->setAmbientCoeff(ambientEdit->text().toFloat());
	vParams->setDiffuseCoeff(0, lightDiff0->text().toFloat());
	vParams->setDiffuseCoeff(1, lightDiff1->text().toFloat());
	vParams->setDiffuseCoeff(2, lightDiff2->text().toFloat());
	vParams->setSpecularCoeff(0,lightSpec0->text().toFloat());
	vParams->setSpecularCoeff(1,lightSpec1->text().toFloat());
	vParams->setSpecularCoeff(2,lightSpec2->text().toFloat());
	vParams->setExponent(shininessEdit->text().toInt());
	
	VizWinMgr::getInstance()->setVizDirty(vParams, LightingBit, true);

	currentViewpoint->setCameraPosLocal(2, camPos2->text().toFloat()-tvExts[2]);
	vParams->setViewDir(0, viewDir0->text().toFloat());
	vParams->setViewDir(1, viewDir1->text().toFloat());
	vParams->setViewDir(2, viewDir2->text().toFloat());
	vParams->setUpVec(0, upVec0->text().toFloat());
	vParams->setUpVec(1, upVec1->text().toFloat());
	vParams->setUpVec(2, upVec2->text().toFloat());
	
	currentViewpoint->setRotationCenterLocal(2,rotCenter2->text().toFloat()-tvExts[2]);

	float sepAngle = stereoSeparationEdit->text().toFloat();
	vParams->setStereoSeparation(sepAngle);
	if (sepAngle > 0.f) stereoCombo->setEnabled(true); 
	else stereoCombo->setEnabled(false);

	
	PanelCommand::captureEnd(cmd, vParams);
	updateRenderer(vParams,false, false);
	guiSetTextChanged(false);

	if (changed) updateTab();
	
}
void ViewpointEventRouter::
viewpointReturnPressed(void){
	confirmText(true);
}


//Insert values from params into tab panel
//
void ViewpointEventRouter::updateTab(){
	if(!MainForm::getTabManager()->isFrontTab(this)) return;
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	
	QString strng;
	Session::getInstance()->blockRecording();
	
	if (vpParams->isLocal())
		LocalGlobal->setCurrentIndex(1);
	else 
		LocalGlobal->setCurrentIndex(0);
	
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
#ifdef TEST_KEYFRAMING
	speedEdit->setText(QString::number(keyframeSpeed));
#endif
	int nLights = vpParams->getNumLights();
	numLights->setText(strng.setNum(nLights));
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	latLonCheckbox->setChecked(vpParams->isLatLon());
	vector <double> tvExts(6,0.);
	if(DataStatus::getInstance()->getDataMgr()){
		tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	}
	//In latlon mode convert the latlon or vice versa:
	bool showlatlon = false;
	if (vpParams->isLatLon()){
		//Do a conversion in the params, to make sure the values are current
		vpParams->convertLocalFromLonLat(timestep);
		showlatlon = true;
	}
	else {
		if (DataStatus::getProjectionString().size() > 0)
			showlatlon = vpParams->convertLocalToLonLat(timestep);
	}
	if (showlatlon){
		camPosLat->setText(QString::number(vpParams->getCamPosLatLon()[0]));
		camPosLon->setText(QString::number(vpParams->getCamPosLatLon()[1]));
		rotCenterLat->setText(QString::number(vpParams->getRotCenterLatLon()[0]));
		rotCenterLon->setText(QString::number(vpParams->getRotCenterLatLon()[1]));
	}
	if (DataStatus::getProjectionString().size() > 0)
		latLonFrame->show();
	else 
		latLonFrame->hide();
	//Always display the current values of the campos and rotcenter
		
	camPos0->setText(strng.setNum(tvExts[0]+currentViewpoint->getCameraPosLocal(0), 'g', 3));
	camPos1->setText(strng.setNum(tvExts[1]+currentViewpoint->getCameraPosLocal(1), 'g', 3));
	camPos2->setText(strng.setNum(tvExts[2]+currentViewpoint->getCameraPosLocal(2), 'g', 3));
	viewDir0->setText(strng.setNum(currentViewpoint->getViewDir(0), 'g', 3));
	viewDir1->setText(strng.setNum(currentViewpoint->getViewDir(1), 'g', 3));
	viewDir2->setText(strng.setNum(currentViewpoint->getViewDir(2), 'g', 3));
	upVec0->setText(strng.setNum(currentViewpoint->getUpVec(0), 'g', 3));
	upVec1->setText(strng.setNum(currentViewpoint->getUpVec(1), 'g', 3));
	upVec2->setText(strng.setNum(currentViewpoint->getUpVec(2), 'g', 3));
	//perspectiveCombo->setCurrentIndex(currentViewpoint->hasPerspective());
	rotCenter0->setText(strng.setNum(tvExts[0]+currentViewpoint->getRotationCenterLocal(0),'g',3));
	rotCenter1->setText(strng.setNum(tvExts[1]+currentViewpoint->getRotationCenterLocal(1),'g',3));
	rotCenter2->setText(strng.setNum(tvExts[2]+currentViewpoint->getRotationCenterLocal(2),'g',3));
	const float* lightDir = vpParams->getLightDirection(0);
	lightPos00->setText(QString::number(lightDir[0]));
	lightPos01->setText(QString::number(lightDir[1]));
	lightPos02->setText(QString::number(lightDir[2]));
	lightDir = vpParams->getLightDirection(1);
	lightPos10->setText(QString::number(lightDir[0]));
	lightPos11->setText(QString::number(lightDir[1]));
	lightPos12->setText(QString::number(lightDir[2]));
	lightDir = vpParams->getLightDirection(2);
	lightPos20->setText(QString::number(lightDir[0]));
	lightPos21->setText(QString::number(lightDir[1]));
	lightPos22->setText(QString::number(lightDir[2]));

	ambientEdit->setText(QString::number(vpParams->getAmbientCoeff()));
	lightDiff0->setText(QString::number(vpParams->getDiffuseCoeff(0)));
	lightDiff1->setText(QString::number(vpParams->getDiffuseCoeff(1)));
	lightDiff2->setText(QString::number(vpParams->getDiffuseCoeff(2)));
	lightSpec0->setText(QString::number(vpParams->getSpecularCoeff(0)));
	lightSpec1->setText(QString::number(vpParams->getSpecularCoeff(1)));
	lightSpec2->setText(QString::number(vpParams->getSpecularCoeff(2)));
	shininessEdit->setText(QString::number(vpParams->getExponent()));
	

	//Enable light direction text boxes as needed:
	bool lightOn;
	lightOn = (nLights > 0);
	lightPos00->setEnabled(lightOn);
	lightPos01->setEnabled(lightOn);
	lightPos02->setEnabled(lightOn);
	lightSpec0->setEnabled(lightOn);
	lightDiff0->setEnabled(lightOn);
	shininessEdit->setEnabled(lightOn);
	ambientEdit->setEnabled(lightOn);
	lightOn = (nLights > 1);
	lightPos10->setEnabled(lightOn);
	lightPos11->setEnabled(lightOn);
	lightPos12->setEnabled(lightOn);
	lightSpec1->setEnabled(lightOn);
	lightDiff1->setEnabled(lightOn);
	lightOn = (nLights > 2);
	lightPos20->setEnabled(lightOn);
	lightPos21->setEnabled(lightOn);
	lightPos22->setEnabled(lightOn);
	lightSpec2->setEnabled(lightOn);
	lightDiff2->setEnabled(lightOn);

	stereoSeparationEdit->setText(QString::number(vpParams->getStereoSeparation()));
	stereoCombo->setCurrentIndex(vpParams->getStereoMode());
	if (vpParams->getStereoSeparation() > 0.f)
		stereoCombo->setEnabled(true);
	else 
		stereoCombo->setEnabled(false);
	
	guiSetTextChanged(false);
	Session::getInstance()->unblockRecording();
	VizWinMgr::getInstance()->getTabManager()->update();
	update();
}
void ViewpointEventRouter::
guiCenterSubRegion(RegionParams* rParams){
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "center sub-region view");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	//Find the largest of the dimensions of the current region, projected orthogonal to view
	//direction:
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	float regionSideVector[3], compVec[3], projvec[3];
	float maxProj = -1.f;
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	double regExts[6];
	rParams->GetBox()->GetLocalExtents(regExts,timestep);
	for (int i = 0; i< 3; i++){
		//Make a vector that points along side(i) of subregion,
		
		for (int j = 0; j<3; j++){
			regionSideVector[j] = 0.f;
			if (j == i) {
				regionSideVector[j] = regExts[j+3] - regExts[j];
			}
		}
		//Now find its component orthogonal to view direction:
		float dotprod = vdot(currentViewpoint->getViewDir(),regionSideVector);
		vmult(currentViewpoint->getViewDir(),dotprod,compVec);
		//projvec is projection orthogonal to view dir:
		vsub(regionSideVector,compVec,projvec);
		float proj = vlength(projvec);
		if (proj > maxProj) maxProj = proj;
	}


	//calculate the camera position: center - 2*viewDir*maxSide;
	//Position the camera 2.5*maxSide units away from the center, aimed
	//at the center
	
	for (int i = 0; i<3; i++){

		float camPosCrd = rParams->getLocalRegionCenter(i,timestep) -(2.5*maxProj*currentViewpoint->getViewDir(i)/stretch[i]);
		currentViewpoint->setCameraPosLocal(i, camPosCrd);
		currentViewpoint->setRotationCenterLocal(i,rParams->getLocalRegionCenter(i, timestep));

	}
	
	//modify near/far distance as needed:
	VizWinMgr::getInstance()->resetViews(vpParams);
	if (vpParams->isLatLon()) {
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		vpParams->convertLocalToLonLat(timestep);
	}
	updateTab();
	updateRenderer(vpParams,false,  false);
	PanelCommand::captureEnd(cmd,vpParams);
	
}
void ViewpointEventRouter::
guiCenterFullRegion(RegionParams* rParams){
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "center full region view");
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	vpParams->centerFullRegion(timestep);
	//modify near/far distance as needed:
	VizWinMgr::getInstance()->resetViews(vpParams);
	updateTab();
	updateRenderer(vpParams,false,  false);
	PanelCommand::captureEnd(cmd,vpParams);
	
}
//Align the view direction to one of the axes.
//axis is 2,3,4 for +X,Y,Z,  and 5,6,7 for -X,-Y,-Z
//
void ViewpointEventRouter::
guiAlignView(int axis){
	float vdir[3] = {0.f, 0.f, 0.f};
	float up[3] = {0.f, 1.f, 0.f};
	float axes[3][3] = {{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};
	float vpos[3];
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	if (axis == 1) {  //Special case to align to closest axis.
		//determine the closest view direction and up vector to the current viewpoint.
		//Check the dot product with all axes
		float maxVDot = -1.f;
		int bestVDir = 0;
		float maxUDot = -1.f;
		int bestUDir = 0;
		float* curViewDir = currentViewpoint->getViewDir();
		const float* curUpVec = currentViewpoint->getUpVec();
		for (int i = 0; i< 3; i++){
			float dotVProd = vdot(axes[i], curViewDir);
			float dotUProd = vdot(axes[i], curUpVec);
			if (abs(dotVProd) > maxVDot) {
				maxVDot = abs(dotVProd);
				bestVDir = i+1;
				if (dotVProd < 0.f) bestVDir = -i-1; 
			}
			if (abs(dotUProd) > maxUDot) {
				maxUDot = abs(dotUProd);
				bestUDir = i+1;
				if (dotUProd < 0.f) bestUDir = -i-1; 
			}
		}
		for (int i = 0; i< 3; i++){
			if (bestVDir > 0) vdir[i] = axes[bestVDir-1][i];
			else vdir[i] = -axes[-1-bestVDir][i];

			if (bestUDir > 0) up[i] = axes[bestUDir-1][i];
			else up[i] = -axes[-1-bestUDir][i];
		}
	} else {
	//establish view direction, up vector:
		switch (axis){
			case(2): vdir[0] = 1.f; break;
			case(3): vdir[1] = 1.f; up[1]=0.f; up[0] = 1.f; break;
			case(4): vdir[2] = 1.f; break;
			case(5): vdir[0] = -1.f; break;
			case(6): vdir[1] = -1.f; up[1]=0.f; up[0] = 1.f; break;
			case(7): vdir[2] = -1.f; break;
			default: assert(0);
		}
	}
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "align viewpoint to axis");
	
	
	//Determine distance from center to camera, in stretched coordinates
	float scampos[3], srotctr[3];
	currentViewpoint->getStretchedCamPosLocal(scampos);
	currentViewpoint->getStretchedRotCtrLocal(srotctr);
	//determine the relative position in stretched coords:
	vsub(scampos,srotctr,vpos);
	float viewDist = vlength(vpos);
	//Keep the view center as is, make view direction vdir 
	currentViewpoint->setViewDir(vdir);
	//Make the up vector +Y, or +X
	currentViewpoint->setUpVec(up);
	//Position the camera the same distance from the center but down the -axis direction
	vmult(vdir, viewDist, vdir);
	vsub(srotctr, vdir, vpos);
	currentViewpoint->setStretchedCamPosLocal(vpos);
	if (vpParams->isLatLon()){
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		vpParams->convertLocalToLonLat(timestep);
	}
	updateTab();
	updateRenderer(vpParams,false,false);
	PanelCommand::captureEnd(cmd,vpParams);
}

//Reset the center of view.  Leave the camera where it is
void ViewpointEventRouter::
guiSetCenter(const float* coords){
	float vdir[3];
	if (savedCommand) {
		delete savedCommand;
		savedCommand = 0;
	}
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	const float* stretch = DataStatus::getInstance()->getStretchFactors();

	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "set view center");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	//Determine the new viewDir in stretched world coords
	
	vcopy(coords, vdir);
	//Stretch the new view center coords
	for (int i = 0; i<3; i++) vdir[i] *= stretch[i];
	float campos[3];
	currentViewpoint->getStretchedCamPosLocal(campos);
	vsub(vdir,campos, vdir);
	
	vnormal(vdir);
	currentViewpoint->setViewDir(vdir);
	
	for (int i = 0; i<3; i++){
		currentViewpoint->setRotationCenterLocal(i,coords[i]);
	}
	if (vpParams->isLatLon()){
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		vpParams->convertLocalToLonLat(timestep);
	}
	updateTab();
	updateRenderer(vpParams,false,  false);
	PanelCommand::captureEnd(cmd,vpParams);
	
}
void ViewpointEventRouter::guiSetStereoMode(int mode){
	//Make sure it's a change in mode.
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	int oldMode = vpParams->getStereoMode();
	if (oldMode == mode) return;
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "set stereo mode");
	float sep = vpParams->getStereoSeparation();
	// How does the angle change?
	float angleChange = sep*M_PI/180.;
	if (oldMode == 0){
		if (mode == 1) angleChange = -0.5*angleChange; //left eye uses negative angle
		else angleChange = 0.5*angleChange;
	} else if (oldMode == 1){
		//left to right, either half or full angle
		if (mode == 0) angleChange = 0.5*angleChange;
	} else { //oldMode == 2; right to left rotation
		if (mode == 0) angleChange = -0.5*angleChange;
		else angleChange = -angleChange;
	}
	vpParams->setStereoMode(mode);
	//	set the view direction to point from the camera to the rotation center.
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	//Determine the new viewDir:
	float vdir[3], updir[3], upComp[3], rightdir[3], newCamPos[3];
	float rotCtr[3], camPos[3];
	currentViewpoint->getStretchedRotCtrLocal(rotCtr);
	currentViewpoint->getStretchedCamPosLocal(camPos);
	vsub(rotCtr, camPos, vdir);
	//Remember the distance:
	float vdist = vlength(vdir);
	
	//Make sure the viewDir is normalized:
	vnormal(vdir);
	//Get the up vector:
	float* upvec = currentViewpoint->getUpVec();
	vcopy(upvec, updir);
	vnormal(updir);
	
	//find the component of updir in vdir direction, subtract it from updir, then normalize,
	//so that updir is orthogonal to vdir.
	vmult(updir, vdot(updir,vdir), upComp);
	vsub(updir, upComp, updir);
	vnormal(updir);
	//Next find the vector pointing to the right (vdir x up)
	vcross(vdir, updir, rightdir);
	//Rotate vdir by angleChange in the plane of vdir and rightdir.
	//New direction is cos(angle)*vdir - sin(angle)*rightdir
	//(neg rotation because left eye uses rotation to right)
	
	vmult(vdir, cos(angleChange), vdir);
	vmult(rightdir, sin(angleChange), rightdir);
	vsub(vdir, rightdir, vdir);


	//New camera position is obtained by subtracting vdist*vdir from rotationCenter:
	vmult(vdir, vdist, newCamPos);
	vsub(rotCtr, newCamPos, newCamPos);
	//Now specify the new viewpoint:
	currentViewpoint->setStretchedCamPosLocal(newCamPos);
	currentViewpoint->setViewDir(vdir);
	currentViewpoint->setUpVec(updir);
	// convert to latlon
	if (vpParams->isLatLon()){
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		vpParams->convertLocalToLonLat(timestep);
	}
	//  set render windows dirty
	updateRenderer(vpParams,false,  false);
	PanelCommand::captureEnd(cmd,vpParams);
}
void ViewpointEventRouter::
setHomeViewpoint(){
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "Set Home Viewpoint");
	Viewpoint* currentViewpoint = vpParams->getCurrentViewpoint();
	vpParams->setHomeViewpoint(new Viewpoint(*currentViewpoint));
	updateTab();
	updateRenderer(vpParams,false,false);
	PanelCommand::captureEnd(cmd, vpParams);
}
void ViewpointEventRouter::
useHomeViewpoint(){
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);

	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "Use Home Viewpoint");
	Viewpoint* homeViewpoint = vpParams->getHomeViewpoint();
	vpParams->setCurrentViewpoint(new Viewpoint(*homeViewpoint));
	//Convert to latlong if necessary
	if (vpParams->isLatLon()){
		int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		assert (timestep >= 0);
		//Convert the latlon coordinates for this time step:
		vpParams->convertLocalToLonLat(timestep);
	}
	updateTab();
	updateRenderer(vpParams,false,  false);
	PanelCommand::captureEnd(cmd, vpParams);
}
void ViewpointEventRouter::
captureMouseUp(){
	//Update the tab:
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	if (!savedCommand) {
		panChanged = false;
		return;
	}
	if (panChanged){
		//Apply the translation to the rotation
		float trans, newRot[3];
		float* camPosLoc = vpParams->getCameraPosLocal();
		float* rotCenterLocal = vpParams->getRotationCenterLocal();
		int ts = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
		for (int i = 0; i<3; i++) {
			trans = camPosLoc[i] - lastCamPos[i];
			newRot[i] = rotCenterLocal[i]+trans;
		}
		vpParams->setRotationCenterLocal(newRot, ts);
		panChanged = false;
		updateRenderer(vpParams,false,false);
	}
	updateTab();
	PanelCommand::captureEnd(savedCommand, vpParams);
	//DON't Set region  dirty
	
	//Just rerender:
	VizWinMgr::getInstance()->refreshViewpoint(vpParams);
	savedCommand = 0;
	
}
//If the mouse drag resulted in a spin, the event is modified when
//the spin is terminated:
void ViewpointEventRouter::
endSpin(){
	//Update the tab:
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	if (!savedCommand) return;
	updateTab();
	
	QString* str = new QString("viewpoint spin");
	savedCommand->setDescription(*str);
	PanelCommand::captureEnd(savedCommand, vpParams);
	savedCommand = 0;
	
}


/*
 * respond to change in viewer frame
 * Don't record position except when mouse up/down
 */
void ViewpointEventRouter::
navigate (ViewpointParams* vpParams, float* posn, float* viewDir, float* upVec){
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	vpParams->setCameraPosLocal(posn, timestep);
	vpParams->setUpVec(upVec);
	vpParams->setViewDir(viewDir);
	updateTab();
}
//Toggle between latlon and local coords
void ViewpointEventRouter::
guiToggleLatLon(bool on){
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	PanelCommand* cmd = PanelCommand::captureStart(vpParams, "toggle lat/lon and local coordinates");
	//Nothing needs to be reset, since the current panel text should not change.
	vpParams->setLatLon(on);
	PanelCommand::captureEnd(cmd,vpParams);
}
//Reinitialize Viewpoint tab settings, session has changed.
//Note that this is called after the globalViewpointParams are set up, but before
//any of the localViewpointParams are setup.
void ViewpointEventRouter::
reinitTab(bool doOverride){
	if (VizWinMgr::getInstance()->getNumVisualizers() > 1) LocalGlobal->setEnabled(true);
	else LocalGlobal->setEnabled(false);
}

//Save undo/redo state when user grabs a rake handle
//
void ViewpointEventRouter::
captureMouseDown(int button){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	guiSetTextChanged(false);
	if (savedCommand) delete savedCommand;
	if (button == 3){//panning
		//save current camera position
		float* camPos = vpParams->getCameraPosLocal();
		for (int i = 0; i<3; i++) lastCamPos[i] = camPos[i];
		panChanged = true;
	}
	savedCommand = PanelCommand::captureStart(vpParams,  "viewpoint navigation");
}

void ViewpointEventRouter::
updateRenderer(ViewpointParams* vpParams, bool prevEnabled,   bool newWindow){
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	bool local = vpParams->isLocal();
	//Always set the values in the active viz.  This amounts to stuffing
	//the new values into the trackball.
	//If the settings are global, then
	//also make the other global visualizers redraw themselves
	//(All global visualizers will be sharing the same trackball)
	//
	VizWin* viz = myVizMgr->getActiveVisualizer();
	viz->getGLWindow()->setViewerCoordsChanged(true);
	//If this panel is associated with the active visualizer, stuff the values
	//into that viz:
	if (viz) {
		if (local) viz->setValuesFromGui(vpParams);
		else viz->setValuesFromGui(myVizMgr->getViewpointParams(viz->getWindowNum()));
	}
	
	if (!local){
		//Find all the viz windows that are using global settings.
		//That is the case if the window is non-null, and if the corresponding
		//vpparms is either null or set to Global
		//
		for (int i = 0; i< MAXVIZWINS; i++){
			if (i == myVizMgr->getActiveViz()) continue;
			if( viz == myVizMgr->getVizWin(i)){
				viz->getGLWindow()->setViewerCoordsChanged(true);
				//Bypass normal access to vpParams!
				if(!(myVizMgr->getRealVPParams(i)) || !(myVizMgr->getRealVPParams(i)->isLocal())){
					viz->updateGL();
				}
			}
		}
	}
}

void ViewpointEventRouter::
makeCurrent(Params* prev, Params* next, bool,int,bool) {
	ViewpointParams* vParams = (ViewpointParams*) next;
	int vizNum = vParams->getVizNum();
	VizWinMgr::getInstance()->setViewpointParams(vizNum, vParams);
	//If the local/global changes, need to tell the window, too.
	if (vizNum >=0 && prev->isLocal() != next->isLocal())
		VizWinMgr::getInstance()->getVizWin(vizNum)->setGlobalViewpoint(!next->isLocal());
	//Also update current Tab.  It's probably visible.
	updateTab();
	updateRenderer(vParams, false, false);
}
#ifdef TEST_KEYFRAMING
void ViewpointEventRouter::
readKeyframes(){
	//Open the file:
	
	QString filename = QFileDialog::getOpenFileName(this,
        	"Specify file name for reading viewpoints", 
			".",
        	"Text files (*.txt)");
	//Check that user did specify a file:
	if (filename.isNull()) {
		return;
	}
	
	//Open the file:
	FILE* vpFile = fopen((const char*)filename.toAscii(),"r");
	if (!vpFile){
		MessageReporter::errorMsg("Viewpoint Load Error;\nUnable to open file %s",(const char*)filename.toAscii());
		return;
	}
	//clear out existing viewpoints:
	
	ViewpointParams::clearLoadedViewpoints();
	while(1){
		Viewpoint* vp = new Viewpoint;
		int timestep;
		float campos[3],viewdir[3],upvec[3],rotcenter[3];
		int nread = fscanf(vpFile,"%d %g %g %g %g %g %g %g %g %g %g %g %g",
			&timestep, campos,campos+1,campos+2,
			viewdir,viewdir+1,viewdir+2,
			upvec,upvec+1,upvec+2,
			rotcenter,rotcenter+1,rotcenter+2);
		if (nread != 13) {delete vp; break;}
		vp->setCameraPosLocal(campos);
		vp->setViewDir(viewdir);
		vp->setUpVec(upvec);
		vp->setRotationCenterLocal(rotcenter);
		ViewpointParams::addViewpoint(vp);
		ViewpointParams::addTimestep((size_t)timestep);
		

	}
	MessageReporter::warningMsg(" %d viewpoints read from file %s", ViewpointParams::getNumLoadedViewpoints(),(const char*)filename.toAscii());
	//Set the current viewpoint
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	int framenum = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	Viewpoint* vp = new Viewpoint(*ViewpointParams::getLoadedViewpoint(framenum));
	vpParams->setCurrentViewpoint(vp);
	updateTab();
	updateRenderer(vpParams,false, false);

}
void ViewpointEventRouter::
writeKeyframe(){
	static QString filename;
	if (!viewpointOutputFile){ //Open a file
		
		filename = QFileDialog::getSaveFileName(this,
        	"Specify (*.txt) file name for saving viewpoints",
			".",
        	"Text files (*.txt)");
		if (filename.isNull())
		return;

		//If the file has no suffix, add .txt
		if (filename.indexOf(".") == -1){
			filename.append(".txt");
		}

		QFileInfo fileInfo(filename);
		if (fileInfo.exists()){
			int rc = QMessageBox::warning(0, "Viewpoint file exists.", QString("OK to replace viewpoint file \n%1 ?").arg(filename), QMessageBox::Ok, 
				QMessageBox::No);
			if (rc != QMessageBox::Ok) return;
		}
		
	
	
		//Open the save file:
		viewpointOutputFile = fopen((const char*)filename.toAscii(),"w");
		if (!viewpointOutputFile){
			MessageReporter::errorMsg("Viewpoint save error;\nUnable to open file %s",(const char*)filename.toAscii());
			return;
		}
	}
	//OK, file is open. Write current viewpoint:
	keyframeSpeed = speedEdit->text().toFloat();
	ViewpointParams* vpParams = (ViewpointParams*)VizWinMgr::getInstance()->getApplicableParams(Params::_viewpointParamsTag);
	Viewpoint* vp = vpParams->getCurrentViewpoint();
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentTimestep();
	int numBytes = fprintf(viewpointOutputFile, "%d %g %g %g %g %g %g %g %g %g %g %g %g %g\n",
		timestep, vp->getCameraPosLocal(0),vp->getCameraPosLocal(1),vp->getCameraPosLocal(2),
		vp->getViewDir(0),vp->getViewDir(1),vp->getViewDir(2),
		vp->getUpVec(0),vp->getUpVec(1),vp->getUpVec(2),
		vp->getRotationCenterLocal(0),vp->getRotationCenterLocal(1),vp->getRotationCenterLocal(2),keyframeSpeed);
	if (numBytes <= 0){
		MessageReporter::errorMsg("Viewpoint save error;\nUnable to write to %s",(const char*)filename.toAscii());
		fclose(viewpointOutputFile);
		viewpointOutputFile = 0;
		return;
	}
	fflush(viewpointOutputFile);
}
void ViewpointEventRouter::
changeKeyframeSpeed(){
	keyframeSpeed = speedEdit->text().toFloat();
}
#endif