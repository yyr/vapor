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
TransferFunction::TransferFunction() : MapperFunction(){
	init();
}
void TransferFunction::
init(){  //reset to starting values:
	
	colorCtrlPoint.resize(2);
	opacCtrlPoint.resize(2);
	colorInterp.resize(2);
	opacInterp.resize(2);
	hue.resize(2);
	sat.resize(2);
	val.resize(2);
	opac.resize(2);
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
	setMinMapValue(0.f);
	setMaxMapValue(1.f);
}
//Currently this is only for use in a dvrparams panel
//Need eventually to also support contours
//
TransferFunction::TransferFunction(Params* p, int nBits): MapperFunction(p,nBits){
	
	//Make a default TF:  Map 0 to 1 to a spectrum with increasing opacity.
	//Provide 2 extra control points at ends, that should never go away
	//

	numColorControlPoints = 6;
	colorCtrlPoint.resize(6);
	opacCtrlPoint.resize(6);
	colorInterp.resize(6);
	opacInterp.resize(6);
	hue.resize(6);
	sat.resize(6);
	val.resize(6);
	opac.resize(6);
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
	
	//Insert at the position indx+1
	colorCtrlPoint.insert(colorCtrlPoint.begin()+indx+1, normPoint);
	
	hue.insert(hue.begin()+indx+1, TFInterpolator::interpCirc(colorInterp[indx], hue[indx], hue[indx+1], ratio));
	sat.insert(sat.begin()+indx+1, TFInterpolator::interpolate(colorInterp[indx], sat[indx], sat[indx+1], ratio));
	val.insert(val.begin()+indx+1, TFInterpolator::interpolate(colorInterp[indx], val[indx], val[indx+1], ratio));
	colorInterp.insert(colorInterp.begin()+indx+1, colorInterp[indx]);
	
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
	//Insert at the position indx+1
	colorCtrlPoint.insert(colorCtrlPoint.begin()+indx+1, point);
	
	hue.insert(hue.begin()+indx+1, h);
	sat.insert(sat.begin()+indx+1, s);
	val.insert(val.begin()+indx+1, v);
	colorInterp.insert(colorInterp.begin()+indx+1, colorInterp[indx]);
	
	numColorControlPoints++;
	if(myParams) myParams->setClutDirty();
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

	opacCtrlPoint.insert(opacCtrlPoint.begin()+indx+1, normPoint);
	
	opac.insert(opac.begin()+indx+1, opacity);
	
	opacInterp.insert(opacInterp.begin()+indx+1, colorInterp[indx]);
	
	numOpacControlPoints++;
	assert(opacCtrlPoint.size() == numOpacControlPoints);
	myParams->setClutDirty();
	return(indx+1);
}
/*
 * Insert a new opacity control point setting opacity
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
	
	opacCtrlPoint.insert(opacCtrlPoint.begin()+indx+1, point);
	opacInterp.insert(opacInterp.begin()+indx+1, opacInterp[indx]);
	opac.insert(opac.begin()+indx+1, opacity);
	
	numOpacControlPoints++;
	if(myParams) myParams->setClutDirty();
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


void TransferFunction::
deleteColorControlPoint(int index){
	assert( index > 0 && index < numColorControlPoints-1);
	if (index >= numColorControlPoints -1) return;
	if (index <= 0) return;
	colorCtrlPoint.erase(colorCtrlPoint.begin()+index);
	hue.erase(hue.begin()+index);
	sat.erase(sat.begin()+index);
	val.erase(val.begin()+index);
	colorInterp.erase(colorInterp.begin()+index);
	
	numColorControlPoints--;
	assert(numColorControlPoints == colorCtrlPoint.size());
	myParams->setClutDirty();
	return;
}

void TransferFunction::
deleteOpacControlPoint(int index){
	assert( index > 0 && index < numOpacControlPoints-1);
	if (index >= numOpacControlPoints -1) return;
	if (index <= 0) return;
	opacCtrlPoint.erase(opacCtrlPoint.begin()+index);
	opac.erase(opac.begin()+index);
	opacInterp.erase(opacInterp.begin()+index);
	
	numOpacControlPoints--;
	assert(numOpacControlPoints == opacCtrlPoint.size());
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
	TFInterpolator::type saveOpacInterp = opacInterp[index];
	//Otherwise move it.  This is done by removing from old position, then
	//inserting in new position:
	
	opacCtrlPoint.erase(opacCtrlPoint.begin()+index);
	opac.erase(opac.begin()+index);
	opacInterp.erase(opacInterp.begin()+index);

	//New index depends on whether index was left or right of old position
	//If it was to the right, the removal reduced its position by 1.
	int newIndex = leftIndex+1;
	if (leftIndex > index) newIndex = leftIndex;
	opacCtrlPoint.insert(opacCtrlPoint.begin()+newIndex, normPoint);
	opac.insert(opac.begin()+newIndex, saveOpacity);
	opacInterp.insert(opacInterp.begin()+newIndex, saveOpacInterp);

	
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
	
	//Otherwise move it.  This is done by removing from old position, then
	//inserting in new position:
	
	colorCtrlPoint.erase(colorCtrlPoint.begin()+index);
	hue.erase(hue.begin()+index);
	sat.erase(sat.begin()+index);
	val.erase(val.begin()+index);
	colorInterp.erase(colorInterp.begin()+index);

	//New index depends on whether index was left or right of old position
	//If it was to the right, the removal reduced its position by 1.
	int newIndex = leftIndex+1;
	if (leftIndex > index) newIndex = leftIndex;
	colorCtrlPoint.insert(colorCtrlPoint.begin()+newIndex, normPoint);
	hue.insert(hue.begin()+newIndex, saveHue);
	sat.insert(sat.begin()+newIndex, saveSat);
	val.insert(val.begin()+newIndex, saveVal);
	colorInterp.insert(colorInterp.begin()+newIndex, saveColorInterp);

	myParams->setClutDirty();
	return newIndex;
	
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


QRgb TransferFunction::
getControlPointRGB(int index){
	float hsv[3], rgb[3];
	hsv[0] = hue[index];
	hsv[1] = sat[index];
	hsv[2] = val[index];
	hsvToRgb(hsv,rgb);
	
	return qRgb((int)(rgb[0]*255.999f),(int)(rgb[1]*255.999f),(int)(rgb[2]*255.999f));
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
	oss << (double)getMinMapValue();
	attrs[_leftBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)getMaxMapValue();
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
bool TransferFunction::
saveToFile(ofstream& ofs){
	const std::string emptyString;
	XmlNode* rootNode = buildNode(emptyString);

	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	delete rootNode;
	return true;
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
				ist >> mapperName;
			}
			else if (StrCmpNoCase(attribName, _leftBoundAttr) == 0) {
				float floatval;
				ist >> floatval;
				setMinMapValue(floatval);
			}
			else if (StrCmpNoCase(attribName, _rightBoundAttr) == 0) {
				float floatval;
				ist >> floatval;
				setMaxMapValue(floatval);
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


