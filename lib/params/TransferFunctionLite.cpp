//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		TransferFunctionLite.cpp
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TransferFunctionLite class  
//		This is the mathematical definition of the transfer function
//		It is defined in terms of floating point coordinates, converted
//		into a mapping of quantized values (LUT) with specified domain at
//		rendering time.  Interfaces to the TFEditor class.
//		The TransferFunctionLite deals with an mapping on the interval [0,1]
//		that is remapped to a specified interval by the user.
//
//----------------------------------------------------------------------------

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
#include <vapor/TransferFunctionLite.h>
#include <vapor/common.h>

#include <vapor/MyBase.h>
#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include <vapor/tfinterpolator.h>
#include "transferfunction.h"

using namespace VAPoR;
using namespace VetsUtil;


//----------------------------------------------------------------------------
// Constructor for empty, default transfer function
//----------------------------------------------------------------------------
TransferFunctionLite::TransferFunctionLite() : MapperFunctionBase("")
{	
	createOpacityMap();
	_colormap = new ColorMapBase();
}

//----------------------------------------------------------------------------
// Construtor 
//
//----------------------------------------------------------------------------
TransferFunctionLite::TransferFunctionLite(int nBits) : 
  MapperFunctionBase(nBits, "")
{
	createOpacityMap();
	_colormap = new ColorMapBase();
}
	
//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
TransferFunctionLite::~TransferFunctionLite() 
{
}

//----------------------------------------------------------------------------
// Save the transfer function to a file. 
//----------------------------------------------------------------------------
bool TransferFunctionLite::saveToFile(ofstream& ofs)
{
	TransferFunction tf(*this);
	tf.saveToFile(ofs);

	return true;
}
