//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		params.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Defines the Params and the RendererParams classes.
//		This is an abstract class for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#ifndef PARAMS_H
#define PARAMS_H

#include "vapor/XmlNode.h"
#include "vapor/ExpatParseMgr.h"
#include "ParamNode.h"
#include <qwidget.h>
#include "assert.h"
#include "vaporinternal/common.h"
#include "vapor/ParamsBase.h"

class QWidget;
using namespace VetsUtil;
//Error tolerance for gui parameters:
#define ROUND_OFF 1.e-6f
#define MAXVIZWINS 16
#define OUT_OF_BOUNDS -1.e30f

namespace VAPoR{
//The dirty bits are kept in each GLWindow, 
//These are dirty flags that need to be communicated between different params;
//i.e. a change in one params forces a renderer to rebuild
enum DirtyBitType {
	ProbeTextureBit,
	TwoDTextureBit,
	RegionBit,//Set when the region bounds change
	DvrRegionBit,//Set when dvr needs to refresh its region data
	ColorscaleBit,
    LightingBit,
	ProjMatrixBit,
	ViewportBit,
	AnimationBit //Set when the current frame number changes
};
class XmlNode;
class MapperFunction;
class TransferFunction;

class PARAMS_API Params : public MyBase, public ParamsBase {
	
public: 
	

Params(
	XmlNode *parent, const string &name, int winNum
 );
Params(int winNum) : ParamsBase() {
	vizNum = winNum;
	if(winNum < 0) local = false; else local = true;
	thisParamType = UnknownParamsType;
	previousClass = 0;
}
	
 //! Destroy object
 //!
 //! \note This destructor does not delete children XmlNodes created
 //! as children of the \p parent constructor parameter.
 //!
 virtual ~Params();


	virtual bool isRenderParams() {return false;}
	enum ParamType {
		UnknownParamsType = 0,
		ViewpointParamsType = 1,
		RegionParamsType = 2,
		DvrParamsType = 4,
		IsoParamsType = 8,
		ProbeParamsType = 16,
		TwoDDataParamsType = 32,
		AnimationParamsType = 64,
		FlowParamsType = 128,
		TwoDImageParamsType = 256
	};
	
	static void	BailOut (const char *errstr, const char *fname, int lineno);

	static QString& paramName(ParamType);
	static const string _dvrParamsTag;
	static const string _isoParamsTag;
	static const string _probeParamsTag;
	static const string _twoDParamsTag;
	static const string _twoDDataParamsTag;
	static const string _twoDImageParamsTag;
	static const string _regionParamsTag;
	static const string _viewpointParamsTag;
	static const string _animationParamsTag;
	static const string _flowParamsTag;
	static const string _vizNumAttr;
	static const string _localAttr;
	static const string _numVariablesAttr;
	static const string _variableTag;
	static const string _leftEditBoundAttr;
	static const string _rightEditBoundAttr;
	static const string _variableNumAttr;
	static const string _variableNameAttr;
	static const string _opacityScaleAttr;
	static const string _numTransformsAttr;
	static const string _useTimestepSampleListAttr;
	static const string _timestepSampleListAttr;

	
	//Each params must be able to make a "deep" copy,
	//I.e. copy everything that is unique to this object
	//
	virtual Params* deepCopy() = 0;
	

	int getVizNum() {return vizNum;}
	virtual void setLocal(bool lg){ if (lg) {local = true; assert (vizNum != -1);}
		else local = false;}
	bool isLocal() {return local;}
	
	void setVizNum(int vnum){vizNum = vnum;}
	
	//When a new metadata is read, all params are notified 
	//If the params have state that depends on the metadata (e.g. region size,
	//variable, etc., they should implement reinit() to respond.
	//Default does nothing.
	//
	virtual bool reinit(bool) {return false;}

	//Following methods are redefined by params that control a box (region), such
	//as regionParams, probeParams, flowParams:
	//Set the box by copying the arrays provided as arguments.
	virtual void setBox(const float[3] /*boxMin[3]*/, const float /*boxMax*/[3], int /*timestep*/) {assert(0);}
	void setStretchedBox(const float[3], const float[3], int);
	//Make a box by copying values to the arguments
	virtual void getBox(float /*boxMin*/[3], float /*boxMax*/[3], int /*timestep*/) {assert( 0);}

	void getStretchedBox(float boxmin[3], float boxmax[3], int timestep);
	//Box orientation:
	virtual float getPhi() {return 0.f;}
	virtual float getTheta() {return 0.f;}
	virtual float getPsi() {return 0.f;}
	
	//Determine the box extents in the unit cube.
	void calcStretchedBoxExtentsInCube(float* extents, int timestep);
	//Extension that allows container of rotated box to be larger.
	//Not used by region params
	virtual void calcContainingStretchedBoxExtentsInCube(float* extents) 
		{return calcStretchedBoxExtentsInCube(extents, -1);}
	void calcStretchedBoxExtents(float* extents, int timestep);
	void calcBoxExtents(float* extents, int timestep);
	//Calculate the box in world coords, using any theta or phi
	virtual void calcBoxCorners(float corners[8][3], float extraThickness, int timestep, float rotation = 0.f, int axis = -1);
	//void calcStretchedBoxCorners(float corners[8][3], float extraThickness, float rotation = 0.f, int axis = -1);
	
	// Construct transformation as a mapping of [-1,1]^3 into volume array
	// coordinates at current resolution
	void buildCoordTransform(float transformMatrix[12], float extraThickness, int timestep, float rotation = 0.f, int axis = -1);


	
	
