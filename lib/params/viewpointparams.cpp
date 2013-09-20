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


float VAPoR::ViewpointParams::defaultUpVec[3] = {0.f, 1.f, 0.f};
float VAPoR::ViewpointParams::defaultViewDir[3] = {0.f, 0.f, -1.f};
float VAPoR::ViewpointParams::defaultLightDirection[3][3] = {{0.f, 0.f, 1.f},{0.f, 1.f, 0.f},{1.f, 0.f, 0.f}};
int VAPoR::ViewpointParams::defaultNumLights = 2;
float VAPoR::ViewpointParams::defaultDiffuseCoeff[3] = {0.8f, 0.8f, 0.8f};
float VAPoR::ViewpointParams::defaultSpecularCoeff[3] = {0.3f, 0.3f, 0.3f};
float VAPoR::ViewpointParams::defaultAmbientCoeff = 0.1f;
float VAPoR::ViewpointParams::defaultSpecularExp = 20.f;


using namespace VAPoR;

const string ViewpointParams::_shortName = "View";
const string ViewpointParams::_latLonAttr = "UseLatLon";
const string ViewpointParams::_currentViewTag = "CurrentViewpoint";
const string ViewpointParams::_homeViewTag = "HomeViewpoint";
const string ViewpointParams::_lightTag = "Light";


ViewpointParams::ViewpointParams(int winnum): Params(winnum, Params::_viewpointParamsTag){
	
	
	homeViewpoint = 0;
	currentViewpoint = 0;
	restart();
	
}
Params* ViewpointParams::
deepCopy(ParamNode*){
	ViewpointParams* p = new ViewpointParams(*this);
	p->currentViewpoint = new Viewpoint(*currentViewpoint);
	p->homeViewpoint = new Viewpoint(*homeViewpoint);
	
	return (Params*)(p);
}
ViewpointParams::~ViewpointParams(){
	
	delete currentViewpoint;
	delete homeViewpoint;
}





//Reinitialize viewpoint settings, to center view on the center of full region.
//(this is starting state)
void ViewpointParams::
restart(){
	
	numLights = defaultNumLights;
	for (int i = 0; i<3; i++){
		for (int j = 0; j<3; j++){
			lightDirection[i][j] = defaultLightDirection[i][j];
		}
		//final component is 0 (for gl directional light)
		lightDirection[i][3] = 0.f;
		diffuseCoeff[i] = defaultDiffuseCoeff[i];
		specularCoeff[i] = defaultSpecularCoeff[i];
	}
	
	specularExp = defaultSpecularExp;
	ambientCoeff = defaultAmbientCoeff;

	if (currentViewpoint) delete currentViewpoint;
	currentViewpoint = new Viewpoint();
	
	if (homeViewpoint) delete homeViewpoint;
	homeViewpoint = new Viewpoint(*currentViewpoint);
	
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
bool ViewpointParams::
reinit(bool doOverride){
	
	setCoordTrans();
	if (doOverride){
		setViewDir(defaultViewDir);
		setUpVec(defaultUpVec);
		
	
		//Set the home viewpoint, but don't call setHomeViewpoint().
		delete homeViewpoint;
		homeViewpoint = new Viewpoint(*currentViewpoint);
		//set lighting to defaults:
		for (int i = 0; i<3; i++){
			for (int j = 0; j<3; j++){
				lightDirection[i][j] = defaultLightDirection[i][j];
			}
			//final component is 0 (for gl directional light)
			lightDirection[i][3] = 0.f;
			diffuseCoeff[i] = defaultDiffuseCoeff[i];
			specularCoeff[i] = defaultSpecularCoeff[i];
		}
	
		specularExp = defaultSpecularExp;
		ambientCoeff = defaultAmbientCoeff;
	} 
	return true;
}
//Rescale viewing parameters when the scene is rescaled by factor
void ViewpointParams::
rescale (float scaleFac[3], int timestep){
	float vtemp[3], vtemp2[3];
	Viewpoint* vp = getCurrentViewpoint();
	Viewpoint* vph = getHomeViewpoint();
	float* vps = vp->getCameraPosLocal();
	float* vctr = vp->getRotationCenterLocal();
	vsub(vps, vctr, vtemp);
	//Want to move the camera in or out, based on scaling in directions orthogonal to view dir.
	for (int i = 0; i<3; i++){
		vtemp[i] /= scaleFac[i];
	}
	vadd(vtemp, vctr, vtemp2);
	vp->setCameraPosLocal(vtemp2);
	//Do same for home viewpoint
	vps = vph->getCameraPosLocal();
	vctr = vph->getRotationCenterLocal();
	vsub(vps, vctr, vtemp);
	for (int i = 0; i<3; i++){
		vtemp[i] /= scaleFac[i];
	}
	vadd(vtemp, vctr, vtemp2);
	vph->setCameraPosLocal(vtemp2);
	
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
