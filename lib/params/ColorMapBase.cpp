//--ColorMapBase.cpp ------------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// A map from data value to/from color.
// 
//----------------------------------------------------------------------------


#include <iostream>
#include <sstream>
#include <algorithm>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include <vapor/MyBase.h>
#include <vapor/ColorMapBase.h>
#include <vapor/MapperFunctionBase.h>
#include "assert.h"
#include "renderparams.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

using namespace std;
using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
// Static member initalization
//----------------------------------------------------------------------------
const string ColorMapBase::_tag = "Colormap";
const string ColorMapBase::_minTag = "MinValue";
const string ColorMapBase::_maxTag = "MaxValue";
const string ColorMapBase::_controlPointsTag = "ColorMapControlPoints";
const string ColorMapBase::_interpTypeTag = "ColorInterpolationType";


//============================================================================
// Class ColorMapBase::Color
//============================================================================

//----------------------------------------------------------------------------
// Default constructor (white)
//----------------------------------------------------------------------------
ColorMapBase::Color::Color() :
  _hue(0.0),
  _sat(0.0),
  _val(1.0)
{
}

//----------------------------------------------------------------------------
// Constructor 
//----------------------------------------------------------------------------
ColorMapBase::Color::Color(float h, float s, float v) :
  _hue(h),
  _sat(s),
  _val(v)
{
}
//----------------------------------------------------------------------------
// Constructor 
//----------------------------------------------------------------------------
ColorMapBase::Color::Color(double h, double s, double v) :
  _hue((float)h),
  _sat((float)s),
  _val((float)v)
{
}

//----------------------------------------------------------------------------
// Copy Constructor 
//----------------------------------------------------------------------------
ColorMapBase::Color::Color(const Color &c) :
  _hue(c._hue),
  _sat(c._sat),
  _val(c._val)
{
}


//----------------------------------------------------------------------------
// Return the rgb components of the color (0.0 ... 1.0)
//----------------------------------------------------------------------------
void ColorMapBase::Color::toRGB(float *rgb)
{

  /*	
   *  hsv-rgb Conversion function.  inputs and outputs	between 0 and 1
   *	copied (with corrections) from Hearn/Baker
   */
  if (_sat == 0.f)  //grey
  {
    rgb[0] = rgb[1] = rgb[2] = _val;
    return;
  }

  int sector    = (int)(_hue*6.f);   
  float sectCrd = _hue*6.f - (float) sector;

  if (sector == 6)
  { 
    sector = 0;
  }

  float a = _val*(1.f - _sat);
  float b = _val*(1.f - sectCrd*_sat);
  float c = _val*(1.f - (_sat*(1.f - sectCrd)));

  switch (sector)
  {
	case (0): //red to green, r>g
      rgb[0] = _val;
      rgb[1] = c;
      rgb[2] = a;
      break;
    case (1): // red to green, g>r
      rgb[1] = _val;
      rgb[2] = a;
      rgb[0] = b;
      break;
	case (2): //green to blue, gr>bl
      rgb[0] = a;
      rgb[1] = _val;
      rgb[2] = c;
      break;
	case (3): //green to blue, gr<bl
      rgb[0] = a;
      rgb[2] = _val;
      rgb[1] = b;
      break;
	case (4): //blue to red, bl>red
      rgb[1] = a;
      rgb[2] = _val;
      rgb[0] = c;
      break;
	case (5): //blue to red, bl<red
      rgb[1] = a;
      rgb[0] = _val;
      rgb[2] = b;
      break;
    default: 
      assert(0);
  }

}
/*
//============================================================================
// class ColorMapBase::ControlPoint
//============================================================================

//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
ColorMapBase::ControlPoint::ControlPoint() :
  _value(0.0),
  _color()
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ColorMapBase::ControlPoint::ControlPoint(Color c, float v) :
  _value(v),
  _color(c)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
ColorMapBase::ControlPoint::ControlPoint(const ControlPoint &cp) :
  _value(cp._value),
  _color(cp._color)
{
}

//############################################################################

*/
//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
 
