//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		transferfunction.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TransferFunction class  
//		This is the mathematical definition of the transfer function
//		It is defined in terms of floating point coordinates, converted
//		into a mapping of quantized values (LUT) with specified domain at
//		rendering time.  Interfaces to the TFEditor and DvrParams classes.
//		The TransferFunction deals with an mapping on the interval [0,1]
//		that is remapped to a specified interval by the user.
//
#ifdef WIN32
#pragma warning(disable : 4251 4100)
#endif
#include "tfinterpolator.h"
#include "transferfunction.h"
#include "assert.h"
#include "vaporinternal/common.h"
#include "dvrparams.h"

#include "vapor/MyBase.h"
#include <vapor/XmlNode.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include <vapor/Metadata.h>
#include <vapor/CFuncs.h>
#include "vapor/ExpatParseMgr.h"
using namespace VAPoR;
using namespace VetsUtil;
// Static member initialization.  Acceptable tags in transfer function output
// Sooner or later we may want to support 
//
const string TransferFunction::_transferFunctionTag = "TransferFunction";
const string TransferFunction::_tfNameAttr = "Name";
const string TransferFunction::_leftBoundAttr = "LeftBound";
const string TransferFunction::_rightBoundAttr = "RightBound";

//Constructor for empty, default transfer function
TransferFunction::TransferFunction() : MapperFunction(){
	
}
void TransferFunction::
init(){  //reset to starting values:
	
	colorCtrlPoint.resize(2);
	opacCtrlPoint.resize(2);
	colorInterp.resize(2);
	opacInterp.resize(2);
	hue.resize(2);
	sat.resize(2);
	val.resize(2);
	opac.resize(2);
	numColorControlPoints = 2;
	colorCtrlPoint[0] = -1.e6f;
	colorCtrlPoint[1] = 1.e6f;
	
	numOpacControlPoints = 2;
	opacCtrlPoint[0] = -1.e6f;
	opacCtrlPoint[1] = 1.e6f;
	colorInterp[0] = TFInterpolator::linear;
	opacInterp[0] = TFInterpolator::linear;
	hue[0] = 0.f;
	sat[0] = 1.f;
	val[0] = 1.f;
	opac[0] = 0.f;
	colorInterp[1] = TFInterpolator::linear;
	opacInterp[1] = TFInterpolator::linear;
	hue[1] = 0.f;
	sat[1] = 1.f;
	val[1] = 1.f;
	opac[1] = 1.f;
	numEntries = 256;
	setMinMapValue(0.f);
	setMaxMapValue(1.f);
}
//Currently this is only for use in a dvrparams panel
//and a probe
//
TransferFunction::TransferFunction(Params* p, int nBits): MapperFunction(p,nBits){
	
	
	
}
	
TransferFunction::~TransferFunction() {

}


//Methods to save and restore transfer functions.
	//The gui specifies FILEs that are then read/written
	//Failure results in false/null pointer
	//
//Construct an XML node from the transfer function
XmlNode* TransferFunction::
buildNode(const string& tfname) {
	//Construct the main node
	string empty;
	std::map <string, string> attrs;
	attrs.empty();
	ostringstream oss;

	if (!tfname.empty()){
		attrs[_tfNameAttr] = tfname;
	}
	oss.str(empty);
	oss << (double)getMinMapValue();
	attrs[_leftBoundAttr] = oss.str();
	oss.str(empty);
	oss << (double)getMaxMapValue();
	attrs[_rightBoundAttr] = oss.str();

	XmlNode* mainNode = new XmlNode(_transferFunctionTag, attrs, numOpacControlPoints+numColorControlPoints);

	//Now add children:  One for each control point.
	//ignore first and last.
	
	
	map <string, string> emptyAttrs;  //empty attribs
	int i;
	
	for (i = 1; i< numOpacControlPoints-1; i++){
		map <string, string> cpAttrs;
		
		oss.str(empty);
		oss << (double)opacCtrlPoint[i];
		cpAttrs[_positionAttr] = oss.str();
		oss.str(empty);
		oss << (double)opac[i];
		cpAttrs[_opacityAttr] = oss.str();
		mainNode->NewChild(_opacityControlPointTag, cpAttrs, 0);
	}
	for (i = 1; i< numColorControlPoints-1; i++){
		map <string, string> cpAttrs;
		
		oss.str(empty);
		oss << (double)colorCtrlPoint[i]; //Put the value in the control point node
		cpAttrs[_positionAttr] = oss.str();
		oss.str(empty);
		oss << (double)hue[i] << " " << (double)sat[i] << " " << (double)val[i];
		cpAttrs[_hsvAttr] = oss.str();
		mainNode->NewChild(_colorControlPointTag, cpAttrs, 0);
	}
	return mainNode;
}


