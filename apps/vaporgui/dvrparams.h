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
	DvrParams(MainForm*, int winnum);
	~DvrParams();
	virtual Params* deepCopy();
	void setTab(Dvr* tab); 
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();

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
	
	void setVarNum(int val) {varNum = val;}
	int getVarNum() {return varNum;}
	const char* getVariableName() {
		return (const char*) (variableNames[varNum].c_str());
	}
	bool attenuationIsDirty() {return attenuationDirty;}
	void setAttenuationDirty(bool dirty) {attenuationDirty = dirty;}
	bool clutIsDirty() {return clutDirty;}
	void setClutDirty(bool dirty);
	float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
	float getMinDataValue(){return minDataValue;}
	float getMaxDataValue() {return maxDataValue;}
	void setMinDataValue(float val) {
		minDataValue = val;
		setDataRange();
	}
	void setMaxDataValue(float val) {
		maxDataValue = val;
		setDataRange();
	}
	void setDataRange();
		
	void setClut(const float newTable[256][4]);
	void setBindButtons();
	void updateTFBounds();
	
	//Methods with undo/redo support:
	//
	virtual void guiSetEnabled(bool value);
	void guiSetVarNum(int val);
	void guiSetNumBits(int val);
	void guiSetLighting(bool val);
	void guiStartTFECenterSlide();
	void guiEndTFECenterSlide(int val);
	void guiMoveTFECenter(int val);
	void guiSetTFESize(int val);
	void guiStartTFDomainCenterSlide();
	void guiEndTFDomainCenterSlide(int val);
	void guiMoveTFDomainCenter(int val);
	void guiSetTFDomainSize(int val);
	void guiSetHistoStretch(int val);
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
	bool enforceConsistency();
	void setTFESliders();
	bool attenuationDirty;
	bool clutDirty;
	
	
	bool lightingOn;
	int numBits;
	float diffuseCoeff, ambientCoeff, specularCoeff;
	float diffuseAtten, ambientAtten, specularAtten;
	int specularExponent;
	
	//Transfer fcn LUT: (R,G,B,A)
	//
	float ctab[256][4];
	Dvr* myDvrTab;
	TransferFunction* myTransFunc;
	TFEditor* myTFEditor;
	float histoStretchFactor;
	PanelCommand* savedCommand;
	float minDataValue, maxDataValue;
	float leftTFLimit, rightTFLimit;
	int varNum;
	int numVariables;
	std::vector<std::string> variableNames;

};
};
#endif //DVRPARAMS_H 
