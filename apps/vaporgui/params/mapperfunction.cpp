//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		mapperfunction.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Implements the MapperFunction class  

//
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "tfinterpolator.h"
#include "mapperfunction.h"
#include "assert.h"
#include "vaporinternal/common.h"
#include "params.h"
#include "messagereporter.h"
#include "vapor/MyBase.h"
#include <vapor/XmlNode.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include <vapor/Metadata.h>
#include <vapor/CFuncs.h>
#include "vapor/ExpatParseMgr.h"
using namespace VAPoR;
using namespace VetsUtil;


//Constructor for empty, default Mapper function
MapperFunction::MapperFunction() {
	
	myParams = 0;
	myMapEditor = 0;
	previousClass = 0;
	
}

//Currently this is only for use in a dvrparams panel
//Need eventually to also support contours
//
MapperFunction::MapperFunction(Params* p, int nBits){
	myParams = p;
	previousClass = 0;
	
	myMapEditor = 0;

	numEntries = 1<<nBits;
	minColorMapBound = 0.f;
	maxColorMapBound = 1.f;
	minOpacMapBound = 0.f;
	maxOpacMapBound = 1.f;
	if(myParams) myParams->setClutDirty();
}
	
MapperFunction::~MapperFunction() {
	
}


//static utility function to map and quantize a float
//
int MapperFunction::
mapPosition(float x,  float minValue, float maxValue, int hSize){
	double psn = (0.5+((((double)x - (double)minValue)*hSize)/((double)maxValue - (double)minValue)));
	//constrain to integer size limit
	if(psn < -1000000000.f) psn = -1000000000.f;
	if(psn > 1000000000.f) psn = 1000000000.f;
	return (int)psn;
}

/*
 * binary search , find the index of the largest control point <= val
 * Requires that control points are increasing.
 * Used for either color or opacity
 * Protected, since uses normalized points.
 */
int MapperFunction::
getLeftIndex(float val, std::vector<float>& ctrlPoint, int numCtrlPoints){
	int left = 0;
	int right = numCtrlPoints-1;
	//Iterate, keeping left to the left of ctrl point
	//
	while (right-left > 1){
		int mid = left+ (right-left)/2;
		if (ctrlPoint[mid] > val) {
			right = mid;
		} else {
			left = mid;
		}
	}
	return left;
}

/*	
 *  hsv-rgb Conversion functions.  inputs and outputs	between 0 and 1
 *	copied (with corrections) from Hearn/Baker
 */
void MapperFunction::
hsvToRgb(float* hsv, float* rgb){
	if (hsv[1] == 0.f) { //grey
		rgb[0] = rgb[1] = rgb[2] = hsv[2];
		return;
	}

	int sector = (int)(hsv[0]*6.f); 

	float sectCrd = hsv[0]*6.f - (float) sector;
	if (sector == 6) sector = 0;
	float a = hsv[2]*(1.f - hsv[1]);
	float b = hsv[2]*(1.f - sectCrd*hsv[1]);
	float c = hsv[2]*(1.f - (hsv[1]*(1.f - sectCrd)));

	switch (sector){
		case (0):
			//red to green, r>g
			rgb[0] = hsv[2];
			rgb[1] = c;
			rgb[2] = a;
			break;
		case (1): // red to green, g>r
			rgb[1] = hsv[2];
			rgb[2] = a;
			rgb[0] = b;
			break;
		case (2): //green to blue, gr>bl
			rgb[0] = a;
			rgb[1] = hsv[2];
			rgb[2] = c;
			break;
		case (3): //green to blue, gr<bl
			rgb[0] = a;
			rgb[2] = hsv[2];
			rgb[1] = b;
			break;
		case (4): //blue to red, bl>red
			rgb[1] = a;
			rgb[2] = hsv[2];
			rgb[0] = c;
			break;
		case (5): //blue to red, bl<red
			rgb[1] = a;
			rgb[0] = hsv[2];
			rgb[2] = b;
			break;
		default: assert(0);
	}
	return;

}
void MapperFunction::
rgbToHsv(float* rgb, float* hsv){
	//value is max (r,g,b)
	float maxval = Max(rgb[0],Max(rgb[1],rgb[2]));
	float minval = Min(rgb[0],Min(rgb[1],rgb[2]));
	float delta = maxval - minval;
	hsv[2] = maxval;
	if (maxval != 0.f) hsv[1] = delta/maxval;
	else hsv[1] = 0.f;
	if (hsv[1] == 0.f) hsv[0] = 0.f; //no hue!
	else {
		if (rgb[0] == maxval){
			hsv[0] = (rgb[1]-rgb[0])/delta;
			if (hsv[0]< 0.f) hsv[0]+= 6.f;
		} else if (rgb[1] == maxval){
			hsv[0] = 2.f + (rgb[2]-rgb[0])/delta;
		} else {
			hsv[0] = 4.f + (rgb[0]-rgb[1])/delta;
		}
		hsv[0] /= 6.f; //Put between 0 and 1
	}
	return;
}
	
void MapperFunction::
setControlPointRGB(int index, QRgb newColor){
	QColor qc(newColor);
	int h, s, v;
	qc.getHsv(&h, &s, &v);
	setControlPointHSV(index, (float)h/360.f, (float)s/255.f, (float)v/255.f);
}

QRgb MapperFunction::
getRgbValue(float point){
	float hsv[3], rgb[3];
	int r,g,b;
	hsvValue(point, hsv, hsv+1, hsv+2);
	hsvToRgb(hsv,rgb);
	r =(int)(rgb[0]*255.f);
	g = (int)(rgb[1]*255.f);
	b =(int)(rgb[2]*255.f);
	QRgb retVal;
	retVal = qRgb(r,g,b);
	return retVal;
}

//Methods to save and restore Mapper functions.
	//The gui specifies FILEs that are then read/written
	//Failure results in false/null pointer
	//



