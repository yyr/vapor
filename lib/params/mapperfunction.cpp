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
const string IsoControl::_isoValuesTag = "IsoValues";

//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunction::MapperFunction() : 
MapperFunctionBase(MapperFunctionBase::_mapperFunctionTag)

{	
	VColormap* _colormap = new VColormap(NULL);
	_colormap->SetInterpType(TFInterpolator::linear);
	SetColorMap(_colormap);
}

//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const string& tag) : 
MapperFunctionBase(tag)
{	
	VColormap* _colormap = new VColormap(NULL);
	_colormap->SetInterpType(TFInterpolator::linear);
	SetColorMap(_colormap);
}
//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(RenderParams* p, int nBits, const string& tag) :
  MapperFunctionBase(p, nBits,tag)
{
	
	 //
    // Delete the opacity maps
    //
	for (int i = opacityPaths.size()-1; i>=0; i--){
		OpacityMapBase* omap = GetOpacityMap(i); 
		deleteOpacityMap(omap);
	}
	
	opacityPaths.clear();

	// Now recreate them with the appropriate type
	//
    VColormap* _colormap = new VColormap(this);
	_colormap->SetInterpType(TFInterpolator::linear);
	SetColorMap(_colormap);

	createOpacityMap();
    
}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const MapperFunction &mapper) :
  MapperFunctionBase(mapper)
{
	_params = mapper._params;
	
}
  //----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const MapperFunctionBase &mapper) :
  MapperFunctionBase(mapper)
{
	_params = mapper.getParams();
	
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
	ColorMapBase* cmap = GetColorMap();
  if (cmap)
  {
    float rgb[3];
    cmap->color(value).toRGB(rgb);

    return ARGB((int)(255*rgb[0]), (int)(255*rgb[1]), (int)(255*rgb[2]));
  }
  
  return ARGB(0,0,0);
}


//----------------------------------------------------------------------------
// Constructor for empty, default IsoControl
//----------------------------------------------------------------------------
IsoControl::IsoControl() : 

  MapperFunction(Params::_IsoControlTag)
{	
	//
    // Delete the opacity maps
    //
	for (int i = opacityPaths.size()-1; i>=0; i--){
		OpacityMapBase* omap = GetOpacityMap(i); 
		deleteOpacityMap(omap);
	}
	
	opacityPaths.clear();
	vector<double>isovalues;
	isovalues.push_back(0.);
	setIsoValues(isovalues);

}


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
IsoControl::IsoControl(RenderParams* p, int nBits) :
  MapperFunction(p,nBits, Params::_IsoControlTag)
{
	
	//
    // Delete the opacity maps
    //
	for (int i = opacityPaths.size()-1; i>=0; i--){
		OpacityMapBase* omap = GetOpacityMap(i); 
		deleteOpacityMap(omap);
	}
	
	opacityPaths.clear();

	vector<double> isoValues;
	isoValues.push_back(0.);
	setIsoValues(isoValues);

}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
IsoControl::IsoControl(const IsoControl &mapper) :
  MapperFunction(mapper)
{
	
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
IsoControl::~IsoControl() 
{
}
void IsoControl::setIsoValue(double val){
	SetValueDouble(_isoValuesTag, "Set Isovalue", val, _params);
}
double IsoControl::getIsoValue(){
	return GetValueDouble(_isoValuesTag);
}
void IsoControl::setIsoValues(const vector<double>& vals){
	SetValueDouble(_isoValuesTag, "Set Isovalues", vals, _params);
}

const vector<double> IsoControl::getIsoValues(){
	return GetValueDoubleVec(_isoValuesTag);
}