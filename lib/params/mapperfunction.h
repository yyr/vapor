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
#include "tfinterpolator.h"
#include "params.h"
#include "OpacityMap.h"
#include "vapor/ExpatParseMgr.h"
#include "Colormap.h"
#include <qcolor.h>
#include <iostream>

namespace VAPoR {
class Params;
class XmlNode;

class PARAMS_API MapperFunction : public ParsedXml 
{

public:
	MapperFunction();
	MapperFunction(RenderParams* p, int nBits);
	MapperFunction(const MapperFunction &mapper);
	virtual ~MapperFunction();

	void setParams(RenderParams* p);
	RenderParams* getParams() { return myParams; }

    //
    // Function values
    //
	float opacityValue(float point);
    QRgb  colorValue(float point);
    void  hsvValue(float point, float* h, float* sat, float* val);

	void setOpaque();

    //
	// Build a lookup table[numEntries][4]
	// (Caller must pass in an empty array)
	//
	void makeLut(float* clut);

    //
    // Data Bounds
    //
	float getMinColorMapValue() { return minColorMapBound; }
	float getMaxColorMapValue() { return maxColorMapBound; }
	float getMinOpacMapValue()  { return minOpacMapBound; }
	float getMaxOpacMapValue()  { return maxOpacMapBound; }

	void setMinColorMapValue(float val) { minColorMapBound = val; }
	void setMaxColorMapValue(float val) { maxColorMapBound = val; }
	void setMinOpacMapValue(float val)  { minOpacMapBound = val; }
	void setMaxOpacMapValue(float val)  { maxOpacMapBound = val; }

    //
    // Variables
    //
   
    int getColorVarNum() { return colorVarNum; }
    int getOpacVarNum() { return opacVarNum; }
   
    virtual void setVarNum(int var)     { colorVarNum = var; opacVarNum = var;}
    virtual void setColorVarNum(int var){ colorVarNum = var; }
    virtual void setOpacVarNum(int var) { opacVarNum = var; }

    //
    // Opacity Maps
    //
    OpacityMap* createOpacityMap(OpacityMap::Type type=OpacityMap::CONTROL_POINT);
    OpacityMap* getOpacityMap(int index);
    void        deleteOpacityMap(OpacityMap *omap);
    int         getNumOpacityMaps() { return (int)_opacityMaps.size(); }

    //
    // Opacity scale factor (scales all opacity maps)
    //
	void setOpacityScaleFactor(float val) { opacityScaleFactor = val; }
	float getOpacityScaleFactor()         { return opacityScaleFactor; }

    //
    // Colormap
    //
    Colormap*   getColormap() { return _colormap; }

    //
	// Color conversion 
    //
	static void hsvToRgb(float* hsv, float* rgb);
	static void rgbToHsv(float* rgb, float* hsv);
	
	//
    // Map a point to the specified range, and quantize it.
	//
	static int mapPosition(float x, float minValue, float maxValue, int hSize);

	int getNumEntries()         { return numEntries; }
	void setNumEntries(int val) { numEntries = val; }

    //
	// Map and quantize a real value to the corresponding table index
	// i.e., quantize to current Mapper function domain
	//
	int mapFloatToColorIndex(float point) 
    {
		
      int indx = mapPosition(point, 
                             getMinColorMapValue(), 
                             getMaxColorMapValue(), numEntries-1);
      if (indx < 0) indx = 0;
      if (indx > numEntries-1) indx = numEntries-1;
      return indx;
	}

	float mapColorIndexToFloat(int indx)
    {
      return (float)(getMinColorMapValue() + 
                     ((float)indx)*(float)(getMaxColorMapValue()-
                                           getMinColorMapValue())/
                     (float)(numEntries-1));
	}

	int mapFloatToOpacIndex(float point) 
    {
      int indx = mapPosition(point, 
                             getMinOpacMapValue(), 
                             getMaxOpacMapValue(), numEntries-1);
      if (indx < 0) indx = 0;
      if (indx > numEntries-1) indx = numEntries-1;
      return indx;
	}

	float mapOpacIndexToFloat(int indx)
    {
      return (float)(getMinOpacMapValue() + 
                     ((float)indx)*(float)(getMaxOpacMapValue()-
                                           getMinOpacMapValue())/
                     (float)(numEntries-1));
	}
	
    //
	// Methods to save and restore Mapper functions.
	// The gui opens the FILEs that are then read/written
	// Failure results in false/null pointer
	//
	// These methods are the same as the transfer function methods,
	// except for specifying separate color and opacity bounds,
	// and not having a name attribute
    //
	virtual XmlNode* buildNode(const string& tfname) ;

	//All the parsing can be done with the start handlers
	virtual bool elementStartHandler(ExpatParseMgr*, int depth, 
                                     std::string&, const char **);

	virtual bool elementEndHandler(ExpatParseMgr*, int, std::string&);
   
    std::string getName() { return mapperName; }

	// Mapper function tag is public, visible to flowparams
	static const string _mapperFunctionTag;

protected:
    //
	// Set to starting values
	//
	virtual void init();  
    	
		
protected:

    vector<OpacityMap*>  _opacityMaps;
    Colormap            *_colormap;

    //
    // XML tags
    //
	static const string _leftColorBoundAttr;
	static const string _rightColorBoundAttr;
	static const string _leftOpacityBoundAttr;
	static const string _rightOpacityBoundAttr;
	static const string _hsvAttr;
	static const string _positionAttr;
	static const string _opacityAttr;
	static const string _opacityControlPointTag;
	static const string _colorControlPointTag;
	// Additional attributes not yet supported:
	static const string _interpolatorAttr;
	static const string _rgbAttr;
	
    //
    // Mapping bounds
    //
	float minColorMapBound, maxColorMapBound;
	float minOpacMapBound, maxOpacMapBound;

    //
    // Parent params
    //
	RenderParams* myParams;
	
    //
	// Size of lookup table.  Always 1<<8 currently!
	//
	int numEntries;

    //
	// Mapper function name, if it's named.
    //
	string mapperName;

	//
    // Corresponding var nums
    //
	int colorVarNum;
    int opacVarNum;	

    //
    // Opacity scale factor
    //
    float opacityScaleFactor;
};
};
#endif //MAPPERFUNCTION_H
