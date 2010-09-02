//************************************************************************
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
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
#include "vapor/ParamNode.h"
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
class ParamNode;
class MapperFunction;
class TransferFunction;
class ViewpointParams;
class RegionParams;

class PARAMS_API Params : public MyBase, public ParamsBase {
	
public: 
	
Params(
	XmlNode *parent, const string &name, int winNum
 );
Params(int winNum, const string& name) : ParamsBase(name) {
	vizNum = winNum;
	if(winNum < 0) local = false; else local = true;
	
	previousClass = 0;
}
	
//Default copy constructor
	Params(const Params& p);
 //! Destroy object
 //!
 //! \note This destructor does not delete children XmlNodes created
 //! as children of the \p parent constructor parameter.
 //!
 	virtual ~Params();

//! Pure virtual method specifying name to display on associated tab
//! \retval string name to identify associated tab
	 virtual const std::string& getShortName()=0;

//! Static method that identifies the instance that is current in the identified window
//! \param[in] pType ParamsBase TypeID of the params class
//! \param[in] winnum index of identified window
//! \retval instance index that is current
	static int GetCurrentParamsInstanceIndex(int pType, int winnum){
		return currentParamsInstance[make_pair(pType,winnum)];
	}
//! Static method that identifies the instance that is current in the identified window
//! Uses tag to identify Params class
//! \param[in] tag Tag (name) of the params class
//! \param[in] winnum index of identified window
//! \retval instance index that is current
	static int GetCurrentParamsInstanceIndex(const std::string tag, int winnum){
		return GetCurrentParamsInstanceIndex(GetTypeFromTag(tag),winnum);
	}
	 
//! Static method that specifies the instance that is current in the identified window
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of identified window
//! \param[in] instance index of instance to be made current
	static void SetCurrentParamsInstanceIndex(int pType, int winnum, int instance){
		currentParamsInstance[make_pair(pType,winnum)] = instance;
	}
	
//! Static method that finds the Params instance 
//! if \p instance is -1, the current instance is found
//! if \p winnum is -1, the default instance is found
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of  window
//! \param[in] instance index 
	static Params* GetParamsInstance(int pType, int winnum = -1, int instance = -1);

//! Static method that finds the Params instance based on tag 
//! if \p instance is -1, the current instance is found
//! if \p winnum is -1, the default instance is found
//! \param[in] tag XML Tag (name) of the params class
//! \param[in] winnum index of  window
//! \param[in] instance index 
	static Params* GetParamsInstance(const std::string tag, int winnum = -1, int instance = -1){
		return GetParamsInstance(GetTypeFromTag(tag), winnum, instance);
	}

//! Static method that returns the instance that is current in the identified window
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of identified window
//! \retval Pointer to specified Params instance
	static Params* GetCurrentParamsInstance(int pType, int winnum){
		Params* p = GetParamsInstance(pType, winnum, -1);
		if (p->isLocal()) return p;
		return GetDefaultParams(pType);
	}
	
//! Static method that returns the default Params instance
//! With non-render params this is the global Params instance
//! \param[in] pType ParamsBase TypeId of the params class
//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(ParamsBase::ParamsBaseType pType){
		return defaultParamsInstance[pType];
	}

//! Static method that returns the default Params instance
//! Based on XML tag (name) of Params class
//! With non-render params this is the global Params instance
//! \param[in] tag XML tag of the Params class
//! \retval Pointer to specified Params instance
	static Params* GetDefaultParams(const string& tag){
		return defaultParamsInstance[GetTypeFromTag(tag)];
	}

//! Static method that specifies the default Params instance
//! With non-render params this is the global Params instance
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] p Pointer to default Params instance
	static void SetDefaultParams(int pType, Params* p) {
		defaultParamsInstance[pType] = p;
	}

//! Static method that specifies the default Params instance
//! Based on Xml Tag of Params class
//! With non-render params this is the global Params instance
//! \param[in] tag XML Tag of the params class
//! \param[in] p Pointer to default Params instance
	static void SetDefaultParams(const string& tag, Params* p) {
		defaultParamsInstance[GetTypeFromTag(tag)] = p;
	}
