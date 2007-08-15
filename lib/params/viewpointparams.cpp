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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "viewpointparams.h"
#include "params.h"
#include "glutil.h"
#include "viewpoint.h"
#include "vapor/XmlNode.h"
#include "datastatus.h"
#include "regionparams.h"

float VAPoR::ViewpointParams::maxStretchedCubeSide = 1.f;


float VAPoR::ViewpointParams::minStretchedCubeCoord[3] = {0.f,0.f,0.f};
float VAPoR::ViewpointParams::maxStretchedCubeCoord[3] = {1.f,1.f,1.f};

using namespace VAPoR;
const string ViewpointParams::_currentViewTag = "CurrentViewpoint";
const string ViewpointParams::_homeViewTag = "HomeViewpoint";
const string ViewpointParams::_lightTag = "Light";
const string ViewpointParams::_lightDirectionAttr = "LightDirection";
const string ViewpointParams::_lightNumAttr = "LightNum";
const string ViewpointParams::_diffuseLightAttr = "DiffuseCoefficient";
const string ViewpointParams::_ambientLightAttr = "AmbientCoefficient";
const string ViewpointParams::_specularLightAttr = "SpecularCoefficient";
const string ViewpointParams::_specularExponentAttr = "SpecularExponent";
const string ViewpointParams::_stereoModeAttr = "StereoMode";
const string ViewpointParams::_stereoSeparationAttr = "StereoSeparation";

ViewpointParams::ViewpointParams(int winnum): Params(winnum){
	thisParamType = ViewpointParamsType;
	
	homeViewpoint = 0;
	currentViewpoint = 0;
	restart();
	
}
Params* ViewpointParams::
deepCopy(){
	ViewpointParams* p = new ViewpointParams(*this);
	p->currentViewpoint = new Viewpoint(*currentViewpoint);
	p->homeViewpoint = new Viewpoint(*homeViewpoint);
	
	return (Params*)(p);
}
ViewpointParams::~ViewpointParams(){
	
	delete currentViewpoint;
	delete homeViewpoint;
}




void ViewpointParams::
centerFullRegion(){
	//Find the largest of the dimensions of the current region:
	const float * fullExtent = DataStatus::getInstance()->getExtents();
	float maxSide = Max(fullExtent[5]-fullExtent[2], 
		Max(fullExtent[4]-fullExtent[1],
		fullExtent[3]-fullExtent[0]));
	//calculate the camera position: center - 2.5*viewDir*maxSide;
	//Position the camera 2.5*maxSide units away from the center, aimed
	//at the center.
	//But divide it by stretch factors
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	for (int i = 0; i<3; i++){ 
		float dataCenter = 0.5f*(fullExtent[i+3]+fullExtent[i]);
		float camPosCrd = dataCenter -2.5*maxSide*currentViewpoint->getViewDir(i)/stretch[i];
		currentViewpoint->setCameraPos(i, camPosCrd);
		setRotationCenter(i,dataCenter);
	}
}

//Reinitialize viewpoint settings, to center view on the center of full region.
//(this is starting state)
void ViewpointParams::
restart(){
	
	numLights = 2;
	lightDirection[0][0] = 0.f;
	lightDirection[0][1] = 0.f;
	lightDirection[0][2] = 1.f;
	lightDirection[1][0] = 0.f;
	lightDirection[1][1] = 1.f;
	lightDirection[1][2] = 0.f;
	lightDirection[2][0] = 1.f;
	lightDirection[2][1] = 0.f;
	lightDirection[2][2] = 0.f;
	//final component is 0 (for gl directional light)
	lightDirection[0][3] = 0.f;
	lightDirection[1][3] = 0.f;
	lightDirection[2][3] = 0.f;
	diffuseCoeff[0] = 0.8f;
	specularCoeff[0] = 0.3f;
	specularExp = 20.f;
	diffuseCoeff[1] = 0.8f;
	specularCoeff[1] = 0.3f;
	diffuseCoeff[2] = 0.8f;
	specularCoeff[2] = 0.3f;
	ambientCoeff = 0.1f;
	if (currentViewpoint) delete currentViewpoint;
	currentViewpoint = new Viewpoint();
	//Set default values in viewpoint:
	stereoSeparation = 0.f;
	stereoMode = 0; //Center view
	currentViewpoint->setPerspective(true);
	for (int i = 0; i< 3; i++){
		setCameraPos(i,0.5f);
		setViewDir(i,0.f);
		setUpVec(i, 0.f);
		setRotationCenter(i,0.5f);
	}
	setUpVec(1,1.f);
	setViewDir(2,-1.f); 
	setCameraPos(2,2.5f);
	if (homeViewpoint) delete homeViewpoint;
	homeViewpoint = new Viewpoint(*currentViewpoint);
	
	setCoordTrans();
	
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
		setViewDir(0,0.f);
		setViewDir(1,0.f);
		setUpVec(0,0.f);
		setUpVec(1,0.f);
		setUpVec(2,1.f);
		setViewDir(2,-1.f);
		centerFullRegion();
		//Set the home viewpoint, but don't call setHomeViewpoint().
		delete homeViewpoint;
		homeViewpoint = new Viewpoint(*currentViewpoint);
	}
	
	return true;
}
//Rescale viewing parameters when the scene is rescaled by factor
void ViewpointParams::
rescale (float scaleFac[3]){
	float vtemp[3], vtemp2[3];
	Viewpoint* vp = getCurrentViewpoint();
	Viewpoint* vph = getHomeViewpoint();
	float* vps = vp->getCameraPos();
	float* vctr = vp->getRotationCenter();
	vsub(vps, vctr, vtemp);
	//Want to move the camera in or out, based on scaling in directions orthogonal to view dir.
	for (int i = 0; i<3; i++){
		vtemp[i] /= scaleFac[i];
	}
	vadd(vtemp, vctr, vtemp2);
	vp->setCameraPos(vtemp2);
	//Do same for home viewpoint
	vps = vph->getCameraPos();
	vctr = vph->getRotationCenter();
	vsub(vps, vctr, vtemp);
	for (int i = 0; i<3; i++){
		vtemp[i] /= scaleFac[i];
	}
	vadd(vtemp, vctr, vtemp2);
	vph->setCameraPos(vtemp2);
}


