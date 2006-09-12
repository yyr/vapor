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
//		Provides separate mapping of color and opacity with separate domain
//		bounds.
//
//
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "tfinterpolator.h"
#include "mapperfunction.h"
#include "mapeditor.h"
#include "assert.h"
#include "vaporinternal/common.h"
#include "params.h"
#include "Colormap.h"
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

const string MapperFunction::_mapperFunctionTag = "MapperFunction";
const string MapperFunction::_leftColorBoundAttr = "LeftColorBound";
const string MapperFunction::_rightColorBoundAttr = "RightColorBound";
const string MapperFunction::_leftOpacityBoundAttr = "LeftOpacityBound";
const string MapperFunction::_rightOpacityBoundAttr = "RightOpacityBound";
const string MapperFunction::_hsvAttr = "HSV";
const string MapperFunction::_positionAttr = "Position";
const string MapperFunction::_opacityAttr = "Opacity";
const string MapperFunction::_opacityControlPointTag = "OpacityControlPoint";
const string MapperFunction::_colorControlPointTag = "ColorControlPoint";

//Constructor for empty, default Mapper function
MapperFunction::MapperFunction() {
	
	myParams = 0;
	myMapEditor = 0;
	previousClass = 0;

	_colormap = new Colormap(NULL);
}

MapperFunction::MapperFunction(RenderParams* p, int nBits){
	myParams = p;
	previousClass = 0;
	
	myMapEditor = 0;

	numEntries = 1<<nBits;
	minColorMapBound = 0.f;
	maxColorMapBound = 1.f;
	minOpacMapBound = 0.f;
	maxOpacMapBound = 1.f;
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
	
    _colormap = new Colormap(myParams);

    _opacityMaps.push_back(new OpacityMap(myParams));
}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const MapperFunction &mapper) :
  _colormap(new Colormap(*mapper._colormap)),
  numOpacControlPoints(mapper.numOpacControlPoints),
  numColorControlPoints(mapper.numColorControlPoints),
  minColorMapBound(mapper.minColorMapBound),
  maxColorMapBound(mapper.maxColorMapBound),
  minOpacMapBound(mapper.minOpacMapBound),
  maxOpacMapBound(mapper.maxOpacMapBound),
  myMapEditor(mapper.myMapEditor),
  myParams(mapper.myParams),	
  numEntries(mapper.numEntries),
  mapperName(mapper.mapperName),
  opacCtrlPoint(mapper.opacCtrlPoint),
  colorCtrlPoint(mapper.colorCtrlPoint),
  hue(mapper.hue),
  sat(mapper.sat),
  val(mapper.val),
  opac(mapper.opac),
  opacInterp(mapper.opacInterp),
  colorInterp(mapper.colorInterp)
{
  for (int i=0; i<mapper._opacityMaps.size(); i++)
  {
    _opacityMaps.push_back(new OpacityMap(*mapper._opacityMaps[i]));
  }
}

MapperFunction::~MapperFunction() 
{
	if (myMapEditor) delete myMapEditor;

    delete _colormap;
    _colormap = NULL;

    for (int i=0; i<_opacityMaps.size(); i++)
    {
      delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }
    
    _opacityMaps.clear();
}

#define GL_TF 1
#ifdef GL_TF

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
float MapperFunction::opacityValue(float value)
{
  //
  // Get the opacity scale factor from the editor (using the square gives
  // better control over low opacity values)
  //
  int count = 0;

  float opacScale = getEditor()->getOpacityScaleFactor();
  opacScale = opacScale*opacScale;
  
  float opacity = opacScale;

  for (int i=0; i<_opacityMaps.size(); i++)
  {
    OpacityMap *omap = _opacityMaps[i];

    if (omap->bounds(value))
    {
      opacity *= omap->opacity(value);
      count++;
    }
  }

  if (count) return opacity;

  return 0.0;
}

//----------------------------------------------------------------------------
// Populate at a RGBA lookup table 
//----------------------------------------------------------------------------
void MapperFunction::makeLut(float* clut)
{
  float ostep = (getMaxOpacMapValue() - getMinOpacMapValue())/(numEntries-1);
  float cstep = (getMaxColorMapValue() - getMinColorMapValue())/(numEntries-1);

  for (int i = 0; i< numEntries; i++)
  {
    float ov = getMinOpacMapValue() + i*ostep;
    float cv = getMinColorMapValue() + i*cstep;

    _colormap->color(cv).toRGB(&clut[4*i]);
    clut[4*i+3] = opacityValue(ov);
  }
}

