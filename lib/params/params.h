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
//	Description:	Defines the Params class.
//		This is an abstract class for all the tabbed panel params classes.
//		Supports functionality common to all the tabbed panel params.
//
#ifndef PARAMS_H
#define PARAMS_H

#include <qwidget.h>
#include "assert.h"
#include "vapor/ExpatParseMgr.h"
#include "vaporinternal/common.h"

class QWidget;
using namespace VetsUtil;
//Error tolerance for gui parameters:
#define ROUND_OFF 1.e-6f
#define MAXVIZWINS 16

namespace VAPoR{
//The dirty bits are kept in each GLWindow, one for each type:
enum DirtyBitType {
	DvrClutBit,
	ProbeTextureBit,
	DvrDatarangeBit,
	RegionBit,
	ColorscaleBit,
	NavigatingBit,
	FlowDataBit,
	FlowGraphicsBit,
    LightingBit
};
class XmlNode;
class MapperFunction;
class MapEditor;

class PARAMS_API Params : public ParsedXml, public MyBase  {
	
public: 
	Params(int winNum) {
		vizNum = winNum;
		if(winNum < 0) local = false; else local = true;
		textChangedFlag = false;
		thisParamType = UnknownParamsType;
		previousClass = 0;
		minColorEditBounds = 0;
		maxColorEditBounds = 0;
		minOpacEditBounds = 0;
		maxOpacEditBounds = 0;
		
	}
	virtual ~Params(){
		if (minColorEditBounds) delete minColorEditBounds;
		if (maxColorEditBounds) delete maxColorEditBounds;
		if (minOpacEditBounds) delete minOpacEditBounds;
		if (maxOpacEditBounds) delete maxOpacEditBounds;

	}
	
	enum ParamType {
		UnknownParamsType,
		ViewpointParamsType,
		RegionParamsType,
		
		DvrParamsType,
		ProbeParamsType,
		
		AnimationParamsType,
		FlowParamsType
	};
	
	
	static QString& paramName(ParamType);
	static const string _dvrParamsTag;
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

	
	//Each params must be able to make a "deep" copy,
	//I.e. copy everything that is unique to this object
	//
	virtual Params* deepCopy() = 0;
	

	int getVizNum() {return vizNum;}
	void setLocal(bool lg){ if (lg) {local = true; assert (vizNum != -1);}
		else local = false;}
	bool isLocal() {return local;}
	bool isEnabled(){return enabled;}
	virtual void setEnabled(bool value) {enabled = value;}
	
	void setVizNum(int vnum){vizNum = vnum;}
	//Methods to be called from gui, get undo/redo support.
	//Need to capture two different params objects (not just two times of the same object)
	//Following is virtual so child classes can attach additional behavior.
	//They should always perform params::guiSetLocal() as well as their own behavior
	//
	//virtual void guiSetLocal(bool lg);
	//Dirty bit comes on when any text changes in the panel.
	//Comes off as soon as it gets confirmed.
	//
	void guiSetTextChanged(bool on) {textChangedFlag = on;}
	//When enter is pressed over a text box, the event is recorded and the
	//params are updated, if anything has changed.
	//
	//void confirmText(bool render);
	//When a new metadata is read, all params are notified 
	//If the params have state that depends on the metadata (e.g. region size,
	//variable, etc., they should implement reinit() to respond.
	//Default does nothing.
	//
	virtual bool reinit(bool) {return false;}

	
	virtual void connectMapperFunction(MapperFunction* , MapEditor* ){assert(0);}
	

	virtual int getVarNum(){ assert(0); return -1;}
	virtual float getHistoStretch() { assert(0); return 1.f;}
	virtual void setBindButtons() {return;}//Needs to be removed!
	virtual bool getEditMode() {assert(0); return true;}
	virtual const float* getCurrentDatarange(){assert(0); return(0);}
	virtual void setCurrentDatarange(){assert(0);}

	//The following must be redefined by renderer params.  Parent version should never happen
	virtual void setMinColorMapBound(float ) {assert(0);}
	virtual void setMaxColorMapBound(float ){assert(0);}
	virtual void setMinOpacMapBound(float ){assert(0);}
	virtual void setMaxOpacMapBound(float ){assert(0);}


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
	virtual void sessionLoadTF(QString* ) {return;}
	virtual MapperFunction* getMapperFunc(){return 0;}
	//Send params to the active renderer(s) that depend on these
	//panel settings, tell renderers that the values must be updated
	//Default method does nothing.  
	//Arguments are "wasEnabled, wasLocal, newWindow"
	// identify if previous panel was enabled (for renderer panels)
	//was local, and whether this is the first rendering in a window
	//
	virtual void updateRenderer(bool, bool, bool) {return;}
	
	//Following methods are redefined by params that control a box (region), such
	//as regionParams, probeParams, flowParams:
	//Set the box by copying the arrays provided as arguments.
	virtual void setBox(const float[3] /*boxMin[3]*/, const float /*boxMax*/[3]) {assert(0);}
	//Make a box by copying values to the arguments
	virtual void getBox(float /*boxMin*/[3], float /*boxMax*/[3]) {assert( 0);}
	//Box orientation:
	virtual float getPhi() {return 0.f;}
	virtual float getTheta() {return 0.f;}
	
	//Determine the box extents in the unit cube.
	void calcBoxExtentsInCube(float* extents);
	//Extension that allows container of rotated box to be larger:
	virtual void calcContainingBoxExtentsInCube(float* extents) 
		{return calcBoxExtentsInCube(extents);}
	void calcBoxExtents(float* extents);
	//Calculate the box in world coords, using any theta or phi
	void calcBoxCorners(float corners[8][3], float extraThickness = 0.f);
	// Construct transformation as a mapping of [-1,1]^3 into volume array
	// coordinates at current resolution
	void buildCoordTransform(float transformMatrix[12], float extraThickness = 0.f);

	virtual int getNumRefinements(){ assert(0); return -1;}


	//The restart method goes back to initial state
	//Default does nothing.
	//
	virtual void restart() {return;}
	//Methods for restore/save:
	virtual XmlNode* buildNode() { return 0;}
	virtual bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/) = 0;
	virtual bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/) = 0;
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

protected:
	
	//The Params is global if either the params object does not exist, OR if
	//it exists and local = false.
	//
	bool local;
	// The enabled flag is only used by renderer params
	//
	bool enabled;
	//Keep track of which window number corresp to this.  -1 for global parameters.
	//
	int vizNum;
	
	bool textChangedFlag;
	ParamType thisParamType;

	//Needed for renderer params:
	float* minColorEditBounds;
	float* maxColorEditBounds;
	float* minOpacEditBounds;
	float* maxOpacEditBounds;


	
};
};
#endif //PARAMS_H 
