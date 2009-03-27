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
class FlowParams;
class Histo;

class PARAMS_API TwoDDataParams : public TwoDParams {
	
public: 
	TwoDDataParams(int winnum);
	~TwoDDataParams();
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
	

	float (&getClut())[256][4] {
		refreshCtab();
		return ctab;
	}
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
	
	TransferFunction* getTransFunc() {return ((transFunc && numVariables>0) ? transFunc[firstVarNum] : 0);}
	
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
	XmlNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	virtual MapperFunction* getMapperFunc();
	void setHistoStretch(float factor){histoStretchFactor = factor;}
	virtual float GetHistoStretch(){return histoStretchFactor;}
	
	void setTwoDTexture(unsigned char* tex, int timestep, int imgSize[2],
		float[4] = 0 ){ 
		unsigned char** textureArray = twoDDataTextures;
		if (!textureArray){
			textureArray = new unsigned char*[maxTimestep + 1];
			textureSizes = new int[2*(maxTimestep+1)];
			for (int i = 0; i<= maxTimestep; i++) {
				textureArray[i] = 0;
				textureSizes[2*i] = 0;
				textureSizes[2*i+1] = 0;
			}
			
			twoDDataTextures = textureArray;	
		}
		if (textureArray[timestep]) 
			delete textureArray[timestep];
		textureSizes[2*timestep] = imgSize[0];
		textureSizes[2*timestep+1] = imgSize[1];
		textureArray[timestep] = tex;
		
		 
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
	//Get the bounding box of data that is actually on disk.  return false if empty
	bool getAvailableBoundingBox(int timestep, size_t boxMinBlk[3], size_t boxMaxBlk[3], size_t boxMin[3], size_t boxMax[3], int numRefs);
	
protected:
	static const string _editModeAttr;
	static const string _histoStretchAttr;
	static const string _variableSelectedAttr;
	void refreshCtab();
			
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

	
	
};
};
#endif //TWODPARAMS_H 
