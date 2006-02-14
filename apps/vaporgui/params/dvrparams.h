//************************************************************************
//										*
//		     Copyright (C)  2004						*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved						*
//											*
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
#include "session.h"
#include "mainform.h"
#include <vector>
#include <string>
#include <map>
class Dvr;

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TFEditor;
class TransferFunction;
class PanelCommand;
class XmlNode;
class DvrParams : public Params{
	
public: 

    enum DvrType
    {
      DVR_TEXTURE3D_LOOKUP,
      DVR_TEXTURE3D_SHADER,
      DVR_VOLUMIZER,   
      DVR_DEBUG,
      DVR_STRETCHED_GRID,
    };

	DvrParams(int winnum);
	~DvrParams();
	virtual Params* deepCopy();
	void setTab(Dvr* tab); 
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();
	//


	virtual void makeCurrent(Params* previousParams, bool newWin);
	virtual void updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow);
	void setNumBits(int val) {numBits = val;}
	void setLighting(bool val) {lightingOn = val; setClutDirty(); }
	bool getLighting() const { return lightingOn; }

	void setDiffuseCoeff(float val) {diffuseCoeff = val; setClutDirty();}
	void setAmbientCoeff(float val) {ambientCoeff = val; setClutDirty();}
	void setSpecularCoeff(float val) {specularCoeff = val; setClutDirty();}
	void setExponent(int val) {specularExponent = val; setClutDirty();}

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
		return (const char*) (Session::getInstance()->getVariableName(varNum).c_str());
	}
	const std::string& getStdVariableName() {
		return Session::getInstance()->getVariableName(varNum);
	}
	bool attenuationIsDirty() {return attenuationDirty;}
	void setAttenuationDirty(bool dirty) {attenuationDirty = dirty;}
	
	virtual void setClutDirty();
	virtual float* getCurrentDatarange(){
		return currentDatarange;
	}
    
    void initTypes();
	void setDatarangeDirty();

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
		if(numVariables == 0) return 0.f;
		
		return (Session::getInstance()->getDataMin(varNum, currentTimeStep));
	}
	float getDataMaxBound(int currentTimeStep){
		if(numVariables == 0) return 1.f;
		
		return (Session::getInstance()->getDataMax(varNum, currentTimeStep));
	}
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	void refreshTFFrame();
	
	void setClut(const float newTable[256][4]);
	virtual void setBindButtons();
	virtual void updateMapBounds();

	//Respond to user request to load/save TF
	void fileLoadTF();
	void fileSaveTF();
	virtual void sessionLoadTF(QString* name);
	
	//Methods with undo/redo support:
	//
	virtual void guiSetEnabled(bool value);
    void guiSetType(int val);
	void guiSetComboVarNum(int val);
	void guiSetNumBits(int val);
	void guiSetLighting(bool val);
	
	
	
	void guiSetOpacityScale(int val);
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetAligned();
	void guiSetNumRefinements(int num);

	//respond to changes in TF (for undo/redo):
	//
	virtual void guiStartChangeMapFcn(char* s);
	virtual void guiEndChangeMapFcn();
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	//Verify that this numRefinements is valid
	//with the current applicable region and animation steps:
	//Return the largest valid number <= num
	//
	int checkNumRefinements(int num) {return num;}
	//Implement virtual function to deal with new session:
	void reinit(bool doOverride);
	void restart();
	XmlNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	TFEditor* getTFEditor();
	virtual MapperFunction* getMapperFunc();
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	virtual float getHistoStretch(){return histoStretchFactor;}
	//Methods to support maintaining a list of histograms
	//in each params (at least those with a TFE)
	virtual Histo* getHistogram(bool mustGet);
	virtual void refreshHistogram();
protected:
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _activeVariableNameAttr;
	
	void refreshCtab();
	void hookupTF(TransferFunction* t, int index);
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);

    DvrType type;
    std::map<int, DvrType> typemap;

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
	Dvr* myDvrTab;
	TransferFunction** transFunc;
	
	float histoStretchFactor;
	PanelCommand* savedCommand;
	int varNum;
	int comboVarNum;
	int numVariables;
	
	
	
};
};
#endif //DVRPARAMS_H 
