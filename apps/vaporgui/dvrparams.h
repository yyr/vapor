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
#include "session.h"
#include "mainform.h"
#include <vector>
#include <string>
class Dvr;

namespace VAPoR{

class MainForm;
class TFEditor;
class TransferFunction;
class PanelCommand;
class DvrParams : public Params{
	
public: 
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
	void updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow);
	void setNumBits(int val) {numBits = val;}
	void setLighting(bool val) {lightingOn = val;}

	void setDiffuseCoeff(float val) {diffuseCoeff = val;}
	void setAmbientCoeff(float val) {ambientCoeff = val;}
	void setSpecularCoeff(float val) {specularCoeff = val;}
	void setExponent(int val) {specularExponent = val;}

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
	int getVarNum() {return varNum;}
	const char* getVariableName() {
		return (const char*) (variableNames[varNum].c_str());
	}
	const std::string& getStdVariableName() {
		return variableNames[varNum];
	}
	bool attenuationIsDirty() {return attenuationDirty;}
	void setAttenuationDirty(bool dirty) {attenuationDirty = dirty;}
	
	void setClutDirty();
	float* getCurrentDatarange(){
		return currentDatarange;
	}
	void setDatarangeDirty();

	float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
	void setMinMapBound(float val) {
		minMapBounds[varNum] = val;
	}
	void setMaxMapBound(float val) {
		maxMapBounds[varNum] = val;
	}
	float getMinMapBound() {
		return minMapBounds[varNum];
	}
	float getMaxMapBound() {
		return maxMapBounds[varNum];
	}
	void setMinEditBound(float val) {
		minEditBounds[varNum] = val;
	}
	void setMaxEditBound(float val) {
		maxEditBounds[varNum] = val;
	}
	float getMinEditBound() {
		return minEditBounds[varNum];
	}
	float getMaxEditBound() {
		return maxEditBounds[varNum];
	}
	float getDataMinBound(){
		if(numVariables == 0) return 0.f;
		return Session::getInstance()->getDataRange(varNum)[0];
	}
	float getDataMaxBound(){
		if(numVariables == 0) return 1.f;
		return Session::getInstance()->getDataRange(varNum)[1];
	}
	void setEditMode(bool mode) {editMode = mode;}
	bool getEditMode() {return editMode;}
	TransferFunction* getTransferFunction() {return myTransFunc;}
	
		
	void setClut(const float newTable[256][4]);
	void setBindButtons();
	void updateTFBounds();

	//Respond to user request to load/save TF
	void fileLoadTF();
	void fileSaveTF();
	void sessionLoadTF(QString* name);
	
	//Methods with undo/redo support:
	//
	virtual void guiSetEnabled(bool value);
	void guiSetVarNum(int val);
	void guiSetNumBits(int val);
	void guiSetLighting(bool val);
	
	
	
	void guiSetHistoStretch(int val);
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetAligned();
	//respond to changes in TF (for undo/redo):
	//
	void guiStartChangeTF(char* s);
	void guiEndChangeTF();
	void guiBindColorToOpac();
	void guiBindOpacToColor();
	//Implement virtual function to deal with new session:
	void reinit();
	
	
protected:
	void refreshCtab();
	void hookupTF(TransferFunction* t);
	bool attenuationDirty;
	bool lightingOn;
	float currentDatarange[2];
	int numBits;
	float diffuseCoeff, ambientCoeff, specularCoeff;
	float diffuseAtten, ambientAtten, specularAtten;
	int specularExponent;
	
	bool editMode;
	//Transfer fcn LUT: (R,G,B,A)
	//
	float ctab[256][4];
	Dvr* myDvrTab;
	TransferFunction* myTransFunc;
	TFEditor* myTFEditor;
	float histoStretchFactor;
	PanelCommand* savedCommand;
	int varNum;
	int numVariables;
	std::vector<std::string> variableNames;
	float* minMapBounds;
	float* maxMapBounds;
	float* minEditBounds;
	float* maxEditBounds;
};
};
#endif //DVRPARAMS_H 