//Create a transfer function by parsing a file.
TransferFunction* TransferFunction::
loadFromFile(ifstream& is){
	TransferFunction* newTF = new TransferFunction();
	// Create an Expat XML parser to parse the XML formatted metadata file
	// specified by 'path'
	//
	ExpatParseMgr* parseMgr = new ExpatParseMgr(newTF);
	parseMgr->parse(is);
	delete parseMgr;
	return newTF;
}
bool TransferFunction::
saveToFile(ofstream& ofs){
	const std::string emptyString;
	XmlNode* rootNode = buildNode(emptyString);

	ofs << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	XmlNode::streamOut(ofs,(*rootNode));
	delete rootNode;
	return true;
}

//Handlers for Expat parsing.
//The parse state is determined by
//whether it's parsing a color or opacity.
//
bool TransferFunction::
elementStartHandler(ExpatParseMgr* pm, int /*depth*/ , std::string& tagString, const char **attrs){
	if (StrCmpNoCase(tagString, _transferFunctionTag) == 0) {
		//If it's a TransferFunction tag, save the left/right bounds attributes.
		//Ignore the name tag
		//Do this by repeatedly pulling off the attribute name and value
		//begin by  resetting everything to starting values.
		init();
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _tfNameAttr) == 0) {
				ist >> mapperName;
			}
			else if (StrCmpNoCase(attribName, _leftBoundAttr) == 0) {
				float floatval;
				ist >> floatval;
				setMinMapValue(floatval);
			}
			else if (StrCmpNoCase(attribName, _rightBoundAttr) == 0) {
				float floatval;
				ist >> floatval;
				setMaxMapValue(floatval);
			}
			
			else return false;
		}
		return true;
	}
	else if (StrCmpNoCase(tagString, _colorControlPointTag) == 0) {
		//Create a color control point with default values,
		//and with specified position
		//Currently only support HSV colors
		//Repeatedly pull off attribute name and values
		string attribName;
		float hue = 0.f, sat = 1.f, val=1.f, posn=0.f;
		while (*attrs){
			attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _positionAttr) == 0) {
				ist >> posn;
			} else if (StrCmpNoCase(attribName, _hsvAttr) == 0) { 
				ist >> hue;
				ist >> sat;
				ist >> val;
			} else return false;//Unknown attribute
		}
		//Then insert color control point
		int indx = insertNormColorControlPoint(posn,hue,sat,val);
		if (indx >= 0)return true;
		return false;
	}
	else if (StrCmpNoCase(tagString, _opacityControlPointTag) == 0) {
		//peel off position and opacity
		string attribName;
		float opacity = 1.f, posn = 0.f;
		while (*attrs){
			attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _positionAttr) == 0) {
				ist >> posn;
			} else if (StrCmpNoCase(attribName, _opacityAttr) == 0) { 
				ist >> opacity;
			} else return false; //Unknown attribute
		}
		//Then insert color control point
		if(insertNormOpacControlPoint(posn, opacity)>=0) return true;
		else return false;
	}
	else return false;
}
//The end handler needs to pop the parse stack, if this is not the top level.
bool TransferFunction::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	//Check only for the transferfunction tag, ignore others.
	if (StrCmpNoCase(tag, _transferFunctionTag) != 0) return true;
	//If depth is 0, this is a transfer function file; otherwise need to
	//pop the parse stack.  The caller will need to save the resulting
	//transfer function (i.e. this)
	if (depth == 0) return true;
	ParsedXml* px = pm->popClassStack();
	bool ok = px->elementEndHandler(pm, depth, tag);
	return ok;
}


