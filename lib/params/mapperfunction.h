//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		mapperfunction.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Defines the MapperFunction class  
//		This is the mathematical definition of a function
//		that can be used to map data to either colors or opacities
//		Subclasses can either implement color or transparency 
//		mapping (or both)
#ifndef MAPPERFUNCTION_H
#define MAPPERFUNCTION_H
#define MAXCONTROLPOINTS 50
#include <iostream>
#include <qcolor.h>
#include <vapor/tfinterpolator.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/MapperFunctionBase.h>
//#include "params.h"
#include "OpacityMap.h"
#include "Colormap.h"

namespace VAPoR {
class Params;
class XmlNode;

class PARAMS_API MapperFunction : public MapperFunctionBase 
{

public:
	MapperFunction();
	MapperFunction(RenderParams* p, int nBits = 8);
	MapperFunction(const MapperFunction &mapper);
	MapperFunction(const MapperFunctionBase &mapper);
	virtual ~MapperFunction();

	void setParams(RenderParams* p);
	RenderParams* getParams() { return myParams; }

    //
    // Function values
    //
    QRgb  colorValue(float point);

    //
    // Opacity Maps
    //
    virtual OpacityMap* createOpacityMap(OpacityMap::Type type=OpacityMap::CONTROL_POINT);

	virtual OpacityMap* MapperFunction::getOpacityMap(int index);

	virtual Colormap*   getColormap();



protected:

    //
    // Parent params
    //
	RenderParams* myParams;
	
};
};
#endif //MAPPERFUNCTION_H