ColorMapBase::ColorMapBase() : ParamsBase(0, _tag)
{
  SetInterpType(TFInterpolator::linear);
  SetMinValue( 0.0);
  SetMaxValue( 1.0);
  //Create a default color map with 4 control points:
  vector<double> cps;
  for (int i = 0; i<4; i++){
	  cps.push_back(1.-(double)i/3.);
	  cps.push_back(1.);
	  cps.push_back(1.);
	  cps.push_back((double)i/3.);
  }
  cps[0] = 0.9;  //start at purple
  SetControlPoints(cps);
}



//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
ColorMapBase::~ColorMapBase()
{
  clear();
}

//----------------------------------------------------------------------------
// Clear (& deallocated) the control points
//----------------------------------------------------------------------------
void ColorMapBase::clear()
{
  vector<double> cps;
  SetControlPoints(cps);
}



//----------------------------------------------------------------------------
// Return the control point's color
//---------------------------------------------------------------------------- 
ColorMapBase::Color ColorMapBase::controlPointColor(int index)
{
  vector<double> cps = GetControlPoints();
  return Color(cps[4*index],cps[4*index+1],cps[4*index+2]);
}


//----------------------------------------------------------------------------
// Set the control point's color.
//---------------------------------------------------------------------------- 
void ColorMapBase::controlPointColor(int index, Color color)
{
	vector<double> cps = GetControlPoints();
	cps[4*index] = color.hue();
	cps[4*index+1] = color.sat();
	cps[4*index+2] = color.val();
  SetControlPoints(cps);
}

//----------------------------------------------------------------------------
// Return the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
float ColorMapBase::controlPointValue(int index)
{
  vector<double> cps = GetControlPoints();
  float norm = (float)cps[4*index+3];

  return (norm * (GetMaxValue() - GetMinValue()) + GetMinValue());
}

//----------------------------------------------------------------------------
// Set the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
void  ColorMapBase::controlPointValue(int index, float value)
{
 
  vector<double> cps = GetControlPoints();

  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());

  float minVal = 0.0;
  float maxVal = 1.0;

  if (index > 0)
  {
    minVal = (cps[3]);
  }
  if (index < cps.size()/4 -1)
	maxVal = cps[cps.size()-1];
  
  if (nv < minVal)
  {
    nv = minVal;
  }
  else if (nv > maxVal)
  {
    nv = maxVal;
  }
  cps[index*4+3] = nv;
  SetControlPoints(cps);
}



//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void ColorMapBase::addControlPointAt(float value)
{
  Color c = color(value);

  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());

  vector<double> cps = GetControlPoints();
  //Find the insertion point:
  int indx = leftIndex(nv)*4;
  cps.insert(cps.begin()+indx++, c.hue());
  cps.insert(cps.begin()+indx++, c.sat());
  cps.insert(cps.begin()+indx++, c.val());
  cps.insert(cps.begin()+indx, nv);
  
  SetControlPoints(cps);
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void ColorMapBase::addControlPointAt(float value, Color color)
{
  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());

  vector<double> cps = GetControlPoints();
  //Find the insertion point:
  int indx = leftIndex(nv)*4;
  cps.insert(cps.begin()+indx++, color.hue());
  cps.insert(cps.begin()+indx++, color.sat());
  cps.insert(cps.begin()+indx++, color.val());
  cps.insert(cps.begin()+indx, nv);
  
  SetControlPoints(cps);
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void ColorMapBase::addNormControlPoint(float normValue, Color color)
{
   
  vector<double> cps = GetControlPoints();
  //Find the insertion point:
  int indx = leftIndex(normValue)*4;
  cps.insert(cps.begin()+indx++, color.hue());
  cps.insert(cps.begin()+indx++, color.sat());
  cps.insert(cps.begin()+indx++, color.val());
  cps.insert(cps.begin()+indx, normValue);
  
  SetControlPoints(cps);
}

