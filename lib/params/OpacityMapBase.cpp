//--OpacityMapBase.cpp -------------------------------------------------------
//
// Copyright (C) 2006 Kenny Gruchalla.  All rights reserved.
//
// Various types of mappings from opacity to data value. 
// 
//----------------------------------------------------------------------------


#include <iostream>
#include <sstream>
#include <cmath>
#include <vector>
#include <algorithm>

#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include <vapor/MyBase.h>
#include <vapor/OpacityMapBase.h>
#include <vapor/MapperFunctionBase.h>
#include "renderparams.h"

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
using namespace VetsUtil;

//----------------------------------------------------------------------------
// Static member initalization
//----------------------------------------------------------------------------
const string OpacityMapBase::_tag = "OpacityMap";
const string OpacityMapBase::_minTag = "MinValue";
const string OpacityMapBase::_maxTag = "MaxValue";
const string OpacityMapBase::_enabledTag = "Enabled";
const string OpacityMapBase::_meanTag = "Mean";
const string OpacityMapBase::_ssqTag = "SSq";
const string OpacityMapBase::_freqTag = "Freq";
const string OpacityMapBase::_phaseTag = "Phase";
const string OpacityMapBase::_typeTag = "Type";
const string OpacityMapBase::_controlPointTag = "OpacityMapControlPoint";
const string OpacityMapBase::_cpOpacityTag = "Opacity";
const string OpacityMapBase::_cpValueTag = "Value";


//============================================================================
// class OpacityMapBase::ControlPoint
//============================================================================

//----------------------------------------------------------------------------
// Default constructor
//----------------------------------------------------------------------------
OpacityMapBase::ControlPoint::ControlPoint() :
  _value(0.0),
  _opacity(1.0)
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
OpacityMapBase::ControlPoint::ControlPoint(float v, float o) :
  _value(v),
  _opacity(o)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
