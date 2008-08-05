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
class PARAMS_API ProbeParams : public RenderParams{
	
public: 
	ProbeParams(int winnum);
	~ProbeParams();
	virtual RenderParams* deepRCopy();
	virtual Params* deepCopy() {return (Params*)deepRCopy();}
	
	bool probeIsDirty(int timestep) {
		if (probeType == 0) return (!probeDataTextures || probeDataTextures[timestep] == 0);
		else return (!probeIBFVTextures || probeIBFVTextures[timestep] == 0);
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
	
	//Set all the cached probe textures dirty
	void setProbeDirty();
	//get/set methods
	void setNumRefinements(int numtrans){numRefinements = numtrans; setProbeDirty();}
	void setMaxNumRefinements(int numtrans) {maxNumRefinements = numtrans;}
	
	//This needs to be fixed to handle multiple variables!
	virtual int getSessionVarNum() { return firstVarNum;}
	
	void getTextureSize(int sze[2]) {sze[0] = textureSize[0]; sze[1] = textureSize[1];}
	//set the texture size appropriately for either ibfv or data probe, return value in sz.
	void adjustTextureSize(int sz[2]);
	// NPN and NMESH are calculated in adjustTextureSize for IBFV textures
	int getNPN() {return NPN;}
	int getNMESH() {return NMESH;}

	float getRealImageWidth() {return probeMax[0]-probeMin[0];}
	float getRealImageHeight() {return probeMax[1]-probeMin[1];}
	
	
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
	
	void setProbeTexture(unsigned char* tex, int timestep, int textureType){ 
		unsigned char** textureArray = (textureType == 0) ? probeDataTextures : probeIBFVTextures;
		if (!textureArray){
			textureArray = new unsigned char*[maxTimestep + 1];
				for (int i = 0; i<= maxTimestep; i++) textureArray[i] = 0;
		}
		if (textureArray[timestep]) delete textureArray[timestep];
		textureArray[timestep] = tex;
		if (textureType == 0) probeDataTextures = textureArray; else probeIBFVTextures = textureArray;
	}
	unsigned char* calcProbeDataTexture(int timestep, int wid, int ht);

	//General method that obtains a list of variables (containing the probe) from the dataMgr
	//Also establishes values of blkMin, blkMax, coordMin, coordMax and actualRefLevel to be used
	//for addressing into the volumes.  Replaces first half of calcProbeDataTexture.
	float** getProbeVariables(int ts, int numVars, int* sesVarNums,
				  size_t blkMin[3], size_t blkMax[3], size_t coordMin[3], size_t coordMax[3],
				  int* actualRefLevel);

	unsigned char* getCurrentProbeTexture(int timestep, int texType) {
		if( texType == 0) return probeDataTextures[timestep];
		else return probeIBFVTextures[timestep];
	}
	void getProbeVoxelExtents(float voxdims[2]);

	virtual float getPhi() {return phi;}
	virtual float getTheta() {return theta;}
	virtual float getPsi() {return psi;}
	
	void setTheta(float th) {theta = th;}
	void setPhi(float ph) {phi = ph;}
	void setPsi(float ps) {psi = ps;}
	bool isPlanar() {return planar;}
	void setPlanar(bool val) {planar = val;}
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
	virtual void calcContainingStretchedBoxExtentsInCube(float* extents);
	//change box dimensions after a rotation so that it appears to be the same size:
	void rotateAndRenormalizeBox(int axis, float rotVal);
	virtual int getNumRefinements() {return numRefinements;}
	virtual void hookupTF(TransferFunction* t, int index);
	float getOpacityScale(); 
	void setOpacityScale(float val); 
	virtual void setEnabled(bool value) {
		enabled = value;
		//Always clear out the cache
		if (!value) setProbeDirty();
	}
	void setVariableSelected(int sessionVarNum, bool value){
		variableSelected[sessionVarNum] = value;
	}
	bool variableIsSelected(int sesindex) {
		if (sesindex >= (int)variableSelected.size()) return false;
		return variableSelected[sesindex];
	}
	void setFirstVarNum(int val){firstVarNum = val;}
	int getFirstVarNum() {return firstVarNum;}
	void setNumVariablesSelected(int numselected){numVariablesSelected = numselected;}
	//Get the bounding box of data that is actually on disk.  return false if empty
	bool getAvailableBoundingBox(int timestep, size_t boxMinBlk[3], size_t boxMaxBlk[3], size_t boxMin[3], size_t boxMax[3], int numRefs);
	//Obtain the smallest region that contains the probe, and fits within the full data volume:
	void getContainingRegion(float regMin[3], float regMax[3]);

	int getProbeType() {return probeType;}
	void setProbeType(int val) {probeType = val;}
	//IBFV parameters:
	float getAlpha() {return alpha;}
	void setAlpha(float val) {alpha = val;}
	//fieldScale is 1/4 times actual scale factor used.
	float getFieldScale() {return fieldScale;}
	void setFieldScale(float val) {fieldScale = val;}
	
	//Advect (x,y) to (*px, *py)
	//all 4 are in grid coords of current probe
	void getIBFVValue(int timestep, float x, float y, float* px, float* py);
	bool buildIBFVFields(int timestep);
	//Project a 3-vector to the probe plane, provide the inverse 3x3 rotation matrix
	void projToPlane(float vecField[3], float invRotMtrx[9], float* U, float* V);
	void setIBFVComboVarNum(int indx, int varnum){
		ibfvComboVarNum[indx] = varnum;
	}
	int getIBFVComboVarNum(int indx) {return ibfvComboVarNum[indx];}
	void setIBFVSessionVarNum(int indx, int varnum){
		ibfvSessionVarNum[indx] = varnum;
	}
	int getIBFVSessionVarNum(int indx) {return ibfvSessionVarNum[indx];}
	bool ibvfPointIsValid(int timestep, int uv){
		return (ibfvValid[timestep][uv] != 0);
	}
	bool ibfvColorMerged() {return mergeColor;}
	void setIBFVColorMerged(bool val) {mergeColor = val;}
	static void setDefaultScale(float val){defaultScale = val;}
	static float getDefaultScale(){return defaultScale;}
	static float getDefaultAlpha(){return defaultAlpha;}
	static void setDefaultAlpha(float val){defaultAlpha = val;}
	static float getDefaultTheta(){return defaultTheta;}
	static void setDefaultTheta(float val){defaultTheta = val;}
	static float getDefaultPhi(){return defaultPhi;}
	static void setDefaultPhi(float val){defaultPhi = val;}
	static float getDefaultPsi(){return defaultPsi;}
	static void setDefaultPsi(float val){defaultPsi = val;}
	
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
	static const string _psiAttr;
	static const string _numTransformsAttr;
	static const string _planarAttr;
	static const string _fieldVarsAttr;
	static const string _mergeColorAttr;
	static const string _fieldScaleAttr;
	static const string _alphaAttr;
	static const string _probeTypeAttr;
	
	
	
	void mapCursor();
	void refreshCtab();
			
	//Utility functions for building texture and histogram
	
	//Find smallest containing cube in integer coords, 
	//that will contain image of probe
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
	bool planar; //whether the probe is required to be planar
	bool clutDirty;
	//The first variable selected is used to specify 
	//which TF will be used.
	int firstVarNum;
	int numVariables;
	int numVariablesSelected;

	int probeType; //0 for data probe; 1 for IBFV
	
	
	//Cache of probe textures, one per timestep.
	unsigned char** probeIBFVTextures;
	unsigned char** probeDataTextures;
	int maxTimestep;
	
	
	//State variables controlled by GUI:
	float probeMin[3], probeMax[3];
	int numRefinements, maxNumRefinements;
	float theta, phi, psi;
	float selectPoint[3];
	float cursorCoords[2];
	
	int textureSize[2];
	//Values used for IBFV sampling
	int NPN;
	int NMESH;
	//IBFV parameters:
	float alpha, fieldScale;
	bool mergeColor;
	//There are 2 ibfv fields for each time step
	//ibfvUField[t][w] is the U-value at point w = (x+wid*y) at timestep t
	//ibfvValid indicates that the associated texture point is within valid region.
	float** ibfvUField;
	float** ibfvVField;
	unsigned char** ibfvValid;
	float ibfvMag; //Saves the average magnitude of the 2d field at the first time step calculated.
	//This avoids change in normalization per timestep; perhaps it would be better to normalize
	//different timesteps differently?
	//To specify variable num, variable 0 is the constant 0 variable.
	int ibfvComboVarNum[3];  //variable nums in combo. These are metadata var nums +1
	int ibfvSessionVarNum[3]; // session variable nums +1
	//scale factors for mapping grid cell space to world coords in probe. Used in projToPlane
	float ibfvXScale, ibfvYScale;  
	static float defaultScale; 
	static float defaultAlpha; 
	static float defaultTheta; 
	static float defaultPhi;
	static float defaultPsi;

	
	
};
};
#endif //PROBEPARAMS_H 
