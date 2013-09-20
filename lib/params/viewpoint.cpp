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

#include "glutil.h"	// Must be included first
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "viewpoint.h"
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>



using namespace VAPoR;
using namespace VetsUtil;
const string Viewpoint::_viewpointTag = "Viewpoint";
const string Viewpoint::_camPosTag = "CameraPosition";
const string Viewpoint::_viewDirTag = "ViewDirection";
const string Viewpoint::_upVecTag = "UpVector";
const string Viewpoint::_rotCenterTag = "RotationCenter";
const string Viewpoint::_rotCenterLatLonTag = "RotCenterLatLon";
const string Viewpoint::_camLatLonTag = "CameraLatLon";



//Force the rotation center to lie in the center of the view.
void Viewpoint::alignCenter(){
	vnormal(viewDirection);
	float rctr[3], cmps[3], temp[3];
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	//stretch rotCenter and campos
	for (int i = 0; i<3; i++){
		rctr[i] = rotationCenter[i]* stretch[i];
		cmps[i] = cameraPosition[i]* stretch[i];
	}
	//then project rotCenter-camPos in direction of view:
	vsub(rctr, cmps, temp);
	
	float dt = vdot(viewDirection, temp);
	vmult(viewDirection, dt, temp);

	//apply unstretch to difference:
	for (int i = 0; i<3; i++) temp[i] /= stretch[i];
	vadd(temp, cameraPosition, rotationCenter);
}
