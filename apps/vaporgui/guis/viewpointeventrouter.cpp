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
#include "glutil.h"	// Must be included first!!!
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
#include "vapor/ControlExecutive.h"
#include "viztab.h"
#include "tabmanager.h"
#include "mainform.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include "command.h"
#include "params.h"
using namespace VAPoR;

ViewpointEventRouter::ViewpointEventRouter(QWidget* parent ): QWidget(parent), Ui_VizTab(), EventRouter(){
	setupUi(this);
	myParamsBaseType = ControlExec::GetTypeFromTag(Params::_viewpointParamsTag);
	
	panChanged = false;
	savedCommand = 0;
	for (int i = 0; i<3; i++)lastCamPos[i] = 0.f;
}


ViewpointEventRouter::~ViewpointEventRouter(){
	
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
	ViewpointParams* vParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	
	Command* cmd = Command::CaptureStart(vParams,"viewpoint text change");
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	const vector<double>& tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	
	vector<double> camPos, rotCtr;
	rotCtr.push_back(rotCenter0->text().toFloat()-tvExts[0]);
	rotCtr.push_back(rotCenter1->text().toFloat()-tvExts[1]);
	rotCtr.push_back(rotCenter2->text().toFloat()-tvExts[2]);
	camPos.push_back(camPos0->text().toFloat()-tvExts[0]);
	camPos.push_back(camPos1->text().toFloat()-tvExts[1]);
	camPos.push_back(camPos2->text().toFloat()-tvExts[2]);
	vParams->setRotationCenterLocal(camPos);
	vParams->setCameraPosLocal(camPos, timestep);
	
	
	
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
	

	vParams->setViewDir(0, viewDir0->text().toFloat());
	vParams->setViewDir(1, viewDir1->text().toFloat());
	vParams->setViewDir(2, viewDir2->text().toFloat());
	vParams->setUpVec(0, upVec0->text().toFloat());
	vParams->setUpVec(1, upVec1->text().toFloat());
	vParams->setUpVec(2, upVec2->text().toFloat());
	
	
	vParams->Validate(false);
	Command::CaptureEnd(cmd, vParams);

	updateRenderer(vParams,false, -1, false);

	guiSetTextChanged(false);

	updateTab();
	VizWinMgr::getInstance()->forceRender(vParams);
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
	
	
	if (vpParams->IsLocal())
		LocalGlobal->setCurrentIndex(1);
	else 
		LocalGlobal->setCurrentIndex(0);
	

	int nLights = vpParams->getNumLights();
	numLights->setText(strng.setNum(nLights));
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	
	vector <double> tvExts(6,0.);
	if(DataStatus::getInstance()->getDataMgr()){
		tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents((size_t)timestep);
	}
	
	
	
	
	latLonFrame->hide();
	//Always display the current values of the campos and rotcenter
		
	camPos0->setText(strng.setNum(tvExts[0]+vpParams->getCameraPosLocal(0), 'g', 3));
	camPos1->setText(strng.setNum(tvExts[1]+vpParams->getCameraPosLocal(1), 'g', 3));
	camPos2->setText(strng.setNum(tvExts[2]+vpParams->getCameraPosLocal(2), 'g', 3));
	const vector<double>&vdir = vpParams->getViewDir();
	const vector<double>&upvec = vpParams->getUpVec();
	viewDir0->setText(strng.setNum(vdir[0], 'g', 3));
	viewDir1->setText(strng.setNum(vdir[1], 'g', 3));
	viewDir2->setText(strng.setNum(vdir[2], 'g', 3));
	upVec0->setText(strng.setNum(upvec[0], 'g', 3));
	upVec1->setText(strng.setNum(upvec[1], 'g', 3));
	upVec2->setText(strng.setNum(upvec[2], 'g', 3));
	
	//perspectiveCombo->setCurrentIndex(currentViewpoint->hasPerspective());
	rotCenter0->setText(strng.setNum(tvExts[0]+vpParams->getRotationCenterLocal(0),'g',3));
	rotCenter1->setText(strng.setNum(tvExts[1]+vpParams->getRotationCenterLocal(1),'g',3));
	rotCenter2->setText(strng.setNum(tvExts[2]+vpParams->getRotationCenterLocal(2),'g',3));
	
	
	lightPos00->setText(QString::number(vpParams->getLightDirection(0,0)));
	lightPos01->setText(QString::number(vpParams->getLightDirection(0,1)));
	lightPos02->setText(QString::number(vpParams->getLightDirection(0,2)));
	
	lightPos10->setText(QString::number(vpParams->getLightDirection(1,0)));
	lightPos11->setText(QString::number(vpParams->getLightDirection(1,1)));
	lightPos12->setText(QString::number(vpParams->getLightDirection(1,2)));
	
	lightPos20->setText(QString::number(vpParams->getLightDirection(2,0)));
	lightPos21->setText(QString::number(vpParams->getLightDirection(2,1)));
	lightPos22->setText(QString::number(vpParams->getLightDirection(2,2)));

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

	
	guiSetTextChanged(false);
	
	VizWinMgr::getInstance()->getTabManager()->update();
	update();
}
void ViewpointEventRouter::
guiCenterSubRegion(RegionParams* rParams){
	
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	//Find the largest of the dimensions of the current region, projected orthogonal to view
	//direction:
	//Make sure the viewDir is normalized:
	vector<double> viewDir = vpParams->getViewDir();
	
	vnormal(viewDir);
	float regionSideVector[3], compVec[3], projvec[3];
	float maxProj = -1.f;
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
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
		double dotprod = 0.;
		
		for (int j=0;j<3;j++)
			dotprod += (viewDir[j]*regionSideVector[j]);
		for (int j=0;j<3;j++)
			compVec[j] = dotprod*viewDir[j];
		//projvec is projection orthogonal to view dir:
		vsub(regionSideVector,compVec,projvec);
		float proj = vlength(projvec);
		if (proj > maxProj) maxProj = proj;
	}


	//calculate the camera position: center - 2*viewDir*maxSide;
	//Position the camera 2.5*maxSide units away from the center, aimed
	//at the center
	Command* cmd = Command::CaptureStart(vpParams,"Center viewpoint on subregion");
	vector<double> camPos;
	vector<double> rotCtr;
	for (int i = 0; i<3; i++){
		float camPosCrd = rParams->getLocalRegionCenter(i,timestep) -(2.5*maxProj*viewDir[i]/stretch[i]);
		camPos.push_back(camPosCrd);
		rotCtr.push_back(rParams->getLocalRegionCenter(i, timestep));
	}
	vpParams->setCameraPosLocal(camPos, timestep);
	vpParams->setRotationCenterLocal(rotCtr);
	Command::CaptureEnd(cmd,vpParams);
	
	//modify near/far distance as needed:
	VizWinMgr::getInstance()->resetViews(vpParams);
	
	updateTab();
	updateRenderer(vpParams,false,-1,  false);
	
	
}
void ViewpointEventRouter::
guiCenterFullRegion(RegionParams* rParams){
	
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	int timestep = VizWinMgr::getActiveAnimationParams()->getCurrentTimestep();
	vpParams->centerFullRegion(timestep);
	//modify near/far distance as needed:
	VizWinMgr::getInstance()->resetViews(vpParams);
	updateTab();
	updateRenderer(vpParams,false, -1, false);
	
	
}
//Align the view direction to one of the axes.
//axis is 2,3,4 for +X,Y,Z,  and 5,6,7 for -X,-Y,-Z
//
void ViewpointEventRouter::
guiAlignView(int axis){
	//float vdir[3] = {0.f, 0.f, 0.f};
	//float up[3] = {0.f, 1.f, 0.f};
	float axes[3][3] = {{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};
	
	vector<double>vdir(3,0.);
	vector<double>up(3,0.);
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	if (axis == 1) {  //Special case to align to closest axis.
		//determine the closest view direction and up vector to the current viewpoint.
		//Check the dot product with all axes
		float maxVDot = -1.f;
		int bestVDir = 0;
		float maxUDot = -1.f;
		int bestUDir = 0;
		vector<double> curViewDir = vpParams->getViewDir();
		vector<double> curUpVec = vpParams->getUpVec();
		for (int i = 0; i< 3; i++){
			double dotVProd = 0.;
			double dotUProd = 0.;
			for (int j = 0; j<3; j++){
				dotUProd += (axes[i][j],curViewDir[j]);
				dotVProd += (axes[i][j],curUpVec[j]);
			}
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
	
	
	Command* cmd = Command::CaptureStart(vpParams,"axis-align view");
	
	//Determine distance from center to camera, in stretched coordinates
	double scampos[3], srotctr[3], dvpos[3];
	vpParams->getStretchedCamPosLocal(scampos);
	vpParams->getStretchedRotCtrLocal(srotctr);
	//determine the relative position in stretched coords:
	vsub(scampos,srotctr,dvpos);
	float viewDist = vlength(dvpos);
	//Keep the view center as is, make view direction vdir 
	vpParams->setViewDir(vdir);
	//Make the up vector +Y, or +X
	vpParams->setUpVec(up);
	//Position the camera the same distance from the center but down the -axis direction
	for (int i = 0; i<3; i++){
		vdir[i] = vdir[i]*viewDist;
		dvpos[i] = srotctr[i] - vdir[i];
	}
	
	vpParams->setStretchedCamPosLocal(dvpos);
	Command::CaptureEnd(cmd,vpParams);
	updateTab();
	updateRenderer(vpParams,false,-1,false);
	
}

//Reset the center of view.  Leave the camera where it is
void ViewpointEventRouter::
guiSetCenter(const double* coords){
	double vdir[3];
	vector<double> nvdir;
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	const double* stretch = DataStatus::getInstance()->getStretchFactors();


	//Determine the new viewDir in stretched world coords
	
	vcopy(coords, vdir);
	//Stretch the new view center coords
	for (int i = 0; i<3; i++) vdir[i] *= stretch[i];
	double campos[3];
	vpParams->getStretchedCamPosLocal(campos);
	vsub(vdir,campos, vdir);
	
	vnormal(vdir);
	vector<double>vvdir;
	Command* cmd = Command::CaptureStart(vpParams,"re-center view");
	for (int i = 0; i<3; i++) vvdir.push_back(vdir[i]);
	vpParams->setViewDir(vvdir);
	vector<double>rotCtr;
	for (int i = 0; i<3; i++){
		rotCtr.push_back(coords[i]);
	}
	vpParams->setRotationCenterLocal(rotCtr);
	Command::CaptureEnd(cmd,vpParams);
	updateTab();
	updateRenderer(vpParams,false, -1, false);
	
	
}

void ViewpointEventRouter::
setHomeViewpoint(){
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	vpParams->setCurrentVPToHome();
	updateTab();
	updateRenderer(vpParams,false,-1,false);
	
}
void ViewpointEventRouter::
useHomeViewpoint(){
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	
	Viewpoint* homeViewpoint = vpParams->getHomeViewpoint();
	vpParams->setCurrentViewpoint(new Viewpoint(*homeViewpoint));
	
	updateTab();
	updateRenderer(vpParams,false, -1, false);
	
}
void ViewpointEventRouter::
captureMouseUp(){
	//Update the tab:
	ViewpointParams* vpParams = VizWinMgr::getActiveVPParams();
	
	if (panChanged){
		//Apply the translation to the rotation
		float trans;
		vector<double> newRot;
		vector<double> camPosLoc = vpParams->getCameraPosLocal();
		vector<double> rotCenterLocal = vpParams->getRotationCenterLocal();
	
		for (int i = 0; i<3; i++) {
			trans = camPosLoc[i] - lastCamPos[i];
			newRot.push_back(rotCenterLocal[i]+trans);
		}
		vpParams->setRotationCenterLocal(newRot);
		panChanged = false;
		updateRenderer(vpParams,false,-1,false);
	}
	updateTab();
	
	if (savedCommand) Command::CaptureEnd(savedCommand,vpParams);
	savedCommand=0;
	
	//Just rerender:
	VizWinMgr::getInstance()->refreshViewpoint(vpParams);
	
}
//If the mouse drag resulted in a spin, the event is modified when
//the spin is terminated:
void ViewpointEventRouter::
endSpin(){
	updateTab();
}



//Reinitialize Viewpoint tab settings, session has changed.
//Note that this is called after the globalViewpointParams are set up, but before
//any of the localViewpointParams are setup.
void ViewpointEventRouter::
reinitTab(bool doOverride){
	if (VizWinMgr::getInstance()->getNumVisualizers() > 1) LocalGlobal->setEnabled(true);
	else LocalGlobal->setEnabled(false);
	savedCommand=0;
}

//Save undo/redo state when user grabs a rake handle
//
void ViewpointEventRouter::
captureMouseDown(int button){
	//If text has changed, will ignore it-- don't call confirmText()!
	//
	ViewpointParams* vpParams = (ViewpointParams*)ControlExec::GetActiveParams(Params::_viewpointParamsTag);
	guiSetTextChanged(false);
	
	if (savedCommand) delete savedCommand;
	savedCommand = Command::CaptureStart(vpParams, "Viewpoint navigation");
	if (button == 2){//panning
		//save current camera position
		const vector<double>& camPos = vpParams->getCameraPosLocal();
		for (int i = 0; i<3; i++) lastCamPos[i] = camPos[i];
		panChanged = true;
	}
	
}

void ViewpointEventRouter::
updateRenderer(ViewpointParams* vpParams, bool prevEnabled,  int /*instance*/, bool newWindow){
	VizWinMgr* myVizMgr = VizWinMgr::getInstance();
	bool local = vpParams->IsLocal();
	//Always set the values in the active viz.  This amounts to stuffing
	//the new values into the trackball.
	//If the settings are global, then
	//also make the other global visualizers redraw themselves
	//(All global visualizers will be sharing the same trackball)
	//
	VizWin* viz = myVizMgr->getActiveVisualizer();
	
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
		for (int i = 0; i< ControlExec::GetNumVisualizers(); i++){
			if (i == ControlExec::GetActiveVizIndex()) continue;
			
			//Bypass normal access to vpParams!
			if(!(myVizMgr->getViewpointParams(i)) || !(myVizMgr->getViewpointParams(i)->IsLocal())){
				viz->updateGL();
			}
			
		}
	}
}