#else // Original transfer function

/* 
 * evaluate the Opacity fcn at a float value
 */
float MapperFunction::opacityValue(float point){
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinOpacMapValue())/(getMaxOpacMapValue() - getMinOpacMapValue());
	
	int index = getLeftIndex(normPoint, opacCtrlPoint, numOpacControlPoints);
	float ratio = (normPoint - opacCtrlPoint[index])/(opacCtrlPoint[index+1]-opacCtrlPoint[index]);
	if (ratio < 0.f || ratio > 1.f) return 0.f;
	return TFInterpolator::interpolate(opacInterp[index], opac[index], opac[index+1], ratio);
}
void MapperFunction::
mapPointToRGBA(float point, float* rgba){
	float hsv[3];
	hsvValue(point, hsv, hsv+1,hsv+2);
	hsvToRgb(hsv, rgba);
	rgba[3] = opacityValue(point);
}


/*  
 *   Build a lookup table[numEntries][4](?) from the TF
 *   Caller must pass in an empty clut array to be filled in
 */
void MapperFunction::makeLut(float* clut){
	//Find the first control points
	//int colorCtrlPtNum = getLeftIndex(getMinMapValue(), colorCtrlPoint, numColorControlPoints);
	//int opacCtrlPtNum = getLeftIndex(getMinMapValue(), opacCtrlPoint, numOpacControlPoints);
	float opacScale = getEditor()->getOpacityScaleFactor();
	//Squared gives better control over low opacity values
	opacScale = opacScale*opacScale;
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
		float opacVal = opacScale*TFInterpolator::interpolate(opacInterp[opacCtrlPtNum],opac[opacCtrlPtNum],opac[opacCtrlPtNum+1],oratio);
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

#endif

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MapperFunction::setParams(RenderParams* p) 
{
  myParams = p; 
  
  _colormap->setParams(p); 

  for (int i=0; i<_opacityMaps.size(); i++)
  {
    _opacityMaps[i]->setParams(p);
  }
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityMap* MapperFunction::createOpacityMap(OpacityMap::Type type)
{
  OpacityMap *omap = new OpacityMap(myParams, type);

  _opacityMaps.push_back(omap);

  return omap;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityMap* MapperFunction::getOpacityMap(int index)
{
  if (index < 0 || index > _opacityMaps.size())
  {
    return NULL;
  }

  return _opacityMaps[index];
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MapperFunction::deleteOpacityMap(OpacityMap *omap)
{
  vector<OpacityMap*>::iterator iter = 
    std::find(_opacityMaps.begin(), _opacityMaps.end(), omap);

  if (iter != _opacityMaps.end())
  {
    _opacityMaps.erase(iter);
  }
}

void MapperFunction::
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
	setMinColorMapValue(0.f);
	setMaxColorMapValue(1.f);
	setMinOpacMapValue(0.f);
	setMaxOpacMapValue(1.f);

    //
    // Delete the opacity maps
    //
    for (int i=0; i<_opacityMaps.size(); i++)
    {
      delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }
    
    _opacityMaps.clear();
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
	if (h<0) { h= 0;}  //white, grey get indeterminate hue
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

/*
* Insert a color control point with normalized coord.
* return new index
*/
int MapperFunction::
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
	return(indx+1);
}
/*
 * Insert an opacity control point possibly changing opacity
 * If opacity < 0, it is not changed.
 */

int MapperFunction::
insertOpacControlPoint(float point, float opacity){
	if (numOpacControlPoints >= MAXCONTROLPOINTS) return -1;
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinOpacMapValue())/(getMaxOpacMapValue() - getMinOpacMapValue());
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
	return(indx+1);
}
/*
 * Insert a new opacity control point setting opacity
 * Uses normalized x-coord
 */

int MapperFunction::
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
	return(indx+1);
}
/*
* Insert a color control point without disturbing values;
* return new index
*/
int MapperFunction::
insertColorControlPoint(float point){
	if (numColorControlPoints >= MAXCONTROLPOINTS) return -1;
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinColorMapValue())/(getMaxColorMapValue() - getMinColorMapValue());
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
	return(indx+1);
}

