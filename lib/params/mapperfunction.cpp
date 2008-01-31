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
#include <vapor/Metadata.h>
#include <vapor/CFuncs.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/tfinterpolator.h>
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vaporinternal/common.h>
#include <qcolor.h>
#include "mapperfunction.h"
#include "params.h"

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunction::MapperFunction() : 
  MapperFunctionBase(),
  _params(NULL)
{	
	// Delete ColorMapBase created by parent class
	if (_colormap) delete _colormap;	
    
	_colormap = new Colormap(NULL);
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(RenderParams* p, int nBits) :
  MapperFunctionBase(nBits),
  _params(p)
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
    _colormap = new Colormap(this);

    _opacityMaps.push_back(new OpacityMap(this));
}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const MapperFunction &mapper) :
  MapperFunctionBase(mapper),
  _params(mapper._params)
{
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
	const Colormap &cmap =  (const Colormap &) (*(mapper._colormap));
	_colormap = new Colormap(cmap, this);

	for (int i=0; i<mapper._opacityMaps.size(); i++) 
    {
      _opacityMaps.push_back(new OpacityMap((const OpacityMap &) 
                                            (*mapper._opacityMaps[i]), this));
	}
}

MapperFunction::MapperFunction(const MapperFunctionBase &mapper) :
  MapperFunctionBase(mapper),
  _params(NULL)
{
	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();

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
QRgb MapperFunction::colorValue(float value)
{
  if (_colormap)
  {
    float rgb[3];
    _colormap->color(value).toRGB(rgb);

    return qRgb((int)(255*rgb[0]), (int)(255*rgb[1]), (int)(255*rgb[2]));
  }
  
  return qRgb(0,0,0);
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
Colormap* MapperFunction::getColormap() {
    return((Colormap *)_colormap);
}
