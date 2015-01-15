//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		MapperFunctionBase.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Defines the MapperFunctionBase class  
//		This is the mathematical definition of a function
//		that can be used to map data to either colors or opacities
//		Subclasses can either implement color or transparency 
//		mapping (or both)

#ifndef MAPPERFUNCTIONBASE_H
#define MAPPERFUNCTIONBASE_H
#define MAXCONTROLPOINTS 50
#include <iostream>
#include <vapor/ExpatParseMgr.h>
#include <vapor/OpacityMapBase.h>
#include <vapor/ColorMapBase.h>
#include <vapor/tfinterpolator.h>
#include <vapor/ParamsBase.h>
#include "renderparams.h"

namespace VAPoR {
class XmlNode;
class ParamNode;
class RenderParams;
class PARAMS_API MapperFunctionBase : public ParamsBase 
{

public:
	MapperFunctionBase(const string& name);
	MapperFunctionBase(int nBits, const string& name);
	
	virtual ~MapperFunctionBase();

    //
    // Function values
    //
	float opacityValue(float point);
    void  hsvValue(float point, float* h, float* sat, float* val);

	void setOpaque();
	bool isOpaque();

    //
	// Build a lookup table[numEntries][4]
	// (Caller must pass in an empty array)
	//
	void makeLut(float* clut);

    //
    // Data Bounds
    //
	float getMinMapValue() { return GetValueDouble(_leftDataBoundTag); }
	float getMaxMapValue() { return GetValueDouble(_rightDataBoundTag); }
	
	void setMinMapValue(float val) { SetValueDouble(_leftDataBoundTag, "Set min map value", val, _params);}
	void setMaxMapValue(float val)  { SetValueDouble(_rightDataBoundTag, "Set max map value", val, _params);}
	
    virtual void setVarNum(int var) {
		SetValueLong(_varNumTag, "Set variable num", var, _params);
	}
	int getVarNum(){
		return GetValueLong(_varNumTag);
	}

    //
    // Opacity Maps
    //
    virtual OpacityMapBase* createOpacityMap(
		OpacityMapBase::Type type=OpacityMapBase::CONTROL_POINT
	);
    virtual OpacityMapBase* GetOpacityMap(int index) const;
    void        deleteOpacityMap(OpacityMapBase *omap);
	int         getNumOpacityMaps() const {return opacityPaths.size();}

    //
    // Opacity scale factor (scales all opacity maps)
    //
	void setOpacityScale(double val) { 
		SetValueDouble(_opacityScaleTag, "Set Opacity Scale", val, _params);
		
	}
	double getOpacityScale()   {return GetValueDouble(_opacityScaleTag);}

    //
    // Opacity composition
    //
    enum CompositionType
    {
      ADDITION = 0,
      MULTIPLICATION = 1
    };

    void setOpacityComposition(CompositionType t) { 
		SetValueLong(_opacityCompositionTag, "Set Opacity Composition Type", (long) t, _params);
	}
    CompositionType getOpacityComposition() {
		return (CompositionType) GetValueLong(_opacityCompositionTag); 
	}
    //
	// Color conversion 
    //
	static void hsvToRgb(float* hsv, float* rgb);
	static void rgbToHsv(float* rgb, float* hsv);
	
	//
    // Map a point to the specified range, and quantize it.
	//
	static int mapPosition(float x, float minValue, float maxValue, int hSize);

	int getNumEntries() const { return numEntries; }

    //
	// Map and quantize a real value to the corresponding table index
	// i.e., quantize to current Mapper function domain
	//
	int mapFloatToIndex(float point) 
    {
      int indx = mapPosition(point, 
                             getMinMapValue(), 
                             getMaxMapValue(), numEntries-1);
      if (indx < 0) indx = 0;
      if (indx > numEntries-1) indx = numEntries-1;
      return indx;
	}

	float mapIndexToFloat(int indx)
    {
      return (float)(getMinMapValue() + 
                     ((float)indx)*(float)(getMaxMapValue()-
                                           getMinMapValue())/
                     (float)(numEntries-1));
	}

	
    //Obtain name (used when TF is saved in session..
    std::string GetName() { 
		return GetValueString(_mapperNameTag);
	}
	void SetName(const string str){
		SetValueString(_mapperNameTag, "Set mapper function name", str, _params);
	}

	// Mapper function tag is public, visible to flowparams
	static const string _mapperFunctionTag;
	TFInterpolator::type getColorInterpType() {
		ColorMapBase* cmap = GetColorMap();
		if (cmap) return cmap->GetInterpType();
		return TFInterpolator::linear;
	}
	void setColorInterpType(TFInterpolator::type t){
		ColorMapBase* cmap = GetColorMap();
		if (cmap) cmap->SetInterpType(t);
	}
	void setParams(RenderParams* p) { _params = p; }
	RenderParams* getParams()  const { return _params; } 
	//! Methods to set/get the Color Map 
	virtual ColorMapBase* GetColorMap() const {
		vector<string>path;
		path.push_back(ColorMapBase::xmlTag());
		return (ColorMapBase*) GetParamsBase(path);
	}
	virtual void SetColorMap(ColorMapBase* cmap) {
		vector<string>path;
		path.push_back(ColorMapBase::xmlTag());
		SetParamsBase(path, cmap);
	}

	

protected:

	
    //
	// Set to starting values
	//
	virtual void init();  

	//Map child index to the path to opacityMapBase
	std::vector<vector<string> > opacityPaths;
	//Provide name for opac map node, unique for this instance;
	string getOpacMapTag();
    
    //
    // XML tags
    //
	
	static const string _leftDataBoundTag;
	static const string _rightDataBoundTag;
    static const string _opacityCompositionTag;
	static const string _mapperNameTag;
	static const string _varNumTag;
	static const string _opacityScaleTag;
	static const string _opacityMapsTag;

    //
    // Parent params
    //

	RenderParams* _params;
	
    //
	// Size of lookup table.  Always 1<<8 currently!
	//
	const int numEntries;
	int opacityMapNum;

   
};
};
#endif //MAPPERFUNCTIONBASE_H
