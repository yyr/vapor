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
#include "assert.h"
#include "Colormap.h"
#include "vapor/MapperFunctionBase.h"

using namespace std;
using namespace VAPoR;

//############################################################################


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Colormap::Colormap(MapperFunctionBase *mapper) :
  ColorMapBase(),
  _mapper(mapper)
{
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
Colormap::Colormap(const Colormap &cmap, MapperFunctionBase *mapper) : 
  ColorMapBase(cmap),
  _mapper(mapper)
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
  assert(_mapper);

  return _mapper->getMinColorMapValue() + _minValue * 
    (_mapper->getMaxColorMapValue() - _mapper->getMinColorMapValue());
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
  assert(_mapper);

  _minValue = 
    (value - _mapper->getMinColorMapValue()) / 
    (_mapper->getMaxColorMapValue() - _mapper->getMinColorMapValue());
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
  assert(_mapper);

  return _mapper->getMinColorMapValue() + _maxValue * 
    (_mapper->getMaxColorMapValue() - _mapper->getMinColorMapValue());
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
  assert(_mapper);

  _maxValue = 
    (value - _mapper->getMinColorMapValue()) / 
    (_mapper->getMaxColorMapValue()-_mapper->getMinColorMapValue());
}