//Static methods for converting between world coords and stretched unit cube coords
//
void ViewpointParams::
worldToStretchedCube(const float fromCoords[3], float toCoords[3]){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]*stretch[i]-minStretchedCubeCoord[i])/maxStretchedCubeSide;
	}
	return;
}



void ViewpointParams::
worldFromStretchedCube(float fromCoords[3], float toCoords[3]){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++){
		toCoords[i] = ((fromCoords[i]*maxStretchedCubeSide) + minStretchedCubeCoord[i])/stretch[i];
	}
	return;
}
void ViewpointParams::
setCoordTrans(){
	if (!DataStatus::getInstance()) return;
	const float* strExtents = DataStatus::getInstance()->getStretchedExtents();
	
	maxStretchedCubeSide = -1.f;
	
	int i;
	//find largest cube side, it will map to 1.0
	for (i = 0; i<3; i++) {
		
		if ((float)(strExtents[i+3]-strExtents[i]) > maxStretchedCubeSide) maxStretchedCubeSide = (float)(strExtents[i+3]-strExtents[i]);
		minStretchedCubeCoord[i] = (float)strExtents[i];
		maxStretchedCubeCoord[i] = (float)strExtents[i+3];
	}
}
//Find far and near dist along view direction, in world coordinates 
//Far is distance from camera to furthest corner of full volume
//Near is distance to full volume if it's positive, otherwise
//is distance to closest corner of region.
//
void ViewpointParams::
getFarNearDist(RegionParams* rParams, float* fr, float* nr){
	//First check full box
	const float* extents = DataStatus::getInstance()->getStretchedExtents();
	float wrk[3], cor[3];
	float maxProj = -.1e30f;
	float minProj = 1.e30f;
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	for (int i = 0; i<8; i++){
		for (int j = 0; j< 3; j++){
			cor[j] = ( (i>>j)&1) ? extents[j+3] : extents[j];
		}
		vsub(cor, getCameraPos(), wrk);
		float dist = vlength(wrk);
		if (minProj > dist) minProj = dist;
		if (maxProj < dist) maxProj = dist;
	}
	if (maxProj < 1.e-10f) maxProj = 1.e-10f;
	if (minProj > 0.03f*maxProj) minProj = 0.03f*maxProj;
	
	*fr = maxProj;
	*nr = minProj;
	return;
}


