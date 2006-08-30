//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		probeparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Definition of the ProbeParams class
//		This contains all the parameters required to support the
//		Probe renderer.  Includes a transfer function and a
//		transfer function editor.
//
#ifndef PROBEPARAMS_H
#define PROBEPARAMS_H

#define HISTOSTRETCHCONSTANT 0.1f
#define OUT_OF_BOUNDS -1.e30f
#include <qwidget.h>
#include "params.h"

#include <vector>
#include <string>
#include "datastatus.h"

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TFEditor;
class TransferFunction;
class PanelCommand;
class XmlNode;
class FlowParams;
class Histo;
class PARAMS_API ProbeParams : public RenderParams{
	
public: 
	ProbeParams(int winnum);
	~ProbeParams();
	virtual Params* deepCopy();
	
	bool probeIsDirty() {return probeDirty;}
	virtual const float* getCurrentDatarange(){
		return currentDatarange;
	}
	virtual void setCurrentDatarange(float minval, float maxval){
		currentDatarange[0] = minval; currentDatarange[1]=maxval;}
	
	const float* getSelectedPoint() {
		mapCursor();
		return selectPoint;
	}
	const float* getCursorCoords(){return cursorCoords;}
	void setCursorCoords(float x, float y){
		cursorCoords[0]=x; cursorCoords[1]=y;
	}

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
	//Virtual methods to set map bounds.  Get() is in parent class
	//this causes it to be set in the mapperfunction (transfer function)
	virtual void setMinColorMapBound(float val);
	virtual void setMaxColorMapBound(float val);
	virtual void setMinOpacMapBound(float val);
	virtual void setMaxOpacMapBound(float val);
	
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
		return (DataStatus::getInstance()->getDataMin(firstVarNum, currentTimeStep));
	}
	float getDataMaxBound(int currentTimeStep){
		if(numVariables == 0) return 1.f;
		return (DataStatus::getInstance()->getDataMax(firstVarNum, currentTimeStep));
	}
	float getProbeMin(int i) {return probeMin[i];}
	float getProbeMax(int i) {return probeMax[i];}
	void setProbeMin(int i, float val){probeMin[i] = val;}
	void setProbeMax(int i, float val){probeMax[i] = val;}
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	TransferFunction* getTransFunc() {return ((transFunc && numVariables>0) ? transFunc[firstVarNum] : 0);}
	
	void setClut(const float newTable[256][4]);
	
	//Respond to user request to load/save TF
	void fileLoadTF();
	void fileSaveTF();
	

	void setProbeDirty(bool dirty = true) {probeDirty = dirty;}
	//get/set methods
	void setNumRefinements(int numtrans){numRefinements = numtrans; setProbeDirty(true);}
	void setMaxNumRefinements(int numtrans) {maxNumRefinements = numtrans;}
	void setXCenter(int sliderval);
	void setYCenter(int sliderval);
	void setZCenter(int sliderval);
	void setXSize(int sliderval);
	void setYSize(int sliderval);
	void setZSize(int sliderval);
	void textToSlider(int coord, float center, float size);
	void sliderToText(int coord, int center, int size);
	//This needs to be fixed to handle multiple variables!
	virtual int getVarNum() { return firstVarNum;}
	
	
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
	//Get the current texture (slice through middle of probe) for rendering
	//This should only be obtained by the router!!
	unsigned char* getCurrentProbeTexture(){return probeTexture;}
	void setProbeTexture(unsigned char* tex){ 
		if (probeTexture) delete probeTexture;
		probeTexture = tex;
	}
	
	virtual float getPhi() {return phi;}
	virtual float getTheta() {return theta;}
	void setTheta(float th) {theta = th;}
	void setPhi(float ph) {phi = ph;}
	virtual void getBox(float boxmin[], float boxmax[]){
		for (int i = 0; i< 3; i++){
			boxmin[i]=probeMin[i];
			boxmax[i]=probeMax[i];
		}
	}
	virtual void setBox(const float boxMin[], const float boxMax[]){
		for (int i = 0; i< 3; i++){
			probeMin[i] = boxMin[i];
			probeMax[i] = boxMax[i];
		}
	}
	//Get the bounding extents of probe, in cube coords
	virtual void calcContainingBoxExtentsInCube(float* extents);
	
	virtual int getNumRefinements() {return numRefinements;}
	void hookupTF(TransferFunction* t, int index);
	float getOpacityScale(); 
	void setOpacityScale(float val); 
	virtual void setEnabled(bool value) {
		enabled = value;
		if (!value && probeTexture){
			delete probeTexture;
			probeTexture = 0;
		}
	}
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	void setVariableSelected(int index, bool value){
		variableSelected[index] = value;
	}
	bool variableIsSelected(int index) {
		if (index >= (int)variableSelected.size()) return false;
		return variableSelected[index];}
	void setFirstVarNum(int val){firstVarNum = val;}
	int getFirstVarNum() {return firstVarNum;}
	void setNumVariablesSelected(int numselected){numVariablesSelected = numselected;}
	//Get the bounding box of data that is actually on disk.  return false if empty
	bool getAvailableBoundingBox(int timestep, size_t boxMinBlk[3], size_t boxMaxBlk[3], size_t boxMin[3], size_t boxMax[3]);
	

protected:
	
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _variableSelectedAttr;
	static const string _geometryTag;
	static const string _probeMinAttr;
	static const string _probeMaxAttr;
	static const string _cursorCoordsAttr;
	static const string _phiAttr;
	static const string _thetaAttr;
	static const string _numTransformsAttr;
	
	
	
	void mapCursor();
	void refreshCtab();
			
	//Utility functions for building texture and histogram
	
	//Find smallest containing cube in integer coords, using
	//current numRefinementsforms, that will contain image of probe
	void getBoundingBox(size_t boxMin[3], size_t boxMax[3]);
	
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
	
	bool probeDirty;
	unsigned char* probeTexture;
	
	
	//State variables controlled by GUI:
	float probeMin[3], probeMax[3];
	int numRefinements, maxNumRefinements;
	float theta, phi;
	float selectPoint[3];
	float cursorCoords[2];
	

	
	
};
};
#endif //PROBEPARAMS_H 
