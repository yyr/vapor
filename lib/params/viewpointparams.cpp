//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viewpointparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2004
//
//	Description:	Implements the ViewpointParams class
//		This class contains the parameters associated with viewpoint and lights
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include "glutil.h"	// Must be included first!!!
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "viewpointparams.h"
#include "params.h"
#include "viewpoint.h"
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include "datastatus.h"
#include "regionparams.h"
#include "vapor/Version.h"
#include "command.h"


double VAPoR::ViewpointParams::defaultUpVec[3] = {0.f, 1.f, 0.f};
double VAPoR::ViewpointParams::defaultViewDir[3] = {0.f, 0.f, -1.f};
double VAPoR::ViewpointParams::defaultLightDirection[3][3] = {{0.f, 0.f, 1.f},{0.f, 1.f, 0.f},{1.f, 0.f, 0.f}};
int VAPoR::ViewpointParams::defaultNumLights = 2;
double VAPoR::ViewpointParams::defaultDiffuseCoeff[3] = {0.8f, 0.8f, 0.8f};
double VAPoR::ViewpointParams::defaultSpecularCoeff[3] = {0.3f, 0.3f, 0.3f};
double VAPoR::ViewpointParams::defaultAmbientCoeff = 0.1f;
double VAPoR::ViewpointParams::defaultSpecularExp = 20.f;


using namespace VAPoR;

const string ViewpointParams::_shortName = "View";
const string ViewpointParams::_currentViewTag = "CurrentViewpoint";
const string ViewpointParams::_homeViewTag = "HomeViewpoint";
const string ViewpointParams::_lightDirectionsTag = "LightDirections";
const string ViewpointParams::_diffuseCoeffTag = "DiffuseCoefficients";
const string ViewpointParams::_specularCoeffTag = "SpecularCoefficients";
const string ViewpointParams::_specularExpTag = "SpecularExponent";
const string ViewpointParams::_ambientCoeffTag = "AmbientCoefficient";
const string ViewpointParams::_numLightsTag = "NumLights";


ViewpointParams::ViewpointParams(XmlNode* parent, int winnum): Params(parent, Params::_viewpointParamsTag, winnum){
	
	restart();
}

ViewpointParams::~ViewpointParams(){
	
	
}

//Reinitialize viewpoint settings, to center view on the center of full region.
//(this is starting state)
void ViewpointParams::
restart(){
	//Make sure we have current and home viewpoints
	if (!GetRootNode()->HasChild(_currentViewTag)){
		Viewpoint* vp = new Viewpoint();
		ParamNode* vpNode = vp->GetRootNode();
		GetRootNode()->AddRegisteredNode(_currentViewTag,vpNode,vp);
	}
	if (!GetRootNode()->HasChild(_homeViewTag)){
		Viewpoint* vp = new Viewpoint();
		ParamNode* vpNode = vp->GetRootNode();
		GetRootNode()->AddRegisteredNode(_homeViewTag,vpNode,vp);
	}
	centerFullRegion(0);
	setNumLights(defaultNumLights);
	vector<double> ldirs;
	vector<double> diffCoeffs;
	vector<double> specCoeffs;
	for (int i = 0; i<3; i++){
		for (int dir = 0; dir<3; dir++){
			ldirs.push_back(defaultLightDirection[i][dir]);
		}
		//final component is 0 (for gl directional light)
		ldirs.push_back(0.);
		diffCoeffs.push_back(defaultDiffuseCoeff[i]);
		specCoeffs.push_back(defaultSpecularCoeff[i]);
	}
	SetValueDouble(_lightDirectionsTag,"",ldirs);
	SetValueDouble(_diffuseCoeffTag,"",diffCoeffs);
	SetValueDouble(_specularCoeffTag,"",specCoeffs);
	setExponent(defaultSpecularExp);
	setAmbientCoeff(defaultAmbientCoeff);
	
	setCoordTrans();
	
}
void ViewpointParams::setDefaultPrefs(){
	for (int i = 0; i<3; i++){
		defaultUpVec[i] = (i == 1) ? 1.f : 0.f;
		defaultViewDir[i]  = (i == 2) ? -1.f : 0.f;
		defaultLightDirection[0][i] = (i == 2) ? 1.f : 0.f;
		defaultLightDirection[1][i] = (i == 1) ? 1.f : 0.f;
		defaultLightDirection[2][i] = (i == 0) ? 1.f : 0.f;
		defaultDiffuseCoeff[i] = 0.8f;
		defaultSpecularCoeff[i] = 0.3f;
	}
	defaultAmbientCoeff = 0.1f;
	defaultSpecularExp = 20.f;
	defaultNumLights = 2;
}
//Reinitialize viewpoint settings, when metadata changes.
//Really not much to do!
//If we can override, set to default for current region.
//Note that this should be called after the region is init'ed
//
void ViewpointParams::
Validate(bool doOverride){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	
	setCoordTrans();
	if (doOverride){
		//set to defaults:
		restart();
		
	} 
	
	return;
}
//Rescale viewing parameters when the scene is rescaled by factor
void ViewpointParams::
rescale (double scaleFac[3], int timestep){
	double vtemp[3];
	Viewpoint* vp = getCurrentViewpoint();
	Viewpoint* vph = getHomeViewpoint();
	const vector<double>& vps = vp->getCameraPosLocal();
	const vector<double>& vctr = vp->getRotationCenterLocal();
	vector<double> vtemp2;
	//Want to move the camera in or out, based on scaling in directions orthogonal to view dir.
	for (int i = 0; i<3; i++){
		vtemp[i] = vps[i] - vctr[i];
		vtemp[i] /= scaleFac[i];
		vtemp2.push_back(vtemp[i]+vctr[i]);
	}
	vp->setCameraPosLocal(vtemp2,this);
	//Do same for home viewpoint
	const vector<double>& vpsh = vp->getCameraPosLocal();
	const vector<double>& vctrh = vp->getRotationCenterLocal();
	
	for (int i = 0; i<3; i++){
		vtemp[i] = vpsh[i]-vctrh[i];
		vtemp[i] /= scaleFac[i];
		vtemp2[i] = vtemp[i] + vctrh[i];
	}

	vph->setCameraPosLocal(vtemp2,this);
	
}


