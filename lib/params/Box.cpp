//************************************************************************
//																		*
//		     Copyright (C)  2011										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		Box.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2011
//
//	Description:	Implements the Box class.
//		Used to control extents and orientation of 2D and 3D data regions.
//		Supports time-varying extents.
//

#include <vapor/XmlNode.h>
#include <cstring>
#include <string>
#include <vector>
#include <vapor/MyBase.h>
#include "command.h"
#include "datastatus.h"
#include "assert.h"
#include "Box.h"
#include "glutil.h"
using namespace VAPoR;

const std::string Box::_boxTag = "Box";
const std::string Box::_anglesTag = "Angles";
const std::string Box::_extentsTag = "Extents";
const std::string Box::_timesTag = "Times";
const std::string Box::_planarTag = "Planar";
const std::string Box::_orientationTag = "Orientation";

Box::Box(): ParamsBase(0, Box::_boxTag) {
	//Initialize with default box:
	vector<double> extents;
	for (int i=0; i<3; i++) extents.push_back(0.0);
	for (int i=0; i<3; i++) extents.push_back(1.0);
	vector<long> times;
	times.push_back(-1);
	vector<double> angles;
	angles.push_back(0.);
	angles.push_back(0.);
	angles.push_back(0.);
	//Provide initial values in XML node
	SetValueDouble(_extentsTag,"",extents,0);
	SetValueLong(_timesTag,"",times,0);
	SetValueDouble(_anglesTag,"",angles,0);
	SetPlanar(true,0); //Default is not planar.
	SetOrientation(-1,0); //Default is no orientation (3D)
}


int Box::GetLocalExtents(double extents[6], int timestep){
	const vector<double> defaultExtents(6,0.);
	const vector<long> defaultTimes(1,0);
	const vector<double> exts = GetValueDoubleVec(Box::_extentsTag,defaultExtents);
	const vector<long> times = GetValueLongVec(Box::_timesTag,defaultTimes);
	//If there are times, look for a match.  The first time should be -1
	for (int i = 1; i<times.size(); i++){
		if (times[i] != timestep) continue;
		//Matching time found:
		if (exts.size()< 6*(i+1)) return -1; //error
		//Copy over the desired extents
		for (int j = 0;j<6; j++){
			extents[j] = exts[6*i+j];
		}
		return 0;
	}
	//No match: return first one
	for (int j = 0; j<6; j++){extents[j] = exts[j];}
	return 0;
}
int Box::GetLocalExtents(float extents[6], int timestep){
	double exts[6];
	int rc = GetLocalExtents(exts, timestep);
	if (rc) return rc;
	for (int i = 0; i<6; i++) extents[i] = exts[i];
	return 0;
}
int Box::GetStretchedLocalExtents(double extents[6], int timestep){
	double exts[6];
	int rc = GetLocalExtents(exts, timestep);
	if (rc) return rc;
	const double *stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<6; i++) extents[i] = exts[i]*stretch[i%3];
	return 0;
}
int Box::SetLocalExtents(const vector<double>& extents, Params*p, int timestep){
	const vector<double> defaultExtents(6,0.);
	const vector<long> defaultTimes(1,0);
	const vector<double> curExts = GetValueDoubleVec(_extentsTag,defaultExtents);
	//If setting default and there are no nondefault extents, 
	//Or if the time is not in the list,
	//just replace default extents
	int index = 0;
	
	if ((timestep < 0) || (curExts.size()>6)) {
		//Check for specified timestep
		const vector<long> times = GetValueLongVec(_timesTag,defaultTimes);
		for (int i = 1; i<times.size(); i++){
			if (times[i] == timestep) {
				index = i;
				break;
			}
		}
	}
	// index will point to extents to replace.  Need to duplicate current extents:
	vector<double> copyExtents = vector<double>(curExts);
	
	for (int i = 0; i<6; i++)
		copyExtents[i+index*6] = extents[i];

	return SetValueDouble(_extentsTag, "Set box extents",copyExtents,p);	
}
int Box::SetLocalExtents(const double extents[6],Params*p, int timestep){
	vector<double> exts;
	for (int i = 0; i<6; i++) exts.push_back(extents[i]);
	return SetLocalExtents(exts, p, timestep);
}
int Box::SetLocalExtents(const float extents[6], Params*p, int timestep){
	vector<double> exts;
	for (int i = 0; i<6; i++) exts.push_back((double)extents[i]);
	return SetLocalExtents(exts, p, timestep);
}
int Box::SetStretchedLocalExtents(const double extents[6], Params*p, int timestep){
	vector<double> exts;
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	for (int i = 0; i<6; i++) exts.push_back((double)(extents[i]/stretch[i%3]));
	return SetLocalExtents(exts, p, timestep);
}
int Box::GetUserExtents(double extents[6], size_t timestep){
	int rc = GetLocalExtents(extents, (int)timestep);
	if (rc) return rc;
	if (!DataStatus::getInstance()->getDataMgr()) return -1;
	const vector<double>tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Time-varying extents are just used to get an offset that varies in time.
	for (int i = 0; i<6; i++){
		extents[i] += tvExts[i%3];
	}
	return 0;
}
	
