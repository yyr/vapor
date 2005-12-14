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
#include "vapor/ExpatParseMgr.h"
#include <qcolor.h>

namespace VAPoR {
class MapEditor;
class Params;
class XmlNode;

class MapperFunction : public ParsedXml {
public:
	MapperFunction(Params* p, int nBits);
	MapperFunction();
	~MapperFunction();
	//Set to starting values
	//
	virtual void init();  
	//Insert a control point without disturbing values;
	//return new index
	//Note:  All public methods use actual real coords.
	//(Protected methods use normalized points in [0,1]
	//
	int insertColorControlPoint(float point);

	//Insert a control point possibly disturbing opacity
	//
	int insertOpacControlPoint(float point, float opacity = -1.f);
	
	void deleteColorControlPoint(int index);
	void deleteOpacControlPoint(int index);
	//Move a control point with specified index to new position.  
	//If newOpacity < 0, don't change opacity at new point. Returns
	//new index of point
	//
	int moveColorControlPoint(int index, float newPoint);
	int moveOpacControlPoint(int index, float newPoint, float newOpacity = -1.f);
	
	//Evaluate the opacity:
	//
	float opacityValue(float point);
	float opacityValue(int controlPointNum) {
		return opac[controlPointNum];
	}
		
	void setEditor(MapEditor* e) {myMapEditor = e;}
	MapEditor* getEditor() {return myMapEditor;}
	void setParams(Params* p) {myParams = p;}
	Params* getParams() {return myParams;}
	int getNumOpacControlPoints() {return numOpacControlPoints;}
	int getNumColorControlPoints() {return numColorControlPoints;}
	float getMinColorMapValue(){
		return minColorMapBound;
	}
	float getMaxColorMapValue(){
		return maxColorMapBound;
	}
	float getMinOpacMapValue(){
		return minOpacMapBound;
	}
	float getMaxOpacMapValue(){
		return maxOpacMapBound;
	}
	void setMinColorMapValue(float val) {minColorMapBound = val;}
	void setMaxColorMapValue(float val) {maxColorMapBound = val;}
	void setMinOpacMapValue(float val) {minOpacMapBound = val;}
	void setMaxOpacMapValue(float val) {maxOpacMapBound = val;}
	
	//Map a point to the specified range, and quantize it.
	//
	static int mapPosition(float x,  float minValue, float maxValue, int hSize);
	//Map and quantize a real value to the corresponding table index
	//i.e., quantize to current Mapper function domain
	//
	int mapFloatToColorIndex(float point) {
		
		int indx = mapPosition(point, getMinColorMapValue(), getMaxColorMapValue(), numEntries-1);
		if (indx < 0) indx = 0;
		if (indx > numEntries-1) indx = numEntries-1;
		return indx;
		//return (int)(0.5f+(numEntries*(point - minMapValue)/(maxMapValue-minMapValue)));
	}
	float mapColorIndexToFloat(int indx){
		return (float)(getMinColorMapValue() + ((float)indx)*(float)(getMaxColorMapValue()-getMinColorMapValue())/(float)(numEntries-1));
	}
	int mapFloatToOpacIndex(float point) {
		int indx =  mapPosition(point, getMinOpacMapValue(), getMaxOpacMapValue(), numEntries-1);
		if (indx < 0) indx = 0;
		if (indx > numEntries-1) indx = numEntries-1;
		return indx;
		//return (int)(0.5f+(numEntries*(point - minMapValue)/(maxMapValue-minMapValue)));
	}
	float mapOpacIndexToFloat(int indx){
		return (float)(getMinOpacMapValue() + ((float)indx)*(float)(getMaxOpacMapValue()-getMinOpacMapValue())/(float)(numEntries-1));
	}
	
	
	float opacCtrlPointPosition(int index){
		return (getMinOpacMapValue() + opacCtrlPoint[index]*(getMaxOpacMapValue()-getMinOpacMapValue()));}
	float colorCtrlPointPosition(int index){
		return (getMinColorMapValue() + colorCtrlPoint[index]*(getMaxColorMapValue()-getMinColorMapValue()));}
	float controlPointOpacity(int index) { return opac[index];}
	void hsvValue(float point, float* h, float* sat, float* val);
	
