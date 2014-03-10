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
	
	bool elevGridIsDirty() { 
		return elevGridDirty;
	}
	void setElevGridDirty(bool val){
		elevGridDirty = val;
	}
	
	virtual const float* getSelectedPointLocal() {
		return selectPoint;
	}
	void setSelectedPointLocal(const float point[3]){
		for (int i = 0; i<3; i++) selectPoint[i] = point[i];
	}
	const float* getCursorCoords(){return cursorCoords;}
	void setCursorCoords(float x, float y){
		cursorCoords[0]=x; cursorCoords[1]=y;
	}
	void setOrientation(int val);
	int getOrientation() {return orientation;}
	bool linearInterpTex() {return linearInterp;}
	void setLinearInterp(bool interp){linearInterp = interp;}
	virtual void getTextureSize(int sze[2]) = 0;
	
	void getTwoDVoxelExtents(float voxdims[2]);
	
	float getLocalTwoDMin(int i) {return (float)(GetBox()->GetLocalExtents()[i]);}
	float getLocalTwoDMax(int i) {return (float)(GetBox()->GetLocalExtents()[i+3]);}
	
	void setLocalTwoDMin(int i, float val){
		double boxexts[6];
		GetBox()->GetLocalExtents(boxexts);
		boxexts[i] = val;
		GetBox()->SetLocalExtents(boxexts);
	}
	void setLocalTwoDMax(int i, float val){
		double boxexts[6];
		GetBox()->GetLocalExtents(boxexts);
		boxexts[i+3] = val;
		GetBox()->SetLocalExtents(boxexts);
	}
	
	void SetHeightVariableName(string varname){
		heightVariableName = varname;
	}
	const string& GetHeightVariableName() {return heightVariableName;}
	
	//Set all the cached twoD textures dirty, as well as elev grid
	virtual void setTwoDDirty()=0;
	int getMaxTimestep() {return maxTimestep;}
	//get/set methods
	virtual void SetRefinementLevel(int numtrans){numRefinements = numtrans; setTwoDDirty();}
	void setMaxNumRefinements(int numtrans) {maxNumRefinements = numtrans;}
	virtual int GetCompressionLevel() {return compressionLevel;}
	virtual void SetCompressionLevel(int val) {compressionLevel = val; setTwoDDirty();}

	virtual bool imageCrop()=0;

	float getRealImageWidth() {
		vector<double> box = GetBox()->GetLocalExtents();
		return (float)(box[3]-box[0]);
	}
	float getRealImageHeight() {
		vector<double> box = GetBox()->GetLocalExtents();
		return (float)(box[4]-box[1]);
	}
	
	
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride) = 0;
	virtual void restart() = 0;
	static void setDefaultPrefs();
	virtual ParamNode* buildNode()=0; 
	virtual bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/)=0;
	virtual bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/)=0;
	
	
	virtual const unsigned char* calcTwoDDataTexture(int timestep, int &wid, int &ht)= 0;

	virtual Box* GetBox(){return myBox;}
	//Get the bounding extents of twoD, in cube coords
	virtual void calcContainingStretchedBoxExtentsInCube(float* extents, bool rotated = false);
	
	virtual int GetRefinementLevel() {return numRefinements;}
	
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
	//Mapping [-1,1]X[-1,1] into local 3D volume coordinates.
	void buildLocal2DTransform(float a[2],float b[2], float* constVal, int mappedDims[3]);
protected:
	
	static const string _geometryTag;
	static const string _twoDMinAttr;
	static const string _twoDMaxAttr;
	static const string _cursorCoordsAttr;
	static const string _numTransformsAttr;
	static const string _terrainMapAttr;
	static const string _verticalDisplacementAttr;
	static const string _orientationAttr;
	static const string _heightVariableAttr;
	void refreshCtab();
			

	bool elevGridDirty;

	int maxTimestep;

	int orientation; //Only settable in image mode
	
	bool linearInterp;
	Box* myBox;
	int numRefinements, maxNumRefinements;
	int  _textureSizes[2]; //2 ints for each time step
	float selectPoint[3];
	float cursorCoords[2];
	float verticalDisplacement;
	bool mapToTerrain;
	float minTerrainHeight, maxTerrainHeight;
	int compressionLevel;
	string heightVariableName;
	
};
};
#endif //TWODPARAMS_H 
