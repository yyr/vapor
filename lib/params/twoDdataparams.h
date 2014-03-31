//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDdataparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Definition of the TwoDParams class
//		This contains all the parameters required to support the
//		TwoD renderer.  Includes a transfer function and a
//		transfer function editor.
//
#ifndef TWODDATAPARAMS_H
#define TWODDATAPARAMS_H

#define HISTOSTRETCHCONSTANT 0.1f

#include <qwidget.h>
#include "params.h"
#include "xtiffio.h"
#include <vector>
#include <string>
#include "datastatus.h"
#include "twoDparams.h"

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TransferFunction;
class PanelCommand;
class XmlNode;
class ParamNode;
class FlowParams;
class Histo;

class PARAMS_API TwoDDataParams : public TwoDParams {
	
public: 
	TwoDDataParams(int winnum);
	~TwoDDataParams();
	static ParamsBase* CreateDefaultInstance() {return new TwoDDataParams(-1);}
	const std::string& getShortName() {return _shortName;}
	
	virtual Params* deepCopy(ParamNode* =0);
	
	
	virtual void getTextureSize(int sze[2]) {
		sze[0] = _textureSizes[0];
		sze[1] = _textureSizes[1];
	}
	
	virtual const float* getCurrentDatarange(){
		return currentDatarange;
	}
	virtual void setCurrentDatarange(float minval, float maxval){
		currentDatarange[0] = minval; currentDatarange[1]=maxval;}
	

	float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
	virtual bool IsOpaque();
	virtual bool imageCrop() {return true;}//always crop to extents
	void setMinMapBound(float val)
		{setMinColorMapBound(val);setMinOpacMapBound(val);}
	void setMaxMapBound(float val)
		{setMaxColorMapBound(val);setMaxOpacMapBound(val);}
	float getMinMapBound(){return getMinColorMapBound();} 	
	float getMaxMapBound(){return getMaxColorMapBound();} 
	//Virtual methods to set map bounds.  Get() is in parent class
	//this causes it to be set in the mapperfunction (transfer function)
	virtual void setMinColorMapBound(float val);
	virtual void setMaxColorMapBound(float val);
	virtual void setMinOpacMapBound(float val);
	virtual void setMaxOpacMapBound(float val);

	virtual bool usingVariable(const string& varname);
	virtual bool UsesMapperFunction() {return true;}
	//Override default, allow manip to go outside of data:
	virtual bool isDomainConstrained() {return false;}
	
	void setMinEditBound(float val) {
		setMinColorEditBound(val, firstVarNum);
		setMinOpacEditBound(val, firstVarNum);
	}
	void setMaxEditBound(float val) {
		setMaxColorEditBound(val, firstVarNum);
		setMaxOpacEditBound(val, firstVarNum);
	}
	float getMinEditBound() {
		return minColorEditBounds[firstVarNum];
	}
	float getMaxEditBound() {
		return maxColorEditBounds[firstVarNum];
	}
	float getDataMinBound(int currentTimeStep){
		if(numVariables == 0) return 0.f;
		if(numVariablesSelected > 1) return (0.f);
		return (DataStatus::getInstance()->getDataMin2D(firstVarNum, currentTimeStep));
	}
	float getDataMaxBound(int currentTimeStep){
		if(numVariables == 0) return 1.f;
		DataStatus* ds = DataStatus::getInstance();
		if(numVariablesSelected <= 1) 
			return (ds->getDataMax2D(firstVarNum, currentTimeStep));
		//calc rms of selected variable maxima
		float sumVal = 0.f;
		for (int i = 0; i<numVariables; i++){
			if (variableSelected[i]) sumVal +=  ((ds->getDataMax2D(i, currentTimeStep)*ds->getDataMax2D(i, currentTimeStep)));
		}
		return (sqrt(sumVal));
	}
	
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	TransferFunction* GetTransFunc() {return ((transFunc && numVariables>0) ? transFunc[firstVarNum] : 0);}
	
	void setClut(const float newTable[256][4]);
	
	//Respond to user request to load/save TF
	void fileLoadTF();
	void fileSaveTF();
	
	//Set all the cached twoD textures dirty, as well as elev grid
	void setTwoDDirty();
	
	//This needs to be fixed to handle multiple variables!
	virtual int getSessionVarNum() { return firstVarNum;}
	
	
	//determine the texture size appropriately for either ibfv or data twoD, return value in sz.
	void adjustTextureSize(int sz[2]);
	
	
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride);
	virtual void restart();
	static void setDefaultPrefs();
	ParamNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	virtual MapperFunction* GetMapperFunc();
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	virtual float GetHistoStretch(){return histoStretchFactor;}
	
	const unsigned char* calcTwoDDataTexture(int timestep, int &wid, int &ht);

	virtual int getInterpolationOrder(){
		return ( linearInterpTex() ? 1 : 0);
	}
	virtual void hookupTF(TransferFunction* t, int index);
	float getOpacityScale(); 
	void setOpacityScale(float val); 
	
	void setVariableSelected(int sessionVarNum, bool value){
		variableSelected[sessionVarNum] = value;
	}
	bool variableIsSelected(int index) {
		if (index >= (int)variableSelected.size()) return false;
		return variableSelected[index];}
	void setFirstVarNum(int val){firstVarNum = val;}
	int getFirstVarNum() {return firstVarNum;}
	void setNumVariablesSelected(int numselected){numVariablesSelected = numselected;}
	int getNumVariablesSelected() {return numVariablesSelected;}
	
		
	virtual bool GetIgnoreFidelity() {return ignoreFidelity;}
	virtual void SetIgnoreFidelity(bool val){ignoreFidelity = val;}
	virtual int GetFidelityLevel() {return fidelityLevel;}
	virtual void SetFidelityLevel(int lev){fidelityLevel = lev;}

protected:
	static const string _shortName;
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _variableSelectedAttr;
	static const string _linearInterpAttr;
	void refreshCtab();
	int fidelityLevel;
	bool ignoreFidelity;
	//Utility functions for building texture and histogram
	
	float currentDatarange[2];
	bool editMode;
	//Transfer fcn LUT: (R,G,B,A)
	//
	float ctab[256][4];
	
	TransferFunction** transFunc;
	float histoStretchFactor;
	std::vector<bool> variableSelected;
	
	bool clutDirty;
	//The first variable selected is used to specify 
	//which TF will be used.
	int firstVarNum;
	int numVariables;
	int numVariablesSelected;

private:
	int _timestep;
	vector <string> _varname;
	int _reflevel;
	int _lod;
	vector <double> _usrExts;
	unsigned char *_texBuf;
	
	bool twoDIsDirty(int timestep);
	
};
};
#endif //TWODPARAMS_H 
