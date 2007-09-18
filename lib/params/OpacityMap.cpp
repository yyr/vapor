//--OpacityMap.cpp ------------------------------------------------------------
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
#include "OpacityMap.h"
#include "params.h"

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
// Constructor
//----------------------------------------------------------------------------
OpacityMap::OpacityMap(RenderParams *params, OpacityMap::Type type) :
  OpacityMapBase(type),
  _params(params)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
OpacityMap::OpacityMap(const OpacityMap &omap) :
  OpacityMapBase(omap),
  _params(omap._params)
{
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
OpacityMap::~OpacityMap()
{
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
// the parameter space.  Slightly increased to ensure that, even with
// numerical error, the max value always is in the opacity map domain
//----------------------------------------------------------------------------
float OpacityMap::maxValue() const
{
  return _params->getMinOpacMapBound() + _maxValue * 1.0001 * 
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
