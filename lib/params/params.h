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

class QWidget;
using namespace VetsUtil;
//Error tolerance for gui parameters:
#define ROUND_OFF 1.e-6f
#define MAXVIZWINS 16

namespace VAPoR{
//The dirty bits are kept in each GLWindow, 
//These are dirty flags that need to be communicated between different params;
//i.e. a change in one params forces a renderer to rebuild
enum DirtyBitType {
	ProbeTextureBit,
	RegionBit,//Set when the region bounds change
	DvrRegionBit,//Set when dvr needs to refresh its region data
	ColorscaleBit,
	NavigatingBit,//Set when the viewpoint changes
    LightingBit,
	AnimationBit //Set when the current frame number changes
};
class XmlNode;
class MapperFunction;

class PARAMS_API Params : public MyBase, public ParsedXml {
	
public: 
	

Params(
	XmlNode *parent, const string &name, int winNum
 );
Params(int winNum) {
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

 //! Set the parent node of the XmlNode tree.
 //!
 //! Sets a new parent node for the XmlNode tree parameter 
 //! data base. The parent node, \p parent, must have been
 //! previously initialized by passing it as an argument to
 //! the class constructor ParamBase(). This method permits
 //! wholesale changing of the entire parameter database.
 //!
 //! \param[in] parent Parent XmlNode.
 //
 void Params::SetParent(XmlNode *parent);


 //! Xml start tag parsing method
 //!
 //! This method is called to handle parsing of an XML file. The contents
 //! of the file will replace any current parameter settings. The method
 //! is virtual so that derived classes may receive notification when
 //! an object instance is reseting state from an XML file
 //
 virtual bool elementStartHandler(
	ExpatParseMgr* pm, int depth, string& tag, const char ** attribs
 );

 //! Xml end tag parsing method
 //!
 //! This method is called to handle parsing of an XML file. The contents
 //! of the file will replace any current parameter settings. The method
 //! is virtual so that derived classes may receive notification when
 //! an object instance has finished reseting state from an XML file
 //
 virtual bool elementEndHandler(ExpatParseMgr* pm, int depth, string& tag);

 //! Return the top (root) of the parameter node tree
 //!
 //! This method returns the top node in the parameter node tree
 //!
 ParamNode *GetRootNode() { return(_rootParamNode); }

	
	
	virtual bool isRenderParams() {return false;}
	enum ParamType {
		UnknownParamsType,
		ViewpointParamsType,
		RegionParamsType,
		DvrParamsType,
		IsoParamsType,
		ProbeParamsType,
		AnimationParamsType,
		FlowParamsType
	};
	
	static void	BailOut (const char *errstr, char *fname, int lineno);

	static QString& paramName(ParamType);
	static const string _dvrParamsTag;
	static const string _isoParamsTag;
	static const string _probeParamsTag;
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
	virtual void setBox(const float[3] /*boxMin[3]*/, const float /*boxMax*/[3]) {assert(0);}
	void setStretchedBox(const float[3], const float[3]);
	//Make a box by copying values to the arguments
	virtual void getBox(float /*boxMin*/[3], float /*boxMax*/[3]) {assert( 0);}

	void getStretchedBox(float boxmin[3], float boxmax[3]);
	//Box orientation:
	virtual float getPhi() {return 0.f;}
	virtual float getTheta() {return 0.f;}
	virtual float getPsi() {return 0.f;}
	
	//Determine the box extents in the unit cube.
	void calcStretchedBoxExtentsInCube(float* extents);
	//Extension that allows container of rotated box to be larger:
	virtual void calcContainingStretchedBoxExtentsInCube(float* extents) 
		{return calcStretchedBoxExtentsInCube(extents);}
	void calcStretchedBoxExtents(float* extents);
	void calcBoxExtents(float* extents);
	//Calculate the box in world coords, using any theta or phi
	void calcBoxCorners(float corners[8][3], float extraThickness, float rotation = 0.f, int axis = -1);
	//void calcStretchedBoxCorners(float corners[8][3], float extraThickness, float rotation = 0.f, int axis = -1);
	
	// Construct transformation as a mapping of [-1,1]^3 into volume array
	// coordinates at current resolution
	void buildCoordTransform(float transformMatrix[12], float extraThickness, float rotation = 0.f, int axis = -1);


	//The restart method goes back to initial state
	//Made pure virtual to coincide with ParamsBase functionality
	//
	virtual void restart() = 0;
	//Methods for restore/save:
	virtual XmlNode* buildNode() { return 0;}

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
	
	

	//inserted from J. Clyne's ParamsBase.h, so
	//that params class now supports ParamsBase functionality:
	ParamNode *_currentParamNode;
	ParamNode *_rootParamNode;

	int _parseDepth;


protected:

 //! Return the current node in the parameter node tree
 //!
 //! This method returns the current node in the parameter node tree. 
 //! \sa Push(), Pop()
 //!
 ParamNode *GetCurrentNode() { return(_currentParamNode); }

 //! Move down a level in the parameter tree
 //!
 //! The underlying storage model for parameter data is a hierarchical tree. 
 //! By default the hierarchy is flat. This method can be used to add
 //! and navigate branches of the tree. Invoking this method makes the branch
 //! named by \p name the current branch (node). If the branch \p name does not
 //! exist it will be created with the name provided. 
 //! Subsequent set and get methods will operate
 //! relative to the current branch.
 //!
 //! \param[in] name The name of the branch
 //! \retval node Returns the new current node
 //!
 //! \sa Pop(), Delete(), GetCurrentNode()
 //
 ParamNode *Push(const string &name);


 //! Move up one level in the paramter tree
 //!
 //! This method move back up the tree hierarchy by one level.
 //! Moving up past the root of the tree is prohibited and will silenty fail
 //! with no ill effects.
 //!
 //! \retval node Returns the new current node
 //!
 //! \sa Pop(), Delete()
 //
 ParamNode *Pop();

 //! Delete the named branch.
 //!
 //! This method deletes the named child, and all decendents, of the current 
 //! destroying it's contents in the process. The 
 //! named node must be a child of the current node. If the named node
 //! does not exit the result is a no-op.
 //!
 //! \param[in] name The name of the branch
 //
 void Remove(const string &name);

 //! Return the attributes associated with the current branch
 //!
 //! \retval a list of attributes
 //
 const map <string, string> &GetAttributes();


 //! Remove (undefine) all parameters
 //!
 //! This method deletes any and all paramters contained in the base 
 //! class as well as deleting any tree branches.
 //
 void Clear();


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
	
	
	virtual int getSessionVarNum(){ assert(0); return -1;}
	virtual float GetHistoStretch() { assert(0); return 1.f;}
	virtual void setBindButtons() {return;}//Needs to be removed!
	virtual bool getEditMode() {assert(0); return true;}
	virtual const float* getCurrentDatarange(){assert(0); return(0);}


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
	
	float getMinOpacMapBound();	
	float getMaxOpacMapBound(); 
	
	void setMinOpacEditBound(float val, int var) {
		minOpacEditBounds[var] = val;
	}
	void setMaxOpacEditBound(float val, int var) {
		maxOpacEditBounds[var] = val;
	}
	float getMinOpacEditBound(int var) {
		return minOpacEditBounds[var];
	}
	float getMaxOpacEditBound(int var) {
		return maxOpacEditBounds[var];
	}
	
	virtual MapperFunction* getMapperFunc()=0;
	virtual int getNumRefinements()=0;
	void setStopFlag(bool val = true) {stopFlag = val;}
	bool getStopFlag() {return stopFlag;}

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

	
};
}; //End namespace VAPoR
#endif //PARAMS_H 
