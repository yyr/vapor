//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		params.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the Params class.
//		This is an abstract class for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#include <cstring>
#include <qwidget.h>
#include "mainform.h"
#include "vizwinmgr.h"
#include "panelcommand.h"
#include "session.h"
#include "assert.h"
#include "params.h"
#include "mapperfunction.h"
#include "glutil.h"


using namespace VAPoR;

const string Params::_dvrParamsTag = "DvrPanelParameters";
const string Params::_probeParamsTag = "ProbePanelParameters";
const string Params::_regionParamsTag = "RegionPanelParameters";
const string Params::_animationParamsTag = "AnimationPanelParameters";
const string Params::_viewpointParamsTag = "ViewpointPanelParameters";
const string Params::_flowParamsTag = "FlowPanelParameters";
const string Params::_localAttr = "Local";
const string Params::_vizNumAttr = "VisualizerNum";
const string Params::_numVariablesAttr = "NumVariables";
const string Params::_variableTag = "Variable";
const string Params::_leftEditBoundAttr = "LeftEditBound";
const string Params::_rightEditBoundAttr = "RightEditBound";
const string Params::_variableNumAttr = "VariableNum";
const string Params::_variableNameAttr = "VariableName";
//Methods to find the "other" params in a local/global switch:
//
Params* Params::
getCorrespondingGlobalParams() {
	return VizWinMgr::getInstance()->getGlobalParams(thisParamType);
}
Params* Params::
getCorrespondingLocalParams() {
	return VizWinMgr::getInstance()->getLocalParams(thisParamType);
}
//To make a local->global change, must copy the local params.
//To make a global->local change, must copy global params
//
void Params::
guiSetLocal(bool lg){
	if (textChangedFlag) confirmText(false);
	/*
	if (lg) { //To convert global to local, "this" may be local or global
		PanelCommand* cmd;
		if(vizNum == -1){
			cmd = PanelCommand::captureStart(this,  "set Global to Local");
		} else {
			cmd = PanelCommand::captureStart(this->getCorrespondingGlobalParams(),  "set Global to Local");
		}
		//always set the local flag on "this"
		//
		setLocal(lg);
		PanelCommand::captureEnd(cmd, this->getCorrespondingLocalParams());
	} else { //To convert local to global, "this" is local:
		assert (vizNum >= 0);
		PanelCommand* cmd = PanelCommand::captureStart(this,  "set Local to Global");
		setLocal(lg);
		PanelCommand::captureEnd(cmd, this->getCorrespondingGlobalParams());
	} 
	*/
	PanelCommand* cmd;
	Params* localParams = getCorrespondingLocalParams();
	if (lg){
		cmd = PanelCommand::captureStart(localParams,  "set Global to Local");
	}
	else cmd = PanelCommand::captureStart(localParams,  "set Local to Global");
	localParams->setLocal(lg);
	PanelCommand::captureEnd(cmd, localParams);
}
	
void Params::confirmText(bool render){
	if (!textChangedFlag) return;
	PanelCommand* cmd = PanelCommand::captureStart(this, "edit text");
	updatePanelState();
	PanelCommand::captureEnd(cmd, this);
	//since only the text has changed, we know that the enabled and local settings
	//are unchanged:
	//
	if (render) updateRenderer(enabled, local, false);
}
QString& Params::paramName(Params::ParamType type){
	switch (type){
		
		case(ViewpointParamsType):
			return *(new QString("Viewpoint"));
		case(RegionParamsType):
			return *(new QString("Region"));
		case(IsoParamsType):
			return *(new QString("Isosurface"));
		case(FlowParamsType):
			return *(new QString("Flow"));
		case(DvrParamsType):
			return *(new QString("DVR"));
		case(ProbeParamsType):
			return *(new QString("Probe"));
		case(ContourParamsType):
			return *(new QString("Contours"));
		case(AnimationParamsType):
			return *(new QString("Animation"));
		case (UnknownParamsType):
		default:
			return *(new QString("Unknown"));
	}
	
}


