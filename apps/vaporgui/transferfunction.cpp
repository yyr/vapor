//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		transferfunction.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TransferFunction class  
//		This is the mathematical definition of the transfer function
//		It is defined in terms of floating point coordinates, converted
//		into a mapping of quantized values (LUT) with specified domain at
//		rendering time.  Interfaces to the TFEditor and DvrParams classes.
//		The TransferFunction deals with an mapping on the interval [0,1]
//		that is remapped to a specified interval by the user.
//
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "tfinterpolator.h"
#include "transferfunction.h"
#include "assert.h"
#include "vaporinternal/common.h"
#include "dvrparams.h"
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
// Static member initialization.  Acceptable tags in transfer function output
// Sooner or later we may want to support 
//
const string TransferFunction::_transferFunctionTag = "TransferFunction";
const string TransferFunction::_commentTag = "Comment";
const string TransferFunction::_leftBoundAttr = "LeftBound";
const string TransferFunction::_rightBoundAttr = "RightBound";
const string TransferFunction::_hsvAttr = "HSV";
const string TransferFunction::_positionAttr = "Position";
const string TransferFunction::_opacityAttr = "Opacity";
const string TransferFunction::_opacityControlPointTag = "OpacityControlPoint";
const string TransferFunction::_colorControlPointTag = "ColorControlPoint";
const string TransferFunction::_tfNameAttr = "Name";

//Constructor for empty, default transfer function
TransferFunction::TransferFunction() {
	
	myParams = 0;
	myTFEditor = 0;
	previousClass = 0;
	init();
}
void TransferFunction::
init(){  //reset to starting values:
	numColorControlPoints = 2;
	colorCtrlPoint[0] = -1.e6f;
	colorCtrlPoint[1] = 1.e6f;
	
	numOpacControlPoints = 2;
	opacCtrlPoint[0] = -1.e6f;
	opacCtrlPoint[1] = 1.e6f;
	colorInterp[0] = TFInterpolator::linear;
	opacInterp[0] = TFInterpolator::linear;
	hue[0] = 0.f;
	sat[0] = 1.f;
	val[0] = 1.f;
	opac[0] = 0.f;
	colorInterp[1] = TFInterpolator::linear;
	opacInterp[1] = TFInterpolator::linear;
	hue[1] = 0.f;
	sat[1] = 1.f;
	val[1] = 1.f;
	opac[1] = 1.f;
	numEntries = 256;
	minMapBound = 0.f;
	maxMapBound = 1.f;
}
//Currently this is only for use in a dvrparams panel
//Need eventually to also support contours
//
TransferFunction::TransferFunction(DvrParams* p, int nBits){
	myParams = p;
	previousClass = 0;
	//Make a default TF:  Map 0 to 1 to a spectrum with increasing opacity.
	//Provide 2 extra control points at ends, that should never go away
	//
	numColorControlPoints = 6;
	colorCtrlPoint[0] = -1.e6f;
	colorCtrlPoint[1] = 0.f;
	colorCtrlPoint[2] = 0.3333f;
	colorCtrlPoint[3] = 0.66667f;
	colorCtrlPoint[4] = 1.f;
	colorCtrlPoint[5] = 1.e6f;
	
	numOpacControlPoints = 6;

	opacCtrlPoint[0] = -1.e6f;
	opacCtrlPoint[1] = 0.f;
	opacCtrlPoint[2] = 0.3333f;
	opacCtrlPoint[3] = 0.6667f;
	opacCtrlPoint[4] = 1.f;
	opacCtrlPoint[5] = 1.e6f;

	for (int i = 0; i<6; i++){
		colorInterp[i] = TFInterpolator::linear;
		opacInterp[i] = TFInterpolator::linear;
		hue[i] = 0.f;
		sat[i] = 1.f;
		val[i] = 1.f;
		opac[i] = 0.f;
	}
	hue[2]=  .3f;
	hue[3] = .6f;
	hue[4]=  .9f;
	hue[5] = .9f;
	
	opac[2] = .3333f;
	opac[3] = .6667f;
	opac[4] = 1.f;
	opac[5] = 1.f;
	myTFEditor = 0;

	numEntries = 1<<nBits;
	minMapBound = 0.f;
	maxMapBound = 1.f;
	if(myParams) myParams->setClutDirty();
}
	
