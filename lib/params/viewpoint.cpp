//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viewpoint.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 2005
//
//	Description:	Implements the Viewpoint class
//		This class contains the parameters associated with one viewpoint
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

#include "viewpoint.h"
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include "glutil.h"



using namespace VAPoR;
using namespace VetsUtil;
const string Viewpoint::_viewpointTag = "Viewpoint";
const string Viewpoint::_camPosTag = "CameraPosition";
const string Viewpoint::_viewDirTag = "ViewDirection";
const string Viewpoint::_upVecTag = "UpVector";
const string Viewpoint::_rotCenterTag = "RotationCenter";
const string Viewpoint::_rotCenterLatLonTag = "RotCenterLatLon";
const string Viewpoint::_camLatLonTag = "CameraLatLon";
const string Viewpoint::_perspectiveAttr = "Perspective";


bool Viewpoint::
elementStartHandler(ExpatParseMgr* pm, int depth, std::string& tagString, const char ** attrs){
	if (StrCmpNoCase(tagString, _viewpointTag) == 0) {
		//If it's a viewpoint tag, save 1 attribute
		//Do this by pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			
			if (StrCmpNoCase(attribName, _perspectiveAttr) == 0) {
				if (value == "true") perspective = true; else perspective = false;
			}
		}
		return true;
	}
	//prepare for data nodes
	else if ((StrCmpNoCase(tagString, _camPosTag) == 0) ||
			(StrCmpNoCase(tagString, _viewDirTag) == 0) ||
			(StrCmpNoCase(tagString, _upVecTag) == 0) ||
			(StrCmpNoCase(tagString, _camLatLonTag) == 0) ||
			(StrCmpNoCase(tagString, _rotCenterLatLonTag) == 0) ||
			(StrCmpNoCase(tagString, _rotCenterTag) == 0) ){
		//Should have a double type attribute
		string attribName = *attrs;
		attrs++;
		string value = *attrs;

		ExpatStackElement *state = pm->getStateStackTop();
		
		state->has_data = 1;
		if (StrCmpNoCase(attribName, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", attribName.c_str());
			return false;
		}
		if (StrCmpNoCase(value, _doubleType) != 0) {
			pm->parseError("Invalid type : %s", value.c_str());
			return false;
		}
		state->data_type = value;
		return true;  
	}
cerr << "XXX Skipping " << tagString << endl;
	pm->skipElement(tagString, depth);
	return(true);
}
bool Viewpoint::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	if (StrCmpNoCase(tag, _viewpointTag) == 0) {
		//If this is a viewpoint, need to
		//pop the parse stack. No need to reference the caller 
		pm->popClassStack();
		return true;
	} else if (StrCmpNoCase(tag, _camPosTag) == 0){
		vector<double> posn = pm->getDoubleData();
		cameraPosition[0] = (float)posn[0];
		cameraPosition[1] = (float)posn[1];
		cameraPosition[2] = (float)posn[2];
		return true;
	} else if (StrCmpNoCase(tag, _camLatLonTag) == 0){
		vector<double> posn = pm->getDoubleData();
		camLatLon[0] = (float)posn[0];
		camLatLon[1] = (float)posn[1];
		return true;
	} else if (StrCmpNoCase(tag, _rotCenterLatLonTag) == 0){
		vector<double> posn = pm->getDoubleData();
		rotCenterLatLon[0] = (float)posn[0];
		rotCenterLatLon[1] = (float)posn[1];
		return true;
	}
	else if (StrCmpNoCase(tag, _viewDirTag) == 0){
		vector<double> vec = pm->getDoubleData();
		viewDirection[0] = (float)vec[0];
		viewDirection[1] = (float)vec[1];
		viewDirection[2] = (float)vec[2];
		return true;
	} else if (StrCmpNoCase(tag, _upVecTag) == 0){
		vector<double> vec = pm->getDoubleData();
		upVector[0] = (float)vec[0];
		upVector[1] = (float)vec[1];
		upVector[2] = (float)vec[2];
		return true;
	} else if (StrCmpNoCase(tag, _rotCenterTag) == 0){
		vector<double> posn = pm->getDoubleData();
		rotationCenter[0] = (float)posn[0];
		rotationCenter[1] = (float)posn[1];
		rotationCenter[2] = (float)posn[2];
		return true;
	}
	else {
		pm->parseError("Unrecognized end tag in Viewpoint %s",tag.c_str());
		return false;  
	}
}
ParamNode* Viewpoint::
buildNode(){
	//Construct a viewpoint node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	ostringstream oss;

	oss.str(empty);
	if (perspective)
		oss << "true";
	else 
		oss << "false";
	attrs[_perspectiveAttr] = oss.str();
	
	ParamNode* viewpointNode = new ParamNode(_viewpointTag, attrs, 4);

	//Now add children
	
	vector<double> dbvec;
	int i;
	dbvec.clear();
	for (i = 0; i< 3; i++){
		dbvec.push_back((double) cameraPosition[i]);
	}
	viewpointNode->SetElementDouble(_camPosTag, dbvec);

	dbvec.clear();
	for (i = 0; i< 2; i++){
		dbvec.push_back((double) camLatLon[i]);
	}
	viewpointNode->SetElementDouble(_camLatLonTag, dbvec);

	dbvec.clear();
	for (i = 0; i< 2; i++){
		dbvec.push_back((double) rotCenterLatLon[i]);
	}
	viewpointNode->SetElementDouble(_rotCenterLatLonTag, dbvec);

	dbvec.clear();
	for (i = 0; i< 3; i++){
		dbvec.push_back((double) upVector[i]);
	}
	viewpointNode->SetElementDouble(_upVecTag, dbvec);
	dbvec.clear();
	for (i = 0; i< 3; i++){
		dbvec.push_back((double) viewDirection[i]);
	}
	viewpointNode->SetElementDouble(_viewDirTag, dbvec);

	dbvec.clear();
	for (i = 0; i< 3; i++){
		dbvec.push_back((double) rotationCenter[i]);
	}
	viewpointNode->SetElementDouble(_rotCenterTag, dbvec);
	
	return viewpointNode;
}