//! Static method that constructs a default Params instance
//! \param[in] pType ParamsBase TypeId of the params class
//! \retval Pointer to new default Params instance
	static Params* CreateDefaultParams(int pType){
		Params*p = (Params*)(createDefaultFcnMap[pType])();
		return p;
	}

//! Static method that finds how many instances of a Params class
//! exist for a particular visualizer
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \retval number of instances that exist 
	static int GetNumParamsInstances(int pType, int winnum){
		return paramsInstances[make_pair(pType, winnum)].size();
	}

//! Static method that finds how many instances of a Params class
//! exist for a particular visualizer
//! Based on the XML tag of the Params class.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \retval number of instances that exist 
	static int GetNumParamsInstances(const std::string tag, int winnum){
		return GetNumParamsInstances(GetTypeFromTag(tag), winnum);
	}
	
//! Static method that appends a new instance to the list of existing 
//! Params instances for a particular visualizer
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] p pointer to Params instance being appended 
	static void AppendParamsInstance(int pType, int winnum, Params* p){
		paramsInstances[make_pair(pType,winnum)].push_back(p);
	}

//! Static method that appends a new instance to the list of existing 
//! Params instances for a particular visualizer
//! Based on the XML tag of the Params class.
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] p pointer to Params instance being appended 
	static void AppendParamsInstance(const std::string tag, int winnum, Params* p){
		AppendParamsInstance(GetTypeFromTag(tag),winnum, p);
	}

//! Static method that removes an instance from the list of existing 
//! Params instances for a particular visualizer
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] instance index of instance to remove 
	static void RemoveParamsInstance(int pType, int winnum, int instance);
	
//! Static method that inserts a new instance into the list of existing 
//! Params instances for a particular visualizer
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \param[in] posn index where new instance will be inserted 
//! \param[in] dp pointer to Params instance being appended 
	static void InsertParamsInstance(int pType, int winnum, int posn, Params* dp){
		vector<Params*>& instances = paramsInstances[make_pair(pType,winnum)];
		instances.insert(instances.begin()+posn, dp);
	}

//! Static method that produces a list of all the Params instances 
//! for a particular visualizer
//! \param[in] pType ParamsBase TypeId of the params class
//! \param[in] winnum index of specified visualizer window
//! \retval vector of the Params pointers associated with the window 
	static vector<Params*>& GetAllParamsInstances(int pType, int winnum){
		return paramsInstances[make_pair(pType,winnum)];
	}

//! Static method that produces a list of all the Params instances 
//! for a particular visualizer
//! based on the XML Tag of the Params class
//! \param[in] tag XML tag associated with Params class
//! \param[in] winnum index of specified visualizer window
//! \retval vector of the Params pointers associated with the window 
	static vector<Params*>& GetAllParamsInstances(const std::string tag, int winnum){
		return GetAllParamsInstances(GetTypeFromTag(tag),winnum);
	}

//! Static method that produces clones of all the Params instances 
//! for a particular visualizer
//! \param[in] winnum index of specified visualizer window
//! \retval std::map mapping from Params typeIDs to std::vectors of Params pointers associated with the window 
	static map <int, vector<Params*> >* cloneAllParamsInstances(int winnum);

//! Static method that produces clones of all the default Params instances 
//! for a particular visualizer
//! \param[in] winnum index of specified visualizer window
//! \retval std::vector of default Params pointers associated with the window, indexed by ParamsBase TypeIDs 
	static vector <Params*>* cloneAllDefaultParams();

//! Static method that tells whether or not any renderer is enabled in a visualizer 
//! \param[in] winnum index of specified visualizer window
//! \retval True if any renderer is enabled 
	static bool IsRenderingEnabled(int winnum);
	