	QRgb getRgbValue(float point);
	
	void mapPointToRGBA(float point, float* rgba);
	void controlPointHSV(int index, float* h, float*s, float*v){
		*h = hue[index];
		*s = sat[index];
		*v = val[index];
	}
	void setControlPointHSV(int index, float h, float s, float v){
		hue[index] = h;
		sat[index] = s;
		val[index] = v;
		myParams->setClutDirty();
	}
	int getLeftOpacityIndex(float val){
		float normVal = (val-getMinOpacMapValue())/(getMaxOpacMapValue()-getMinOpacMapValue());
		return getLeftIndex(normVal, opacCtrlPoint, numOpacControlPoints);
	}
	int getLeftColorIndex(float val){
		float normVal = (val-getMinColorMapValue())/(getMaxColorMapValue()-getMinColorMapValue());
		return getLeftIndex(normVal, colorCtrlPoint, numColorControlPoints);
	}
	
	
		
	virtual QRgb getControlPointRGB(int index);
	void setControlPointRGB(int index, QRgb newColor);
	void setOpaque();

	//Build a lookup table[numEntries][4]? from the TF
	//Caller must pass in an empty array to fill in
	//
	void makeLut(float* clut);
	
	int getNumEntries() {return numEntries;}
	void setNumEntries(int val){ numEntries = val;}
	void startChange(char* s){ myParams->guiStartChangeMapFcn(s);}
	void endChange(){ myParams->guiEndChangeMapFcn();}
	//color conversion 
	static void hsvToRgb(float* hsv, float* rgb);
	static void rgbToHsv(float* rgb, float* hsv);
	
		
	//Methods to save and restore Mapper functions.
	//The gui opens the FILEs that are then read/written
	//Failure results in false/null pointer
	//
	//These methods are the same as the transfer function methods,
	//except for specifying separate color and opacity bounds,
	//and not having a name attribute
	virtual XmlNode* buildNode(const string& tfname) ;
	//All the parsing can be done with the start handlers
	virtual bool elementStartHandler(ExpatParseMgr*, int depth , std::string& , const char **);
	virtual bool elementEndHandler(ExpatParseMgr*, int , std::string&) ;
	
	
	string& getName() {return mapperName;}
	//Mapper function tag is public, visible to flowparams
	static const string _mapperFunctionTag;
protected:
	
	static const string _leftColorBoundAttr;
	static const string _rightColorBoundAttr;
	static const string _leftOpacityBoundAttr;
	static const string _rightOpacityBoundAttr;
	static const string _hsvAttr;
	static const string _positionAttr;
	static const string _opacityAttr;
	static const string _opacityControlPointTag;
	static const string _colorControlPointTag;
	//Additional attributes not yet supported:
	static const string _interpolatorAttr;
	static const string _rgbAttr;
	
	//Insert a control point without disturbing values;
	//return new index
	//Note:  All public methods use actual real coords.
	//These Protected methods use normalized points in [0,1]
	//
	int insertNormColorControlPoint(float point, float h, float s, float v);

	//Insert a control point with specified opacity
	//
	int insertNormOpacControlPoint(float point, float opacity);
	
	
	//find the index of the largest control point to the left of val
	//Input value is normalized (note this is protected)
	//
	int getLeftIndex(float val, std::vector<float>& ctrlPointArray, int arraySize);
	int numOpacControlPoints;
	int numColorControlPoints;
	
	float minColorMapBound, maxColorMapBound;
	float minOpacMapBound, maxOpacMapBound;
	//When a TF is being edited, it keeps a pointer to the editor
	//
	MapEditor* myMapEditor;
	Params* myParams;
	
	//Size of lookup table.  Always 1<<8 currently!
	//
	int numEntries;
	//Mapper function name, if it's named.
	string mapperName;
	//Data defining the functions:
	std::vector<float> opacCtrlPoint;
	std::vector<float> colorCtrlPoint;
	std::vector<float> hue;
	std::vector<float> sat;
	std::vector<float> val;
	std::vector<float> opac;
	std::vector<TFInterpolator::type> opacInterp;
	std::vector<TFInterpolator::type> colorInterp;
	
};
};
#endif //MAPPERFUNCTION_H
