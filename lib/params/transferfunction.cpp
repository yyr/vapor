//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		transferfunction.cpp//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TransferFunction class  
//		This is the mathematical definition of the transfer function
//		It is defined in terms of floating point coordinates, converted
//		into a mapping of quantized values (LUT) with specified domain at
//		rendering time.  Interfaces to the TFEditor and DvrParams classes.
//		The TransferFunction deals with an mapping on the interval [0,1]
//		that is remapped to a specified interval by the user.
//
//----------------------------------------------------------------------------

#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include <vapor/tfinterpolator.h>
#include "transferfunction.h"
#include "assert.h"
#include <vapor/common.h>
#include "dvrparams.h"

#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
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

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
// Static member initialization.  Acceptable tags in transfer function output
// Sooner or later we may want to support 
//----------------------------------------------------------------------------
const string TransferFunction::_transferFunctionTag = "TransferFunction";
const string TransferFunction::_tfNameAttr = "Name";
const string TransferFunction::_leftBoundAttr = "LeftBound";
const string TransferFunction::_rightBoundAttr = "RightBound";
const string TransferFunction::_leftBoundTag = "LeftBound";
const string TransferFunction::_rightBoundTag = "RightBound";
const string TransferFunction::_tfNameTag = "Name";

//----------------------------------------------------------------------------
// Constructor for empty, default transfer function
//----------------------------------------------------------------------------
TransferFunction::TransferFunction() : MapperFunction(_transferFunctionTag)
{	
	
}

//----------------------------------------------------------------------------
// Reset to starting values
//----------------------------------------------------------------------------
void TransferFunction::init()
{
	numEntries = 256;
	setMinMapValue(0.f);
	setMaxMapValue(1.f);

    for (int i=0; i<_opacityMaps.size(); i++)
    {
      if (_opacityMaps[i]) delete _opacityMaps[i];
      _opacityMaps[i] = NULL;
    }    

    _opacityMaps.clear();
    _colormap->clear();
}

//----------------------------------------------------------------------------
// Construtor 
//
// Currently this is only for use in a dvrparams panel and a probe
//----------------------------------------------------------------------------
TransferFunction::TransferFunction(RenderParams* p, int nBits) : 
  MapperFunction(p, nBits)
{
}

TransferFunction::TransferFunction(const MapperFunctionBase &mapper) :
  MapperFunction(mapper)
{
}
	
//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
TransferFunction::~TransferFunction() 
{
}

//----------------------------------------------------------------------------
// Construct an XML node from the transfer function
//----------------------------------------------------------------------------

ParamNode* TransferFunction::buildNode(const string& tfname) 
{
	ParamNode* node = buildNode();
	if (!tfname.empty())
	{
		node->SetElementString(_tfNameTag,tfname);
	}
	return node;
}
ParamNode* TransferFunction::buildNode() 
{
  // Construct the main node
  string empty;
  std::map <string, string> attrs;
  attrs.empty();
  ostringstream oss;

  attrs[_typeAttr] = ParamNode::_paramsBaseAttr;


  // 
  // Add children nodes 
  //
  int numChildren = _opacityMaps.size()+4; // opacity maps + 1 colormap + 3 values

  ParamNode* mainNode = new ParamNode(_transferFunctionTag, attrs, numChildren);

 
	mainNode->SetElementDouble(_leftBoundTag, (double) getMinMapValue());
	mainNode->SetElementDouble(_rightBoundTag, (double) getMaxMapValue());
	mainNode->SetElementLong(_opacityCompositionTag,(long)getOpacityComposition());


  //
  // Opacity maps
  //
  for (int i=0; i<_opacityMaps.size(); i++)
  {
	  mainNode->XmlNode::AddChild(_opacityMaps[i]->buildNode());
  }

  //
  // Color map
  //
  mainNode->XmlNode::AddChild(_colormap->buildNode());
  
  return mainNode;
}


//----------------------------------------------------------------------------
// Create a transfer function by parsing a file.
//----------------------------------------------------------------------------
TransferFunction* TransferFunction::loadFromFile(ifstream& is, 
                                                 RenderParams *params)
{
  TransferFunction* newTF = new TransferFunction(params);

  //
  // Create an Expat XML parser to parse the XML formatted metadata file
  // specified by 'path'
  //
  ExpatParseMgr* parseMgr = new ExpatParseMgr(newTF);
  parseMgr->parse(is);
  delete parseMgr;
  ParamNode* pNode = newTF->buildNode();
  newTF->SetRootParamNode(pNode);
  pNode->SetParamsBase(newTF);
  return newTF;
}

//----------------------------------------------------------------------------
// Save the transfer function to a file. 
//----------------------------------------------------------------------------
bool TransferFunction::saveToFile(ofstream& ofs)
{
  const std::string emptyString;
  ParamNode* rootNode = buildNode();

  ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" 
      << endl;

  XmlNode::streamOut(ofs,(*rootNode));

  delete rootNode;

  return true;
}


