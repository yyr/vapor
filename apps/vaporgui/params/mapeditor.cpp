//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		mapeditor.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2005
//
//	Description:	Implements the MapEditor class.  This is the abstract class
//		that manages a mapper function editing session
//
#include "mapeditor.h"
#include "mapperfunction.h"
#include <qframe.h>
#include <qimage.h>
#include "session.h"
#include "params.h"
#include "vizwinmgr.h"
using namespace VAPoR;

MapEditor::MapEditor(MapperFunction* tf,  QFrame* frm){
	myMapperFunction = tf;
	myFrame = frm;
	tf->setEditor(this);
	colorVarNum = -1;
	opacVarNum = -1;
}
MapEditor::~MapEditor(){
	//Don't delete the MapperFunction, that will cause a loop.
}
/*
 * Coordinate mapping functions:
 * Map TF variable to window coords.  Left goes to -1, right goes to width.
 */
int MapEditor::
mapColorVar2Win(float v, bool classify){
	double va = v;
	//if (v < -1.e7f) v = -1.e7f;
	//if (v > 1.e7f) v = 1.e7f;
	//First map to a float value in pixel space:
	
	double cvrt = ((HORIZOVERLAP + (va - getMinColorEditValue())/(getMaxColorEditValue()-getMinColorEditValue()))*
		(((double)width - 1.0)/(1.0 + 2.0*HORIZOVERLAP)));
	
	int pixVal;
	if(classify){
		if (cvrt< 0.0) return -1;
		if (cvrt > ((double)(width) -.5)) return width;
		pixVal = (int)(cvrt+0.5);
		return pixVal;
	}
	if (cvrt >= 0.0)
		pixVal = (int)(cvrt+0.5);
	else pixVal = (int)(cvrt - 0.5);
	return pixVal;
}
	 
/*
 * Coordinate mapping functions:
 * Map TF variable to window coords.  Left goes to -1, right goes to width.
 */
int MapEditor::
mapOpacVar2Win(float v, bool classify){
	double va = v;
	//if (v < -1.e7f) v = -1.e7f;
	//if (v > 1.e7f) v = 1.e7f;
	//First map to a float value in pixel space:
	
	double cvrt = ((HORIZOVERLAP + (va - getMinOpacEditValue())/(getMaxOpacEditValue()-getMinOpacEditValue()))*
		(((double)width - 1.0)/(1.0 + 2.0*HORIZOVERLAP)));
	
	int pixVal;
	if(classify){
		if (cvrt< 0.0) return -1;
		if (cvrt > ((double)(width) -.5)) return width;
		pixVal = (int)(cvrt+0.5);
		return pixVal;
	}
	if (cvrt >= 0.0)
		pixVal = (int)(cvrt+0.5);
	else pixVal = (int)(cvrt - 0.5);
	return pixVal;
}	
/*
 * map window to variable.  Inverse of above function.
 */
float MapEditor::
mapColorWin2Var(int x){
	float ratio = (float)x/(float)(width -1);	
	// Adjust coord based on slight overlap:
	// 0->minEditValue - (max - min)*HORIZOVERLAP
	// (wid-1)-> maxEditValue + (max-min)*HORIZOVERLAP
	//
	float var = getMinColorEditValue() + (getMaxColorEditValue() - getMinColorEditValue())*
			(ratio*(1.f + 2.f*HORIZOVERLAP) - HORIZOVERLAP);
	return var;
}
/*
 * map window to variable.  Inverse of above function.
 */
float MapEditor::
mapOpacWin2Var(int x){
	float ratio = (float)x/(float)(width -1);	
	// Adjust coord based on slight overlap:
	// 0->minEditValue - (max - min)*HORIZOVERLAP
	// (wid-1)-> maxEditValue + (max-min)*HORIZOVERLAP
	//
	float var = getMinOpacEditValue() + (getMaxOpacEditValue() - getMinOpacEditValue())*
			(ratio*(1.f + 2.f*HORIZOVERLAP) - HORIZOVERLAP);
	return var;
}
/*
 * Map window to discrete mapping value (0..2**nbits - 1)
 * if truncate, limits are mapped to ending discrete value
 */
int MapEditor::
mapColorWin2Discrete(int x, bool truncate){
	int val = mapColorVar2Discrete(mapColorWin2Var(x));
	if(truncate){
		if (val < 0) val = 0;
		if (val >= getMapperFunction()->getNumEntries()){
			val = getMapperFunction()->getNumEntries()-1;
		}
	}
	return val;
}
/*
 * Map window to discrete mapping value (0..2**nbits - 1)
 * if truncate, limits are mapped to ending discrete value
 */
int MapEditor::
mapOpacWin2Discrete(int x, bool truncate){
	int val = mapOpacVar2Discrete(mapOpacWin2Var(x));
	if(truncate){
		if (val < 0) val = 0;
		if (val >= getMapperFunction()->getNumEntries()){
			val = getMapperFunction()->getNumEntries()-1;
		}
	}
	return val;
}
/*
 * map variable to discrete.  
 */
int MapEditor::
mapColorVar2Discrete(float v){
	return getMapperFunction()->mapFloatToColorIndex(v);
}
/*
 * map variable to discrete.  
 */
int MapEditor::
mapOpacVar2Discrete(float v){
	return getMapperFunction()->mapFloatToOpacIndex(v);
}