void ViewpointParams::
setCoordTrans(){
	const float* strSizes = DataStatus::getInstance()->getFullStretchedSizes();
	
}



//Rotate a vector based on current modelview matrix transpose.  Use to rotate vector in world coords to
//Camera coord system.
void  ViewpointParams::transform3Vector(const float vec[3], float resvec[3])
{
	resvec[0] = modelViewMatrix[0]*vec[0] + modelViewMatrix[1]*vec[1] + modelViewMatrix[2]*vec[2];
	resvec[1] = modelViewMatrix[4]*vec[0] + modelViewMatrix[5]*vec[1] + modelViewMatrix[6]*vec[2];
	resvec[2] = modelViewMatrix[8]*vec[0] + modelViewMatrix[9]*vec[1] + modelViewMatrix[10]*vec[2];
}
//  First project all 8 box corners to the center line of the camera view, finding the furthest and
//  nearest projection in front of the camera.  The furthest distance is used as the far distance.
//  If some point projects behind the camera, then either the camera is inside the box, or a corner of the
//  box is behind the camera.  This calculation is always performed in local coordinates since a translation won't affect
//  the result
void ViewpointParams::
getFarNearDist(float* boxFar, float* boxNear){
	//First check full box
	float extents[6];
	double wrk[3], cor[3], boxcor[3], cmpos[3];
	double camPosBox[3],dvdir[3];
	double maxProj = -1.e30;
	double minProj = 1.e30; 

	const float* exts = DataStatus::getInstance()->getLocalExtents();
	//convert to local extents??
	for (int i = 0; i<6; i++) extents[i] = exts[i]-exts[i%3];

	//Convert camera position and corners to stretched  coordinates
	for (int i = 0; i<3; i++) cmpos[i] = (double)getCameraPosLocal()[i];
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++) camPosBox[i] = cmpos[i]*stretch[i];
	
	
	for (int i = 0;i<3; i++) dvdir[i] = getViewDir()[i];
	vnormal(dvdir);
	
	//For each box corner, 
	//   convert to box coords, then project to line of view
	for (int i = 0; i<8; i++){
		for (int j = 0; j< 3; j++){
			cor[j] = ( (i>>j)&1) ? extents[j+3] : extents[j];
		}
		for (int k=0;k<3;k++) boxcor[i] = cor[i]*stretch[i];
		
		vsub(boxcor, camPosBox, wrk);
		
		float mdist = vdot(wrk, dvdir);
		if (minProj > mdist) {
			minProj = mdist;
		}
		if (maxProj < mdist) {
			maxProj = mdist;
		}
	}

	if (maxProj < 1.e-10) maxProj = 1.e-10;
	if (minProj > 0.03*maxProj) minProj = 0.03*maxProj;
	//minProj will be < 0 if either the camera is in the box, or
	//if some of the region is behind the camera plane.  In that case, just
	//set the nearDist a reasonable multiple of the fardist
	if (minProj <= 0.0) minProj = 0.002*maxProj;
	*boxFar = (float)maxProj;
	*boxNear = (float)minProj;
	
	return;
}
void ViewpointParams::
centerFullRegion(int timestep){
	//Find the largest of the dimensions of the current region:
	const float* fullExtent = DataStatus::getInstance()->getLocalExtents();

	float maxSide = Max(fullExtent[5]-fullExtent[2], 
		Max(fullExtent[4]-fullExtent[1],
		fullExtent[3]-fullExtent[0]));
	//calculate the camera position: center - 2.5*viewDir*maxSide;
	//Position the camera 2.5*maxSide units away from the center, aimed
	//at the center.
	//But divide it by stretch factors
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	Viewpoint* currentViewpoint = getCurrentViewpoint();
	//Make sure the viewDir is normalized:
	double viewDir[3];
	for (int j = 0; j<3; j++) viewDir[j] = currentViewpoint->getViewDir()[j];
	vnormal(viewDir);
	Command* cmd = Command::CaptureStart(this,"Center view on region");
	
	for (int i = 0; i<3; i++){
		float dataCenter = 0.5f*(fullExtent[i+3]-fullExtent[i]);
		float camPosCrd = dataCenter -2.5*maxSide*viewDir[i]/stretch[i];
		currentViewpoint->setCameraPosLocal(i, camPosCrd,this);
		currentViewpoint->setRotationCenterLocal(i, dataCenter,this);
	}
	
	Command::CaptureEnd(cmd,this);
	
}