TransferFunction::~TransferFunction() {

	
}

/*
* Insert a color control point without disturbing values;
* return new index
*/
int TransferFunction::
insertColorControlPoint(float point){
	if (numColorControlPoints >= MAXCONTROLPOINTS) return -1;
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinMapValue())/(getMaxMapValue() - getMinMapValue());
	if (normPoint > 1.f || normPoint < 0.f) return -1;
	int indx = getLeftIndex(normPoint, colorCtrlPoint, numColorControlPoints);
	//Find the value
	//
	float leftVal = colorCtrlPoint[indx];
	float rightVal = colorCtrlPoint[indx+1];
	assert (rightVal > leftVal);
	float ratio = (normPoint-leftVal)/(rightVal-leftVal);
	
	//Create a space to insert the value
	for (int i = numColorControlPoints; i>indx+1; i--){
		hue[i] = hue[i-1];
		sat[i] = sat[i-1];
		val[i] = val[i-1];
		colorCtrlPoint[i] = colorCtrlPoint[i-1];
		colorInterp[i] = colorInterp[i-1];
	}
	colorCtrlPoint[indx+1] = normPoint;
	hue[indx+1] = TFInterpolator::interpCirc(colorInterp[indx], hue[indx], hue[indx+2], ratio);
	sat[indx+1] = TFInterpolator::interpolate(colorInterp[indx],  sat[indx], sat[indx+2], ratio);
	val[indx+1] = TFInterpolator::interpolate(colorInterp[indx],  val[indx], val[indx+2], ratio);
	colorInterp[indx+1] = colorInterp[indx];
	numColorControlPoints++;
	myParams->setClutDirty();
	return(indx+1);
}
/*
* Insert a color control point with normalized coord.
* return new index
*/
int TransferFunction::
insertNormColorControlPoint(float point, float h, float s, float v){
	if (numColorControlPoints >= MAXCONTROLPOINTS) return -1;
	//normalize point to value between 0 and 1:
	//
	
	if (point > 1.f || point < 0.f) return -1;
	int indx = getLeftIndex(point, colorCtrlPoint, numColorControlPoints);
	//Find the value
	//
	float leftVal = colorCtrlPoint[indx];
	float rightVal = colorCtrlPoint[indx+1];
	assert (rightVal > leftVal);
	
	//Create a space to insert the value
	for (int i = numColorControlPoints; i>indx+1; i--){
		hue[i] = hue[i-1];
		sat[i] = sat[i-1];
		val[i] = val[i-1];
		colorCtrlPoint[i] = colorCtrlPoint[i-1];
		colorInterp[i] = colorInterp[i-1];
	}
	colorCtrlPoint[indx+1] = point;
	hue[indx+1] = h;
	sat[indx+1] = s;
	val[indx+1] = v;
	colorInterp[indx+1] = colorInterp[indx];
	numColorControlPoints++;
	myParams->setClutDirty();
	return(indx+1);
}
/*
 * Insert an opacity control point possibly changing opacity
 * If opacity < 0, it is not changed.
 */

