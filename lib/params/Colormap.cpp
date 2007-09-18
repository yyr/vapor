//--Colormap.cpp ------------------------------------------------------------
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
#include "Colormap.h"
#include "params.h"

using namespace std;
using namespace VAPoR;

//############################################################################


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Colormap::Colormap(RenderParams *params) : 
  ColorMapBase(),
  _params(params)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
Colormap::Colormap(const Colormap &cmap) : 
  ColorMapBase(cmap),
  _params(cmap._params)
{
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
Colormap::~Colormap()
{
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
