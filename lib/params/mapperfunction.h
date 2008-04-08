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

#include "OpacityMap.h"
#include "Colormap.h"

namespace VAPoR {
class Params;
class RenderParams;
class XmlNode;

class PARAMS_API MapperFunction : public MapperFunctionBase 
{

public:
	MapperFunction();
	MapperFunction(RenderParams* p, int nBits = 8);
	MapperFunction(const MapperFunction &mapper);
	MapperFunction(const MapperFunctionBase &mapper);
	virtual ~MapperFunction();

	void setParams(RenderParams* p) { _params = p; }
	RenderParams* getParams()       { return _params; }

    //
    // Function values
    //
    QRgb  colorValue(float point);

    //
    // Opacity Maps
    //
    virtual OpacityMap* createOpacityMap(OpacityMap::Type type=OpacityMap::CONTROL_POINT);

	virtual OpacityMap* getOpacityMap(int index);

	virtual Colormap*   getColormap();



protected:

    //
    // Parent params
    //
	RenderParams* _params;
	
};
class PARAMS_API IsoControl : public MapperFunction{
public:
	IsoControl();
	IsoControl(RenderParams* p, int nBits = 8);
	//Copy constructor
	IsoControl(const IsoControl &mapper);
	virtual ~IsoControl();
	void setIsoValue(double val){isoValue = val;}
	double getIsoValue(){return isoValue;}
	
	virtual XmlNode* buildNode(const string&);
	void setMinHistoValue(float val){setMinOpacMapValue(val);}
	void setMaxHistoValue(float val){setMaxOpacMapValue(val);}
	float getMinHistoValue() {return getMinOpacMapValue();}
	float getMaxHistoValue() {return getMaxOpacMapValue();}
	
	virtual bool elementStartHandler(ExpatParseMgr*, int depth, 
                                     std::string&, const char **);

	virtual bool elementEndHandler(ExpatParseMgr*, int, std::string&);
protected:
	static const string _leftHistoBoundAttr;
	static const string _rightHistoBoundAttr;
	double isoValue;
};
};
#endif //MAPPERFUNCTION_H
