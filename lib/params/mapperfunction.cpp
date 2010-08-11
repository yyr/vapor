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

#include "vapor/ParamsBase.h"
#include <vapor/CFuncs.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/tfinterpolator.h>
#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vaporinternal/common.h>
#include <qcolor.h>
#include "mapperfunction.h"
#include "params.h"
#include "ParamsIso.h"
#include "ParamNode.h"

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
MapperFunctionBase(MapperFunctionBase::_mapperFunctionTag),
  _params(NULL)
{	
	// Delete ColorMapBase created by parent class
	if (_colormap) delete _colormap;	
 
	_colormap = new VColormap(NULL);
}

//----------------------------------------------------------------------------
// Constructor for empty, default Mapper function
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(const string& tag) : 
MapperFunctionBase(tag),
  _params(NULL)
{	
	// Delete ColorMapBase created by parent class
	if (_colormap) delete _colormap;	
 
	_colormap = new VColormap(NULL);
}
//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
MapperFunction::MapperFunction(RenderParams* p, int nBits) :
  MapperFunctionBase(nBits,MapperFunctionBase::_mapperFunctionTag),
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
    _colormap = new VColormap(this);

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
  MapperFunctionBase(mapper),
  _params(NULL)
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
	isoValue = 0.;
	if (_colormap) delete _colormap;
	_colormap = 0;
}

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
IsoControl::IsoControl(RenderParams* p, int nBits) :
  MapperFunction(p,nBits)
{
	// Delete ColorMapBase and OpacityMapBase created by parent class
	if (_colormap) delete _colormap;	

	for (int i=0; i<_opacityMaps.size(); i++) {
		delete _opacityMaps[i];
		_opacityMaps[i] = NULL;
    }
    _opacityMaps.clear();

	isoValue = 0.;
	// Now recreate color map with the appropriate type
	//
    //_colormap = new Colormap(this);
	_colormap = 0;

}

//----------------------------------------------------------------------------
// Copy Constructor
//----------------------------------------------------------------------------
IsoControl::IsoControl(const IsoControl &mapper) :
  MapperFunction(mapper)
{
	// Delete ColorMapBase and OpacityMapBase created by parent class
	if (_colormap) delete _colormap;	

	for (int i=0; i<_opacityMaps.size(); i++) 
    {
      delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }

    _opacityMaps.clear();

	isoValue = mapper.isoValue;
	// Now recreate color map with the appropriate type
	//
	//const Colormap &cmap =  (const Colormap &) (*(mapper._colormap));
	//_colormap = new Colormap(cmap, this);
	_colormap = 0;

}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
IsoControl::~IsoControl() 
{
}
//----------------------------------------------------------------------------
// Handlers for Expat parsing. The parse state is determined by whether it's 
// parsing a color or opacity map.
//----------------------------------------------------------------------------
bool IsoControl::elementStartHandler(ExpatParseMgr* pm, 
                                         int depth , 
                                         std::string& tagString, 
                                         const char **attrs)
{
	if (StrCmpNoCase(tagString, ParamsIso::_IsoControlTag) == 0) 
	{
		//If it's a IsoControl tag, save the left/right opac bounds attributes.
		//These are really used only for histo bounds
    
		init();
    
		while (*attrs) 
		{
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
		    //Newer tf's will have ParamsBase node attribute; that's OK
			if (StrCmpNoCase(attribName, _typeAttr) == 0) 
			{
				  if (value != ParamNode::_paramsBaseAttr) return false;
			}
			if (StrCmpNoCase(attribName, _leftHistoBoundAttr) == 0) 
			{
				float floatval;
				ist >> floatval;
				setMinOpacMapValue(floatval);
			}
			else if (StrCmpNoCase(attribName, _rightHistoBoundAttr) == 0) 
			{
				float floatval;
				ist >> floatval;
				setMaxOpacMapValue(floatval);
			}      
			else
			{
				return false;
			}
		}
		return true;
	}
  
	//Otherwise it could be an isoValue or left/right histo bounds tag:
	else if (StrCmpNoCase(tagString, ParamsIso::_IsoValueTag) == 0 ||
		(StrCmpNoCase(tagString, _leftHistoBoundTag) == 0 ) ||
		(StrCmpNoCase(tagString, _rightHistoBoundTag) == 0 ))
		
	{
		//Should have a double type attribute
		string attribName = *attrs;
		attrs++;
		string value = *attrs;

		ExpatStackElement *state = pm->getStateStackTop();
			
		state->has_data = 1;
		if (StrCmpNoCase(attribName, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", attribName.c_str());
			return false;
		}
		if (StrCmpNoCase(value, _doubleType) != 0) {
			pm->parseError("Invalid type : %s", value.c_str());
			return false;
		}
		state->data_type = value;
		return true;  
	}
	else return false;
	
}


//----------------------------------------------------------------------------
// The end handler needs to pop the parse stack, 
//----------------------------------------------------------------------------
bool IsoControl::elementEndHandler(ExpatParseMgr* pm, int depth , 
                                       std::string& tag)
{
	//Check for the MapperFunctionBase tag, ignore others.
	if (StrCmpNoCase(tag, ParamsIso::_IsoControlTag) == 0) {
		//depth should not be zero; otherwise need to
		//pop the parse stack.  The caller will need to save the resulting
		//mapper function (i.e. this)
		if (depth == 0) return false;
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, ParamsIso::_IsoValueTag) == 0) {		
		vector<double> isoval = pm->getDoubleData();
		isoValue = isoval[0];
		return true;
	} else if (StrCmpNoCase(tag, _leftHistoBoundTag) == 0) {		
		vector<double> histval = pm->getDoubleData();
		setMinOpacMapValue(histval[0]);
		return true;
	} else if (StrCmpNoCase(tag, _rightHistoBoundTag) == 0) {		
		vector<double> histval = pm->getDoubleData();
		setMaxOpacMapValue(histval[0]);
		return true;
	} else return false;
}

//----------------------------------------------------------------------------
// Construct an XML node from the iso control
//----------------------------------------------------------------------------

ParamNode* IsoControl::buildNode() 
{
	 // Construct the main node
  string empty;
  std::map <string, string> attrs;
  attrs.empty();
  ostringstream oss;

  attrs[_typeAttr] = ParamNode::_paramsBaseAttr;


  
  ParamNode* mainNode = new ParamNode(ParamsIso::_IsoControlTag, attrs, 0);

 
	mainNode->SetElementDouble(_leftHistoBoundTag, (double) getMinOpacMapValue());
	mainNode->SetElementDouble(_rightHistoBoundTag, (double) getMaxOpacMapValue());
	
	//add an isovalue node:
	vector<double> isoval;
	isoval.clear();
	isoval.push_back(isoValue);
	mainNode->SetElementDouble(ParamsIso::_IsoValueTag, isoval);
	return mainNode;
}