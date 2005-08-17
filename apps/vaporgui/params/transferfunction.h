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
	
	//Note:  All public methods use actual real coords.
	//(Protected methods use normalized points in [0,1]
	//
	
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

	int mapFloatToIndex(float f) { return mapFloatToColorIndex(f);}
	float mapIndexToFloat(int indx) {return mapColorIndexToFloat(indx);}
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


};
};
#endif //TRANSFERFUNCTION_H
