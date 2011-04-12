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
#include <vapor/common.h>

#include <vector>
#include <string>
#include <map>

namespace VAPoR{
class ExpatParseMgr;

class TransferFunction;
class PanelCommand;
class XmlNode;
class ParamNode;
class PARAMS_API DvrParams : public RenderParams{
	
public: 
	 enum DvrType
    {
      DVR_TEXTURE3D_LOOKUP,
      DVR_TEXTURE3D_SHADER,
      DVR_VOLUMIZER,   
      DVR_DEBUG,
      DVR_STRETCHED_GRID,
      DVR_SPHERICAL_SHADER,
      DVR_RAY_CASTER,
	  DVR_RAY_CASTER_2_VAR,
	  DVR_INVALID_TYPE
    };
	 void setType(DvrType val) {type = val; }
	 DvrType getType() {return type;}

	DvrParams(int winnum);
	~DvrParams();
	const std::string& getShortName() {return _shortName;}
	static ParamsBase* CreateDefaultInstance() {return new DvrParams(-1);}
	
	virtual Params* deepCopy(ParamNode* = 0); 
	virtual bool usingVariable(const string& varname){
		return (DataStatus::getInstance()->getVariableName3D(varNum) == varname);
	}

	
	void setNumBits(int val) {numBits = val;}
	int getNumBits() {return numBits;}
	void setLighting(bool val) {lightingOn = val; }
	bool getLighting() const { return lightingOn; }

	void setPreIntegration(bool val) {preIntegrationOn = val; }
	bool getPreIntegration() const { return preIntegrationOn; }
	
	//In addition to setting variableNum, must also 
	//update mapping bounds and edit bounds
	//as well as force a rebuilding of the transfer function
	//
	void setVarNum(int val); 
	virtual int getSessionVarNum() {return varNum;}
	const char* getVariableName() {
		return (const char*) (DataStatus::getInstance()->getVariableName3D(varNum).c_str());
	}
	const std::string& getStdVariableName() {
		return DataStatus::getInstance()->getVariableName3D(varNum);
	}
	
	virtual const float* getCurrentDatarange(){
		return currentDatarange;
	}
	virtual void setCurrentDatarange(float minval, float maxval){
		currentDatarange[0] = minval; currentDatarange[1] = maxval;}
    
	virtual int GetCompressionLevel() {return compressionLevel;}
	virtual void SetCompressionLevel(int val) {compressionLevel = val; }

	virtual bool IsOpaque();

	float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
	void setMinMapBound(float val)
		{ 
          setMinColorMapBound(val);setMinOpacMapBound(val);}
	void setMaxMapBound(float val)
		{
          setMaxColorMapBound(val);setMaxOpacMapBound(val);}
	float getMinMapBound(){return getMinColorMapBound();} 	
	float getMaxMapBound(){return getMaxColorMapBound();} 
	float getOpacityScale(); 
	void setOpacityScale(float val); 
	virtual int GetRefinementLevel() {return numRefinements;}
	void SetRefinementLevel(int n) {numRefinements = n;}

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
		
		return (DataStatus::getInstance()->getDataMin3D(varNum, currentTimeStep));
	}
	float getDataMaxBound(int currentTimeStep){
		if(!DataStatus::getInstance() || numVariables == 0) return 1.f;
		
		return (DataStatus::getInstance()->getDataMax3D(varNum, currentTimeStep));
	}
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	
	void setClut(const float newTable[256][4]);
	
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride);
	virtual void restart();
	static void setDefaultPrefs();
	ParamNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	virtual MapperFunction* GetMapperFunc();
	virtual bool UsesMapperFunction() {return true;}
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	virtual float GetHistoStretch(){return histoStretchFactor;}
	
	int getComboVarNum(){return DataStatus::getInstance()->mapSessionToActiveVarNum3D(varNum);}
	int getNumVariables() {return numVariables;}
	TransferFunction* GetTransFunc(int varnum) {return (transFunc ? transFunc[varnum] : 0);}
	TransferFunction* GetTransFunc() {return ((transFunc && numVariables>0) ? transFunc[varNum] : 0);}

	virtual void hookupTF(TransferFunction* t, int index);
	static int getDefaultBitsPerVoxel() {return defaultBitsPerVoxel;}
	static bool getDefaultLightingEnabled(){return defaultLightingEnabled;}
	static bool getDefaultPreIntegrationEnabled(){return defaultPreIntegrationEnabled;}
	static void setDefaultBitsPerVoxel(int val){defaultBitsPerVoxel = val;}
	static void setDefaultPreIntegration(bool val){defaultPreIntegrationEnabled = val;}
	static void setDefaultLighting(bool val) {defaultLightingEnabled = val;}
protected:
	static const string _shortName;
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _activeVariableNameAttr;
	static const string _dvrLightingAttr;
    static const string _dvrPreIntegrationAttr;
	static const string _numBitsAttr;

	DvrType type;
    
	void refreshCtab();

	bool lightingOn;
    bool preIntegrationOn;
	float currentDatarange[2];
	int numBits;
	
	int numRefinements;
	int compressionLevel;
	
	bool editMode;
	//Transfer fcn LUT: (R,G,B,A)
	//
	float ctab[256][4];
	TransferFunction** transFunc;
	
	float histoStretchFactor;
	PanelCommand* savedCommand;
	int varNum;
	int numVariables;
	static int defaultBitsPerVoxel;
	static bool defaultPreIntegrationEnabled;
	static bool defaultLightingEnabled;
	
};
};
#endif //DVRPARAMS_H 
