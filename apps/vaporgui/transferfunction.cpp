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
#include "tfinterpolator.h"

#include "transferfunction.h"
#include "assert.h"
#include "vaporinternal/common.h"
#include "dvrparams.h"
#include "vapor/MyBase.h"
using namespace VAPoR;
using namespace VetsUtil;
//Currently this is only for use in a dvrparams panel
//Need eventually to also support contours
//
TransferFunction::TransferFunction(DvrParams* p, int nBits){
	myParams = p;
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

//Methods to save and restore transfer functions.
	//The gui the FILEs that are then read/written
	//Failure results in false/null pointer
	//
bool TransferFunction::
saveToFile(FILE* f){
	int nchar = fprintf(f, "%d %d %g %g \n", numOpacControlPoints, numColorControlPoints, 
		getMinMapValue(), getMaxMapValue());
	if (nchar <= 0) return false;
	int i;
	for (i = 0; i<numOpacControlPoints; i++){
		nchar = fprintf(f, "%d %g %g\n", 
			opacInterp[i],
			opac[i], opacCtrlPoint[i]);
		if (nchar <= 0) return false;
	}
	for (i = 0; i<numColorControlPoints; i++){
		nchar = fprintf(f, "%d %g %g %g %g\n", 
			colorInterp[i],
			hue[i],sat[i],val[i],
			colorCtrlPoint[i]);
		if (nchar <= 0) return false;
	}
	fclose(f);
	return true;
}
TransferFunction* TransferFunction::
loadFromFile(FILE* f, DvrParams* p){
	int numOpac, numClr, i;
	float minBnd, maxBnd;
	int rc = fscanf(f, "%d %d %g %g", &numOpac, &numClr, &minBnd, &maxBnd);
	if (rc != 4) return 0;

	//Construct default transfer function, 8 bits lut
	TransferFunction* newTF = new TransferFunction(0,8);
	newTF->numOpacControlPoints = numOpac;
	newTF->numColorControlPoints = numClr;
	//The min/max map bounds go into the dvrparams:
	newTF->setMinMapValue(minBnd);
	newTF->setMaxMapValue(maxBnd);
	for (i = 0; i<numOpac; i++){
		rc = fscanf(f, "%d %g %g", &(newTF->opacInterp[i]), 
			&(newTF->opac[i]),
			&(newTF->opacCtrlPoint[i]));
		if (rc != 3){
			delete newTF;
			return false;
		}
	}
	for (i = 0; i<numClr; i++){
		rc = fscanf(f, "%d %g %g %g %g", &(newTF->colorInterp[i]), 
			&(newTF->hue[i]),&(newTF->sat[i]),&(newTF->val[i]),
			&(newTF->colorCtrlPoint[i]));
		if (rc != 5){
			delete newTF;
			return false;
		}
	}
	
	
	return newTF;
}