int TransferFunction::
insertOpacControlPoint(float point, float opacity){
	if (numOpacControlPoints >= MAXCONTROLPOINTS) return -1;
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinMapValue())/(getMaxMapValue() - getMinMapValue());
	if (normPoint < 0.f || normPoint > 1.f) return -1;
	int indx = getLeftIndex(normPoint, opacCtrlPoint, numOpacControlPoints);
	//Find the value at the new point
	//
	if (opacity < 0.f){
		float leftVal = opacCtrlPoint[indx];
		float rightVal = opacCtrlPoint[indx+1];
		assert (rightVal > leftVal);
		float ratio = (normPoint-leftVal)/(rightVal-leftVal);
		opacity = TFInterpolator::interpolate(opacInterp[indx],  opac[indx], opac[indx+1], ratio);
	}
	
	assert(opacity <= 1.f);
	
	//Create a space to insert the value
	//
	for (int i = numOpacControlPoints; i>indx+1; i--){
		opac[i] = opac[i-1];
		opacCtrlPoint[i]=opacCtrlPoint[i-1];
		opacInterp[i] = opacInterp[i-1];
	}
	opacCtrlPoint[indx+1] = normPoint;
	opacInterp[indx+1] = opacInterp[indx];
	opac[indx+1] = opacity;
	numOpacControlPoints++;
	myParams->setClutDirty();
	return(indx+1);
}
/*
 * Insert an opacity control point setting opacity
 * Uses normalized x-coord
 */

int TransferFunction::
insertNormOpacControlPoint(float point, float opacity){
	if (numOpacControlPoints >= MAXCONTROLPOINTS) return -1;
	//normalize point to value between 0 and 1:
	//
	
	if (point < 0.f || point > 1.f) return -1;
	int indx = getLeftIndex(point, opacCtrlPoint, numOpacControlPoints);
	assert(opacity <= 1.f);
	
	//Create a space to insert the value
	//
	for (int i = numOpacControlPoints; i>indx+1; i--){
		opac[i] = opac[i-1];
		opacCtrlPoint[i]=opacCtrlPoint[i-1];
		opacInterp[i] = opacInterp[i-1];
	}
	opacCtrlPoint[indx+1] = point;
	opacInterp[indx+1] = opacInterp[indx];
	opac[indx+1] = opacity;
	numOpacControlPoints++;
	myParams->setClutDirty();
	return(indx+1);
}
/* 
 * evaluate the Opacity fcn at a float value
 */
float TransferFunction::
opacityValue(float point){
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinMapValue())/(getMaxMapValue() - getMinMapValue());
	
	int index = getLeftIndex(normPoint, opacCtrlPoint, numOpacControlPoints);
	float ratio = (normPoint - opacCtrlPoint[index])/(opacCtrlPoint[index+1]-opacCtrlPoint[index]);
	if (ratio < 0.f || ratio > 1.f) return 0.f;
	return TFInterpolator::interpolate(opacInterp[index], opac[index], opac[index+1], ratio);
}
void TransferFunction::
hsvValue(float point, float*h, float*s, float*v){
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinMapValue())/(getMaxMapValue() - getMinMapValue());
	
	int index = getLeftIndex(normPoint,colorCtrlPoint, numColorControlPoints);
	float ratio = (normPoint - colorCtrlPoint[index])/(colorCtrlPoint[index+1]-colorCtrlPoint[index]);
	if (ratio < 0.f || ratio > 1.f){
		*h = 0.f;
		*s = 0.f;
		*v = 0.f;
	} else {
		*h = TFInterpolator::interpCirc(colorInterp[index], hue[index], hue[index+1], ratio);
		*s = TFInterpolator::interpolate(colorInterp[index], sat[index], sat[index+1], ratio);
		*v = TFInterpolator::interpolate(colorInterp[index], val[index], val[index+1], ratio);
	}
}
//static utility function to map and quantize a float
//
int TransferFunction::
mapPosition(float x,  float minValue, float maxValue, int hSize){
	double psn = (0.5+((((double)x - (double)minValue)*hSize)/((double)maxValue - (double)minValue)));
	//constrain to integer size limit
	if(psn < -1000000000.f) psn = -1000000000.f;
	if(psn > 1000000000.f) psn = 1000000000.f;
	return (int)psn;
}


