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


