//--OpacityMap.cpp ------------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Various types of mappings from opacity to data value. 
// 
//----------------------------------------------------------------------------

#include "OpacityMap.h"
#include "params.h"
#include <vapor/XmlNode.h>

#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>
#include <algorithm>

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef M_PI 
#define M_PI 3.1415926535897932384626433832795 
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

using namespace std;
using namespace VAPoR;

//----------------------------------------------------------------------------
// Static member initalization
//----------------------------------------------------------------------------
const string OpacityMap::_tag = "OpacityMap";
const string OpacityMap::_minTag = "MinValue";
const string OpacityMap::_maxTag = "MaxValue";
const string OpacityMap::_meanTag = "Mean";
const string OpacityMap::_ssqTag = "SSq";
const string OpacityMap::_freqTag = "Freq";
const string OpacityMap::_phaseTag = "Phase";
const string OpacityMap::_typeTag = "Type";
const string OpacityMap::_controlPointTag = "OpacityMapControlPoint";
const string OpacityMap::_cpOpacityTag = "Opacity";
const string OpacityMap::_cpValueTag = "Value";


//============================================================================
// class OpacityMap::ControlPoint
//============================================================================

//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
OpacityMap::ControlPoint::ControlPoint() :
  _value(0.0),
  _opacity(1.0),
  _selected(false)
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
OpacityMap::ControlPoint::ControlPoint(float v, float o) :
  _value(v),
  _opacity(o),
  _selected(false)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
