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
	MapperFunctionBase()
{	
	myParams = 0;

	// Delete ColorMapBase created by parent class
	if (_colormap) delete _colormap;	
    
	_colormap = new Colormap(NULL);
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(RenderParams* p, int nBits) :
	MapperFunctionBase(nBits)
{
	myParams = p;

	// Delete ColorMapBase and OpacityMapBase created by parent class
	if (_colormap) delete _colormap;	

	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();


	// Now recreate them with the appropriate type
	//
    _colormap = new Colormap(myParams);

    _opacityMaps.push_back(new OpacityMap(myParams));
}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const MapperFunction &mapper) :
	MapperFunctionBase(mapper),
	myParams(mapper.myParams)
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
	const Colormap &cmap =  (const Colormap &) (*(mapper._colormap));
	_colormap = new Colormap(cmap);

	for (int i=0; i<mapper._opacityMaps.size(); i++) {
		_opacityMaps.push_back(new OpacityMap((const OpacityMap &) (*mapper._opacityMaps[i])));
	}
}

MapperFunction::MapperFunction(const MapperFunctionBase &mapper) :
	MapperFunctionBase(mapper)
{
	myParams = NULL;
	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();

	for (int i=0; i<mapper.getNumOpacityMaps(); i++) {
		const OpacityMapBase *omap =  mapper.getOpacityMap(i);
		_opacityMaps.push_back(new OpacityMap((const OpacityMap &) *omap));
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
void MapperFunction::setParams(RenderParams* p) 
{
  myParams = p; 

  Colormap *cmap = (Colormap *) _colormap;
  
  cmap->setParams(p); 

  for (int i=0; i<_opacityMaps.size(); i++)
  {
    OpacityMap *omap = (OpacityMap *) _opacityMaps[i];
    omap->setParams(p);
  }
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityMap* MapperFunction::createOpacityMap(OpacityMap::Type type)
{
  OpacityMap *omap = new OpacityMap(myParams, type);

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