void MapperFunction::
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
	return;
}

void MapperFunction::
deleteOpacControlPoint(int index){
	assert( index > 0 && index < numOpacControlPoints-1);
	if (index >= numOpacControlPoints -1) return;
	if (index <= 0) return;
	opacCtrlPoint.erase(opacCtrlPoint.begin()+index);
	opac.erase(opac.begin()+index);
	opacInterp.erase(opacInterp.begin()+index);
	
	numOpacControlPoints--;
	assert(numOpacControlPoints == opacCtrlPoint.size());
	return;
}
/*
 * Move an opacity control point with specified index to new position.
 * If newOpacity > -1.f, that becomes the new opacity of the control point.
 * Otherwise, assume the existing opacity at the new position, 
 */
int MapperFunction::
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
	float normPoint = (newPoint-getMinOpacMapValue())/(getMaxOpacMapValue() - getMinOpacMapValue());
	if (normPoint < 0.f) normPoint = 0.f;
	if (normPoint > 1.f) normPoint = 1.f;

	//If the new point is in the same interval, just change the values:
	int leftIndex = getLeftIndex(normPoint, opacCtrlPoint, numOpacControlPoints);
	if (leftIndex == index || leftIndex == index-1){
		opacCtrlPoint[index] = normPoint;
		opac[index] = saveOpacity;
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

	
	return newIndex;
	
}
/*
 * Move a control point with specified index to new position.
 * Keep the original color.
 */
int MapperFunction::
moveColorControlPoint(int index, float newPoint){
	//normalize point to value between 0 and 1:
	float normPoint = (newPoint-getMinColorMapValue())/(getMaxColorMapValue() - getMinColorMapValue());
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

	return newIndex;
	
}	

