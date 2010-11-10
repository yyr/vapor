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
#include "assert.h"

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
const string ColorMapBase::_controlPointTag = "ColorMapControlPoint";
const string ColorMapBase::_cpHSVTag = "HSV";
const string ColorMapBase::_cpValueTag = "Value";


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

//============================================================================
// class ColorMapBase::ControlPoint
//============================================================================

//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
ColorMapBase::ControlPoint::ControlPoint() :
  _value(0.0),
  _color(),
  _selected(false),
  _type(TFInterpolator::linear)
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ColorMapBase::ControlPoint::ControlPoint(Color c, float v) :
  _value(v),
  _color(c),
  _selected(false),
  _type(TFInterpolator::linear)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
ColorMapBase::ControlPoint::ControlPoint(const ControlPoint &cp) :
  _value(cp._value),
  _color(cp._color),
  _selected(cp._selected),
  _type(cp._type)
{
}

//############################################################################


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ColorMapBase::ColorMapBase() :
  _minValue(0.0),
  _maxValue(1.0)
{
  _controlPoints.push_back(new ControlPoint(Color(0, 1.0, 1.0), 0.0));
  _controlPoints.push_back(new ControlPoint(Color(0.333, 1.0, 1.0), 0.333));
  _controlPoints.push_back(new ControlPoint(Color(0.667, 1.0, 1.0), 0.667));
  _controlPoints.push_back(new ControlPoint(Color(0.833, 1.0, 1.0), 1.0));
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
ColorMapBase::ColorMapBase(const ColorMapBase &cmap) :
  _minValue(cmap._minValue),
  _maxValue(cmap._maxValue)
{
  for(int i=0; i<cmap._controlPoints.size(); i++)
  {
    ControlPoint *cp = cmap._controlPoints[i];

    _controlPoints.push_back(new ControlPoint(*cp));
  }
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
  for(int i=0; i<_controlPoints.size(); i++)
  {
    if (_controlPoints[i]) delete _controlPoints[i];
	_controlPoints[i] = NULL;
  }

  _controlPoints.clear();
}


//----------------------------------------------------------------------------
// Build an xml node
//----------------------------------------------------------------------------
ParamNode* ColorMapBase::buildNode()
{
  std::map <string, string> attrs;
  string empty;

  attrs.empty();
  ostringstream oss;

  oss.str(empty);
  oss << (double)_minValue;
  attrs[_minTag] = oss.str();

  oss.str(empty);
  oss << (double)_maxValue;
  attrs[_maxTag] = oss.str();

  ParamNode* mainNode = new ParamNode(_tag, attrs, _controlPoints.size());

  for (int i=0; i < _controlPoints.size(); i++)
  {
    map <string, string> cpAttrs;

    oss.str(empty);
    oss << (double)_controlPoints[i]->color().hue() << " " 
        << (double)_controlPoints[i]->color().sat() << " " 
        << (double)_controlPoints[i]->color().val();
    cpAttrs[_cpHSVTag] = oss.str();
    
    oss.str(empty);
    oss << (double)_controlPoints[i]->value();
    cpAttrs[_cpValueTag] = oss.str();
    
    mainNode->NewChild(_controlPointTag, cpAttrs, 0);
  }

  return mainNode;
}

//----------------------------------------------------------------------------
// Return the minimum value of the color map (in data coordinates).
// 
//----------------------------------------------------------------------------
float ColorMapBase::minValue() const
{

  return _minValue;
}

//----------------------------------------------------------------------------
// Set the minimum value of the color map (in data coordinates).
// 
//----------------------------------------------------------------------------
void ColorMapBase::minValue(float value)
{
  _minValue = value;
}

//----------------------------------------------------------------------------
// Return the maximum value of the color map (in data coordinates).
// 
//----------------------------------------------------------------------------
float ColorMapBase::maxValue() const
{
  return _maxValue;
}

//----------------------------------------------------------------------------
// Set the maximum value of the color map (in data coordinates).
// 
//----------------------------------------------------------------------------
void ColorMapBase::maxValue(float value)
{
  _maxValue = value;
}

//----------------------------------------------------------------------------
// Return the control point's color
//---------------------------------------------------------------------------- 
ColorMapBase::Color ColorMapBase::controlPointColor(int index)
{
  assert(index>=0 && index<_controlPoints.size());
  return _controlPoints[index]->color();
}


//----------------------------------------------------------------------------
// Set the control point's color.
//---------------------------------------------------------------------------- 
void ColorMapBase::controlPointColor(int index, Color color)
{
  assert(index>=0 && index<_controlPoints.size());
  _controlPoints[index]->color(color);
}

//----------------------------------------------------------------------------
// Return the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
float ColorMapBase::controlPointValue(int index)
{
  assert(index>=0 && index<_controlPoints.size());
  float norm = _controlPoints[index]->value();

  return (norm * (maxValue() - minValue()) + minValue());
}

//----------------------------------------------------------------------------
// Set the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
void  ColorMapBase::controlPointValue(int index, float value)
{
  if (index < 0 || index >= _controlPoints.size())
  {
    return;
  }

  vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;
  
  ControlPoint *cp = *iter;

  float nv = (value - minValue()) / (maxValue() - minValue());

  float minVal = 0.0;
  float maxVal = 1.0;

  if (iter != _controlPoints.begin())
  {
    minVal = (*(iter-1))->value();
  }
  
  if (iter+1 != _controlPoints.end())
  {
    maxVal = (*(iter+1))->value();
  }

  if (nv < minVal)
  {
    nv = minVal;
  }
  else if (nv > maxVal)
  {
    nv = maxVal;
  }
  
  cp->value(nv);
}


//----------------------------------------------------------------------------
// Sort criterion used to sort control points
//----------------------------------------------------------------------------
bool ColorMapBase::sortCriterion(ColorMapBase::ControlPoint *p1, 
                             ColorMapBase::ControlPoint *p2)
{
  return p1->value() < p2->value();
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void ColorMapBase::addControlPointAt(float value)
{
  Color c = color(value);

  float nv = (value - minValue()) / (maxValue() - minValue());

  _controlPoints.push_back(new ControlPoint(c, nv));

  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void ColorMapBase::addControlPointAt(float value, Color color)
{
  float nv = (value - minValue()) / (maxValue() - minValue());

  _controlPoints.push_back(new ControlPoint(color, nv));

  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void ColorMapBase::addNormControlPoint(float normValue, Color color)
{
  _controlPoints.push_back(new ControlPoint(color, normValue));

  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Delete the control point.
//----------------------------------------------------------------------------
void ColorMapBase::deleteControlPoint(int index)
{
  if (index >= 0 && index < _controlPoints.size() && _controlPoints.size() > 1)
  {
    vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;

    ControlPoint *cp = *iter;

    _controlPoints.erase(iter);
    
    if (cp) delete cp;
    cp = NULL;
  }
}

//----------------------------------------------------------------------------
// Move the control point
//---------------------------------------------------------------------------- 
void ColorMapBase::move(int index, float delta)
{
  if (index > 0 && index < _controlPoints.size()-1)
  {
    vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;

    ControlPoint *cp = *iter;

    float minVal = 0.0;
    float maxVal = 1.0;
    float ndx = delta / (maxValue() - minValue());

    if (iter != _controlPoints.begin())
    {
      minVal = (*(iter-1))->value();
    }

    if (iter+1 != _controlPoints.end())
    {
      maxVal = (*(iter+1))->value();
    }

    float value = cp->value() + ndx;

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
    
    cp->value(value);
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
  float nv = (value - minValue()) / (maxValue() - minValue());
  
  //
  // Find the bounding control points
  //
  int index = leftIndex(nv);

  assert(index>=0 && index+1<_controlPoints.size());
  ControlPoint *cp0 = _controlPoints[index];
  ControlPoint *cp1 = _controlPoints[index+1];

  float ratio = (nv - cp0->value())/(cp1->value() - cp0->value());

  if (ratio > 0.f && ratio < 1.f)
  {

    float h = TFInterpolator::interpCirc(cp0->type(), 
                                         cp0->color().hue(),
                                         cp1->color().hue(), 
                                         ratio);

    float s = TFInterpolator::interpolate(cp0->type(), 
                                          cp0->color().sat(),
                                          cp1->color().sat(), 
                                          ratio);

    float v = TFInterpolator::interpolate(cp0->type(), 
                                          cp0->color().val(),
                                          cp1->color().val(), 
                                          ratio);

    return Color(h, s, v);
  }

  if (ratio >= 1.0)
  {
    return cp1->color();
  }

  return cp0->color();
}
 

//----------------------------------------------------------------------------
// binary search , find the index of the largest control point <= val
// Requires that control points are increasing. 
//
// Developed by Alan Norton. 
//----------------------------------------------------------------------------
int ColorMapBase::leftIndex(float val)
{
  int left = 0;
  int right = _controlPoints.size()-1;

  //
  // Iterate, keeping left to the left of ctrl point
  //
  while (right-left > 1)
  {
    int mid = left+ (right-left)/2;

    if (_controlPoints[mid]->value() > val) 
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

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool ColorMapBase::elementStartHandler(ExpatParseMgr* pm, int depth, string& tag,
                                   const char **attrs)
{
  if (StrCmpNoCase(tag, _tag) == 0)
  {
    //
    // Clear the current control points
    //
    clear();

    //
    // Read in the attributes
    //
    while (*attrs)
    {
      string attribName = *attrs;
      attrs++;
      string value = *attrs;
      attrs++;
      istringstream ist(value);

      // Minimum extent
      if (StrCmpNoCase(attribName, _minTag) == 0) 
      {
        ist >> _minValue;
      }
      
      // Maximum extent
      else if (StrCmpNoCase(attribName, _maxTag) == 0) 
      {
        ist >> _maxValue;
      }

      else return false;
    }
    
    return true;
  }
  //
  // Read in control points
  //
  else if (StrCmpNoCase(tag, _controlPointTag) == 0) 
  {
    float hue;
    float sat;
    float val;
    float value;

    while (*attrs)
    {
      string attribName = *attrs;
      attrs++;
      istringstream ist(*attrs);
      attrs++;

      // Color
      if (StrCmpNoCase(attribName, _cpHSVTag) == 0) 
      {
        ist >> hue;
        ist >> sat;
        ist >> val;
      }

      // Value
      else if (StrCmpNoCase(attribName, _cpValueTag) == 0) 
      {
        ist >> value;
      }

      else return false; 
    }    

    _controlPoints.push_back(new ControlPoint(Color(hue, sat, val), value));

    return true;
  }

  return false;
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool ColorMapBase::elementEndHandler(ExpatParseMgr* pm, int depth, string &tag)
{
  //Check only for the transferfunction tag, ignore others.
  if (StrCmpNoCase(tag, _tag) != 0) return true;
  
  //If depth is 0, this is a color map file; otherwise need to
  //pop the parse stack.  The caller will need to save the resulting
  //transfer function (i.e. this)
  if (depth == 0) return true;
	
  ParsedXml* px = pm->popClassStack();
  bool ok = px->elementEndHandler(pm, depth, tag);

  return ok;
}
