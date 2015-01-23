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
const string OpacityMapBase::_controlPointsTag = "OpacityMapControlPoints";
const string OpacityMapBase::_interpTypeTag = "OpacityInterpolation";



//============================================================================
// class OpacityMapBase::ControlPoint
//============================================================================



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
  SetInterpType( TFInterpolator::linear);
  SetType(type);
  SetEnabled(true);
  SetMean(0.5);
  SetSSQ(0.1);
  SetFreq(5.0);
  SetPhase(2.*M_PI);
  
  vector<double> cps;
  for (int i = 0; i<4; i++){
	  cps.push_back((double)i/4.);
	  cps.push_back((double)i/4.);
  }
  SetControlPoints(cps);
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
OpacityMapBase::~OpacityMapBase()
{
}

//----------------------------------------------------------------------------
// Clear  the control points
//----------------------------------------------------------------------------
void OpacityMapBase::clear()
{
  vector<double> cps;
  SetControlPoints(cps);
}

//----------------------------------------------------------------------------
// Add a new control point to the opacity map.
//----------------------------------------------------------------------------
void OpacityMapBase::addNormControlPoint(float normVal, float opacity)
{
	vector<double>cps = GetControlPoints();
	int index = leftControlIndex(normVal)*2;
	cps.insert(cps.begin()+index++, opacity);
	cps.insert(cps.begin()+index,normVal);
                          
}

//----------------------------------------------------------------------------
// Add a new control point to the opacity map.
//----------------------------------------------------------------------------
void OpacityMapBase::addControlPoint(float value, float opacity)
{
 
  float normVal = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());
  addNormControlPoint(normVal, opacity);
                           
}

//----------------------------------------------------------------------------
// Delete the control point.
//----------------------------------------------------------------------------
void OpacityMapBase::deleteControlPoint(int index)
{
	vector<double>cps = GetControlPoints();
  if (index >= 0 && index < cps.size()/2 -1 && cps.size() > 2)
  {
	cps.erase(cps.begin()+2*index, cps.begin()+2*index+1);
    SetControlPoints(cps);
  }
}

//----------------------------------------------------------------------------
// Move the control point.
//---------------------------------------------------------------------------- 
void OpacityMapBase::moveControlPoint(int index, float dx, float dy)
{
	vector<double>cps = GetControlPoints();
  if (index >= 0 && index < cps.size()/2 -1)
  {
    
    float minVal = 0.0;
    float maxVal = 1.0;
    float ndx = dx / (GetMaxValue() - GetMinValue());

    if (index != 0)
    {
      minVal = cps[2*index-1];
    }

    if (index < cps.size()/2 -1)
    {
      maxVal = cps[2*index+3];
    }

    float value = cps[2*index+1] + ndx;

    if (value < minVal)
    {
      value = minVal;
    }
    else if (value > maxVal)
    {
      value = maxVal;
    }
    
    cps[2*index+1]=value;
    
    float opacity = cps[2*index] + dy;
    
    if (opacity < 0.0)
    {
      opacity = 0.0;
    }
    else if (opacity > 1.0)
    {
      opacity = 1.0;
    }

    cps[2*index] = opacity;
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
  vector<double> cps = GetControlPoints();
  if (index >= cps.size()/2 -1)
  {
    return 1.0;
  }

  return cps[2*index];
}

//----------------------------------------------------------------------------
// Set the control point's opacity.
//---------------------------------------------------------------------------- 
void OpacityMapBase::controlPointOpacity(int index, float opacity)
{
  vector<double> cps = GetControlPoints();
  if (index < 0 || 2*index >= cps.size())
    return;

  if (opacity < 0.0) opacity = 0.;
  else if (opacity > 1.0) opacity = 1.;
  
  cps[index*2]=opacity;
 
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
  vector<double> cps = GetControlPoints();
  if (2*index+1 >= cps.size())
  {
    return GetMaxValue();
  }

  float norm = cps[2*index+1];

  return (norm * (GetMaxValue() - GetMinValue()) + GetMinValue());
}

//----------------------------------------------------------------------------
// Set the control point's value (in data coordinates).
//---------------------------------------------------------------------------- 
void OpacityMapBase::controlPointValue(int index, float value)
{
  vector<double> cps = GetControlPoints();
  if (index < 0 || index*2 >= cps.size())
  {
    return;
  }


  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());

  float minVal = 0.0;
  float maxVal = 1.0;

  if (index >0)
  {
    minVal = cps[2*index -1];
  }
  
  if (index*2+3 < cps.size())
  {
    maxVal = cps[2*index+3];
  }

  if (nv < minVal)
  {
    nv = minVal;
  }
  else if (nv > maxVal)
  {
    nv = maxVal;
  }
  
  cps[2*index+1] = nv;
}

//----------------------------------------------------------------------------
// Set the mean value (normalized coordinates) for the gaussian function
//----------------------------------------------------------------------------
void OpacityMapBase::SetMean(double mean)
{
  SetValueDouble(_meanTag, "Set opacity mean", mean, _mapper->getParams());
}

//----------------------------------------------------------------------------
// Set the sigma squared value (normalized coordinates) for the gaussian 
// function
//----------------------------------------------------------------------------
void OpacityMapBase::SetSSQ(double ssq)
{
	SetValueDouble(_ssqTag,"Set Opac SSQ", ssq, _mapper->getParams());
}

//----------------------------------------------------------------------------
// Set the frequency (normalized coordinates) of the sine function
//----------------------------------------------------------------------------
void OpacityMapBase::SetFreq(double freq) 
{ 
  SetValueDouble(_freqTag,"Set Opac Freq", freq, _mapper->getParams());
}

