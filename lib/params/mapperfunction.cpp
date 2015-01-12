//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		mapperfunction.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Implements the MapperFunction class
//		Provides separate mapping of color and opacity with separate domain
//		bounds.
//
//
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>

#include <vapor/ParamsBase.h>
#include <vapor/CFuncs.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/tfinterpolator.h>
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vapor/common.h>
#include "mapperfunction.h"
#include "params.h"
#include <vapor/ParamNode.h>

using namespace VAPoR;
using namespace VetsUtil;
const string IsoControl::_leftHistoBoundAttr = "LeftHistoBound";
const string IsoControl::_rightHistoBoundAttr = "RightHistoBound";
const string IsoControl::_leftHistoBoundTag = "LeftHistoBound";
const string IsoControl::_rightHistoBoundTag = "RightHistoBound";

//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunction::MapperFunction() : 
MapperFunctionBase(MapperFunctionBase::_mapperFunctionTag)

{	
	// Delete ColorMapBase created by parent class
	if (_colormap) delete _colormap;	
 
	_colormap = new VColormap(NULL);
	_colormap->interpType(TFInterpolator::linear);
}

//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const string& tag) : 
MapperFunctionBase(tag)
{	
	// Delete ColorMapBase created by parent class
	if (_colormap) delete _colormap;	
 
	_colormap = new VColormap(NULL);
	_colormap->interpType(TFInterpolator::linear);
}
//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(RenderParams* p, int nBits) :
  MapperFunctionBase(nBits,MapperFunctionBase::_mapperFunctionTag)
{
	// Delete ColorMapBase and OpacityMapBase created by parent class
	if (_colormap) delete _colormap;	

	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();


	// Now recreate them with the appropriate type
	//
    _colormap = new VColormap(this);
	_colormap->interpType(TFInterpolator::linear);

    _opacityMaps.push_back(new OpacityMap(this));
}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const MapperFunction &mapper) :
  MapperFunctionBase(mapper)
{
	_params = mapper._params;
	// Delete ColorMapBase and OpacityMapBase created by parent class
	if (_colormap) delete _colormap;	

	for (int i=0; i<_opacityMaps.size(); i++) 
    {
      delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }

    _opacityMaps.clear();

	// Now recreate them with the appropriate type
	//
	if (mapper._colormap){
		const VColormap &cmap =  (const VColormap &) (*(mapper._colormap));
		_colormap = new VColormap(cmap, this);
	}

	for (int i=0; i<mapper._opacityMaps.size(); i++) 
    {
      _opacityMaps.push_back(new OpacityMap((const OpacityMap &) 
                                            (*mapper._opacityMaps[i]), this));
	}
}

MapperFunction::MapperFunction(const MapperFunctionBase &mapper) :
  MapperFunctionBase(mapper)
{
	// Delete ColorMapBase and OpacityMapBase created by parent class
	if (_colormap) delete _colormap;	

	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();

	// Now recreate them with the appropriate type
	//
	const ColorMapBase *cmap =  mapper.getColormap();
	if (cmap){
		_colormap = new VColormap((const VColormap &) *cmap, this);
	}
	for (int i=0; i<mapper.getNumOpacityMaps(); i++) 
    {
      const OpacityMapBase *omap =  mapper.getOpacityMap(i);
      _opacityMaps.push_back(new OpacityMap((const OpacityMap &)*omap, this));
	}
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
MapperFunction::~MapperFunction() 
{
}


//----------------------------------------------------------------------------
// Calculate the color given the data value
//----------------------------------------------------------------------------
ARGB MapperFunction::colorValue(float value)
{
  if (_colormap)
  {
    float rgb[3];
    _colormap->color(value).toRGB(rgb);

    return ARGB((int)(255*rgb[0]), (int)(255*rgb[1]), (int)(255*rgb[2]));
  }
  
  return ARGB(0,0,0);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityMap* MapperFunction::createOpacityMap(OpacityMap::Type type)
{
  OpacityMap *omap = new OpacityMap(this, type);

  _opacityMaps.push_back(omap);

  return omap;
}

//----------------------------------------------------------------------------
//  
//----------------------------------------------------------------------------
OpacityMap* MapperFunction::getOpacityMap(int index)
{     
	return((OpacityMap *) MapperFunctionBase::getOpacityMap(index));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VColormap* MapperFunction::getColormap() {
    return((VColormap *)_colormap);
}


//----------------------------------------------------------------------------
// Constructor for empty, default IsoControl
//----------------------------------------------------------------------------
IsoControl::IsoControl() : 
  MapperFunction()
{	
	//delete opacity map:
	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();
	isoValues.clear();
	isoValues.push_back(0.);

}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
IsoControl::IsoControl(RenderParams* p, int nBits) :
  MapperFunction(p,nBits)
{
	
	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();

	isoValues.clear();
	isoValues.push_back(0.);

}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
IsoControl::IsoControl(const IsoControl &mapper) :
  MapperFunction(mapper)
{
	// Delete OpacityMapBase created by parent class
	

	for (int i=0; i<_opacityMaps.size(); i++) 
    {
      delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }

    _opacityMaps.clear();

	isoValues = mapper.isoValues;
	

}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
IsoControl::~IsoControl() 
{
}
