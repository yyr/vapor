//--Colormap.cpp ------------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// A map from data value to/from color.
// 
//----------------------------------------------------------------------------

#include "Colormap.h"

#include "params.h"
#include <vapor/XmlNode.h>

#include <iostream>
#include <sstream>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

using namespace std;
using namespace VAPoR;

//----------------------------------------------------------------------------
// Static member initalization
//----------------------------------------------------------------------------
const string Colormap::_tag = "Colormap";
const string Colormap::_minTag = "MinValue";
const string Colormap::_maxTag = "MaxValue";
const string Colormap::_controlPointTag = "ColorMapControlPoint";
const string Colormap::_cpHSVTag = "HSV";
const string Colormap::_cpValueTag = "Value";


//============================================================================
// Class Colormap::Color
//============================================================================

//----------------------------------------------------------------------------
// Default constructor (white)
//----------------------------------------------------------------------------
Colormap::Color::Color() :
  _hue(0.0),
  _sat(0.0),
  _val(1.0)
{
}

//----------------------------------------------------------------------------
// Constructor 
//----------------------------------------------------------------------------
Colormap::Color::Color(float h, float s, float v) :
  _hue(h),
  _sat(s),
  _val(v)
{
}

//----------------------------------------------------------------------------
// Copy Constructor 
//----------------------------------------------------------------------------
Colormap::Color::Color(const Color &c) :
  _hue(c._hue),
  _sat(c._sat),
  _val(c._val)
{
}


//----------------------------------------------------------------------------
// Return the rgb components of the color (0.0 ... 1.0)
//----------------------------------------------------------------------------
void Colormap::Color::toRGB(float *rgb)
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
// class Colormap::ControlPoint
//============================================================================

//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
Colormap::ControlPoint::ControlPoint() :
  _value(0.0),
  _color(),
  _selected(false)
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Colormap::ControlPoint::ControlPoint(Color c, float v) :
  _value(v),
  _color(c),
  _selected(false)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
Colormap::ControlPoint::ControlPoint(const ControlPoint &cp) :
  _value(cp._value),
  _color(cp._color),
  _selected(cp._selected)
{
}

//############################################################################


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Colormap::Colormap(RenderParams *params) :
  _params(params),
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
Colormap::Colormap(const Colormap &cmap) :
  _params(cmap._params),
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
Colormap::~Colormap()
{
  clear();
}

//----------------------------------------------------------------------------
// Clear (& deallocated) the control points
//----------------------------------------------------------------------------
void Colormap::clear()
{
  for(int i=0; i<_controlPoints.size(); i++)
  {
    ControlPoint *cp = _controlPoints[i];
    delete cp;
  }

  _controlPoints.clear();
}


//----------------------------------------------------------------------------
// Build an xml node
//----------------------------------------------------------------------------
XmlNode* Colormap::buildNode()
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

  XmlNode* mainNode = new XmlNode(_tag, attrs, _controlPoints.size());

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
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float Colormap::minValue() const
{
  assert(_params);

  return _params->getMinColorMapBound() + _minValue * 
    (_params->getMaxColorMapBound() - _params->getMinColorMapBound());
}

//----------------------------------------------------------------------------
// Set the minimum value of the color map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void Colormap::minValue(float value)
{
  assert(_params);

  _minValue = 
    (value - _params->getMinColorMapBound()) / 
    (_params->getMaxColorMapBound() - _params->getMinColorMapBound());
}

//----------------------------------------------------------------------------
// Return the maximum value of the color map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float Colormap::maxValue() const
{
  assert(_params);

  return _params->getMinColorMapBound() + _maxValue * 
    (_params->getMaxColorMapBound() - _params->getMinColorMapBound());
}

//----------------------------------------------------------------------------
// Set the maximum value of the color map (in data coordinates).
// 
// The maximum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void Colormap::maxValue(float value)
{
  assert(_params);

  _maxValue = 
    (value - _params->getMinColorMapBound()) / 
    (_params->getMaxColorMapBound()-_params->getMinColorMapBound());
}


//----------------------------------------------------------------------------
// Return the control point's color
//---------------------------------------------------------------------------- 
Colormap::Color Colormap::controlPointColor(int index)
{
  return _controlPoints[index]->color();
}


//----------------------------------------------------------------------------
// Set the control point's color.
//---------------------------------------------------------------------------- 
void Colormap::controlPointColor(int index, Color color)
{
  _controlPoints[index]->color(color);
}

//----------------------------------------------------------------------------
// Return the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
float Colormap::controlPointValue(int index)
{
  float norm = _controlPoints[index]->value();

  return (norm * (maxValue() - minValue()) + minValue());
}

//----------------------------------------------------------------------------
// Set the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
void  Colormap::controlPointValue(int index, float value)
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
bool Colormap::sortCriterion(Colormap::ControlPoint *p1, 
                             Colormap::ControlPoint *p2)
{
  return p1->value() < p2->value();
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void Colormap::addControlPointAt(float value)
{
  Color c = color(value);

  float nv = (value - minValue()) / (maxValue() - minValue());

  _controlPoints.push_back(new ControlPoint(c, nv));

  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Add a new control point to the colormap.
//----------------------------------------------------------------------------
void Colormap::addControlPointAt(float value, Color color)
{
  float nv = (value - minValue()) / (maxValue() - minValue());

  _controlPoints.push_back(new ControlPoint(color, nv));

  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Delete the control point.
//----------------------------------------------------------------------------
void Colormap::deleteControlPoint(int index)
{
  if (index >= 0 && index < _controlPoints.size() && _controlPoints.size() > 1)
  {
    vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;

    ControlPoint *cp = *iter;

    _controlPoints.erase(iter);
    
    delete cp;
    cp = NULL;
  }
}

//----------------------------------------------------------------------------
// Move the control point
//---------------------------------------------------------------------------- 
void Colormap::move(int index, float delta)
{
  if (index >= 0 && index < _controlPoints.size())
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

    if (value < minVal)
    {
      value = minVal;
    }
    else if (value > maxVal)
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
Colormap::Color Colormap::color(float value)
{
  //
  // normalize the value
  // 
  float nv = (value - minValue()) / (maxValue() - minValue());
  
  //
  // Find the bounding control points
  //
  int index = leftIndex(nv);

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
int Colormap::leftIndex(float val)
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
bool Colormap::elementStartHandler(ExpatParseMgr* pm, int depth, string& tag,
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
bool Colormap::elementEndHandler(ExpatParseMgr* pm, int depth, string &tag)
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
