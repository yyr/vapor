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

// distance for being "close" to a pixel
#define CLOSE_DISTANCE 4
//Layout parameters for bar
#define COLORBARWIDTH 15
//Separator is between color bar and opacity graph
#define SEPARATOR 4
//CoordMargin is at bottom 
#define COORDMARGIN 10
//Topmargin is between curve and top of editing window
#define TOPMARGIN 3
//DomainSlider is above image and possibly above color bar
#define SLIDERWIDTH 12

//Factor for getting wider than specified window:
#define HORIZOVERLAP 0.02f
//Float constants to describe vertical regions:
#define ABOVEWINDOW 2.f
#define ONOPACITYSLIDER 1.f
#define ONSEPARATOR -1.f
#define ONCOLORSLIDER -2.f
#define ONCOLORBAR -3.f
#define BELOWCOLORBAR -4.f
#define BELOWWINDOW -5.f

//COLORS:
#define SEPARATORCOLOR qRgb(64,64,64)
#define DEFAULTBACKGROUNDCOLOR qRgb(233,236,216)
#define HISTOCOLOR qRgb(0,200,200)
#define HIGHLIGHTCOLOR yellow
#define OPACITYCURVECOLOR white
#define CONTROLPOINTCOLOR darkGray
#define ENDLINECOLOR lightGray
#define DOMAINSLIDERCOLOR qRgb(128,0,0) //Dark red
#define DOMAINENDSLIDERCOLOR qRgb(255,0,0) //Full red

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
	
	//set to default state:
	virtual void reset()=0;
	
	//map window to variable, can go to any value
	float mapColorWin2Var(int x);
	float mapOpacWin2Var(int x);
	//Coordinate mapping functions:
	//map TF variable to window coords.  
	// if classify, Left goes to -1, right goes to width.
	int mapColorVar2Win(float x, bool classify = false);
	int mapOpacVar2Win(float x, bool classify = false);
	
	//map window to discrete mapping value (0..2**nbits - 1)
	//if truncate, limits are mapped to ending discrete value
	int  mapColorWin2Discrete(int x, bool truncate = false);
	int  mapOpacWin2Discrete(int x, bool truncate = false);
	//map variable to discrete, optionally limits to end values
	int mapColorVar2Discrete(float x);
	int mapOpacVar2Discrete(float x);
	
	//Map verticalWin to opacity, optionally  special constants
	virtual float mapWin2Opac(int y, bool classify = false) = 0;
	//map opacity to window position, optionally truncate to valid
	virtual int mapOpac2Win(float op, bool truncate = false) = 0;

	void setColorVarNum(int varnum) {colorVarNum = varnum;}
	int getColorVarNum() {return colorVarNum;}
	void setOpacVarNum(int varnum) {opacVarNum = varnum;}
	int getOpacVarNum() {return opacVarNum;}
	
	virtual void setDirty() = 0;
	
	int getNumColorControlPoints() {return myMapperFunction->getNumColorControlPoints();}
	int getNumOpacControlPoints() {return myMapperFunction->getNumOpacControlPoints();}
	QRgb getControlPointColor(int index){
		return myMapperFunction->getControlPointRGB(index);
	}
	
	
	
	float getMinColorEditValue(){
		return getParams()->getMinColorEditBound(colorVarNum);
	}
	float getMinOpacEditValue(){
		return getParams()->getMinOpacEditBound(opacVarNum);
	}
	float getMaxColorEditValue(){
		return getParams()->getMaxColorEditBound(colorVarNum);
	}
	float getMaxOpacEditValue(){
		return getParams()->getMaxOpacEditBound(opacVarNum);
	}
	void setMinColorEditValue(float val) {
		getParams()->setMinColorEditBound(val, colorVarNum);
	}
	void setMinOpacEditValue(float val) {
		getParams()->setMinOpacEditBound(val, opacVarNum);
	}
	void setMaxColorEditValue(float val) {
		getParams()->setMaxColorEditBound(val, colorVarNum);
	}
	void setMaxOpacEditValue(float val) {
		getParams()->setMaxOpacEditBound(val, opacVarNum);
	}
	MapperFunction* getMapperFunction() {return myMapperFunction;}
	void setMapperFunction(MapperFunction* tf) {myMapperFunction = tf;}
	void setFrame(QFrame* frm) {myFrame = frm;}
	

	virtual int getHistoValue(float /*point*/){return 0;}
	Params* getParams() {return myMapperFunction->getParams();}

	
protected:
	
	int height;
	int width;
	MapperFunction* myMapperFunction;
	
	QFrame* myFrame;
	int colorVarNum, opacVarNum;
	
};
};


#endif
