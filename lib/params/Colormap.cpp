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




