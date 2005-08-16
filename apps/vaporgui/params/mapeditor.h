//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		mapeditor.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Defines the MapEditor class.  This is the abstract class
//		that manages a mapper function editing panel
//
#ifndef MAPEDITOR_H
#define MAPEDITOR_H


class QImage;
#include "params.h"
#include "mapperfunction.h"
class QFrame;
namespace VAPoR {
class MapperFunction;
class TFInterpolator;
class Session;

class Histo;
class TFELocationTip;
class Params;

class MapEditor{
public:
	MapEditor(MapperFunction* TF, QFrame*);
	~MapEditor();
	
	void setColorEditingRange(float minVal, float maxVal){
		getParams()->setMinColorEditBound(minVal, varNum);
		getParams()->setMaxColorEditBound(maxVal, varNum);
		setDirty();
	}
	void setOpacEditingRange(float minVal, float maxVal){
		getParams()->setMinOpacEditBound(minVal,varNum);
		getParams()->setMaxOpacEditBound(maxVal,varNum);
		setDirty();
	}
	void setVarNum(int varnum) {varNum = varnum;}
	int getVarNum() {return varNum;}
	
	virtual void setDirty() = 0;
	
	int getNumColorControlPoints() {return myMapperFunction->getNumColorControlPoints();}
	int getNumOpacControlPoints() {return myMapperFunction->getNumOpacControlPoints();}
	QRgb getControlPointColor(int index){
		return myMapperFunction->getControlPointRGB(index);
	}
	
	
	
	float getMinColorEditValue(){
		return getParams()->getMinColorEditBound(varNum);
	}
	float getMinOpacEditValue(){
		return getParams()->getMinOpacEditBound(varNum);
	}
	float getMaxColorEditValue(){
		return getParams()->getMaxColorEditBound(varNum);
	}
	float getMaxOpacEditValue(){
		return getParams()->getMaxOpacEditBound(varNum);
	}
	void setMinColorEditValue(float val) {
		getParams()->setMinColorEditBound(val, varNum);
	}
	void setMinOpacEditValue(float val) {
		getParams()->setMinOpacEditBound(val, varNum);
	}
	void setMaxColorEditValue(float val) {
		getParams()->setMaxColorEditBound(val, varNum);
	}
	void setMaxOpacEditValue(float val) {
		getParams()->setMaxOpacEditBound(val, varNum);
	}
	MapperFunction* getMapperFunction() {return myMapperFunction;}
	void setMapperFunction(MapperFunction* tf) {myMapperFunction = tf;}
	void setFrame(QFrame* frm) {myFrame = frm;}
	

	
	Params* getParams() {return myMapperFunction->getParams();}

	
protected:
	
	
	MapperFunction* myMapperFunction;
	
	QFrame* myFrame;
	int varNum;
	
};
};


#endif