Viewpoint* Viewpoint::interpolate(Viewpoint* vp1, Viewpoint* vp2, float alpha){

	//Interpolation is complicated by stretch factors.  Following code is same
	//as is used in AnimationParams::buildViewAndTimes()
	const float* strExts = DataStatus::getInstance()->getStretchedExtents();
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	
	float stretchedSize;
	float maxStretchedSize = -1.e30;
	for (int i = 0; i<3; i++) {
		stretchedSize = strExts[i+3] - strExts[i];
		if (stretchedSize > maxStretchedSize){
			maxStretchedSize = stretchedSize;
		}
	}
	
	//Modify viewpoints by stretching according to scene stretch factors.
	//Modify speeds by factor to make speed = 1 for crossing entire scene in one frame
	Viewpoint* VP1 = new Viewpoint(*vp1);
	Viewpoint* VP2= new Viewpoint(*vp2);
	for (int j = 0; j<3; j++){
		//The displacement between the camera and rotation center is stretched
		//The rotation center is not moved.
		float camdisp = stretch[j]*(VP1->getCameraPosLocal(j)-VP1->getRotationCenterLocal(j));
		VP1->setCameraPosLocal(j,VP1->getRotationCenterLocal(j)+camdisp);
		//force vdir to point from camera to rot center
		VP1->setViewDir(j,VP1->getRotationCenterLocal(j)-VP1->getCameraPosLocal(j));

		camdisp = stretch[j]*(VP2->getCameraPosLocal(j)-VP2->getRotationCenterLocal(j));
		VP2->setCameraPosLocal(j,VP2->getRotationCenterLocal(j)+camdisp);
		//force vdir to point from camera to rot center
		VP2->setViewDir(j,VP2->getRotationCenterLocal(j)-VP2->getCameraPosLocal(j));
	}
	vnormal(VP1->getViewDir());
	vnormal(VP2->getViewDir());
	
	
	Viewpoint* vp = new Viewpoint(*VP1);
	float rCenter[3], vdir[3],campos[3],upvec[3];
	//, imagQuat[3];
	//float imagQuat1[3],imagQuat2[3];
	float quat1[4],quat2[4],quatres[4];
	float camdist1 = vdist(VP1->getCameraPosLocal(), VP1->getRotationCenterLocal());
	float camdist2 = vdist(VP2->getCameraPosLocal(), VP2->getRotationCenterLocal());
	//Multiplicative interpolation of dist:
	float camInterp = pow(camdist1, (float)(1.-alpha))*pow(camdist2,alpha);
	// Linearly interpolate the rotation center:
	for (int i = 0; i<3; i++){
		rCenter[i] = (1.-alpha)*VP1->getRotationCenterLocal(i) + alpha*VP1->getRotationCenterLocal(i);
	}
	vp->setRotationCenterLocal(rCenter);
	//Interpolate rotation in quaternions:  First convert two VP orientations to norm-1 quaternions
	view2Quat(VP1->getViewDir(),VP1->getUpVec(),quat1);
	view2Quat(VP2->getViewDir(),VP2->getUpVec(),quat2);
	slerp(quat1,quat2, 0.5, quatres);
	quat2View(quatres,vdir,upvec);
	vp->setUpVec(upvec);
	vp->setViewDir(vdir);
	/*
	//convert both vps to pure imaginary quaternion, relative to startQuat 
	views2ImagQuats(VP1->getViewDir(),VP1->getUpVec(),VP2->getViewDir(),VP2->getUpVec(),imagQuat1,imagQuat2);
	
	//interpolate linearly in imag quaternions:
	vscale(imagQuat2, alpha);
	vscale(imagQuat1, (1.-alpha));
	vadd(imagQuat1,imagQuat2,imagQuat);
	//convert back to a viewpoint:
	imagQuat2View(imagQuat, vdir, upvec);

	vp->setUpVec(upvec);
	vp->setViewDir(vdir);
	*/
	//Use the interpolated viewDist to find new campos, based on vdir:
	for(int i = 0; i<3; i++){
		campos[i] = rCenter[i] - camInterp*vdir[i];
	}
	vp->setCameraPosLocal(campos);

	//Undo stretch:
	
	for (int j = 0; j<3; j++){
		float unstretch = 1./stretch[j];
		float camdisp = unstretch*(vp->getCameraPosLocal(j)-vp->getRotationCenterLocal(j));
		vp->setCameraPosLocal(j,vp->getRotationCenterLocal(j)+camdisp);
	}
		
	return vp;
}