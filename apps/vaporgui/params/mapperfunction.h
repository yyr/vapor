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
class TFEditor;
class Params;
class XmlNode;

class MapperFunction : public ParsedXml {
public:
	MapperFunction(Params* p, int nBits);
	MapperFunction();
	~MapperFunction();
	//Set to starting values
	//
	virtual void init() = 0;  
	//Insert a control point without disturbing values;
	//return new index
	//Note:  All public methods use actual real coords.
	//(Protected methods use normalized points in [0,1]
	//
	virtual int insertColorControlPoint(float point)= 0;

	//Insert a control point possibly disturbing opacity
	//
	virtual int insertOpacControlPoint(float point, float opacity = -1.f) = 0;
	
	virtual void deleteColorControlPoint(int index) = 0;
	virtual void deleteOpacControlPoint(int index) = 0;
	//Move a control point with specified index to new position.  
	//If newOpacity < 0, don't change opacity at new point. Returns
	//new index of point
	//
	virtual int moveColorControlPoint(int index, float newPoint) = 0;
	virtual int moveOpacControlPoint(int index, float newPoint, float newOpacity = -1.f) = 0;
	
	//Evaluate the opacity:
	//
	virtual float opacityValue(float point) = 0;
	virtual float opacityValue(int controlPointNum) = 0;
		
	void setEditor(TFEditor* e) {myMapEditor = e;}
	TFEditor* getEditor() {return myMapEditor;}
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
		return mapPosition(point, getMinColorMapValue(), getMaxColorMapValue(), numEntries-1);
		//return (int)(0.5f+(numEntries*(point - minMapValue)/(maxMapValue-minMapValue)));
	}
	float mapColorIndexToFloat(int indx){
		return (float)(getMinColorMapValue() + ((float)indx)*(float)(getMaxColorMapValue()-getMinColorMapValue())/(float)(numEntries-1));
	}
	int mapFloatToOpacIndex(float point) {
		return mapPosition(point, getMinOpacMapValue(), getMaxOpacMapValue(), numEntries-1);
		//return (int)(0.5f+(numEntries*(point - minMapValue)/(maxMapValue-minMapValue)));
	}
	float mapOpacIndexToFloat(int indx){
		return (float)(getMinOpacMapValue() + ((float)indx)*(float)(getMaxOpacMapValue()-getMinOpacMapValue())/(float)(numEntries-1));
	}
	virtual float opacCtrlPointPosition(int index) = 0;
	virtual float colorCtrlPointPosition(int index) = 0;
	virtual float controlPointOpacity(int index) = 0;
	virtual void hsvValue(float point, float* h, float*sat, float*val) = 0;
	QRgb getRgbValue(float point);
	virtual void controlPointHSV(int index, float* h, float*s, float*v) = 0;
		
	virtual void setControlPointHSV(int index, float h, float s, float v) = 0;
		
	virtual QRgb getControlPointRGB(int index) = 0;
	void setControlPointRGB(int index, QRgb newColor);

	int getNumEntries() {return numEntries;}
	void setNumEntries(int val){ numEntries = val;}
	void startChange(char* s){ myParams->guiStartChangeMapFcn(s);}
	void endChange(){ myParams->guiEndChangeMapFcn();}
	//color conversion 
	static void hsvToRgb(float* hsv, float* rgb);
	static void rgbToHsv(float* rgb, float* hsv);
	//find left index for opacity 
	//
	virtual int getLeftOpacityIndex(float val) = 0;
		
	virtual int getLeftColorIndex(float val)= 0;
		
	//Methods to save and restore Mapper functions.
	//The gui opens the FILEs that are then read/written
	//Failure results in false/null pointer
	//
	
	virtual XmlNode* buildNode(const string& tfname) = 0;
	//All the parsing can be done with the start handlers
	virtual bool elementStartHandler(ExpatParseMgr*, int depth , std::string& s, const char **attr) = 0;
	virtual bool elementEndHandler(ExpatParseMgr*, int , std::string&) = 0;
	//Mapper function tag is visible to session 
	static const string _MapperFunctionTag;
	string& getName() {return mapperName;}
	
protected:

	//Insert a control point without disturbing values;
	//return new index
	//Note:  All public methods use actual real coords.
	//These Protected methods use normalized points in [0,1]
	//
	virtual int insertNormColorControlPoint(float point, float h, float s, float v) = 0;

	//Insert a control point with specified opacity
	//
	virtual int insertNormOpacControlPoint(float point, float opacity) = 0;
	//Tags and attributes for reading and writing
	

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
	TFEditor* myMapEditor;
	Params* myParams;
	
	//Size of lookup table.  Always 1<<8 currently!
	//
	int numEntries;
	//Mapper function name, if it's named.
	string mapperName;

	
};
};
#endif //MAPPERFUNCTION_H