void TransferFunction::
deleteColorControlPoint(int index){
	assert( index > 0 && index < numColorControlPoints-1);
	if (index >= numColorControlPoints -1) return;
	if (index <= 0) return;
	//Move all higher values down:
	for (int i = index; i<numColorControlPoints-1; i++){
		colorCtrlPoint[i] = colorCtrlPoint[i+1];
		hue[i] = hue[i+1];
		sat[i] = sat[i+1];
		val[i] = val[i+1];
		colorInterp[i] = colorInterp[i+1];
	}
	numColorControlPoints--;
	myParams->setClutDirty();
	return;
}

void TransferFunction::
deleteOpacControlPoint(int index){
	assert( index > 0 && index < numOpacControlPoints-1);
	if (index >= numOpacControlPoints -1) return;
	if (index <= 0) return;
	//Move all higher values down:
	for (int i = index; i<numOpacControlPoints-1; i++){
		opacCtrlPoint[i] = opacCtrlPoint[i+1];
		opac[i] = opac[i+1];
		opacInterp[i] = opacInterp[i+1];
	}
	numOpacControlPoints--;
	myParams->setClutDirty();
	return;
}
/*
 * Move an opacity control point with specified index to new position.
 * If newOpacity > -1.f, that becomes the new opacity of the control point.
 * Otherwise, assume the existing opacity at the new position, 
 */
int TransferFunction::
moveOpacControlPoint(int index, float newPoint, float newOpacity){
	
	float saveOpacity;
	// If newOpacity ==-1 find the current opacity at the the new
	// position, use the color formerly associated with the control point
	//
	if (newOpacity <= -1.f){
		saveOpacity = opacityValue(newPoint);
		
	}
	// if newopacity > -1, use this opacity, but set the colors to be
	// those already at the new point.  Force the opacity to be between
	// 0. and 1.f
	//
	else {
		saveOpacity = newOpacity;
		if (saveOpacity > 1.f) saveOpacity = 1.f;
		if (saveOpacity < 0.f) saveOpacity = 0.f;
	}
	
	//normalize newPoint to value between 0 and 1:
	//
	float normPoint = (newPoint-getMinMapValue())/(getMaxMapValue() - getMinMapValue());
	if (normPoint < 0.f) normPoint = 0.f;
	if (normPoint > 1.f) normPoint = 1.f;

	//If the new point is in the same interval, just change the values:
	int leftIndex = getLeftIndex(normPoint, opacCtrlPoint, numOpacControlPoints);
	if (leftIndex == index || leftIndex == index-1){
		opacCtrlPoint[index] = normPoint;
		opac[index] = saveOpacity;
		myParams->setClutDirty();
		return index;
	}
	//Otherwise, move it to a new interval;
	
	TFInterpolator::type saveOpacInterp = opacInterp[index];
	int newIndex;
	if (leftIndex < index-1) { //move intermediate values to the right
		for (int i = index; i > leftIndex+1; i--){
			opacCtrlPoint[i] = opacCtrlPoint[i-1];
			opac[i] = opac[i-1];
			opacInterp[i] = opacInterp[i-1];
		}
		//Fill in new values at target point, newIndex
		//
		newIndex = leftIndex+1;
	} else { //move intermediate values to left
		for (int i = index; i < leftIndex; i++){
			opacCtrlPoint[i] = opacCtrlPoint[i+1];
			opac[i] = opac[i+1];
			opacInterp[i] = opacInterp[i+1];
		}
		//Fill in new values at newIndex:
		newIndex = leftIndex;
	}
	//Fill in the values at the moved control point.
	opacCtrlPoint[newIndex] = normPoint;
	opac[newIndex] = saveOpacity;
	opacInterp[newIndex] = saveOpacInterp;
	myParams->setClutDirty();
	return newIndex;
	
}
/*
 * Move a control point with specified index to new position.
 * Keep the original color.
 */
