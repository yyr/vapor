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
#define MAXCONTROLPOINTS 100
#include "tfinterpolator.h"
#include "dvrparams.h"
#include "vapor/ExpatParseMgr.h"
#include <qcolor.h>

namespace VAPoR {
class TFEditor;
class DvrParams;
class XmlNode;

class TransferFunction : public ParsedXml {
public:
	TransferFunction(DvrParams* p, int nBits);
	TransferFunction();
	~TransferFunction();
	//Set to starting values
	//
	void init();  
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
	void setEditor(TFEditor* e) {myTFEditor = e;}
	void setParams(DvrParams* p) {myParams = p;}
	int getNumOpacControlPoints() {return numOpacControlPoints;}
	int getNumColorControlPoints() {return numColorControlPoints;}
	float getMinMapValue(){
		return minMapBound;
	}
	float getMaxMapValue(){
		return maxMapBound;
	}
	void setMinMapValue(float val) {minMapBound = val;}
	void setMaxMapValue(float val) {maxMapBound = val;}
	
	//Map a point to the specified range, and quantize it.
	//
	static int mapPosition(float x,  float minValue, float maxValue, int hSize);
	//Map and quantize a real value to the corresponding table index
	//i.e., quantize to current transfer function domain
	//
	int mapFloatToIndex(float point) {
		return mapPosition(point, getMinMapValue(), getMaxMapValue(), numEntries-1);
		//return (int)(0.5f+(numEntries*(point - minMapValue)/(maxMapValue-minMapValue)));
	}
	float mapIndexToFloat(int indx){
		return (float)(getMinMapValue() + ((float)indx)*(float)(getMaxMapValue()-getMinMapValue())/(float)(numEntries-1));
	}
	float opacCtrlPointPosition(int index){
		return (getMinMapValue() + opacCtrlPoint[index]*(getMaxMapValue()-getMinMapValue()));}
	float colorCtrlPointPosition(int index){
		return (getMinMapValue() + colorCtrlPoint[index]*(getMaxMapValue()-getMinMapValue()));}
	float controlPointOpacity(int index) { return opac[index];}
	void hsvValue(float point, float* h, float*sat, float*val);
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
	QRgb getControlPointRGB(int index);
	void setControlPointRGB(int index, QRgb newColor);

	int getNumEntries() {return numEntries;}
	void setNumEntries(int val){ numEntries = val;}
	void startChange(char* s){ myParams->guiStartChangeTF(s);}
	void endChange(){ myParams->guiEndChangeTF();}
	//color conversion 
	static void hsvToRgb(float* hsv, float* rgb);
	static void rgbToHsv(float* rgb, float* hsv);
	//find left index for opacity 
	//
	int getLeftOpacityIndex(float val){
		float normVal = (val-getMinMapValue())/(getMaxMapValue()-getMinMapValue());
		return getLeftIndex(normVal, opacCtrlPoint, numOpacControlPoints);
	}
	int getLeftColorIndex(float val){
		float normVal = (val-getMinMapValue())/(getMaxMapValue()-getMinMapValue());
		return getLeftIndex(normVal, colorCtrlPoint, numColorControlPoints);
	}
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
	string& getName() {return tfName;}
	
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

	//find the index of the largest control point to the left of val
	//Input value is normalized (note this is protected)
	//
	int getLeftIndex(float val, float* ctrlPointArray, int arraySize);
	int numOpacControlPoints;
	int numColorControlPoints;
	float opacCtrlPoint[MAXCONTROLPOINTS];
	float colorCtrlPoint[MAXCONTROLPOINTS];
	float hue[MAXCONTROLPOINTS];
	float sat[MAXCONTROLPOINTS];
	float val[MAXCONTROLPOINTS];
	float opac[MAXCONTROLPOINTS];
	TFInterpolator::type opacInterp[MAXCONTROLPOINTS];
	TFInterpolator::type colorInterp[MAXCONTROLPOINTS];
	float minMapBound, maxMapBound;
	//When a TF is being edited, it keeps a pointer to the editor
	//
	TFEditor* myTFEditor;
	DvrParams* myParams;
	
	//Size of lookup table.  Always 1<<8 currently!
	//
	int numEntries;
	//transfer function name, if it's named.
	string tfName;

	
};
};
#endif //TRANSFERFUNCTION_H