void MapperFunction::
hsvValue(float point, float*h, float*s, float*v){
	//normalize point to value between 0 and 1:
	//
	float normPoint = (point-getMinColorMapValue())/(getMaxColorMapValue() - getMinColorMapValue());
	
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

QRgb MapperFunction::
getControlPointRGB(int index){
	float hsv[3], rgb[3];
	hsv[0] = hue[index];
	hsv[1] = sat[index];
	hsv[2] = val[index];
	hsvToRgb(hsv,rgb);
	
	return qRgb((int)(rgb[0]*255.999f),(int)(rgb[1]*255.999f),(int)(rgb[2]*255.999f));
}
	
void MapperFunction::setOpaque()
{
  // Old-school
  for (int i = 0; i<numOpacControlPoints; i++)
  {
    opac[i] = 1.f;
  }

  // New-school
  for (int i=0; i<_opacityMaps.size(); i++)
  {
    _opacityMaps[i]->setOpaque();
  }
}

//Construct an XML node from the transfer function
XmlNode* MapperFunction::
buildNode(const string& tfname) {
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.empty();
	ostringstream oss;

	oss.str(empty);
	oss << (double)getMinColorMapValue();
	attrs[_leftColorBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)getMaxColorMapValue();
	attrs[_rightColorBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)getMinOpacMapValue();
	attrs[_leftOpacityBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)getMaxOpacMapValue();
	attrs[_rightOpacityBoundAttr] = oss.str();
	

  // 
  // Add children nodes 
  //
  int numChildren = _opacityMaps.size()+1; // opacity maps + 1 colormap

  XmlNode* mainNode = new XmlNode(_mapperFunctionTag, attrs, numChildren);

  //
  // Opacity maps
  //
  for (int i=0; i<_opacityMaps.size(); i++)
  {
    mainNode->AddChild(_opacityMaps[i]->buildNode());
  }

  //
  // Color map
  //
  mainNode->AddChild(_colormap->buildNode());

  return mainNode;
}




//Handlers for Expat parsing.
//The parse state is determined by
//whether it's parsing a color or opacity.
//
bool MapperFunction::
elementStartHandler(ExpatParseMgr* pm, 
                    int depth , 
                    std::string& tagString, 
                    const char **attrs)
{
  if (StrCmpNoCase(tagString, _mapperFunctionTag) == 0) 
  {
    //If it's a MapperFunction tag, save the left/right bounds attributes.
    //Ignore the name tag
    //Do this by repeatedly pulling off the attribute name and value
    //begin by  resetting everything to starting values.
    init();
    
    while (*attrs) 
    {
      string attribName = *attrs;
      attrs++;
      string value = *attrs;
      attrs++;
      istringstream ist(value);
      
      if (StrCmpNoCase(attribName, _leftColorBoundAttr) == 0) 
      {
        float floatval;
        ist >> floatval;
        setMinColorMapValue(floatval);
      }
      else if (StrCmpNoCase(attribName, _rightColorBoundAttr) == 0) 
      {
        float floatval;
        ist >> floatval;
        setMaxColorMapValue(floatval);
      }
      else if (StrCmpNoCase(attribName, _leftOpacityBoundAttr) == 0) 
      {
        float floatval;
        ist >> floatval;
        setMinOpacMapValue(floatval);
      }
      else if (StrCmpNoCase(attribName, _rightOpacityBoundAttr) == 0) 
      {
        float floatval;
        ist >> floatval;
        setMaxOpacMapValue(floatval);
      }      
      else
      {
        return false;
      }
    }
    
    return true;
  }
  else if (StrCmpNoCase(tagString, _colorControlPointTag) == 0) 
  {
    //Create a color control point with default values,
    //and with specified position
    //Currently only support HSV colors
    //Repeatedly pull off attribute name and values
    string attribName;
    float hue = 0.f, sat = 1.f, val=1.f, posn=0.f;

    while (*attrs)
    {
      attribName = *attrs;
      attrs++;
      string value = *attrs;
      attrs++;
      istringstream ist(value);
      if (StrCmpNoCase(attribName, _positionAttr) == 0) 
      {
        ist >> posn;
      } else if (StrCmpNoCase(attribName, _hsvAttr) == 0) 
      { 
        ist >> hue;
        ist >> sat;
        ist >> val;
      } else return false;//Unknown attribute
    }

    _colormap->addNormControlPoint(posn, Colormap::Color(hue, sat, val));

    return true;

  }
  else if (StrCmpNoCase(tagString, _opacityControlPointTag) == 0) 
  {
    //peel off position and opacity
    string attribName;
    float opacity = 1.f, posn = 0.f;

    while (*attrs)
    {
      attribName = *attrs;
      attrs++;
      string value = *attrs;
      attrs++;
      istringstream ist(value);
      if (StrCmpNoCase(attribName, _positionAttr) == 0) 
      {
        ist >> posn;
      } else if (StrCmpNoCase(attribName, _opacityAttr) == 0) 
      { 
        ist >> opacity;
      } else return false; //Unknown attribute
    }

    if (_opacityMaps.size() == 0)
    {
      // Create a opacity to hold the new control points
      OpacityMap *map = createOpacityMap();
     
      // Remove default control points
      map->clear(); 
    }

    _opacityMaps[0]->addNormControlPoint(posn, opacity);

    return true;
  }
  else if (StrCmpNoCase(tagString, OpacityMap::xmlTag()) == 0) 
  {
    OpacityMap *map = createOpacityMap();
    pm->pushClassStack(map);

    return map->elementStartHandler(pm, depth, tagString, attrs);
  }
  else if (StrCmpNoCase(tagString, Colormap::xmlTag()) == 0) 
  {
    pm->pushClassStack(_colormap);

    return _colormap->elementStartHandler(pm, depth, tagString, attrs);
  }

  else return false;
}


//The end handler needs to pop the parse stack, if this is not the top level.
bool MapperFunction::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	//Check only for the MapperFunction tag, ignore others.
	if (StrCmpNoCase(tag, _mapperFunctionTag) != 0) return true;
	//If depth is 0, this is a transfer function file; otherwise need to
	//pop the parse stack.  The caller will need to save the resulting
	//mapper function (i.e. this)
	if (depth == 0) return true;
	ParsedXml* px = pm->popClassStack();
	bool ok = px->elementEndHandler(pm, depth, tag);
	return ok;
}