int TransferFunction::
moveColorControlPoint(int index, float newPoint){
	//normalize point to value between 0 and 1:
	float normPoint = (newPoint-getMinMapValue())/(getMaxMapValue() - getMinMapValue());
	if (normPoint < 0.f) normPoint = 0.f;
	if (normPoint > 1.f) normPoint = 1.f;

	float saveHue = hue[index];
	float saveVal = val[index];
	float saveSat = sat[index];
	
	//If the new point is in the same interval, just change the values:
	int leftIndex = getLeftIndex(normPoint, colorCtrlPoint, numColorControlPoints);
	if (leftIndex == index || leftIndex == index-1){
		colorCtrlPoint[index] = normPoint;
		hue[index] = saveHue;
		sat[index] = saveSat;
		val[index] = saveVal;
		myParams->setClutDirty();
		return index;
	}
	//Otherwise, move it to a new interval;
	TFInterpolator::type saveColorInterp = colorInterp[index];
	int newIndex;
	if (leftIndex < index-1) { //move intermediate values to the right
		for (int i = index; i > leftIndex+1; i--){
			colorCtrlPoint[i] = colorCtrlPoint[i-1];
			hue[i] = hue[i-1];
			sat[i] = sat[i-1];
			val[i] = val[i-1];
			colorInterp[i] = colorInterp[i-1];
		}
		//Fill in new values at target point, newIndex
		newIndex = leftIndex+1;
	} else { //move intermediate values to left
		for (int i = index; i < leftIndex; i++){
			colorCtrlPoint[i] = colorCtrlPoint[i+1];
			hue[i] = hue[i+1];
			sat[i] = sat[i+1];
			val[i] = val[i+1];
			colorInterp[i] = colorInterp[i+1];
		}
		//Fill in new values at newIndex:
		newIndex = leftIndex;
	}
	//Fill in the values at the moved control point.
	colorCtrlPoint[newIndex] = normPoint;
	hue[newIndex] = saveHue;
	sat[newIndex] = saveSat;
	val[newIndex] = saveVal;
	colorInterp[newIndex] = saveColorInterp;
	myParams->setClutDirty();
	return newIndex;
	
}	
/*
 * binary search , find the index of the largest control point <= val
 * Requires that control points are increasing.
 * Used for either color or opacity
 * Protected, since uses normalized points.
 */
