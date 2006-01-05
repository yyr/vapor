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
#include "session.h"
#include "mainform.h"
#include <vector>
#include <string>
class ProbeTab;

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TFEditor;
class TransferFunction;
class PanelCommand;
class XmlNode;
class FlowParams;
class Histo;
class ProbeParams : public Params{
	
public: 
	ProbeParams(int winnum);
	~ProbeParams();
	virtual Params* deepCopy();
	void setTab(ProbeTab* tab); 
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();
	//


	virtual void makeCurrent(Params* previousParams, bool newWin);
	virtual void updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow);
	

	
	//In addition to setting variableNum, must also 
	//update mapping bounds and edit bounds
	//as well as force a rebuilding of the transfer function
	//
	void setVarNum(int val); 
	
	const char* getVariableName() {
		return (const char*) (variableNames[firstVarNum].c_str());
	}
	const std::string& getStdVariableName() {
		return variableNames[firstVarNum];
	}
	
	virtual void setClutDirty();
	virtual float* getCurrentDatarange(){
		return currentDatarange;
	}
	void setDatarangeDirty();
	float* getSelectedPoint() {
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
		return (Session::getInstance()->getDataMin(firstVarNum, currentTimeStep));
	}
	float getDataMaxBound(int currentTimeStep){
		if(numVariables == 0) return 1.f;
		return (Session::getInstance()->getDataMax(firstVarNum, currentTimeStep));
	}
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	void refreshTFFrame();
	
	void setClut(const float newTable[256][4]);
	void setBindButtons();
	virtual void updateMapBounds();

	//Respond to user request to load/save TF
	void fileLoadTF();
	void fileSaveTF();
	virtual void sessionLoadTF(QString* name);

	void setProbeDirty(bool dirty = true);
	//get/set methods
	void setNumTrans(int numtrans){numTransforms = numtrans; setProbeDirty(true);}
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
	
	//Methods with undo/redo support, accessed from gui
	//
	virtual void guiSetEnabled(bool value);
	
	void guiCenterProbe();
	void guiAttachSeed(bool attach, FlowParams*);
	void guiChangeVariables();
	void guiSetXCenter(int sliderVal);
	void guiSetYCenter(int sliderVal);
	void guiSetZCenter(int sliderVal);
	void guiSetOpacityScale(int val);
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetAligned();
	void guiSetNumTrans(int numtrans);
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiStartCursorMove();
	void guiEndCursorMove();
	
	//respond to changes in TF (for undo/redo):
	//
	virtual void guiStartChangeMapFcn(char* s);
	virtual void guiEndChangeMapFcn();
	void guiBindColorToOpac();
	void guiBindOpacToColor();
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
	//Get the current texture (slice through middle of probe) for rendering
	unsigned char* getProbeTexture();
	
	virtual float getPhi() {return phi;}
	virtual float getTheta() {return theta;}
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
	//Save state when user clicks on a probe handle in the scene
	void captureMouseDown();
	virtual void captureMouseUp();
	bool seedIsAttached(){return seedAttached;}

	//Methods to support maintaining a list of histograms
	//in each params (at least those with a TFE)
	virtual Histo* getHistogram(bool mustGet);
	virtual void refreshHistogram();

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
	
	//Determine the value of the variable(s) at specified point.
	//Return OUT_OF_BOUNDS if not in probe and in full domain
	float calcCurrentValue(float point[3]);

	
	void mapCursor();
	void refreshCtab();
	void hookupTF(TransferFunction* t, int index);
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	//Get the data associated with the probe region for a variable and timestep
	float* getContainingVolume(size_t blkMin[3], size_t blkMax[3], int varNum, int timeStep);
	float getOpacityScale(); 
	void setOpacityScale(float val); 
		
	//Utility functions for building texture and histogram
	

	//Find smallest containing cube in integer coords, using
	//current numTransforms, that will contain image of probe
	void getBoundingBox(size_t boxMinBlk[3], size_t boxMaxBlk[3], int boxMin[3], int boxMax[3]);

	float currentDatarange[2];
	
	bool editMode;
	//Transfer fcn LUT: (R,G,B,A)
	//
	float ctab[256][4];
	ProbeTab* myProbeTab;
	TransferFunction** transFunc;
	
	float histoStretchFactor;
	PanelCommand* savedCommand;
	std::vector<std::string> variableNames;
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
	int numTransforms, maxNumTrans, minNumTrans;
	float theta, phi;
	float selectPoint[3];
	float cursorCoords[2];
	bool seedAttached;
	FlowParams* attachedFlow;
	
};
};
#endif //PROBEPARAMS_H 
