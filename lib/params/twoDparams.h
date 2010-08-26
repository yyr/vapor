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
//		This contains common parameters required to support the
//		TwoD Image and Data renderer.  
//
#ifndef TWODPARAMS_H
#define TWODPARAMS_H

#include <qwidget.h>
#include "params.h"
#include "xtiffio.h"
#include <vector>
#include <string>
#include "datastatus.h"

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TransferFunction;
class PanelCommand;
class XmlNode;
class ParamNode;
class FlowParams;
class Histo;
class ViewpointParams;
class RegionParams;

class PARAMS_API TwoDParams : public RenderParams {
	
public: 
	TwoDParams(int winnum, const string& name);
	~TwoDParams();
	
	virtual bool twoDIsDirty(int timestep)= 0;
		
	bool elevGridIsDirty() { 
		return elevGridDirty;
	}
	void setElevGridDirty(bool val){
		elevGridDirty = val;
	}
	
	const float* getSelectedPoint() {
		return selectPoint;
	}
	void setSelectedPoint(const float point[3]){
		for (int i = 0; i<3; i++) selectPoint[i] = point[i];
	}
	const float* getCursorCoords(){return cursorCoords;}
	void setCursorCoords(float x, float y){
		cursorCoords[0]=x; cursorCoords[1]=y;
	}
	void setOrientation(int val);
	int getOrientation() {return orientation;}
	
	virtual void getTextureSize(int sze[2], int timestep) = 0;
	
	void getTwoDVoxelExtents(float voxdims[2]);
	
	float getTwoDMin(int i) {return twoDMin[i];}
	float getTwoDMax(int i) {return twoDMax[i];}
	void setTwoDMin(int i, float val){twoDMin[i] = val;}
	void setTwoDMax(int i, float val){
		twoDMax[i] = val;
	}
	
	
	
	//Set all the cached twoD textures dirty, as well as elev grid
	virtual void setTwoDDirty()=0;
	int getMaxTimestep() {return maxTimestep;}
	//get/set methods
	void setNumRefinements(int numtrans){numRefinements = numtrans; setTwoDDirty();}
	void setMaxNumRefinements(int numtrans) {maxNumRefinements = numtrans;}

	virtual bool imageCrop()=0;

	float getRealImageWidth() {return twoDMax[0]-twoDMin[0];}
	float getRealImageHeight() {return twoDMax[1]-twoDMin[1];}
	
	
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride) = 0;
	virtual void restart() = 0;
	static void setDefaultPrefs();
	virtual ParamNode* buildNode()=0; 
	virtual bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/)=0;
	virtual bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/)=0;
	
	
	virtual void setTwoDTexture(unsigned char* tex, int timestep, int imgSize[2],
		float imExts[4] = 0 ) = 0;
	virtual unsigned char* calcTwoDDataTexture(int timestep, int wid, int ht)= 0;

	virtual unsigned char* getCurrentTwoDTexture(int timestep)= 0; 
	
	virtual void getBox(float boxmin[], float boxmax[], int = -1 ){
		for (int i = 0; i< 3; i++){
			boxmin[i]=twoDMin[i];
			boxmax[i]=twoDMax[i];
		}
	}
	virtual void setBox(const float boxMin[], const float boxMax[], int = -1){
		for (int i = 0; i< 3; i++){
			twoDMin[i] = boxMin[i];
			twoDMax[i] = boxMax[i];
		}
	}
	//Get the bounding extents of twoD, in cube coords
	virtual void calcContainingStretchedBoxExtentsInCube(float* extents);
	
	virtual int getNumRefinements() {return numRefinements;}
	
	virtual void setEnabled(bool value) {
		enabled = value;
		
	}
	virtual float getCameraDistance(ViewpointParams* vp, RegionParams* rp, int timestep);
	float getVerticalDisplacement(){return verticalDisplacement;}
	void setVerticalDisplacement(float val) {verticalDisplacement = val;}
	bool isMappedToTerrain() {return mapToTerrain;}
	void setMappedToTerrain(bool val) {mapToTerrain = val;}
	//override parent version for 2D box corners:
	virtual void calcBoxCorners(float corners[8][3], float extraThickness, int timestep=-1, float rotation = 0.f, int axis = -1);
	//Construct transform of form (x,y)-> a[0]x+b[0],a[1]y+b[1],
	//Mapping [-1,1]X[-1,1] into 3D volume.
	void build2DTransform(float a[2],float b[2], float* constVal, int mappedDims[3]);
	void setLastTwoDTexture(unsigned char* tex) {lastTwoDTexture = tex;}
	unsigned char* getLastTwoDTexture() {return lastTwoDTexture;}
protected:
	
	static const string _geometryTag;
	static const string _twoDMinAttr;
	static const string _twoDMaxAttr;
	static const string _cursorCoordsAttr;
	static const string _numTransformsAttr;
	static const string _terrainMapAttr;
	static const string _verticalDisplacementAttr;
	static const string _orientationAttr;
	void refreshCtab();
			
	//Utility functions for building texture and histogram
	
	//Find smallest containing cube in integer coords, 
	//that will contain image of twoD
	void getBoundingBox(int timestep, size_t boxMin[3], size_t boxMax[3], int numRefs);
	

	bool elevGridDirty;

	//Cache of twoD textures, one per timestep.
	//Also cache image positions, because each image has 
	//different LL and UR coordinates (meters in projection space)
	//for each time step.
	//The elev grid is cached in the 
	//renderer class
	unsigned char** twoDDataTextures;
	int maxTimestep;

	int orientation; //Only settable in image mode
	

	float twoDMin[3], twoDMax[3];
	int numRefinements, maxNumRefinements;
	int * textureSizes; //2 ints for each time step
	float selectPoint[3];
	float cursorCoords[2];
	float verticalDisplacement;
	bool mapToTerrain;
	float minTerrainHeight, maxTerrainHeight;
	unsigned char* lastTwoDTexture;
	
};
};
#endif //TWODPARAMS_H 