OpacityMapBase::ControlPoint::ControlPoint(const ControlPoint &cp) :
  _value(cp._value),
  _opacity(cp._opacity)
{
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
OpacityMapBase::OpacityMapBase(OpacityMapBase::Type type) : ParamsBase(0, _tag),
  _minSSq(0.0001),
  _maxSSq(1.0),
  _minFreq(1.0),
  _maxFreq(30.0),
  _minPhase(0.0),
  _maxPhase(2*M_PI),
  _mapper(NULL)
{
  _interpType = TFInterpolator::linear;
  
  _type =type,
  _enabled = true;
  _mean = 0.5;
  _ssq = 0.1;
  _freq = 5.0;
  _phase=2*M_PI;
  
  _controlPoints.push_back(new ControlPoint(0.0, 0.0));
  _controlPoints.push_back(new ControlPoint(0.333, 0.333));
  _controlPoints.push_back(new ControlPoint(0.667, 0.667));
  _controlPoints.push_back(new ControlPoint(1.0, 1.0));
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
OpacityMapBase::~OpacityMapBase()
{
  clear();
}

//----------------------------------------------------------------------------
// Clear (& deallocated) the control points
//----------------------------------------------------------------------------
void OpacityMapBase::clear()
{
  for(int i=0; i<_controlPoints.size(); i++)
  {
    ControlPoint *cp = _controlPoints[i];
    if (cp) delete cp;
  }

  _controlPoints.clear();
}


//----------------------------------------------------------------------------
// Sort criterion used to sort control points
//----------------------------------------------------------------------------
bool OpacityMapBase::sortCriterion(OpacityMapBase::ControlPoint *p1, 
                               OpacityMapBase::ControlPoint *p2)
{
  return p1->value() < p2->value();
}

//----------------------------------------------------------------------------
// Add a new control point to the opacity map.
//----------------------------------------------------------------------------
void OpacityMapBase::addNormControlPoint(float normVal, float opacity)
{
  _controlPoints.push_back(new ControlPoint(normVal, opacity));
                           
  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Add a new control point to the opacity map.
//----------------------------------------------------------------------------
void OpacityMapBase::addControlPoint(float value, float opacity)
{
  float normVal = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());

  _controlPoints.push_back(new ControlPoint(normVal, opacity));
                           
  std::sort(_controlPoints.begin(), _controlPoints.end(), sortCriterion);
}

//----------------------------------------------------------------------------
// Delete the control point.
//----------------------------------------------------------------------------
void OpacityMapBase::deleteControlPoint(int index)
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
// Move the control point.
//---------------------------------------------------------------------------- 
void OpacityMapBase::moveControlPoint(int index, float dx, float dy)
{
  if (index >= 0 && index < _controlPoints.size())
  {
    vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;

    ControlPoint *cp = *iter;

    float minVal = 0.0;
    float maxVal = 1.0;
    float ndx = dx / (GetMaxValue() - GetMinValue());

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
float OpacityMapBase::controlPointOpacity(int index)
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
void OpacityMapBase::controlPointOpacity(int index, float opacity)
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
float OpacityMapBase::controlPointValue(int index)
{
  if (index < 0)
  {
    return GetMinValue();
  }

  if (index >= _controlPoints.size())
  {
    return GetMaxValue();
  }

  float norm = _controlPoints[index]->value();

  return (norm * (GetMaxValue() - GetMinValue()) + GetMinValue());
}

//----------------------------------------------------------------------------
// Set the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
void OpacityMapBase::controlPointValue(int index, float value)
{
  if (index < 0 || index >= _controlPoints.size())
  {
    return;
  }

  vector<ControlPoint*>::iterator iter = _controlPoints.begin()+index;
  
  ControlPoint *cp = *iter;

  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());

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
void OpacityMapBase::mean(double mean)
{
  _mean = mean;
}

//----------------------------------------------------------------------------
// Set the sigma squared value (normalized coordinates) for the gaussian 
// function
//----------------------------------------------------------------------------
void OpacityMapBase::sigmaSq(double ssq)
{
  _ssq = denormSSq(ssq); 
}

//----------------------------------------------------------------------------
// Set the frequency (normalized coordinates) of the sine function
//----------------------------------------------------------------------------
void OpacityMapBase::sineFreq(double freq) 
{ 
  _freq = denormSineFreq(freq); 
}

//----------------------------------------------------------------------------
// Set the phase (normalized coordinates) of the sine function
//----------------------------------------------------------------------------
void OpacityMapBase::sinePhase(double p)
{
  _phase = denormSinePhase(p); 
} 


//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
float OpacityMapBase::opacity(float value)
{
  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());
 
  if (value < GetMinValue())
  {
    nv = 0;
  }
  
  if (value > GetMaxValue())
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
        
        float o = TFInterpolator::interpolate(interpType(), 
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

    case INVERTED_GAUSSIAN:
    {
      return 1.0-pow(M_E, -((nv-_mean) * (nv-_mean))/(2.0*_ssq));
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
bool OpacityMapBase::bounds(float value)
{
  return (value >= GetMinValue() && value <= GetMaxValue());
}

//----------------------------------------------------------------------------
// binary search , find the index of the largest control point <= val
// Requires that control points are increasing. 
//
// Developed by Alan Norton. 
//----------------------------------------------------------------------------
int OpacityMapBase::leftControlIndex(float val)
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
double OpacityMapBase::normSSq(double ssq) const
{
  return (ssq - _minSSq) / (_maxSSq - _minSSq);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::denormSSq(double ssq) const
{
  return _minSSq + (ssq * (_maxSSq - _minSSq));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::normSineFreq(double freq) const
{
  return (freq - _minFreq) / (_maxFreq - _minFreq);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::denormSineFreq(double freq) const
{
  return _minFreq + (freq * (_maxFreq - _minFreq));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::normSinePhase(double phase) const
{
  return (phase - _minPhase) / (_maxPhase - _minPhase);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::denormSinePhase(double phase) const
{
  return _minPhase + (phase * (_maxPhase - _minPhase));
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void OpacityMapBase::setOpaque()
{
  for(int i=0; i<_controlPoints.size(); i++)
  {
    ControlPoint *cp = _controlPoints[i];
    cp->opacity(1.0);
  }
}
bool OpacityMapBase::isOpaque()
{
  for(int i=0; i<_controlPoints.size(); i++)
  {
    ControlPoint *cp = _controlPoints[i];
    if (cp->opacity() < 1.0) return false;
  }
  return true;
}

void OpacityMapBase::SetMinValue(double val) {SetValueDouble(_minTag,"Set min opacity map value", val, _mapper->getParams());}
void OpacityMapBase::SetMaxValue(double val) {SetValueDouble(_maxTag,"Set max opacity map value", val, _mapper->getParams());}