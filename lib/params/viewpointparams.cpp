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

float VAPoR::ViewpointParams::maxStretchedCubeSide = 1.f;


float VAPoR::ViewpointParams::minStretchedCubeCoord[3] = {0.f,0.f,0.f};
float VAPoR::ViewpointParams::maxStretchedCubeCoord[3] = {1.f,1.f,1.f};

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
const string ViewpointParams::_lightDirectionAttr = "LightDirection";
const string ViewpointParams::_lightNumAttr = "LightNum";
const string ViewpointParams::_diffuseLightAttr = "DiffuseCoefficient";
const string ViewpointParams::_ambientLightAttr = "AmbientCoefficient";
const string ViewpointParams::_specularLightAttr = "SpecularCoefficient";
const string ViewpointParams::_specularExponentAttr = "SpecularExponent";
const string ViewpointParams::_stereoModeAttr = "StereoMode";
const string ViewpointParams::_stereoSeparationAttr = "StereoSeparation";

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




void ViewpointParams::
centerFullRegion(int timestep){
	//Find the largest of the dimensions of the current region:
	float fullExtent[6];
	DataStatus::getInstance()->getLocalExtentsCartesian(fullExtent);

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
		float dataCenter = 0.5f*(fullExtent[i+3]-fullExtent[i]);
		float camPosCrd = dataCenter -2.5*maxSide*currentViewpoint->getViewDir(i)/stretch[i];
		currentViewpoint->setCameraPosLocal(i, camPosCrd);
		currentViewpoint->setRotationCenterLocal(i, dataCenter);
	}
	if (useLatLon) {
		convertLocalToLonLat(timestep);
	}
}

//Reinitialize viewpoint settings, to center view on the center of full region.
//(this is starting state)
void ViewpointParams::
restart(){
	useLatLon = false;
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
	//Set default values in viewpoint:
	stereoSeparation = 0.f;
	stereoMode = 0; //Center view
	setCamPosLatLon(0.f, 0.f);
	setRotCenterLatLon(0.f,0.f);
	
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
		
	
		centerFullRegion(0);
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
	} else { //possible translation if old session file was used...
		if (DataStatus::pre22Session()){
			float * offset = DataStatus::getPre22Offset();
			float* cpos = getCurrentViewpoint()->getCameraPosLocal();
			float* lpos = getHomeViewpoint()->getCameraPosLocal();
			float* rpos = getCurrentViewpoint()->getRotationCenterLocal();
			float* rhpos = getHomeViewpoint()->getRotationCenterLocal();
			//In old session files, the coordinate of box extents were not 0-based
			for (int i = 0; i<3; i++) {
				cpos[i] -= offset[i];
				lpos[i] -= offset[i];
				rpos[i] -= offset[i];
				rhpos[i] -= offset[i];
			}
			getCurrentViewpoint()->setCameraPosLocal(cpos);
			getHomeViewpoint()->setCameraPosLocal(lpos);
			getCurrentViewpoint()->setRotationCenterLocal(rpos);
			getHomeViewpoint()->setRotationCenterLocal(rhpos);
			//Modify so that rotation centers are in view center (required for viewpoint animation)
			getHomeViewpoint()->alignCenter();
			getCurrentViewpoint()->alignCenter();
		}
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
	//Convert to lonlat
	if (useLatLon)
		convertLocalToLonLat(timestep);
}


//Static methods for converting between local world coords and stretched unit cube coords
//

void ViewpointParams::
localToStretchedCube(const float fromCoords[3], float toCoords[3]){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]*stretch[i])/maxStretchedCubeSide;
	}
	return;
}
//Static methods for converting between local coords and stretched unit cube coords
//
void ViewpointParams::
localToStretchedCube(const double fromCoords[3], double toCoords[3]){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++){
		toCoords[i] = (fromCoords[i]*stretch[i])/maxStretchedCubeSide;
	}
	return;
}


