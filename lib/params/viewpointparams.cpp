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
float VAPoR::ViewpointParams::maxCubeSide = 1.f;
float VAPoR::ViewpointParams::minCubeCoord[3] = {0.f,0.f,0.f};
float VAPoR::ViewpointParams::maxCubeCoord[3] = {1.f,1.f,1.f};

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
	//at the center
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	for (int i = 0; i<3; i++){
		float dataCenter = 0.5f*(fullExtent[i+3]+fullExtent[i]);
		float camPosCrd = dataCenter -2.5*maxSide*currentViewpoint->getViewDir(i);
		currentViewpoint->setCameraPos(i, camPosCrd);
		setRotationCenter(i,dataCenter);
	}
}

//Reinitialize viewpoint settings, to center view on the center of full region.
//(this is starting state)
void ViewpointParams::
restart(){
	
	numLights = 1;
	lightDirection[0][0] = 0.f;
	lightDirection[0][1] = 0.f;
	lightDirection[0][2] = 1.f;
	lightDirection[1][0] = 0.f;
	lightDirection[1][1] = 1.f;
	lightDirection[1][2] = 0.f;
	lightDirection[2][0] = 0.f;
	lightDirection[2][1] = 0.f;
	lightDirection[2][2] = 1.f;
	//final component is 0 (for gl directional light)
	lightDirection[0][3] = 0.f;
	lightDirection[1][3] = 0.f;
	lightDirection[2][3] = 0.f;
	diffuseCoeff[0] = 0.8f;
	specularCoeff[0] = 0.5f;
	specularExp = 10.f;
	diffuseCoeff[1] = 0.8f;
	specularCoeff[1] = 0.5f;
	diffuseCoeff[2] = 0.8f;
	specularCoeff[2] = 0.5f;
	ambientCoeff = 0.1f;
	if (currentViewpoint) delete currentViewpoint;
	currentViewpoint = new Viewpoint();
	//Set default values in viewpoint:
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
//Static methods for converting between world coords and unit cube coords:
//
void ViewpointParams::
worldToCube(const float fromCoords[3], float toCoords[3]){
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]-minCubeCoord[i])/maxCubeSide;
	}
	return;
}

void ViewpointParams::
worldFromCube(float fromCoords[3], float toCoords[3]){
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]*maxCubeSide) + minCubeCoord[i];
	}
	return;
}
void ViewpointParams::
setCoordTrans(){
	if (!DataStatus::getInstance()) return;
	const float* extents = DataStatus::getInstance()->getExtents();
	maxCubeSide = -1.f;
	int i;
	//find largest cube side, it will map to 1.0
	for (i = 0; i<3; i++) {
		if ((float)(extents[i+3]-extents[i]) > maxCubeSide) maxCubeSide = (float)(extents[i+3]-extents[i]);
		minCubeCoord[i] = (float)extents[i];
		maxCubeCoord[i] = (float)extents[i+3];
	}
}
//Find far and near dist along view direction. 
//Far is distance from camera to furthest corner of full volume
//Near is distance to full volume if it's positive, otherwise
//is distance to closest corner of region.
//
void ViewpointParams::
getFarNearDist(RegionParams* rParams, float* fr, float* nr){
	//First check full box
	const float* extents = DataStatus::getInstance()->getExtents();
	float wrk[3], cor[3];
	float maxProj = -.1e30f;
	float minProj = 1.e30f;
	//Make sure the viewDir is normalized:
	vnormal(currentViewpoint->getViewDir());
	//project corners of full box in view direction:
	for (int i = 0; i<8; i++){
		for (int j = 0; j< 3; j++){
			cor[j] = ( (i>>j)&1) ? extents[j+3] : extents[j];
		}
		vsub(cor, getCameraPos(), wrk);
		//float len1 = wrk[0]*getViewDir()[0]+wrk[1]*getViewDir()[1]+wrk[2]*getViewDir()[2];
		float len = vdot(wrk, getViewDir());
		//check for max and min of len's
		if (len > maxProj) maxProj = len;
		if (len < minProj) minProj = len;
	}
	if ((minProj < 1.e-20f) || (maxProj <= minProj)) {//big box is not far enough away
		//Calculate distances to region corners:
		float corners[8][3];
		rParams->calcBoxCorners(corners);
		minProj = 1.e30f;
		//Project each corner along the line in the view direction
		for (int i = 0; i< 8; i++){
			//first subtract a corner from the camera, and 
			//project the resulting vector in the camera direction 
			vsub(corners[i],getCameraPos(),wrk);
			float len = vdot(wrk,getViewDir());
			//check for min of len's
			if (len < minProj) minProj = len;
		}
		//ensure far > near
		if (maxProj < 2.f*minProj) maxProj = 2.f*minProj;

	}
	//Now these coords must be stretched based on cube coord system extents
	float maxCoord = Max(Max(maxCubeCoord[0],maxCubeCoord[1]),maxCubeCoord[2]);

	*fr = maxProj/maxCoord;
	*nr = minProj/maxCoord;
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
			if (StrCmpNoCase(attribName, _ambientLightAttr) == 0) {
				ist >> ambientCoeff;
			}
			else if (StrCmpNoCase(attribName, _specularExponentAttr) == 0){
				ist >>specularExp;
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
