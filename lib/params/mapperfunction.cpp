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
#include "ParamsIso.h"

using namespace VAPoR;
using namespace VetsUtil;
const string IsoControl::_leftHistoBoundAttr = "LeftHistoBound";
const string IsoControl::_rightHistoBoundAttr = "RightHistoBound";

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
    _colormap = new Colormap(this);

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
	const Colormap &cmap =  (const Colormap &) (*(mapper._colormap));
	_colormap = new Colormap(cmap, this);

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
  
	//Otherwise it could be an isoValue tag:
	else if (StrCmpNoCase(tagString, ParamsIso::_IsoValueTag) == 0) 
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
	} else return false;
}

//----------------------------------------------------------------------------
// Construct an XML node from the transfer function
//----------------------------------------------------------------------------

XmlNode* IsoControl::buildNode(const string& tfname) 
{
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.empty();
	ostringstream oss;
	  
	//min and max opac map are really histo bounds
	oss << (double)getMinOpacMapValue();
	attrs[_leftHistoBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)getMaxOpacMapValue();
	attrs[_rightHistoBoundAttr] = oss.str();
	  
	// 
	// Add child node 
	//
  
	XmlNode* mainNode = new XmlNode(ParamsIso::_IsoControlTag, attrs,0);
	//add an isovalue node:
	vector<double> isoval;
	isoval.clear();
	isoval.push_back(isoValue);
	mainNode->SetElementDouble(ParamsIso::_IsoValueTag, isoval);
	return mainNode;
}



//----------------------------------------------------------------------------
// Handle