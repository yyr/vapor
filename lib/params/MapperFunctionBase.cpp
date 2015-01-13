//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		MapperFunctionBase.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Implements the MapperFunctionBase class
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
#include <vapor/CFuncs.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/MapperFunctionBase.h>
#include <vapor/common.h>
#include <vapor/ColorMapBase.h>
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vapor/tfinterpolator.h>
#include <vapor/ParamsBase.h>
#include <vapor/ParamNode.h>

using namespace VAPoR;
using namespace VetsUtil;

const string MapperFunctionBase::_mapperFunctionTag = "MapperFunction";
const string MapperFunctionBase::_leftColorBoundAttr = "LeftColorBound";
const string MapperFunctionBase::_rightColorBoundAttr = "RightColorBound";
const string MapperFunctionBase::_leftOpacityBoundAttr = "LeftOpacityBound";
const string MapperFunctionBase::_rightOpacityBoundAttr = "RightOpacityBound";
const string MapperFunctionBase::_opacityCompositionAttr = "OpacityComposition";

const string MapperFunctionBase::_leftColorBoundTag = "LeftColorBound";
const string MapperFunctionBase::_rightColorBoundTag = "RightColorBound";
const string MapperFunctionBase::_leftOpacityBoundTag = "LeftOpacityBound";
const string MapperFunctionBase::_rightOpacityBoundTag = "RightOpacityBound";
const string MapperFunctionBase::_opacityCompositionTag = "OpacityComposition";
const string MapperFunctionBase::_hsvAttr = "HSV";
const string MapperFunctionBase::_positionAttr = "Position";
const string MapperFunctionBase::_opacityAttr = "Opacity";
const string MapperFunctionBase::_opacityControlPointTag = "OpacityControlPoint";
const string MapperFunctionBase::_colorControlPointTag = "ColorControlPoint";


