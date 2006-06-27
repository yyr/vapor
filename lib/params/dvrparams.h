//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		dvrparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Definition of the DvrParams class
//		This contains all the parameters required to support the
//		dvr renderer.  Includes a transfer function and a
//		transfer function editor.
//
#ifndef DVRPARAMS_H
#define DVRPARAMS_H

#define HISTOSTRETCHCONSTANT 0.1f
#include <qwidget.h>
#include "params.h"
#include "datastatus.h"
#include "vaporinternal/common.h"

#include <vector>
#include <string>
#include <map>

namespace VAPoR{
class ExpatParseMgr;

class TFEditor;
class TransferFunction;
class PanelCommand;
class XmlNode;
class PARAMS_API DvrParams : public Params{
	
public: 
	 enum DvrType
    {
      DVR_TEXTURE3D_LOOKUP,
      DVR_TEXTURE3D_SHADER,
      DVR_VOLUMIZER,   
      DVR_DEBUG,
      DVR_STRETCHED_GRID,
	  DVR_INVALID_TYPE
    };
	 void setType(DvrType val) {type = val; }
	 DvrType getType() {return type;}

	DvrParams(int winnum);
	~DvrParams();
	virtual Params* deepCopy();
	
	void setNumBits(int val) {numBits = val;}
	int getNumBits() {return numBits;}
	void setLighting(bool val) {lightingOn = val; }
	bool getLighting() const { return lightingOn; }

	void setDiffuseCoeff(float val) {diffuseCoeff = val;}
	void setAmbientCoeff(float val) {ambientCoeff = val; }
	void setSpecularCoeff(float val) {specularCoeff = val; }
	void setExponent(int val) {specularExponent = val;}

	float getDiffuseCoeff()  { return diffuseCoeff; }
	float getAmbientCoeff()  { return ambientCoeff; }
	float getSpecularCoeff() { return specularCoeff; }
	int   getExponent()      { return specularExponent; }

	void setDiffuseAttenuation(float val) {diffuseAtten = val;
		attenuationDirty = true;}
	void setAmbientAttenuation(float val) {ambientAtten = val;
		attenuationDirty = true;}
	void setSpecularAttenuation(float val) {specularAtten = val;
		attenuationDirty = true;}
	float getDiffuseAttenuation() {return diffuseAtten;}
	float getAmbientAttenuation() {return ambientAtten;}
	float getSpecularAttenuation() {return specularAtten;}
	//In addition to setting variableNum, must also 
	//update mapping bounds and edit bounds
	//as well as force a rebuilding of the transfer function
	//
	void setVarNum(int val); 
	virtual int getVarNum() {return varNum;}
	const char* getVariableName() {
		return (const char*) (DataStatus::getInstance()->getVariableName(varNum).c_str());
	}
	const std::string& getStdVariableName() {
		return DataStatus::getInstance()->getVariableName(varNum);
	}
	bool attenuationIsDirty() {return attenuationDirty;}
	void setAttenuationDirty(bool dirty) {attenuationDirty = dirty;}
	
	virtual const float* getCurrentDatarange(){
		return currentDatarange;
	}
	virtual void setCurrentDatarange(float minval, float maxval){
		currentDatarange[0] = minval; currentDatarange[1] = maxval;}
    

	float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
	void setMinMapBound(float val)
		{setMinColorMapBound(val);setMinOpacMapBound(val);}
	void setMaxMapBound(float val)
		{setMaxColorMapBound(val);setMaxOpacMapBound(val);}
	float getMinMapBound(){return getMinColorMapBound();} 	
	float getMaxMapBound(){return getMaxColorMapBound();} 
	float getOpacityScale(); 
	void setOpacityScale(float val); 
	virtual int getNumRefinements() {return numRefinements;}
	void setNumRefinements(int n) {numRefinements = n;}

	//Virtual methods to set map bounds.  Get() is in parent class
	//this causes it to be set in the mapperfunction (transfer function)
	virtual void setMinColorMapBound(float val);
	virtual void setMaxColorMapBound(float val);
	virtual void setMinOpacMapBound(float val);
	virtual void setMaxOpacMapBound(float val);
	
	void setMinEditBound(float val) {
		setMinColorEditBound(val, varNum);
		setMinOpacEditBound(val, varNum);
	}
	void setMaxEditBound(float val) {
		
		setMaxColorEditBound(val, varNum);
		setMaxOpacEditBound(val, varNum);
	}
	float getMinEditBound() {
		
		return minColorEditBounds[varNum];
	}
	float getMaxEditBound() {
		
		return maxColorEditBounds[varNum];
	}
	float getDataMinBound(int currentTimeStep){
		if(!DataStatus::getInstance() || numVariables == 0) return 0.f;
		
		return (DataStatus::getInstance()->getDataMin(varNum, currentTimeStep));
	}
	float getDataMaxBound(int currentTimeStep){
		if(!DataStatus::getInstance() || numVariables == 0) return 1.f;
		
		return (DataStatus::getInstance()->getDataMax(varNum, currentTimeStep));
	}
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	
	void setClut(const float newTable[256][4]);
	

	//Verify that this numRefinements is valid
	//with the current applicable region and animation steps:
	//Return the largest valid number <= num
	//
	int checkNumRefinements(int num) {return num;}
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride);
	void restart();
	XmlNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	TFEditor* getTFEditor();
	virtual MapperFunction* getMapperFunc();
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	virtual float getHistoStretch(){return histoStretchFactor;}
	
	int getComboVarNum(){return comboVarNum;}
	int getNumVariables() {return numVariables;}
	TransferFunction* getTransFunc(int varnum) {return (transFunc ? transFunc[varnum] : 0);}
	TransferFunction* getTransFunc() {return ((transFunc && numVariables>0) ? transFunc[varNum] : 0);}
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	void hookupTF(TransferFunction* t, int index);
protected:
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _activeVariableNameAttr;
	DvrType type;
    
	void refreshCtab();

	bool attenuationDirty;
	bool lightingOn;
	float currentDatarange[2];
	int numBits;
	float diffuseCoeff, ambientCoeff, specularCoeff;
	float diffuseAtten, ambientAtten, specularAtten;
	int specularExponent;
	int numRefinements;
	
	bool editMode;
	//Transfer fcn LUT: (R,G,B,A)
	//
	float ctab[256][4];
	TransferFunction** transFunc;
	
	float histoStretchFactor;
	PanelCommand* savedCommand;
	int varNum;
	int comboVarNum;
	int numVariables;
	
	
	
};
};
#endif //DVRPARAMS_H 