//! Virtual method indicating whether a params is a render params instance
//! \retval returns true if it is a render params
	virtual bool isRenderParams() {return false;}
	
	static void	BailOut (const char *errstr, const char *fname, int lineno);

	static const std::string& paramName(ParamsBaseType t);
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
	static const string _CompressionLevelTag;
	static const string _vizNumAttr;
	static const string _localAttr;
	static const string _numVariablesAttr;
	static const string _variableTag;
	static const string _variablesTag;
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
//! Pure virtual method that clones a Params instance.
//! Derived from ParamsBase.  With Params instances, the argument is ignored.
	virtual Params* deepCopy(ParamNode* nd = 0);

//! Pure virtual method, sets a Params instance to its default state
	virtual void restart() = 0;
	
//! Identify the visualizer associated with this instance.
//! With global params this is -1 
	int getVizNum() {return vizNum;}

//! Specify whether a params is local or global. 
	virtual void setLocal(bool lg){ if (lg) {local = true; assert (vizNum != -1);}
		else local = false;}

//! Indicate whether a Params is local or not
	bool isLocal() {return local;}
	
//! Specify the visualizer index of a Params instance.  
	void setVizNum(int vnum){vizNum = vnum;}
	
//! Virtual method to set up the Params to deal with new metadata.
//! When a new metadata is read, all params are notified 
//! If the params have state that depends on the metadata (e.g. region size,
//! variable, etc., they should implement reinit() to respond.
//! Default does nothing.
//
	virtual bool reinit(bool) {return false;}

	//Following deprecated methods are redefined by params that control a box (region), such
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
	
	// Construct transformation as a mapping of [-1,1]^3 into volume array
	// coordinates at current resolution
	void buildCoordTransform(float transformMatrix[12], float extraThickness, int timestep, float rotation = 0.f, int axis = -1);


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
	
	
	//Params instances are vectors of Params*, one per instance, indexed by paramsBaseType, winNum
	static map<pair<int,int>,vector<Params*> > paramsInstances;
	//CurrentRenderParams indexed by paramsBaseType, winNum
	static map<pair<int,int>, int> currentParamsInstance;
	//default params instances indexed by paramsBaseType
	static map<int, Params*> defaultParamsInstance;

};

//Subclass for params that control rendering.
class PARAMS_API RenderParams : public Params {
public: 
	RenderParams(int winNum, const string& name) : Params(winNum, name) {
		
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
	
	virtual Params* deepCopy(ParamNode* nd = 0);
	virtual bool isRenderParams() {return true;}

	//this does nothing for renderParams
	virtual void setLocal(bool ){ assert(0);}
		
	bool isEnabled(){return enabled;}
	virtual void setEnabled(bool value) {enabled = value; stopFlag = false;}
	
	virtual int getNumRefinements()=0;
	virtual int GetCompressionLevel()=0;
	virtual void SetCompressionLevel(int val)=0;
	virtual int getSessionVarNum() = 0;
	virtual float GetHistoStretch() { assert(0); return 1.f;}
	virtual void setBindButtons() {return;}//Needs to be removed!
	virtual bool getEditMode() {assert(0); return true;}
	virtual const float* getCurrentDatarange(){assert(0); return(0);}


	virtual void hookupTF(TransferFunction* , int ) {assert(0);}

	//Default implementation finds distance to region box:
	virtual float getCameraDistance(ViewpointParams* vpp, RegionParams* rp, int timestep);
	virtual bool isOpaque() { assert(0); return true;}
	//The following must be redefined by renderer params.  Parent version should never happen
	virtual void setMinColorMapBound(float ) =0;
	virtual void setMaxColorMapBound(float )=0;
	virtual void setMinOpacMapBound(float )=0;
	virtual void setMaxOpacMapBound(float )=0;


	float getMinColorMapBound();	
	float getMaxColorMapBound(); 
	
	virtual void setMinColorEditBound(float val, int var) {
		minColorEditBounds[var] = val;
	}
	virtual void setMaxColorEditBound(float val, int var) {
		maxColorEditBounds[var] = val;
	}
	virtual float getMinColorEditBound(int var) {
		return minColorEditBounds[var];
	}
	virtual float getMaxColorEditBound(int var) {
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