//----------------------------------------------------------------------------
// Handlers for Expat parsing. The parse state is determined by whether it's 
// parsing a color or opacity.
//----------------------------------------------------------------------------
bool TransferFunction::elementStartHandler(ExpatParseMgr* pm, int depth , 
                                           std::string& tagString, 
                                           const char **attrs)
{
  if (StrCmpNoCase(tagString, _transferFunctionTag) == 0 || StrCmpNoCase(tagString, _mapperFunctionTag) == 0)
  {
    //If it's a TransferFunction tag, save the left/right bounds attributes.
    //Ignore the name tag
    //Do this by repeatedly pulling off the attribute name and value
    //begin by  resetting everything to starting values.
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
      else if (StrCmpNoCase(attribName, _tfNameAttr) == 0) 
      {
        ist >> mapperName;
      }
      else if (StrCmpNoCase(attribName, _leftBoundAttr) == 0 ||StrCmpNoCase(attribName, _leftColorBoundAttr) == 0)
      {
        float floatval;
        ist >> floatval;
        setMinMapValue(floatval);
      }
      else if (StrCmpNoCase(attribName, _rightBoundAttr) == 0 || StrCmpNoCase(attribName, _rightColorBoundAttr) == 0)
      {
        float floatval;
        ist >> floatval;
        setMaxMapValue(floatval);
      }   
      else if (StrCmpNoCase(attribName, _opacityCompositionAttr) == 0) 
      {
        int type;
        ist >> type;
        setOpacityComposition((CompositionType)type);
      }
    }

    return true;
  }
  else if (StrCmpNoCase(tagString, _colorControlPointTag) == 0) 
  {
    //Create a color control point with default values,
    //and with specified position
    //Currently only support HSV colors
    //Repeatedly pull off attribute name and values
    string attribName;
    float hue = 0.f, sat = 1.f, val=1.f, posn=0.f;

    while (*attrs)
    {
      attribName = *attrs;
      attrs++;
      string value = *attrs;
      attrs++;
      istringstream ist(value);
      if (StrCmpNoCase(attribName, _positionAttr) == 0) 
      {
        ist >> posn;
      } else if (StrCmpNoCase(attribName, _hsvAttr) == 0) 
      { 
        ist >> hue;
        ist >> sat;
        ist >> val;
      } 
    }

    _colormap->addNormControlPoint(posn, VColormap::Color(hue, sat, val));

    return true;

  }
  else if (StrCmpNoCase(tagString, _opacityControlPointTag) == 0) 
  {
    //peel off position and opacity
    string attribName;
    float opacity = 1.f, posn = 0.f;

    while (*attrs)
    {
      attribName = *attrs;
      attrs++;
      string value = *attrs;
      attrs++;
      istringstream ist(value);
      if (StrCmpNoCase(attribName, _positionAttr) == 0) 
      {
        ist >> posn;
      } else if (StrCmpNoCase(attribName, _opacityAttr) == 0) 
      { 
        ist >> opacity;
      } 
    }

    if (_opacityMaps.size() == 0)
    {
      // Create a opacity to hold the new control points
      OpacityMap *map = createOpacityMap();
     
      // Remove default control points
      map->clear(); 
    }

    _opacityMaps[0]->addNormControlPoint(posn, opacity);

    return true;
  }
  else if (StrCmpNoCase(tagString, OpacityMap::xmlTag()) == 0) 
  {
    OpacityMap *map = createOpacityMap();
    pm->pushClassStack(map);

    return map->elementStartHandler(pm, depth, tagString, attrs);
  }
  else if (StrCmpNoCase(tagString, VColormap::xmlTag()) == 0) 
  {
    pm->pushClassStack(_colormap);

    return _colormap->elementStartHandler(pm, depth, tagString, attrs);
  }
  //Prepare for data tags, only in versions after 1.5
  else if ((StrCmpNoCase(tagString, _leftBoundTag) == 0) ||
			(StrCmpNoCase(tagString, _rightBoundTag) == 0) || 
			//MapperFunction tags are read here too:
			(StrCmpNoCase(tagString, _leftColorBoundTag) == 0) ||
			(StrCmpNoCase(tagString, _rightColorBoundTag) == 0) || 
			(StrCmpNoCase(tagString, _opacityCompositionTag) == 0) ||
			(StrCmpNoCase(tagString, _tfNameTag) == 0)) {
		//Should have a double or long type attribute
		string attribName = *attrs;
		attrs++;
		string value = *attrs;

		ExpatStackElement *state = pm->getStateStackTop();
		
		state->has_data = 1;
		if (StrCmpNoCase(attribName, _typeAttr) != 0) {
			pm->parseError("Invalid attribute : %s", attribName.c_str());
			return false;
		}
		
		state->data_type = value;
		return true;  
	} else {
		pm->skipElement(tagString, depth);
		return true;
	}
}


//----------------------------------------------------------------------------
// The end handler needs to pop the parse stack, if this is not the top level
//----------------------------------------------------------------------------
bool TransferFunction::elementEndHandler(ExpatParseMgr* pm, int depth , 
                                         std::string& tag)
{
	  //Check for the transferfunction tag,
	if (StrCmpNoCase(tag, _transferFunctionTag) == 0) {
	  
		  //If depth is 0, this is a transfer function file; otherwise need to
		  //pop the parse stack.  The caller will need to save the resulting
		  //transfer function (i.e. this)
		  if (depth == 0) return true;
			
		  ParsedXml* px = pm->popClassStack();
		  bool ok = px->elementEndHandler(pm, depth, tag);
		  return ok;
	} //Otherwise need to obtain data values
	else if (StrCmpNoCase(tag, _leftBoundTag) == 0 || StrCmpNoCase(tag, _leftColorBoundTag) == 0){
		vector<double> val = pm->getDoubleData();
		setMinMapValue((float)val[0]);
		return true;
	} else if (StrCmpNoCase(tag, _rightBoundTag) == 0 || StrCmpNoCase(tag, _rightColorBoundTag) == 0){
		vector<double> val = pm->getDoubleData();
		setMaxMapValue((float)val[0]);
		return true;
	} else if (StrCmpNoCase(tag, _opacityCompositionTag) == 0){
		vector<long> val = pm->getLongData();
		setOpacityComposition((CompositionType)val[0]);
		return true;
	} else if (StrCmpNoCase(tag, _tfNameTag) == 0){
		const string& val = pm->getStringData();
		mapperName = val;
		return true;
	}
	//Ignore other tags
	else return true;
}