//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunctionBase::MapperFunctionBase(const string& name) :
	ParamsBase(0, name)
{	
	
	previousClass = 0;
    opacityScaleFactor = 1.0;
	colorVarNum = 0;
    opacVarNum  = 0;	
    
    _colormap = NULL;
	_params = NULL;

    _compType = ADDITION;
	numEntries = 256;
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MapperFunctionBase::MapperFunctionBase(int nBits, const string& name) : 
	ParamsBase(name)
{
	previousClass = 0;
	//Currently ignore the nBits parameter:
	numEntries = 256;

	minColorMapBound = 0.f;
	maxColorMapBound = 1.f;
	minOpacMapBound = 0.f;
	maxOpacMapBound = 1.f;
    opacityScaleFactor = 1.0;
	colorVarNum = 0;
    opacVarNum  = 0;	

    _colormap = NULL;
	_params = NULL;
    _compType = ADDITION;
}


//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
MapperFunctionBase::~MapperFunctionBase() 
{

    if (_colormap) delete _colormap;
    _colormap = NULL;

    for (int i=0; i<_opacityMaps.size(); i++)
    {
      if (_opacityMaps[i]) delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }
    
    _opacityMaps.clear();
}

//----------------------------------------------------------------------------
// Calculate the opacity given the data value
//----------------------------------------------------------------------------
float MapperFunctionBase::opacityValue(float value)
{
  int count = 0;

  //
  // Using the square of the opacity scaling factor gives
  // better control over low opacity values.
  // But this correction will be made in the GUI
  //
  float opacScale = opacityScaleFactor;
  
  float opacity = 0.0;

  if (_compType == MULTIPLICATION)
  {
    opacity = 1.0;
  }

  for (int i=0; i<_opacityMaps.size(); i++)
  {
    OpacityMapBase *omap = _opacityMaps[i];

    if (omap->IsEnabled() && omap->bounds(value))
    {
      if (_compType == ADDITION)
      {
        opacity += omap->opacity(value);
      }
      else // _compType == MULTIPLICATION
      {
        opacity *= omap->opacity(value);
      }
        
      count++;

      if (opacity*opacScale > 1.0)
      {
        return 1.0;
      }
    }
  }

  if (count) return opacity*opacScale;

  return 0.0;
}

//----------------------------------------------------------------------------
// Calculate the color given the data value
//----------------------------------------------------------------------------
void MapperFunctionBase::hsvValue(float value, float *h, float *s, float *v)
{
  if (_colormap)
  {
    ColorMapBase::Color color = _colormap->color(value);

    *h = color.hue();
    *s = color.sat();
    *v = color.val();
  }
}

//----------------------------------------------------------------------------
// Populate at a RGBA lookup table 
//----------------------------------------------------------------------------
void MapperFunctionBase::makeLut(float* clut)
{
  float ostep = (getMaxOpacMapValue() - getMinOpacMapValue())/(numEntries-1);
  float cstep = (getMaxColorMapValue() - getMinColorMapValue())/(numEntries-1);

  for (int i = 0; i< numEntries; i++)
  {
    float ov = getMinOpacMapValue() + i*ostep;
    float cv = getMinColorMapValue() + i*cstep;

    _colormap->color(cv).toRGB(&clut[4*i]);
    clut[4*i+3] = opacityValue(ov);
  }
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityMapBase* MapperFunctionBase::createOpacityMap(OpacityMapBase::Type type)
{
  OpacityMapBase *omap = new OpacityMapBase(type);

  _opacityMaps.push_back(omap);

  return omap;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
OpacityMapBase* MapperFunctionBase::getOpacityMap(int index) const
{
  if (index < 0 || index >= _opacityMaps.size())
  {
    return NULL;
  }

  return _opacityMaps[index];
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ColorMapBase* MapperFunctionBase::getColormap() const {
	return(_colormap);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void MapperFunctionBase::deleteOpacityMap(OpacityMapBase *omap)
{
  vector<OpacityMapBase*>::iterator iter = 
    std::find(_opacityMaps.begin(), _opacityMaps.end(), omap);

  if (iter != _opacityMaps.end())
  {
	if (*iter) delete *iter;
    _opacityMaps.erase(iter);
  }
}

//----------------------------------------------------------------------------
// Reset to starting values
//----------------------------------------------------------------------------
void MapperFunctionBase::init()
{  	
	numEntries = 256;
	setMinColorMapValue(0.f);
	setMaxColorMapValue(1.f);
	setMinOpacMapValue(0.f);
	setMaxOpacMapValue(1.f);
    setOpacityScaleFactor(1.0f);

    //
    // Delete the opacity maps
    //
    for (int i=0; i<_opacityMaps.size(); i++)
    {
      if (_opacityMaps[i]) delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }
    
    _opacityMaps.clear();
    if(_colormap) _colormap->clear();
}

//----------------------------------------------------------------------------
// Static utility function to map and quantize a float
//----------------------------------------------------------------------------
int MapperFunctionBase::mapPosition(float x,  float minValue, float maxValue, 
                                int hSize)
{
	double psn = (0.5+((((double)x - (double)minValue)*hSize)/((double)maxValue - (double)minValue)));
	//constrain to integer size limit
	if(psn < -1000000000.f) psn = -1000000000.f;
	if(psn > 1000000000.f) psn = 1000000000.f;
	return (int)psn;
}


//----------------------------------------------------------------------------
// hsv-rgb Conversion function.  inputs and outputs between 0 and 1
// copied (with corrections) from Hearn/Baker
//----------------------------------------------------------------------------
void MapperFunctionBase::hsvToRgb(float* hsv, float* rgb)
{
	if (hsv[1] == 0.f) { //grey
		rgb[0] = rgb[1] = rgb[2] = hsv[2];
		return;
	}

	int sector = (int)(hsv[0]*6.f); 

	float sectCrd = hsv[0]*6.f - (float) sector;
	if (sector == 6) sector = 0;
	float a = hsv[2]*(1.f - hsv[1]);
	float b = hsv[2]*(1.f - sectCrd*hsv[1]);
	float c = hsv[2]*(1.f - (hsv[1]*(1.f - sectCrd)));

	switch (sector){
		case (0):
			//red to green, r>g
			rgb[0] = hsv[2];
			rgb[1] = c;
			rgb[2] = a;
			break;
		case (1): // red to green, g>r
			rgb[1] = hsv[2];
			rgb[2] = a;
			rgb[0] = b;
			break;
		case (2): //green to blue, gr>bl
			rgb[0] = a;
			rgb[1] = hsv[2];
			rgb[2] = c;
			break;
		case (3): //green to blue, gr<bl
			rgb[0] = a;
			rgb[2] = hsv[2];
			rgb[1] = b;
			break;
		case (4): //blue to red, bl>red
			rgb[1] = a;
			rgb[2] = hsv[2];
			rgb[0] = c;
			break;
		case (5): //blue to red, bl<red
			rgb[1] = a;
			rgb[0] = hsv[2];
			rgb[2] = b;
			break;
		default: assert(0);
	}
	return;

}

//----------------------------------------------------------------------------
// rgb-hsv Conversion function.  inputs and outputs between 0 and 1
// copied (with corrections) from Hearn/Baker
//----------------------------------------------------------------------------
void MapperFunctionBase::rgbToHsv(float* rgb, float* hsv)
{
	//value is max (r,g,b)
	float maxval = Max(rgb[0],Max(rgb[1],rgb[2]));
	float minval = Min(rgb[0],Min(rgb[1],rgb[2]));
	float delta = maxval - minval; //chrominance
	hsv[2] = maxval;
	if (maxval != 0.f) hsv[1] = delta/maxval;
	else hsv[1] = 0.f;
	if (hsv[1] == 0.f) hsv[0] = 0.f; //no hue!
	else {
		if (rgb[0] == maxval){
			hsv[0] = (rgb[1]-rgb[2])/delta;
			if (hsv[0]< 0.f) hsv[0]+= 6.f;
		} else if (rgb[1] == maxval){
			hsv[0] = 2.f + (rgb[2]-rgb[0])/delta;
		} else {
			hsv[0] = 4.f + (rgb[0]-rgb[1])/delta;
		}
		hsv[0] /= 6.f; //Put between 0 and 1
	}
	return;
}
	
//----------------------------------------------------------------------------
// Set the opacity function to 1.
//---------------------------------------------------------------------------- 
void MapperFunctionBase::setOpaque()
{
  // New-school
  for (int i=0; i<_opacityMaps.size(); i++)
  {
    _opacityMaps[i]->setOpaque();
  }
}
bool MapperFunctionBase::isOpaque()
{

  for (int i=0; i<_opacityMaps.size(); i++)
  {
    if(!_opacityMaps[i]->isOpaque()) return false;
  }
  return true;
}