float Params::getMinColorMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMinColorMapValue(): 0.f);
}
float Params::getMaxColorMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMaxColorMapValue(): 1.f);
}
float Params::getMinOpacMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMinOpacMapValue(): 0.f);
}
float Params::getMaxOpacMapBound(){
	return (getMapperFunc()? getMapperFunc()->getMaxOpacMapValue(): 1.f);
}
bool Params::isCurrent() {
		return (VizWinMgr::getInstance()->getApplicableParams(thisParamType) == this);
}
//For params subclasses that have a box:
void Params::calcBoxExtentsInCube(float* extents){
	float* fullExtents = Session::getInstance()->getExtents();
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax);
	float maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (int i = 0; i<3; i++){
		extents[i] = (boxMin[i] - fullExtents[i])/maxSize;
		extents[i+3] = (boxMax[i] - fullExtents[i])/maxSize;
	}
}
//For params subclasses that have a box, do the same in world coords
void Params::calcBoxExtents(float* extents){
	
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax);
	for (int i = 0; i<3; i++){
		extents[i] = boxMin[i];
		extents[i+3] = boxMax[i];
	}
}
void Params::
calcBoxCorners(float corners[8][3]){
	float transformMatrix[12];
	buildCoordTransform(transformMatrix);
	float boxCoord[3];
	//Return the corners of the box (in world space)
	//Go counter-clockwise around the back, then around the front
	//X increases fastest, then y then z; 

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
void Params::
buildCoordTransform(float transformMatrix[12]){
	//Note:  transformMatrix is a 3x4 matrix that converts Box coords
	// in the range [-1,1] to float coords in the volume.
	//The last column of the matrix is the translation
	//float transformMatrix[12];
	//calculate rotation matrix from phi and theta.
	//This is the matrix that takes box coords 
	// (with center at origin) to coords in the volume,
	// taking the origin to the box center.
	//  the z-axis is mapped to N, by rotating by theta and phi
	//  the x axis is mapped to U by rotating by theta,
	//  and the y axis is mapped to N x U. 
	
	//Rotation matrix is found by first rotating an angle (180-phi) in the Z-X plane,
	//then by angle theta in the X-Y plane.
	//The rows of the rotation matrix are:
	// 1st:  Cth Cph,  -Sth, Cth Sph
	// 2nd:  Sth Cph, Cth, SthSph
	// 3rd:  -Sph, 0, Cph
	//Note we are reversing phi, since it is more intuitive to have the user
	//looking down the negative z-axis.
	float theta = getTheta();
	float phi = getPhi();
	float cosTheta = cos(theta*M_PI/180.);
	float sinTheta = sin(theta*M_PI/180.);
	float sinPhi = sin((180.-phi)*M_PI/180.);
	float cosPhi = cos((180.-phi)*M_PI/180.);
	float boxSize[3];
	float boxMin[3], boxMax[3];
	getBox(boxMin, boxMax);
	for (int i = 0; i< 3; i++) {
		boxSize[i] = (boxMax[i] - boxMin[i]);
	}
	//1st row
	transformMatrix[0] = 0.5*boxSize[0]*cosTheta*cosPhi;
	transformMatrix[1] = -0.5*boxSize[1]*sinTheta;
	transformMatrix[2] = 0.5*boxSize[2]*cosTheta*sinPhi;
	//2nd row:
	transformMatrix[4] = sinTheta*cosPhi*0.5*boxSize[0];
	transformMatrix[5] = cosTheta*0.5*boxSize[1];
	transformMatrix[6] = sinTheta*sinPhi*0.5*boxSize[2];
	//3rd row:
	transformMatrix[8] = -sinPhi*0.5*boxSize[0];
	transformMatrix[9] = 0.f;
	transformMatrix[10] = cosPhi*0.5*boxSize[2];
	//last column
	transformMatrix[3] = .5f*(boxMax[0]+boxMin[0]);
	transformMatrix[7] = .5f*(boxMax[1]+boxMin[1]);
	transformMatrix[11] = .5f*(boxMax[2]+boxMin[2]);
	
}
