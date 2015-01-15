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
#include <vapor/ParamNode.h>
#include "OpacityMap.h"
#include <vapor/MapperFunctionBase.h>

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
OpacityMap::OpacityMap(MapperFunctionBase *mapper, OpacityMap::Type type) :
  OpacityMapBase(type)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
OpacityMap::OpacityMap(const OpacityMap &omap, MapperFunctionBase *mapper) :
  OpacityMapBase(omap)
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
float OpacityMap::minValue()
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
void OpacityMap::minValue(float value)
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
float OpacityMap::maxValue()
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
void OpacityMap::maxValue(float value)
{
  SetMaxValue(
    MIN(1.0, 
        (value - _mapper->getMinMapValue()) / 
        (_mapper->getMaxMapValue()-_mapper->getMinMapValue()))
		);
}