//----------------------------------------------------------------------------
// Set the phase (normalized coordinates) of the sine function
//----------------------------------------------------------------------------
void OpacityMapBase::SetPhase(double p)
{
	SetValueDouble(_phaseTag, "Set Opac Phase", denormSinePhase(p), _mapper->getParams());
} 


//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
float OpacityMapBase::opacity(float value)
{
  float nv = (value - GetMinValue()) / (GetMaxValue() - GetMinValue());
  vector<double>cps = GetControlPoints();
  if (value < GetMinValue())
  {
    nv = 0;
  }
  
  if (value > GetMaxValue())
  {
    nv = 1.0;
  }
  OpacityMapBase::Type _type = GetType();
  switch(_type)
  {
    case CONTROL_POINT:
    {
      //
      // Find the bounding control points
      //
      int index = leftControlIndex(nv);
      double val0 = cps[2*index+1];
	  double val1 = cps[2*index+3];

      float ratio = (nv - val0)/(val1 - val0);

      if (ratio > 0. && ratio < 1.)
      {
        
        float o = TFInterpolator::interpolate(GetInterpType(), 
                                              cps[2*index],
                                              cps[2*index+2],
                                              ratio);
        return o;
      }
      
      if (ratio >= 1.0)
      {
        return cps[2*index+2];
      }
      
      return cps[2*index];
    }

    case GAUSSIAN:
    {
      return pow(M_E, -((nv-GetMean()) * (nv-GetMean()))/(2.0*GetSSQ()));
    }

    case INVERTED_GAUSSIAN:
    {
      return 1.0-pow(M_E, -((nv-GetMean()) * (nv-GetMean()))/(2.0*GetSSQ()));
    }

    case SINE:
    {
      return (0.5 + sin(GetFreq() * M_PI * nv + GetPhase())/2);
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
int OpacityMapBase::leftControlIndex(float normval)
{
  vector<double>cps = GetControlPoints();
  int left = 0;
  int right = cps.size()/2 - 1;

  //
  // Iterate, keeping left to the left of ctrl point
  //
  while (right-left > 1)
  {
    int mid = left+ (right-left)/2;
	if (cps[mid*2+1] > normval)
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
double OpacityMapBase::normSSq(double ssq) 
{
  return (GetSSQ() - _minSSq) / (_maxSSq - _minSSq);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::denormSSq(double ssq) 
{
  return _minSSq + (GetSSQ() * (_maxSSq - _minSSq));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::normSineFreq(double freq) 
{
  return (GetFreq() - _minFreq) / (_maxFreq - _minFreq);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::denormSineFreq(double freq) 
{
  return _minFreq + (GetFreq() * (_maxFreq - _minFreq));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::normSinePhase(double phase) 
{
  return (GetPhase() - _minPhase) / (_maxPhase - _minPhase);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
double OpacityMapBase::denormSinePhase(double phase)
{
  return _minPhase + (GetPhase() * (_maxPhase - _minPhase));
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void OpacityMapBase::setOpaque()
{
  vector<double> cps = GetControlPoints();
  for(int i=0; i<cps.size()/2; i++)
  {
    cps[2*i] = 1.;
  }
}
bool OpacityMapBase::isOpaque()
{
  vector<double> cps = GetControlPoints();
  for(int i=0; i<cps.size(); i+=2)
  {
    if (cps[i] < 1.0) return false;
  }
  return true;
}

void OpacityMapBase::SetMinValue(double val) {SetValueDouble(_minTag,"Set min opacity map value", val, _mapper->getParams());}
void OpacityMapBase::SetMaxValue(double val) {SetValueDouble(_maxTag,"Set max opacity map value", val, _mapper->getParams());}
void OpacityMapBase::SetEnabled(bool val) {SetValueLong(_enabledTag, "Change opacity map enabled", (long)val, _mapper->getParams());}
void OpacityMapBase::SetControlPoints(vector<double> controlPoints){
	SetValueDouble(_controlPointsTag,"Set opacity control points", controlPoints, _mapper->getParams());
}
void OpacityMapBase::SetInterpType(TFInterpolator::type t){
	SetValueLong(_interpTypeTag, "Set Opacity Interpolation", (long)t, _mapper->getParams());
}
void OpacityMapBase::SetType(OpacityMapBase::Type t){
	SetValueLong(_typeTag, "Set Opacity Map Type", (long)t, _mapper->getParams());
}
//----------------------------------------------------------------------------
// Return the minimum value of the opacity map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float OpacityMapBase::minValue()
{
  return _mapper->getMinMapValue() + GetMinValue() * 
    (_mapper->getMaxMapValue() - _mapper->getMinMapValue());
}

//----------------------------------------------------------------------------
// Set the minimum value of the opacity map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void OpacityMapBase::minValue(float value)
{
  SetMinValue(
    MAX(0.0, 
        (value - _mapper->getMinMapValue()) / 
        (_mapper->getMaxMapValue() - _mapper->getMinMapValue()))
		);
}

//----------------------------------------------------------------------------
// Return the maximum value of the opacity map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space.  Slightly increased to ensure that, even with
// numerical error, the max value always is in the opacity map domain
//----------------------------------------------------------------------------
float OpacityMapBase::maxValue()
{
  return _mapper->getMinMapValue() + GetMaxValue() * 1.0001 * 
    (_mapper->getMaxMapValue() - _mapper->getMinMapValue());
}

//----------------------------------------------------------------------------
// Set the maximum value of the opacity map (in data coordinates).
// 
// The maximum value is stored as normalized coordinates in the parameter 
// space. Therefore, the opacity map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void OpacityMapBase::maxValue(float value)
{
  SetMaxValue(
    MIN(1.0, 
        (value - _mapper->getMinMapValue()) / 
        (_mapper->getMaxMapValue()-_mapper->getMinMapValue()))
		);
}