//----------------------------------------------------------------------------
// Delete the control point.
//----------------------------------------------------------------------------
void ColorMapBase::deleteControlPoint(int index)
{
  vector<double> cps = GetControlPoints();
  if (index >= 0 && index < cps.size()/4 && cps.size() > 4)
  {
	cps.erase(cps.begin()+4*index, cps.begin()+4*index+4);
   
	SetControlPoints(cps);
  }
}

//----------------------------------------------------------------------------
// Move the control point, but not past adjacent control points
//---------------------------------------------------------------------------- 
void ColorMapBase::move(int index, float delta)
{
  vector<double> cps = GetControlPoints();
  if (index > 0 && index < cps.size()/4 -1) //don't move first or last control point!
  {
    
    float ndx = delta / (GetMaxValue() - GetMinValue());

	float minVal = cps[index*4 -1]; //value to the left
   
    float maxVal = cps[index*4 + 7]; //value to the right
  
    float value = cps[index*4+3] + ndx;

	if (value < 0.005) 
		value = 0.005;
	
    if (value <= minVal)
    {
      value = minVal;
    }
    else if (value >= maxVal)
    {
      value = maxVal;
    }
    
    cps[index*4+3] = value;
	SetControlPoints(cps);
  }
}


//----------------------------------------------------------------------------
// Interpolate a color at the value (data coordinates)
//
// Developed by Alan Norton. 
//----------------------------------------------------------------------------
ColorMapBase::Color ColorMapBase::color(float value)
{
  //
  // normalize the value
  // 
  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());
  vector<double> cps = GetControlPoints();
  //
  // Find the bounding control points
  //
  int index = leftIndex(nv);

  assert(index>=0 && index*4+7< cps.size());
  double leftVal = cps[4*index +3];
  double rightVal = cps[4*index + 7];

  float ratio = (nv - leftVal)/(rightVal - leftVal);

  if (ratio > 0.f && ratio < 1.f)
  {
	TFInterpolator::type itype = GetInterpType();
    float h = TFInterpolator::interpCirc(itype, 
                                         cps[4*index], //hue
                                         cps[4*index+4], 
                                         ratio);

    float s = TFInterpolator::interpolate(itype, 
                                          cps[4*index+1], //sat
                                          cps[4*index+5], 
                                          ratio);

    float v = TFInterpolator::interpolate(itype, 
                                          cps[4*index+2],//val
                                          cps[4*index+6], 
                                          ratio);

    return Color(h, s, v);
  }

  if (ratio >= 1.0)
  {
    return Color(cps[4*index+4],cps[4*index+5],cps[4*index+6]);
  }

  return Color(cps[4*index],cps[4*index+1],cps[4*index+2]);
}
 

//----------------------------------------------------------------------------
// binary search , find the index of the largest control point <= val
// Requires that control points are increasing. 
//
// Developed by Alan Norton. 
//----------------------------------------------------------------------------
int ColorMapBase::leftIndex(float val)
{
  vector<double> cps = GetControlPoints();
  int left = 0;
  int right = cps.size()/4 -1;

  //
  // Iterate, keeping left to the left of ctrl point
  //
  while (right-left > 1)
  {
    int mid = left+ (right-left)/2;
	if (cps[mid*4+3] > val)
    {
      right = mid;
    }
    else 
    {
      left = mid;
    }
  }
  return left;
}

void ColorMapBase::SetMinValue(double val) {SetValueDouble(_minTag,"Set min color map value", val, _mapper->getParams());
}
void ColorMapBase::SetMaxValue(double val) {SetValueDouble(_maxTag,"Set max color map value", val, _mapper->getParams());
}
int ColorMapBase::SetControlPoints(vector<double> controlPoints){
	return SetValueDouble(_controlPointsTag,"Set color control points", controlPoints, _mapper->getParams());
}
void ColorMapBase::SetInterpType(TFInterpolator::type t){
	SetValueLong(_interpTypeTag, "Set Color Interpolation", (long)t, _mapper->getParams());
}