OpacityMap::ControlPoint::ControlPoint(const ControlPoint &cp) :
  _value(cp._value),
  _opacity(cp._opacity),
  _selected(cp._selected)
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
OpacityMap::OpacityMap(RenderParams *params, OpacityMap::Type type) :
  _params(params),
  _type(type),
  _minValue(0.0),
  _maxValue(1.0),
  _mean(0.5),
  _ssq(0.1),
  _freq(5.0),
  _phase(2*M_PI),
  _minSSq(0.0001),
  _maxSSq(1.0),
  _minFreq(1.0),
  _maxFreq(30.0),
  _minPhase(0.0),
  _maxPhase(2*M_PI)
{
  _controlPoints.push_back(new ControlPoint(0.0, 0.0));
  _controlPoints.push_back(new ControlPoint(0.333, 0.333));
  _controlPoints.push_back(new ControlPoint(0.667, 0.667));
  _controlPoints.push_back(new ControlPoint(1.0, 1.0));
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
OpacityMap::OpacityMap(const OpacityMap &omap) :
  _params(omap._params),
  _type(omap._type),
  _minValue(omap._minValue),
  _maxValue(omap._maxValue),
  _mean(omap._mean),
  _ssq(omap._ssq),
  _freq(omap._freq),
  _phase(omap._phase),
  _minSSq(0.0001),
  _maxSSq(1.0),
  _minFreq(1.0),
  _maxFreq(60.0),
  _minPhase(0.0),
  _maxPhase(2*M_PI)
{
  for(int i=0; i<omap._controlPoints.size(); i++)
  {
    ControlPoint *cp = omap._controlPoints[i];

    _controlPoints.push_back(new ControlPoint(*cp));
  }
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
OpacityMap::~OpacityMap()
{
  clear();
}

//----------------------------------------------------------------------------
// Clear (& deallocated) the control points
//----------------------------------------------------------------------------
void OpacityMap::clear()
{
  for(int i=0; i<_controlPoints.size(); i++)
  {
    ControlPoint *cp = _controlPoints[i];
    delete cp;
  }

  _controlPoints.clear();
}

//----------------------------------------------------------------------------
// Build an xml node (caller has ownership)
//----------------------------------------------------------------------------
XmlNode* OpacityMap::buildNode()
{
  std::map <string, string> attrs;
  string empty;

  attrs.empty();
  ostringstream oss;

  oss.str(empty);
  oss << (int)_type;
  attrs[_typeTag] = oss.str();

  oss.str(empty);
  oss << (double)_minValue;
  attrs[_minTag] = oss.str();

  oss.str(empty);
  oss << (double)_maxValue;
  attrs[_maxTag] = oss.str();

  oss.str(empty);
  oss << (double)_mean;
  attrs[_meanTag] = oss.str();

  oss.str(empty);
  oss << (double)_ssq;
  attrs[_ssqTag] = oss.str();

  oss.str(empty);
  oss << (double)_freq;
  attrs[_freqTag] = oss.str();

  oss.str(empty);
  oss << (double)_phase;
  attrs[_phaseTag] = oss.str();

  XmlNode* mainNode = new XmlNode(_tag, attrs, _controlPoints.size());

  for (int i=0; i < _controlPoints.size(); i++)
  {
    map <string, string> cpAttrs;

    oss.str(empty);
    oss << (double)_controlPoints[i]->opacity();
    cpAttrs[_cpOpacityTag] = oss.str();
    
    oss.str(empty);
    oss << (double)_controlPoints[i]->value();
    cpAttrs[_cpValueTag] = oss.str();
    
    mainNode->NewChild(_controlPointTag, cpAttrs, 0);
  }

  return mainNode;
}


//----------------------------------------------------------------------------
// Sort criterion used to sort control points
//----------------------------------------------------------------------------
bool OpacityMap::sortCriterion(OpacityMap::ControlPoint *p1, 
                               OpacityMap::ControlPoint *p2)
{
  return p1->value() < p2->value();
}

//----------------------------------------------------------------------------
// Add a new control point to the opacity map.
//----------------------------------------------------------------------------
void OpacityMap::addNormControlPoint(float normVal, float opacity)
{
  _controlPoints.push_back(new ControlPoint(normVal, opacity));
                           
  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Add a new control point to the opacity map.
//----------------------------------------------------------------------------
void OpacityMap::addControlPoint(float value, float opacity)
{
  float normVal = (value - minValue()) / (maxValue() - minValue());

  _controlPoints.push_back(new ControlPoint(normVal, opacity));
                           
  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Delete the control point.
//----------------------------------------------------------------------------
void OpacityMap::deleteControlPoint(int index)
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
// Move the control point.
//---------------------------------------------------------------------------- 
void OpacityMap::moveControlPoint(int index, float dx, float dy)
{
  if (index >= 0 && index < _controlPoints.size())
  {
    vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;

    ControlPoint *cp = *iter;

    float minVal = 0.0;
    float maxVal = 1.0;
    float ndx = dx / (maxValue() - minValue());

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
    
    float opacity = cp->opacity() + dy;
    
    if (opacity < 0.0)
    {
      opacity = 0.0;
    }
    else if (opacity > 1.0)
    {
      opacity = 1.0;
    }

    cp->opacity(opacity);
  }
}

//----------------------------------------------------------------------------
// Return the control point's opacity.
//---------------------------------------------------------------------------- 
float OpacityMap::controlPointOpacity(int index)
{
  if (index < 0)
  {
    return 0.0;
  }

  if (index >= _controlPoints.size())
  {
    return 1.0;
  }

  return _controlPoints[index]->opacity();
}

//----------------------------------------------------------------------------
// Set the control point's opacity.
//---------------------------------------------------------------------------- 
void OpacityMap::controlPointOpacity(int index, float opacity)
{
  if (index < 0 || index >= _controlPoints.size())
  {
    return;
  }

  if (opacity < 0.0) 
  {
    _controlPoints[index]->opacity(0.0);
  }
  else if (opacity > 1.0)
  {
    _controlPoints[index]->opacity(1.0);
  }
  else
  {
    _controlPoints[index]->opacity(opacity);
  }
}


//----------------------------------------------------------------------------
// Return the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
float OpacityMap::controlPointValue(int index)
{
  if (index < 0)
  {
    return minValue();
  }

  if (index >= _controlPoints.size())
  {
    return maxValue();
  }

  float norm = _controlPoints[index]->value();

  return (norm * (maxValue() - minValue()) + minValue());
}

//----------------------------------------------------------------------------
// Set the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
void OpacityMap::controlPointValue(int index, float value)
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
// Set the mean value (normalized coordinates) for the gaussian function
//----------------------------------------------------------------------------
void OpacityMap::mean(double mean)
{
  _mean = mean;
}

//----------------------------------------------------------------------------
// Set the sigma squared value (normalized coordinates) for the gaussian 
// function
//----------------------------------------------------------------------------
void OpacityMap::sigmaSq(double ssq)
{
  _ssq = denormSSq(ssq); 
}

//----------------------------------------------------------------------------
// Set the frequency (normalized coordinates) of the sine function
//----------------------------------------------------------------------------
void OpacityMap::sineFreq(double freq) 
{ 
  _freq = denormSineFreq(freq); 
}

//----------------------------------------------------------------------------
// Set the phase (normalized coordinates) of the sine function
//----------------------------------------------------------------------------
void OpacityMap::sinePhase(double p)
{
  _phase = denormSinePhase(p); 
} 

//----------------------------------------------------------------------------
// Return the minimum value of the opacity map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float OpacityMap::minValue() const
{
  return _params->getMinOpacMapBound() + _minValue * 
    (_params->getMaxOpacMapBound() - _params->getMinOpacMapBound());
}

//----------------------------------------------------------------------------
// Set the minimum value of the opacity map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void OpacityMap::minValue(float value)
{
  _minValue = 
    MAX(0.0, 
        (value - _params->getMinOpacMapBound()) / 
        (_params->getMaxOpacMapBound() - _params->getMinOpacMapBound()));
}

//----------------------------------------------------------------------------
// Return the maximum value of the opacity map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float OpacityMap::maxValue() const
{
  return _params->getMinOpacMapBound() + _maxValue * 
    (_params->getMaxOpacMapBound() - _params->getMinOpacMapBound());
}

//----------------------------------------------------------------------------
// Set the maximum value of the opacity map (in data coordinates).
// 
// The maximum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void OpacityMap::maxValue(float value)
{
  _maxValue = 
    MIN(1.0, 
        (value - _params->getMinOpacMapBound()) / 
        (_params->getMaxOpacMapBound()-_params->getMinOpacMapBound()));
}


//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
float OpacityMap::opacity(float value)
{
  float nv = (value - minValue()) / (maxValue() - minValue());
 
  if (value < minValue())
  {
    nv = 0;
  }
  
  if (value > maxValue())
  {
    nv = 1.0;
  }
  
  switch(_type)
  {
    case CONTROL_POINT:
    {
      //
      // Find the bounding control points
      //
      int index = leftControlIndex(nv);
      
      ControlPoint *cp0 = _controlPoints[index];
      ControlPoint *cp1 = _controlPoints[index+1];

      float ratio = (nv - cp0->value())/(cp1->value() - cp0->value());

      if (ratio > 0.f && ratio < 1.f)
      {
        
        float o = TFInterpolator::interpolate(cp0->type(), 
                                              cp0->opacity(),
                                              cp1->opacity(),
                                              ratio);
        return o;
      }
      
      if (ratio >= 1.0)
      {
        return cp1->opacity();
      }
      
      return cp0->opacity();
    }

    case GAUSSIAN:
    {
      return pow(M_E, -((nv-_mean) * (nv-_mean))/(2.0*_ssq));
    }

    case SINE:
    {
      return (0.5 + sin(_freq * M_PI * nv + _phase)/2);
    }
  }

  return 0.0;
}

//----------------------------------------------------------------------------
// Determine if the opacity map bounds the value.
//----------------------------------------------------------------------------
bool OpacityMap::bounds(float value)
{
  return (value >= minValue() && value <= maxValue());
}

//----------------------------------------------------------------------------
// binary search , find the index of the largest control point <= val
// Requires that control points are increasing. 
//
// Developed by Alan Norton. 
//----------------------------------------------------------------------------
int OpacityMap::leftControlIndex(float val)
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
double OpacityMap::normSSq(double ssq) const
{
  return (ssq - _minSSq) / (_maxSSq - _minSSq);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMap::denormSSq(double ssq) const
{
  return _minSSq + (ssq * (_maxSSq - _minSSq));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMap::normSineFreq(double freq) const
{
  return (freq - _minFreq) / (_maxFreq - _minFreq);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMap::denormSineFreq(double freq) const
{
  return _minFreq + (freq * (_maxFreq - _minFreq));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMap::normSinePhase(double phase) const
{
  return (phase - _minPhase) / (_maxPhase - _minPhase);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMap::denormSinePhase(double phase) const
{
  return _minPhase + (phase * (_maxPhase - _minPhase));
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void OpacityMap::setOpaque()
{
  for(int i=0; i<_controlPoints.size(); i++)
  {
    ControlPoint *cp = _controlPoints[i];
    cp->opacity(1.0);
  }
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool OpacityMap::elementStartHandler(ExpatParseMgr* pm, int depth, string& tag,
                                     const char **attrs)
{
  if (StrCmpNoCase(tag, _tag) == 0)
  {
    //
    // Clear the current control points
    //
    for(int i=0; i<_controlPoints.size(); i++)
    {
      ControlPoint *cp = _controlPoints[i];
      delete cp;
    }

    _controlPoints.clear();

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

      // Type
      else if (StrCmpNoCase(attribName, _typeTag) == 0)
      {
        int ival;
        ist >> ival;

        _type = (OpacityMap::Type)ival;
      }

      // Gaussian Mean
      else if (StrCmpNoCase(attribName, _meanTag) == 0)
      {
        ist >> _mean;
      }

      // Gaussian sigma^2
      else if (StrCmpNoCase(attribName, _ssqTag) == 0)
      {
        ist >> _ssq;
      }

      // Sine Freq
      else if (StrCmpNoCase(attribName, _freqTag) == 0)
      {
        ist >> _freq;
      }

      // Sine phase
      else if (StrCmpNoCase(attribName, _phaseTag) == 0)
      {
        ist >> _phase;
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
    float opacity;
    float value;

    while (*attrs)
    {
      string attribName = *attrs;
      attrs++;
      istringstream ist(*attrs);
      attrs++;

      // Opacity
      if (StrCmpNoCase(attribName, _cpOpacityTag) == 0) 
      {
        ist >> opacity;
      }
      // Value
      else if (StrCmpNoCase(attribName, _cpValueTag) == 0) 
      {
        ist >> value;
      }
      else return false; 
    }    

    _controlPoints.push_back(new ControlPoint(value, opacity));

    return true;
  }

  return false;
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool OpacityMap::elementEndHandler(ExpatParseMgr* pm, int depth, string &tag)
{
  //Check only for the transferfunction tag, ignore others.
  if (StrCmpNoCase(tag, _tag) != 0) return true;
  
  //If depth is 0, this is a opacity map file; otherwise need to
  //pop the parse stack.  The caller will need to save the resulting
  //transfer function (i.e. this)
  if (depth == 0) return true;
	
  ParsedXml* px = pm->popClassStack();
  bool ok = px->elementEndHandler(pm, depth, tag);

  return ok;
}
