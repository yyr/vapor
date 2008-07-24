//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Definition of the TwoDParams class
//		This contains all the parameters required to support the
//		TwoD renderer.  Includes a transfer function and a
//		transfer function editor.
//
#ifndef TWODPARAMS_H
#define TWODPARAMS_H

#define HISTOSTRETCHCONSTANT 0.1f

#include <qwidget.h>
#include "params.h"

#include <vector>
#include <string>
#include "datastatus.h"

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TransferFunction;
class PanelCommand;
class XmlNode;
class FlowParams;
class Histo;
class PARAMS_API TwoDParams : public RenderParams {
	
public: 
	TwoDParams(int winnum);
	~TwoDParams();
	virtual RenderParams* deepRCopy();
	virtual Params* deepCopy() {return (Params*)deepRCopy();}
	
	bool twoDIsDirty(int timestep) {
		return (!twoDDataTextures || twoDDataTextures[timestep] == 0);
	}
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
	float getTwoDMin(int i) {return twoDMin[i];}
	float getTwoDMax(int i) {return twoDMax[i];}
	void setTwoDMin(int i, float val){twoDMin[i] = val;}
	void setTwoDMax(int i, float val){twoDMax[i] = val;}
	void setEditMode(bool mode) {editMode = mode;}
	virtual bool getEditMode() {return editMode;}
	
	TransferFunction* getTransFunc() {return ((transFunc && numVariables>0) ? transFunc[firstVarNum] : 0);}
	
	void setClut(const float newTable[256][4]);
	
	//Respond to user request to load/save TF
	void fileLoadTF();
	void fileSaveTF();
	
	//Set all the cached twoD textures dirty
	void setTwoDDirty();
	//get/set methods
	void setNumRefinements(int numtrans){numRefinements = numtrans; setTwoDDirty();}
	void setMaxNumRefinements(int numtrans) {maxNumRefinements = numtrans;}
	
	//This needs to be fixed to handle multiple variables!
	virtual int getSessionVarNum() { return firstVarNum;}
	
	void getTextureSize(int sze[2]) {sze[0] = textureSize[0]; sze[1] = textureSize[1];}
	//set the texture size appropriately for either ibfv or data twoD, return value in sz.
	void adjustTextureSize(int sz[2]);
	

	float getRealImageWidth() {return twoDMax[0]-twoDMin[0];}
	float getRealImageHeight() {return twoDMax[1]-twoDMin[1];}
	
	
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride);
	virtual void restart();
	static void setDefaultPrefs();
	XmlNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	virtual MapperFunction* getMapperFunc();
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	virtual float GetHistoStretch(){return histoStretchFactor;}
	
	void setTwoDTexture(unsigned char* tex, int timestep){ 
		unsigned char** textureArray = twoDDataTextures;
		if (!textureArray){
			textureArray = new unsigned char*[maxTimestep + 1];
				for (int i = 0; i<= maxTimestep; i++) textureArray[i] = 0;
		}
		if (textureArray[timestep]) delete textureArray[timestep];
		textureArray[timestep] = tex;
		twoDDataTextures = textureArray; 
	}
	unsigned char* calcTwoDDataTexture(int timestep, int wid, int ht);

	//General method that obtains a list of variables (containing the twoD) from the dataMgr
	//Also establishes values of blkMin, blkMax, coordMin, coordMax and actualRefLevel to be used
	//for addressing into the volumes.  Replaces first half of calcTwoDDataTexture.
	float** getTwoDVariables(int ts, int numVars, int* sesVarNums,
				  size_t blkMin[3], size_t blkMax[3], size_t coordMin[3], size_t coordMax[3],
				  int* actualRefLevel);

	unsigned char* getCurrentTwoDTexture(int timestep) {
		return twoDDataTextures[timestep];
		
	}
	void getTwoDVoxelExtents(float voxdims[2]);

	
	virtual void getBox(float boxmin[], float boxmax[]){
		for (int i = 0; i< 3; i++){
			boxmin[i]=twoDMin[i];
			boxmax[i]=twoDMax[i];
		}
	}
	virtual void setBox(const float boxMin[], const float boxMax[]){
		for (int i = 0; i< 3; i++){
			twoDMin[i] = boxMin[i];
			twoDMax[i] = boxMax[i];
		}
	}
	//Get the bounding extents of twoD, in cube coords
	virtual void calcContainingStretchedBoxExtentsInCube(float* extents);
	
	virtual int getNumRefinements() {return numRefinements;}
	virtual void hookupTF(TransferFunction* t, int index);
	float getOpacityScale(); 
	void setOpacityScale(float val); 
	virtual void setEnabled(bool value) {
		enabled = value;
		//Always clear out the cache
		if (!value) setTwoDDirty();
	}
	void setVariableSelected(int sessionVarNum, bool value){
		variableSelected[sessionVarNum] = value;
	}
	bool variableIsSelected(int index) {
		if (index >= (int)variableSelected.size()) return false;
		return variableSelected[index];}
	void setFirstVarNum(int val){firstVarNum = val;}
	int getFirstVarNum() {return firstVarNum;}
	void setNumVariablesSelected(int numselected){numVariablesSelected = numselected;}
	//Get the bounding box of data that is actually on disk.  return false if empty
	bool getAvailableBoundingBox(int timestep, size_t boxMinBlk[3], size_t boxMaxBlk[3], size_t boxMin[3], size_t boxMax[3], int numRefs);
	//Obtain the smallest region that contains the twoD, and fits within the full data volume:
	void getContainingRegion(float regMin[3], float regMax[3]);
	
protected:
	
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _variableSelectedAttr;
	static const string _geometryTag;
	static const string _twoDMinAttr;
	static const string _twoDMaxAttr;
	static const string _cursorCoordsAttr;
	
	static const string _numTransformsAttr;

	void mapCursor();
	void refreshCtab();
			
	//Utility functions for building texture and histogram
	
	//Find smallest containing cube in integer coords, 
	//that will contain image of twoD
	void getBoundingBox(int timestep, size_t boxMin[3], size_t boxMax[3], int numRefs);
	//Get the rotated box sides in the unit cube, based on current angles:
	void getRotatedBoxDims(float boxdims[3]);

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

	//Cache of twoD textures, one per timestep.
	
	unsigned char** twoDDataTextures;
	int maxTimestep;
	
	
	//State variables controlled by GUI:
	float twoDMin[3], twoDMax[3];
	int numRefinements, maxNumRefinements;
	float theta, phi, psi;
	float selectPoint[3];
	float cursorCoords[2];
	
	int textureSize[2];
	
	
	
	
};
};
#endif //TWODPARAMS_H 