bool ViewpointParams::
elementStartHandler(ExpatParseMgr* pm, int  depth , std::string& tagString, const char ** attrs){
	//Get the attributes, make the viewpoints parse the children
	//Lights get parsed at the end of lightDirection tag
	if (StrCmpNoCase(tagString, _viewpointParamsTag) == 0) {
		//If it's a viewpoint tag, save 2 from Params class
		//Do this by repeatedly pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			else if (StrCmpNoCase(attribName, _localAttr) == 0) {
				if (value == "true") setLocal(true); else setLocal(false);
			}
			else if (StrCmpNoCase(attribName, _ambientLightAttr) == 0) {
				ist >> ambientCoeff;
			}
			else if (StrCmpNoCase(attribName, _specularExponentAttr) == 0){
				ist >> specularExp;
			}
			else if (StrCmpNoCase(attribName, _stereoSeparationAttr) == 0){
				ist >> stereoSeparation;
			}
			//Possible values are center, left, right (0,1,2)
			else if (StrCmpNoCase(attribName, _stereoModeAttr) == 0){
				if (value == "left") setStereoMode(1);
				else if (value == "right") setStereoMode(2);
				else setStereoMode(0);
			}
			else return false;
		}
		//Start by assuming no lights...
		parsingLightNum = -1;  
		numLights = 0;
		return true;
	}
	//Parse current and home viewpoints
	else if (StrCmpNoCase(tagString, _currentViewTag) == 0) {
		//Need to "push" to viewpoint parser.
		//That parser will "pop" back to viewpointparams when done.
		pm->pushClassStack(currentViewpoint);
		currentViewpoint->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else if (StrCmpNoCase(tagString, _homeViewTag) == 0) {
		//Need to "push" to viewpoint parser.
		//That parser will "pop" back to viewpointparams when done.
		pm->pushClassStack(homeViewpoint);
		homeViewpoint->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else if (StrCmpNoCase(tagString, _lightTag) == 0) {
		double dir[3];
		float diffCoeff = 0.8f;
		float specCoeff = 0.5f;
		//Get the lightNum and direction
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _lightNumAttr) == 0) {
				ist >> parsingLightNum;
				//Make sure that numLights is > lightNum
				if(numLights <= parsingLightNum) numLights = parsingLightNum+1;
			}
			else if (StrCmpNoCase(attribName, _lightDirectionAttr) == 0) {
				ist >> dir[0]; ist >> dir[1]; ist >>dir[2];
			}
			else if (StrCmpNoCase(attribName, _diffuseLightAttr) == 0){
				ist >>diffCoeff;
			}
			else if (StrCmpNoCase(attribName, _specularLightAttr) == 0){
				ist >>specCoeff;
			}
			else return false;
		}// We should now have a lightnum and a direction
		if (parsingLightNum < 0 || parsingLightNum >2) return false;
		lightDirection[parsingLightNum][0] = (float)dir[0];
		lightDirection[parsingLightNum][1] = (float)dir[1];
		lightDirection[parsingLightNum][2] = (float)dir[2];
		lightDirection[parsingLightNum][3] = 0.f;
		diffuseCoeff[parsingLightNum] = diffCoeff;
		specularCoeff[parsingLightNum] = specCoeff;
		return true;
	}
	else return false;
}
bool ViewpointParams::
elementEndHandler(ExpatParseMgr* pm, int depth, std::string& tag){
	if (StrCmpNoCase(tag, _viewpointParamsTag) == 0) {
		//If this is a viewpointparams, need to
		//pop the parse stack.  
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, _homeViewTag) == 0){
		return true;
	} else if (StrCmpNoCase(tag, _currentViewTag) == 0){
		return true;
	} else if (StrCmpNoCase(tag, _lightTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in ViewpointParams %s",tag.c_str());
		return false;  //Could there be other end tags that we ignore??
	}
}
XmlNode* ViewpointParams::
buildNode(){
	//Construct the viewpoint node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	if (local)
		oss << "true";
	else 
		oss << "false";
	attrs[_localAttr] = oss.str();

	oss.str(empty);
	oss << (double)ambientCoeff;
	attrs[_ambientLightAttr] = oss.str();
	oss.str(empty);
	oss << (long) specularExp;
	attrs[_specularExponentAttr] = oss.str();
	oss.str(empty);
	oss << (double)stereoSeparation;
	attrs[_stereoSeparationAttr] = oss.str();
	oss.str(empty);
	if (stereoMode == 1) oss << "left";
	else if (stereoMode == 2) oss << "right";
	else oss << "center";
	attrs[_stereoModeAttr] = oss.str();

	XmlNode* vpParamsNode = new XmlNode(_viewpointParamsTag, attrs, 2);
	
	//Now add children: lights, and home and current viewpoints:
	//There is one light node for each light source
	
	
	for (int i = 0; i< numLights; i++){
		attrs.clear();
		oss.str(empty);
		oss << (long)i;
		attrs[_lightNumAttr] = oss.str();
		oss.str(empty);
		for (int j = 0; j< 3; j++){
			oss << (double)(lightDirection[i][j]);
			oss <<" ";
		}
		attrs[_lightDirectionAttr] = oss.str();
		oss.str(empty);
		oss << (double) diffuseCoeff[i];
		attrs[_diffuseLightAttr] = oss.str();
		oss.str(empty);
		oss << (double) specularCoeff[i];
		attrs[_specularLightAttr] = oss.str();
		vpParamsNode->NewChild(_lightTag, attrs, 0);
	}
	attrs.clear();
	XmlNode* currVP = vpParamsNode->NewChild(_currentViewTag, attrs, 1);
	currVP->AddChild(currentViewpoint->buildNode());
	XmlNode* homeVP = vpParamsNode->NewChild(_homeViewTag, attrs, 1);
	homeVP->AddChild(homeViewpoint->buildNode());
	return vpParamsNode;
}
//Rotate a vector based on current modelview matrix transpose.  Use to rotate vector in world coords to
//Camera coord system.
void  ViewpointParams::transform3Vector(const float vec[3], float resvec[3])
{
	resvec[0] = modelViewMatrix[0]*vec[0] + modelViewMatrix[1]*vec[1] + modelViewMatrix[2]*vec[2];
	resvec[1] = modelViewMatrix[4]*vec[0] + modelViewMatrix[5]*vec[1] + modelViewMatrix[6]*vec[2];
	resvec[2] = modelViewMatrix[8]*vec[0] + modelViewMatrix[9]*vec[1] + modelViewMatrix[10]*vec[2];
}