void ViewpointParams::
localFromStretchedCube(float fromCoords[3], float toCoords[3]){
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<3; i++){
		toCoords[i] = ((fromCoords[i]*maxStretchedCubeSide))/stretch[i];
	}
	return;
}
void ViewpointParams::
setCoordTrans(){
	if (!DataStatus::getInstance()) return;
	const float* strSizes = DataStatus::getInstance()->getFullStretchedSizes();
	
	maxStretchedCubeSide = -1.f;
	
	int i;
	//find largest cube side, it will map to 1.0
	for (i = 0; i<3; i++) {
		
		if (strSizes[i] > maxStretchedCubeSide) maxStretchedCubeSide = (float)strSizes[i];
		minStretchedCubeCoord[i] = 0.;
		maxStretchedCubeCoord[i] = strSizes[i];
	}
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

	DataStatus::getInstance()->getLocalExtentsCartesian(extents);
	//convert to local extents
	for (int i = 0; i<6; i++) extents[i] -= extents[i%3];

	//Convert camera position and corners to stretched box coordinates
	for (int i = 0; i<3; i++) cmpos[i] = (double)getCameraPosLocal()[i];
	localToStretchedCube(cmpos, camPosBox);
	
	float* vdir = getViewDir();
	
	for (int i = 0;i<3; i++) dvdir[i] = vdir[i];
	vnormal(dvdir);
	
	//For each box corner, 
	//   convert to box coords, then project to line of view
	for (int i = 0; i<8; i++){
		for (int j = 0; j< 3; j++){
			cor[j] = ( (i>>j)&1) ? extents[j+3] : extents[j];
		}
		localToStretchedCube(cor, boxcor);
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


bool ViewpointParams::
elementStartHandler(ExpatParseMgr* pm, int  depth , std::string& tagString, const char ** attrs){
	//Get the attributes, make the viewpoints parse the children
	//Lights get parsed at the end of lightDirection tag

	
	if (StrCmpNoCase(tagString, _viewpointParamsTag) == 0) {
		//If it's a viewpoint tag, save 2 from Params class
		//Do this by repeatedly pulling off the attribute name and value
		useLatLon = false;
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
			else if (StrCmpNoCase(attribName, _latLonAttr) == 0) {
				if (value == "true") setLatLon(true);
				else setLatLon(false);
			}
			//Possible values are center, left, right (0,1,2)
			else if (StrCmpNoCase(attribName, _stereoModeAttr) == 0){
				if (value == "left") setStereoMode(1);
				else if (value == "right") setStereoMode(2);
				else setStereoMode(0);
			}
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
//		currentViewpoint->elementStartHandler(pm, depth, tagString, attrs);
		return true;
	}
	else if (StrCmpNoCase(tagString, _homeViewTag) == 0) {
		//Need to "push" to viewpoint parser.
		//That parser will "pop" back to viewpointparams when done.
		pm->pushClassStack(homeViewpoint);
//		homeViewpoint->elementStartHandler(pm, depth, tagString, attrs);
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
cerr <<"SKIPPING " << tagString << endl;
	pm->skipElement(tagString, depth);
	return true;
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
ParamNode* ViewpointParams::
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
	if (isLatLon())
		oss << "true";
	else 
		oss << "false";
	attrs[_latLonAttr] = oss.str();

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

	ParamNode* vpParamsNode = new ParamNode(_viewpointParamsTag, attrs, 2);
	
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
	ParamNode* currVP = new ParamNode(_currentViewTag, attrs, 1);
	vpParamsNode->AddChild(currVP);
	currVP->AddChild(currentViewpoint->buildNode());
	ParamNode* homeVP = new ParamNode(_homeViewTag, attrs, 1);
	vpParamsNode->AddChild(homeVP);
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
bool ViewpointParams::
convertLocalFromLonLat(int timestep){
//Convert the latlon coordinates for this time step.
//Note that the latlon coords are in the order Lat,lon
	double coords[4];
	
	coords[0] = getCamPosLatLon()[1];
	coords[1] = getCamPosLatLon()[0];
	coords[2] = getRotCenterLatLon()[1];
	coords[3] = getRotCenterLatLon()[0];
	
	bool ok = DataStatus::convertLocalFromLonLat(timestep, coords,2);
	if (ok){
		currentViewpoint->setCameraPosLocal(0, (float)(coords[0]));
		currentViewpoint->setCameraPosLocal(1, (float)(coords[1]));
		currentViewpoint->setRotationCenterLocal(0,(float)(coords[2]));
		currentViewpoint->setRotationCenterLocal(1,(float)(coords[3]));
	}
	return ok;
}
//Convert local extents, (timestep is >= 0)

bool ViewpointParams::
convertLocalToLonLat(int timestep){
	assert (timestep >= 0);
//Convert the latlon coordinates for this time step:
	double coords[4];
	
	coords[0] = getCameraPosLocal(0);
	coords[1] = getCameraPosLocal(1);
	coords[2] = getRotationCenterLocal(0);
	coords[3] = getRotationCenterLocal(1);
	
	bool ok = DataStatus::convertLocalToLonLat(timestep, coords,2);
	if (ok){
		setCamPosLatLon((float)coords[1],(float)coords[0]);
		setRotCenterLatLon((float)coords[3],(float)coords[2]);
	}
	return ok;
}