	//Identify if this params is at the front of the tabbed params
	//bool isCurrent(); 
	ParamType getParamType() {return thisParamType;}

	//Helper function for testing distances,
	//testing whether a point is inside or outside of cube 
	//Return is positive if point is outside.  Arguments are outward pointing
	//unit normal vectors and associated points (corners) of probe faces.
	static float distanceToCube(const float point[3],const float faceNormals[6][3], const float faceCorners[6][3]);

	
    //
    // Parse command line arguements 
    //
    static bool searchCmdLine(const char *flag);
    static const char* parseCmdLine(const char *flag);

	//Determine a new value of theta and phi when the probe is rotated around either the
	//x-, y-, or z- axis.  axis is 0,1,or 1. rotation is in degrees.
	void convertThetaPhiPsi(float *newTheta, float* newPhi, float* newPsi, int axis, float rotation);
	
	
protected:
	bool local;
	
	//Keep track of which window number corresp to this.  -1 for global or default parameters.
	//
	int vizNum;
	ParamType thisParamType;

};

//Subclass for params that control rendering.
class PARAMS_API RenderParams : public Params {
public: 
	RenderParams(int winNum) : Params(winNum) {
		
		local = true;
		
		previousClass = 0;
		minColorEditBounds = 0;
		maxColorEditBounds = 0;
		minOpacEditBounds = 0;
		maxOpacEditBounds = 0;
		
	}
	RenderParams(XmlNode *parent, const string &name, int winnum); 
	virtual ~RenderParams(){
		if (minColorEditBounds) delete minColorEditBounds;
		if (maxColorEditBounds) delete maxColorEditBounds;
		if (minOpacEditBounds) delete minOpacEditBounds;
		if (maxOpacEditBounds) delete maxOpacEditBounds;

	}
	virtual RenderParams* deepRCopy() = 0;
	virtual bool isRenderParams() {return true;}

	//this does nothing for renderParams
	virtual void setLocal(bool ){ assert(0);}
		
	bool isEnabled(){return enabled;}
	virtual void setEnabled(bool value) {enabled = value; stopFlag = false;}
	
	
	virtual int getSessionVarNum() = 0;
	virtual float GetHistoStretch() { assert(0); return 1.f;}
	virtual void setBindButtons() {return;}//Needs to be removed!
	virtual bool getEditMode() {assert(0); return true;}
	virtual const float* getCurrentDatarange(){assert(0); return(0);}


	virtual void hookupTF(TransferFunction* , int ) {assert(0);}
	//The following must be redefined by renderer params.  Parent version should never happen
	virtual void setMinColorMapBound(float ) =0;
	virtual void setMaxColorMapBound(float )=0;
	virtual void setMinOpacMapBound(float )=0;
	virtual void setMaxOpacMapBound(float )=0;


	float getMinColorMapBound();	
	float getMaxColorMapBound(); 
	
	void setMinColorEditBound(float val, int var) {
		minColorEditBounds[var] = val;
	}
	void setMaxColorEditBound(float val, int var) {
		maxColorEditBounds[var] = val;
	}
	float getMinColorEditBound(int var) {
		return minColorEditBounds[var];
	}
	float getMaxColorEditBound(int var) {
		return maxColorEditBounds[var];
	}
	//And opacity:
	
	virtual float getMinOpacMapBound();	
	virtual float getMaxOpacMapBound(); 
	

	virtual void setMinOpacEditBound(float val, int var) {
		minOpacEditBounds[var] = val;
	}
	virtual void setMaxOpacEditBound(float val, int var) {
		maxOpacEditBounds[var] = val;
	}
	virtual float getMinOpacEditBound(int var) {
		return minOpacEditBounds[var];
	}
	virtual float getMaxOpacEditBound(int var) {
		return maxOpacEditBounds[var];
	}

	
	virtual MapperFunction* getMapperFunc()=0;
	virtual int getNumRefinements()=0;
	void setStopFlag(bool val = true) {stopFlag = val;}
	bool getStopFlag() {return stopFlag;}
	//Bypass flag is used to indicate a renderer should
	//not render until its state is changed.
	//Partial bypass is only used by DVR, to 
	//indicate a renderer that should be bypassed at
	//full resolution but not at interactive resolution.
	void setBypass(int i) {bypassFlags[i] = 2;}
	void setPartialBypass(int i) {bypassFlags[i] = 1;}
	void setAllBypass(bool val);
	int getBypassValue(int i) {return bypassFlags[i];} //only used for debugging
	bool doBypass(int ts) {return ((ts < bypassFlags.size()) && bypassFlags[ts]);}
	bool doAlwaysBypass(int ts) {return ((ts < bypassFlags.size()) && bypassFlags[ts]>1);}
	void initializeBypassFlags();
	//Get a variable region from the datamanager
	float* getContainingVolume(size_t blkMin[3], size_t blkMax[3], int refinements, int varNum, int timeStep, bool twoDim);

protected:
	
	// The enabled flag is only used by renderer params
	//
	bool enabled;
	bool stopFlag;  //Indicates if user asked to stop (only with flow calc now)
	//Needed for renderer params:
	float* minColorEditBounds;
	float* maxColorEditBounds;
	float* minOpacEditBounds;
	float* maxOpacEditBounds;

	vector<int> bypassFlags;
};
}; //End namespace VAPoR
#endif //PARAMS_H 