int TransferFunction::
getLeftIndex(float val, float* ctrlPoint, int numCtrlPoints){
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
 *   Build a lookup table[numEntries][4](?) from the TF
 *   Caller must pass in an empty clut array to be filled in
 */
void TransferFunction::
makeLut(float* clut){
	//Find the first control points
	//int colorCtrlPtNum = getLeftIndex(getMinMapValue(), colorCtrlPoint, numColorControlPoints);
	//int opacCtrlPtNum = getLeftIndex(getMinMapValue(), opacCtrlPoint, numOpacControlPoints);
	int colorCtrlPtNum = 0;
	int opacCtrlPtNum = 0;
	for (int i = 0; i< numEntries; i++){
		//map the interval [minmapval,maxmapval] to [0..numEntries-1]
		//float xvalue = getMinMapValue() + ((float)i)*(getMaxMapValue()-getMinMapValue())/((float)(numEntries-1));
		float normXValue = ((float)i)/(float)(numEntries - 1);
		//Check if need next control point.
		//find first control point to the right of this point
		while (colorCtrlPoint[colorCtrlPtNum+1] < normXValue){
			colorCtrlPtNum++;
		}
		while (opacCtrlPoint[opacCtrlPtNum+1] < normXValue){
			opacCtrlPtNum++;
		}
		//use the interpolators on that interval
		float hsv[3], rgb[3];
		float cratio = (normXValue - colorCtrlPoint[colorCtrlPtNum])/(colorCtrlPoint[colorCtrlPtNum+1]-colorCtrlPoint[colorCtrlPtNum]);
		float oratio = (normXValue - opacCtrlPoint[opacCtrlPtNum])/(opacCtrlPoint[opacCtrlPtNum+1]-opacCtrlPoint[opacCtrlPtNum]);
		float opacVal = TFInterpolator::interpolate(opacInterp[opacCtrlPtNum],opac[opacCtrlPtNum],opac[opacCtrlPtNum+1],oratio);
		assert( opacVal >= 0.f && opacVal <= 1.f);
		hsv[0] = TFInterpolator::interpCirc(colorInterp[colorCtrlPtNum],hue[colorCtrlPtNum],hue[colorCtrlPtNum+1],cratio);
		hsv[1] = TFInterpolator::interpolate(colorInterp[colorCtrlPtNum],sat[colorCtrlPtNum],sat[colorCtrlPtNum+1],cratio);
		hsv[2] = TFInterpolator::interpolate(colorInterp[colorCtrlPtNum],val[colorCtrlPtNum],val[colorCtrlPtNum+1],cratio);
		assert( hsv[0] >= 0.f && hsv[0] <= 1.f);
		assert( hsv[1] >= 0.f && hsv[1] <= 1.f);
		assert( hsv[2] >= 0.f && hsv[2] <= 1.f);
		//convert to rgb
		hsvToRgb(hsv, rgb);
		
		assert( rgb[0] >= 0.f && rgb[0] <= 1.f);
		assert( rgb[1] >= 0.f && rgb[1] <= 1.f);
		assert( rgb[2] >= 0.f && rgb[2] <= 1.f);
		clut[4*i] = rgb[0];
		clut[4*i+1] = rgb[1];
		clut[4*i+2] = rgb[2];
		clut[4*i+3] = opacVal;
	}
}
/*	
 *  hsv-rgb Conversion functions.  inputs and outputs	between 0 and 1
 *	copied (with corrections) from Hearn/Baker
 */
void TransferFunction::
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
void TransferFunction::
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
QRgb TransferFunction::
getControlPointRGB(int index){
	float hsv[3], rgb[3];
	hsv[0] = hue[index];
	hsv[1] = sat[index];
	hsv[2] = val[index];
	hsvToRgb(hsv,rgb);
	
	return qRgb((int)(rgb[0]*255.999f),(int)(rgb[1]*255.999f),(int)(rgb[2]*255.999f));
}
	
void TransferFunction::
setControlPointRGB(int index, QRgb newColor){
	QColor qc(newColor);
	int h, s, v;
	qc.getHsv(&h, &s, &v);
	setControlPointHSV(index, (float)h/360.f, (float)s/255.f, (float)v/255.f);
}

QRgb TransferFunction::
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

//Methods to save and restore transfer functions.
	//The gui specifies FILEs that are then read/written
	//Failure results in false/null pointer
	//
//Construct an XML node from the transfer function
XmlNode* TransferFunction::
buildNode(const string& tfname) {
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.empty();
	ostringstream oss;

	if (!tfname.empty()){
		attrs[_tfNameAttr] = tfname;
	}
	oss.str(empty);
	oss << (double)minMapBound;
	attrs[_leftBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)maxMapBound;
	attrs[_rightBoundAttr] = oss.str();

	XmlNode* mainNode = new XmlNode(_transferFunctionTag, attrs, numOpacControlPoints+numColorControlPoints);

	//Now add children:  One for each control point.
	//ignore first and last.
	
	
	map <string, string> emptyAttrs;  //empty attribs
	int i;
	
	for (i = 1; i< numOpacControlPoints-1; i++){
		map <string, string> cpAttrs;
		
		oss.str(empty);
		oss << (double)opacCtrlPoint[i];
		cpAttrs[_positionAttr] = oss.str();
		oss.str(empty);
		oss << (double)opac[i];
		cpAttrs[_opacityAttr] = oss.str();
		mainNode->NewChild(_opacityControlPointTag, cpAttrs, 0);
	}
	for (i = 1; i< numColorControlPoints-1; i++){
		map <string, string> cpAttrs;
		
		oss.str(empty);
		oss << (double)colorCtrlPoint[i]; //Put the value in the control point node
		cpAttrs[_positionAttr] = oss.str();
		oss.str(empty);
		oss << (double)hue[i] << " " << (double)sat[i] << " " << (double)val[i];
		cpAttrs[_hsvAttr] = oss.str();
		mainNode->NewChild(_colorControlPointTag, cpAttrs, 0);
	}
	return mainNode;
}

bool TransferFunction::
saveToFile(ofstream& ofs){
	const std::string emptyString;
	XmlNode* rootNode = buildNode(emptyString);

	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	delete rootNode;
	return true;
}
//Create a transfer function by parsing a file.
TransferFunction* TransferFunction::
loadFromFile(ifstream& is){
	TransferFunction* newTF = new TransferFunction();
	// Create an Expat XML parser to parse the XML formatted metadata file
	// specified by 'path'
	//
	ExpatParseMgr* parseMgr = new ExpatParseMgr(newTF);
	parseMgr->parse(is);
	delete parseMgr;
	return newTF;
}



//Handlers for Expat parsing.
//The parse state is determined by
//whether it's parsing a color or opacity.
//
bool TransferFunction::
elementStartHandler(ExpatParseMgr* pm, int /*depth*/ , std::string& tagString, const char **attrs){
	if (StrCmpNoCase(tagString, _transferFunctionTag) == 0) {
		//If it's a TransferFunction tag, save the left/right bounds attributes.
		//Ignore the name tag
		//Do this by repeatedly pulling off the attribute name and value
		//begin by  resetting everything to starting values.
		init();
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _tfNameAttr) == 0) {
				ist >> tfName;
			}
			else if (StrCmpNoCase(attribName, _leftBoundAttr) == 0) {
				ist >> minMapBound;
			}
			else if (StrCmpNoCase(attribName, _rightBoundAttr) == 0) {
				ist >> maxMapBound;
			}
			
			else return false;
		}
		return true;
	}
	else if (StrCmpNoCase(tagString, _colorControlPointTag) == 0) {
		//Create a color control point with default values,
		//and with specified position
		//Currently only support HSV colors
		//Repeatedly pull off attribute name and values
		string attribName;
		float hue = 0.f, sat = 1.f, val=1.f, posn=0.f;
		while (*attrs){
			attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _positionAttr) == 0) {
				ist >> posn;
			} else if (StrCmpNoCase(attribName, _hsvAttr) == 0) { 
				ist >> hue;
				ist >> sat;
				ist >> val;
			} else return false;//Unknown attribute
		}
		//Then insert color control point
		int indx = insertNormColorControlPoint(posn,hue,sat,val);
		if (indx >= 0)return true;
		return false;
	}
	else if (StrCmpNoCase(tagString, _opacityControlPointTag) == 0) {
		//peel off position and opacity
		string attribName;
		float opacity = 1.f, posn = 0.f;
		while (*attrs){
			attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _positionAttr) == 0) {
				ist >> posn;
			} else if (StrCmpNoCase(attribName, _opacityAttr) == 0) { 
				ist >> opacity;
			} else return false; //Unknown attribute
		}
		//Then insert color control point
		if(insertNormOpacControlPoint(posn, opacity)>=0) return true;
		else return false;
	}
	else return false;
}
//The end handler needs to pop the parse stack, if this is not the top level.
bool TransferFunction::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	//Check only for the transferfunction tag, ignore others.
	if (StrCmpNoCase(tag, _transferFunctionTag) != 0) return true;
	//If depth is 0, this is a transfer function file; otherwise need to
	//pop the parse stack.  The caller will need to save the resulting
	//transfer function (i.e. this)
	if (depth == 0) return true;
	ParsedXml* px = pm->popClassStack();
	bool ok = px->elementEndHandler(pm, depth, tag);
	return ok;
}


