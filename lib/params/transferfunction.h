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
#include <vapor/tfinterpolator.h>

#include "mapperfunction.h"

namespace VAPoR {
class TFEditor;
class RenderParams;
class XmlNode;
class ParamNode;

class PARAMS_API TransferFunction : public MapperFunction 
{

public:

	TransferFunction();
	TransferFunction(RenderParams* p, int nBits=8);
	TransferFunction(const MapperFunctionBase &mapper);
	TransferFunction(const ParamsBase* parent);
	virtual ~TransferFunction();
	static ParamsBase* CreateDefaultInstance(){return new TransferFunction();}

  
    //
	// Methods to save and restore transfer functions.
	// The gui opens the FILEs that are then read/written
	// Failure results in false/null pointer
	//
	bool saveToFile(ofstream& f);
    static TransferFunction* loadFromFile(ifstream& is, RenderParams *p);

	//Transfer function tag is visible to session 
	static const string _transferFunctionTag;
	virtual void hookup(RenderParams* p, int cvar){
		_params = p;
		setVarNum(cvar);
		ColorMapBase* cmap = GetColorMap();
		if(cmap) cmap->setMapper(this);
		if(getNumOpacityMaps()>0) (GetOpacityMap(0))->setMapper(this);
	}
	virtual TransferFunction* deepCopy(ParamNode* newRoot = 0){
		TransferFunction* tf = new TransferFunction(*this);
		tf->SetRootParamNode(newRoot);
		if(newRoot) newRoot->SetParamsBase(tf);
		return tf;
	}
	
protected:
	
    //
    // XML tags
    //
	static const string _tfNameAttr;
	static const string _leftBoundAttr;
	static const string _rightBoundAttr;
	static const string _leftBoundTag;
	static const string _rightBoundTag;
	static const string _tfNameTag;
};
};
#endif //TRANSFERFUNCTION_H
