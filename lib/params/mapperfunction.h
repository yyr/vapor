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
#include <vapor/tfinterpolator.h>
#include <vapor/ExpatParseMgr.h>
#include <vapor/MapperFunctionBase.h>
#include <vapor/ParamNode.h>
#include "OpacityMap.h"
#include "Colormap.h"

namespace VAPoR {
class Params;
class RenderParams;
class XmlNode;
class ParamNode;

class PARAMS_API MapperFunction : public MapperFunctionBase 
{

public:
	MapperFunction();
	MapperFunction(const string& tag);
	MapperFunction(RenderParams* p, int nBits = 8);
	MapperFunction(const MapperFunction &mapper);
	MapperFunction(const MapperFunctionBase &mapper);
	static ParamsBase* CreateDefaultInstance(){return new MapperFunction();}

	virtual ~MapperFunction();
	virtual MapperFunction* deepCopy(ParamNode* newRoot = 0){
		MapperFunction* mf = new MapperFunction(*this);
		mf->SetRootParamNode(newRoot);
		if(newRoot) newRoot->SetParamsBase(mf);
		return mf;
	}

	
	


    //
    // Function values
    //
    ARGB  colorValue(float point);

    //
    // Opacity Maps
    //
    virtual OpacityMap* createOpacityMap(OpacityMap::Type type=OpacityMap::CONTROL_POINT);

	virtual OpacityMap* getOpacityMap(int index);

	virtual VColormap*   getColormap();



protected:

    
	
};
class PARAMS_API IsoControl : public MapperFunction{
public:
	IsoControl();
	IsoControl(RenderParams* p, int nBits = 8);
	//Copy constructor
	IsoControl(const IsoControl &mapper);
	virtual IsoControl* deepCopy(ParamNode* newRoot = 0){
		IsoControl* mf = new IsoControl(*this);
		mf->SetRootParamNode(newRoot);
		if(newRoot) newRoot->SetParamsBase(mf);
		return mf;
	}
	static ParamsBase* CreateDefaultInstance(){return new IsoControl();}
	virtual ~IsoControl();
	void setIsoValue(double val){isoValues[0] = val;}
	double getIsoValue(){return isoValues[0];}
	void setIsoValues(const vector<double>& vals){isoValues = vals;}
	const vector<double>& getIsoValues(){return isoValues;}
	
	void setMinHistoValue(float val){
		setMinOpacMapValue(val);
		setMinColorMapValue(val);
	}
	void setMaxHistoValue(float val){
		setMaxOpacMapValue(val);
		setMaxColorMapValue(val);
	}
	float getMinHistoValue() {return getMinOpacMapValue();}
	float getMaxHistoValue() {return getMaxOpacMapValue();}
	

protected:
	static const string _leftHistoBoundAttr;
	static const string _rightHistoBoundAttr;
	static const string _leftHistoBoundTag;
	static const string _rightHistoBoundTag;
	vector<double> isoValues;
};
};
#endif //MAPPERFUNCTION_H