int Box::GetUserExtents(float extents[6], size_t timestep){
	int rc = GetLocalExtents(extents, (int)timestep);
	if (rc) return rc;
	if (!DataStatus::getInstance()->getDataMgr()) return -1;
	const vector<double>tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
	//Time-varying extents are just used to get an offset that varies in time.
	for (int i = 0; i<6; i++){
		extents[i] += tvExts[i%3];
	}
	return 0;
}
void Box::Trim(Params* p,int numTimes){
		if (numTimes > GetTimes().size()) return;
		//Not a user-accessible change, do not put in command queue.
		vector<long> times = GetTimes();
		times.resize(numTimes);
		SetValueLong(Box::_timesTag,"",times, p);
		vector<double> exts; 
		vector<double>defaultExts(6,0.);
		exts = GetValueDoubleVec(Box::_extentsTag,defaultExts);
		SetValueDouble(_extentsTag, "", exts,p);
}
void Box::
buildLocalCoordTransform(double transformMatrix[12], double extraThickness, int timestep, double rotation, int axis){
	
	double theta, phi, psi;
	if (rotation != 0.) {
		convertThetaPhiPsi(&theta,&phi,&psi, axis, rotation);
	} else {
		vector<double> angles = GetAngles();
		theta = angles[0];
		phi = angles[1];
		psi = angles[2];
	}
	
	double boxSize[3];
	double boxExts[6];
	GetLocalExtents(boxExts, timestep);
	

	for (int i = 0; i< 3; i++) {
		boxExts[i] -= extraThickness;
		boxExts[i+3] += extraThickness;
		boxSize[i] = (boxExts[i+3] - boxExts[i]);
	}
	
	//Get the 3x3 rotation matrix:
	double rotMatrix[9];
	getRotationMatrix(theta*M_PI/180., phi*M_PI/180., psi*M_PI/180., rotMatrix);

	//then scale according to box:
	transformMatrix[0] = 0.5*boxSize[0]*rotMatrix[0];
	transformMatrix[1] = 0.5*boxSize[1]*rotMatrix[1];
	transformMatrix[2] = 0.5*boxSize[2]*rotMatrix[2];
	//2nd row:
	transformMatrix[4] = 0.5*boxSize[0]*rotMatrix[3];
	transformMatrix[5] = 0.5*boxSize[1]*rotMatrix[4];
	transformMatrix[6] = 0.5*boxSize[2]*rotMatrix[5];
	//3rd row:
	transformMatrix[8] = 0.5*boxSize[0]*rotMatrix[6];
	transformMatrix[9] = 0.5*boxSize[1]*rotMatrix[7];
	transformMatrix[10] = 0.5*boxSize[2]*rotMatrix[8];
	//last column, i.e. translation:
	transformMatrix[3] = .5*(boxExts[3]+boxExts[0]);
	transformMatrix[7] = .5*(boxExts[4]+boxExts[1]);
	transformMatrix[11] = .5*(boxExts[5]+boxExts[2]);
	
}
//Determine a new value of theta phi and psi when the probe is rotated around either the
//x-, y-, or z- axis.  axis is 0,1,or 2 1. rotation is in degrees.
//newTheta and newPhi are in degrees, with theta between -180 and 180, phi between 0 and 180
//and newPsi between -180 and 180
void Box::convertThetaPhiPsi(double *newTheta, double* newPhi, double* newPsi, int axis, double rotation){

	//First, get original rotation matrix R0(theta, phi, psi)
	double origMatrix[9], axisRotate[9], newMatrix[9];
	vector<double> angles = GetAngles();
	getRotationMatrix(angles[0]*M_PI/180., angles[1]*M_PI/180., angles[2]*M_PI/180., origMatrix);
	//Second, get rotation matrix R1(axis,rotation)
	getAxisRotation(axis, rotation*M_PI/180., axisRotate);
	//New rotation matrix is R1*R0
	mmult33(axisRotate, origMatrix, newMatrix);
	//Calculate newTheta, newPhi, newPsi from R1*R0 
	getRotAngles(newTheta, newPhi, newPsi, newMatrix);
	//Convert back to degrees:
	(*newTheta) *= (180./M_PI);
	(*newPhi) *= (180./M_PI);
	(*newPsi) *= (180./M_PI);
	return;
}
//Following calculates box corners in user space.  Does not use
//stretching.
void Box::
calcLocalBoxCorners(double corners[8][3], float extraThickness, int timestep, double rotation, int axis){
	double transformMatrix[12];
	buildLocalCoordTransform(transformMatrix, extraThickness, timestep, rotation, axis);
	double boxCoord[3];
	//Return the corners of the box (in world space)
	//Go counter-clockwise around the back, then around the front
	//X increases fastest, then y then z; 

	//Fatten box slightly, in case it is degenerate.  This will
	//prevent us from getting invalid face normals.

	boxCoord[0] = -1.f;
	boxCoord[1] = -1.f;
	boxCoord[2] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[0]);
	boxCoord[0] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[1]);
	boxCoord[1] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[3]);
	boxCoord[0] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[2]);
	boxCoord[1] = -1.f;
	boxCoord[2] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[4]);
	boxCoord[0] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[5]);
	boxCoord[1] = 1.f;
	vtransform(boxCoord, transformMatrix, corners[7]);
	boxCoord[0] = -1.f;
	vtransform(boxCoord, transformMatrix, corners[6]);
	
}
//Find the smallest stretched extents containing the rotated box
//Similar to above, but using stretched extents
void Box::calcRotatedStretchedBoxExtents(double* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the probe.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	double corners[8][3];
	calcLocalBoxCorners(corners, 0.f, -1);
	
	double boxMin[3],boxMax[3];
	int crd, cor;
	
	//initialize extents, and variables that will be min,max
	for (crd = 0; crd< 3; crd++){
		boxMin[crd] = 1.e30f;
		boxMax[crd] = -1.e30f;
	}
	
	
	for (cor = 0; cor< 8; cor++){
		//make sure the container includes it:
		for(crd = 0; crd< 3; crd++){
			if (corners[cor][crd]<boxMin[crd]) boxMin[crd] = corners[cor][crd];
			if (corners[cor][crd]>boxMax[crd]) boxMax[crd] = corners[cor][crd];
		}
	}
	//Now convert the min,max back into extents 
	const double* stretch = DataStatus::getInstance()->getStretchFactors();
	
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd]*stretch[crd]);
		bigBoxExtents[crd+3] = (boxMax[crd]*stretch[crd]);
	}
	return;
}