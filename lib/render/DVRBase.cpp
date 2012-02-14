//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		DVRBase.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Description:	Implementation of the DVRBase class
//		Modified by A. Norton to support a quick implementation of
//		volumizer renderer
//
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DVRBase.h"
using namespace VAPoR;
DVRBase::DVRBase() 
{ 
	_max_texture = 512;
	_range[0] = 0.0;
	_range[1] = 1.0;
}
