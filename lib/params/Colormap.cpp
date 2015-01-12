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
#include <vapor/ParamNode.h>
#include "assert.h"
#include "Colormap.h"
#include <vapor/MapperFunctionBase.h>

using namespace std;
using namespace VAPoR;

//############################################################################


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
VColormap::VColormap(MapperFunctionBase *mapper) :
  ColorMapBase()
  
{
	_mapper = mapper;
}

//----------------------------------------------------------------------------
// Copy constructor
//----------------------------------------------------------------------------
VColormap::VColormap(const VColormap &cmap, MapperFunctionBase *mapper) : 
  ColorMapBase(cmap)
{
	_mapper=mapper;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
VColormap::~VColormap()
{
}



//----------------------------------------------------------------------------
// Return the minimum value of the color map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float VColormap::minValue()
{
  assert(_mapper);

  return _mapper->getMinColorMapValue() + (float)GetMinValue() * 
    (_mapper->getMaxColorMapValue() - _mapper->getMinColorMapValue());
}

//----------------------------------------------------------------------------
// Set the minimum value of the color map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void VColormap::minValue(float value)
{
  assert(_mapper);

  SetMinValue( 
    (value - _mapper->getMinColorMapValue()) / 
    (_mapper->getMaxColorMapValue() - _mapper->getMinColorMapValue())
	);
}

//----------------------------------------------------------------------------
// Return the maximum value of the color map (in data coordinates).
// 
// The minimum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
float VColormap::maxValue()
{
  assert(_mapper);

  return _mapper->getMinColorMapValue() + GetMaxValue() * 
    (_mapper->getMaxColorMapValue() - _mapper->getMinColorMapValue());
}

//----------------------------------------------------------------------------
// Set the maximum value of the color map (in data coordinates).
// 
// The maximum value is stored as normalized coordinates in the parameter 
// space. Therefore, the color map will change relative to any changes in
// the parameter space. 
//----------------------------------------------------------------------------
void VColormap::maxValue(float value)
{
  assert(_mapper);

  SetMaxValue(
    (value - _mapper->getMinColorMapValue()) / 
    (_mapper->getMaxColorMapValue()-_mapper->getMinColorMapValue())
	);
}
