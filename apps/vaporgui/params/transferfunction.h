//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		transferfunction.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Defines the TransferFunction class  
//		This is the mathematical definition of the transfer function
//		It is defined in terms of floating point coordinates, converted
//		into a mapping of quantized values (LUT) with specified domain at
//		rendering time.  Interfaces to the TFEditor and DvrParams classes.
//		The TransferFunction deals with an mapping on the interval [0,1]
//		that is remapped to a specified interval by the user.
//
#ifndef TRANSFERFUNCTION_H
#define TRANSFERFUNCTION_H
#define MAXCONTROLPOINTS 50
#include "tfinterpolator.h"
#include "params.h"
#include "mapperfunction.h"
#include "vapor/ExpatParseMgr.h"
#include <qcolor.h>

namespace VAPoR {
class TFEditor;
class DvrParams;
class XmlNode;

class TransferFunction : public MapperFunction {
public:
	TransferFunction(Params* p, int nBits);
	TransferFunction();
	~TransferFunction();
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
	
	virtual void deleteColorControlPoint(int index);
	virtual void deleteOpacControlPoint(int index);
	//Move a control point with specified index to new position.  
	//If newOpacity < 0, don't change opacity at new point. Returns
	//new index of point
	//
	virtual int moveColorControlPoint(int index, float newPoint);
	virtual int moveOpacControlPoint(int index, float newPoint, float newOpacity = -1.f);
	//Build a lookup table[numEntries][4]? from the TF
	//Caller must pass in an empty array to fill in
	//
	void makeLut(float* clut);
	//Evaluate the opacity:
	//
	float opacityValue(float point);
	float opacityValue(int controlPointNum) {
		return opac[controlPointNum];
	}
	
	
	virtual float opacCtrlPointPosition(int index){
		return (getMinMapValue() + opacCtrlPoint[index]*(getMaxMapValue()-getMinMapValue()));}
	virtual float colorCtrlPointPosition(int index){
		return (getMinMapValue() + colorCtrlPoint[index]*(getMaxMapValue()-getMinMapValue()));}
	virtual float controlPointOpacity(int index) { return opac[index];}
	virtual void hsvValue(float point, float* h, float*sat, float*val);
	
	virtual void controlPointHSV(int index, float* h, float*s, float*v){
		*h = hue[index];
		*s = sat[index];
		*v = val[index];
	}
	virtual void setControlPointHSV(int index, float h, float s, float v){
		hue[index] = h;
		sat[index] = s;
		val[index] = v;
		myParams->setClutDirty();
	}
	virtual int getLeftOpacityIndex(float val){
		float normVal = (val-getMinMapValue())/(getMaxMapValue()-getMinMapValue());
		return getLeftIndex(normVal, opacCtrlPoint, numOpacControlPoints);
	}
	virtual int getLeftColorIndex(float val){
		float normVal = (val-getMinMapValue())/(getMaxMapValue()-getMinMapValue());
		return getLeftIndex(normVal, colorCtrlPoint, numColorControlPoints);
	}
	virtual QRgb getControlPointRGB(int index);
	
	int getNumEntries() {return numEntries;}
	void setNumEntries(int val){ numEntries = val;}
	void startChange(char* s){ myParams->guiStartChangeMapFcn(s);}
	void endChange(){ myParams->guiEndChangeMapFcn();}
	
	//Transfer function has identical min,max map bounds, but
	//Parent class has them potentially unequal.
	void setMinMapValue(float minVal){
		setMinOpacMapValue(minVal);
		setMinColorMapValue(minVal);
	}
	void setMaxMapValue(float val){
		setMaxOpacMapValue(val);
		setMaxColorMapValue(val);
	}
	float getMinMapValue() {return getMinColorMapValue();}
	float getMaxMapValue() {return getMaxColorMapValue();}

	int mapFloatToIndex(float f) { return mapFloatToOpacIndex(f);}
	float mapIndexToFloat(int indx) {return mapOpacIndexToFloat(indx);}
	//Methods to save and restore transfer functions.
	//The gui opens the FILEs that are then read/written
	//Failure results in false/null pointer
	//
	bool saveToFile(ofstream& f);
	static TransferFunction* loadFromFile(ifstream& is);
	XmlNode* buildNode(const string& tfname);
	//All the parsing can be done with the start handlers
	bool elementStartHandler(ExpatParseMgr*, int depth , std::string& s, const char **attr);
	bool elementEndHandler(ExpatParseMgr*, int , std::string&);
	//Transfer function tag is visible to session 
	static const string _transferFunctionTag;

	
protected:

	//Insert a control point without disturbing values;
	//return new index
	//Note:  All public methods use actual real coords.
	//These Protected methods use normalized points in [0,1]
	//
	int insertNormColorControlPoint(float point, float h, float s, float v);

	//Insert a control point with specified opacity
	//
	int insertNormOpacControlPoint(float point, float opacity);
	//Tags and attributes for reading and writing
	
	static const string _tfNameAttr;
	static const string _leftBoundAttr;
	static const string _rightBoundAttr;
	static const string _hsvAttr;
	static const string _positionAttr;
	static const string _opacityAttr;
	static const string _opacityControlPointTag;
	static const string _colorControlPointTag;
	//Additional attributes not yet supported:
	static const string _interpolatorAttr;
	static const string _rgbAttr;
	static const string _commentTag;
	static const string _numEntriesAttr;


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
#endif //TRANSFERFUNCTION_H
